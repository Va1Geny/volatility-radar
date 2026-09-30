#pragma once
#include <QAbstractTableModel>
#include <QString>
#include <QVector>
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

	// Share of stocks that make a "big move" in a typical week (train prints base_rate).
	// Keep in sync with the alert levels in StockDelegate.h and analyzer config.py.
	static constexpr double BigMoveBaseRate = 0.16;

	explicit StockModel(QObject * parent = nullptr);

	void loadFromJson(const QString & path);

	void addRecord(const StockRecord & record);

	int rowCount(const QModelIndex & parent = {}) const override;

	int columnCount(const QModelIndex & parent = {}) const override;

	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	const StockRecord & recordAt(int row) const;

	QStringList uniqueSectors() const;

public slots:
	void updateLivePrice(const QString & symbol, double price);
	void setPrediction(const QString & symbol, double bigMoveProb);

private:
	QVector<StockRecord> m_data;
	QHash<QString, int> m_symbolToRow;
	// By symbol, not row: predictions can arrive before the stock's row is loaded.
	QHash<QString, double> m_bigMoveProb;
	static const QStringList HEADERS;
};
