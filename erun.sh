#!/bin/bash
# SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
SCRIPT_DIR=./build/
"$SCRIPT_DIR"/ec -O3 "$@" | "$SCRIPT_DIR"/eexec -
