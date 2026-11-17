# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Make level-matched listening examples from validated native HRTF impulses.

CPU-only optional NumPy/h5py tooling; no training, archived code or GPU runtime.
The neural FIRs are emitted by NeuralBinaural C++; listening signals are convolved
with these finite impulses offline. This is not a dynamic/head-tracked renderer test.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import wave

import h5py
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
CORPUS = ROOT / 'build/hrtf-training/sadie-ii-v2-2'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def native_impulses(directory):
    report = json.loads((directory / 'report.json').read_text(encoding='utf-8'))
    if report['status'] != 'passed' or report['sampleRate'] != 48000:
        raise ValueError('Use a passed native 48 kHz rendering report')
    audio = directory / 'audio.bin'
    with audio.open('rb') as stream:
        count, frames, sections = struct.unpack('<3I', stream.read(12))
    if count != report['testDirections'] or not 1 <= count <= 100000 or not 1 <= frames <= 65536 or not 1 <= sections <= 32:
        raise ValueError('Native impulse header mismatch')
    fields = 2 * (sections * 3 + 1) + 4
    dtype = np.dtype([('parameters', '<f8', (fields,)), ('impulse', '<f4', (2, frames))])
    if audio.stat().st_size != 12 + count * dtype.itemsize:
        raise ValueError('Native impulse size mismatch')
    data = np.memmap(audio, mode='r', dtype=dtype, offset=12, shape=(count,))
    if not np.isfinite(data['impulse']).all():
        raise ValueError('Nonfinite native impulses')
    return report, data['impulse']


def stimulus(rate):
    x = np.zeros(4 * rate, dtype=np.float64)
    rng = np.random.default_rng(20261116)
    def insert(first, last, value):
        begin, end = round(first * rate), round(last * rate)
        y = np.asarray(value, dtype=np.float64)
        fade = min(round(.025 * rate), len(y) // 2)
        ramp = .5 - .5 * np.cos(np.linspace(0, np.pi, fade))
        y[:fade] *= ramp
        y[-fade:] *= ramp[::-1]
        x[begin:end] += y
    t = np.arange(round(.8 * rate)) / rate
    tone = sum(np.sin(2 * np.pi * 220 * harmonic * t) / harmonic for harmonic in range(1, 9)) * .10
    insert(.25, 1.05, tone)
    t = np.arange(round(.8 * rate)) / rate
    slope = np.log(18000 / 200) / .8
    chirp = .12 * np.sin(2 * np.pi * 200 * np.expm1(slope * t) / slope)
    insert(1.15, 1.95, chirp)
    count = round(.8 * rate)
    noise = rng.standard_normal(count)
    spectrum = np.fft.rfft(noise)
    hz = np.fft.rfftfreq(count, 1 / rate)
    band = (hz >= 200) & (hz <= 18000)
    spectrum *= band / np.sqrt(np.maximum(hz, 200))
    noise = np.fft.irfft(spectrum, n=count)
    noise *= .12 / np.sqrt(np.mean(noise * noise))
    insert(2.05, 2.85, noise)
    return x


def convolve(signal, impulse):
    length = len(signal) + len(impulse) - 1
    size = 1 << (length - 1).bit_length()
    return np.fft.irfft(np.fft.rfft(signal, size) * np.fft.rfft(impulse, size), size)[:len(signal)]


def wav24(path, audio, rate):
    if not np.isfinite(audio).all() or np.max(np.abs(audio)) >= 1:
        raise ValueError('Nonfinite/clipping playback samples')
    quantized = np.rint(audio * 8388607).astype('<i4').reshape(-1)
    packed = quantized.view(np.uint8).reshape(-1, 4)[:, :3].copy().tobytes()
    with wave.open(str(path), 'wb') as stream:
        stream.setnchannels(2)
        stream.setsampwidth(3)
        stream.setframerate(rate)
        stream.writeframes(packed)
    with wave.open(str(path), 'rb') as stream:
        if stream.getnframes() != len(audio) or stream.getnchannels() != 2 or stream.getsampwidth() != 3 or stream.getframerate() != rate:
            raise AssertionError('WAV header round trip')
        payload = np.frombuffer(stream.readframes(len(audio)), dtype=np.uint8).reshape(-1, 3).astype(np.int32)
    decoded = payload[:, 0] | (payload[:, 1] << 8) | (payload[:, 2] << 16)
    decoded = (decoded ^ 0x800000) - 0x800000
    if not np.array_equal(decoded, quantized):
        raise AssertionError('WAV PCM24 round trip')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before-render', type=Path, required=True)
    parser.add_argument('--after-render', type=Path, required=True)
    parser.add_argument('--before-run', type=Path, required=True)
    parser.add_argument('--after-run', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    before, before_ir = native_impulses(args.before_render)
    after, after_ir = native_impulses(args.after_render)
    if before['model']['source'] != after['model']['source'] or before_ir.shape != after_ir.shape:
        raise ValueError('Models need matching rate/source/test dimensions')
    for run, report in ((args.before_run, before), (args.after_run, after)):
        if sha(run / 'best.pt') != report['model']['checkpointSha256']:
            raise ValueError('Checkpoint/report mismatch')
    with np.load(args.before_run / 'split.npz', allow_pickle=False) as old, np.load(args.after_run / 'split.npz', allow_pickle=False) as new:
        for key in ('train', 'validation', 'test'):
            if not np.array_equal(old[key], new[key]):
                raise ValueError('Train/validation/test directions differ')
        selection = new['test'].copy()
    source = (CORPUS / before['model']['source']['file']).resolve()
    if not source.is_relative_to(CORPUS.resolve()) or sha(source) != before['model']['source']['sha256']:
        raise ValueError('SOFA provenance mismatch')
    with h5py.File(source, 'r') as sofa:
        positions = np.asarray(sofa['SourcePosition'])
        az, el = np.deg2rad(positions[:, 0]), np.deg2rad(positions[:, 1])
        directions = np.column_stack((np.cos(el)*np.cos(az), np.cos(el)*np.sin(az), np.sin(el)))
        ears = np.argsort(-np.asarray(sofa['ReceiverPosition']).reshape(2, 3)[:, 1])
        delays = np.asarray(sofa['Data.Delay'])
        if np.any(delays != 0):
            raise ValueError('This listening helper expects the known zero-Data.Delay D2 corpus')
        measured = np.asarray(sofa['Data.IR'])[selection][:, ears]
    labels = [('avant', (1, 0, 0)), ('gauche', (0, 1, 0)), ('arriere', (-1, 0, 0)),
              ('droite', (0, -1, 0)), ('dessus', (0, 0, 1)), ('dessous', (0, 0, -1))]
    rate = 48000
    signal = stimulus(rate)
    recordings = {name: [] for name in ('SOFA-mesure', 'Neural-avant', 'Neural-L4-134000')}
    poses = []
    for label, target in labels:
        index = int(np.argmax(directions[selection] @ np.asarray(target)))
        h = measured[index].copy()
        onsets = []
        for ear in h:
            active = np.flatnonzero(np.abs(ear) > .01 * np.max(np.abs(ear)))
            onsets.append(int(active[0]) if len(active) else 0)
        common = min(onsets)
        h = h[:, common:]
        impulses = (h, before_ir[index], after_ir[index])
        for name, response in zip(recordings, impulses):
            recordings[name].append(np.column_stack([convolve(signal, response[ear]) for ear in (0, 1)]))
        poses.append({'label': label, 'sourceIndex': int(selection[index]), 'direction': directions[selection[index]].tolist(),
                      'seconds': [4 * len(poses), 4 * (len(poses) + 1)], 'measuredCommonOnsetRemovedFrames': common})
    recordings = {name: np.concatenate(chunks) for name, chunks in recordings.items()}
    peak = max(float(np.max(np.abs(audio))) for audio in recordings.values())
    gain = 10 ** (-3 / 20) / peak
    destination = args.output.resolve()
    if not destination.is_relative_to((ROOT / 'build/hrtf-listening').resolve()):
        raise ValueError('Listening artifacts stay in ignored build/hrtf-listening')
    destination.mkdir(parents=True, exist_ok=False)
    files = {}
    for name, audio in recordings.items():
        audio *= gain
        path = destination / (name + '.wav')
        wav24(path, audio, rate)
        files[path.name] = {'sha256': sha(path), 'bytes': path.stat().st_size, 'peakDbfs': float(20*np.log10(np.max(np.abs(audio)))),
                            'rmsDbfs': float(20*np.log10(np.sqrt(np.mean(audio*audio))))}
    for name in ('LICENSE-data.txt', 'NOTICE-data.txt'):
        shutil.copyfile(args.after_run / name, destination / name)
    conditions = {'sampleRate': rate, 'durationSeconds': len(next(iter(recordings.values()))) / rate, 'pcmBits': 24,
                  'sharedPlaybackGain': gain, 'directions': poses, 'files': files,
                  'source': 'Original deterministic harmonic/chirp/pinkish-noise signal, 200..18000 Hz',
                  'method': 'Offline linear convolution of the stimulus with impulses actually emitted by the native C++ renderers; static poses, no head tracking',
                  'beforeModel': before['model']['sha256'], 'afterModel': after['model']['sha256'],
                  'beforeNativeAudioSha256': sha(args.before_render / 'audio.bin'),
                  'afterNativeAudioSha256': sha(args.after_render / 'audio.bin'), 'sofaSha256': sha(source),
                  'interpretation': 'No per-file loudness normalization; no listening/localization study or perceptual score',
                  'dataRights': 'SADIE II Apache-2.0, accompanying license/notice; helper code original MIT'}
    (destination / 'conditions.json').write_text(json.dumps(conditions, indent=2) + '\n', encoding='utf-8')
    text = '''# Comparaison HRTF au casque — ancien / L4 / SOFA

Trois WAV stéréo PCM 24 bits, 48 kHz, 24 secondes : SOFA-mesure.wav (référence),
Neural-avant.wav (ancien modèle) et Neural-L4-134000.wav (nouveau checkpoint).
Même signal et même gain pour tous, sans normalisation individuelle. Pic maximal
commun : -3 dBFS. Écouter à volume confortable, sans effet spatial ajouté au lecteur.

Chaque position dure 4 secondes : avant (0–4), gauche (4–8), arrière (8–12),
droite (12–16), dessus (16–20), dessous (20–24). Les directions sont les points
réservés au test les plus proches des axes ; positions exactes dans conditions.json.
Chaque segment contient des harmoniques, un balayage 200–18000 Hz puis un bruit
large bande, avec fondus et silence entre les positions.

Comparer surtout timbre/brillance, stabilité gauche/droite et distinction
avant/arrière/hauteur. Les différences entre ancien et nouveau peuvent être
subtiles. Ce mannequin KEMAR n’est pas votre propre HRTF : une différence de
localisation personnelle ne constitue pas une mesure universelle du modèle.

Les impulsions neuronales ont été calculées par NeuralBinaural C++, puis le signal
d’écoute a été convolué avec ces réponses finies hors ligne. La référence conserve
la phase mesurée ; seul son onset commun est retiré, en gardant l’écart entre
oreilles. Le réseau ne conserve pas toute cette phase. Ces fichiers illustrent
des poses statiques ; ils ne testent ni trajectoires ni suivi de tête. Leur création
ne constitue pas une validation perceptive. Données SADIE II sous Apache-2.0,
avec LICENSE-data.txt et NOTICE-data.txt ; outil de génération original MIT.
'''
    (destination / 'README.fr.md').write_text(text, encoding='utf-8', newline='\n')
    shutil.copyfile(Path(__file__), destination / 'generator-source.py')
    print(json.dumps({'directory': str(destination), 'files': files, 'durationSeconds': conditions['durationSeconds']}))


if __name__ == '__main__':
    main()
