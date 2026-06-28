#include "StockDelegate.h"

#include <QPainter>
#include <QApplication>

StockDelegate::StockDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{
}

QSize StockDelegate::sizeHint(const QStyleOptionViewItem &option,
							  const QModelIndex &index) const
{
	QSize hint = QStyledItemDelegate::sizeHint(option, index);
	hint.setHeight(RowHeight);
	return hint;
}

void StockDelegate::paint(QPainter *painter,
						  const QStyleOptionViewItem &option,
						  const QModelIndex &index) const
{
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);

	switch (index.column()) {
	case Column::NetChange:
	case Column::PctChange:
		paintChangeCell(painter, option, index);
		break;

	case Column::Price:
	case Column::Volume:
	case Column::MarketCap:
		paintNumericCell(painter, option, index);
		break;

	case Column::Symbol:
		paintSymbolCell(painter, option, index);
		break;

	case Column::BigMove:
		paintBigMoveCell(painter, option, index);
		break;

	default:
		paintTextCell(painter, option, index);
		break;
	}

	painter->restore();
}

double StockDelegate::numericValue(const QModelIndex &index)
{
	QVariant raw = index.data(Qt::UserRole);
	if (raw.isValid() && raw.canConvert<double>())
		return raw.toDouble();

	QString text = index.data(Qt::DisplayRole).toString().trimmed();
	text.remove(QChar('$'));
	text.remove(QChar('%'));
	text.remove(QChar(','));
	text.remove(QChar('+'));

	bool ok = false;
	double val = text.toDouble(&ok);
	return ok ? val : 0.0;
}

QColor StockDelegate::drawBackground(QPainter *painter,
									  const QStyleOptionViewItem &option) const
{
	const bool selected = option.state & QStyle::State_Selected;
	const bool hovered  = option.state & QStyle::State_MouseOver;

	if (selected) {
		painter->fillRect(option.rect, option.palette.highlight());
		return option.palette.highlightedText().color();
	}

	if (hovered) {
		QColor hover = option.palette.highlight().color();
		hover.setAlphaF(0.08);
		painter->fillRect(option.rect, hover);
	}

	return option.palette.text().color();
}

void StockDelegate::paintChangeCell(QPainter *painter,
									const QStyleOptionViewItem &option,
									const QModelIndex &index) const
{
	QColor defaultFg = drawBackground(painter, option);
	const bool selected = option.state & QStyle::State_Selected;

	const double value = numericValue(index);
	const QString text = index.data(Qt::DisplayRole).toString().trimmed();

	QColor fg;
	QColor pillBg;
	if (qFuzzyIsNull(value)) {
		fg = selected ? defaultFg : NeutralColor;
	} else if (value > 0.0) {
		fg     = selected ? defaultFg : PositiveColor;
		pillBg = PositiveColor;
		pillBg.setAlphaF(0.20);
	} else {
		fg     = selected ? defaultFg : NegativeColor;
		pillBg = NegativeColor;
		pillBg.setAlphaF(0.20);
	}

	QRect contentRect = option.rect.adjusted(CellPadding, 0, -CellPadding, 0);

	if (!qFuzzyIsNull(value) && !selected) {
		QFontMetrics fm(option.font);
		int textWidth = fm.horizontalAdvance(text);
		int pillW = textWidth + 12;
		int pillH = qMin(option.rect.height() - 4, 22);
		int pillX = contentRect.right() - pillW;
		int pillY = option.rect.center().y() - pillH / 2;

		QRectF pillRect(pillX, pillY, pillW, pillH);
		painter->setPen(Qt::NoPen);
		painter->setBrush(pillBg);
		painter->drawRoundedRect(pillRect, pillH / 2.0, pillH / 2.0);

		painter->setPen(fg);
		painter->setFont(option.font);
		painter->drawText(pillRect, Qt::AlignCenter, text);
	} else {
		painter->setPen(fg);
		painter->setFont(option.font);
		painter->drawText(contentRect, Qt::AlignVCenter | Qt::AlignRight, text);
	}
}

void StockDelegate::paintNumericCell(QPainter *painter,
									 const QStyleOptionViewItem &option,
									 const QModelIndex &index) const
{
	QColor fg = drawBackground(painter, option);

	QFont mono(QStringLiteral("Consolas"), 11);
	mono.setStyleHint(QFont::Monospace);
	painter->setFont(mono);
	painter->setPen(fg);

	QRect contentRect = option.rect.adjusted(CellPadding, 0, -CellPadding, 0);
	QString text = index.data(Qt::DisplayRole).toString();
	painter->drawText(contentRect, Qt::AlignVCenter | Qt::AlignRight, text);
}

void StockDelegate::paintSymbolCell(QPainter *painter,
									const QStyleOptionViewItem &option,
									const QModelIndex &index) const
{
	QColor fg = drawBackground(painter, option);

	QFont bold = option.font;
	bold.setBold(true);
	bold.setPointSizeF(bold.pointSizeF() + 1.0);
	painter->setFont(bold);
	painter->setPen(fg);

	QRect contentRect = option.rect.adjusted(CellPadding, 0, -CellPadding, 0);
	QString text = index.data(Qt::DisplayRole).toString();
	painter->drawText(contentRect, Qt::AlignVCenter | Qt::AlignLeft, text);
}

void StockDelegate::paintTextCell(QPainter *painter,
								  const QStyleOptionViewItem &option,
								  const QModelIndex &index) const
{
	QColor fg = drawBackground(painter, option);

	painter->setFont(option.font);
	painter->setPen(fg);

	QRect contentRect = option.rect.adjusted(CellPadding, 0, -CellPadding, 0);
	QString text = index.data(Qt::DisplayRole).toString();
	painter->drawText(contentRect, Qt::AlignVCenter | Qt::AlignLeft, text);
}

void StockDelegate::paintBigMoveCell(QPainter *painter,
									 const QStyleOptionViewItem &option,
									 const QModelIndex &index) const
{
	QColor defaultFg = drawBackground(painter, option);
	const bool selected = option.state & QStyle::State_Selected;

	const QVariant raw = index.data(Qt::UserRole);
	const double prob = raw.isValid() ? raw.toDouble() : -1.0;

	QRect contentRect = option.rect.adjusted(CellPadding, 0, -CellPadding, 0);

	if (prob < 0.0)
	{
		painter->setPen(NeutralColor);
		painter->setFont(option.font);
		painter->drawText(contentRect, Qt::AlignCenter, QStringLiteral("-"));
		return;
	}

	QColor accent;
	if (prob >= 0.5)       accent = QColor(0xEF, 0x53, 0x50);
	else if (prob >= 0.35) accent = QColor(0xF5, 0xA6, 0x23);
	else                   accent = NeutralColor;

	const QString text = QString::number(prob * 100.0, 'f', 0) + QStringLiteral("%");

	QFontMetrics fm(option.font);
	int textWidth = fm.horizontalAdvance(text);
	int pillW = textWidth + 14;
	int pillH = qMin(option.rect.height() - 4, 22);
	int pillX = contentRect.center().x() - pillW / 2;
	int pillY = option.rect.center().y() - pillH / 2;
	QRectF pillRect(pillX, pillY, pillW, pillH);

	QColor pillBg = accent;
	pillBg.setAlphaF(0.20);
	painter->setPen(Qt::NoPen);
	painter->setBrush(pillBg);
	painter->drawRoundedRect(pillRect, pillH / 2.0, pillH / 2.0);

	painter->setPen(selected ? defaultFg : accent);
	painter->setFont(option.font);
	painter->drawText(pillRect, Qt::AlignCenter, text);
}
