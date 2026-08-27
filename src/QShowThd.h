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
	double m_currentExposure[MAXCAMNUM];
	double m_currentGain[MAXCAMNUM];
	double m_manualExposureBeforeAuto[MAXCAMNUM];
	double m_manualGainBeforeAuto[MAXCAMNUM];
	double m_autoMin[MAXCAMNUM];
	double m_autoMax[MAXCAMNUM];
	int m_aeCounter[MAXCAMNUM];
	int m_aeInterval[MAXCAMNUM];
	int m_aeFailureCount[MAXCAMNUM];
	int m_aeHoldLogCounter[MAXCAMNUM];
	bool m_autoReady[MAXCAMNUM];
	bool m_autoOverrideActive[MAXCAMNUM];
	bool m_manualStateSaved[MAXCAMNUM];
	bool m_prevAutoEnabled = false;  // 上一周期总开关状态(用于上升沿播种)

	void enterAutoBrightnessMode();
	void leaveAutoBrightnessMode();
	void updateAutoBrightness();
signals:
	void threadover_Camdat_std(QPixmap picQPixmap,int iChlV);

private:
	ICamera *m_cam[MAXCAMNUM];
	CAMERAINFORSTRUCT m_camInfor[MAXCAMNUM];
	unsigned char* pDataBuffer[MAXCAMNUM];
	__int64 pDataBufferSize[MAXCAMNUM];
	QLabel* m_labelShow[MAXCAMNUM];

};


