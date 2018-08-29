#!/bin/bash
cmd="$1"
./run_loop.sh "$cmd" > >(tee -a "$cmd".stdout.log) 2> >(tee -a "$cmd".stderr.log >&2 )
