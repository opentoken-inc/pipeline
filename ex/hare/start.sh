#!/bin/bash
set -e
cd ../../ && . install.sh && cd ex/hare
session_name="$(basename "$(pwd)")"
ret=0
tmux -2 attach-session -d -t "$session_name" || ret=$?
if (( $ret != 0 )); then
  tmux new-session -s "$session_name" -d
  tmux -2 attach-session -d -t "$session_name"
fi
tmux source .tmux.conf
