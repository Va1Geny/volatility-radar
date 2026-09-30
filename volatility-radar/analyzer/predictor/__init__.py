"""Big-move predictor: modular Keras pipeline.

Submodules (split for debugging):
    config        paths, ticker list (watchlist.txt), hyperparameters
    data_loader   yfinance download + on-disk cache
    features      technical indicators and the big-move target label
    dataset       windowing -> (X, y) sequences, scaling, train/val split
    model         Keras model factory (dilated CNN, optional CNN+LSTM)
    train         CLI: build dataset, train, save best model + scaler
    predict       load model, score P(big move) for a ticker
    serve         local websocket server pushing predictions to the app
"""
