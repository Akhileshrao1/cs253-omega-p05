#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

python_command="${PYTHON:-python3}"
"$python_command" bench/run.py --all "$@"

for argument in "$@"; do
    case "$argument" in
        -h|--help|--build-only|--case|--case=*) exit 0 ;;
    esac
done

exec "$python_command" results/plot.py results
