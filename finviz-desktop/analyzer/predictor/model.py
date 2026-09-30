"""Keras model factory. Binary classifier: P(big move within horizon).

build_model() dispatches on CONFIG.arch:
    "cnn"     -> dilated causal Conv1D stack (fast, strong on local patterns)
    "cnnlstm" -> Conv1D front end + LSTM (captures longer sequence structure)
Both are regularized (dropout, BatchNorm, L2) to resist overfitting.
"""
from __future__ import annotations

import keras
from keras import layers, regularizers

from .config import CONFIG


def _compile(model: keras.Model) -> keras.Model:
    model.compile(
        optimizer=keras.optimizers.Adam(CONFIG.learning_rate),
        loss="binary_crossentropy",
        metrics=[
            "accuracy",
            keras.metrics.AUC(name="auc"),
            # At the alert level, not 0.5: calibrated outputs rarely reach 0.5 (base rate ~0.16).
            keras.metrics.Precision(thresholds=CONFIG.alert_prob, name="precision"),
            keras.metrics.Recall(thresholds=CONFIG.alert_prob, name="recall"),
        ],
    )
    return model


def build_cnn(n_features: int) -> keras.Model:
    """Dilated causal Conv1D stack -> global pooling -> sigmoid."""
    reg = regularizers.l2(CONFIG.l2)
    inp = layers.Input(shape=(CONFIG.window, n_features))
    x = inp
    for dilation in (1, 2, 4):
        x = layers.Conv1D(CONFIG.conv_filters, 3, padding="causal",
                          dilation_rate=dilation, activation="relu",
                          kernel_regularizer=reg)(x)
        x = layers.BatchNormalization()(x)
        x = layers.Dropout(CONFIG.dropout)(x)

    x = layers.GlobalAveragePooling1D()(x)
    x = layers.Dense(32, activation="relu", kernel_regularizer=reg)(x)
    x = layers.Dropout(CONFIG.dropout)(x)
    out = layers.Dense(1, activation="sigmoid")(x)
    return _compile(keras.Model(inp, out, name="bigmove_cnn"))


def build_cnnlstm(n_features: int) -> keras.Model:
    reg = regularizers.l2(CONFIG.l2)
    inp = layers.Input(shape=(CONFIG.window, n_features))
    x = layers.Conv1D(CONFIG.conv_filters, 3, padding="causal",
                      activation="relu", kernel_regularizer=reg)(inp)
    x = layers.BatchNormalization()(x)
    x = layers.Dropout(CONFIG.dropout)(x)
    x = layers.LSTM(CONFIG.lstm_units, kernel_regularizer=reg)(x)
    x = layers.Dropout(CONFIG.dropout)(x)
    x = layers.Dense(32, activation="relu", kernel_regularizer=reg)(x)
    x = layers.Dropout(CONFIG.dropout)(x)
    out = layers.Dense(1, activation="sigmoid")(x)
    return _compile(keras.Model(inp, out, name="bigmove_cnnlstm"))


def build_model(n_features: int) -> keras.Model:
    if CONFIG.arch == "cnnlstm":
        return build_cnnlstm(n_features)
    return build_cnn(n_features)
