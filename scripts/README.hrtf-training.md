# Optional HRTF GPU training pilot

Original MIT experimental tool. It trains a small directional field for one
known SADIE II listener; it is not a production neural renderer. Original CyDSP
code stays MIT. Apache-2.0 measurement rights/attribution are copied beside each
run. PyTorch/CUDA, NumPy and h5py are training tools only, not linked to or
installed by the C++ package.

## Isolated environment

Python 3.11 was used on Windows x64. The commands below use an activated venv;
on Windows activate build/hrtf-venv/Scripts/Activate.ps1, or use the explicit
python.exe from that folder. On Linux activate build/hrtf-venv/bin/activate.

~~~sh
python -m venv build/hrtf-venv
# Activate the venv before the following commands.
python -m pip install torch==2.9.1 --index-url https://download.pytorch.org/whl/cu128
python -m pip install -r scripts/requirements-hrtf-training.txt
python scripts/fetch_hrtf_training.py
python scripts/train_hrtf_pilot.py --self-test
python scripts/train_hrtf_pilot.py --subject D2 --steps 30000 --batch-size 512 --validation-every 1000
~~~

CUDA is required: the tool fails explicitly rather than timing a CPU fallback.
The complete installed environment is recorded in each run. A Linux/A100 run
needs the actual GPU/driver/runtime checked separately; no speed ratio is assumed.

## Model and objective

Default: xyz plus Fourier features, two 64-unit tanh hidden layers, 10443 learned
parameters (41772 bytes of raw float32 parameters). Each ear uses 12 bounded
RBJ peaking sections plus a common ear gain. The last output predicts ITD,
defined as right-minus-left arrival time in microseconds. The prediction
produces filters and a delay; it does not process individual audio samples.

Peaking centers are logarithmically spaced from 100 to 20000 Hz and may shift
by 0.45 of one log interval. Q is in [0.3,12], section gain in [-24,24] dB,
ear gain in [-30,30] dB and ITD in [-1200,1200] us. The analytic response uses
a sine-product identity to avoid low-frequency cancellation in float32.
Equations agree with the double complex RBJ response oracle, zero-gain identity
and gradient checks; the known-delay and pole checks run before each training.

Original 96 kHz/512-tap SOFA files remain unchanged. FFT targets use zero-padding
to 4096, not additional measured bandwidth. Default training uses 256 logarithmic bins
from 200 Hz to 20 kHz; the mixed option uses 128 logarithmic and 128 linear bins.
Both use ITD from a 200-1500 Hz cross-ear phase slope. One
training-only mean dB offset centers the target; retain magnitudeOffsetDb
when reconstructing absolute response.

The fixed direction split is 80% train, 10% validation, 10% test, grouping equal
directions and both ears together. This measures interpolation within a known
listener, not generalization to a new listener. Dense angular neighbors can
still fall in different partitions. A later cross-person model must use the
subject partitions from the corpus catalog.

## Output and interpretation

Each unique run lives under ignored build/hrtf-runs. metadata.json records
configuration, source SHA-256 and environment. progress.csv records CUDA-
synchronized times and validation. best.pt is a tensor checkpoint loadable with
weights_only=True; weights.json is a plain inspection export. best.pt contains inference weights. The separate atomic resume.pt contains model,
Adam, sampler/global RNG state, original schedule, data/split hashes and the best
checkpoint. --resume-run restores this complete state with the same trainer and
PyTorch version, without repeating warm-up. A GPU change may change numerical
rounding; bitwise equivalence is only checked within the same environment.
--initial-weights explicitly fine-tunes compatible weights with a fresh optimizer;
the previous offset is retained and the initial model remains eligible as best.
report.json contains held-out error on both the training log grid and an
independent linear grid. Mean-training-spectrum and nearest-training-direction
comparators use only training measurements.

The initial 50 warm-up updates are disclosed and excluded from trainingSeconds.
wallSeconds includes data preparation, warm-up, validation, checkpointing,
final evaluation and export; interpreter startup and self-tests are excluded.
Iteration estimates hold the model/batch/bin configuration fixed and do not
predict convergence or speed on another GPU.

This is magnitude plus a low-band ITD approximation: full HRIR phase and
headphone/listener fit have not been validated. It adds no C++ API, neural
runtime dependency or CPU performance claim. Keep checkpoints outside source
packages until separately evaluated and deliberately selected for distribution.

## Quality comparison without selecting on the test set

~~~sh
python scripts/diagnose_hrtf_field.py --run build/hrtf-runs/EXISTING_RUN
python scripts/compare_hrtf_quality.py --subject D2 --screen-steps 8000 --refine-steps 20000
python scripts/check_hrtf_field.py --run build/hrtf-runs/SELECTED_RUN
~~~

The diagnostic uses train/validation only. It reports uniform-frequency band
errors and fits free per-direction parameters on a small training subset.
Those finite fits indicate representation limits; they are not a deployable
field or a proven optimum.

The comparison writes its four cases and selection rule before training into
ignored build/hrtf-quality. All cases train with mixed frequency bins. They
compare the logarithmic and hybrid center grids, a larger direction network,
and 12 versus 24 peaking sections per ear. The hybrid grid places three nominal
centers at 150, about 474 and 1500 Hz; the others span 2500..20000 Hz linearly.
Center movement uses 0.45 of the smaller neighboring log distance. Increasing
--feature-bands adds sin/cos direction features; defaults remain unchanged.

Screening selects one candidate per filter budget using only the balanced
log/linear validation MSE plus normalized ITD MSE. Each chosen model is refined;
both checkpoint hashes are frozen in selection.json before any new test
evaluation. Screening/refinement reports have heldOutTestEvaluated=false.
The explicit --evaluate-run command writes evaluation.json for a frozen model.
The best initial checkpoint is retained if fine-tuning does not improve it.

check_hrtf_field.py verifies snapshot and source-data hashes, stable poles for
all original directions, a complex128 RBJ oracle, CPU/GPU numerical agreement
and exact JSON weight export. It never executes the archived Python snapshot.

The 12/24 labels are filter budgets, not measured audio CPU performance levels.
A larger predictor also costs more at direction updates. Report low/high bands
and ITD alongside average spectral errors: a lower average is not a guarantee
that every frequency or direction improves. Keep the measured renderer available.

## Total ITD versus external delay

The delay head predicts desired total right-minus-left ITD. A peaking cascade
also contributes interaural phase. Before applying any external ear delay, use
compensated_delays_us(prediction) to subtract that intrinsic phase-slope ITD.
The returned left/right values are nonnegative microseconds; convert to frames
with sampleRate/1000000. The helper allocates and is a preparation/analysis tool,
not a C++ audio callback API. Phase is fitted over 200..1500 Hz on the same FFT
grid as the measured target. This aligns one low-band slope, not full HRIR phase.

Apply the residual external delays instead of also applying the raw head value.
Restore magnitudeOffsetDb for absolute output gain. Current models target
96 kHz; another processing rate needs an adapted and validated design.
check_hrtf_field.py compares implied compensated/uncompensated phase slopes
against the original measurement target, separately from streamed audio tests.

## Google Colab / A100 with Drive

Open [the Colab notebook](../notebooks/CyDSP_HRTF_A100.ipynb). Put its matching
CyDSP-HRTF-Colab.zip from build/colab in MyDrive/CyDSP-HRTF, select an A100
runtime and execute the cells in order. The private repository needs no access
token. PyTorch/CUDA, NumPy/h5py and pinned SADIE II data are downloaded from their
official package/data origins. The isolated venv leaves the Colab kernel alone.
A100 40/80 GB use the same float32 recipe, with TF32/AMP disabled. No system driver
is installed. Resource availability remains controlled by Colab.

The default refines the previously selected D2 hybrid-24, width-96, four-feature-band
model for a new 100000-update schedule with batch 512. Its initial 50 warm-up
updates are additional and recorded. D2 references have already been tested;
final test metrics track the same test rather than a fresh independent holdout.
Other subjects train independent per-listener models. This is not a
multi-listener conditional model. Keep the existing measured renderer.

The optional batch benchmark uses only training directions and reports
milliseconds/update, examples/second, peak GPU allocation and a fixed-step
compute estimate. It does not automatically select a batch or predict convergence,
end-to-end wall time, A100 speedup or audio CPU rendering performance.

Each experiment freezes its configuration in Drive. Every validation checkpoint
atomically replaces local resume.pt; the notebook uploads a verified immutable ZIP
approximately every 60 seconds and on exit. The latest.json pointer is written
after a read-back hash check. Older generations are retained. Recovery may lose
work since the last successful Drive checkpoint, including checkpoint/copy delay.
The default caches original ZIPs on Drive, while SOFA extraction and GPU work stay
on local storage. Neither raw SOFA nor CUDA binaries are in the upload bundle.

Keep the same notebook, source bundle and experiment plan to resume. A new plan
requires a new EXPERIMENT label. To continue with a new total-step schedule after
completion, start a new experiment using --initial-weights; changing the old
schedule is not a resume. Mid-update interrupts never overwrite the last complete
optimizer checkpoint. --stop-after N pauses at an absolute step without changing
the planned schedule; --output-dir requires a new directory under build/hrtf-runs.

Rebuild the matched notebook/bundle pair explicitly:

~~~sh
python scripts/make_hrtf_colab.py
# A clean checkout without the optional local reference weights:
python scripts/make_hrtf_colab.py --without-reference-weights
python scripts/benchmark_hrtf_training.py --subject D2 --filters 24
~~~

Sources: [Colab resource limits](https://research.google.com/colaboratory/faq.html),
[official PyTorch CUDA wheels](https://pytorch.org/get-started/previous-versions/),
[pip isolated interpreter](https://pip.pypa.io/en/stable/topics/python-option/),
[SADIE II](https://zenodo.org/records/12092466).


## Native runtime and 48 kHz assets

CyDSP 0.4.2 exposes the experimental NeuralHrtfModel / NeuralBinaural C++ API.
Its runtime needs no Python, PyTorch, ONNX or GPU. The exporter writes bounded
CYHRTF02 binary weights plus a source-hash manifest and data notices under build.
The host loads bytes; weights are not automatically installed with CyDSP.

Use --sample-rate 48000 on the trainer after fetching --rate 48000. A 96 kHz
model cannot simply be used at 48 kHz: the RBJ response changes. The explicit
--transfer-sample-rate together with --initial-weights permits a new, same-listener
cross-rate adaptation with a fresh optimizer/schedule, original split and recorded
source hashes. Existing archived training-source.py snapshots remain authoritative
for resuming their exact old runs; changing the trainer is not an exact resume.

~~~sh
python scripts/export_hrtf_native.py --run build/hrtf-runs/YOUR_RUN --output build/hrtf-models/D2-48000/model.cyhrtf
python scripts/validate_hrtf_rendering.py --run build/hrtf-runs/YOUR_RUN
# Independently execute scalar/SSE2/AVX2/AVX-512 on capable CPUs:
python scripts/validate_hrtf_rendering.py --run build/hrtf-runs/YOUR_RUN --backend scalar
~~~

NeuralBinaural uses double biquads and unity-magnitude Thiran fractional delay,
with the cascade's intrinsic ITD subtracted before applying positive ear delays.
Ear gains already include the recorded training magnitude offset. Predict/design
on a control worker and publish completed designs between audio blocks. The
sequential batch API reuses SIMD additions over transposed shared weights.
See the CyDoc native neural HRTF guide for lifetime, concurrency, transition,
rate, format and quality limits. Objective spectrum/ILD/ITD checks do not certify
perceptual localization, and this model does not recover full measured HRIR phase.


## Colab L4 quality continuation, about 30 compute minutes

Use notebooks/CyDSP_HRTF_L4_Quality.ipynb with its separate matching
build/colab/CyDSP-HRTF-L4-Quality.zip. The recipe starts from the validated
48 kHz D2/24-section checkpoint, with 140000 additional updates, batch 512,
and cosine Adam learning rate 0.0003 down to 0.00001. The earlier L4 measured
about 12.6 ms/update: 29.4 compute minutes, excluding setup, validation and
Drive transport, with actual timing/convergence dependent on the session.
The baseline model stays eligible for best validation; no deployment asset
is overwritten. This is a new optimizer schedule, not an exact continuation
of the completed 96 kHz Colab optimizer. A disconnected session of the new
experiment resumes its own model/Adam/RNG/schedule from Drive.

Rebuild with python scripts/make_hrtf_colab.py --profile l4-quality. This
leaves the older A100-named notebook and its matching bundle unchanged.
The trainer exposes --learning-rate and --minimum-learning-rate; defaults
remain 0.002 and 0.0002 for existing recipes. The new profile has separate
source hashes and a separate Drive experiment name. Never change a frozen
experiment plan during resume. No local CUDA training is required to generate
or check this notebook/source bundle.


## L4 without Google Drive authorization

notebooks/CyDSP_HRTF_L4_Direct.ipynb takes its matching
build/colab/CyDSP-HRTF-L4-Direct.zip through google.colab.files.upload, then
returns CyDSP-HRTF-L4-Results.zip through files.download. It never mounts Drive
or calls Drive authentication. It uses the same 48 kHz quality refinement and
stores periodic checkpoints on the Colab VM. Download the result before releasing
the VM; unexported state disappears if that VM is lost. The final cell also
exports the latest complete checkpoint after an interruption. RESTORE_RESULT=True
allows a saved result archive to restore its verified checkpoints on a new VM.
The outer result stores the fixed plan and only each run's latest complete snapshot,
including Adam/RNG/weights, source and notices. Checks reject wrong archive kinds,
corrupt pointers, unsafe paths and conflicting existing output before extraction.
Rebuild with python scripts/make_hrtf_direct_colab.py. Older Drive notebook/bundle
pairs are preserved. CPU fixture tests cover orchestration and archive transport;
the live Colab L4 and upload/download widgets require their own execution.


## Comparaison d’écoute issue du rendu natif

Après deux validations avec validate_hrtf_rendering.py, make_hrtf_listening.py
compare les mêmes directions réservées au test et crée trois WAV stéréo PCM24 :
SOFA mesuré, ancien modèle et nouveau modèle. Le signal synthétique et le gain
sont communs ; les impulsions neuronales viennent du C++, puis la convolution
d’écoute se fait hors ligne. Six poses statiques, sans mouvement ni étude d’écoute.
Outil optionnel CPU NumPy/h5py ; aucune dépendance ajoutée au runtime CyDSP.

```powershell
python scripts/make_hrtf_listening.py --before-render build/hrtf-native-rendering/BEFORE --after-render build/hrtf-native-rendering/AFTER --before-run build/hrtf-runs/BEFORE --after-run build/hrtf-runs/AFTER --output build/hrtf-listening/COMPARISON
```

L’outil attend deux rapports 48 kHz réussis du même SOFA D2 à Data.Delay nul,
des tableaux train/validation/test identiques et les checkpoints correspondants.
Il vérifie provenance, tailles des enregistrements, données finies et encodage PCM.
Les fichiers restent dans build/hrtf-listening ignoré. Conserver README.fr.md,
conditions.json et notices avec les exemples ; aucune normalisation individuelle
ne doit masquer les différences de niveau entre modèles.


## Scènes musicales et parlées en mouvement

make_hrtf_scenes.py utilise HrtfScenes.cpp pour rendre réellement en C++ les trois
versions SOFA/ancien/nouveau : voix française synthétique, pas/impacts synthétisés
et boucle musicale originale à 100 BPM. Chaque scène de 24 secondes effectue un
tour horizontal puis un passage au-dessus/en dessous. Contrôle à 62,5 Hz, fondus
256 frames, blocs 128 frames. La référence utilise 8 voisins et sinc17 ; ses 8
frames de latence algorithmique sont alignées hors ligne pour les comparaisons.
Le gain est commun aux versions d’une scène ; les WAV PCM24 sont vérifiés après
encodage. Les aperçus assemblent trois extraits de huit secondes des mêmes scènes.

```powershell
./scripts/make_hrtf_scene_voice.ps1
python scripts/make_hrtf_scenes.py --voices build/hrtf-listening/scene-voices-fr --output build/hrtf-listening/NEW-SCENES
```

Le premier outil nécessite Windows/System.Speech et une voix française installée
(Microsoft Julie par défaut, configurable avec -Voice). Le second utilise le
modèle/corpus validé localement et le build C++ statique de production, à sélectionner
avec --before/--after/--reference/--library si nécessaire. Aucun téléchargement,
entraînement ou runtime vocal n’est ajouté à CyDSP. Les voix système et fichiers
sonores générés restent hors du dépôt et du paquet runtime. L’outil ne prétend pas
produire une mesure perceptive ou un test de suivi de tête individuel.


## Neural rendering optimization without retraining

CyDSP 0.4.4 renders the existing CYHRTF02 assets with specialized double cascades.
NeuralBinaural::processBatch accepts N mono channels and 2*N independent output
channels (L0,R0,...), groups up to four voices and leaves mixing to the host.
setDesigns installs worker-prepared designs with whole-batch preflight and grouped
raw-input replay. Individual rendering/updates remain available. Replay defaults
to 512 frames, is configurable from 0 to 65536 during prepare, and adds CPU work
when applying a fading target. It approximates an infinite IIR past; it is not a
new training run or a universal guarantee of artifact-free movement.

The explicit CPU-only benchmark needs only Python's standard library, a native
compiler and an existing 48 kHz model. Optional frozen baseline include/archive
paths allow a true before/after comparison without rebuilding old sources.

```powershell
python scripts/measure_neural_optimization.py --model build/hrtf-models/D2-48000-L4-134000/model.cyhrtf --baseline-library build/neural-optimization/20261117/before/CyDSP.lib --baseline-include build/neural-optimization/20261117/before/include
```

Static, moving-with-prepared-designs and total-moving costs are separate. Total
includes prediction/design CPU even if the host moves that work to a worker.
The table includes 1/16/64 voices and replay 0/512/2048; zero preserves the earlier
cold-start transition policy. These are medians of series means, not per-block
worst cases or a callback deadline certificate. Every run writes native sources,
model/library/executable hashes, JSON and a report under ignored build output.
No additional dataset, Python ML runtime, GPU or model download is required.
