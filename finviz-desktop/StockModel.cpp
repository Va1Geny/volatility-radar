#include "StockModel.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

const QStringList StockModel::HEADERS = {
	"Symbol", "Name", "Last Sale", "Volume",
	"Net Change", "% Change", "Sector", "Industry"
};


StockModel::StockModel(QObject * parent)
	: QAbstractTableModel(parent)
{}

const StockRecord & StockModel::recordAt(int row) const
{
	return m_data.at(row);
}


void StockModel::loadFromCsv(const QString & path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

	QByteArray rawData = file.readAll();
	file.close();

	beginResetModel();
	m_data.clear();

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
			r.country  = obj["country"].toString().trimmed();
			r.ipoyear  = obj["ipoyear"].toString().trimmed();
			r.industry = obj["industry"].toString().trimmed();
			r.sector   = obj["sector"].toString().trimmed();

			if (!r.symbol.isEmpty())
			{
				m_data.append(r);
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

			if (!r.symbol.isEmpty())
			{
				m_data.append(r);
			}
		}
	}

	endResetModel();
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
	if (!index.isValid() || role != Qt::DisplayRole)
	{
		return {};
	}

	const StockRecord & record = m_data[index.row()];

	switch (index.column())
	{
		case 0: return record.symbol;
		case 1: return record.name;
		case 2: return QString("$%1").arg(record.lastSale, 0, 'f', 2);
		case 3: return QString::number((long long) record.volume);
		case 4: return QString::number(record.netChange, 'f', 2);
		case 5: return QString("%1%").arg(record.pctChange, 0, 'f', 2);
		case 6: return record.sector;
		case 7: return record.industry;
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