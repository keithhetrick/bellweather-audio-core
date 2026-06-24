#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_PREFIX="${IMAGE_PREFIX:-bellweather-audio-core}"
HEARTBEAT_SECONDS="${HEARTBEAT_SECONDS:-60}"
TAIL_LINES="${TAIL_LINES:-120}"
LOG_ROOT="${LOG_ROOT:-${TMPDIR:-/tmp}/bellweather-audio-core-docker-logs}"
bws_docker_build_args=()

if [[ -f "${ROOT}/tools/ci/docker/image_identity.sh" ]]; then
    # shellcheck source=../../ci/docker/image_identity.sh
    source "${ROOT}/tools/ci/docker/image_identity.sh"
else
    bws_prepare_docker_build_args() {
        local root="$1"
        local revision created arch
        revision="$(git -C "${root}" rev-parse HEAD 2>/dev/null || printf 'unknown')"
        created="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
        arch="${BWS_IMAGE_ARCH:-${DOCKER_DEFAULT_PLATFORM:-unknown}}"
        bws_docker_build_args=(
            --build-arg "BWS_IMAGE_REVISION=${revision}"
            --build-arg "BWS_IMAGE_CREATED=${created}"
            --build-arg "BWS_IMAGE_ARCH=${arch}"
        )
    }
fi
bws_prepare_docker_build_args "${ROOT}"

library_lanes=(
    clang-fetch
    gcc-fetch
    gcc-system-catch2
)

plugin_lanes=(
    barometer-clang
)

lanes=("${library_lanes[@]}")
if [[ "${RUN_PLUGIN_LANES:-0}" == "1" ]]; then
    lanes+=("${plugin_lanes[@]}")
fi

format_elapsed() {
    local seconds="$1"
    printf '%02d:%02d:%02d' "$((seconds / 3600))" "$(((seconds % 3600) / 60))" "$((seconds % 60))"
}

run_with_heartbeat() {
    local label="$1"
    shift

    mkdir -p "${LOG_ROOT}"
    local safe_label
    safe_label="$(printf '%s' "${label}" | tr -c '[:alnum:]_.-' '_')"
    local log_file="${LOG_ROOT}/${safe_label}.log"

    local started
    started="$(date +%s)"
    echo "    ${label} started (log: ${log_file})"

    "$@" >"${log_file}" 2>&1 &
    local pid="$!"

    while kill -0 "${pid}" 2>/dev/null; do
        sleep "${HEARTBEAT_SECONDS}"
        if kill -0 "${pid}" 2>/dev/null; then
            local now
            now="$(date +%s)"
            echo "    ${label} still running ($(format_elapsed "$((now - started))") elapsed)"
        fi
    done

    local status=0
    wait "${pid}" || status="$?"
    local now
    now="$(date +%s)"
    if [[ "${status}" -ne 0 ]]; then
        echo "    ${label} failed ($(format_elapsed "$((now - started))") elapsed)"
        echo "    showing last ${TAIL_LINES} lines from ${log_file}:"
        tail -n "${TAIL_LINES}" "${log_file}" || true
        return "${status}"
    fi

    echo "    ${label} passed ($(format_elapsed "$((now - started))") elapsed)"
}

total="${#lanes[@]}"
current=0

for lane in "${lanes[@]}"; do
    current="$((current + 1))"
    image="${IMAGE_PREFIX}:${lane}"
    echo "==> Docker lane ${current}/${total}: ${lane}"
    run_with_heartbeat "build ${lane}" docker build "${bws_docker_build_args[@]}" --progress=plain --target "${lane}" -t "${image}" "${ROOT}"
    run_with_heartbeat "check ${lane}" docker run --rm "${image}"
done

echo "==> All requested Docker lanes passed"
echo "==> Logs: ${LOG_ROOT}"
