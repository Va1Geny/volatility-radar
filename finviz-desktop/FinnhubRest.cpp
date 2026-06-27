#include "FinnhubRest.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

static constexpr int kTickMs = 1050;

FinnhubRest::FinnhubRest(const QString & apiToken, QObject * parent)
	: QObject(parent)
	, m_nam(new QNetworkAccessManager(this))
	, m_timer(new QTimer(this))
	, m_token(apiToken)
{
	m_timer->setInterval(kTickMs);
	connect(m_timer, &QTimer::timeout, this, &FinnhubRest::dispatchNext);
	connect(m_nam, &QNetworkAccessManager::finished,
		this, &FinnhubRest::onReplyFinished);
}

void FinnhubRest::loadSymbols(const QStringList & symbols)
{
	if (m_token.isEmpty())
	{
		emit errorOccurred("No Finnhub API token for REST load.");
		return;
	}

	m_queue.clear();
	m_partial.clear();
	m_remaining.clear();
	m_done = 0;

	for (const QString & raw : symbols)
	{
		const QString sym = raw.trimmed().toUpper();
		if (sym.isEmpty()) continue;

		StockRecord r;
		r.symbol = sym;
		m_partial.insert(sym, r);
		m_remaining.insert(sym, 2);
		enqueue(sym, "quote");
		enqueue(sym, "profile");
	}

	m_total = m_partial.size();
	if (m_total == 0)
	{
		emit finished();
		return;
	}

	dispatchNext();
	m_timer->start();
}

void FinnhubRest::enqueue(const QString & symbol, const QString & kind)
{
	m_queue.enqueue(qMakePair(symbol, kind));
}

void FinnhubRest::dispatchNext()
{
	if (m_queue.isEmpty())
	{
		m_timer->stop();
		return;
	}

	const QPair<QString, QString> job = m_queue.dequeue();
	const QString & symbol = job.first;
	const QString & kind = job.second;

	QUrl url(kind == "quote"
		? "https://finnhub.io/api/v1/quote"
		: "https://finnhub.io/api/v1/stock/profile2");
	QUrlQuery query;
	query.addQueryItem("symbol", symbol);
	query.addQueryItem("token", m_token);
	url.setQuery(query);

	QNetworkRequest req(url);
	QNetworkReply * reply = m_nam->get(req);
	reply->setProperty("symbol", symbol);
	reply->setProperty("kind", kind);
}

void FinnhubRest::onReplyFinished(QNetworkReply * reply)
{
	reply->deleteLater();

	const QString symbol = reply->property("symbol").toString();
	const QString kind = reply->property("kind").toString();
	if (symbol.isEmpty() || !m_partial.contains(symbol))
		return;

	if (reply->error() == QNetworkReply::NoError)
	{
		const QJsonObject obj =
			QJsonDocument::fromJson(reply->readAll()).object();
		StockRecord & r = m_partial[symbol];

		if (kind == "quote")
		{
			r.lastSale = obj["c"].toDouble();
			r.netChange = obj["d"].toDouble();
			r.pctChange = obj["dp"].toDouble();
			r.previousClose = obj["pc"].toDouble();
		}
		else
		{
			r.name = obj["name"].toString();
			r.country = obj["country"].toString();
			r.sector = obj["finnhubIndustry"].toString();
			r.industry = obj["finnhubIndustry"].toString();
			r.marketCap = obj["marketCapitalization"].toDouble() * 1e6;
			const QString ipo = obj["ipo"].toString();
			if (ipo.size() >= 4) r.ipoyear = ipo.left(4);
		}
	}
	else
	{
		emit errorOccurred(QString("%1 %2: %3")
			.arg(symbol, kind, reply->errorString()));
	}

	if (--m_remaining[symbol] <= 0)
	{
		StockRecord r = m_partial.take(symbol);
		m_remaining.remove(symbol);
		if (r.name.isEmpty()) r.name = symbol;
		emit recordReady(r);
		emit progress(++m_done, m_total);
		if (m_done >= m_total)
			emit finished();
	}
}
