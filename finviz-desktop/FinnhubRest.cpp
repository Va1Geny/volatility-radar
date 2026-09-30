#include "FinnhubRest.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

// Free tier allows 60 calls/minute; one call per ~1s stays under it.
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
	m_queue.clear();
	m_partial.clear();
	m_remaining.clear();
	m_done = 0;

	for (const QString & raw : symbols)
	{
		const QString sym = raw.trimmed().toUpper();
		if (sym.isEmpty() || m_partial.contains(sym)) continue;

		StockRecord r;
		r.symbol = sym;
		m_partial.insert(sym, r);
		m_remaining.insert(sym, 2);
		m_queue.enqueue({ sym, "quote" });
		m_queue.enqueue({ sym, "profile" });
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
	url.setQuery(query);

	// Token goes in a header, not the URL: Qt puts the full URL into error strings.
	QNetworkRequest req(url);
	req.setRawHeader("X-Finnhub-Token", m_token.toUtf8());
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

	// Rate limited: put the job back in line instead of losing the stock.
	// ponytail: retries forever at the tick rate; add a retry cap if Finnhub ever 429s persistently.
	if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 429)
	{
		m_queue.enqueue({ symbol, kind });
		if (!m_timer->isActive()) m_timer->start();
		return;
	}

	const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
	if (reply->error() != QNetworkReply::NoError)
	{
		emit errorOccurred(QString("%1 %2: %3").arg(symbol, kind, reply->errorString()));
	}
	else if (obj.contains("error"))
	{
		emit errorOccurred(QString("%1 %2: %3").arg(symbol, kind, obj["error"].toString()));
	}
	else if (kind == "quote")
	{
		StockRecord & r = m_partial[symbol];
		r.lastSale = obj["c"].toDouble();
		r.netChange = obj["d"].toDouble();
		r.pctChange = obj["dp"].toDouble();
		r.previousClose = obj["pc"].toDouble();
	}
	else
	{
		// profile2 has one coarse classification (e.g. "Technology"); it goes in Sector.
		StockRecord & r = m_partial[symbol];
		r.name = obj["name"].toString();
		r.country = obj["country"].toString();
		r.sector = obj["finnhubIndustry"].toString();
		// Reported in the home currency (TSM in TWD, TM in JPY). No free FX rates: show "-" rather than a wrong number.
		if (obj["currency"].toString() == "USD")
			r.marketCap = obj["marketCapitalization"].toDouble() * 1e6;
		const QString ipo = obj["ipo"].toString();
		if (ipo.size() >= 4) r.ipoyear = ipo.left(4);
	}

	if (--m_remaining[symbol] > 0)
		return;

	StockRecord r = m_partial.take(symbol);
	m_remaining.remove(symbol);
	// No price means the quote failed or Finnhub doesn't know the symbol: skip it, don't show $0.00.
	if (r.lastSale > 0.0)
	{
		if (r.name.isEmpty()) r.name = symbol;
		emit recordReady(r);
	}
	else
	{
		emit errorOccurred(QString("%1: no quote, skipped").arg(symbol));
	}
	emit progress(++m_done, m_total);
	if (m_done >= m_total)
		emit finished();
}
