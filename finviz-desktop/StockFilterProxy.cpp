#include "StockFilterProxy.h"

static constexpr int ColSymbol    = 0;
static constexpr int ColName      = 1;
static constexpr int ColLastSale  = 2;
static constexpr int ColVolume    = 3;
static constexpr int ColNetChange = 4;
static constexpr int ColPctChange = 5;
static constexpr int ColMarketCap = 6;
static constexpr int ColSector    = 7;

static bool isNumericColumn(int col)
{
	return col == ColLastSale
		|| col == ColVolume
		|| col == ColNetChange
		|| col == ColPctChange
		|| col == ColMarketCap;
}

StockFilterProxy::StockFilterProxy(QObject* parent)
	: QSortFilterProxyModel(parent)
{
}

void StockFilterProxy::setTextFilter(const QString& text)
{
	m_textFilter = text;
	invalidateFilter();
}

void StockFilterProxy::setMinPrice(double minPrice)
{
	m_minPrice = minPrice;
	invalidateFilter();
}

void StockFilterProxy::setMaxPrice(double maxPrice)
{
	m_maxPrice = maxPrice;
	invalidateFilter();
}

void StockFilterProxy::setSectorFilter(const QString& sector)
{
	m_sectorFilter = sector;
	invalidateFilter();
}

void StockFilterProxy::resetFilters()
{
	m_textFilter.clear();
	m_minPrice = 0.0;
	m_maxPrice = 0.0;
	m_sectorFilter.clear();
	invalidateFilter();
}

bool StockFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	auto idx = [&](int col) {
		return sourceModel()->index(sourceRow, col, sourceParent);
	};

	if (!m_textFilter.isEmpty()) {
		const QString symbol = idx(ColSymbol).data(Qt::DisplayRole).toString();
		const QString name   = idx(ColName).data(Qt::DisplayRole).toString();

		if (!symbol.contains(m_textFilter, Qt::CaseInsensitive)
			&& !name.contains(m_textFilter, Qt::CaseInsensitive)) {
			return false;
		}
	}

	const double price = idx(ColLastSale).data(Qt::UserRole).toDouble();

	if (m_minPrice > 0.0 && price < m_minPrice) {
		return false;
	}

	if (m_maxPrice > 0.0 && price > m_maxPrice) {
		return false;
	}

	if (!m_sectorFilter.isEmpty()) {
		const QString sector = idx(ColSector).data(Qt::DisplayRole).toString();
		if (sector != m_sectorFilter) {
			return false;
		}
	}

	return true;
}

bool StockFilterProxy::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
	if (isNumericColumn(left.column())) {
		const double lVal = left.data(Qt::UserRole).toDouble();
		const double rVal = right.data(Qt::UserRole).toDouble();
		return lVal < rVal;
	}

	return QSortFilterProxyModel::lessThan(left, right);
}
