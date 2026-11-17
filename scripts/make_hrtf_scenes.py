# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Original moving HRTF listening scenes; CPU-only native renderer, no training."""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess
import wave

import numpy as np
from make_hrtf_listening import sha, wav24

ROOT = Path(__file__).resolve().parents[1]
CMAKE = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe')
RATE = 48000
SECONDS = 24
UPDATE_FRAMES = 768  # 62.5 Hz control, 256-frame fades


def add(destination, start_seconds, value, gain=1):
    start = round(start_seconds * RATE)
    end = min(len(destination), start + len(value))
    if end > start:
        destination[start:end] += gain * value[:end-start]


def faded(value, seconds=.008):
    value = np.array(value, dtype=np.float64, copy=True)
    count = min(round(seconds * RATE), len(value)//2)
    ramp = .5 - .5*np.cos(np.linspace(0, np.pi, count))
    value[:count] *= ramp
    value[-count:] *= ramp[::-1]
    return value


def level(value):
    value -= np.mean(value)
    peak = float(np.max(np.abs(value)))
    if peak <= 0 or not np.isfinite(value).all():
        raise ValueError('Invalid dry source')
    return value * (.62 / peak)


def voice(directory):
    output = np.zeros(SECONDS * RATE)
    for i in range(6):
        with wave.open(str(directory / f'phrase-{i}.wav'), 'rb') as stream:
            if stream.getframerate() != RATE or stream.getnchannels() != 1 or stream.getsampwidth() != 2:
                raise ValueError('Voice WAV must be mono PCM16/48 kHz')
            values = np.frombuffer(stream.readframes(stream.getnframes()), dtype='<i2').astype(np.float64) / 32768
        if len(values) > 3.6 * RATE:
            raise ValueError('Voice phrase too long for a four-second chapter')
        add(output, 4*i+.20, faded(values, .005))
    return level(output)


def footsteps():
    rng = np.random.default_rng(20261117)
    output = np.zeros(SECONDS * RATE)
    for step, start in enumerate(np.arange(.22, SECONDS-.35, .57)):
        count = round(.32 * RATE)
        t = np.arange(count) / RATE
        noise = rng.standard_normal(count)
        grain = np.diff(noise, prepend=noise[0])
        thump = np.sin(2*np.pi*(68*t + 24*t*t))*np.exp(-t/0.045)
        sole = np.sin(2*np.pi*148*t)*np.exp(-t/0.018)
        contact = grain * (1-np.exp(-t/.0015))*np.exp(-t/.025)
        gravel = noise * np.exp(-t/.065) * (.4+.6*np.sin(2*np.pi*67*t)**2)
        x = .30*thump + .06*sole + .075*contact + .045*gravel
        add(output, start, faded(x,.003), .88 + .12*(step%2))
    for start in (3.4, 7.6, 11.5, 15.4, 19.6, 23.0):
        t = np.arange(round(.55*RATE))/RATE
        wood = .28*np.sin(2*np.pi*310*t)*np.exp(-t/.055)
        metal = sum(.055*np.sin(2*np.pi*f*t)*np.exp(-t/d) for f,d in ((1280,.13),(2197,.24),(3511,.17)))
        transient = .12*rng.standard_normal(len(t))*np.exp(-t/.006)
        add(output,start,faded(wood+metal+transient,.001))
    return level(output)


def music():
    rng = np.random.default_rng(20261118)
    output = np.zeros(SECONDS * RATE)
    beat = .6
    chords = ((57,60,64,67),(53,57,60,64),(48,52,55,59),(55,59,62,64))
    def hz(note): return 440*2**((note-69)/12)
    def key(note, duration, decay, gain):
        t=np.arange(round(duration*RATE))/RATE
        frequency=hz(note)
        x=sum((np.sin(2*np.pi*frequency*(k+.0003*(k%2))*t) * np.exp(-t/(decay/(1+.65*(k-1))))) / k**1.7 for k in range(1,7))
        return gain*faded(x*(1-np.exp(-t/.004)),.012)
    for bar,start in enumerate(np.arange(0,SECONDS,4*beat)):
        chord=chords[bar%len(chords)]
        for note in chord:
            add(output,start,key(note,2.6,1.5,.030))
        for index in range(8):
            note=chord[(index*3+bar)%4]+12+(12 if index==7 else 0)
            add(output,start+index*beat/2+.012*(index%2),key(note,.85,.27,.065))
        add(output,start,key(chord[0]-12,1.0,.30,.13))
        add(output,start+2*beat,key(chord[0]-12,.85,.26,.10))
        for index in (0,2):
            t=np.arange(round(.23*RATE))/RATE
            kick=np.sin(2*np.pi*(48*t + (95*.025)*(1-np.exp(-t/.025))))*np.exp(-t/.065)
            add(output,start+index*beat,faded(.15*kick,.002))
        for index in (1,3):
            t=np.arange(round(.19*RATE))/RATE
            snare=.025*rng.standard_normal(len(t))*np.exp(-t/.037)+.035*np.sin(2*np.pi*185*t)*np.exp(-t/.045)
            add(output,start+index*beat,faded(snare,.002))
        for index in range(8):
            t=np.arange(round(.055*RATE))/RATE
            noise=rng.standard_normal(len(t));noise=np.diff(noise,prepend=noise[0])
            add(output,start+index*beat/2,faded(noise*np.exp(-t/.010),.001),.008 if index%2 else .012)
    return level(faded(output,.04))


def trajectory():
    t=np.arange((SECONDS*RATE+UPDATE_FRAMES-1)//UPDATE_FRAMES)*UPDATE_FRAMES/RATE
    az=np.minimum(t,16)*2*np.pi/16
    elevation=np.zeros_like(t)
    up=(t>=16)&(t<20); down=t>=20
    elevation[up]=np.deg2rad(85)*np.sin(np.pi*(t[up]-16)/4)
    elevation[down]=-np.deg2rad(85)*np.sin(np.pi*(t[down]-20)/4)
    result=np.column_stack((np.cos(elevation)*np.cos(az),np.cos(elevation)*np.sin(az),np.sin(elevation)))
    if not np.allclose(np.linalg.norm(result,axis=1),1,atol=1e-14):
        raise AssertionError('Unit movement directions')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--voices',type=Path,required=True)
    parser.add_argument('--reference',type=Path,default=ROOT/'build/hrtf-native-rendering/20261116T221323Z-d983e7e8')
    parser.add_argument('--before',type=Path,default=ROOT/'build/hrtf-models/D2-48000/model.cyhrtf')
    parser.add_argument('--after',type=Path,default=ROOT/'build/hrtf-models/D2-48000-L4-134000/model.cyhrtf')
    parser.add_argument('--library',type=Path,default=ROOT/'build/windows-x64/Release/CyDSP.lib')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    destination=args.output.resolve()
    if not destination.is_relative_to((ROOT/'build/hrtf-listening').resolve()):
        raise ValueError('Scene artifacts stay under ignored build/hrtf-listening')
    report=json.loads((args.reference/'report.json').read_text(encoding='utf-8'))
    if report['status']!='passed' or report['sampleRate']!=RATE or sha(args.after)!=report['model']['sha256']:
        raise ValueError('Validated reference and new model do not match')
    if sha(args.reference/'corpus.bin')!='a8bb5abcdb50a4c507c057e276c28ad9258cac93140cea4407c2bb6dedc72df5':
        raise ValueError('Expected the verified original 48 kHz D2 corpus')
    destination.mkdir(parents=True,exist_ok=False)
    sources=np.stack((voice(args.voices),footsteps(),music())).astype('<f4')
    poses=trajectory()
    packed=b'CYHRAUD1'+struct.pack('<4I',RATE,sources.shape[1],len(sources),len(poses))
    packed+=poses.astype('<f8').tobytes()+sources.tobytes()
    (destination/'scenes.bin').write_bytes(packed)
    source=ROOT/'scripts/native/HrtfScenes.cpp'
    cmake=f'''cmake_minimum_required(VERSION 3.24)
project(CyDSPHrtfScenes LANGUAGES CXX)
add_executable(hrtf-scenes "{source.as_posix()}")
target_compile_features(hrtf-scenes PRIVATE cxx_std_20)
target_include_directories(hrtf-scenes PRIVATE "{(ROOT/'include').as_posix()}")
target_link_libraries(hrtf-scenes PRIVATE "{args.library.resolve().as_posix()}")
if(MSVC)
 target_compile_options(hrtf-scenes PRIVATE /W4 /fp:precise)
endif()
'''
    (destination/'CMakeLists.txt').write_text(cmake,encoding='utf-8')
    subprocess.run([str(CMAKE),'-S',str(destination),'-B',str(destination/'cmake-build'),'-A','x64'],check=True)
    subprocess.run([str(CMAKE),'--build',str(destination/'cmake-build'),'--config','Release'],check=True)
    native=subprocess.run([str(destination/'cmake-build/Release/hrtf-scenes.exe'),str(args.before.resolve()),str(args.after.resolve()),str((args.reference/'corpus.bin').resolve()),str(destination/'scenes.bin'),str(destination/'rendered.bin')],check=True,capture_output=True,text=True)
    renderer=json.loads(native.stdout)
    with (destination/'rendered.bin').open('rb') as stream:
        if stream.read(8)!=b'CYHRDEM1':raise ValueError('Output magic')
        rate,frames,count,modes=struct.unpack('<4I',stream.read(16))
    if (rate,frames,count,modes)!=(RATE,SECONDS*RATE,3,3):raise ValueError('Output shape')
    if (destination/'rendered.bin').stat().st_size!=24+count*modes*frames*2*4:raise ValueError('Output length')
    audio=np.memmap(destination/'rendered.bin',mode='r',dtype='<f4',offset=24,shape=(3,3,frames,2))
    if not np.isfinite(audio).all():raise ValueError('Nonfinite audio')
    mode_names=('SOFA','Ancien','Nouveau')
    scene_names=('Voix','Pas-impacts','Musique')
    conditions={'sampleRate':RATE,'durationSecondsPerScene':SECONDS,'pcmBits':24,'nativeRenderer':renderer,
                'beforeModelSha256':sha(args.before),'afterModelSha256':sha(args.after),'librarySha256':sha(args.library),
                'sourceSha256':sha(source),'corpusSha256':sha(args.reference/'corpus.bin'),
                'voice':'French synthetic speech, installed Microsoft Julie voice; original phrases',
                'music':'Original procedural keys/arpeggios/bass/drums, 100 BPM; not an acoustic recording',
                'foley':'Original synthesized footsteps and wood/metal impacts; not field recordings',
                'motion':'0..16 s full horizontal orbit front-left-back-right-front; 16..20 s above, 20..24 s below, max elevation +/-85 degrees',
                'gainPolicy':'One shared gain across SOFA/old/new per scene, peak ceiling -3 dBFS; no per-version normalization',
                'comparison':'Actual moving native C++ processing, same mono sources/directions/fades. Reference HRTF uses onset alignment, 8-neighbour interpolation and sinc17; its 8-frame algorithmic delay is aligned offline. No headphone/OS spatialization added.',
                'scope':'Listening examples only, no perceptual score, individualized listener validation or certified realtime callback',
                'scenes':{},'previews':{}}
    normalized=[]
    for i,scene in enumerate(scene_names):
        folder=destination/scene;folder.mkdir()
        peak=float(np.max(np.abs(audio[i])))
        gain=10**(-3/20)/peak
        normalized.append(np.array(audio[i],dtype=np.float64)*gain)
        conditions['scenes'][scene]={'sharedGain':gain,'files':{}}
        for j,mode in enumerate(mode_names):
            path=folder/(mode+'.wav');wav24(path,normalized[-1][j],RATE)
            conditions['scenes'][scene]['files'][mode]={'path':path.relative_to(destination).as_posix(),'bytes':path.stat().st_size,'sha256':sha(path),'peakDbfs':float(20*np.log10(np.max(np.abs(normalized[-1][j]))))}
    # A short overview from the same fully rendered scenes, shared across versions.
    # Voice front-to-left, footsteps behind/right, music above/below.
    for j,mode in enumerate(mode_names):
        excerpts=[]
        for i,(first,last) in enumerate(((0,8),(8,16),(16,24))):
            x=normalized[i][j,first*RATE:last*RATE].copy()
            for ear in (0,1):x[:,ear]=faded(x[:,ear],.025)
            excerpts.append(x)
        path=destination/('Apercu-'+mode+'.wav');wav24(path,np.concatenate(excerpts),RATE)
        conditions['previews'][mode]={'path':path.name,'bytes':path.stat().st_size,'sha256':sha(path)}
    (destination/'conditions.json').write_text(json.dumps(conditions,indent=2)+'\n',encoding='utf-8')
    readme='''# Démonstrations HRTF avec sources en mouvement

Apercu-Nouveau.wav : 0–8 s voix française (avant vers gauche), 8–16 s pas/impacts
(arrière vers droite), 16–24 s musique (au-dessus puis en dessous).
Apercu-SOFA.wav et Apercu-Ancien.wav contiennent exactement les mêmes extraits
avec la référence mesurée ou l’ancien modèle. Même gain entre versions.

Les dossiers Voix, Pas-impacts et Musique proposent chacun les trois versions
complètes de 24 secondes : SOFA.wav, Ancien.wav et Nouveau.wav. Chaque scène fait
un tour horizontal complet de 0 à 16 s, puis passe au-dessus (16–20) et en dessous
(20–24). Écouter au casque, à volume confortable, avec les effets spatiaux du lecteur
désactivés. La HRTF KEMAR n’est pas personnelle : les indices de hauteur peuvent
être différents selon l’auditeur. Aucun résultat perceptif n’est garanti.

La voix française est synthétisée localement avec Microsoft Julie, sur des phrases
originales. La musique (clavier, arpèges, basse, batterie à 100 BPM) et les bruitages
de pas/impacts sont des créations procédurales, pas des enregistrements acoustiques.

Rendu réellement calculé par les moteurs C++ CyDSP avec les mêmes sources mono,
directions actualisées à 62,5 Hz et fondus de 256 frames. La référence SOFA utilise 8 voisins et un délai
sinc17 ; ses 8 frames de latence algorithmique sont alignées hors ligne. Chaque
scène possède un gain commun aux trois versions, sans normalisation individuelle.
Les WAV PCM24/48 kHz ont été encodés, relus puis vérifiés. Conditions et empreintes
dans conditions.json. La création des fichiers n’est pas une étude d’écoute.

Runtime et outils originaux MIT. Données HRTF SADIE II Apache-2.0 avec notices.
Les voix Microsoft restent des composants du système ; aucun moteur de synthèse
vocale ni modèle de voix n’est intégré à CyDSP. Les fichiers sont des exemples
locaux, exclus du paquet runtime et du dépôt Git.
'''
    (destination/'README.fr.md').write_text(readme,encoding='utf-8',newline='\n')
    for name in ('LICENSE-data.txt','NOTICE-data.txt'):shutil.copyfile(args.after.parent/name,destination/name)
    shutil.copyfile(source,destination/'renderer-source.cpp')
    shutil.copyfile(__file__,destination/'generator-source.py')
    print(json.dumps({'directory':str(destination),'renderer':renderer,'previews':conditions['previews']}))


if __name__=='__main__':
    main()
