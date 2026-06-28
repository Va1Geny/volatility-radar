#pragma once

#include <QStyledItemDelegate>

class StockDelegate : public QStyledItemDelegate
{
public:
	explicit StockDelegate(QObject *parent = nullptr);

	void paint(QPainter *painter,
			   const QStyleOptionViewItem &option,
			   const QModelIndex &index) const override;

	QSize sizeHint(const QStyleOptionViewItem &option,
				   const QModelIndex &index) const override;

private:
	enum Column {
		Symbol    = 0,
		Name      = 1,
		Price     = 2,
		Volume    = 3,
		NetChange = 4,
		PctChange = 5,
		MarketCap = 6,
		Sector    = 7,
		Industry  = 8,
		BigMove   = 9
	};

	static constexpr int RowHeight   = 32;
	static constexpr int CellPadding = 8;

	static inline const QColor PositiveColor{0x26, 0xA6, 0x9A};
	static inline const QColor NegativeColor{0xEF, 0x53, 0x50};
	static inline const QColor NeutralColor {0x78, 0x7B, 0x86};

	static double numericValue(const QModelIndex &index);

	void paintChangeCell(QPainter *painter,
						 const QStyleOptionViewItem &option,
						 const QModelIndex &index) const;

	void paintNumericCell(QPainter *painter,
						  const QStyleOptionViewItem &option,
						  const QModelIndex &index) const;

	void paintSymbolCell(QPainter *painter,
						 const QStyleOptionViewItem &option,
						 const QModelIndex &index) const;

	void paintTextCell(QPainter *painter,
					   const QStyleOptionViewItem &option,
					   const QModelIndex &index) const;

	void paintBigMoveCell(QPainter *painter,
						  const QStyleOptionViewItem &option,
						  const QModelIndex &index) const;

	QColor drawBackground(QPainter *painter,
						  const QStyleOptionViewItem &option) const;
};
