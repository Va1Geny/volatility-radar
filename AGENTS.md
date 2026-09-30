# Agent guide: Volatility Radar

Shared instructions for coding agents (Codex and Cursor read this file directly; Claude Code imports it from CLAUDE.md).
Human-facing docs are in README.md.

## What this is

- `volatility-radar/`: a Qt 6 / C++17 desktop stock screener. Live prices come from Finnhub (REST at startup, websocket for trades).
- `volatility-radar/analyzer/`: a Python/Keras model that scores P(big move within 5 trading days) and streams the scores to the app over `ws://127.0.0.1:8765`.

## Build and test

```powershell
# C++ (Windows, MSVC + Qt 6.5+). Put the Qt CMake/Ninja dirs on PATH *before* vcvars64.bat;
# inside a single `cmd /c` line, %PATH% is expanded before vcvars runs.
cmake -S volatility-radar -B volatility-radar/build/cli -G Ninja -DCMAKE_PREFIX_PATH=<Qt>/msvc2022_64
cmake --build volatility-radar/build/cli

# Python (from volatility-radar/analyzer, venv active)
python test_labels.py               # label sanity check; run after touching features/dataset
python -m predictor.train --refresh # retrain (~5 min); rewrites the committed artifacts/
python -m predictor.serve           # prediction feed for the app
```

The C++ side has no unit tests. Verify changes by building, then running the app offline (no `.env`) and live (with `.env`).

## Invariants: keep these in sync

- **Columns**: `StockModel::Column` is the only column list. The proxy and delegate use it, and numeric columns expose raw values via `Qt::UserRole` (sorting relies on that).
- **Alert levels**: `alert_prob` in `analyzer/predictor/config.py` must match `BigMoveWarn`/`BigMoveAlert` in `StockDelegate.h`. Both assume a base rate of ~0.16 at `move_mult=1.5`.
- **Tickers**: `volatility-radar/watchlist.txt` is read by both the app and the analyzer. Use Finnhub format (`BRK.B`); the loader converts to Yahoo format.
- **Predictions** are stored by symbol in `StockModel::m_bigMoveProb`, because they can arrive before a row exists.
- **Data files** (`.env`, `watchlist.txt`, `stocks.json`, `style.qss`) are looked up in the source tree first (`APP_SOURCE_DIR`), then next to the exe.

## Rules

- Never commit `.env` or API keys. The REST token goes in the `X-Finnhub-Token` header, never in URLs (Qt puts URLs into error strings).
- Labels for bars with no known future must stay NaN (not 0). Train/validation splits keep a `horizon`-row gap.
- Keep diffs small and match the surrounding style: tabs, Allman braces, and Qt types in C++.
- Commits: Conventional Commits (`fix(app): ...`, `feat(analyzer): ...`, `docs: ...`), with a short body.

## Keeping the knowledge graph current

Install graphify once (`uv tool install graphifyy` or `pip install graphifyy`), then run `graphify hook install` in each clone.
That adds git hooks that rebuild the graph after every commit and checkout. Claude Code and Cursor also run `graphify update .` after each file edit (see `.claude/settings.json`, `.cursor/hooks.json`).

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

When the user types `/graphify`, use the installed graphify skill or instructions before doing anything else.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- Dirty graphify-out/ files are expected after hooks or incremental updates; dirty graph files are not a reason to skip graphify. Only skip graphify if the task is about stale or incorrect graph output, or the user explicitly says not to use it.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
