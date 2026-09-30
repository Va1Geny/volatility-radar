#pragma once
#include <QObject>
#include <QWebSocket>
#include <QStringList>
#include <QAbstractSocket>
#include <QTimer>

class FinnhubClient: public QObject
{
	Q_OBJECT
public:
	explicit FinnhubClient(const QString & apiToken, QObject * parent = nullptr);

	void connectToServer();
	void subscribe(const QStringList & symbols);

signals:
	void tradeReceived(const QString & symbol, double price);
	void connected();
	void disconnected();
	void errorOccurred(const QString & message);

private slots:
	void onConnected();
	void onDisconnected();
	void onTextMessageReceived(const QString & message);
	void onError(QAbstractSocket::SocketError error);

private:
	void sendSub(const QString & symbol);
	void scheduleReconnect();

	static constexpr int MinReconnectMs = 3000;
	static constexpr int MaxReconnectMs = 60000;

	QWebSocket m_socket;
	QString m_token;
	QStringList m_subscribed;
	QTimer m_reconnectTimer;
	int m_reconnectMs = MinReconnectMs;
};
