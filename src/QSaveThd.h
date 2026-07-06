#pragma once

#include <QObject>
#include "Globals.h"
#include "DataType.h"
#include "Camera.h"
#include "tszarr/TsZarrBridge.h"

using namespace std;
class QSaveThd : public QObject
{
	Q_OBJECT

public:
	QSaveThd(QObject *parent, ICamera *pDel, CAMERAINFORSTRUCT camInfor, int ichl);
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

	//////数据加密////////////////////////////////////////////////////////////////////
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
	ICamera* m_cam;
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

	// 密钥沿用现约定：文件名前两个'-'分段 "{时间}-{相机名}"，与地面端解密一致
	void CreateEncryptorFromFileName(const QString &strAllFile);

	__int64 m_iPicCnt;

	//////Zarr v3 (TensorStore) 存储////////////////////////////////////////////////////
	// m_zarrMode=true 时不再写 .camdatx，改为异步写 zarr 目录；camidx 照写不变。
	bool m_zarrMode;
	void* m_tsWriter;
	BYTE* m_pTransformBuf;       // 旋转/翻转输出缓冲（变换开启时才分配）
	__int64 m_iPacketBytes;      // 一包字节数 = MAXFRAME * iSizeFrame
	int m_matRows, m_matCols;    // 变换前的包矩阵尺寸（像素）
	int m_matType;               // CV_8UC1 / CV_8UC3
	__int64 m_zarrRows, m_zarrCols; // 变换后 zarr 数组的 y/x 尺寸（x 为字节数）
	QString m_strAllfile_zarrTmp;   // 写入中的目录 xxx.zarr.tmp
	QString m_strAllfile_zarrFinal; // 收口后重命名为 xxx.zarr

	void OpenZarrSaveStore(QString str_path_file);
	void CloseZarrStore();
	// 按 SaveTransform 变换并原地 LUT 加密，返回待写入缓冲区指针
	BYTE* TransformAndEncrypt(BYTE* pSrc);

public:


	void StopSaveImg(void);
	BOOL SaveFrameData(pPICTUREIMAGESTRUCT pPicImg, __int64 isn);



private:

};

