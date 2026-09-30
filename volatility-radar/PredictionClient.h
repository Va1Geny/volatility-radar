#pragma once
#include <QObject>
#include <QWebSocket>
#include <QAbstractSocket>
#include <QTimer>

class PredictionClient: public QObject
{
	Q_OBJECT
public:
	explicit PredictionClient(const QString & url, QObject * parent = nullptr);

	void start();

signals:
	void predictionReceived(const QString & symbol, double bigMoveProb);
	void connected();
	void disconnected();

private slots:
	void onConnected();
	void onDisconnected();
	void onTextMessageReceived(const QString & message);
	void onError(QAbstractSocket::SocketError error);

private:
	void scheduleReconnect();

	static constexpr int MinReconnectMs = 5000;
	static constexpr int MaxReconnectMs = 30000;

	QWebSocket m_socket;
	QString m_url;
	QTimer m_reconnectTimer;
	int m_reconnectMs = MinReconnectMs;
};
