"""Optional: broadcast model predictions over a local websocket.

The terminal can connect to ws://127.0.0.1:8765 and read JSON lines:
    {"ticker": "AAPL", "prob_up": 0.57, "direction": "UP"}

This is the prediction channel — separate from the Finnhub live-price
channel the C++ terminal already consumes. Run after training a model:

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
REFRESH_SECONDS = 300


async def _handler(websocket):
    """Push a fresh prediction sweep to one client on a loop."""
    try:
        while True:
            for ticker in CONFIG.tickers:
                try:
                    msg = predict_ticker(ticker)
                except Exception as exc:
                    msg = {"ticker": ticker, "error": str(exc)}
                await websocket.send(json.dumps(msg))
            await asyncio.sleep(REFRESH_SECONDS)
    except websockets.ConnectionClosed:
        pass


async def main() -> None:
    print(f"[serve] prediction feed on ws://{HOST}:{PORT}")
    async with websockets.serve(_handler, HOST, PORT):
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main())
