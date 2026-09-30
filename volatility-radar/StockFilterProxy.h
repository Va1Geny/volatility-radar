#pragma once

#include <QSortFilterProxyModel>
#include <QString>

class StockFilterProxy : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	explicit StockFilterProxy(QObject* parent = nullptr);

	void setTextFilter(const QString& text);
	void setMinPrice(double minPrice);
	void setMaxPrice(double maxPrice);
	void setSectorFilter(const QString& sector);
	void resetFilters();

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
	bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
	QString m_textFilter;
	double m_minPrice = 0.0;
	double m_maxPrice = 0.0;
	QString m_sectorFilter;
};
