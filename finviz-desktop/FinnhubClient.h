#pragma once
#include <QObject>
#include <QWebSocket>
#include <QStringList>
#include <QAbstractSocket>

class FinnhubClient: public QObject
{
	Q_OBJECT
public:
	explicit FinnhubClient(const QString & apiToken, QObject * parent = nullptr);

	bool hasToken() const { return !m_token.isEmpty(); }

	void connectToServer();
	void subscribe(const QString & symbol);
	void subscribe(const QStringList & symbols);
	void unsubscribe(const QString & symbol);

signals:
	void tradeReceived(const QString & symbol, double price, double volume);
	void connected();
	void disconnected();
	void errorOccurred(const QString & message);

private slots:
	void onConnected();
	void onDisconnected();
	void onTextMessageReceived(const QString & message);
	void onError(QAbstractSocket::SocketError error);

private:
	void sendSub(const QString & type, const QString & symbol);

	QWebSocket m_socket;
	QString m_token;
	QStringList m_pending;
	bool m_connected = false;
};
