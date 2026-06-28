#pragma once
#include <QAbstractTableModel>
#include <QString>
#include <QVector>
#include <QSet>
#include <QHash>

struct StockRecord
{
	QString symbol;
	QString name;
	double lastSale = 0.0;
	double volume = 0.0;
	double netChange = 0.0;
	double pctChange = 0.0;
	double marketCap = 0.0;
	double previousClose = 0.0;
	double bigMoveProb = -1.0;
	QString sector;
	QString industry;
	QString country;
	QString ipoyear;
};

class StockModel: public QAbstractTableModel
{
	Q_OBJECT
public:
	enum Column {
		ColSymbol = 0,
		ColName,
		ColLastSale,
		ColVolume,
		ColNetChange,
		ColPctChange,
		ColMarketCap,
		ColSector,
		ColIndustry,
		ColBigMove,
		ColCount
	};

	explicit StockModel(QObject * parent = nullptr);

	void loadFromCsv(const QString & path);

	void clearAll();
	void addRecord(const StockRecord & record);

	int rowCount(const QModelIndex & parent = {}) const override;

	int columnCount(const QModelIndex & parent = {}) const override;

	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	const StockRecord & recordAt(int row) const;

	QStringList uniqueSectors() const;

	QStringList uniqueCountries() const;

	QStringList allSymbols() const;

public slots:
	void updateLivePrice(const QString & symbol, double price, double volume);
	void setPrediction(const QString & symbol, double bigMoveProb);

private:
	QVector<StockRecord> m_data;
	QSet<QString> m_sectors;
	QSet<QString> m_countries;
	QHash<QString, int> m_symbolToRow;
	static const QStringList HEADERS;
};