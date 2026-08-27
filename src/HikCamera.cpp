#include "HikCamera.h"
#include <chrono>

// 海康黑白线阵相机封装。设计目标：对外接口与 DALSA 的 CCamera 完全一致，
// 上层显示/存储/参数线程通过 ICamera* 多态调用，无需区分相机品牌。
//
// 设备匹配：海康是网口相机，沿用 DALSA 网口约定，用 CAMERAINFORSTRUCT.Name 匹配，
// Name 需与 MVS 的 Device User ID(用户自定义名) / 固定IP / 序列号 之一一致；为空则取第一台。

namespace
{
    // 将海康设备信息里的 nCurrentIp(主机字节序) 转为点分十进制字符串
    QString IpToString(unsigned int ip)
    {
        return QString("%1.%2.%3.%4")
            .arg((ip >> 24) & 0xff)
            .arg((ip >> 16) & 0xff)
            .arg((ip >> 8) & 0xff)
            .arg(ip & 0xff);
    }
}

CHikCamera::CHikCamera(CAMERAINFORSTRUCT camInfor, QMilThd *milDetect)
{
    m_camInfor = camInfor;
    cam_milDetect = milDetect;
    m_handle = NULL;
    m_bGrabbing = FALSE;
    m_bEnable = FALSE;
}

CHikCamera::~CHikCamera()
{
    StopCamera();
    DestroyObjects();
}

bool CHikCamera::FindDeviceIndex(MV_CC_DEVICE_INFO_LIST &devList, int &matchIndex)
{
    matchIndex = -1;
    if (devList.nDeviceNum == 0)
        return false;

    // 海康是网口相机，沿用 DALSA 网口的约定：靠 Name 匹配(对应 MVS 的 Device User ID)。
    // 多台相机时若 Name 都留空，会全部命中同一台设备(索引0)，第二台独占打开失败，故必须填。
    QString wanted = m_camInfor.Name.trimmed();

    // 无标识：只有单台场景才退化为取第一台
    if (wanted.isEmpty())
    {
        matchIndex = 0;
        return true;
    }

    for (unsigned int i = 0; i < devList.nDeviceNum; i++)
    {
        MV_CC_DEVICE_INFO *pDev = devList.pDeviceInfo[i];
        if (pDev == NULL)
            continue;

        if (pDev->nTLayerType == MV_GIGE_DEVICE)
        {
            MV_GIGE_DEVICE_INFO &g = pDev->SpecialInfo.stGigEInfo;
            QString ip = IpToString(g.nCurrentIp);
            QString userName = QString::fromLocal8Bit(reinterpret_cast<const char *>(g.chUserDefinedName));
            QString serial = QString::fromLocal8Bit(reinterpret_cast<const char *>(g.chSerialNumber));

            if (ip == wanted || userName == wanted || serial == wanted)
            {
                matchIndex = static_cast<int>(i);
                return true;
            }
        }
        else if (pDev->nTLayerType == MV_USB_DEVICE)
        {
            MV_USB3_DEVICE_INFO &u = pDev->SpecialInfo.stUsb3VInfo;
            QString userName = QString::fromLocal8Bit(reinterpret_cast<const char *>(u.chUserDefinedName));
            QString serial = QString::fromLocal8Bit(reinterpret_cast<const char *>(u.chSerialNumber));
            if (userName == wanted || serial == wanted)
            {
                matchIndex = static_cast<int>(i);
                return true;
            }
        }
    }
    return false;
}

void CHikCamera::ApplyCameraParams()
{
    if (m_handle == NULL)
        return;

    // 黑白像素格式（best-effort，相机本身一般已是 Mono8）
    MV_CC_SetEnumValueByString(m_handle, "PixelFormat", "Mono8");

    // 触发模式：1=外触发(编码器)，0=内触发
    if (m_camInfor.TriggerMode == 1)
    {
    //    MV_CC_SetEnumValue(m_handle, "TriggerMode", 1); // On
        // 行触发，触发源走编码器；不同型号节点名可能不同，best-effort
        //MV_CC_SetEnumValueByString(m_handle, "TriggerSelector", "LineStart");
       // MV_CC_SetEnumValueByString(m_handle, "TriggerSource", "EncoderModuleOut");
    }
    else
    {
        MV_CC_SetEnumValue(m_handle, "TriggerMode", 0); // Off，内部行频
        if (m_camInfor.LineFreq > 0)
        {
            MV_CC_SetBoolValue(m_handle, "AcquisitionLineRateEnable", true);
            // 行频部分型号是 Int，部分是 Float，两者都尝试
            if (MV_CC_SetIntValueEx(m_handle, "AcquisitionLineRate", (int64_t)m_camInfor.LineFreq) != MV_OK)
                MV_CC_SetFloatValue(m_handle, "AcquisitionLineRate", (float)m_camInfor.LineFreq);
        }
    }

    // 曝光、增益
    if (m_camInfor.ExposureTime > 0)
        MV_CC_SetFloatValue(m_handle, "ExposureTime", (float)m_camInfor.ExposureTime);
    if (m_camInfor.Gain > 0)
        MV_CC_SetFloatValue(m_handle, "Gain", (float)m_camInfor.Gain);

    // GigE 网口：探测并设置最佳包大小，降低丢包
    int nPacketSize = MV_CC_GetOptimalPacketSize(m_handle);
    if (nPacketSize > 0)
        MV_CC_SetIntValueEx(m_handle, "GevSCPSPacketSize", (int64_t)nPacketSize);
}

bool CHikCamera::ConnectCamera()
{
    // 枚举网口 + USB 设备
    MV_CC_DEVICE_INFO_LIST devList;
    memset(&devList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &devList);
    if (nRet != MV_OK || devList.nDeviceNum == 0)
        return false;

    int matchIndex = -1;
    if (!FindDeviceIndex(devList, matchIndex) || matchIndex < 0)
        return false;

    nRet = MV_CC_CreateHandle(&m_handle, devList.pDeviceInfo[matchIndex]);
    if (nRet != MV_OK || m_handle == NULL)
        return false;

    nRet = MV_CC_OpenDevice(m_handle, MV_ACCESS_Exclusive, 0);
    if (nRet != MV_OK)
    {
        MV_CC_DestroyHandle(m_handle);
        m_handle = NULL;
        return false;
    }

    ApplyCameraParams();

    // 注册图像回调（等价 DALSA 的 XferCallback）
    nRet = MV_CC_RegisterImageCallBackEx(m_handle, ImageCallBackEx, this);
    if (nRet != MV_OK)
    {
        MV_CC_CloseDevice(m_handle);
        MV_CC_DestroyHandle(m_handle);
        m_handle = NULL;
        return false;
    }

    nRet = MV_CC_StartGrabbing(m_handle);
    if (nRet != MV_OK)
    {
        MV_CC_CloseDevice(m_handle);
        MV_CC_DestroyHandle(m_handle);
        m_handle = NULL;
        return false;
    }

    m_bGrabbing = TRUE;
    return true;
}

void CHikCamera::ImageCallBackEx(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser)
{
    if (pUser == NULL)
        return;
    CHikCamera *pThis = reinterpret_cast<CHikCamera *>(pUser);
    pThis->OnFrame(pData, pFrameInfo);
}

void CHikCamera::OnFrame(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo)
{
    if (pData == NULL || pFrameInfo == NULL)
        return;

    __int64 dataLen = (__int64)pFrameInfo->nFrameLen;

    if (!m_bEnable) // 相机已连接，未开始存储
    {
        WriteToShowDeque(pData, dataLen);
    }
    else // 开始存储
    {
        WriteToSaveDeque(pData, dataLen, pFrameInfo);
        WriteToShowDeque(pData, dataLen);
    }
}

void CHikCamera::FillFrameTimestamp(MV_FRAME_OUT_INFO_EX *pFrameInfo, FRAMEIMAGESTRUCT &fmst)
{
    if (pFrameInfo == NULL)
        return;

    // 设备时间戳：高32 + 低32 拼成 64 位（相机内部时钟，用于多通道对齐）
    unsigned __int64 deviceTimestamp =
        ((unsigned __int64)pFrameInfo->nDevTimeStampHigh << 32) |
        (unsigned __int64)pFrameInfo->nDevTimeStampLow;
    fmst.deviceTimestamp = deviceTimestamp;
    fmst.timestampFlags |= FRAME_TIMESTAMP_DEVICE_VALID;

    // 主机时间戳（SDK 收到帧时的主机时刻）
    fmst.hostTimestamp = (unsigned __int64)pFrameInfo->nHostTimeStamp;
    fmst.timestampFlags |= FRAME_TIMESTAMP_HOST_VALID;
}

void CHikCamera::WriteToShowDeque(unsigned char *pData, __int64 dataLen)
{
    FRAMEIMAGESTRUCT fmst;
    fmst.iHeight = m_camInfor.ImageHeight;
    fmst.iWeight = m_camInfor.ImageWidth;
    fmst.iSizeFrame = m_camInfor.iSizeFrame;
    fmst.pDataPoint = new unsigned char[m_camInfor.iSizeFrame];
    fmst.isShow = false;

    __int64 copyLen = dataLen < m_camInfor.iSizeFrame ? dataLen : m_camInfor.iSizeFrame;
    if (copyLen < m_camInfor.iSizeFrame)
        memset(fmst.pDataPoint, 0, m_camInfor.iSizeFrame);
    memcpy(fmst.pDataPoint, pData, copyLen);

    {
        std::lock_guard<std::mutex> lock(m_showMutex);
        m_ImgDeqShow.push_back(fmst);
    }
}

void CHikCamera::WriteToSaveDeque(unsigned char *pData, __int64 dataLen, MV_FRAME_OUT_INFO_EX *pFrameInfo)
{
    FRAMEIMAGESTRUCT fmst;
    fmst.iHeight = m_camInfor.ImageHeight;
    fmst.iWeight = m_camInfor.ImageWidth;
    fmst.iSizeFrame = m_camInfor.iSizeFrame;
    fmst.pDataPoint = new unsigned char[m_camInfor.iSizeFrame];
    fmst.mil = cam_milDetect->GetMileageInfor();

    // 1.系统时间戳(ms)
    auto now1 = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now1.time_since_epoch()).count();
    fmst.timems = timestamp;
    // 设备/主机时间戳
    FillFrameTimestamp(pFrameInfo, fmst);

    fmst.dHFreqPrecision = -2;             // 保存线程每取出一包后统一读取一次行频
    fmst.pulseCount = fmst.mil.iPulseCount; // 3.脉冲数
    fmst.gykMil = m_dmil_GYK;              // 4.gyk里程

    __int64 copyLen = dataLen < m_camInfor.iSizeFrame ? dataLen : m_camInfor.iSizeFrame;
    if (copyLen < m_camInfor.iSizeFrame)
        memset(fmst.pDataPoint, 0, m_camInfor.iSizeFrame);
    memcpy(fmst.pDataPoint, pData, copyLen);

    {
        std::lock_guard<std::mutex> lock(m_saveMutex);
        m_ImgDeqSave.push_back(fmst);
    }
}

BOOL CHikCamera::IsSave(void)
{
    std::lock_guard<std::mutex> lock(m_saveMutex);
    return m_ImgDeqSave.size() >= MAXFRAME;
}

BOOL CHikCamera::IsShowII(void)
{
    std::lock_guard<std::mutex> lock(m_showMutex);
    return m_ImgDeqShow.size() >= MAXFRAME;
}

void CHikCamera::StartAcquisition(void)
{
    m_bEnable = TRUE;
}

void CHikCamera::StopAcquisition(void)
{
    m_bEnable = FALSE;
}

void CHikCamera::StopCamera()
{
    if (m_handle != NULL && m_bGrabbing)
    {
        MV_CC_StopGrabbing(m_handle);
        m_bGrabbing = FALSE;
    }
}

BOOL CHikCamera::GetShowDeqDataII(unsigned char *pDataBuffer, int isize)
{
    {
        std::lock_guard<std::mutex> lock(m_showMutex);
        for (int j = 0; j < isize; j++)
        {
            FRAMEIMAGESTRUCT pft = m_ImgDeqShow.at(j);
            int dst = m_camInfor.iSizeFrame * j;
            memcpy(pDataBuffer + dst, pft.pDataPoint, m_camInfor.iSizeFrame);
        }
    }
    ClearShowDeg();
    return TRUE;
}

void CHikCamera::ClearShowDeg()
{
    std::lock_guard<std::mutex> lock(m_showMutex);
    int isize = m_ImgDeqShow.size();
    for (int j = 0; j < isize; j++)
    {
        if (m_ImgDeqShow.size() > 0)
        {
            FRAMEIMAGESTRUCT jp_tmp = m_ImgDeqShow.front();
            delete[] jp_tmp.pDataPoint;
            jp_tmp.pDataPoint = NULL;
            m_ImgDeqShow.pop_front();
        }
    }
}

BOOL CHikCamera::GetSaveDeqData(pPICTUREIMAGESTRUCT pPic)
{
    int isize = pPic->picInfor.iFrameNum;
    {
        std::lock_guard<std::mutex> lock(m_saveMutex);
        for (int j = 0; j < isize; j++)
        {
            FRAMEIMAGESTRUCT pft = m_ImgDeqSave.at(j);
            pPic->picInfor.mil = pft.mil;
            pPic->picInfor.deviceTimestamp[j] = pft.deviceTimestamp;
            pPic->picInfor.hostTimestamp[j] = pft.hostTimestamp;
            pPic->picInfor.auxiliaryTimestamp[j] = pft.auxiliaryTimestamp;
            pPic->picInfor.timestampFlags[j] = pft.timestampFlags;
            pPic->picInfor.timems[j] = pft.timems;                   // 1.时间戳
            pPic->picInfor.dHFreqPrecision[j] = pft.dHFreqPrecision; // 2.行频
            pPic->picInfor.pulseCount[j] = pft.pulseCount;           // 3.脉冲数
            pPic->picInfor.gykMil[j] = pft.gykMil;                   // 4.gyk里程
            int dst = m_camInfor.iSizeFrame * j;
            memcpy(pPic->pDataBuffer + dst, pft.pDataPoint, m_camInfor.iSizeFrame);
        }
    }
    ClearSaveDeg(isize);
    return TRUE;
}

void CHikCamera::ClearSaveDeg(int iSize)
{
    std::lock_guard<std::mutex> lock(m_saveMutex);
    for (int j = 0; j < iSize; j++)
    {
        if (m_ImgDeqSave.size() > 0)
        {
            FRAMEIMAGESTRUCT jp_tmp = m_ImgDeqSave.front();
            delete[] jp_tmp.pDataPoint;
            jp_tmp.pDataPoint = NULL;
            m_ImgDeqSave.pop_front();
        }
    }
}

BOOL CHikCamera::SetExpTime(double itime)
{
    std::lock_guard<std::mutex> lock(m_parameterMutex);
    if (m_handle == NULL)
        return FALSE;
    return MV_CC_SetFloatValue(m_handle, "ExposureTime", (float)itime) == MV_OK;
}

BOOL CHikCamera::GetExpTime(double &itime)
{
    std::lock_guard<std::mutex> lock(m_parameterMutex);
    if (m_handle == NULL)
        return FALSE;
    MVCC_FLOATVALUE st;
    memset(&st, 0, sizeof(st));
    if (MV_CC_GetFloatValue(m_handle, "ExposureTime", &st) == MV_OK)
    {
        itime = st.fCurValue;
        return TRUE;
    }
    return FALSE;
}

BOOL CHikCamera::SetCamGain(double iGain)
{
    std::lock_guard<std::mutex> lock(m_parameterMutex);
    if (m_handle == NULL)
        return FALSE;
    return MV_CC_SetFloatValue(m_handle, "Gain", (float)iGain) == MV_OK;
}

BOOL CHikCamera::GetCamGain(double &iGain)
{
    std::lock_guard<std::mutex> lock(m_parameterMutex);
    if (m_handle == NULL)
        return FALSE;
    MVCC_FLOATVALUE st;
    memset(&st, 0, sizeof(st));
    if (MV_CC_GetFloatValue(m_handle, "Gain", &st) == MV_OK)
    {
        iGain = st.fCurValue;
        return TRUE;
    }
    return FALSE;
}

BOOL CHikCamera::BeginSoftwareAutoBrightness()
{
    std::lock_guard<std::mutex> lock(m_parameterMutex);
    if (m_handle == NULL) return FALSE;
    if (m_autoBrightnessOverrideActive) return TRUE;

    MVCC_ENUMVALUE value;
    memset(&value, 0, sizeof(value));
    m_savedExposureAutoValid = MV_CC_GetEnumValue(m_handle, "ExposureAuto", &value) == MV_OK;
    if (m_savedExposureAutoValid) m_savedExposureAuto = value.nCurValue;
    memset(&value, 0, sizeof(value));
    m_savedGainAutoValid = MV_CC_GetEnumValue(m_handle, "GainAuto", &value) == MV_OK;
    if (m_savedGainAutoValid) m_savedGainAuto = value.nCurValue;

    // 海康GenICam枚举中 Off=0。某些线阵型号没有GainAuto节点；曝光/增益本身
    // 仍可手动写入时，不因缺少自动节点而阻断软件闭环。
    const bool exposureDisabled = !m_savedExposureAutoValid || m_savedExposureAuto == 0 ||
        MV_CC_SetEnumValue(m_handle, "ExposureAuto", 0) == MV_OK;
    const bool gainDisabled = !m_savedGainAutoValid || m_savedGainAuto == 0 ||
        MV_CC_SetEnumValue(m_handle, "GainAuto", 0) == MV_OK;
    if (!exposureDisabled || !gainDisabled)
    {
        if (m_savedExposureAutoValid)
            MV_CC_SetEnumValue(m_handle, "ExposureAuto", m_savedExposureAuto);
        if (m_savedGainAutoValid)
            MV_CC_SetEnumValue(m_handle, "GainAuto", m_savedGainAuto);
        return FALSE;
    }
    m_autoBrightnessOverrideActive = true;
    return TRUE;
}

void CHikCamera::EndSoftwareAutoBrightness()
{
    std::lock_guard<std::mutex> lock(m_parameterMutex);
    if (m_handle == NULL || !m_autoBrightnessOverrideActive) return;
    if (m_savedExposureAutoValid)
        MV_CC_SetEnumValue(m_handle, "ExposureAuto", m_savedExposureAuto);
    if (m_savedGainAutoValid)
        MV_CC_SetEnumValue(m_handle, "GainAuto", m_savedGainAuto);
    m_autoBrightnessOverrideActive = false;
    m_savedExposureAutoValid = false;
    m_savedGainAutoValid = false;
}

BOOL CHikCamera::GetCamFreg(double &iFreg)
{
    if (m_handle == NULL)
        return FALSE;

    // 优先读实际行频 ResultingLineRate，失败再读设置值 AcquisitionLineRate
    MVCC_FLOATVALUE st;
    memset(&st, 0, sizeof(st));
    if (MV_CC_GetFloatValue(m_handle, "ResultingLineRate", &st) == MV_OK)
    {
        iFreg = st.fCurValue;
        return TRUE;
    }
    memset(&st, 0, sizeof(st));
    if (MV_CC_GetFloatValue(m_handle, "AcquisitionLineRate", &st) == MV_OK)
    {
        iFreg = st.fCurValue;
        return TRUE;
    }
    // 部分型号行频为整型
    MVCC_INTVALUE_EX stInt;
    memset(&stInt, 0, sizeof(stInt));
    if (MV_CC_GetIntValueEx(m_handle, "AcquisitionLineRate", &stInt) == MV_OK)
    {
        iFreg = (double)stInt.nCurValue;
        return TRUE;
    }
    return FALSE;
}

BOOL CHikCamera::SetCamFreg(double iFreg)
{
    if (m_handle == NULL)
        return FALSE;
    if (MV_CC_SetFloatValue(m_handle, "AcquisitionLineRate", (float)iFreg) == MV_OK)
        return TRUE;
    return MV_CC_SetIntValueEx(m_handle, "AcquisitionLineRate", (int64_t)iFreg) == MV_OK;
}

_int64 CHikCamera::get_ImgDeqShow_Size()
{
    std::lock_guard<std::mutex> lock(m_showMutex);
    return static_cast<_int64>(m_ImgDeqShow.size());
}

_int64 CHikCamera::get_ImgDeqSave_Size()
{
    std::lock_guard<std::mutex> lock(m_saveMutex);
    return static_cast<_int64>(m_ImgDeqSave.size());
}

BOOL CHikCamera::DestroyObjects()
{
    if (m_handle != NULL)
    {
        if (m_bGrabbing)
        {
            MV_CC_StopGrabbing(m_handle);
            m_bGrabbing = FALSE;
        }
        MV_CC_CloseDevice(m_handle);
        MV_CC_DestroyHandle(m_handle);
        m_handle = NULL;
    }

    // 清空残留队列，释放图像内存
    ClearShowDeg();
    {
        int isize = 0;
        {
            std::lock_guard<std::mutex> lock(m_saveMutex);
            isize = (int)m_ImgDeqSave.size();
        }
        ClearSaveDeg(isize);
    }
    return TRUE;
}
