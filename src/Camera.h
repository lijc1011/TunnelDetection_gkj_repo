#pragma once
#include <atomic>
#include <mutex>
#include "Globals.h"
#include "DataType.h"
#include "ICamera.h"
#include "QMilThd.h"

using namespace std;
class	CCamera : public ICamera
{
public:
	CCamera(CAMERAINFORSTRUCT camInfor, QMilThd* milDetect);
	~CCamera();
 
 	void SetOffSet(int i0);
	BOOL m_xgflg;
	void ReleaseDeqDuiqi(void);
	
private: 
	
	SapAcqDevice	*m_AcqDevice;
	SapBuffer		*m_Buffers;
	SapTransfer		*m_Xfer;
//////////////////////////////////////////////////////////////////////////
	SapManager       *m_pManger;
	SapAcquisition   *m_pAcquisition;
//	SapAcqDevice     *m_pCamera_m_AcqDevice;
//	SapBuffer        *m_pBuffers;
	SapBuffer		 *m_pBufferExtrack;
//	SapTransfer      *m_Xfer;
	SapFeature       *m_pFeature;

	int m_ServerCount;						///< 采集卡数量
	int m_CameraCount;						///< 相机数量
	char *m_ServerName;					    ///< 采集卡名称
	char *m_CameraName;						///< 相机名称
	BOOL InitCamera_CamLink(char *serverName, int resourceIndex, char *cameraName, QString ccfPath);
	int m_iImageWidth, m_iImageHeight; ///< 图像宽高

	//////////////////////////////////////////////////////////////////////////
	std::mutex m_showMutex;
	std::mutex m_saveMutex;
	deque<FRAMEIMAGESTRUCT> m_ImgDeqShow;
	deque<FRAMEIMAGESTRUCT> m_ImgDeqSave;
	int m_IsFullCnt;
	CAMERAINFORSTRUCT m_camInfor;
	QMilThd* cam_milDetect;
	std::atomic<bool> m_bEnable;
	std::atomic<unsigned long long> m_trashFrameCount;

	
public:
	
 	bool ConnectCamera();
	BOOL IsFullCnt(void);
	void StartAcquisition(void);
	void StopAcquisition(void);
	BOOL IsSave(void);
 
	BOOL IsShowII(void);
	BOOL GetShowDeqDataII(unsigned char* pDataBuffer, int isize); 
	BOOL ClearShowDeg();
	BOOL GetSaveDeqData(pPICTUREIMAGESTRUCT pPic);
    BOOL ClearSaveDeg(int iSize);

	void YaSuoDaoFile(unsigned char* pBits,int w,int h,int q,QString strpath);
	BOOL SetExpTime(double itime);
	BOOL GetExpTime(double &itime);

	BOOL SetCamGain(int iGain);
	BOOL GetCamGain(double &iGain);
	BOOL GetCamFreg(double &iFreg);
	BOOL SetCamFreg(double iFreg);

	BOOL ClearShowAndSaveDeque();

	_int64 get_ImgDeqShow_Size();
	_int64 get_ImgDeqSave_Size();

private:
	static void XferCallback(SapXferCallbackInfo *pInfo);
	void CallBackSub(SapXferCallbackInfo *pInfo);
	BOOL LoadXML(char* lpPath);
	bool NewObjects();
	BOOL CreateObjects();
	void FillFrameTimestamp(SapXferCallbackInfo *pInfo, FRAMEIMAGESTRUCT &fmst);
	
	void WriteToShowDeque(void* pData);
	void WriteToSaveDeque(void* pData, SapXferCallbackInfo *pInfo);

public:
	BOOL DestroyObjects();
	void StopCamera();
#if 0

public:
	
	
	void ReleaseDeqA(int num);
	
	
	
	
	void AbortGrab(void);

//	FRAMEIMAGESTRUCT GetDeqFrontA(int &tmp,int incc);
	void ClearDeqA_All(void);
#endif // 0
};
