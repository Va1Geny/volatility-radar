#include "FinnhubClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>

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
	connect(&m_reconnectTimer, &QTimer::timeout,
		this, &FinnhubClient::connectToServer);
}

void FinnhubClient::connectToServer()
{
	// Finnhub's websocket only accepts the token as a query parameter.
	QUrl url("wss://ws.finnhub.io");
	QUrlQuery query;
	query.addQueryItem("token", m_token);
	url.setQuery(query);

	m_socket.open(url);
}

void FinnhubClient::subscribe(const QStringList & symbols)
{
	for (const QString & s : symbols)
	{
		if (s.isEmpty() || m_subscribed.contains(s)) continue;
		m_subscribed << s;
		if (m_socket.state() == QAbstractSocket::ConnectedState)
			sendSub(s);
	}
}

// Exponential backoff so a bad token or an outage doesn't hammer the server.
void FinnhubClient::scheduleReconnect()
{
	if (m_reconnectTimer.isActive()) return;
	m_reconnectTimer.start(m_reconnectMs);
	m_reconnectMs = qMin(m_reconnectMs * 2, MaxReconnectMs);
}

void FinnhubClient::sendSub(const QString & symbol)
{
	QJsonObject obj;
	obj["type"]   = "subscribe";
	obj["symbol"] = symbol;
	m_socket.sendTextMessage(
		QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
}

void FinnhubClient::onConnected()
{
	m_reconnectTimer.stop();
	m_reconnectMs = MinReconnectMs;
	for (const QString & s : m_subscribed)
		sendSub(s);
	emit connected();
}

void FinnhubClient::onDisconnected()
{
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
	const QString type = root["type"].toString();
	if (type == "error")
		emit errorOccurred(root["msg"].toString());
	if (type != "trade")
		return;

	const QJsonArray data = root["data"].toArray();
	for (const QJsonValue & v : data)
	{
		QJsonObject t = v.toObject();
		const QString sym = t["s"].toString();
		const double price = t["p"].toDouble();
		if (!sym.isEmpty() && price > 0.0)
			emit tradeReceived(sym, price);
	}
}
