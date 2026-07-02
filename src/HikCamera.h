#pragma once
#include <mutex>
#include <deque>
#include "Globals.h"
#include "DataType.h"
#include "ICamera.h"
#include "QMilThd.h"

// 海康 MVS SDK
#include "MvCameraControl.h"

using namespace std;

// 海康黑白线阵相机封装，接口与 DALSA 的 CCamera 完全一致，
// 上层通过 ICamera* 多态调用，DALSA 通路零改动。
class CHikCamera : public ICamera
{
public:
    CHikCamera(CAMERAINFORSTRUCT camInfor, QMilThd *milDetect);
    ~CHikCamera() override;

    // ICamera 接口实现
    bool ConnectCamera() override;
    void StopCamera() override;
    BOOL DestroyObjects() override;

    void StartAcquisition(void) override;
    void StopAcquisition(void) override;

    BOOL IsSave(void) override;
    BOOL GetSaveDeqData(pPICTUREIMAGESTRUCT pPic) override;

    BOOL IsShowII(void) override;
    BOOL GetShowDeqDataII(unsigned char *pDataBuffer, int isize) override;

    BOOL SetExpTime(double itime) override;
    BOOL GetExpTime(double &itime) override;
    BOOL SetCamGain(int iGain) override;
    BOOL GetCamGain(double &iGain) override;
    BOOL GetCamFreg(double &iFreg) override;
    BOOL SetCamFreg(double iFreg) override;

    _int64 get_ImgDeqShow_Size() override;
    _int64 get_ImgDeqSave_Size() override;

private:
    // 海康图像回调（静态 + 实例分发，和 DALSA 的 XferCallback/CallBackSub 一致）
    static void __stdcall ImageCallBackEx(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser);
    void OnFrame(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo);

    // 根据 Name（海康网口用作 Device User ID / IP / 序列号标识）查找设备
    bool FindDeviceIndex(MV_CC_DEVICE_INFO_LIST &devList, int &matchIndex);
    void ApplyCameraParams();
    void FillFrameTimestamp(MV_FRAME_OUT_INFO_EX *pFrameInfo, FRAMEIMAGESTRUCT &fmst);
    void WriteToShowDeque(unsigned char *pData, __int64 dataLen);
    void WriteToSaveDeque(unsigned char *pData, __int64 dataLen, MV_FRAME_OUT_INFO_EX *pFrameInfo);

    void ClearShowDeg();
    void ClearSaveDeg(int iSize);

private:
    void *m_handle;       // 海康设备句柄
    BOOL m_bGrabbing;     // 是否已开始取流
    BOOL m_bEnable;       // 相机已连接(FALSE只显示) / 已开始存储(TRUE)

    std::mutex m_showMutex;
    std::mutex m_saveMutex;
    deque<FRAMEIMAGESTRUCT> m_ImgDeqShow;
    deque<FRAMEIMAGESTRUCT> m_ImgDeqSave;

    CAMERAINFORSTRUCT m_camInfor;
    QMilThd *cam_milDetect;
};
