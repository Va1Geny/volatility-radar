"""Stock direction predictor — modular Keras pipeline.

Submodules (split for debugging):
    config        paths, ticker universe, hyperparameters
    data_loader   yfinance download + on-disk cache
    features      technical indicators (returns, RSI, MACD, volume z-score)
    dataset       windowing -> (X, y) sequences, scaling, train/val split
    model         Keras model factory (LSTM classifier)
    train         CLI: build dataset, train, save model + scaler
    predict       load model, predict next-day direction for a ticker
    serve         local websocket server pushing predictions to the terminal
"""

__all__ = [
    "config",
    "data_loader",
    "features",
    "dataset",
    "model",
]
