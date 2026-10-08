# G384 hardware gain prediction

## Summary

This optional example asks one question: does a frozen, calibrated native C++
compressor predict the recorded gain of a particular SSL Logic FX G384 better
than the nominal baseline, on three songs excluded from development fitting?
It reproduces a fixed comparison. It does not optimize a new model or assert
faithful audio emulation. The implementation is maintained internally and
selected for export into audio-core; it is not a separate repository, installed
library API or a dependency of Pressure, Barometer or Audio Attest.

| Concern                               | Owner / evidence                                                                                    |
| ------------------------------------- | --------------------------------------------------------------------------------------------------- |
| Intended equations, tests and verdict | [CONTRACT.md](CONTRACT.md), G384-1..7                                                               |
| Native sample processing              | `Baseline.h`; `render.cpp` is the offline binary adapter                                            |
| Frozen model parameters               | `candidate.json`; origin explained below                                                            |
| Hardware reference                    | `dataset.json`, nine upstream input/CV/audio files with revision, size and SHA-256                  |
| Full evaluation command               | `reproduce.py`; native qualification, reference admission, rendering and comparison                 |
| Full recovery/replay qualification    | `qualify.py`; invokes the real command, corrupts a disposable data copy, checks recovery and replay |
| Measurement instrument tests          | `evaluation_test.py`, metric zero/span, verdict controls and independent WAV scaling controls       |
| Native detection power                | `native_test.py`, analytic step/decay, actual native mutation and restoration                       |
| Child cleanup                         | `lifecycle_test.py`, real successful, timed-out and interrupted processes                           |
| Public file provenance                | Generated `source-provenance.json`; canonical and post-transformation hashes                        |

## What is modeled and where the data comes from

The reference dataset is [SSL Bus Compressor Control Voltage Dataset](https://huggingface.co/datasets/bthomp23/SSL_bus_compressor_control_voltage_dataset),
by Benjamin R. Thompson and Michael C. Heilemann. Its
[paper](https://arxiv.org/html/2606.18573v1) describes the measured G384 and capture
procedure. The dataset release declares CC-BY-4.0; retain attribution and the
pinned revision. This example retrieves only the three declared input/CV/audio triples,
not the full dataset, and does not redistribute the recordings in source or CI
artifacts. A dataset license statement is not a guarantee about every possible
downstream use of underlying music. The listening files are generated locally; the source release and hosted CI
evidence contain no music. Review any separate redistribution in its own scope.

Dataset revision: `64f1aebb431da651fa0792956eade9516fb35ef3`. Setting: `p063`.
Records: `clip_00131` (OpenFire), `clip_00143` (Polemic), `clip_00209`
(WellTalkAboutItAllTonight). Each is 30 seconds at 44.1 kHz. Input is stereo
24-bit PCM; CV reference is mono floating-point signed gain in dB, not audio.
Recorded hardware output is stereo 24-bit PCM. Nine files total 63,504,504 bytes. Exact URLs, byte sizes and hashes are executable
inputs in dataset.json. Input reaches the native model; recorded CV reaches only
the comparator. Hash verification precedes decoding.

The model is peak detection, a hard-knee compression curve and one smoothed
attenuation state. See G384-1 for equations, units, initial conditions and
exclusions. No hardware schematic is reconstructed and no neural network is
trained. This is behavioral modeling. The separate diode example models an
ideal circuit against analytic and SPICE references. Neither makes Barometer or
Pressure a G384 emulation. Matching this dataset's gain is not the same as
matching the complete recorded audio signal.

## Parameter origin and independence

The nominal baseline is threshold -23.823639042 dBFS, ratio 4, attack .01 seconds,
release .3 seconds and makeup .2525259881 dB. The calibrated parameters in
candidate.json were frozen before opening these three records. The development
candidate SHA-256 and original native-header SHA-256 are retained there as
historical identifiers. Source export may change comments/formatting, so current
source hashes are independently recorded for each execution.

Development used the publisher's 1 kHz ramp, 10 ms burst and 1 second burst at
p063. Only threshold, ratio, attack and release were fitted; makeup stayed fixed.
Bounds were T [-36,-6], R [1.1,20], attack [.0001,.1] seconds, release [.01,2]
seconds, with time constants optimized in log space. The native model processed
whole records from zero state. Ramp loss used the whole aligned record. Burst
windows spanned the first through last input magnitude >.1, extended .1 seconds
before and 1.5 seconds after, clipped to the aligned record.

Residuals used every 16th window sample divided by sqrt(selected sample count),
so the squared objective equally weighted the three record mean-square errors.
The optimizer was SciPy least_squares, trf, two-point Jacobian, x_scale=jac,
linear loss, ftol/xtol/gtol=1e-7, max_nfev=60 and at most 400 residual calls,
starting at nominal parameters. This explains the supplied candidate; this
example does not rerun or independently qualify that optimization. Reproducing
parameter estimation is a separate deliverable, not a hidden prerequisite.

The reserved song groups were selected by SHA-256 ordering with prefix GCB-1
from source-song metadata, excluding the previously inspected clip_00000 song.
They were not calibration inputs. They are now consumed evaluation data. Any
future tuning informed by these results requires a new independent evaluation
set. Do not rename a tuned result as untouched evaluation.

## Run on native macOS or Linux

Prerequisites: a little-endian POSIX system, Python 3.12 with venv, CMake >=3.22,
and a C++17 Clang/GCC toolchain. On macOS install Xcode Command Line Tools and
provide Python 3.12/CMake through your normal package manager. These are external
setup dependencies; this command does not install system packages. Windows
uses the Linux container procedure below; native Windows is outside this lane.

Set SOURCE to the absolute example directory. In the monorepo it is
`tools/dsp-modeling/g384`; in audio-core it is `examples/g384`. Set WORK to a new
absolute directory outside the checkout. The setup downloads pinned Python
wheels from PyPI; --require-hashes verifies them. The shared modeling lock is
copied into the public example. Native canonical use finds it in the parent.

```sh
set -euo pipefail
SOURCE=/absolute/path/to/examples/g384
WORK=/absolute/path/to/new-g384-work
mkdir "$WORK"
python3.12 -m venv "$WORK/venv"
LOCK="$SOURCE/requirements.lock"
if [ ! -f "$LOCK" ]; then LOCK="$SOURCE/../requirements.lock"; fi
"$WORK/venv/bin/pip" install --require-hashes --only-binary=:all: -r "$LOCK"
PYTHONDONTWRITEBYTECODE=1 OPENBLAS_NUM_THREADS=1 \
  "$WORK/venv/bin/python" "$SOURCE/reproduce.py" "$WORK/run"
```

Expected exit 0 and results.json verdict improved_on_all_records for the supplied
candidate. Exit 2 is a valid measured mixed_or_not_improved result. Exit 1 is an
evidence_error; it cannot support a numerical conclusion. Hardware numbers are
observations, not golden acceptance thresholds. Source edits may change them.

For offline replay append `--data /absolute/path/to/prior-run/data`. The runner
copies and rehashes the nine files; it does not trust a cache path. No network
operation occurs in offline mode. Never edit retained data in place.

## Dedicated lanes

From the example directory, with the environment above:

```sh
set -euo pipefail
PYTHON=/absolute/path/to/new-g384-work/venv/bin/python
PYTHONDONTWRITEBYTECODE=1 "$PYTHON" evaluation_test.py
PYTHONDONTWRITEBYTECODE=1 "$PYTHON" lifecycle_test.py
PYTHONDONTWRITEBYTECODE=1 OPENBLAS_NUM_THREADS=1 \
  "$PYTHON" qualify.py /absolute/path/to/new-qualification \
  --data /absolute/path/to/prior-run/data
```

The first command qualifies metric, verdict, alignment, window and WAV instruments. The second
executes real child success, timeout, SIGINT and SIGTERM checks. The third runs
the actual complete workflow, injects corrupt data, proves rejection at
acquisition, recovers in a fresh directory, requires identical records and gain
bytes, and proves an existing output directory is rejected unchanged.
Corruption controls separately truncate a file and flip one byte while retaining
its length. Qualification also sends SIGINT to the actual reproduction command
after observing an active, deliberately stalled configure subprocess. It checks
the failed receipt, exit code and child absence within five seconds, then runs
fresh recovery with real CMake. This fault injection replaces only the external
configure command in that attempt; it is lifecycle evidence, not a successful
compiler execution. Lower-level real-process tests separately cover timeout,
SIGTERM and descendants. Missing readiness or cleanup evidence fails qualification.

Alignment controls use an independently defined ramp and adjacent boundary
impulses to expose lag direction, one-sample errors, omitted startup, overlapping
windows and dropped final samples. The same comparison function produces the
hardware report. These controls verify the declared two-sample convention;
they do not independently establish the physical acquisition delay or its uncertainty.

Every reproduction builds the native renderer with warnings treated as errors,
checks its closed-form step/decay, mono/stereo channel mapping, fresh state,
invalid binary inputs, actual doubled-time-constant mutation and restoration,
and failed/missing child output. These are integration checks; a metric unit
test cannot replace them. Each test cites G384-1..7 and its failure witness.
The main evidence is the complete input -> native gain -> independent recorded
CV -> measured verdict path. The model never consumes CV.

## Container and source archive qualification

The Linux amd64 Dockerfile pins a base-image digest and package versions, and
uses hashed Python dependencies. OS package repositories remain external,
mutable provisioning services; this is not a promise of indefinite bit-identical
image rebuilding. Retain the built image ID and build log for each qualification.
The container has no SPICE/JUCE dependency.

For the public example, build from its directory:

```sh
set -euo pipefail
docker build --platform linux/amd64 -t bws-g384-reference "$SOURCE"
docker image inspect bws-g384-reference > "$WORK/container-image.json"
mkdir "$WORK/container-results"
docker run --rm --platform linux/amd64 --user "$(id -u):$(id -g)" \
  -v "$SOURCE:/source:ro" -v "$WORK/container-results:/results" \
  bws-g384-reference /source/qualify.py /results/qualification
```

Online acquisition requires HTTPS access to Hugging Face and its download hosts.
For network-disabled qualification, additionally mount a verified prior data
directory at /data:ro, pass --network none, and append --data /data.
For canonical Docker builds use `docker build -f "$SOURCE/Dockerfile"
"$SOURCE/.."` so the shared lock is available in the build context.

Qualify the actual exported source archive, not a hand-copied substitute:
extract a newly generated audio-core archive into a directory whose name
contains spaces; verify it contains no .git or private research dependencies;
point SOURCE at its examples/g384; mount source read-only as above. Retain the
archive SHA-256, source-provenance.json, image ID, command, logs and qualification
receipt. The public G384 workflow executes this archive path. Do not count
previous library, diode or platform results as new G384 qualification.

## Evidence and recovery

Output includes candidate.json, source-identities.json, acquisition.json,
results.json, native gains (.f64), input transport files, controls/controls.json,
raw logs, per-process command files and CMake build metadata. qualification.json
adds workflow checks and replay hashes. Its qualified status describes the
instrumentation; the command preserves the numerical exit code, so an adverse
hardware outcome still exits 2 and cannot make the CI job green. Run receipts record Python, NumPy,
SciPy, OS and elapsed time. CMake cache and configure logs identify compilers.
The CV files' unused PEAK chunk produces a recognized SciPy metadata warning;
decoding.json retains it. Unrecognized diagnostics fail decoding.

A missing dependency leaves evidence_error at dependencies; install from the
lock and use a fresh output. A download/hash failure leaves partial evidence;
check the pinned URL, connectivity and bytes before a fresh run. A native or
control failure leaves diagnostics; do not reinterpret it as hardware mismatch.
SIGINT/SIGTERM retains failure evidence and cleans owned children. Detached
processes and forced owner termination are excluded. Limits: renderer 180 s,
configure/build 900 s each, total experiment 3600 s and cleanup 5 s. Recovery always
uses a new output; never overwrite evidence to make a run appear successful.

Source and data identities are checked before/after. Runtime and storage vary;
retain measured values from your own results.json. Plan for at least 2 GB external
workspace for qualification, plus Docker images, Python packages and compiler
installation. Byte-identical replay is claimed within a recorded environment,
not across arbitrary compilers or floating-point libraries.

## Interpreting the result

The initial local experiment measured fitted RMS gain errors approximately
.442, .406 and .335 dB, versus nominal 2.931, 2.763 and 2.490 dB respectively.
The model generally predicted too much attenuation, with mean errors around
-.432,-.396 and-.310 dB. These are historical observations, not tolerances.
The report retains per-file MAE/RMS/peak/bias and one-second windows so silence,
startup or averaging cannot be silently discarded. Every file must improve
both MAE and RMS; a pooled score cannot override a worse file.

No absolute accuracy target, measurement uncertainty bound, listening result,
full audio fidelity, cross-setting/unit result or production realtime deadline
is established. Public availability demonstrates this measured workflow only.
Improving the model or connecting it to a product requires a separate declared
target and suitable independent evidence.

## Listen to locally reproduced audio

The same reproduce.py command also creates LISTEN.md and four tracks for each
of the three records. No extra command, player, private plugin or pre-rendered
model result is required. Open the WAV files under each clip's listen/ directory
in a DAW at 44.1 kHz, align their starts and solo one at a time. Disable effects,
time stretching and automatic normalization. Begin at a comfortable low volume.

| File                      | Meaning                                                                        |
| ------------------------- | ------------------------------------------------------------------------------ |
| 01-input.wav              | Original input, cropped to the common comparison length                        |
| 02-model-gain-only.wav    | Input multiplied by the locally rendered fitted model gain                     |
| 03-recorded-hardware.wav  | Publisher's recorded G384 output, including its analog and acquisition path    |
| 04-hardware-gain-only.wav | Input multiplied by recorded hardware gain with the fixed two-sample CV offset |

All tracks contain N-2 frames. Model audio uses g[n]; hardware gain reconstruction
uses CV[n+2]. Recorded hardware audio starts at its original first sample; no
waveform delay fitting is performed. Comparing 02 with 04 isolates gain behavior
more directly than comparing 02 with the complete hardware recording. None of
these tracks establishes an accurate model of coloration or full hardware fidelity.

measurement/ contains unscaled float32 audio. listen/ contains separate copies
matched to the quietest whole-record stereo RMS, with shared attenuation if
needed to keep sample peaks at or below -3 dBFS. This is constant-gain RMS
matching, not perceptual loudness matching, limiting or a true-peak guarantee.
results.json records frame counts, gains and SHA-256 hashes. Every written WAV
is read back and checked against its intended samples; qualify.py requires all
24 WAV hashes to match after fresh recovery. Qualification also independently
checks labeled tracks against native gain and source records, substitutes dry
audio for the model track to prove detection, then restores and verifies it. The existing gain-error verdict
uses unmodified data and cannot be improved by listening normalization.

This demonstrates a usable behavioral-model output. It does not qualify a
production plugin. The diode circuit example and Barometer remain separate.
Audio is fetched from pinned upstream URLs and generated only in the chosen
external output directory; source archives and hosted evidence exclude it.
