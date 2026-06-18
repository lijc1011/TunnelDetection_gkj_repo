//#include "stdafx.h"
#include "QInitThd.h"

QInitThd::QInitThd(QObject *parent)
	: QObject(parent)
{

}

QInitThd::~QInitThd()
{

}
void QInitThd::startThd()
{
	m_isBreakThd_org = true;
	m_isCanRun_org = false;
	std::thread *pt1(new std::thread(genOrgJpgFun, this));
}
void QInitThd::stopThd()
{
	m_isBreakThd_org = false;
	m_isCanRun_org = false;
}
void QInitThd::readThd(__int64 readPos, __int64 readH)
{
	currentTime_S = QDateTime::currentDateTime().toMSecsSinceEpoch();
	m_isCanRun_org = true;
}
void QInitThd::pauseThd()
{
	m_isCanRun_org = false;
}
void QInitThd::genOrgJpgFun(QInitThd* pParameter)
{
	pParameter->sub_genOrgJpgFun();
}
void QInitThd::sub_genOrgJpgFun()
{
	while (m_isBreakThd_org)
	{
		std::lock_guard<std::mutex> lockGuard(my_lock_org);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		if (m_isCanRun_org)
		{
			


			currentTime_E = QDateTime::currentDateTime().toMSecsSinceEpoch();
			long long abc1 = abs(currentTime_E - currentTime_S);
 		//	emit threadover_Camdat_std(picQPixmap, m_jpgCnt, iChannelNum);
		
			m_isCanRun_org = false;

		}
	}
}