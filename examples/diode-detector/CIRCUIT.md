# Diode circuit modeling

> **Last Verified:** 2026-10-07

## Summary

This experiment implements an ideal diode detector in native C++, compares the
same discrete equations using a Python solver, and compares the circuit against
ngspice. It is a numerical modeling example, not an emulation of a named device.
The authoritative equations and numerical requirements are in
[the circuit contract](../../docs/testing/contracts/diode-detector-modeling.md).

## Role and independence

This is a standalone numerical experiment. Its native executable renders voltage
samples for comparison; it is not an audio plugin. The reproduction command
builds the circuit executable and its tests, runs the reference comparisons, and
writes results, plots and source identities to a new directory.

In the public audio-core repository, this example is separate from the reusable
libraries and the Barometer plugin. Barometer is a transparent gain and metering
plugin; it does not use this model. Running this example requires neither JUCE
nor a Barometer build. The root library quickstart and the Barometer build do not
execute this experiment. Circuit results establish only the numerical claims in
the circuit contract; they do not establish plugin behavior or hardware fidelity.

## Circuit target and data origin

The target is the ideal circuit specified in the contract, not a particular
compressor, commercial diode part or measured hardware unit. Its component
values are declared inputs to the experiment, not parameters fitted to hardware
recordings. It contains no compressor gain-reduction stage.

```text
Input voltage -- 1 kohm resistor -- diode -- Output voltage
                                              |
                                   1 uF capacitor and
                                   100 kohm resistor
                                   in parallel to ground
```

The diode follows the Shockley equation with saturation current 1e-12 A,
ideality factor 1 and temperature 300 K. Input voltage charges the capacitor
through the resistor and diode; the load resistor discharges it. The capacitor
voltage is the state carried between samples. Excluded physical effects are
listed in the contract.

All numerical inputs and reference traces are generated during reproduction.
No downloaded hardware dataset, music recording or ITU audio file is used.

| Data or result             | Origin                                                               | Location under the chosen output directory                                            |
| -------------------------- | -------------------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| Input voltage waveform     | Deterministic zero, DC turn-on and sine-burst definitions            | `experiment/<case>/source.csv` and `source.inc`                                       |
| Simulator circuit          | Declared components and ngspice settings                             | `experiment/<case>/*.cir`                                                             |
| SPICE reference voltages   | ngspice transient simulation using trap and Gear integration         | `experiment/<case>/trap-*.txt` and `gear-*.txt`; diagnostics in matching `.log` files |
| Native voltages            | C++ backward-Euler model at the tested rates                         | `experiment/<case>/native-*.csv`                                                      |
| Python comparison          | Brent root solving of the same discrete equations for selected cases | `experiment/<case>/python-48000.csv`                                                  |
| Steady-state reference     | Separate DC circuit equation                                         | `reference.log`                                                                       |
| Numerical verdict and plot | Contract checks applied to the generated measurements                | `experiment/results.json` and `experiment/comparison.png`                             |
| Run identity and integrity | Actual sources, tools, binary and retained files                     | `results.json`, `experiment/results.json` and manifest files                          |

Case names are `zero`, `dc`, `burst10`, `burst100`, `burst1000` and `hf18k`.
Python agreement checks another implementation of the same discretization;
analytic DC and SPICE supply different-formulation witnesses. None of these
references is a physical hardware recording. Failed runs can contain partial
evidence; their terminal result identifies the failed stage.

## Source and distribution

The model and reproduction workflow are maintained in Bellweather's internal
monorepo and exported together as this standalone audio-core example. The
public example is not a separately implemented model. Its generated
`source-provenance.json` records canonical and exported file identities;
license/comment/format transformations can make their hashes differ.
Reproduction uses the exported sources and needs no internal checkout.

The source archive supplies the model, scripts, tests, contract and dependency
specifications. Measurements are generated into the requested output directory;
the archive is not a bundle of captured hardware responses. Exporting this
example neither inserts the model into a plugin nor validates a hardware match.

## Reproduce

Use Python 3.12, CMake >= 3.22, Ninja, a C++17 compiler, Catch2 3.7.0 and ngspice 47.
Python dependencies are locked in requirements.lock. The native path can fetch
Catch2 at the commit pinned in cmake/BwsCatch2.cmake; this requires Git and network
access. A source archive needs no Git metadata or private repository. Install
prerequisites explicitly; the reproduction command does not install system tools.

From this example directory, create a virtual environment outside the source:

```sh
python3.12 -m venv /absolute/path/to/circuit-environment
/absolute/path/to/circuit-environment/bin/pip install --require-hashes --only-binary=:all: -r requirements.lock
/absolute/path/to/circuit-environment/bin/python -B reproduce.py /absolute/path/to/new-run
```

On macOS, provision the compiler using Xcode command-line tools; install CMake,
Ninja, Python 3.12 and ngspice 47 before running. Check the simulator version;
using a different version requires fresh reference qualification, not silently
relaxing checks. The public Linux container is also usable through Docker on
macOS or Windows; native Windows execution is not qualified.

From the public source root, the reference container procedure is:

Run this block in Bash from the public repository root. It creates a fresh
external results directory automatically and prints its location:

```bash
set -euo pipefail
BWS_CIRCUIT_RESULTS="$(mktemp -d "${TMPDIR:-/tmp}/bws-diode.XXXXXX")"
printf 'Evidence directory: %s\n' "$BWS_CIRCUIT_RESULTS"
docker build --platform linux/amd64 -t bws-diode-reference examples/diode-detector \
  2>&1 | tee "$BWS_CIRCUIT_RESULTS/image-build.log"
docker image inspect bws-diode-reference > "$BWS_CIRCUIT_RESULTS/image.json"
BWS_CIRCUIT_IMAGE="$(docker image inspect --format '{{.Id}}' bws-diode-reference)"
docker run --rm --platform linux/amd64 --network none \
  -v "$PWD:/source:ro" -v "$BWS_CIRCUIT_RESULTS:/results" \
  "$BWS_CIRCUIT_IMAGE" /results/run \
  2>&1 | tee "$BWS_CIRCUIT_RESULTS/run.log"
```

Expected: exit zero, a `passed:` terminal line and `run/results.json` with
`"outcome": "passed"`. Keep the printed directory. It includes the image identity,
build diagnostics and complete numerical evidence. A measured single run took
about 130 seconds after provisioning; build caches, emulation and host load
change runtime. Three-run qualification retained about 2.34 GB including builds;
the reference image occupied about 385 MB. These are observations, not limits.

Build provisioning needs network access; experiment execution does not. The base
image digest, ngspice source checksum, Catch2 commit and Python wheel hashes pin
those inputs. Direct OS package versions are pinned to the qualified image;
package availability and transitive resolution still depend on mutable feeds.
Retain the resulting image ID and observed tool versions. This does not promise byte-identical image rebuilding.

## Model and decisions

A voltage source drives Rs=1 kohm, an ideal Shockley diode, and an output node
loaded by C=1 uF in parallel with RL=100 kohm. Temperature is 300 K. The contract
specifies saturation current, thermal voltage, exclusions and units explicitly.
Kirchhoff current balance and Shockley's equation define the continuous model.
Backward Euler discretizes capacitor current at the end of each sample interval.
Eliminating current yields a monotone diode-voltage root. Native code uses at most
48 bisections; Python uses Brent's method on the same algebra. This solver diversity
checks implementation agreement, not independent correctness of that algebra.
Analytic DC and ngspice's circuit integration provide different formulations.

For sample interval dt=1/fs, define A=C*fs+1/RL, H=C*fs*v_previous,
R_eff=Rs+1/A, and offset=u-H/A. Solve
`d + R_eff*Is*expm1(d/Vt) - offset = 0`within the bracket`[min(offset,0), max(offset,0)]`. Recover current as
`i=(offset-d)/R_eff`, then `v=(i+H)/A`. The residual is monotone because
its derivative is `1+R_eff*Is\*exp(d/Vt)/Vt > 0`. Thus bisection brackets a
unique root for the declared domain. Stop at a bracket width <= 1e-12 V
or reject after 48 iterations. t=0 records the discharged initial state;
the first processed source sample is at dt. An overflowed positive exponential
still selects the positive side of the bracket; a nonfinite final voltage
rejects without changing state.

The prepare/reset/process interface hides numerical state. Invalid preparation
preserves configuration and state; valid preparation resets the capacitor. Input
rejection holds state. The sample path performs no allocation, I/O or locking.
These structural properties are not a measured host deadline guarantee.

The PWL source is defined on a 768 kHz grid. Both paths consume that same source;
the simulator's actual source is checked before resampling. No fitted alignment,
gain or normalization is applied. Native rates are 44.1/48/96 kHz at 1x/2x/4x.
This is increased numerical integration rate, not a production oversampler.
Trap integration at two step limits and Gear at the finer limit establish observed
reference disagreement. The contract states absolute/relative budgets and the
additional improvement margin. Simulator agreement is not a rigorous error bound.

## Read the evidence

The outer results.json records stages, identities, elapsed time and terminal
outcome. experiment/results.json owns numerical measurements; comparison.png
plots them. Raw signals, netlists, simulator diagnostics, binary hash, source and
contract hashes, controls and logs remain beside the reports. manifest.json hashes
retained evidence. Hashes establish artifact identity, not cross-platform equality.

A nonzero exit means the complete workflow did not pass. Inspect the failed stage
and the experiment verdict: malformed/tool evidence fails; insufficient reference
precision or unresolved improvement is inconclusive; contract mismatch fails.
The expected wrong-capacitance case passes its control only when its error grows.
An actually mutated candidate must fail the unchanged reference comparison.
No plots, successful subprocess, or single comparison can override a non-pass.

## Failure and recovery

Use a new output directory for every attempt. Source remains read-only and build,
cache and result files remain external. Missing tools are errors, not skipped
checks. Raw diagnostics distinguish infrastructure failure from model mismatch.
Renderer/SPICE invocations allow 180 seconds, configuration/build 900 seconds,
and the experiment 3600 seconds, with five seconds for owned-group cleanup.
These limits await qualification on each execution environment; they are not
speed guarantees. SIGINT/SIGTERM are handled; SIGKILL of the owner, detached
processes and external resources are outside the cleanup claim.

Correct the reported cause and rerun into a fresh directory. Keep the original
failed evidence. Do not edit the acceptance budgets to manufacture success.

## Evidence boundary

The experiment demonstrates the declared ideal circuit and tested conditions.
It does not demonstrate hardware fidelity, perceptual equivalence, audio ML,
alias suppression, a production compressor, or plugin/DAW qualification.

## Demonstrate detection and restoration

Run `python -B qualify.py /absolute/path/to/new-controls` from the example
folder. It runs the complete correct candidate, copies the explicit dependency
closure into a separate source directory, doubles the native capacitance,
and requires a native/Python comparison failure. A separate restored copy must
pass. All three runs and source copies remain available. The original source
is never changed. This procedure is intentionally longer than one reproduction.
In the reference container, override the entry point with
`--entrypoint python` and invoke `/source/examples/diode-detector/qualify.py`.
