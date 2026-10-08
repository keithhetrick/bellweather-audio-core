# G384 gain prediction contract

## Summary

G384-1 through G384-7 specify a fixed behavioral-model evaluation, not a
production compressor or circuit reconstruction. The question is whether the
frozen calibrated model improves both MAE and RMS gain error on each of three
recordings excluded from its development fit. No threshold here establishes
absolute hardware fidelity, audibility, uncertainty or statistical significance.
This contract graduates the previously frozen local experiment without retuning.

## G384-1: Model and observable

At sample rate f, p=max(abs(left),abs(right)), L=20*log10(max(p,1e-12)),
q=(1-1/R)*max(L-T,0). Initial attenuation a[-1]=0. For each sample choose
s=attack when q>a[n-1], otherwise release; c=exp(-1/(f*s));
a[n]=c*a[n-1]+(1-c)\*q; output gain g[n]=makeup-a[n], in dB.
Time constants are seconds; T is peak dBFS. Mono supplies a zero right channel.
Fresh renderer invocation resets state. There is no knee, circuit solver,
feedback detector, saturation or oversampling. G384-7 applies predicted gain
to input audio; it does not model the complete analog audio path.

The fixed nominal and fitted parameters are in candidate.json. Candidate data
is configuration, not executable code. The renderer admits finite arguments,
one or two channels, positive sample rate/time constants and ratio >=1; rejects
empty, truncated or nonfinite samples and malformed arguments. Transport is
little-endian IEEE float64, interleaved input and mono gain output, one output
per frame. Any child failure invalidates its output, including partially written
bytes; preexisting output is rejected before invoking a renderer.

## G384-2: Reference custody

The target is the publisher's measured SSL Logic FX G384 at setting p063,
not all SSL compressors. dataset.json binds nine files to a repository revision,
byte counts and SHA-256 values. These are three stereo input/mono CV/recorded stereo output triples for
clip_00131, clip_00143 and clip_00209. Each has 1,323,000 frames at 44,100 Hz.
24-bit input PCM is decoded without normalization; CV is signed gain in dB,
not audio. Hash, framing, rate, finite-value and exact record-set checks precede
comparison. A corrupt file is never silently repaired or dropped.

Download at most the declared nine bodies, limiting each to expected bytes+1,
60 seconds per socket operation, 3,600 seconds for the complete run. Retain
partial files on failure. Offline reuse copies and verifies the same inputs;
no network operation is required when --data is supplied. No acquired music
belongs in source archives or uploaded CI evidence.

## G384-3: Metrics and verdict

Render entire inputs separately from zero state. Compare g[n] to CV[n+2] for
n=0..N-3, preserving startup. No target-informed normalization, delay search,
warmup exclusion or fitting. Report full-record and consecutive 44,100-frame
window MAE, RMS, peak absolute and signed mean error, retaining the final partial
window. Report constant makeup as a descriptive third comparator.

Each file improves only when fitted MAE < nominal MAE AND fitted RMS < nominal
RMS. All three must improve for improved_on_all_records. Equality or any adverse
metric produces mixed_or_not_improved. Execution or integrity failure has
precedence: evidence_error. Exits are respectively 0, 2 and 1. No averaging across
files can override an adverse file. Missing or duplicate IDs cannot pass.

Zero residual must measure zero; [3,-4] must measure MAE 3.5, RMS sqrt(12.5),
peak 4, bias -0.5 within 1e-12 dB. +6 offset measures 6; 6 versus -6 measures 12.
Empty, unequal, multidimensional and nonfinite arrays are inadmissible.

The comparison interface accepts complete equal-length gain/CV records with
at least three samples. It applies the fixed two-sample offset exactly once.
A known-answer ramp with CV[n+2]=g[n] must give zero full-record and window
errors; moving CV one sample forward or backward must expose a 1 dB residual
for a 1 dB/sample ramp. No data-dependent alignment is allowed.
For 44,103 input frames the aligned record has 44,101 frames: windows start
at 0 and 44,100 with lengths 44,100 and 1. An impulse at the last frame of
the first window and a differently sized impulse in the final partial window
must retain their independently computed metrics. These controls protect
startup, boundary ownership, lag sign and the final partial window.

## G384-4: Native and transport controls

Before reading reference data, verify the native implementation against a
closed-form step/decay: 4,410 ones then 4,410 zeros, f=44100, T=-24, R=4,
attack=.01, release=.3, makeup=0. During the step a[k]=18*(1-exp(-k/441));
during decay a[k]=a[end]*exp(-k/13230), with k starting at 1. Gain is -a.
Maximum error must be <1e-9 dB. Silence must equal fixed makeup within 1e-12.
Left-only and right-only stereo must equal mono; a fresh invocation must agree
byte-for-byte. Reject truncated and nonfinite binary input.

In a disposable source copy, double both native time constants. The analytic
oracle must detect error >=1e-9; restored source must pass the original bound.
A failed child that wrote apparently valid bytes, and a successful child that
wrote no output, must both be rejected. These integration controls detect native
math, parameter mapping and process/output transport failures. Component metric
checks alone cannot establish them. The mutation does not establish sensitivity
to every conceivable model defect.

## G384-5: Lifetime and recovery

Output is a new absent directory outside source. Reject existing directories
without changes. Scientific imports occur after evidence ownership is established
so missing dependencies leave an actionable failed record. Source and input
hashes must remain unchanged across the run. Bytecode/build/temp/dependency
outputs must not enter source. Configure/build <=900s each, renderer <=180s,
run <=3600s, child cleanup grace <=5s. Record commands and logs.

Real child success, timeout and SIGINT/SIGTERM interruption controls, including
a child with an active same-group descendant, must
establish process absence after cleanup. Time interruption together with active
child ownership is the relevant overlap; no mutable realtime parameter/reset
API is exposed. Detached descendants and forced owner termination are excluded.
Failed evidence is retained; recovery always uses a fresh directory.
Qualification interrupts the actual reproduction CLI while a real child is
active. It must exit 1, retain evidence_error with interruption diagnostics,
and leave the observed child absent within the five-second cleanup grace.
Readiness must be observed, not inferred from a fixed sleep. Failure to observe
the overlap is qualification failure, never a skipped passing control.

## G384-6: Qualification and evidence limits

The qualification command preserves the numerical exit: 0 for all-record
improvement, 2 for a valid adverse outcome, and 1 for qualification failure.
A `qualified` instrumentation receipt cannot turn an adverse hardware result
into a successful CI exit.

Two complete runs from the same sources/data/environment must produce identical
numerical records and gain bytes. Source archive qualification runs from a path
containing spaces, with no Git metadata or private research files. Pin Python
dependencies, record compiler/system versions, and retain provenance through
actual export transformations. Changed identities require new evidence.
Reference rejection includes both truncated data and an altered byte with
unchanged file length. The latter must fail at reference acquisition, proving
hash enforcement independently of size enforcement. The valid cache remains
unchanged and supplies fresh-directory recovery.

Integration carries the main evidence. Component tests qualify metric/verdict
instruments. A few full workflows qualify archive, replay, failure and recovery.
The known model/control, negative witnesses and scope above are the acceptance
sources; previous measured hardware numbers are observations, not golden bounds.
Both favorable and unfavorable hardware outcomes must be retained.

A public example proves this fixed comparison only. It does not rerun parameter
estimation, identify physical components, qualify Pressure/Barometer, establish
full audio fidelity or establish realtime performance. The three recordings
are now consumed evaluation material; future tuning needs new independent data.

## Decoder diagnostics

The pinned CV WAV files contain a PEAK metadata chunk unused by the gain
comparison. SciPy's exact `Chunk (non-data) not understood, skipping it.` warning
is admitted for these hash-verified CV files only and retained in decoding.json.
Other warnings fail decoding. Input PCM must decode exact full-scale fractions:
24-bit integers [-8388608, 0, 4194304] represent [-1, 0, .5]. Float CV values
[-6,0,.25] remain those dB values without scaling. These independent transport
controls and wrong-format/rate controls qualify the WAV-to-double boundary.

## G384-7: Reproducible listening comparison

Every complete run writes four 44,100 Hz stereo float32 WAV files per record:
original input, fitted model gain applied to input, recorded hardware output,
and recorded hardware gain applied to input. All contain the first N-2 input
frames. Model reconstruction is x[n]*10^(g[n]/20). Hardware gain reconstruction
is x[n]*10^(CV[n+2]/20), using the comparison's fixed offset exactly once.
Recorded hardware audio is truncated to the common length without delay fitting,
resampling or normalization. It includes the recorded analog path and acquisition
chain; no sample-alignment or null-test claim is made about that audio track.
CV is never written or presented as playable audio.

Preserve unscaled reconstructions in measurement/. Separate listen/ copies use
one constant gain per entire stereo record. Match whole-record RMS to the
quietest track, then apply common attenuation if needed so every sample peak
is <= 10^(-3/20). No boosts, limiting, dynamic normalization or channel-independent
gains. This is RMS matching, not perceptual loudness or true-peak qualification.
Reject silence, nonfinite samples, unequal shapes and invalid gain framing.
Record applied gains, frame counts and WAV hashes; every emitted WAV must decode
at the declared rate and equal the intended float32 samples. Failure invalidates
the run; listening generation never upgrades the numerical verdict.

Component controls use stereo [1,-1] with 0 dB and -20 dB gains to require
unity and one-tenth amplitude, and a distinct CV prefix to expose wrong lag.
Constant full-scale and half-scale tracks must match to half scale. A lone
full-scale track must reach the declared peak ceiling. Invalid shape, NaN and
silence controls establish rejection. These qualify the arithmetic, but cannot
prove native gain transport. The complete reproduction command additionally
reconstructs from the actual native gain bytes and acquired references, reads
back every WAV, and full recovery qualification requires exact WAV hashes on
replay. Qualification independently checks each labeled WAV against input, native
gain and hardware records. A deliberate substitution of dry input for the model
track must fail that check; restoring the exact artifact must pass. Listening is descriptive; neither a pleasant sound nor exact replay
establishes hardware fidelity or a blinded perceptual result.
