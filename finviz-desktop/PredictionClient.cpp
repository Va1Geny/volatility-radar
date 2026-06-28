#include "PredictionClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

PredictionClient::PredictionClient(const QString & url, QObject * parent)
	: QObject(parent)
	, m_url(url)
{
	connect(&m_socket, &QWebSocket::connected,
		this, &PredictionClient::onConnected);
	connect(&m_socket, &QWebSocket::disconnected,
		this, &PredictionClient::onDisconnected);
	connect(&m_socket, &QWebSocket::textMessageReceived,
		this, &PredictionClient::onTextMessageReceived);
	connect(&m_socket, &QWebSocket::errorOccurred,
		this, &PredictionClient::onError);

	m_reconnectTimer.setSingleShot(true);
	connect(&m_reconnectTimer, &QTimer::timeout, this, [this]
	{
		m_socket.open(QUrl(m_url));
	});
}

void PredictionClient::start()
{
	m_socket.open(QUrl(m_url));
}

void PredictionClient::onConnected()
{
	m_reconnectTimer.stop();
	emit connected();
}

void PredictionClient::onDisconnected()
{
	emit disconnected();
	scheduleReconnect();
}

void PredictionClient::onError(QAbstractSocket::SocketError)
{
	scheduleReconnect();
}

void PredictionClient::scheduleReconnect()
{
	if (!m_reconnectTimer.isActive())
		m_reconnectTimer.start(m_reconnectMs);
}

void PredictionClient::onTextMessageReceived(const QString & message)
{
	QJsonParseError err;
	QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject())
		return;

	QJsonObject obj = doc.object();
	const QString symbol = obj["ticker"].toString();
	if (symbol.isEmpty() || !obj.contains("prob_bigmove"))
		return;

	emit predictionReceived(symbol, obj["prob_bigmove"].toDouble());
}
