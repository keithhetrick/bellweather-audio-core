#!/usr/bin/env bash
set -euo pipefail
exec "${BWS_MODELING_PYTHON:-python3}" "$(dirname "${BASH_SOURCE[0]}")/reproduce.py" "$@"
