#pragma once

#include <QObject>
#include "Globals.h"
#include "DataType.h"
#include "Camera.h"

using namespace std;
class QSaveThd : public QObject
{
	Q_OBJECT

public:
	QSaveThd(QObject *parent, CCamera *pDel, CAMERAINFORSTRUCT camInfor, int ichl);
	~QSaveThd();

	void startThd();
	void stopThd();
	void pauseThd();
	long long currentTime_S;
	long long currentTime_E;

public:
	std::thread genOrgThreads;
	static void genOrgJpgFun(QSaveThd* pParameter);
	void sub_genOrgJpgFun();
	std::mutex my_lock_org;
	bool m_isBreakThd_org;
	bool m_isCanRun_org;

	void Capture(BYTE* pBits, int w, int h, QString imgName);

	//////���ݼ���////////////////////////////////////////////////////////////////////
	ImageEncryptor *im_encode;

signals:
 	void thread_signal(int iChlV);
	void thread_signal_stopsave(int iChlV);

public:
	void StartSave(QString str_path, QString str_file);
	void StopSave();

	QString m_str_ImgFileName;
	QString m_str_IdxFileName;

private:
	int m_ichl;
	CCamera* m_cam;
	CAMERAINFORSTRUCT m_camInfor;
	pPICTUREIMAGESTRUCT m_pPicture;

	QString m_strAllfile_camData;
	QString m_strAllfile_camIdx;
	FILE* lpFile;
	FILE* lpFile_camData;
	FILE* lpFile_camIdx;

	void OpenCamDataSaveFile(QString str_path_file);
	void OpenCamIdxSaveFile(QString str_path_file);
	void CloseSaveFile();
	void ReNameDatFile(void);
	void ReNameIdxFile(void);

	__int64 m_iPicCnt;

public:


	void StopSaveImg(void);
	BOOL SaveFrameData(pPICTUREIMAGESTRUCT pPicImg, __int64 isn);



private:
	
};


