"""Broadcast model predictions over a local websocket.

The desktop app connects to ws://127.0.0.1:8765 and reads one JSON message
per ticker:
    {"ticker": "AAPL", "prob_bigmove": 0.21, "label": "calm"}

One background sweep re-downloads prices and scores every ticker in
watchlist.txt, then broadcasts the results to all connected clients. New
clients get the latest results immediately. Run after training a model:

    python -m predictor.serve
"""
from __future__ import annotations

import asyncio
import json

import websockets

from .config import CONFIG
from .predict import predict_ticker

HOST = "127.0.0.1"
PORT = 8765
# Daily bars only change once a day (plus today's partial bar), so every 30 min is plenty.
REFRESH_SECONDS = 1800

CLIENTS: set = set()
LATEST: dict[str, str] = {}


async def _handler(websocket):
    CLIENTS.add(websocket)
    try:
        for msg in list(LATEST.values()):
            await websocket.send(msg)
        await websocket.wait_closed()
    finally:
        CLIENTS.discard(websocket)


async def _sweep_forever() -> None:
    while True:
        for ticker in CONFIG.tickers:
            try:
                # Blocking download + model call: keep it off the event loop.
                result = await asyncio.to_thread(predict_ticker, ticker, True)
            except Exception as exc:
                print(f"[serve] {ticker}: {exc}")
                continue
            LATEST[ticker] = json.dumps(result)
            websockets.broadcast(CLIENTS, LATEST[ticker])
        print(f"[serve] sweep done, {len(LATEST)} tickers, {len(CLIENTS)} clients")
        await asyncio.sleep(REFRESH_SECONDS)


async def main() -> None:
    print(f"[serve] prediction feed on ws://{HOST}:{PORT}")
    async with websockets.serve(_handler, HOST, PORT):
        await _sweep_forever()


if __name__ == "__main__":
    asyncio.run(main())
