"""Train the big-move model and save the best epoch.

Run from the analyzer/ directory (so the venv + package import resolve):

    python -m predictor.train
    python -m predictor.train --refresh --epochs 60
"""
from __future__ import annotations

import argparse

import keras

from .config import CONFIG, MODEL_PATH
from .data_loader import load_earnings, load_many
from .dataset import build_training_set
from .model import build_model


def main() -> None:
    parser = argparse.ArgumentParser(description="Train the big-move model")
    parser.add_argument("--refresh", action="store_true",
                        help="re-download price data instead of using cache")
    parser.add_argument("--epochs", type=int, default=CONFIG.epochs)
    parser.add_argument("--tickers", nargs="*", default=None,
                        help="override the ticker universe")
    parser.add_argument("--no-early-stop", action="store_true",
                        help="run all epochs; do not stop early on plateau")
    args = parser.parse_args()

    keras.utils.set_random_seed(CONFIG.seed)

    print("[train] loading prices...")
    frames = load_many(args.tickers, refresh=args.refresh)
    print(f"[train] {len(frames)} tickers loaded")
    earnings = {t: load_earnings(t, refresh=args.refresh) for t in frames}

    print("[train] building windowed dataset...")
    Xtr, ytr, Xva, yva = build_training_set(frames, earnings)
    print(f"[train] X_train={Xtr.shape}  X_val={Xva.shape}  "
          f"base_rate={ytr.mean():.3f}")

    model = build_model(n_features=Xtr.shape[2])
    model.summary()

    # No class_weight on purpose: unweighted cross-entropy keeps the output a real
    # probability, which the app shows as a %. Alerts use CONFIG.alert_prob instead of 0.5.
    callbacks = [
        keras.callbacks.ModelCheckpoint(
            str(MODEL_PATH), monitor="val_auc", mode="max",
            save_best_only=True),
        keras.callbacks.ReduceLROnPlateau(
            monitor="val_auc", mode="max", factor=CONFIG.lr_factor,
            patience=CONFIG.lr_patience, min_lr=CONFIG.min_lr, verbose=1),
    ]
    if not args.no_early_stop:
        callbacks.append(keras.callbacks.EarlyStopping(
            monitor="val_auc", mode="max", patience=CONFIG.early_patience,
            restore_best_weights=True, verbose=1))

    model.fit(
        Xtr, ytr,
        validation_data=(Xva, yva),
        epochs=args.epochs,
        batch_size=CONFIG.batch_size,
        callbacks=callbacks,
        verbose=2,
    )

    # ModelCheckpoint already wrote the best epoch; don't overwrite it with the last one.
    model = keras.models.load_model(MODEL_PATH)
    val = model.evaluate(Xva, yva, verbose=0, return_dict=True)
    print(f"[train] best model -> {MODEL_PATH}")
    print("[train] val " + "  ".join(f"{k}={v:.4f}" for k, v in val.items()))


if __name__ == "__main__":
    main()
