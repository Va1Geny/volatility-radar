// Unit tests for the table model and filter proxy. Run: ctest --test-dir <build dir>
#include <QtTest>
#include <QTemporaryFile>
#include "StockModel.h"
#include "StockFilterProxy.h"

class TestStockModel: public QObject
{
	Q_OBJECT

	static StockRecord record(const QString & symbol, double price, double previousClose = 0.0)
	{
		StockRecord r;
		r.symbol = symbol;
		r.name = symbol + " Corp";
		r.lastSale = price;
		r.previousClose = previousClose;
		return r;
	}

	static QString symbolAt(const QAbstractItemModel & m, int row)
	{
		return m.index(row, StockModel::ColSymbol).data().toString();
	}

private slots:
	void predictionArrivingBeforeRowIsKept()
	{
		StockModel m;
		m.setPrediction("AAPL", 0.3);
		m.addRecord(record("AAPL", 100));
		const QModelIndex cell = m.index(0, StockModel::ColBigMove);
		QCOMPARE(cell.data(Qt::UserRole).toDouble(), 0.3);
		QCOMPARE(cell.data().toString(), QString("30%"));
		QVERIFY(cell.data(Qt::ToolTipRole).toString().startsWith("30% chance"));
	}

	void missingPredictionShowsDashAndNoTooltip()
	{
		StockModel m;
		m.addRecord(record("MSFT", 100));
		const QModelIndex cell = m.index(0, StockModel::ColBigMove);
		QCOMPARE(cell.data().toString(), QString("-"));
		QVERIFY(cell.data(Qt::ToolTipRole).isNull());
		QVERIFY(!m.headerData(StockModel::ColBigMove, Qt::Horizontal, Qt::ToolTipRole).isNull());
	}

	void livePriceRecomputesChangeFromPreviousClose()
	{
		StockModel m;
		m.addRecord(record("NVDA", 100, 100));
		m.updateLivePrice("NVDA", 110);
		QCOMPARE(m.recordAt(0).lastSale, 110.0);
		QCOMPARE(m.recordAt(0).netChange, 10.0);
		QCOMPARE(m.recordAt(0).pctChange, 10.0);
		m.updateLivePrice("UNKNOWN", 1);   // ignored, no crash
	}

	void numericColumnsSortByValueNotText()
	{
		StockModel m;
		m.addRecord(record("AAA", 9));
		m.addRecord(record("BBB", 45));
		m.addRecord(record("CCC", 100));
		StockFilterProxy p;
		p.setSourceModel(&m);

		// As text "$100.00" < "$45.00" < "$9.00"; by value 9 < 45 < 100.
		p.sort(StockModel::ColLastSale, Qt::AscendingOrder);
		QCOMPARE(symbolAt(p, 0), QString("AAA"));
		QCOMPARE(symbolAt(p, 2), QString("CCC"));

		// As text "9%" > "45%"; by value 45% is the bigger risk.
		m.setPrediction("AAA", 0.09);
		m.setPrediction("BBB", 0.45);
		p.sort(StockModel::ColBigMove, Qt::DescendingOrder);
		QCOMPARE(symbolAt(p, 0), QString("BBB"));
	}

	void filtersBySearchTextAndPrice()
	{
		StockModel m;
		m.addRecord(record("AAPL", 200));
		m.addRecord(record("AMD", 50));
		m.addRecord(record("KO", 60));
		StockFilterProxy p;
		p.setSourceModel(&m);

		p.setTextFilter("a");      // case-insensitive, matches AAPL and AMD
		QCOMPARE(p.rowCount(), 2);
		p.setMinPrice(100);
		QCOMPARE(p.rowCount(), 1);
		QCOMPARE(symbolAt(p, 0), QString("AAPL"));
		p.resetFilters();
		QCOMPARE(p.rowCount(), 3);
	}

	void loadsNasdaqSnapshotJson()
	{
		QTemporaryFile file;
		QVERIFY(file.open());
		file.write(R"({"data":{"rows":[
			{"symbol":"A","name":"Agilent","lastsale":"$115.08","netchange":"0.12","pctchange":"0.104%",
			 "volume":"2839162","marketCap":"32521874640.00","country":"United States","ipoyear":"1999",
			 "industry":"Lab Instruments","sector":"Industrials"},
			{"symbol":"A","name":"duplicate row"},
			{"symbol":"","name":"no symbol"}]}})");
		file.close();

		StockModel m;
		m.loadFromJson(file.fileName());
		QCOMPARE(m.rowCount(), 1);
		QCOMPARE(m.recordAt(0).lastSale, 115.08);
		QCOMPARE(m.recordAt(0).pctChange, 0.104);
		QCOMPARE(m.recordAt(0).previousClose, 115.08 - 0.12);
		QCOMPARE(m.index(0, StockModel::ColMarketCap).data().toString(), QString("$32.52B"));
		QCOMPARE(m.uniqueSectors(), QStringList { "Industrials" });
	}
};

QTEST_GUILESS_MAIN(TestStockModel)
#include "test_stockmodel.moc"
