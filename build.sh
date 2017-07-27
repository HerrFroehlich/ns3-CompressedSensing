#!/bin/bash

export LINKFLAGS_EXTRA="-L."
export LDFLAGS_EXTRA="-lmatio"
./waf build
