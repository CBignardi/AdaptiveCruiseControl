#!/bin/bash
SESSION="acc"

PYTHON_SERVER_DIR="../python_server"          # Directory with env_server.py
CPP_EXE_DIR="bin"                             # Direcotry with the executable file
CPP_EXECUTABLE="ACC"                          # executable file name

# If the session exits, I'll close it before creating a new one.
tmux kill-session -t $SESSION 2>/dev/null

tmux new-session -d -s $SESSION -n main

# Pannel 0: server Python
tmux send-keys -t $SESSION:0 "cd $PYTHON_SERVER_DIR && source venv/bin/activate && python env_server.py" C-m

sleep 2  # Some time for the server to start

# Vertical Split, Pannel 1: client C++
tmux split-window -h -t $SESSION:0
tmux send-keys -t $SESSION:0.1 "cd $CPP_EXE_DIR && ./$CPP_EXECUTABLE" C-m

tmux attach -t $SESSION