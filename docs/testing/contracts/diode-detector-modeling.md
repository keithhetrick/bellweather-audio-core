# Diode detector modeling experiment

> **Status:** Experimental circuit tool contract; not a product specification
> **Domain:** dsp, testing
> **Content Provenance:** hand-authored
> **Last Verified:** 2026-10-07

## Summary

This experiment demonstrates circuit-to-digital modeling. No
production engine consumes this model. Reference agreement is not hardware
equivalence. This contract precedes the implementation and acceptance checks.

## DDM-1: Circuit

Source u, Rs=1000 ohm, forward Shockley diode, then output v with C=1e-6 F
and RL=100000 ohm to ground. Is=1e-12 A, ideality=1, temperature=300 K,
Vt=8.617333262145e-5\*300 V. Exclude junction capacitance, breakdown, package
resistance and component tolerances. SPICE temp/tnom are both 26.85 Celsius.
This refines the earlier research sketch's rounded 25.85 mV value.

```
C dv/dt = i - v/RL
i = Is * expm1((u - Rs*i - v)/Vt)
```

The native model implements backward Euler with the input at the END of each
interval. Initial voltage is zero. Python solves the discrete equations;
SPICE independently integrates the circuit. Port parity is not model accuracy.

## DDM-2: Interface and containment

Prepare accepts finite rates [8000,768000] Hz and capacitance [1e-9,1e-3] F.
Valid prepare resets state. Invalid prepare returns false without changing
configuration or state. Inputs must be finite and in [-4,4] V. Invalid input
holds state/output and returns accepted=false, iterations=0. Reset discharges
the capacitor. Zero input from reset returns exactly zero. Reset immediately
followed by rejected input must remain zero.

A root solve uses at most 48 iterations and terminates when the diode-voltage
bracket is <=1e-12 V. Failure holds state and reports accepted=false. The
sample method is noexcept and has no allocation, locks, I/O or logging. This
is a structural bound, not a measured DAW deadline or RT qualification.

## DDM-3: Independent witnesses

For steady u=2 V, DC output is the unique positive root of
`u-v-Rs*v/RL-Vt*log1p(v/(RL*Is))=0`. After 0.5 s output must be within
1e-5 V of that root. Python computes this independent DC condition, not the
native transient solution. Zero input is the other known-good witness.

Measurement rejects nonfinite or mismatched signals. Identity measures zero;
a constant 0.01 V difference measures 0.01 V within 1e-12 V. Native/Python
port error must be <=1e-9 V (numerical, not perceptual tolerance).

## DDM-4: Reference experiment

Inputs: zero; 2 V DC turn-on; 2 V peak 1 kHz bursts lasting 10/100/1000 ms,
each with 10 ms pre-roll and 200 ms recovery; an 18 kHz, 100 ms burst.
Share PWL source samples at 768 kHz between simulator and digital renderer.
The simulator may evaluate the same linear interpolation algebraically to
avoid large-table lookup costs. Its exported input must match the declared
PWL at the simulator's original timestamps within 1e-6 V, a numerical budget
below the 1e-4 V reference budget. A 0.01 V source offset must fail this check.
Validate before output-grid resampling, which has separate interpolation error.
Native input interpolates that exact source. Output at t=0 is zero; updates
begin at t=1/fs. No fitted alignment, normalization or gain.

Digital rates: 44.1/48/96 kHz with 1x/2x/4x integration rates. This is a
known-source numerical experiment, not a qualified audio oversampler. Report
all errors. For 1 kHz bursts, 4x RMS error must be smaller than 1x at each
base rate. This tests convergence, not full-band alias suppression.

SPICE: trap maxstep 2 us and 0.5 us; Gear maxstep 0.5 us. Compare on a 96 kHz
grid. Max reference disagreement must be <=1e-4 V, one tenth of a declared
1 mV reporting resolution. For each nonzero burst, reference disagreement
must also be <=10% of the base-rate digital RMS error before accepting its
convergence claim. Insufficient reference precision is inconclusive.

Double C only in the digital candidate. The 100 ms, 1 kHz burst RMS error must
exceed correct-candidate error by >=2x. This detects a wrong circuit while
preserving an independent reference. Invalid configuration/input and
reset/rejection overlap test containment. Runtime parameter automation is
outside this fixed-circuit interface.

## DDM-5: Evidence and limits

An output directory must be absent. Retain stimuli, netlists, raw outputs,
versions, source/binary hashes, commands, metrics, plots and terminal state.
Failed subprocess or missing simulator is failure, never skipped success.
Checks remain active with Python optimization. Failed runs are not overwritten.

Physical hardware, spectral alias qualification, stereo, compressor gain
laws, fitting, production-plugin integration and CPU deadlines are outside this first
slice. The broader nonlinear-extension research design remains a roadmap.

## DDM-6: Reproduction and portability

The shared Python entry point owns dependency checks, standalone CMake build,
interface/metrology tests, real-tool calibration, reference experiment and
terminal evidence. The shell entry delegates to it. All output/build/cache paths
must be outside the source tree; an existing output directory is rejected without
mutation. Source archives require no Git metadata. Missing tools are failure,
never skipped success. Source identity must remain unchanged across a run.

## DDM-7: Resolved improvement and outcome precedence

Python uses an independent solver of the same discrete equations; analytic DC
and SPICE are different-formulation witnesses. Let e1/e4 be the DDM-4 RMS errors
and U the maximum absolute disagreement between its SPICE references. e4 >= e1
fails. A positive improvement <= 2U is inconclusive; improvement > 2U is resolved
only when the existing absolute/relative reference budgets pass. The factor two
allows either compared error to move by U (triangle inequality). U is measured
simulator disagreement, not a rigorous bound on the true solution.

Malformed/missing/nonfinite evidence or tool failure fails the workflow without
asserting a model mismatch. Otherwise any contract failure wins over inconclusive;
only all-complete checks pass. Equality at 2U is inconclusive.

## DDM-8: Process lifetime and simulator calibration

Own a process group for each invocation. Renderer/SPICE timeout is 180 seconds;
configure/build timeout is 900 seconds; experiment timeout is 3600 seconds.
Timeout or SIGINT/SIGTERM terminates the owned group and proves disappearance
within five seconds. Preserve logs and a failed terminal record. Detached
processes, external resources and SIGKILL of the owner are outside this guarantee.
Recovery uses a new directory. These are operational limits, not speed claims.

The supported simulator is ngspice 47. Execute known-good, invalid circuit,
restored and warning circuits using that tool. Check numerical output independently
and retain diagnostics. Error, convergence and unknown warnings are non-pass.
Only explicitly documented warning controls may admit a recognized warning;
normal experiment execution admits no warnings. Empty/incomplete outputs fail.
Known bad controls must actually activate their named diagnostic boundary.

## DDM-9: Export custody and detection power

The export uses an explicit circuit file closure, with canonical/exported hashes
recorded after public source transforms. It preserves contract references. Each
run hashes the actual closure, contract, dependency locks, build helper and binary.
A source archive must run without private sources, Git metadata, or writable source.
Output hashes attest identity, not numerical equivalence across toolchains.

A disposable source mutation doubling effective native capacitance must make the
full reproduction command fail against unchanged reference checks. Restoring the
source must pass in a new directory. This differs from the DDM-4 expected wrong-C
control, whose harness passes when it detects the wrong circuit.

## Test admission and witnesses

| Contract | Risk / smallest sufficient level                       | Control and detectable failure                                                                                                                    |
| -------- | ------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| DDM-2    | Interface state corruption; component                  | Accepted endpoints, outside/nonfinite rejection; unaffected continuation twin detects rejected preparation changing state; valid reprepare resets |
| DDM-3/7  | Biased measurement/verdict; component                  | Analytic zero/span; resolved/unresolved/equality/reversed examples; removing uncertainty comparison certifies unresolved improvement              |
| DDM-1/4  | Wrong circuit/solver; integration                      | Analytic DC, SPICE and native outputs; doubled candidate C fails unchanged Python comparator                                                      |
| DDM-8    | False simulator pass; real-tool integration            | Good/bad/restored/warning circuits with numeric and diagnostic witnesses                                                                          |
| DDM-6/8  | Orphan child or false success; process integration     | Success, live child timeout and owner interruption; child disappearance plus fresh-run recovery                                                   |
| DDM-6/9  | Private dependency / stale artifact; complete workflow | Actual archive read-only, no Git, spaces, external outputs; missing dependency and source mutation non-pass                                       |

Assertions are specified from these contracts before scaffolding. Runtime
parameter automation is absent; its overlap is outside this fixed-circuit API.
Reset/rejection and rejected-prepare/continuation interactions are included.
