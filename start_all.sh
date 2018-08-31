#!/bin/bash
set -e
. install.sh
session_name="$(basename "$(pwd)")"
ret=0
tmux -2 attach-session -d -t "$session_name" || ret=$?
if (( $ret != 0 )); then
  tmux new-session -s "$session_name" -d
  tmux split-window -v './run_forever.sh ./scrape_ws_coinbase'
  tmux split-window -h './run_forever.sh ./compress'
  tmux split-window -v './run_forever.sh ./upload'
  tmux new-window './run_forever.sh ./scrape_ws_bittrex'
  tmux new-window './run_forever.sh ./scrape_ws_binance'
  tmux -2 attach-session -d -t "$session_name"
fi
tmux source .tmux.conf
