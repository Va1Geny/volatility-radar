"""Technical + calendar features. Pure pandas/numpy, no TA library.

Input: OHLCV DataFrame from data_loader.load_prices().
Output: DataFrame with CONFIG.feature_cols plus a 'target' label.
"""
from __future__ import annotations

import numpy as np
import pandas as pd

from .config import CONFIG


def _rsi(close: pd.Series, period: int = 14) -> pd.Series:
    delta = close.diff()
    gain = delta.clip(lower=0.0)
    loss = -delta.clip(upper=0.0)
    avg_gain = gain.ewm(alpha=1 / period, min_periods=period).mean()
    avg_loss = loss.ewm(alpha=1 / period, min_periods=period).mean()
    rs = avg_gain / avg_loss.replace(0.0, np.nan)
    return 100.0 - (100.0 / (1.0 + rs))


def _macd(close: pd.Series, fast: int = 12, slow: int = 26, signal: int = 9):
    ema_fast = close.ewm(span=fast, adjust=False).mean()
    ema_slow = close.ewm(span=slow, adjust=False).mean()
    macd = ema_fast - ema_slow
    macd_signal = macd.ewm(span=signal, adjust=False).mean()
    return macd, macd_signal, macd - macd_signal


def _atr(df: pd.DataFrame, period: int = 14) -> pd.Series:
    high, low, close = df["High"], df["Low"], df["Close"]
    prev_close = close.shift(1)
    tr = pd.concat([
        high - low,
        (high - prev_close).abs(),
        (low - prev_close).abs(),
    ], axis=1).max(axis=1)
    return tr.ewm(alpha=1 / period, min_periods=period).mean()


def _stoch_k(df: pd.DataFrame, period: int = 14) -> pd.Series:
    low_n = df["Low"].rolling(period).min()
    high_n = df["High"].rolling(period).max()
    return (df["Close"] - low_n) / (high_n - low_n).replace(0.0, np.nan) * 100.0


def build_features(df: pd.DataFrame, horizon: int | None = None) -> pd.DataFrame:
    """Add indicator + calendar columns and a binary forward-direction target.

    target = 1 if Close goes up `horizon` bars ahead, else 0.
    All price-scale features are normalized so they generalize across tickers.
    """
    horizon = horizon or CONFIG.horizon
    out = pd.DataFrame(index=df.index)

    close = df["Close"]
    volume = df["Volume"]

    out["ret_1"] = close.pct_change(1)
    out["ret_5"] = close.pct_change(5)
    out["ret_10"] = close.pct_change(10)

    out["rsi_14"] = _rsi(close, 14)

    macd, macd_signal, macd_hist = _macd(close)
    out["macd"] = macd / close
    out["macd_signal"] = macd_signal / close
    out["macd_hist"] = macd_hist / close

    out["vol_z"] = (volume - volume.rolling(20).mean()) / volume.rolling(20).std()

    ma20 = close.rolling(20).mean()
    sd20 = close.rolling(20).std()
    out["bb_pct"] = (close - (ma20 - 2 * sd20)) / (4 * sd20).replace(0.0, np.nan)

    out["atr_n"] = _atr(df, 14) / close

    out["stoch_k"] = _stoch_k(df, 14)

    dow = df.index.dayofweek.to_numpy()
    month = df.index.month.to_numpy()
    out["dow_sin"] = np.sin(2 * np.pi * dow / 5.0)
    out["dow_cos"] = np.cos(2 * np.pi * dow / 5.0)
    out["month_sin"] = np.sin(2 * np.pi * month / 12.0)
    out["month_cos"] = np.cos(2 * np.pi * month / 12.0)

    fwd_ret = close.shift(-horizon) / close - 1.0
    if CONFIG.task == "bigmove":
        daily_std = close.pct_change().rolling(20).std()
        expected_move = daily_std * np.sqrt(horizon)
        out["target"] = (fwd_ret.abs() > CONFIG.move_mult * expected_move).astype("float32")
    else:
        out["target"] = (fwd_ret > 0).astype("float32")

    out = out.replace([np.inf, -np.inf], np.nan).dropna()
    return out[CONFIG.feature_cols + ["target"]]
