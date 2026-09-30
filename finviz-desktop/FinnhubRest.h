#pragma once
#include <QObject>
#include <QStringList>
#include <QQueue>
#include <QHash>
#include <QPair>
#include "StockModel.h"

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class FinnhubRest: public QObject
{
	Q_OBJECT
public:
	explicit FinnhubRest(const QString & apiToken, QObject * parent = nullptr);

	void loadSymbols(const QStringList & symbols);

signals:
	void recordReady(const StockRecord & record);
	void progress(int done, int total);
	void finished();
	void errorOccurred(const QString & message);

private slots:
	void dispatchNext();
	void onReplyFinished(QNetworkReply * reply);

private:
	QNetworkAccessManager * m_nam;
	QTimer * m_timer;
	QString m_token;

	QQueue<QPair<QString, QString>> m_queue;
	QHash<QString, StockRecord> m_partial;
	QHash<QString, int> m_remaining;
	int m_total = 0;
	int m_done = 0;
};
