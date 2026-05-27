#pragma once
#include "StockModel.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
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

private:
	Ui::MainWindow * ui;
	StockModel * m_model;
};
