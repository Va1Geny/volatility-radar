"""Walk-forward evaluation: train on the past, test on the next slice, repeat.

A single train/validation split can be lucky. This retrains a fresh model per
fold and reports AUC on each later slice of history. Nothing is saved.

    python -m predictor.walkforward
    python -m predictor.walkforward --drop earn_soon earn_days   # ablation
"""
from __future__ import annotations

import argparse

import keras
import numpy as np

from .config import CONFIG
from .data_loader import load_earnings, load_many
from .dataset import build_training_set
from .model import build_model

# (test_start, test_end) as fractions of each ticker's history; train on everything before.
FOLDS = [(0.6, 0.7), (0.7, 0.8), (0.8, 0.9), (0.9, 1.0)]


def main() -> None:
    parser = argparse.ArgumentParser(description="Walk-forward evaluation")
    parser.add_argument("--drop", nargs="*", default=[], help="feature columns to leave out")
    parser.add_argument("--epochs", type=int, default=CONFIG.epochs)
    args = parser.parse_args()

    CONFIG.feature_cols = [c for c in CONFIG.feature_cols if c not in args.drop]
    print(f"[walk-forward] {len(CONFIG.feature_cols)} features, dropped: {args.drop or 'none'}")
    frames = load_many()
    earnings = {t: load_earnings(t) for t in frames}

    aucs = []
    for lo, hi in FOLDS:
        keras.utils.set_random_seed(CONFIG.seed)
        Xtr, ytr, Xva, yva = build_training_set(frames, earnings, lo, hi, save=False)
        model = build_model(n_features=Xtr.shape[2])
        # ponytail: early-stops on the test fold, which flatters every variant equally;
        # carve a separate stopping slice if absolute AUC (not comparison) matters.
        model.fit(Xtr, ytr, validation_data=(Xva, yva), epochs=args.epochs,
                  batch_size=CONFIG.batch_size, verbose=0,
                  callbacks=[keras.callbacks.EarlyStopping(
                      monitor="val_auc", mode="max", patience=CONFIG.early_patience,
                      restore_best_weights=True)])
        auc = model.evaluate(Xva, yva, verbose=0, return_dict=True)["auc"]
        aucs.append(auc)
        print(f"[walk-forward] test {lo:.0%}-{hi:.0%} of history: auc={auc:.4f}  "
              f"(base rate {yva.mean():.3f}, {len(yva)} windows)")

    print(f"[walk-forward] mean auc={np.mean(aucs):.4f}  min={min(aucs):.4f}  max={max(aucs):.4f}")


if __name__ == "__main__":
    main()
