import zmq, gymnasium as gym, highway_env
import numpy as np


context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:5555")

env = gym.make("highway-v0", render_mode="human")
env.unwrapped.config.update({
    "observation": {
        "type": "Kinematics", "vehicles_count": 6,
        "features": ["presence", "x", "y", "vx", "vy"],
        "absolute": True, "normalize": False,
    },
    "action": {"type": "ContinuousAction", "longitudinal": True, "lateral": False},
    "controlled_vehicles": 1,
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