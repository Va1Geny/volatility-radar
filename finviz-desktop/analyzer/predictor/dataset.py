"""Turn per-ticker feature frames into windowed (X, y) arrays for the LSTM.

Scaling is fit on the training portion only and persisted to SCALER_PATH
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


def build_training_set(price_frames: dict[str, pd.DataFrame]):
    """Build (X_train, y_train, X_val, y_val) across all tickers.

    Split is chronological per ticker (no shuffle) to avoid look-ahead leak,
    then concatenated. Scaler is fit on training rows only.
    """
    window = CONFIG.window
    train_feat_rows = []
    per_ticker = []

    for ticker, df in price_frames.items():
        feats = build_features(df)
        if len(feats) <= window + 1:
            print(f"[dataset] skip {ticker}: not enough rows ({len(feats)})")
            continue
        split = int(len(feats) * (1.0 - CONFIG.val_split))
        train_df = feats.iloc[:split]
        val_df = feats.iloc[split:]
        per_ticker.append((train_df, val_df))
        train_feat_rows.append(train_df[CONFIG.feature_cols].to_numpy("float32"))

    if not per_ticker:
        raise ValueError("No ticker produced enough data to train on.")

    scaler = _fit_scaler(np.concatenate(train_feat_rows, axis=0))
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


def build_inference_window(df: pd.DataFrame) -> np.ndarray:
    """Latest single window (1, window, n_features) for predicting next move."""
    scaler = load_scaler()
    feats = build_features(df)
    mat = _apply_scaler(feats[CONFIG.feature_cols].to_numpy("float32"), scaler)
    window = CONFIG.window
    if len(mat) < window:
        raise ValueError(f"Need >= {window} rows, got {len(mat)}")
    return mat[-window:][np.newaxis, ...]
