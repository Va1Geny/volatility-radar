"""Download OHLCV history via yfinance, cached to parquet.

Supersedes the old analyzer/stock_data.py (which had bugs:
interval="id", raw.empty(), self.choice). Use load_prices() instead.
"""
from __future__ import annotations

from pathlib import Path

import pandas as pd
import yfinance as yf

from .config import CACHE_DIR, CONFIG


def _cache_path(ticker: str, interval: str) -> Path:
    return CACHE_DIR / f"{ticker.upper()}_{interval}.parquet"


def load_prices(
    ticker: str,
    period: str | None = None,
    interval: str | None = None,
    refresh: bool = False,
) -> pd.DataFrame:
    """Return a tidy OHLCV DataFrame indexed by date.

    Columns: Open, High, Low, Close, Volume. Cached on disk; pass
    refresh=True to force a re-download.
    """
    ticker = ticker.upper()
    period = period or CONFIG.period
    interval = interval or CONFIG.interval
    path = _cache_path(ticker, interval)

    if path.exists() and not refresh:
        return pd.read_parquet(path)

    raw = yf.download(
        ticker,
        period=period,
        interval=interval,
        auto_adjust=True,
        progress=False,
    )

    if raw is None or raw.empty:
        raise ValueError(f"No data returned for {ticker}")

    if isinstance(raw.columns, pd.MultiIndex):
        raw.columns = raw.columns.get_level_values(0)

    raw = raw[["Open", "High", "Low", "Close", "Volume"]].dropna()
    raw.index.name = "Date"
    raw.to_parquet(path)
    return raw


def load_many(
    tickers: list[str] | None = None,
    refresh: bool = False,
) -> dict[str, pd.DataFrame]:
    """Load several tickers; skip any that fail rather than aborting."""
    tickers = tickers or CONFIG.tickers
    out: dict[str, pd.DataFrame] = {}
    for t in tickers:
        try:
            out[t] = load_prices(t, refresh=refresh)
        except Exception as exc:
            print(f"[data_loader] skip {t}: {exc}")
    return out
