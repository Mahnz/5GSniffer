import asyncio
from typing import Set
from fastapi import WebSocket
from .schemas import RntiDelta


class WebSocketManager:
    def __init__(self):
        self.active_connections: Set[WebSocket] = set()
        self._queue: asyncio.Queue[RntiDelta] = asyncio.Queue()

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.add(websocket)

    def disconnect(self, websocket: WebSocket):
        self.active_connections.discard(websocket)

    async def enqueue(self, delta: RntiDelta):
        await self._queue.put(delta)

    async def broadcaster(self, interval_ms: int = 500):
        """Periodically drain the queue and fan out to all clients."""
        while True:
            await asyncio.sleep(interval_ms / 1000.0)
            batch = []
            try:
                while True:
                    item = self._queue.get_nowait()
                    batch.append(item)
            except asyncio.QueueEmpty:
                pass

            if not batch:
                continue

            data = [
                {"event": d.event, "snapshot": d.snapshot.model_dump()} for d in batch
            ]
            stale = []
            for ws in list(self.active_connections):
                try:
                    await ws.send_json({"type": "batch", "items": data})
                except Exception:
                    stale.append(ws)
            for s in stale:
                self.disconnect(s)
