# Graph Report - oopksr  (2026-09-30)

## Corpus Check
- 30 files · ~9,529 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 11 file(s) not represented in the graph (top: (none) 6, .mdc 1, .example 1)

## Summary
- 372 nodes · 581 edges · 14 communities (13 shown, 1 thin omitted)
- Extraction: 96% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 22 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `736b79f2`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- StockModel
- dataset.py
- FinnhubClient
- FinnhubRest
- mainwindow.cpp
- build_features
- StockDelegate
- predictor/config.py (all settings)
- PredictionClient
- StockFilterProxy
- mainwindow.h
- __init__.py
- volatility-radar
- main.cpp

## God Nodes (most connected - your core abstractions)
1. `FinnhubClient` - 27 edges
2. `FinnhubRest` - 26 edges
3. `StockModel` - 22 edges
4. `PredictionClient` - 22 edges
5. `StockDelegate` - 19 edges
6. `StockRecord` - 17 edges
7. `StockFilterProxy` - 16 edges
8. `build_features()` - 12 edges
9. `build_training_set()` - 10 edges
10. `paint` - 10 edges

## Surprising Connections (you probably didn't know these)
- `Big Move CNN model` --references--> `TensorFlow + Keras 3`  [INFERRED]
  volatility-radar/analyzer/MODEL_MANUAL.txt → volatility-radar/analyzer/requirements.txt
- `FinnhubRest` --references--> `StockRecord`  [EXTRACTED]
  volatility-radar/FinnhubRest.h → volatility-radar/StockModel.h
- `PredictionClient` --references--> `QTimer`  [EXTRACTED]
  volatility-radar/PredictionClient.h → volatility-radar/FinnhubRest.h
- `_sweep_forever()` --indirect_call--> `predict_ticker()`  [INFERRED]
  volatility-radar/analyzer/predictor/serve.py → volatility-radar/analyzer/predictor/predict.py
- `build_inference_window()` --calls--> `build_features()`  [EXTRACTED]
  volatility-radar/analyzer/predictor/dataset.py → volatility-radar/analyzer/predictor/features.py

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Watchlist shared between Qt app and analyzer** — volatility_radar_watchlist_watchlist, volatility_radar_cmakelists_volatility_radar_target, volatility_radar_analyzer_model_manual_config_py, volatility_radar_analyzer_model_manual_prediction_server [EXTRACTED 1.00]

## Communities (14 total, 1 thin omitted)

### Community 0 - "StockModel"
Cohesion: 0.06
Nodes (46): Orientation, QAbstractTableModel, qdebug, qfile, qlocale, qset, QVariant, QVector (+38 more)

### Community 1 - "dataset.py"
Cohesion: 0.06
Nodes (52): argparse, asyncio, dataclasses, json, keras, ndarray, pandas, Path (+44 more)

### Community 2 - "FinnhubClient"
Cohesion: 0.08
Nodes (34): qabstractsocket, qjsonarray, qurl, qurlquery, QObject, QString, QStringList, SocketError (+26 more)

### Community 3 - "FinnhubRest"
Cohesion: 0.08
Nodes (31): qnetworkaccessmanager, qnetworkrequest, QPair, QQueue, QStringList, QObject, QString, QStringList (+23 more)

### Community 4 - "mainwindow.cpp"
Cohesion: 0.08
Nodes (26): qbarcategoryaxis, qbarseries, qbarset, qchart, qchartview, qcoreapplication, qfileinfo, qlabel (+18 more)

### Community 5 - "build_features"
Cohesion: 0.23
Nodes (14): numpy, Series, _atr(), build_features(), _macd(), DataFrame, Technical + calendar features. Pure pandas/numpy, no TA library. Input: OHLCV…, Add indicator + calendar columns and a binary forward-direction target. target… (+6 more)

### Community 6 - "StockDelegate"
Cohesion: 0.17
Nodes (27): qfontmetrics, QPainter, QSize, QStyledItemDelegate, QStyleOptionViewItem, QColor, QModelIndex, QObject (+19 more)

### Community 7 - "predictor/config.py (all settings)"
Cohesion: 0.13
Nodes (20): alert_prob threshold (0.32), Big Move CNN model, build_features() in features.py, CNN+LSTM alternative architecture, predictor/config.py (all settings), move_mult (big-move definition), predictor.serve websocket feed, Parquet price cache (+12 more)

### Community 8 - "PredictionClient"
Cohesion: 0.09
Nodes (27): qjsondocument, qjsonobject, QObject, QString, SocketError, Q_OBJECT, QObject, QString (+19 more)

### Community 9 - "StockFilterProxy"
Cohesion: 0.13
Nodes (21): QSortFilterProxyModel, QString, QModelIndex, QObject, QString, Q_OBJECT, QString, StockFilterProxy (+13 more)

### Community 10 - "mainwindow.h"
Cohesion: 0.25
Nodes (7): qmainwindow, QT_BEGIN_NAMESPACE, QT_END_NAMESPACE, namespace(), QChartView, QLabel, QMainWindow()

### Community 12 - "volatility-radar"
Cohesion: 0.07
Nodes (25): Agent guide: Volatility Radar, Build and test, graphify, Invariants: keep these in sync, Keeping the knowledge graph current, Rules, What this is, Claude Code (+17 more)

### Community 13 - "main.cpp"
Cohesion: 0.33
Nodes (4): qapplication, qicon, qstylefactory, volatility_radar_mainwindow

## Knowledge Gaps
- **92 isolated node(s):** `1. Install the prerequisites`, `2. Get the code`, `3. Build and run`, `Choosing stocks`, `What changes in live mode` (+87 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 188 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `FinnhubRest` connect `FinnhubRest` to `StockModel`, `FinnhubClient`?**
  _High betweenness centrality (0.076) - this node is a cross-community bridge._
- **Why does `FinnhubClient` connect `FinnhubClient` to `StockFilterProxy`, `FinnhubRest`?**
  _High betweenness centrality (0.072) - this node is a cross-community bridge._
- **Why does `PredictionClient` connect `PredictionClient` to `FinnhubClient`?**
  _High betweenness centrality (0.060) - this node is a cross-community bridge._
- **What connects `1. Install the prerequisites`, `2. Get the code`, `3. Build and run` to the rest of the system?**
  _92 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `StockModel` be split into smaller, more focused modules?**
  _Cohesion score 0.05851063829787234 - nodes in this community are weakly interconnected._
- **Should `dataset.py` be split into smaller, more focused modules?**
  _Cohesion score 0.059887005649717516 - nodes in this community are weakly interconnected._
- **Should `FinnhubClient` be split into smaller, more focused modules?**
  _Cohesion score 0.08258258258258258 - nodes in this community are weakly interconnected._