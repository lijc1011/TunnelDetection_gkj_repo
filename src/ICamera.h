#pragma once

// 相机抽象接口：上层（采集、显示、存储、参数设置）只依赖此接口，
// 具体实现可以是 DALSA(CCamera) 或 海康(CHikCamera)，互不影响。

#include <windows.h> // BOOL
#include <QString>
#include "DataType.h" // pPICTUREIMAGESTRUCT 等

class ICamera
{
public:
    virtual ~ICamera() {}

    // 连接/断开
    virtual bool ConnectCamera() = 0;
    virtual void StopCamera() = 0;
    virtual BOOL DestroyObjects() = 0;

    // 采集（开始/停止存储）
    virtual void StartAcquisition(void) = 0;
    virtual void StopAcquisition(void) = 0;

    // 存储队列
    virtual BOOL IsSave(void) = 0;
    virtual BOOL GetSaveDeqData(pPICTUREIMAGESTRUCT pPic) = 0;

    // 显示队列
    virtual BOOL IsShowII(void) = 0;
    virtual BOOL GetShowDeqDataII(unsigned char *pDataBuffer, int isize) = 0;

    // 参数：曝光/增益/行频
    virtual BOOL SetExpTime(double itime) = 0;
    virtual BOOL GetExpTime(double &itime) = 0;
    virtual BOOL SetCamGain(int iGain) = 0;
    virtual BOOL GetCamGain(double &iGain) = 0;
    virtual BOOL GetCamFreg(double &iFreg) = 0;
    virtual BOOL SetCamFreg(double iFreg) = 0;

    // 队列长度查询
    virtual _int64 get_ImgDeqShow_Size() = 0;
    virtual _int64 get_ImgDeqSave_Size() = 0;
};
