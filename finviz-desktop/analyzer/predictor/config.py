"""Central configuration. Tune here, not in the other modules."""
from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

PREDICTOR_DIR = Path(__file__).resolve().parent
ANALYZER_DIR = PREDICTOR_DIR.parent
CACHE_DIR = ANALYZER_DIR / "cache"
ARTIFACTS_DIR = ANALYZER_DIR / "artifacts"

CACHE_DIR.mkdir(exist_ok=True)
ARTIFACTS_DIR.mkdir(exist_ok=True)

MODEL_PATH = ARTIFACTS_DIR / "bigmove_cnn.keras"
SCALER_PATH = ARTIFACTS_DIR / "scaler.json"

UNIVERSE = [
    "AAPL", "MSFT", "GOOGL", "AMZN", "NVDA", "META", "TSLA",
    "JPM", "V", "MA", "WMT", "JNJ", "PG", "HD", "KO", "PEP", "DIS",
    "NFLX", "AMD", "INTC", "CSCO", "ORCL", "ADBE", "CRM", "NKE", "MCD",
    "BA", "XOM", "CVX", "PFE", "MRK", "BAC", "VZ", "IBM", "QCOM",
    "COST", "TXN", "TSM", "BABA", "TM",
]


@dataclass
class Config:
    tickers: list[str] = field(default_factory=lambda: list(UNIVERSE))

    task: str = "bigmove"
    move_mult: float = 1.5

    period: str = "10y"
    interval: str = "1d"

    window: int = 40
    horizon: int = 5
    val_split: float = 0.2

    feature_cols: list[str] = field(default_factory=lambda: [
        "ret_1", "ret_5", "ret_10",
        "rsi_14",
        "macd", "macd_signal", "macd_hist",
        "vol_z",
        "bb_pct", "atr_n", "stoch_k",
        "dow_sin", "dow_cos", "month_sin", "month_cos",
    ])

    arch: str = "cnn"
    conv_filters: int = 64
    lstm_units: int = 64
    dropout: float = 0.3
    l2: float = 1e-4

    batch_size: int = 128
    epochs: int = 50
    learning_rate: float = 1e-3
    seed: int = 42

    lr_factor: float = 0.5
    lr_patience: int = 5
    min_lr: float = 1e-6
    early_patience: int = 12


CONFIG = Config()
