"""Download OHLCV history via yfinance, cached to parquet."""
from __future__ import annotations

import time
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
        ticker.replace(".", "-"),  # Yahoo spells share classes BRK-B, Finnhub BRK.B
        period=period,
        interval=interval,
        auto_adjust=True,
        progress=False,
    )

    if raw is None or raw.empty:
        if path.exists():
            return pd.read_parquet(path)  # download failed: fall back to the last good copy
        raise ValueError(f"No data returned for {ticker}")

    if isinstance(raw.columns, pd.MultiIndex):
        raw.columns = raw.columns.get_level_values(0)

    raw = raw[["Open", "High", "Low", "Close", "Volume"]].dropna()
    raw.index.name = "Date"
    raw.to_parquet(path)
    return raw


def load_earnings(ticker: str, refresh: bool = False) -> pd.DatetimeIndex:
    """Earnings report timestamps (past and scheduled) from Yahoo, cached to parquet.

    refresh=True re-downloads only if the cache is older than a day: report dates
    rarely change, and the prediction feed calls this every sweep.
    """
    ticker = ticker.upper()
    path = CACHE_DIR / f"{ticker}_earnings.parquet"
    cached = lambda: pd.DatetimeIndex(pd.read_parquet(path)["when"] if path.exists() else [])
    if path.exists() and (not refresh or time.time() - path.stat().st_mtime < 86_400):
        return cached()

    try:
        table = yf.Ticker(ticker.replace(".", "-")).get_earnings_dates(limit=100)
    except Exception as exc:
        table = None
        print(f"[data_loader] earnings dates for {ticker} failed: {exc}")
    if table is None or table.empty:
        return cached()  # keep the last good copy (or nothing) rather than fail the run

    pd.DataFrame({"when": table.index}).to_parquet(path)
    return pd.DatetimeIndex(table.index)


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
