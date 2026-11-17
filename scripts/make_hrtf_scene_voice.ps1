# Copyright (c) 2026 MrMybal CyberAlien. SPDX-License-Identifier: MIT
param([string]$Voice = 'Microsoft Julie', [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $projectRoot 'build\hrtf-listening\scene-voices-fr' }
$voiceTarget = [IO.Path]::GetFullPath($OutputDirectory)
$allowedRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build\hrtf-listening')) + [IO.Path]::DirectorySeparatorChar
if (-not $voiceTarget.StartsWith($allowedRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must stay under build/hrtf-listening.' }
if (Test-Path -LiteralPath $voiceTarget) { throw 'Choose a new output directory to preserve existing clips.' }
Add-Type -AssemblyName System.Speech
$synthesizer = [System.Speech.Synthesis.SpeechSynthesizer]::new()
try {
    $synthesizer.SelectVoice($Voice)
    $synthesizer.Rate = 1
    $synthesizer.Volume = 100
    $format = [System.Speech.AudioFormat.SpeechAudioFormatInfo]::new(48000, [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen, [System.Speech.AudioFormat.AudioChannel]::Mono)
    New-Item -ItemType Directory -Path $voiceTarget | Out-Null
    $phrases = @('Bonjour. Je commence devant vous.', 'Écoutez. Je passe sur votre gauche.', 'Ma voix se trouve maintenant derrière vous.', 'Je reviens sur votre droite.', 'Maintenant, ma voix passe au-dessus de vous.', 'Puis elle redescend en dessous.')
    for ($i = 0; $i -lt $phrases.Count; $i++) {
        $synthesizer.SetOutputToWaveFile((Join-Path $voiceTarget ('phrase-' + $i + '.wav')), $format)
        $synthesizer.Speak($phrases[$i])
        $synthesizer.SetOutputToNull()
    }
    @{ voice = $Voice; phrases = $phrases; source = 'Original text; installed Windows speech synthesizer; no voice engine bundled' } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $voiceTarget 'phrases.json') -Encoding utf8
} finally { $synthesizer.Dispose() }
Write-Output $voiceTarget
