#pragma once
#include "StockModel.h"
#include "StockFilterProxy.h"
#include "StockDelegate.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QChartView;
class QLabel;
QT_END_NAMESPACE

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
	void onTradeReceived(const QString & symbol);

private:
	void setupTheme();
	void populateFilterCombos();
	void updateFilterStatus();
	void createEmbeddedChart();
	void setupLiveData(const QString & token);
	void setupPredictions();

	Ui::MainWindow * ui;
	StockModel * m_model;
	StockFilterProxy * m_proxy;
	StockDelegate * m_delegate;
	QChartView * m_chartView = nullptr;
	QLabel * m_predictionStatus = nullptr;
};
