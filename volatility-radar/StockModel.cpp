#include "StockModel.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLocale>
#include <QSet>

const QStringList StockModel::HEADERS = {
	"Symbol", "Name", "Last Sale", "Volume",
	"Net Change", "% Change", "Market Cap", "Sector", "Industry", "Big Move"
};


StockModel::StockModel(QObject * parent)
	: QAbstractTableModel(parent)
{}

const StockRecord & StockModel::recordAt(int row) const
{
	return m_data.at(row);
}

QStringList StockModel::uniqueSectors() const
{
	QSet<QString> sectors;
	for (const StockRecord & r : m_data)
		if (!r.sector.isEmpty()) sectors.insert(r.sector);
	QStringList list(sectors.begin(), sectors.end());
	list.sort();
	return list;
}

void StockModel::updateLivePrice(const QString & symbol, double price)
{
	auto it = m_symbolToRow.constFind(symbol);
	if (it == m_symbolToRow.constEnd()) return;

	const int row = it.value();
	StockRecord & r = m_data[row];

	r.lastSale = price;
	if (r.previousClose > 0.0)
	{
		r.netChange = price - r.previousClose;
		r.pctChange = r.netChange / r.previousClose * 100.0;
	}

	emit dataChanged(index(row, ColLastSale), index(row, ColPctChange),
		{ Qt::DisplayRole, Qt::UserRole });
}

void StockModel::setPrediction(const QString & symbol, double bigMoveProb)
{
	m_bigMoveProb.insert(symbol, bigMoveProb);

	auto it = m_symbolToRow.constFind(symbol);
	if (it == m_symbolToRow.constEnd()) return;

	const int row = it.value();
	emit dataChanged(index(row, ColBigMove), index(row, ColBigMove),
		{ Qt::DisplayRole, Qt::UserRole });
}

// Offline snapshot: the Nasdaq screener JSON export ({"data":{"rows":[...]}}).
void StockModel::loadFromJson(const QString & path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
	{
		qWarning() << "Cannot open snapshot:" << path;
		return;
	}

	QJsonParseError parseError;
	const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
	if (parseError.error != QJsonParseError::NoError)
	{
		qWarning() << "JSON parse error in" << path << ":" << parseError.errorString();
		return;
	}

	auto num = [](const QJsonValue & v)
	{
		return v.toString().remove('$').remove('%').trimmed().toDouble();
	};

	beginResetModel();
	m_data.clear();
	m_symbolToRow.clear();

	const QJsonArray rows = doc.object()["data"].toObject()["rows"].toArray();
	for (const QJsonValue & val : rows)
	{
		const QJsonObject obj = val.toObject();
		StockRecord r;
		r.symbol    = obj["symbol"].toString().trimmed();
		r.name      = obj["name"].toString().trimmed();
		r.lastSale  = num(obj["lastsale"]);
		r.volume    = num(obj["volume"]);
		r.netChange = num(obj["netchange"]);
		r.pctChange = num(obj["pctchange"]);
		r.marketCap = num(obj["marketCap"]);
		r.country   = obj["country"].toString().trimmed();
		r.ipoyear   = obj["ipoyear"].toString().trimmed();
		r.industry  = obj["industry"].toString().trimmed();
		r.sector    = obj["sector"].toString().trimmed();
		r.previousClose = r.lastSale - r.netChange;

		if (r.symbol.isEmpty() || m_symbolToRow.contains(r.symbol)) continue;
		m_symbolToRow.insert(r.symbol, m_data.size());
		m_data.append(r);
	}

	endResetModel();
}

void StockModel::addRecord(const StockRecord & record)
{
	if (record.symbol.isEmpty()) return;

	auto it = m_symbolToRow.constFind(record.symbol);
	if (it != m_symbolToRow.constEnd())
	{
		const int row = it.value();
		m_data[row] = record;
		emit dataChanged(index(row, 0), index(row, ColCount - 1));
		return;
	}

	const int row = m_data.size();
	beginInsertRows(QModelIndex(), row, row);
	m_data.append(record);
	m_symbolToRow.insert(record.symbol, row);
	endInsertRows();
}

int StockModel::rowCount(const QModelIndex &) const
{
	return m_data.size();
}

int StockModel::columnCount(const QModelIndex &) const
{
	return ColCount;
}

QVariant StockModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
	{
		return {};
	}

	const StockRecord & r = m_data[index.row()];
	const double bigMoveProb = m_bigMoveProb.value(r.symbol, -1.0);

	// Raw numbers for sorting and painting. Only numeric columns answer this role.
	if (role == Qt::UserRole)
	{
		switch (index.column())
		{
			case ColLastSale:  return r.lastSale;
			case ColVolume:    return r.volume;
			case ColNetChange: return r.netChange;
			case ColPctChange: return r.pctChange;
			case ColMarketCap: return r.marketCap;
			case ColBigMove:   return bigMoveProb;
			default: return {};
		}
	}

	if (role == Qt::ToolTipRole && index.column() == ColBigMove && bigMoveProb >= 0.0)
	{
		return QString("%1% chance of an unusually large move (up or down) within 5 trading days.\n"
			"A typical stock scores about %2%.")
			.arg(bigMoveProb * 100.0, 0, 'f', 0).arg(BigMoveBaseRate * 100.0, 0, 'f', 0);
	}

	if (role != Qt::DisplayRole)
	{
		return {};
	}

	switch (index.column())
	{
		case ColSymbol:    return r.symbol;
		case ColName:      return r.name;
		case ColLastSale:  return QString("$%1").arg(r.lastSale, 0, 'f', 2);
		case ColVolume:    return r.volume > 0.0
			? QLocale(QLocale::English, QLocale::UnitedStates).toString((qlonglong)r.volume)
			: QString("-");
		case ColNetChange: return QString::number(r.netChange, 'f', 2);
		case ColPctChange: return QString("%1%").arg(r.pctChange, 0, 'f', 2);
		case ColMarketCap:
		{
			if (r.marketCap >= 1e12)
				return QString("$%1T").arg(r.marketCap / 1e12, 0, 'f', 2);
			if (r.marketCap >= 1e9)
				return QString("$%1B").arg(r.marketCap / 1e9, 0, 'f', 2);
			if (r.marketCap >= 1e6)
				return QString("$%1M").arg(r.marketCap / 1e6, 0, 'f', 2);
			if (r.marketCap > 0)
				return QString("$%1K").arg(r.marketCap / 1e3, 0, 'f', 0);
			return QString("-");
		}
		case ColSector:    return r.sector;
		case ColIndustry:  return r.industry;
		case ColBigMove:
			return bigMoveProb < 0.0
				? QString("-")
				: QString("%1%").arg(bigMoveProb * 100.0, 0, 'f', 0);
		default: return {};
	}
}

QVariant StockModel::headerData(int section, Qt::Orientation orientation, int role)
const
{
	if (role == Qt::ToolTipRole && orientation == Qt::Horizontal && section == ColBigMove)
	{
		return QString("Model's probability of an unusually large move (up or down) within 5 trading days.\n"
			"Typical: about %1%. Amber = elevated, red = about twice as likely as usual.")
			.arg(BigMoveBaseRate * 100.0, 0, 'f', 0);
	}

	if (role != Qt::DisplayRole)
	{
		return {};
	}

	if (orientation == Qt::Horizontal)
	{
		return HEADERS.value(section);
	}
	return section + 1;
}
