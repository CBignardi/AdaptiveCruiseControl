import zmq, gymnasium as gym, highway_env
import numpy as np


context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:5555")

env = gym.make("highway-v0", render_mode="human")
env.unwrapped.config.update({
    "observation": {
        "type": "Kinematics", "vehicles_count": 5,
        "features": ["presence", "x", "y", "vx", "vy"],
        "absolute": False, "normalize": False,
    },
    "action": {"type": "ContinuousAction", "longitudinal": True, "lateral": False},
})

obs, info = env.reset()

# FACCIAMOLO FARE AL CONTROLLORE
# def find_leader(obs):
#     """Trova il veicolo più vicino davanti, stessa corsia."""
#     best = None
#     for row in obs[1:]:
#         presence, x, y, vx, vy = row
#         if presence > 0.5 and x > 0 and abs(y) < 2.0:
#             if best is None or x < best[0]:
#                 best = (x, vx, vy)
#     return best  # (distanza, vx_rel, vy_rel) oppure None


while True:
    msg = socket.recv_json()
    if msg["cmd"] == "reset":
        obs, info = env.reset()
        socket.send_json({"obs": obs.tolist()})
    elif msg["cmd"] == "step":
        accel = msg["accel"]  # float, es. -3.0..3.0 m/s^2
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