#ifndef QINITTHD_H
#define QINITTHD_H

#include <QObject>
#include "Globals.h"

class QInitThd : public QObject
{
	Q_OBJECT

public:
	QInitThd(QObject *parent);
	~QInitThd();

	void startThd();
	void stopThd();
	void readThd(__int64 readPos = 0, __int64 readH = 0);
 	void pauseThd();
	long long currentTime_S;
	long long currentTime_E;

public:
	std::thread genOrgThreads;
	static void genOrgJpgFun(QInitThd* pParameter);
	void sub_genOrgJpgFun();
	std::mutex my_lock_org;
	bool m_isBreakThd_org;
	bool m_isCanRun_org;

	
signals:
	void threadover_Camdat_std(QPixmap picQPixmap, int iSecV, int iChlV);

private:
	
};

#endif // QINITTHD_H
