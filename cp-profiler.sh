#!/bin/bash

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
SCRIPT_DIR=$(dirname $(cd "$(dirname "$BASH_SOURCE")"; pwd))
$SCRIPT_DIR/cp-profiler "$@"
