#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "FinnhubClient.h"
#include "FinnhubRest.h"
#include "PredictionClient.h"
#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineSeries>
#include <QMap>
#include <QProcess>
#include <QSettings>
#include <QStatusBar>
#include <QValueAxis>
#include <QVBoxLayout>

// The source tree when built from source (so edits apply without a copy step), else next to the exe.
static QString appFile(const QString & name)
{
	const QString inSource = QString(APP_SOURCE_DIR) + '/' + name;
	return QFileInfo::exists(inSource) ? inSource : QCoreApplication::applicationDirPath() + '/' + name;
}

// FINNHUB_API_KEY from the environment, else from a .env file (KEY=value lines).
static QString resolveToken()
{
	const QString fromEnv = qEnvironmentVariable("FINNHUB_API_KEY").trimmed();
	if (!fromEnv.isEmpty()) return fromEnv;

	QFile env(appFile(".env"));
	if (!env.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
	const QString key = "FINNHUB_API_KEY=";
	while (!env.atEnd())
	{
		const QString line = QString::fromUtf8(env.readLine()).trimmed();
		if (line.startsWith(key))
			return line.mid(key.size()).remove('"').remove('\'').trimmed();
	}
	return {};
}

// One symbol per line; '#' starts a comment. Shared with analyzer/predictor/config.py.
static QStringList loadWatchlist()
{
	QStringList symbols;
	QFile file(appFile("watchlist.txt"));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "watchlist.txt not found";
		return symbols;
	}
	while (!file.atEnd())
	{
		const QString sym = QString::fromUtf8(file.readLine()).section('#', 0, 0).trimmed().toUpper();
		if (!sym.isEmpty()) symbols << sym;
	}
	return symbols;
}

// Empty chart in the app's dark theme.
static QChart * themedChart(const QString & title)
{
	QChart * chart = new QChart();
	chart->setTitle(title);
	chart->legend()->setVisible(false);
	chart->setBackgroundBrush(QBrush(QColor("#1E222D")));
	chart->setPlotAreaBackgroundBrush(QBrush(QColor("#131722")));
	chart->setPlotAreaBackgroundVisible(true);
	chart->setTitleBrush(QBrush(QColor("#D1D4DC")));
	chart->setTitleFont(QFont("Segoe UI", 10, QFont::Bold));
	return chart;
}

// Gray, non-bold title for "nothing to show yet" messages.
static QChart * placeholderChart(const QString & message)
{
	QChart * chart = themedChart(message);
	chart->setTitleBrush(QBrush(QColor("#787B86")));
	chart->setTitleFont(QFont("Segoe UI", 10));
	return chart;
}

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m_model(new StockModel(this))
	, m_proxy(new StockFilterProxy(this))
	, m_delegate(new StockDelegate(this))
{
	ui->setupUi(this);
	setupTheme();

	const QString token = resolveToken();
	if (token.isEmpty())
		m_model->loadFromJson(appFile("stocks.json"));

	m_proxy->setSourceModel(m_model);
	// Live ticks must not reshuffle rows under the cursor: re-sort only on header click or after a load.
	m_proxy->setDynamicSortFilter(false);

	ui->tableView->setModel(m_proxy);
	ui->tableView->setItemDelegate(m_delegate);
	ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->tableView->horizontalHeader()->setStretchLastSection(true);
	ui->tableView->horizontalHeader()->setHighlightSections(false);
	ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	ui->tableView->verticalHeader()->setVisible(false);
	ui->tableView->verticalHeader()->setDefaultSectionSize(32);
	ui->tableView->setShowGrid(true);
	ui->tableView->setSortingEnabled(true);
	ui->tableView->setAlternatingRowColors(true);

	ui->tableView->setColumnWidth(StockModel::ColSymbol, 80);
	ui->tableView->setColumnWidth(StockModel::ColName, 220);
	ui->tableView->setColumnWidth(StockModel::ColLastSale, 100);
	ui->tableView->setColumnWidth(StockModel::ColVolume, 110);
	ui->tableView->setColumnWidth(StockModel::ColNetChange, 100);
	ui->tableView->setColumnWidth(StockModel::ColPctChange, 90);
	ui->tableView->setColumnWidth(StockModel::ColMarketCap, 110);
	ui->tableView->setColumnWidth(StockModel::ColSector, 140);
	ui->tableView->setColumnWidth(StockModel::ColBigMove, 90);

	ui->splitter->setSizes({ 240, 800, 320 });

	// Window size, panel layout and sort order from the last session (saved in closeEvent).
	QSettings settings;
	restoreGeometry(settings.value("geometry").toByteArray());
	ui->splitter->restoreState(settings.value("splitter").toByteArray());
	ui->tableView->sortByColumn(settings.value("sortColumn", StockModel::ColSymbol).toInt(),
		Qt::SortOrder(settings.value("sortOrder", Qt::AscendingOrder).toInt()));

	populateFilterCombos();

	ui->stockCountLabel->setText(
		QString("%1 stocks loaded").arg(m_model->rowCount()));
	updateFilterStatus();

	connect(ui->tableView->selectionModel(),
		&QItemSelectionModel::currentRowChanged,
		this, &MainWindow::onRowSelected);

	connect(ui->showChartBtn, &QPushButton::clicked,
		this, &MainWindow::onShowChart);

	connect(ui->applyBtn, &QPushButton::clicked,
		this, &MainWindow::onApplyFilters);

	connect(ui->resetBtn, &QPushButton::clicked,
		this, &MainWindow::onResetFilters);

	connect(ui->searchBar, &QLineEdit::textChanged,
		this, &MainWindow::onSearchTextChanged);

	createEmbeddedChart();

	m_predictionStatus = new QLabel("Predictions: offline", this);
	statusBar()->addPermanentWidget(m_predictionStatus);

	setupLiveData(token);
	setupPredictions();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent * event)
{
	QSettings settings;
	settings.setValue("geometry", saveGeometry());
	settings.setValue("splitter", ui->splitter->saveState());
	settings.setValue("sortColumn", m_proxy->sortColumn());
	settings.setValue("sortOrder", int(m_proxy->sortOrder()));
	QMainWindow::closeEvent(event);
}

void MainWindow::setupTheme()
{
	QFile qssFile(appFile("style.qss"));
	if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text))
		qApp->setStyleSheet(QString::fromUtf8(qssFile.readAll()));

	setWindowTitle("VOLATILITY RADAR");
	resize(1600, 900);
}

void MainWindow::populateFilterCombos()
{
	ui->sectorCombo->clear();
	ui->sectorCombo->addItem("All Sectors", "");
	const QStringList sectors = m_model->uniqueSectors();
	for (const QString & s : sectors)
	{
		ui->sectorCombo->addItem(s, s);
	}
}

void MainWindow::updateFilterStatus()
{
	int showing = m_proxy->rowCount();
	int total = m_model->rowCount();
	if (showing == total)
	{
		ui->filterStatusLabel->setText("Showing all stocks");
	}
	else
	{
		ui->filterStatusLabel->setText(
			QString("Showing %1 of %2 stocks").arg(showing).arg(total));
	}
}

void MainWindow::createEmbeddedChart()
{
	m_chartView = new QChartView(
		placeholderChart("Select a stock for its price history,\nor use 'Show Sector Chart'"));
	m_chartView->setRenderHint(QPainter::Antialiasing);
	m_chartView->setStyleSheet("background: transparent; border: none;");

	ui->chartContainerLayout->addWidget(m_chartView);
}

void MainWindow::setupLiveData(const QString & token)
{
	if (token.isEmpty())
	{
		statusBar()->showMessage(
			"Offline snapshot. Put FINNHUB_API_KEY in .env for live data (see README).");
		return;
	}

	// Finnhub's free plan has no daily volume or sub-industry: hide the always-empty columns.
	ui->tableView->setColumnHidden(StockModel::ColVolume, true);
	ui->tableView->setColumnHidden(StockModel::ColIndustry, true);

	const QStringList watchlist = loadWatchlist();

	auto * finnhub = new FinnhubClient(token, this);
	connect(finnhub, &FinnhubClient::tradeReceived,
		m_model, &StockModel::updateLivePrice);
	connect(finnhub, &FinnhubClient::tradeReceived,
		this, &MainWindow::onTradeReceived);
	connect(finnhub, &FinnhubClient::connected, this, [this]
	{
		statusBar()->showMessage("Live: connected to Finnhub");
	});
	connect(finnhub, &FinnhubClient::disconnected, this, [this]
	{
		statusBar()->showMessage("Live: disconnected, reconnecting...");
	});
	connect(finnhub, &FinnhubClient::errorOccurred, this, [this](const QString & msg)
	{
		statusBar()->showMessage("Live error: " + msg);
	});
	finnhub->subscribe(watchlist);
	finnhub->connectToServer();

	auto * rest = new FinnhubRest(token, this);
	connect(rest, &FinnhubRest::recordReady,
		m_model, &StockModel::addRecord);
	connect(rest, &FinnhubRest::errorOccurred, this, [this](const QString & msg)
	{
		statusBar()->showMessage("Load error: " + msg, 5000);
	});
	connect(rest, &FinnhubRest::progress, this, [this](int done, int total)
	{
		ui->stockCountLabel->setText(
			QString("Loading %1/%2 stocks...").arg(done).arg(total));
	});
	connect(rest, &FinnhubRest::finished, this, [this]
	{
		populateFilterCombos();
		ui->stockCountLabel->setText(
			QString("%1 stocks loaded").arg(m_model->rowCount()));
		m_proxy->sort(m_proxy->sortColumn(), m_proxy->sortOrder());
		updateFilterStatus();
	});
	rest->loadSymbols(watchlist);
}

void MainWindow::setupPredictions()
{
	startPredictionServer();

	auto * predictions = new PredictionClient("ws://127.0.0.1:8765", this);
	connect(predictions, &PredictionClient::predictionReceived,
		m_model, &StockModel::setPrediction);
	connect(predictions, &PredictionClient::predictionReceived,
		this, &MainWindow::onPrediction);
	connect(predictions, &PredictionClient::connected, this, [this]
	{
		m_predictionStatus->setText("Predictions: live");
	});
	connect(predictions, &PredictionClient::disconnected, this, [this]
	{
		m_predictionStatus->setText("Predictions: offline");
	});
	predictions->start();
}

// Launch the analyzer's prediction feed in the background if it is set up
// (venv + trained model). If another copy already runs, ours fails to bind and exits.
// The QProcess is owned by the window, so it is stopped when the app closes.
void MainWindow::startPredictionServer()
{
	const QString analyzer = appFile("analyzer");
#ifdef Q_OS_WIN
	const QString python = analyzer + "/.venv/Scripts/python.exe";
#else
	const QString python = analyzer + "/.venv/bin/python";
#endif
	if (!QFileInfo::exists(python) || !QFileInfo::exists(analyzer + "/artifacts/bigmove_cnn.keras"))
		return;

	auto * server = new QProcess(this);
	server->setWorkingDirectory(analyzer);
	server->setStandardOutputFile(QProcess::nullDevice());
	server->setStandardErrorFile(QProcess::nullDevice());
	server->start(python, { "-m", "predictor.serve" });
	m_predictionStatus->setText("Predictions: starting...");
}

QString MainWindow::currentSymbol() const
{
	const QModelIndex current = ui->tableView->currentIndex();
	return current.isValid() ? m_model->recordAt(m_proxy->mapToSource(current).row()).symbol : QString();
}

void MainWindow::onPrediction(const QString & symbol, double, const QList<double> & closes)
{
	if (closes.isEmpty()) return;
	m_history.insert(symbol, closes);
	if (symbol == currentSymbol())
		showPriceChart(symbol);
}

void MainWindow::onTradeReceived(const QString & symbol)
{
	if (symbol == currentSymbol())
		updateDetails(m_proxy->mapToSource(ui->tableView->currentIndex()).row());
}

void MainWindow::onRowSelected(const QModelIndex & current, const QModelIndex &)
{
	if (!current.isValid()) return;
	const int row = m_proxy->mapToSource(current).row();
	updateDetails(row);
	showPriceChart(m_model->recordAt(row).symbol);
}

void MainWindow::setChart(QChart * chart)
{
	QChart * old = m_chartView->chart();
	m_chartView->setChart(chart);
	delete old;
}

void MainWindow::showPriceChart(const QString & symbol)
{
	const QList<double> closes = m_history.value(symbol);
	if (closes.size() < 2)
	{
		setChart(placeholderChart(symbol + ": no price history yet.\n"
			"It arrives with the analyzer's predictions (watchlist stocks only)."));
		return;
	}

	auto * series = new QLineSeries();
	for (int i = 0; i < closes.size(); ++i)
		series->append(i, closes[i]);
	series->setPen(QPen(QColor(closes.last() >= closes.first() ? "#26A69A" : "#EF5350"), 2));

	QChart * chart = themedChart(QString("%1 - last %2 trading days").arg(symbol).arg(closes.size()));
	chart->addSeries(series);
	chart->createDefaultAxes();
	chart->axes(Qt::Horizontal).first()->hide();  // x is just the bar index

	auto * axisY = static_cast<QValueAxis *>(chart->axes(Qt::Vertical).first());
	axisY->setLabelFormat("$%.0f");
	axisY->setLabelsColor(QColor("#787B86"));
	axisY->setGridLineColor(QColor("#2A2E39"));
	axisY->setLabelsFont(QFont("Segoe UI", 8));

	setChart(chart);
}

void MainWindow::updateDetails(int row)
{
	const StockRecord & r = m_model->recordAt(row);

	auto orDash = [](const QString & s)
	{
		return s.isEmpty() || s == "-" ? QString::fromUtf8("\xe2\x80\x94") : s;
	};
	auto cell = [&](int col)
	{
		return orDash(m_model->index(row, col).data().toString());
	};

	ui->tickerLabel->setText(r.symbol);
	ui->companyLabel->setText(r.name);
	ui->priceLabel_detail->setText(QString("$%1").arg(r.lastSale, 0, 'f', 2));

	QString changeColor = r.netChange >= 0 ? "#26A69A" : "#EF5350";
	QString changeSign = r.netChange >= 0 ? "+" : "";
	ui->netChangeLabel->setText(QString("%1%2").arg(changeSign).arg(r.netChange, 0, 'f', 2));
	ui->netChangeLabel->setStyleSheet(
		QString("font-size: 14px; font-weight: bold; font-family: Consolas; color: %1;").arg(changeColor));

	ui->pctChangeLabel->setText(QString("(%1%2%)").arg(changeSign).arg(r.pctChange, 0, 'f', 2));
	ui->pctChangeLabel->setStyleSheet(
		QString("font-size: 14px; font-weight: bold; font-family: Consolas; color: %1;").arg(changeColor));

	ui->volumeValue->setText(cell(StockModel::ColVolume));
	ui->marketCapValue->setText(cell(StockModel::ColMarketCap));
	ui->sectorValue->setText(orDash(r.sector));
	ui->industryValue->setText(orDash(r.industry));
	ui->countryValue->setText(orDash(r.country));
	ui->ipoValue->setText(orDash(r.ipoyear));
}

void MainWindow::onShowChart()
{
	QMap<QString, int> sectorCounts;
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		const StockRecord & r = m_model->recordAt(i);
		QString sector = r.sector.isEmpty() ? "Unknown" : r.sector;
		sectorCounts[sector]++;
	}

	if (sectorCounts.isEmpty())
	{
		return;
	}

	QList<QPair<QString, int>> sorted;
	for (auto it = sectorCounts.constBegin(); it != sectorCounts.constEnd(); ++it)
	{
		sorted.append({ it.key(), it.value() });
	}
	std::sort(sorted.begin(), sorted.end(),
		[](const QPair<QString, int> & a, const QPair<QString, int> & b)
		{
			return a.second > b.second;
		});

	int maxBars = qMin(sorted.size(), 10);

	QBarSet * set = new QBarSet("Stocks per Sector");
	set->setColor(QColor("#2962FF"));
	set->setBorderColor(QColor("#3179F5"));
	QStringList categories;
	for (int i = 0; i < maxBars; ++i)
	{
		*set << sorted[i].second;
		categories << sorted[i].first;
	}

	QBarSeries * series = new QBarSeries();
	series->append(set);
	series->setBarWidth(0.7);

	QChart * chart = themedChart("Stocks by Sector (Top 10)");
	chart->addSeries(series);
	chart->setAnimationOptions(QChart::SeriesAnimations);

	QBarCategoryAxis * axisX = new QBarCategoryAxis();
	axisX->append(categories);
	axisX->setLabelsColor(QColor("#787B86"));
	axisX->setGridLineColor(QColor("#2A2E39"));
	axisX->setLabelsAngle(-45);
	QFont axisFont("Segoe UI", 8);
	axisX->setLabelsFont(axisFont);
	chart->addAxis(axisX, Qt::AlignBottom);
	series->attachAxis(axisX);

	QValueAxis * axisY = new QValueAxis();
	axisY->setTitleText("Count");
	axisY->setTitleBrush(QBrush(QColor("#787B86")));
	axisY->setLabelsColor(QColor("#787B86"));
	axisY->setGridLineColor(QColor("#2A2E39"));
	axisY->setLabelsFont(axisFont);
	chart->addAxis(axisY, Qt::AlignLeft);
	series->attachAxis(axisY);

	setChart(chart);
}

void MainWindow::onApplyFilters()
{
	QString sector = ui->sectorCombo->currentData().toString();
	m_proxy->setSectorFilter(sector);

	m_proxy->setMinPrice(ui->minPriceSpin->value());
	m_proxy->setMaxPrice(ui->maxPriceSpin->value());

	updateFilterStatus();
}

void MainWindow::onResetFilters()
{
	ui->searchBar->clear();
	ui->sectorCombo->setCurrentIndex(0);
	ui->minPriceSpin->setValue(0.0);
	ui->maxPriceSpin->setValue(0.0);

	m_proxy->resetFilters();
	updateFilterStatus();
}

void MainWindow::onSearchTextChanged(const QString & text)
{
	m_proxy->setTextFilter(text);
	updateFilterStatus();
}
