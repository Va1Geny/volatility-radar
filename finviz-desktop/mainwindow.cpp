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
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QMap>
#include <QSet>
#include <QValueAxis>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m_model(new StockModel(this))
	, m_proxy(new StockFilterProxy(this))
	, m_delegate(new StockDelegate(this))
{
	ui->setupUi(this);
	setupTheme();

	m_token = resolveToken();
	m_watchlist = megaCapWatchlist();

	if (m_token.isEmpty())
	{
		QString csvPath = QCoreApplication::applicationDirPath() + "\\stocks.csv";
		QFileInfo check_file(csvPath);
		if (!check_file.exists())
			qWarning() << "Error: stocks.csv file not found at:" << csvPath;
		m_model->loadFromCsv(csvPath);
	}

	m_proxy->setSourceModel(m_model);

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
	ui->tableView->setColumnWidth(StockModel::ColName, 250);
	ui->tableView->setColumnWidth(StockModel::ColLastSale, 100);
	ui->tableView->setColumnWidth(StockModel::ColVolume, 110);
	ui->tableView->setColumnWidth(StockModel::ColNetChange, 100);
	ui->tableView->setColumnWidth(StockModel::ColPctChange, 90);
	ui->tableView->setColumnWidth(StockModel::ColMarketCap, 110);
	ui->tableView->setColumnWidth(StockModel::ColSector, 140);
	ui->tableView->setColumnWidth(StockModel::ColBigMove, 90);

	ui->splitter->setSizes({ 240, 800, 320 });

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

	setupLiveData();
	setupPredictions();

	statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::setupTheme()
{
	QString qssPath = QCoreApplication::applicationDirPath() + "\\style.qss";
	QFile qssFile(qssPath);
	if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QString styleSheet = qssFile.readAll();
		qApp->setStyleSheet(styleSheet);
		qssFile.close();
	}

	setWindowTitle("FINVIZ TERMINAL");
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
	QChart * chart = new QChart();
	chart->setTitle("Select 'Show Sector Chart' to view distribution");
	chart->legend()->setVisible(false);
	chart->setAnimationOptions(QChart::NoAnimation);

	chart->setBackgroundBrush(QBrush(QColor("#1E222D")));
	chart->setPlotAreaBackgroundBrush(QBrush(QColor("#131722")));
	chart->setPlotAreaBackgroundVisible(true);
	chart->setTitleBrush(QBrush(QColor("#787B86")));
	QFont titleFont("Segoe UI", 10);
	chart->setTitleFont(titleFont);

	m_chartView = new QChartView(chart);
	m_chartView->setRenderHint(QPainter::Antialiasing);
	m_chartView->setStyleSheet("background: transparent; border: none;");

	ui->chartContainerLayout->addWidget(m_chartView);
}

QString MainWindow::resolveToken() const
{
	QString token = qEnvironmentVariable("FINNHUB_API_KEY");
	if (token.isEmpty())
	{
		QFile tokenFile(QCoreApplication::applicationDirPath() + "\\finnhub.token");
		if (tokenFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			token = QString::fromUtf8(tokenFile.readAll()).trimmed();
			tokenFile.close();
		}
	}
	return token;
}

QStringList MainWindow::megaCapWatchlist()
{
	return {
		"AAPL", "MSFT", "GOOGL", "AMZN", "NVDA", "META", "TSLA", "BRK.B",
		"JPM", "V", "MA", "WMT", "JNJ", "PG", "HD", "KO", "PEP", "DIS",
		"NFLX", "AMD", "INTC", "CSCO", "ORCL", "ADBE", "CRM", "NKE", "MCD",
		"BA", "XOM", "CVX", "PFE", "MRK", "BAC", "VZ", "IBM", "QCOM",
		"COST", "TSM", "BABA", "TM", "TXN",
	};
}

void MainWindow::setupLiveData()
{
	m_finnhub = new FinnhubClient(m_token, this);

	if (m_token.isEmpty())
	{
		statusBar()->showMessage(
			"Live data off — set FINNHUB_API_KEY or add finnhub.token next to the app");
		return;
	}

	connect(m_finnhub, &FinnhubClient::tradeReceived,
		m_model, &StockModel::updateLivePrice);
	connect(m_finnhub, &FinnhubClient::tradeReceived,
		this, &MainWindow::onTradeReceived);
	connect(m_finnhub, &FinnhubClient::connected, this, [this]
	{
		statusBar()->showMessage("Live — connected to Finnhub");
	});
	connect(m_finnhub, &FinnhubClient::errorOccurred, this, [this](const QString & msg)
	{
		statusBar()->showMessage("Live error: " + msg);
	});

	m_finnhub->connectToServer();
	m_finnhub->subscribe(m_watchlist);

	m_rest = new FinnhubRest(m_token, this);
	connect(m_rest, &FinnhubRest::recordReady,
		m_model, &StockModel::addRecord);
	connect(m_rest, &FinnhubRest::progress, this, [this](int done, int total)
	{
		ui->stockCountLabel->setText(
			QString("Loading %1/%2 stocks...").arg(done).arg(total));
	});
	connect(m_rest, &FinnhubRest::finished, this, [this]
	{
		populateFilterCombos();
		ui->stockCountLabel->setText(
			QString("%1 stocks loaded").arg(m_model->rowCount()));
		updateFilterStatus();
	});

	m_model->clearAll();
	m_rest->loadSymbols(m_watchlist);
}

void MainWindow::setupPredictions()
{
	m_predictions = new PredictionClient("ws://127.0.0.1:8765", this);
	connect(m_predictions, &PredictionClient::predictionReceived,
		m_model, &StockModel::setPrediction);
	m_predictions->start();
}

void MainWindow::onTradeReceived(const QString & symbol, double, double)
{
	QModelIndex current = ui->tableView->currentIndex();
	if (!current.isValid()) return;

	QModelIndex src = m_proxy->mapToSource(current);
	if (m_model->recordAt(src.row()).symbol == symbol)
		onRowSelected(current, QModelIndex());
}

void MainWindow::onRowSelected(const QModelIndex & current, const QModelIndex &)
{
	if (!current.isValid()) return;

	QModelIndex srcIndex = m_proxy->mapToSource(current);
	const StockRecord & r = m_model->recordAt(srcIndex.row());

	QLocale locale(QLocale::English, QLocale::UnitedStates);

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

	ui->volumeValue->setText(locale.toString((qlonglong)r.volume));

	QString mcapStr;
	if (r.marketCap >= 1e12)
		mcapStr = QString("$%1T").arg(r.marketCap / 1e12, 0, 'f', 2);
	else if (r.marketCap >= 1e9)
		mcapStr = QString("$%1B").arg(r.marketCap / 1e9, 0, 'f', 2);
	else if (r.marketCap >= 1e6)
		mcapStr = QString("$%1M").arg(r.marketCap / 1e6, 0, 'f', 2);
	else if (r.marketCap > 0)
		mcapStr = QString("$%1K").arg(r.marketCap / 1e3, 0, 'f', 0);
	else
		mcapStr = QString::fromUtf8("\xe2\x80\x94");
	ui->marketCapValue->setText(mcapStr);

	ui->sectorValue->setText(r.sector.isEmpty() ? QString::fromUtf8("\xe2\x80\x94") : r.sector);
	ui->industryValue->setText(r.industry.isEmpty() ? QString::fromUtf8("\xe2\x80\x94") : r.industry);
	ui->countryValue->setText(r.country.isEmpty() ? QString::fromUtf8("\xe2\x80\x94") : r.country);
	ui->ipoValue->setText(r.ipoyear.isEmpty() ? QString::fromUtf8("\xe2\x80\x94") : r.ipoyear);
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

	QChart * chart = new QChart();
	chart->addSeries(series);
	chart->setTitle("Stocks by Sector (Top 10)");
	chart->setAnimationOptions(QChart::SeriesAnimations);

	chart->setBackgroundBrush(QBrush(QColor("#1E222D")));
	chart->setPlotAreaBackgroundBrush(QBrush(QColor("#131722")));
	chart->setPlotAreaBackgroundVisible(true);
	chart->setTitleBrush(QBrush(QColor("#D1D4DC")));
	QFont titleFont("Segoe UI", 10, QFont::Bold);
	chart->setTitleFont(titleFont);

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

	chart->legend()->setVisible(false);

	if (m_chartView)
	{
		QChart * oldChart = m_chartView->chart();
		m_chartView->setChart(chart);
		delete oldChart;
	}
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
