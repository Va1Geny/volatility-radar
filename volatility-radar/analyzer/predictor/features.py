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


def _days_to_earnings(index: pd.DatetimeIndex, earnings: pd.DatetimeIndex | None) -> np.ndarray:
    """Business days from each bar to the next earnings *reaction* day (inf if none known).

    The reaction day is the report day for pre-market reports and the next business
    day for reports after 12:00 New York time (after-close reports move the next open).
    Report dates are scheduled weeks ahead, so using future ones is not look-ahead.
    """
    days = np.full(len(index), np.inf)
    if earnings is None or len(earnings) == 0:
        return days
    ny = earnings.tz_convert("America/New_York") if earnings.tz is not None else earnings
    react = ny.tz_localize(None).normalize()
    react = react.where(ny.hour < 12, react + pd.offsets.BDay(1))
    react = np.unique(react.values.astype("datetime64[D]"))
    bars = index.values.astype("datetime64[D]")
    pos = np.searchsorted(react, bars, side="right")  # first reaction strictly after the bar
    known = pos < len(react)
    days[known] = np.busday_count(bars[known], react[pos[known]])
    return days


def build_features(df: pd.DataFrame, earnings: pd.DatetimeIndex | None = None,
                   horizon: int | None = None) -> pd.DataFrame:
    """Add indicator, calendar and earnings columns plus the target label.

    bigmove task: target = 1 if |return over `horizon` bars| is unusually large.
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

    # Earnings are the single biggest cause of big moves.
    days = _days_to_earnings(df.index, earnings)
    out["earn_soon"] = ((days >= 1) & (days <= horizon)).astype("float32")
    out["earn_days"] = np.minimum(days, 21) / 21.0

    fwd_ret = close.shift(-horizon) / close - 1.0
    if CONFIG.task == "bigmove":
        expected_move = close.pct_change().rolling(20).std() * np.sqrt(horizon)
        label = fwd_ret.abs() > CONFIG.move_mult * expected_move
        known = fwd_ret.notna() & expected_move.notna()
    else:
        label = fwd_ret > 0
        known = fwd_ret.notna()
    # Unknown future -> NaN, not 0. Training drops those rows; inference still
    # needs them (the latest bars are exactly the ones with no future yet).
    out["target"] = label.astype("float32").where(known)

    out = out.replace([np.inf, -np.inf], np.nan).dropna(subset=CONFIG.feature_cols)
    return out[CONFIG.feature_cols + ["target"]]
