#include "FinnhubClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

FinnhubClient::FinnhubClient(const QString & apiToken, QObject * parent)
	: QObject(parent)
	, m_token(apiToken)
{
	connect(&m_socket, &QWebSocket::connected,
		this, &FinnhubClient::onConnected);
	connect(&m_socket, &QWebSocket::disconnected,
		this, &FinnhubClient::onDisconnected);
	connect(&m_socket, &QWebSocket::textMessageReceived,
		this, &FinnhubClient::onTextMessageReceived);
	connect(&m_socket, &QWebSocket::errorOccurred,
		this, &FinnhubClient::onError);

	m_reconnectTimer.setSingleShot(true);
	connect(&m_reconnectTimer, &QTimer::timeout, this, [this]
	{
		connectToServer();
	});
}

void FinnhubClient::connectToServer()
{
	if (m_token.isEmpty())
	{
		emit errorOccurred("No Finnhub API token set (FINNHUB_API_KEY).");
		return;
	}

	QUrl url("wss://ws.finnhub.io");
	QUrlQuery query;
	query.addQueryItem("token", m_token);
	url.setQuery(query);

	m_socket.open(url);
}

void FinnhubClient::subscribe(const QString & symbol)
{
	if (symbol.isEmpty()) return;

	if (!m_subscribed.contains(symbol))
		m_subscribed << symbol;

	if (m_connected)
		sendSub("subscribe", symbol);
}

void FinnhubClient::subscribe(const QStringList & symbols)
{
	for (const QString & s : symbols)
		subscribe(s);
}

void FinnhubClient::unsubscribe(const QString & symbol)
{
	m_subscribed.removeAll(symbol);
	if (m_connected)
		sendSub("unsubscribe", symbol);
}

void FinnhubClient::scheduleReconnect()
{
	if (m_token.isEmpty()) return;
	if (!m_reconnectTimer.isActive())
		m_reconnectTimer.start(m_reconnectMs);
}

void FinnhubClient::sendSub(const QString & type, const QString & symbol)
{
	QJsonObject obj;
	obj["type"]   = type;
	obj["symbol"] = symbol;
	m_socket.sendTextMessage(
		QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
}

void FinnhubClient::onConnected()
{
	m_connected = true;
	m_reconnectTimer.stop();
	for (const QString & s : m_subscribed)
		sendSub("subscribe", s);
	emit connected();
}

void FinnhubClient::onDisconnected()
{
	m_connected = false;
	emit disconnected();
	scheduleReconnect();
}

void FinnhubClient::onError(QAbstractSocket::SocketError)
{
	emit errorOccurred(m_socket.errorString());
	scheduleReconnect();
}

void FinnhubClient::onTextMessageReceived(const QString & message)
{
	QJsonParseError err;
	QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject())
		return;

	QJsonObject root = doc.object();
	if (root["type"].toString() != "trade")
		return;

	const QJsonArray data = root["data"].toArray();
	for (const QJsonValue & v : data)
	{
		QJsonObject t = v.toObject();
		const QString sym = t["s"].toString();
		const double price = t["p"].toDouble();
		const double vol   = t["v"].toDouble();
		if (!sym.isEmpty())
			emit tradeReceived(sym, price, vol);
	}
}
