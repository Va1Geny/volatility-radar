#include "StockModel.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLocale>

const QStringList StockModel::HEADERS = {
	"Symbol", "Name", "Last Sale", "Volume",
	"Net Change", "% Change", "Market Cap", "Sector", "Industry", "Volatility"
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
	QStringList list(m_sectors.begin(), m_sectors.end());
	list.sort();
	return list;
}

QStringList StockModel::uniqueCountries() const
{
	QStringList list(m_countries.begin(), m_countries.end());
	list.sort();
	return list;
}

QStringList StockModel::allSymbols() const
{
	QStringList list;
	list.reserve(m_data.size());
	for (const StockRecord & r : m_data)
		list << r.symbol;
	return list;
}

void StockModel::updateLivePrice(const QString & symbol, double price, double volume)
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
	if (volume > 0.0)
		r.volume += volume;

	emit dataChanged(index(row, ColLastSale), index(row, ColPctChange),
		{ Qt::DisplayRole, Qt::UserRole });
}

void StockModel::setPrediction(const QString & symbol, double bigMoveProb)
{
	auto it = m_symbolToRow.constFind(symbol);
	if (it == m_symbolToRow.constEnd()) return;

	const int row = it.value();
	m_data[row].bigMoveProb = bigMoveProb;

	emit dataChanged(index(row, ColBigMove), index(row, ColBigMove),
		{ Qt::DisplayRole, Qt::UserRole });
}

void StockModel::loadFromCsv(const QString & path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

	QByteArray rawData = file.readAll();
	file.close();

	beginResetModel();
	m_data.clear();
	m_sectors.clear();
	m_countries.clear();
	m_symbolToRow.clear();

	QByteArray trimmed = rawData.trimmed();
	if (trimmed.startsWith('{'))
	{
		QJsonParseError parseError;
		QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseError);
		if (parseError.error != QJsonParseError::NoError)
		{
			qWarning() << "JSON parse error:" << parseError.errorString();
			endResetModel();
			return;
		}

		QJsonObject root = doc.object();
		QJsonObject data = root["data"].toObject();
		QJsonArray rows = data["rows"].toArray();

		for (const QJsonValue & val : rows)
		{
			QJsonObject obj = val.toObject();
			StockRecord r;
			r.symbol   = obj["symbol"].toString().trimmed();
			r.name     = obj["name"].toString().trimmed();
			QString lastSaleStr = obj["lastsale"].toString().trimmed();
			lastSaleStr.remove('$');
			r.lastSale = lastSaleStr.toDouble();
			r.volume   = obj["volume"].toString().trimmed().toDouble();
			r.netChange = obj["netchange"].toString().trimmed().toDouble();
			QString pctStr = obj["pctchange"].toString().trimmed();
			pctStr.remove('%');
			r.pctChange = pctStr.toDouble();
			r.marketCap = obj["marketCap"].toString().trimmed().toDouble();
			r.country  = obj["country"].toString().trimmed();
			r.ipoyear  = obj["ipoyear"].toString().trimmed();
			r.industry = obj["industry"].toString().trimmed();
			r.sector   = obj["sector"].toString().trimmed();
			r.previousClose = r.lastSale - r.netChange;

			if (!r.symbol.isEmpty())
			{
				m_data.append(r);
				m_symbolToRow.insert(r.symbol, m_data.size() - 1);
				if (!r.sector.isEmpty()) m_sectors.insert(r.sector);
				if (!r.country.isEmpty()) m_countries.insert(r.country);
			}
		}
	}
	else
	{
		QTextStream in(rawData);
		in.readLine();

		while (!in.atEnd())
		{
			QString line = in.readLine();
			QStringList fields = line.split(",");

			if (fields.size() < 11)
			{
				break;
			}

			StockRecord r;
			r.symbol   = fields[0].trimmed().remove('"');
			r.name     = fields[1].trimmed().remove('"');
			r.lastSale = fields[2].trimmed().remove('"').remove('$').toDouble();
			r.netChange = fields[4].trimmed().remove('"').toDouble();
			r.pctChange = fields[5].trimmed().remove('"').remove('%').toDouble();
			r.volume   = fields[3].trimmed().remove('"').toDouble();
			r.country  = fields[7].trimmed().remove('"');
			r.ipoyear  = fields[8].trimmed().remove('"');
			r.industry = fields[9].trimmed().remove('"');
			r.sector   = fields[10].trimmed().remove('"');
			r.previousClose = r.lastSale - r.netChange;

			if (!r.symbol.isEmpty())
			{
				m_data.append(r);
				m_symbolToRow.insert(r.symbol, m_data.size() - 1);
				if (!r.sector.isEmpty()) m_sectors.insert(r.sector);
				if (!r.country.isEmpty()) m_countries.insert(r.country);
			}
		}
	}

	endResetModel();
}

void StockModel::clearAll()
{
	beginResetModel();
	m_data.clear();
	m_sectors.clear();
	m_countries.clear();
	m_symbolToRow.clear();
	endResetModel();
}

void StockModel::addRecord(const StockRecord & record)
{
	if (record.symbol.isEmpty()) return;

	auto it = m_symbolToRow.constFind(record.symbol);
	if (it != m_symbolToRow.constEnd())
	{
		const int row = it.value();
		const double keepProb = m_data[row].bigMoveProb;
		m_data[row] = record;
		if (record.bigMoveProb < 0.0)
			m_data[row].bigMoveProb = keepProb;
		emit dataChanged(index(row, 0), index(row, ColCount - 1));
		return;
	}

	const int row = m_data.size();
	beginInsertRows(QModelIndex(), row, row);
	m_data.append(record);
	m_symbolToRow.insert(record.symbol, row);
	if (!record.sector.isEmpty()) m_sectors.insert(record.sector);
	if (!record.country.isEmpty()) m_countries.insert(record.country);
	endInsertRows();
}

int StockModel::rowCount(const QModelIndex &) const
{
	return m_data.size();
}

int StockModel::columnCount(const QModelIndex &) const
{
	return HEADERS.size();
}

QVariant StockModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
	{
		return {};
	}

	const StockRecord & r = m_data[index.row()];

	if (role == Qt::UserRole)
	{
		switch (index.column())
		{
			case ColLastSale:  return r.lastSale;
			case ColVolume:    return r.volume;
			case ColNetChange: return r.netChange;
			case ColPctChange: return r.pctChange;
			case ColMarketCap: return r.marketCap;
			case ColBigMove:   return r.bigMoveProb;
			default: return {};
		}
	}

	if (role == Qt::TextAlignmentRole)
	{
		switch (index.column())
		{
			case ColLastSale:
			case ColVolume:
			case ColNetChange:
			case ColPctChange:
			case ColMarketCap:
				return QVariant(Qt::AlignRight | Qt::AlignVCenter);
			case ColBigMove:
				return QVariant(Qt::AlignCenter);
			default:
				return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
		}
	}

	if (role != Qt::DisplayRole)
	{
		return {};
	}

	QLocale locale(QLocale::English, QLocale::UnitedStates);

	switch (index.column())
	{
		case ColSymbol:    return r.symbol;
		case ColName:      return r.name;
		case ColLastSale:  return QString("$%1").arg(r.lastSale, 0, 'f', 2);
		case ColVolume:    return r.volume > 0.0
			? locale.toString((qlonglong)r.volume)
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
			return r.bigMoveProb < 0.0
				? QString("-")
				: QString("%1%").arg(r.bigMoveProb * 100.0, 0, 'f', 0);
		default: return {};
	}
}

QVariant StockModel::headerData(int section, Qt::Orientation orientation, int role)
const
{
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