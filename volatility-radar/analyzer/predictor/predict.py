"""Load the trained model and score tickers for an upcoming big move.

    python -m predictor.predict AAPL MSFT
"""
from __future__ import annotations

import sys

import keras

from .config import CONFIG, MODEL_PATH
from .data_loader import load_earnings, load_prices
from .dataset import build_inference_window, completed_bars

HISTORY_BARS = 126  # ~6 months of closes for the app's price chart

_model: keras.Model | None = None


def _get_model() -> keras.Model:
    global _model
    if _model is None:
        if not MODEL_PATH.exists():
            raise FileNotFoundError(
                f"No model at {MODEL_PATH}. Train first: python -m predictor.train")
        _model = keras.models.load_model(MODEL_PATH)
    return _model


def predict_ticker(ticker: str, refresh: bool = False) -> dict:
    """Return {'ticker', 'prob_bigmove' | 'prob_up', 'label', 'closes'} for the latest finished bar.

    For the bigmove task: prob = P(unusually large move within horizon),
    label = "BIG MOVE" if prob >= CONFIG.alert_prob else "calm".
    closes = the last ~6 months of daily closes, oldest first.
    """
    df = completed_bars(load_prices(ticker, refresh=refresh))
    window = build_inference_window(df, load_earnings(ticker, refresh=refresh))
    prob = float(_get_model().predict(window, verbose=0)[0, 0])

    if CONFIG.task == "bigmove":
        label = "BIG MOVE" if prob >= CONFIG.alert_prob else "calm"
        key = "prob_bigmove"
    else:
        label = "UP" if prob >= 0.5 else "DOWN"
        key = "prob_up"

    closes = [round(float(c), 2) for c in df["Close"].iloc[-HISTORY_BARS:]]
    return {"ticker": ticker.upper(), key: round(prob, 4), "label": label, "closes": closes}


def main() -> None:
    tickers = sys.argv[1:] or CONFIG.tickers
    for t in tickers:
        try:
            print({k: v for k, v in predict_ticker(t).items() if k != "closes"})
        except Exception as exc:
            print(f"{t}: error {exc}")


if __name__ == "__main__":
    main()
