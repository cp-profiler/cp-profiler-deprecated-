#!/bin/bash

SCRIPT_DIR=$(dirname "$BASH_SOURCE")
export LD_LIBRARY_PATH=$SCRIPT_DIR:$LD_LIBRARY_PATH
$SCRIPT_DIR/cp-profiler "$@"
