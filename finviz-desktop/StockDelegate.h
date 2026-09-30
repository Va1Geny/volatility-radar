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
	static constexpr int RowHeight   = 32;
	static constexpr int CellPadding = 8;

	// Big-move alert levels. The model's base rate is ~16% at move_mult=1.5,
	// so these are ~1.5x and ~2x "normal". Retune with analyzer/predictor/config.py:alert_prob.
	static constexpr double BigMoveWarn  = 0.24;
	static constexpr double BigMoveAlert = 0.32;

	static inline const QColor PositiveColor{0x26, 0xA6, 0x9A};
	static inline const QColor NegativeColor{0xEF, 0x53, 0x50};
	static inline const QColor NeutralColor {0x78, 0x7B, 0x86};
	static inline const QColor WarnColor    {0xF5, 0xA6, 0x23};

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
