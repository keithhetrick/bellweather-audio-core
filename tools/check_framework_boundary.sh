#!/usr/bin/env bash
# Verifies that framework-specific JUCE types stay above the core library
# boundary. The Barometer plugin still uses JUCE through bw_juce_adapters.
# Run directly, or via `ctest -L framework-boundary`.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

modules=(
    bw_core bw_audio_types bw_state_types bw_ports bw_rt
    bw_fft_adapters bw_dsp_core bw_dsp_metering bw_dsp_processors
)

dirs=()
for m in "${modules[@]}"; do
    for sub in include src; do
        [ -d "${root}/modules/${m}/${sub}" ] && dirs+=("${root}/modules/${m}/${sub}")
    done
done

# Ignore comments so examples and explanatory text do not count as framework
# usage. Surviving hits are code-level boundary violations.
hits="$(grep -RInE 'juce::|#include[[:space:]]*[<"]juce' "${dirs[@]}" \
            --include='*.h' --include='*.hpp' --include='*.cpp' 2>/dev/null \
        | sed -E 's://.*$::; s:/\*[^*]*\*+([^/*][^*]*\*+)*/::g' \
        | grep -vE ':[[:space:]]*\*' \
        | grep -E 'juce::|#include[[:space:]]*[<"]juce' || true)"

if [ -n "${hits}" ]; then
    echo "FAIL: framework-specific JUCE code crossed into core modules:" >&2
    echo "${hits}" >&2
    exit 1
fi

echo "PASS: core framework boundary holds (${#modules[@]} modules checked)."
