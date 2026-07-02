#pragma once

#include <QObject>
#include "Globals.h"
#include "Camera.h"
#include "AutoExposureController.h"

class QShowThd : public QObject
{
	Q_OBJECT

public:
	QShowThd(QObject *parent);
	~QShowThd();

	void setCamera(ICamera *pDel[], CAMERAINFORSTRUCT camInfor[]);
	void setCamera(QLabel* plabel[]);

	void startThd();
	void stopThd();
	void readThd(__int64 readPos = 0, __int64 readH = 0);
	void pauseThd();
	long long currentTime_S;
	long long currentTime_E;

public:
	std::thread genOrgThreads;
	static void genOrgJpgFun(QShowThd* pParameter);
	void sub_genOrgJpgFun();
	std::mutex my_lock_org;
	bool m_isBreakThd_org;
	bool m_isCanRun_org;

	void Capture(BYTE* pBits, int w, int h, QString imgName);

	AutoExposureController m_aec[MAXCAMNUM]; // 每台相机一个控制器(状态独立)
	float m_currentExposure[MAXCAMNUM];
	float m_currentGain[MAXCAMNUM];
	int m_aeCounter = 0;             // AE 降频计数
	int m_aeInterval = 3;            // 每 N 幅拼好图执行一次 AE
	bool m_prevAutoEnabled = false;  // 上一周期总开关状态(用于上升沿播种)
signals:
	void threadover_Camdat_std(QPixmap picQPixmap,int iChlV);

private:
	ICamera *m_cam[MAXCAMNUM];
	CAMERAINFORSTRUCT m_camInfor[MAXCAMNUM];
	unsigned char* pDataBuffer[MAXCAMNUM];
	__int64 pDataBufferSize[MAXCAMNUM];
	QLabel* m_labelShow[MAXCAMNUM];

};


