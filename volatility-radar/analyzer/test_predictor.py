"""Predictor sanity checks. Run from analyzer/:  python test_predictor.py  (or pytest).

Needs only pandas + numpy (no TensorFlow, no network).
"""
import numpy as np
import pandas as pd

from predictor.config import CONFIG
from predictor.dataset import completed_bars
from predictor.features import _days_to_earnings, build_features


def _prices(n: int = 300, start: str = "2020-01-01") -> pd.DataFrame:
    rng = np.random.default_rng(0)
    close = 100 * np.exp(np.cumsum(rng.normal(0, 0.02, n)))
    return pd.DataFrame({
        "Open": close, "High": close * 1.01, "Low": close * 0.99, "Close": close,
        "Volume": rng.integers(100_000, 1_000_000, n).astype(float),
    }, index=pd.bdate_range(start, periods=n))


def test_latest_bars_kept_but_unlabelled():
    df = _prices()
    feats = build_features(df)
    h = CONFIG.horizon
    assert feats.index[-1] == df.index[-1]           # inference sees today's bar
    assert feats["target"].iloc[-h:].isna().all()    # no future yet -> no label, not 0
    assert feats["target"].iloc[-h - 200:-h].notna().all()
    assert feats["target"].dropna().isin([0.0, 1.0]).all()


def test_days_to_earnings_reaction_day():
    ny = "America/New_York"
    bars = pd.bdate_range("2024-01-01", periods=60)  # Jan 1 2024 is a Monday
    # After-close report on Wed Feb 14 -> market reacts Thu Feb 15.
    # Pre-market report on Mon Mar 18 -> market reacts the same day.
    earnings = pd.DatetimeIndex([pd.Timestamp("2024-02-14 16:05", tz=ny),
                                 pd.Timestamp("2024-03-18 07:30", tz=ny)])
    days = pd.Series(_days_to_earnings(bars, earnings), index=bars)
    assert days["2024-02-14"] == 1    # reaction is tomorrow
    assert days["2024-02-08"] == 5    # Thu -> next Thu: inside a 5-day horizon
    assert days["2024-02-07"] == 6    # just outside it
    assert days["2024-02-15"] == 22   # reaction day itself counts toward the NEXT report
    assert days["2024-03-15"] == 1    # Fri before a Monday pre-market report
    assert np.isinf(days["2024-03-18"])                    # nothing scheduled after
    assert np.isinf(_days_to_earnings(bars, None)).all()   # no data -> never "soon"


def test_unfinished_bar_dropped_only_during_session():
    df = _prices(10, start="2024-03-11")             # last bar: Fri Mar 22 2024
    ny = "America/New_York"
    assert len(completed_bars(df, pd.Timestamp("2024-03-22 11:00", tz=ny))) == 9   # session open
    assert len(completed_bars(df, pd.Timestamp("2024-03-22 16:30", tz=ny))) == 10  # after close
    assert len(completed_bars(df, pd.Timestamp("2024-03-25 11:00", tz=ny))) == 10  # bar is from Friday


if __name__ == "__main__":
    test_latest_bars_kept_but_unlabelled()
    test_days_to_earnings_reaction_day()
    test_unfinished_bar_dropped_only_during_session()
    print("ok")
