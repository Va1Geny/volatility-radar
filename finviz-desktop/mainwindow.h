#pragma once
#include "StockModel.h"
#include "StockFilterProxy.h"
#include "StockDelegate.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QChartView;
QT_END_NAMESPACE

class FinnhubClient;
class FinnhubRest;

class MainWindow: public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget * parent = nullptr);
	~MainWindow();

private slots:
	void onRowSelected(const QModelIndex & current, const QModelIndex & previous);
	void onShowChart();
	void onApplyFilters();
	void onResetFilters();
	void onSearchTextChanged(const QString & text);
	void onTradeReceived(const QString & symbol, double price, double volume);

private:
	void setupTheme();
	void populateFilterCombos();
	void updateFilterStatus();
	void createEmbeddedChart();
	void setupLiveData();
	QString resolveToken() const;
	static QStringList megaCapWatchlist();

	Ui::MainWindow * ui;
	StockModel * m_model;
	StockFilterProxy * m_proxy;
	StockDelegate * m_delegate;
	QChartView * m_chartView = nullptr;
	FinnhubClient * m_finnhub = nullptr;
	FinnhubRest * m_rest = nullptr;
	QString m_token;
	QStringList m_watchlist;
};
