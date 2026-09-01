import zmq, gymnasium as gym, highway_env
import numpy as np


context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:5555")

env = gym.make("highway-v0", render_mode="human")
env.unwrapped.config.update({
    "observation": {
        "type": "Kinematics",
        "features": ["presence", "x", "y", "vx", "vy"],
        "absolute": True, "normalize": False,
    },
    "action": {"type": "ContinuousAction", "longitudinal": True, "lateral": False},
    "simulation_frequency": 75,  
    "policy_frequency": 25, 
    "screen_width": 400,
    "screen_height": 50, 
    "vehicles_count": 15,
    "lanes_count": 3,
    "duration": 120, 
})

obs, info = env.reset()

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