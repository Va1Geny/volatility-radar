"""Label sanity check. Run from analyzer/:  python test_labels.py  (or pytest)."""
import numpy as np
import pandas as pd

from predictor.config import CONFIG
from predictor.features import build_features


def _prices(n: int = 300) -> pd.DataFrame:
    rng = np.random.default_rng(0)
    close = 100 * np.exp(np.cumsum(rng.normal(0, 0.02, n)))
    return pd.DataFrame({
        "Open": close, "High": close * 1.01, "Low": close * 0.99, "Close": close,
        "Volume": rng.integers(100_000, 1_000_000, n).astype(float),
    }, index=pd.bdate_range("2020-01-01", periods=n))


def test_latest_bars_kept_but_unlabelled():
    df = _prices()
    feats = build_features(df)
    h = CONFIG.horizon
    assert feats.index[-1] == df.index[-1]           # inference sees today's bar
    assert feats["target"].iloc[-h:].isna().all()    # no future yet -> no label, not 0
    assert feats["target"].iloc[-h - 200:-h].notna().all()
    assert feats["target"].dropna().isin([0.0, 1.0]).all()


if __name__ == "__main__":
    test_latest_bars_kept_but_unlabelled()
    print("ok")
