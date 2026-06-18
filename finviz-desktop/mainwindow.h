#pragma once
#include "StockModel.h"
#include "StockFilterProxy.h"
#include "StockDelegate.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QChartView;
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

private:
	void setupTheme();
	void populateFilterCombos();
	void updateFilterStatus();
	void createEmbeddedChart();

	Ui::MainWindow * ui;
	StockModel * m_model;
	StockFilterProxy * m_proxy;
	StockDelegate * m_delegate;
	QChartView * m_chartView = nullptr;
};
