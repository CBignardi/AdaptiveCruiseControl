import zmq, gymnasium as gym, highway_env
import numpy as np

# Helped by AI 

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:5555")

env = gym.make("highway-v0",
                render_mode="human",
                config={
                    "screen_width": 550,
                    "screen_height": 120,
                })
env.unwrapped.config.update({
    "observation": {
        "type": "Kinematics",
        "features": ["presence", "x", "y", "vx", "vy"],
        "absolute": True,
        "normalize": False,
    },
    "action": {"type": "ContinuousAction", "longitudinal": True, "lateral": False}, # Disable the steering
    "simulation_frequency": 60,
    "policy_frequency": 20,
    "vehicles_count": 15,
    "lanes_count": 3,
    "duration": 60, # [s] Max duration of the simulation
})

while True:
    msg = socket.recv_json()
    if msg["cmd"] == "reset":
        obs, info = env.reset()
        socket.send_json({"obs": obs.tolist()})
    elif msg["cmd"] == "step":
        accel = msg["accel"]
        obs, reward, terminated, truncated, info = env.step([accel])
        socket.send_json({
            "obs": obs.tolist(),
            "terminated": bool(terminated),
            "truncated": bool(truncated),
        })
    elif msg["cmd"] == "close":
        socket.send_json({"ok": True})
        break
    
env.close()