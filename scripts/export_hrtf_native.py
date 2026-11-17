# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
"""Export an experimental checkpoint for CyDSP's native memory-only HRTF API.

PyTorch is an offline export dependency only. Assets are not bundled in CyDSP.
"""
import argparse
import json
import math
from pathlib import Path
import struct
import torch
from train_hrtf_pilot import ROOT, model_from_metadata, sha256, write_json
from benchmark_hrtf_native import f32bytes


def export_model(run, destination):
    run, destination = Path(run).resolve(), Path(destination).resolve()
    if not run.is_relative_to((ROOT / 'build/hrtf-runs').resolve()):
        raise ValueError('Use a local checkpoint under ignored build/hrtf-runs')
    if not destination.is_relative_to((ROOT / 'build').resolve()):
        raise ValueError('Experimental exports stay under ignored build')
    checkpoint = torch.load(run / 'best.pt', map_location='cpu', weights_only=True)
    meta = checkpoint['metadata']
    if sha256(run / 'training-source.py') != meta['sourceCodeSha256']:
        raise ValueError('Training source snapshot hash mismatch')
    model = model_from_metadata(meta, 'cpu').eval()
    model.load_state_dict(checkpoint['state_dict'])
    width, filters = meta['arguments']['width'], meta['arguments']['filters']
    bands = meta['arguments'].get('feature_bands', 3)
    rate = meta['arguments'].get('sample_rate', 96000)
    data = bytearray(b'CYHRTF02' + struct.pack('<6If', 3+6*bands, width, 2*(3*filters+1)+1,
                                             filters, bands, rate, meta['magnitudeOffsetDb']))
    for index in (0, 2, 4):
        for name in ('weight', 'bias'):
            data.extend(f32bytes(checkpoint['state_dict'][f'network.{index}.{name}']))
    for value in (model.centers, model.center_shift,
                  torch.tensor([math.pi*2**i for i in range(bands)], dtype=torch.float32)):
        data.extend(f32bytes(value))
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(data)
    manifest = {'format': 'CYHRTF02', 'version': 2, 'sampleRate': rate,
                'bytes': len(data), 'sha256': sha256(destination),
                'checkpointSha256': sha256(run / 'best.pt'), 'bestStep': checkpoint['step'],
                'trainingSourceSha256': meta['sourceCodeSha256'], 'source': meta['source'],
                'parameters': meta['parameters'], 'magnitudeOffsetDb': meta['magnitudeOffsetDb'],
                'meaning': 'One known listener, magnitude plus low-band ITD; not measured full HRIR phase',
                'rights': 'Original CyDSP runtime MIT; SADIE II Apache-2.0 source attribution retained. No automatic model redistribution.'}
    write_json(destination.with_suffix('.json'), manifest)
    for name in ('LICENSE-data.txt', 'NOTICE-data.txt'):
        (destination.parent / name).write_bytes((run / name).read_bytes())
    return model, meta, manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    _, _, manifest = export_model(args.run, args.output)
    print(json.dumps(manifest), flush=True)


if __name__ == '__main__':
    main()
