"""Turn per-ticker feature frames into windowed (X, y) arrays for the LSTM.

Split is chronological per ticker with a `horizon`-row gap so labels never
see validation prices. Scaling is fit on the training portion only and persisted to SCALER_PATH
(plain JSON: per-feature mean/std) so predict.py can reuse it without
re-reading the training data.
"""
from __future__ import annotations

import json

import numpy as np
import pandas as pd

from .config import CONFIG, SCALER_PATH
from .features import build_features


def _windows(arr: np.ndarray, labels: np.ndarray, window: int):
    """Slide a window of length `window` over rows; label = row at window end."""
    X, y = [], []
    for i in range(window, len(arr)):
        X.append(arr[i - window:i])
        y.append(labels[i])
    if not X:
        return np.empty((0, window, arr.shape[1])), np.empty((0,))
    return np.asarray(X, dtype="float32"), np.asarray(y, dtype="float32")


def _fit_scaler(mat: np.ndarray) -> dict:
    mean = mat.mean(axis=0)
    std = mat.std(axis=0)
    std[std == 0.0] = 1.0
    return {"mean": mean.tolist(), "std": std.tolist(),
            "cols": CONFIG.feature_cols}


def _apply_scaler(mat: np.ndarray, scaler: dict) -> np.ndarray:
    mean = np.asarray(scaler["mean"], dtype="float32")
    std = np.asarray(scaler["std"], dtype="float32")
    return (mat - mean) / std


def save_scaler(scaler: dict) -> None:
    SCALER_PATH.write_text(json.dumps(scaler, indent=2))


def load_scaler() -> dict:
    return json.loads(SCALER_PATH.read_text())


def build_training_set(price_frames: dict[str, pd.DataFrame],
                       earnings: dict[str, pd.DatetimeIndex] | None = None,
                       val_start: float = 1.0 - CONFIG.val_split, val_end: float = 1.0,
                       save: bool = True):
    """Build (X_train, y_train, X_val, y_val) across all tickers.

    Per ticker, rows before `val_start` (fraction of its history) train and rows in
    [val_start, val_end) validate; the default is the last `val_split` of history.
    Split is chronological (no shuffle), then concatenated. The scaler is fit on
    training rows only and saved unless save=False (walk-forward folds).
    """
    window = CONFIG.window
    earnings = earnings or {}
    train_feat_rows = []
    per_ticker = []

    for ticker, df in price_frames.items():
        feats = build_features(df, earnings.get(ticker)).dropna(subset=["target"])
        lo, hi = int(len(feats) * val_start), int(len(feats) * val_end)
        if lo - CONFIG.horizon <= window + 1 or hi - lo <= window + 1:
            print(f"[dataset] skip {ticker}: not enough rows ({len(feats)})")
            continue
        # The last `horizon` train labels look into the validation period: drop them.
        train_df = feats.iloc[:lo - CONFIG.horizon]
        val_df = feats.iloc[lo:hi]
        per_ticker.append((train_df, val_df))
        train_feat_rows.append(train_df[CONFIG.feature_cols].to_numpy("float32"))

    if not per_ticker:
        raise ValueError("No ticker produced enough data to train on.")

    scaler = _fit_scaler(np.concatenate(train_feat_rows, axis=0))
    if save:
        save_scaler(scaler)

    Xtr, ytr, Xva, yva = [], [], [], []
    for train_df, val_df in per_ticker:
        xt = _apply_scaler(train_df[CONFIG.feature_cols].to_numpy("float32"), scaler)
        xv = _apply_scaler(val_df[CONFIG.feature_cols].to_numpy("float32"), scaler)
        a, b = _windows(xt, train_df["target"].to_numpy("float32"), window)
        c, d = _windows(xv, val_df["target"].to_numpy("float32"), window)
        Xtr.append(a); ytr.append(b); Xva.append(c); yva.append(d)

    return (
        np.concatenate(Xtr), np.concatenate(ytr),
        np.concatenate(Xva), np.concatenate(yva),
    )


def completed_bars(df: pd.DataFrame, now: pd.Timestamp | None = None) -> pd.DataFrame:
    """Drop today's bar while the US market is still open.

    During the session Yahoo returns a partial daily bar (partial volume, moving
    close); the model only ever trained on finished days.
    """
    now = now or pd.Timestamp.now(tz="America/New_York")
    if len(df) and df.index[-1].date() == now.date() and now.hour < 16:
        return df.iloc[:-1]
    return df


def build_inference_window(df: pd.DataFrame,
                           earnings: pd.DatetimeIndex | None = None) -> np.ndarray:
    """Latest single window (1, window, n_features) for predicting next move."""
    scaler = load_scaler()
    feats = build_features(df, earnings)
    mat = _apply_scaler(feats[CONFIG.feature_cols].to_numpy("float32"), scaler)
    window = CONFIG.window
    if len(mat) < window:
        raise ValueError(f"Need >= {window} rows, got {len(mat)}")
    return mat[-window:][np.newaxis, ...]
