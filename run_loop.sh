#!/bin/bash
cmd="$1"
while true; do
  "$cmd" 
  >&2 echo "$cmd" crashed, restarting
  sleep $(python -c "print(30 + $RANDOM/32000.)")
done
