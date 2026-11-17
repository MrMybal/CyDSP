# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Validate streamed native HRTF impulses and measure complete source rendering."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import struct
import subprocess
import uuid
import h5py
import numpy as np
import torch
from export_hrtf_native import export_model
from train_hrtf_pilot import ROOT, CORPUS, estimate_itd_us, sha256, write_json, compensated_delays_us
from benchmark_hrtf_native import DEFAULT_CMAKE, f32bytes
from benchmark_hrtf_inference import cpu_name


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', type=Path, required=True)
    parser.add_argument('--library', type=Path, default=ROOT/'build/windows-x64/Release/CyDSP.lib')
    parser.add_argument('--cmake', type=Path, default=DEFAULT_CMAKE)
    parser.add_argument('--backend', choices=('automatic','scalar','sse2','avx2','avx512'),default='automatic')
    args = parser.parse_args()
    torch.set_num_threads(1)
    torch.set_num_interop_threads(1)
    out = ROOT/'build/hrtf-native-rendering'/(datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8])
    out.mkdir(parents=True)
    model, meta, manifest = export_model(args.run, out/'model.cyhrtf')
    rate = model.sample_rate
    original = (CORPUS/meta['source']['file']).resolve()
    if not original.is_relative_to(CORPUS.resolve()) or sha256(original) != meta['source']['sha256']:
        raise ValueError('SOFA source provenance mismatch')
    with h5py.File(original,'r') as sofa:
        position = np.asarray(sofa['SourcePosition'])
        az,el = np.deg2rad(position[:,0]),np.deg2rad(position[:,1])
        xyz = np.column_stack((np.cos(el)*np.cos(az),np.cos(el)*np.sin(az),np.sin(el))).astype('float32')
        ears = np.argsort(-np.asarray(sofa['ReceiverPosition']).reshape(2,3)[:,1])
        impulse = np.asarray(sofa['Data.IR'])[:,ears]
        delays = np.broadcast_to(np.asarray(sofa['Data.Delay']).reshape(-1,2)[:,ears],(len(xyz),2))
    selected = np.load(args.run/'split.npz')['test']
    data = bytearray(b'CYHRQA02'+struct.pack('<3I',len(xyz),impulse.shape[-1],len(selected)))
    data.extend(f32bytes(xyz));data.extend(f32bytes(impulse))
    data.extend(np.asarray(delays,dtype='<f8').tobytes());data.extend(np.asarray(selected,dtype='<u4').tobytes())
    (out/'corpus.bin').write_bytes(data)
    source = ROOT/'scripts/native/HrtfRendering.cpp'
    directory=out/'cmake-source';directory.mkdir()
    (directory/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.24)
project(CyDSPHrtfRendering LANGUAGES CXX)
add_executable(hrtf-rendering "{source.as_posix()}")
target_compile_features(hrtf-rendering PRIVATE cxx_std_20)
target_include_directories(hrtf-rendering PRIVATE "{(ROOT/'include').as_posix()}")
target_link_libraries(hrtf-rendering PRIVATE "{args.library.resolve().as_posix()}")
if(MSVC)
 target_compile_options(hrtf-rendering PRIVATE /W4 /fp:precise)
endif()
''',encoding='utf-8')
    subprocess.run([str(args.cmake),'-S',str(directory),'-B',str(out/'cmake-build'),'-A','x64'],check=True)
    subprocess.run([str(args.cmake),'--build',str(out/'cmake-build'),'--config','Release','--parallel','8'],check=True)
    exe=out/'cmake-build/Release/hrtf-rendering.exe'
    result=subprocess.run([str(exe),str(out/'model.cyhrtf'),str(out/'corpus.bin'),str(out/'audio.bin'),args.backend],check=True,capture_output=True,text=True)
    benchmark=json.loads(result.stdout)
    write_json(out/'benchmark.json',benchmark)
    with (out/'audio.bin').open('rb') as stream:
        count,frames,filters=struct.unpack('<3I',stream.read(12))
        if count!=len(selected) or filters!=model.filters or frames!=(8192 if rate<=48000 else 16384 if rate<=96000 else 32768):
            raise ValueError('Native impulse output shape')
        values=[];rendered=[]
        for _ in selected:
            values.append(np.frombuffer(stream.read((2*(filters*3+1)+4)*8),dtype='<f8'))
            rendered.append(np.frombuffer(stream.read(2*frames*4),dtype='<f4').reshape(2,frames))
        if stream.read(1):raise ValueError('Trailing output')
    values=np.stack(values);rendered=np.stack(rendered)
    if not np.isfinite(values).all() or not np.isfinite(rendered).all():raise AssertionError('Nonfinite native signal')
    with torch.inference_mode():
        directions=torch.as_tensor(xyz[selected])
        directions=directions/directions.norm(dim=-1,keepdim=True)
        predicted=model(directions)
        c,q,g,level,itd=[t.numpy() for t in predicted]
        reference=np.concatenate((np.concatenate((np.stack((c,q,g),axis=-1).reshape(count,2,-1),
                                                   (level+meta['magnitudeOffsetDb'])[...,None]),axis=-1).reshape(count,-1),
                                  itd[:,None]),axis=-1)
        _,_,intrinsic=compensated_delays_us(predicted,rate)
    physical_error=np.abs(values[:,:reference.shape[1]]-reference)
    intrinsic_error=float(np.abs(values[:,-3]-intrinsic.numpy()).max())
    if intrinsic_error>.05:raise AssertionError('Native intrinsic ITD oracle disagreement')
    fft_native=np.fft.rfft(rendered,n=frames,axis=-1)
    fft_source=np.fft.rfft(impulse[selected],n=frames,axis=-1)
    hz=np.fft.rfftfreq(frames,1/rate)
    native_db=20*np.log10(np.maximum(abs(fft_native),1e-7))
    source_db=20*np.log10(np.maximum(abs(fft_source),1e-7))
    oracle_bins=np.flatnonzero((hz>=200)&(hz<=20000))[::8]
    with torch.inference_mode():
        oracle,_=model.response(tuple(t[:32] for t in predicted),torch.as_tensor(hz[oracle_bins],dtype=torch.float32))
    oracle_difference=np.abs(native_db[:32,:,oracle_bins]-(oracle.numpy()+meta['magnitudeOffsetDb']))
    numerical_max=float(oracle_difference.max())
    if numerical_max>.005:raise AssertionError(f'Rendered impulse differs from model: {numerical_max} dB')
    grids={'log':np.geomspace(200,20000,256),'linear':np.linspace(200,20000,257)}
    errors={}
    for name,grid in grids.items():
        actual=np.stack([np.stack([np.interp(grid,hz,ear) for ear in row]) for row in native_db])
        target=np.stack([np.stack([np.interp(grid,hz,ear) for ear in row]) for row in source_db])
        diff=actual-target
        errors[name]={'magnitudeRmseDb':float(np.sqrt(np.mean(diff**2))),
                      'directionRmseP95Db':float(np.percentile(np.sqrt(np.mean(diff**2,axis=(1,2))),95)),
                      'ildRmseDb':float(np.sqrt(np.mean((diff[:,0]-diff[:,1])**2)))}
    actual_itd=estimate_itd_us(fft_native[...,::frames//4096],np.fft.rfftfreq(4096,1/rate),np.zeros((count,2)),rate)
    source_itd=estimate_itd_us(fft_source[...,::frames//4096],np.fft.rfftfreq(4096,1/rate),delays[selected],rate)
    error_itd=actual_itd-source_itd
    itd_metrics={'rmseUs':float(np.sqrt(np.mean(error_itd**2))),
                 'absoluteP95Us':float(np.percentile(abs(error_itd),95)),
                 'absoluteMaxUs':float(abs(error_itd).max()),
                 'rendererVsHeadMaxUs':float(abs(actual_itd-values[:,-4]).max())}
    if itd_metrics['rendererVsHeadMaxUs']>1:raise AssertionError('Fractional delay introduces unexpected low-band ITD error')
    report={'status':'passed','sampleRate':rate,'testDirections':count,'renderedFramesPerDirection':frames,
            'cpu':cpu_name(),'backendRequested':args.backend,'model':manifest,'nativeLibrarySha256':sha256(args.library),
            'nativeExecutableSha256':sha256(exe),'nativeSourceSha256':sha256(source),
            'renderedSignalVsTheoreticalMaxErrorDb':numerical_max,'intrinsicItdOracleMaxErrorUs':intrinsic_error,
            'nativePhysicalParameterMaxError':float(physical_error.max()),
            'sourceComparison':errors,'streamedItd':itd_metrics,'benchmark':benchmark,
            'scope':['Original D2 KEMAR known listener, 881 directional test points; not unseen listeners',
                     'Magnitude and low-band ITD approximation, no measured full-phase equivalence',
                     f'Measured renderer: all {len(xyz)} points, original {impulse.shape[-1]} taps, exact directions, onset detection disabled',
                     'Single CPU thread, 128-frame float I/O, double biquad states, warm median/p95/max, no audio device',
                     'Static rendering and continuous full-branch fades; prediction/design/control measured separately',
                     'No listening/localization user study; objective ILD and ITD are proxies only']}
    write_json(out/'report.json',report)
    (out/'native-source.cpp').write_bytes(source.read_bytes())
    (out/'validation-source.py').write_bytes(Path(__file__).read_bytes())
    dumpbin=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\bin\Hostx64\x64\dumpbin.exe')
    if dumpbin.is_file():
        dependencies=subprocess.check_output([str(dumpbin),'/dependents',str(exe)],text=True)
        (out/'dependencies.txt').write_text(dependencies,encoding='utf-8')
        if any(name in dependencies.lower() for name in ('torch','python','onnx','cuda','cudnn')):
            raise AssertionError('External inference runtime linked')
    rows=['| Traitement | Sources | Médiane µs/bloc | p95 µs/bloc |','|---|---:|---:|---:|']
    for item in benchmark['measurements']:
        rows.append(f"| {item['kind']} | {item['voices']} | {item['medianUs']:.2f} | {item['p95Us']:.2f} |")
    text=f'''# Rendu HRTF neuronal natif : validation et mesures

{rate} Hz ; {cpu_name()} ; un thread ; blocs de 128 frames (budget {128/rate*1e6:.2f} µs).
Poids : {manifest['sha256']} ; checkpoint : {manifest['checkpointSha256']}.

881 directions de test D2 KEMAR, signaux impulsionnels réellement rendus en C++.
Erreur spectrale vs SOFA : {errors['linear']['magnitudeRmseDb']:.5f} dB RMS, grille linéaire 200–20 000 Hz.
ILD : {errors['linear']['ildRmseDb']:.5f} dB RMS. ITD : {itd_metrics['rmseUs']:.5f} µs RMS.
Écart numérique maximal du signal C++ à la réponse théorique : {numerical_max:.6f} dB.

'''+ '\n'.join(rows)+'''

Les temps de rendu incluent le mixage. Préparer prédiction/conception sur un worker,
puis publier le design entre blocs. SOFA : les 256 taps d'origine, sans détection d'attaque.
Le modèle conserve l'amplitude et l'ITD approximatifs, pas toute la phase mesurée.
Aucune étude d'écoute/localisation humaine. Aucun callback OS certifié.
'''
    (out/'Rapport.fr.md').write_text(text,encoding='utf-8',newline='\n')
    print(json.dumps({'directory':out.relative_to(ROOT).as_posix(),'quality':errors,'itd':itd_metrics,'benchmark':benchmark}),flush=True)


if __name__=='__main__':
    main()
