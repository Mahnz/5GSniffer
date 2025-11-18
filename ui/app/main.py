# app/main.py
import os
import asyncio
import time
from typing import Optional
from contextlib import asynccontextmanager

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from .state import RntiState
from .zmq_subscriber import ZmqSubscriber
from .websocket_manager import WebSocketManager
from .schemas import SnapshotResponse, RntiDelta

STATE = RntiState(ttl_seconds=float(os.getenv("RNTI_TTL_SECONDS", "30")))
SUB = ZmqSubscriber(os.getenv("ZMQ_ENDPOINT"))
WS = WebSocketManager()


@asynccontextmanager
async def lifespan(app: FastAPI):
    # Startup
    print(
        f"[viewer] TTL={STATE.ttl} ZMQ_ENDPOINT={os.getenv('ZMQ_ENDPOINT', 'tcp://127.0.0.1:5557')}"
    )
    loop = asyncio.get_event_loop()
    loop.create_task(_zmq_loop())
    loop.create_task(_expiry_loop())
    loop.create_task(
        WS.broadcaster(interval_ms=int(os.getenv("WS_BROADCAST_INTERVAL_MS", "500")))
    )
    yield
    # Shutdown (if needed)


app = FastAPI(title="rt-rnti-viewer", version="1.0.0", lifespan=lifespan)


@app.get("/healthz")
def healthz():
    return {"ok": True, "ttl": STATE.ttl}


@app.get("/snapshot", response_model=SnapshotResponse)
def get_snapshot():
    now, active, rec_exp, stats = STATE.snapshot()
    return {"now": now, "active": active, "recently_expired": rec_exp, "stats": stats}


@app.get("/", response_class=HTMLResponse)
def index():
    with open(
        os.path.join(os.path.dirname(__file__), "..", "static", "index.html"),
        "r",
        encoding="utf-8",
    ) as f:
        return HTMLResponse(f.read())


@app.websocket("/ws")
async def ws_ep(ws: WebSocket):
    await WS.connect(ws)
    try:
        # Keep the connection alive; we push from the broadcaster task.
        while True:
            await asyncio.sleep(60)
    except WebSocketDisconnect:
        WS.disconnect(ws)


# ----------------- Background tasks -----------------


async def _zmq_loop():
    # Pure await: no blocking calls, no busy spin
    while True:
        delta: Optional[RntiDelta] = await SUB.recv()
        if delta is not None:
            STATE.upsert(delta)
            await WS.enqueue(delta)


async def _expiry_loop():
    while True:
        due = STATE.expire_due()
        now = time.time()
        if due:
            for r in due:
                snap = STATE.mark_expired(r, now)
                if snap is not None:
                    await WS.enqueue(RntiDelta(event="expire", snapshot=snap))
        await asyncio.sleep(STATE.ttl / 4.0)


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(
        "app.main:app",
        host=os.getenv("HOST", "0.0.0.0"),
        port=int(os.getenv("PORT", "8088")),
        reload=False,
        access_log=False,
    )
