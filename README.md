<p align="center"><img src="volatility-radar/resources/app.png" width="128" alt="Volatility Radar icon"></p>

# volatility-radar

[![CI](https://github.com/Va1Geny/volatility-radar/actions/workflows/ci.yml/badge.svg)](https://github.com/Va1Geny/volatility-radar/actions/workflows/ci.yml)

A dark desktop stock screener, inspired by Finviz, built with C++ and Qt 6.
It shows live US stock prices from [Finnhub](https://finnhub.io). An optional
Python model flags which stocks are likely to make an unusually large move in
the next week.

- **Screener table**: sort by any column, search by ticker or name, and filter by sector and price range.
- **Live prices**: streamed over a websocket, with net and % change against the previous close.
- **Stock details panel** with a 6-month price chart (watchlist stocks), plus a sector distribution chart.
- **Big Move column**: the model's probability of an unusually large move (up *or* down) within 5 trading days. Hover a value for what it means.
- **Offline mode**: with no API key, the app loads a bundled snapshot of about 7,000 US listings, so you can try it with no account.
- Remembers its window size, panel layout and sort order between runs.

![Volatility Radar with live Finnhub prices and Big Move predictions](docs/app.png)

> Not financial advice. The model predicts *volatility*, not direction, and
> its edge is modest (validation AUC around 0.63). Use it to rank stocks worth
> watching, never as a buy/sell signal.

---

## Quick start

### Option A: download (Windows, no build tools)

1. Download `VolatilityRadar-<version>-windows-x64.zip` from the
   [latest release](https://github.com/Va1Geny/volatility-radar/releases/latest) and unzip it anywhere.
2. Run `VolatilityRadar/bin/volatility-radar.exe`. It starts in offline mode.
3. For live prices, add your key to a `.env` file in the same `bin` folder (see [Live data](#live-data-free-finnhub-key)).
   For predictions, set up the analyzer in `bin/analyzer` (see [Big-move predictions](#big-move-predictions-optional)).

### Option B: build from source

#### 1. Install the prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| [Qt](https://www.qt.io/download-qt-installer-oss) | 6.5 or newer | In the installer, also tick **Qt Charts** and **Qt WebSockets** (under *Additional Libraries*). |
| C++ compiler | C++17 | MSVC 2022 or MinGW (both ship with the Qt installer on Windows). |
| CMake | 3.22 or newer | Also ships with the Qt installer. |

Developed on Windows 11; CI builds and tests it on Windows and Linux. macOS
should work but is untested.

#### 2. Get the code

```bash
git clone https://github.com/Va1Geny/volatility-radar.git
cd volatility-radar/volatility-radar
```

#### 3. Build and run

**Easiest: Qt Creator.** Choose *File → Open File or Project*, pick
`volatility-radar/CMakeLists.txt`, select a kit (for example *Desktop Qt 6.x MSVC2022 64bit*),
then press **Run** (Ctrl+R).

**Command line:**

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/msvc2022_64
cmake --build build
```

Replace the `CMAKE_PREFIX_PATH` with your Qt install. To get a standalone
folder you can run or zip, with all the Qt DLLs, install it:

```bash
cmake --install build --prefix dist
```

With no API key the app starts in **offline mode** and shows the bundled snapshot.

---

## Live data (free Finnhub key)

1. Create a free account at <https://finnhub.io/register> and copy your API key.
2. In the `volatility-radar/` folder (downloaded app: the `bin` folder next to the `.exe`),
   copy `.env.example` to a new file named `.env`.
3. Paste your key after the `=`:

   ```
   FINNHUB_API_KEY=your_key_here
   ```

4. Restart the app. The status bar should say **Live: connected to Finnhub**.
   It takes about a minute to load all stocks, because the free plan allows 60 requests per minute.

`.env` is gitignored, so your key never gets committed. You can also set
`FINNHUB_API_KEY` as a normal environment variable instead; it takes priority over `.env`.

The app looks for `.env`, `watchlist.txt`, `stocks.json` and `style.qss` in
the source folder it was built from. If that folder doesn't exist (for example
on another PC), it looks next to the `.exe`.

### Choosing stocks

Live mode tracks the tickers in [`volatility-radar/watchlist.txt`](volatility-radar/watchlist.txt):
one symbol per line, with `#` for comments. The analyzer uses the same file.
Finnhub's free plan streams up to 50 symbols, and each symbol costs 2 API calls at startup.

### What changes in live mode

- The **Volume** and **Industry** columns are hidden, because Finnhub's free plan doesn't provide either.
- **Sector** is Finnhub's industry group (for example *Technology* or *Banking*).
- **Market Cap** shows `-` for foreign listings (such as TSM or TM), because Finnhub reports those in the home currency, not USD.
- Rows don't reorder on every price tick, so they stay put under your cursor.
  To re-sort with fresh prices, click a column header.

---

## Big-move predictions (optional)

The `volatility-radar/analyzer/` folder holds a small Keras model. It scores each
watchlist ticker with P(unusually large move in the next 5 trading days) and
streams the scores to the app's **Big Move** column.

The base rate is about 16%, so a typical stock scores around 16%.
**Amber** (24% and up) means about 1.5× more likely than usual.
**Red** (32% and up) means about 2× more likely than usual.

### Setup (once)

Needs Python 3.10–3.13 (TensorFlow's supported range). Create the venv inside the
analyzer folder: `volatility-radar/analyzer` from source, or `bin/analyzer` in the download.

```powershell
cd volatility-radar/analyzer
python -m venv .venv
.\.venv\Scripts\Activate.ps1        # macOS/Linux: source .venv/bin/activate
pip install -r requirements.txt
```

### Train the model (optional)

The repo ships with a trained model, so you can skip straight to the feed.
Retrain to refresh it with newer data, or after you change `watchlist.txt`
or `config.py`:

```powershell
python -m predictor.train --refresh    # downloads ~10 years of daily prices
```

Training runs on the CPU. For the default 41-stock watchlist it takes about
5 minutes, including the download, and it stops early once it stops improving.
The best epoch is saved to `analyzer/artifacts/bigmove_cnn.keras`.

### The prediction feed

**The app starts it for you.** Once `analyzer/.venv` exists, the app launches
`python -m predictor.serve` in the background at startup and stops it on exit.
The status bar goes from **Predictions: starting...** to **Predictions: live**
after about 10–20 seconds (TensorFlow is slow to load).

The feed re-downloads prices and re-scores every ticker every 30 minutes. During
market hours it ignores today's unfinished bar, because the model only trained on
completed days. Each prediction also carries 6 months of closes, which the app
draws as the price chart.

To run the feed by hand instead, run `python -m predictor.serve` before starting the app.
To score a few tickers once from the terminal, run `python -m predictor.predict AAPL TSLA`.

To tune the model (thresholds, features, architecture), see
[`analyzer/MODEL_MANUAL.txt`](volatility-radar/analyzer/MODEL_MANUAL.txt).

---

## How it fits together

```
Finnhub REST       (names, quotes at startup) ──┐
Finnhub websocket  (live trades)              ──┼──►  volatility-radar (Qt app)
analyzer/serve.py  (Big Move %, local ws:8765) ─┘
      ▲
      └── yfinance daily prices ──► Keras model
```

## Project layout

```
volatility-radar/
  main.cpp, mainwindow.*     window, layout, wiring
  StockModel.*               table data (one row per stock)
  StockFilterProxy.*         search, sector, price filters and numeric sorting
  StockDelegate.*            cell painting (colored change pills, Big Move badges)
  FinnhubRest.*              startup load: quote and profile per symbol, rate-limited
  FinnhubClient.*            live trade stream (websocket, auto-reconnect)
  PredictionClient.*         reads Big Move scores from the analyzer
  watchlist.txt              tickers for live mode and the model
  stocks.json                offline snapshot (Nasdaq screener export)
  style.qss                  dark theme
  resources/, app.rc         app icon (window, taskbar, .exe)
  .env.example               template for your API key
  tests/                     C++ unit tests (Qt Test)
  analyzer/                  Python model: train, predict, serve, walk-forward evaluation
```

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| Status bar says *Offline snapshot* | No key found. Check that `.env` sits in `volatility-radar/` and has `FINNHUB_API_KEY=...` with no spaces around `=`. |
| *Live error: …* or constant reconnects | The key is invalid, or you're over the free plan's limits. The app backs off automatically, up to 60 s between retries. |
| A ticker is missing in live mode | Finnhub returned no quote for it (unknown symbol, or not on the free plan). The status bar shows `SYMBOL: no quote, skipped`. |
| *Predictions: offline* | `analyzer/.venv` doesn't exist yet (see [Setup](#setup-once)), or the feed crashed. Run `python -m predictor.serve` by hand to see its error. |
| No price chart for a stock | Charts come with predictions, so only watchlist stocks have one, and only while the feed is live. |
| App won't start: missing `Qt6*.dll` | Run it from Qt Creator, or use `cmake --install build --prefix dist`, which copies the DLLs. |
| `No model at …` from the analyzer | Train first: `python -m predictor.train`. |

## Running the tests

```bash
# C++: model, sorting, filters, snapshot parser (no window needed)
cmake --build build && ctest --test-dir build --output-on-failure

# Python: labels, earnings feature, partial-bar handling (needs only pandas + numpy)
cd volatility-radar/analyzer && python test_predictor.py
```

CI runs both on every push and pull request, on Windows and Linux. Pushing a
`v*` tag builds the Windows download and attaches it to a GitHub Release.

## Contributing (people and AI agents)

- **[AGENTS.md](AGENTS.md)** has the build and test commands, the invariants that must stay in sync, and the house rules.
  Codex and Cursor read it directly; Claude Code loads it through [CLAUDE.md](CLAUDE.md).
- **[graphify-out/](graphify-out/)** is a knowledge graph of the codebase
  ([graphify](https://github.com/safishamsi/graphify)). Agents query it instead of grepping.
  To explore it interactively, run `graphify export html` and open the generated `graphify-out/graph.html`.

  ![Knowledge graph of the codebase: 372 nodes in 14 communities, one color per module](docs/knowledge-graph.png)
- **Keeping the graph current:** install graphify (`uv tool install graphifyy`) and run `graphify hook install` once per clone.
  Git then rebuilds the graph after each commit, and the Claude Code and Cursor hooks refresh it after each edit.

## License and credits

Created by **Valentyn Sarkisov** ([@Va1Geny](https://github.com/Va1Geny)).
Parts of the code were refined with the help of AI coding assistants.

Licensed under the [Apache License 2.0](LICENSE). See [NOTICE](NOTICE).
Market data © Finnhub and Yahoo Finance, subject to their terms of use.
