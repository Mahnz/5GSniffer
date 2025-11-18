# app/zmq_subscriber.py
import os
import json
import zmq
import zmq.asyncio
from typing import Optional
from .schemas import RntiSnapshot, RntiDelta


class ZmqSubscriber:
    """
    Async SUB socket. Supports both single-frame and multi-part PUB messages.
    If a topic is used, last frame is expected to be the JSON payload.
    """

    def __init__(self, endpoint: Optional[str] = None, topic: Optional[bytes] = None):
        self.endpoint = endpoint or os.getenv("ZMQ_ENDPOINT", "tcp://127.0.0.1:5557")
        self.ctx = zmq.asyncio.Context.instance()
        self.sock = self.ctx.socket(zmq.SUB)
        self.sock.setsockopt(zmq.RCVHWM, 100000)
        self.sock.setsockopt(zmq.LINGER, 0)

        sub = topic if topic is not None else os.getenv("ZMQ_TOPIC", "").encode()
        self.sock.setsockopt(zmq.SUBSCRIBE, sub)
        self.sock.connect(self.endpoint)

        print(f"[zmq] SUB connected to {self.endpoint!r} topic={sub!r}")

    async def recv(self) -> Optional[RntiDelta]:
        try:
            parts = (
                await self.sock.recv_multipart()
            )  # awaitable; doesn't block the loop
            payload = parts[-1]  # last frame is the JSON
        except Exception:
            return None

        try:
            obj = json.loads(payload.decode("utf-8"))
        except Exception:
            return None

        try:
            t = float(obj.get("t_seconds"))
            snap = RntiSnapshot(
                rnti=int(obj["rnti"]),
                cell_id=obj.get("cell_id"),
                scrambling_id=obj.get("scrambling_id"),
                coreset_id=obj.get("coreset_id"),
                t_seconds=t,
                sample_index=obj.get("sample_index"),
                seen_count=int(obj.get("seen_count", 0)),
                revivals=int(obj.get("revivals", 0)),
                status=obj.get("status", "active"),
                last_seen=t,
                first_seen=obj.get("first_seen", t),
            )
            return RntiDelta(event=obj.get("event", "update"), snapshot=snap)
        except Exception:
            return None
