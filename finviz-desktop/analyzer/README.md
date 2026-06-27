# analyzer — stock direction predictor

Modular Keras pipeline. Each stage is its own file for easy debugging.

```
predictor/
  config.py       paths, ticker universe, hyperparameters   <- tune here
  data_loader.py  yfinance download + parquet cache
  features.py     RSI, MACD, returns, volume z-score, target label
  dataset.py      windowing -> (X, y), scaling, train/val split
  model.py        Keras LSTM classifier factory
  train.py        CLI: build dataset, train, save model
  predict.py      load model, score next-day direction
  serve.py        optional websocket feed of predictions for the terminal
```

## Setup

```powershell
cd analyzer
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

## Train (when you want)

```powershell
python -m predictor.train               # uses cached data
python -m predictor.train --refresh     # re-download prices
python -m predictor.train --epochs 60 --tickers AAPL MSFT NVDA
```

Artifacts land in `analyzer/artifacts/` (`direction_lstm.keras`, `scaler.json`).

## Predict

```powershell
python -m predictor.predict AAPL MSFT
```

## Prediction feed (optional, for terminal)

```powershell
python -m predictor.serve   # ws://127.0.0.1:8765, JSON predictions
```

## Reality check

Daily direction prediction is near coin-flip. Model outputs P(up next bar)
as a **signal**, not a guarantee. Judge it by validation AUC over time, and
walk-forward, not by a single accuracy number. Features are returns/indicators,
never raw price.

---

The old `stock_data.py` is superseded by `predictor/data_loader.py`
(it had bugs: `interval="id"`, `raw.empty()`, `self.choice`).
