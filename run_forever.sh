#!/bin/bash
cmd="$1"
THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
. "$THIS_DIR"/setup.sh
"$THIS_DIR"/run_loop.sh "$cmd" > >(tee -a "$cmd".stdout.log) 2> >(tee -a "$cmd".stderr.log >&2 )
