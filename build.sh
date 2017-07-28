#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LINKFLAGS_EXTRA="-L."
export LDFLAGS_EXTRA="-lmatio"

cd "$DIR"
$DIR/waf build
