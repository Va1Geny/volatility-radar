#include "StockFilterProxy.h"
#include "StockModel.h"

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
		const QString symbol = idx(StockModel::ColSymbol).data(Qt::DisplayRole).toString();
		const QString name   = idx(StockModel::ColName).data(Qt::DisplayRole).toString();

		if (!symbol.contains(m_textFilter, Qt::CaseInsensitive)
			&& !name.contains(m_textFilter, Qt::CaseInsensitive)) {
			return false;
		}
	}

	const double price = idx(StockModel::ColLastSale).data(Qt::UserRole).toDouble();

	if (m_minPrice > 0.0 && price < m_minPrice) {
		return false;
	}

	if (m_maxPrice > 0.0 && price > m_maxPrice) {
		return false;
	}

	if (!m_sectorFilter.isEmpty()) {
		const QString sector = idx(StockModel::ColSector).data(Qt::DisplayRole).toString();
		if (sector != m_sectorFilter) {
			return false;
		}
	}

	return true;
}

// Numeric columns expose their raw value via Qt::UserRole; sort on that, not the formatted text.
bool StockFilterProxy::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
	const QVariant l = left.data(Qt::UserRole);
	const QVariant r = right.data(Qt::UserRole);
	if (l.isValid() && r.isValid()) {
		return l.toDouble() < r.toDouble();
	}

	return QSortFilterProxyModel::lessThan(left, right);
}
