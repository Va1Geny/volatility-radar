# Graph Report - oopksr  (2026-10-01)

## Corpus Check
- 32 files · ~12,084 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 11 file(s) not represented in the graph (top: (none) 6, .mdc 1, .example 1)

## Summary
- 438 nodes · 701 edges · 28 communities (19 shown, 9 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 36 edges (avg confidence: 0.84)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `d004cc0e`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- StockModel
- dataset.py
- FinnhubClient
- FinnhubRest
- mainwindow.cpp
- TestStockModel
- StockDelegate
- predictor/config.py (all settings)
- PredictionClient
- StockFilterProxy
- mainwindow.h
- __init__.py
- volatility-radar
- volatility_radar_mainwindow
- qchart
- DataFrame
- SocketError
- QWebSocket
- QHash
- build_features
- StockModel.cpp
- StockRecord
- data
- test_stockmodel.cpp
- ndarray
- Model
- FinnhubClient.cpp
- PredictionClient.cpp

## God Nodes (most connected - your core abstractions)
1. `StockModel` - 30 edges
2. `FinnhubClient` - 27 edges
3. `FinnhubRest` - 26 edges
4. `PredictionClient` - 22 edges
5. `StockDelegate` - 19 edges
6. `StockFilterProxy` - 18 edges
7. `StockRecord` - 17 edges
8. `build_features()` - 14 edges
9. `build_training_set()` - 13 edges
10. `TestStockModel` - 12 edges

## Surprising Connections (you probably didn't know these)
- `Invariants: keep these in sync` --references--> `predict_ticker()`  [INFERRED]
  AGENTS.md → volatility-radar/analyzer/predictor/predict.py
- `Big Move CNN model` --references--> `TensorFlow + Keras 3`  [INFERRED]
  volatility-radar/analyzer/MODEL_MANUAL.txt → volatility-radar/analyzer/requirements.txt
- `FinnhubRest` --references--> `StockRecord`  [EXTRACTED]
  volatility-radar/FinnhubRest.h → volatility-radar/StockModel.h
- `MainWindow::onShowChart()` --references--> `QChart`  [INFERRED]
  volatility-radar/mainwindow.cpp → volatility-radar/mainwindow.h
- `MainWindow::setChart()` --references--> `QChart`  [EXTRACTED]
  volatility-radar/mainwindow.cpp → volatility-radar/mainwindow.h

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Watchlist shared between Qt app and analyzer** — volatility_radar_watchlist_watchlist, volatility_radar_cmakelists_volatility_radar_target, volatility_radar_analyzer_model_manual_config_py, volatility_radar_analyzer_model_manual_prediction_server [EXTRACTED 1.00]

## Communities (28 total, 9 thin omitted)

### Community 0 - "StockModel"
Cohesion: 0.18
Nodes (13): QAbstractTableModel, QHash, QVector, QString, Q_OBJECT, QStringList, StockModel, BigMoveBaseRate (+5 more)

### Community 1 - "dataset.py"
Cohesion: 0.05
Nodes (63): argparse, asyncio, dataclasses, json, keras, Model, numpy, pandas (+55 more)

### Community 2 - "FinnhubClient"
Cohesion: 0.14
Nodes (17): FinnhubClient, connected, disconnected, errorOccurred, m_reconnectMs, m_reconnectTimer, m_socket, m_subscribed (+9 more)

### Community 3 - "FinnhubRest"
Cohesion: 0.08
Nodes (32): qnetworkaccessmanager, qnetworkrequest, QPair, QQueue, qurlquery, QObject, QString, QStringList (+24 more)

### Community 4 - "mainwindow.cpp"
Cohesion: 0.06
Nodes (43): qbarcategoryaxis, qbarseries, qbarset, qchartview, QCloseEvent, qcoreapplication, qfileinfo, qlabel (+35 more)

### Community 5 - "TestStockModel"
Cohesion: 0.23
Nodes (6): Q_OBJECT, QAbstractItemModel, QObject, QString, TestStockModel, StockRecord

### Community 6 - "StockDelegate"
Cohesion: 0.17
Nodes (27): qfontmetrics, QPainter, QSize, QStyledItemDelegate, QStyleOptionViewItem, QColor, QModelIndex, QObject (+19 more)

### Community 7 - "predictor/config.py (all settings)"
Cohesion: 0.13
Nodes (20): alert_prob threshold (0.32), Big Move CNN model, build_features() in features.py, CNN+LSTM alternative architecture, predictor/config.py (all settings), move_mult (big-move definition), predictor.serve websocket feed, Parquet price cache (+12 more)

### Community 8 - "PredictionClient"
Cohesion: 0.14
Nodes (16): qabstractsocket, QTimer, QWebSocket, signals, Q_OBJECT, QObject, PredictionClient, connected (+8 more)

### Community 9 - "StockFilterProxy"
Cohesion: 0.13
Nodes (21): QSortFilterProxyModel, QString, QModelIndex, QObject, QString, Q_OBJECT, QString, StockFilterProxy (+13 more)

### Community 10 - "mainwindow.h"
Cohesion: 0.13
Nodes (12): qapplication, qicon, qmainwindow, qstylefactory, QT_BEGIN_NAMESPACE, QT_END_NAMESPACE, namespace(), QChartView (+4 more)

### Community 12 - "volatility-radar"
Cohesion: 0.06
Nodes (28): Agent guide: Volatility Radar, Build and test, graphify, Invariants: keep these in sync, Keeping the knowledge graph current, Rules, What this is, Claude Code (+20 more)

### Community 19 - "build_features"
Cohesion: 0.18
Nodes (19): Series, _atr(), build_features(), _days_to_earnings(), _macd(), DataFrame, DatetimeIndex, ndarray (+11 more)

### Community 20 - "StockModel.cpp"
Cohesion: 0.16
Nodes (13): qdebug, qfile, qlocale, qset, QObject, QString, QStringList, addRecord (+5 more)

### Community 21 - "StockRecord"
Cohesion: 0.14
Nodes (14): QString, StockRecord, country, industry, ipoyear, lastSale, marketCap, name (+6 more)

### Community 22 - "data"
Cohesion: 0.29
Nodes (7): Orientation, QVariant, QModelIndex, columnCount, data, headerData, rowCount

### Community 23 - "test_stockmodel.cpp"
Cohesion: 0.40
Nodes (4): qtemporaryfile, qttest, stockfilterproxy, stockmodel

### Community 26 - "FinnhubClient.cpp"
Cohesion: 0.16
Nodes (15): qjsonobject, QObject, QString, QStringList, SocketError, connectToServer, FinnhubClient::FinnhubClient(), onConnected (+7 more)

### Community 27 - "PredictionClient.cpp"
Cohesion: 0.18
Nodes (13): qjsonarray, qjsondocument, qurl, SocketError, QObject, QString, onConnected, onDisconnected (+5 more)

## Knowledge Gaps
- **94 isolated node(s):** `What this is`, `Build and test`, `Rules`, `Keeping the knowledge graph current`, `graphify` (+89 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 214 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **9 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `StockModel` connect `StockModel` to `TestStockModel`, `StockModel.cpp`, `StockRecord`, `data`?**
  _High betweenness centrality (0.088) - this node is a cross-community bridge._
- **Why does `StockRecord` connect `StockRecord` to `StockModel`, `FinnhubRest`, `StockModel.cpp`?**
  _High betweenness centrality (0.063) - this node is a cross-community bridge._
- **Why does `FinnhubRest` connect `FinnhubRest` to `FinnhubClient`, `StockRecord`?**
  _High betweenness centrality (0.060) - this node is a cross-community bridge._
- **Are the 7 inferred relationships involving `StockModel` (e.g. with `.filtersBySearchTextAndPrice()` and `.livePriceRecomputesChangeFromPreviousClose()`) actually correct?**
  _`StockModel` has 7 INFERRED edges - model-reasoned connections that need verification._
- **What connects `What this is`, `Build and test`, `Rules` to the rest of the system?**
  _94 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `dataset.py` be split into smaller, more focused modules?**
  _Cohesion score 0.0539906103286385 - nodes in this community are weakly interconnected._
- **Should `FinnhubClient` be split into smaller, more focused modules?**
  _Cohesion score 0.13725490196078433 - nodes in this community are weakly interconnected._