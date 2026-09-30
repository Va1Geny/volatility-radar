#include "mainwindow.h"

#include <QApplication>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char * argv[])
{
	QApplication a(argc, argv);
	a.setOrganizationName("Va1Geny");          // QSettings location
	a.setApplicationName("Volatility Radar");
	a.setWindowIcon(QIcon(":/resources/app.png"));

	a.setStyle(QStyleFactory::create("Fusion"));

	QPalette darkPalette;
	darkPalette.setColor(QPalette::Window, QColor("#131722"));
	darkPalette.setColor(QPalette::WindowText, QColor("#D1D4DC"));
	darkPalette.setColor(QPalette::Base, QColor("#1E222D"));
	darkPalette.setColor(QPalette::AlternateBase, QColor("#242832"));
	darkPalette.setColor(QPalette::Text, QColor("#D1D4DC"));
	darkPalette.setColor(QPalette::Button, QColor("#2A2E39"));
	darkPalette.setColor(QPalette::ButtonText, QColor("#D1D4DC"));
	darkPalette.setColor(QPalette::Highlight, QColor("#2962FF"));
	darkPalette.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
	darkPalette.setColor(QPalette::ToolTipBase, QColor("#1E222D"));
	darkPalette.setColor(QPalette::ToolTipText, QColor("#D1D4DC"));
	darkPalette.setColor(QPalette::PlaceholderText, QColor("#787B86"));
	a.setPalette(darkPalette);

	MainWindow w;
	w.show();
	return a.exec();
}
