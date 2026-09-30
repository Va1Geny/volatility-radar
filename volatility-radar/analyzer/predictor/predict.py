"""Load the trained model and score tickers for an upcoming big move.

    python -m predictor.predict AAPL MSFT
"""
from __future__ import annotations

import sys

import keras

from .config import CONFIG, MODEL_PATH
from .data_loader import load_prices
from .dataset import build_inference_window

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
    """Return {'ticker', 'prob_bigmove' | 'prob_up', 'label'} for the latest bar.

    For the bigmove task: prob = P(unusually large move within horizon),
    label = "BIG MOVE" if prob >= CONFIG.alert_prob else "calm".
    """
    df = load_prices(ticker, refresh=refresh)
    window = build_inference_window(df)
    prob = float(_get_model().predict(window, verbose=0)[0, 0])

    if CONFIG.task == "bigmove":
        label = "BIG MOVE" if prob >= CONFIG.alert_prob else "calm"
        key = "prob_bigmove"
    else:
        label = "UP" if prob >= 0.5 else "DOWN"
        key = "prob_up"

    return {"ticker": ticker.upper(), key: round(prob, 4), "label": label}


def main() -> None:
    tickers = sys.argv[1:] or CONFIG.tickers
    for t in tickers:
        try:
            print(predict_ticker(t))
        except Exception as exc:
            print(f"{t}: error {exc}")


if __name__ == "__main__":
    main()
