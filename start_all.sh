#!/bin/bash
set -e
. install.sh
ret=0
tmux -2 attach-session -d || ret=$?
if (( $ret != 0 )); then
  tmux new-session -s "$(basename "$(pwd)")" -d
  tmux split-window -v './run_forever.sh ./scrape_ws_coinbase'
  tmux split-window -h './run_forever.sh ./compress'
  tmux split-window -v './run_forever.sh ./upload'
  tmux -2 attach-session -d
fi
tmux source .tmux.conf
