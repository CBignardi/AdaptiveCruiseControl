#!/bin/bash
SESSION="acc"

# --- MODIFICA QUESTE 3 RIGHE con i tuoi percorsi reali ---
PYTHON_SERVER_DIR="../python_server"          # cartella con env_server.py
 #VENV_DIR="python_server/venv"              # cartella del venv
CPP_BUILD_DIR="bin"                      # cartella con l'eseguibile compilato
CPP_EXECUTABLE="ACC"                       # nome dell'eseguibile (dal CMakeLists)
# -----------------------------------------------------------

# Se la sessione esiste già, la chiude prima di ricrearla (evita conflitti)
tmux kill-session -t $SESSION 2>/dev/null

tmux new-session -d -s $SESSION -n main

# Pannello 0: server Python
tmux send-keys -t $SESSION:0 "cd $PYTHON_SERVER_DIR && source venv/bin/activate && python env_server.py" C-m

sleep 2  # dà tempo al server di avviarsi prima che il client provi a connettersi

# Split verticale, pannello 1: client C++
tmux split-window -h -t $SESSION:0
tmux send-keys -t $SESSION:0.1 "cd $CPP_BUILD_DIR && ./$CPP_EXECUTABLE" C-m

tmux attach -t $SESSION