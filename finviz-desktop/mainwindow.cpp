#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QCoreApplication>
#include <QDialog>
#include <qfileinfo.h>
#include <QMap>
#include <QValueAxis>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m_model(new StockModel(this))
{
	ui->setupUi(this);

	QString csvPath = QCoreApplication::applicationDirPath() + "\\stocks.csv";

	QFileInfo check_file(csvPath);
	if (!check_file.exists())
	{
		qWarning() << "Error: stocks.csv file not found at:" << csvPath;
	}
	else
	{
		m_model->loadFromCsv(csvPath);
	}

	ui->tableView->setModel(m_model);
	ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->tableView->horizontalHeader()->setStretchLastSection(true);
	ui->tableView->setSortingEnabled(false);
	ui->splitter->setSizes({ 220, 700, 280 });

	connect(ui->tableView->selectionModel(),
		&QItemSelectionModel::currentRowChanged,
		this, &MainWindow::onRowSelected);

	connect(ui->pushButton, &QPushButton::clicked,
		this, &MainWindow::onShowChart);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::onRowSelected(const QModelIndex & current, const QModelIndex &)
{
	if (!current.isValid()) return;
	const StockRecord & r = m_model->recordAt(current.row());


	ui->label->setText("Ticker: " + r.symbol);
	ui->label_2->setText("Sector: " + r.sector);
	ui->label_3->setText("Price: $" + QString::number(r.lastSale, 'f', 2));
	ui->label_4->setText("Name: " + r.name);
	ui->label_5->setText("Volume: " + QString::number((long long) r.volume));
	ui->label_6->setText("Industry: " + r.industry);
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
		ui->plainTextEdit->setPlainText("No data loaded to chart.");
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
	QStringList categories;
	for (int i = 0; i < maxBars; ++i)
	{
		*set << sorted[i].second;
		categories << sorted[i].first;
	}

	QBarSeries * series = new QBarSeries();
	series->append(set);

	QChart * chart = new QChart();
	chart->addSeries(series);
	chart->setTitle("Stocks by Sector (Top 10)");
	chart->setAnimationOptions(QChart::SeriesAnimations);

	QBarCategoryAxis * axisX = new QBarCategoryAxis();
	axisX->append(categories);
	chart->addAxis(axisX, Qt::AlignBottom);
	series->attachAxis(axisX);

	QValueAxis * axisY = new QValueAxis();
	axisY->setTitleText("Count");
	chart->addAxis(axisY, Qt::AlignLeft);
	series->attachAxis(axisY);

	chart->legend()->setVisible(false);

	QChartView * chartView = new QChartView(chart);
	chartView->setRenderHint(QPainter::Antialiasing);

	QDialog * dlg = new QDialog(this);
	dlg->setWindowTitle("Sector Distribution Chart");
	dlg->resize(900, 600);
	QVBoxLayout * layout = new QVBoxLayout(dlg);
	layout->addWidget(chartView);
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->show();
}
