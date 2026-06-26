//#include "stdafx.h"
#include "Camera.h"

CCamera::CCamera(CAMERAINFORSTRUCT camInfor, QMilThd* milDetect)
{
	
//	m_myProDlg = pPro;
	m_camInfor = camInfor;
	cam_milDetect = milDetect;
	m_Xfer = NULL;
//	m_View = NULL;
	m_Buffers = NULL;
	m_AcqDevice = NULL;

	m_bEnable = FALSE;
    m_IsFullCnt = 0;
    m_xgflg = FALSE;
}
CCamera::~CCamera()
{
	StopCamera();
	DestroyObjects(); 
}
 
void CCamera::SetOffSet(int i0)
{
	m_xgflg = TRUE;
	 

}

bool CCamera::ConnectCamera()
{
	if (g_program_setInfor.CameraType == 0) // 网口相机
	{
		QByteArray ba = m_camInfor.Name.toLocal8Bit();
		char *dataPath1 = ba.data();
		if (!LoadXML(dataPath1))
		{
			//	WRITE_LOG(_T("LoadXML False"),LOG_LEVEL_RUN);
			return false;
		}
		bool tmp = NewObjects();
		return tmp;
	}
	else if (g_program_setInfor.CameraType == 1) // CamLink口相机
	{
		// 		if (m_camInfor.iExposureMode == 0)//0表示此相机不工作
		// 		{
		// 			return false;
		// 		}

		m_ServerCount = m_pManger->GetServerCount(SapManager::ResourceAcq);				  ///< 获取采集卡数量
		m_CameraCount = m_pManger->GetServerCount(SapManager::ResourceAcqDevice);		  ///< 获取相机数量

		if (m_ServerCount > 0)
		{
			m_ServerName = new char[256];
			m_CameraName = new char[256];

			for (int serverIndex = 0; serverIndex < m_ServerCount; serverIndex++)
			{
				m_pManger->GetServerName(serverIndex, SapManager::ResourceAcq, m_ServerName);			 ///< 根据索引获取采集卡名称
				m_pManger->GetServerName(serverIndex, SapManager::ResourceAcqDevice, m_CameraName);      ///< 
 			 
 				char CamLinkName_c[256];
				strcpy(CamLinkName_c, m_camInfor.CamLinkName.toLocal8Bit().data());
				if (strcmp(m_CameraName, CamLinkName_c) == 0)
				{
					break;
				}
				 
			}
		}
		else
		{
			return false;
		}
		QString FileName = ".\\";
		FileName += m_camInfor.Name;
		FileName += ".ccf";
		if (InitCamera_CamLink(m_ServerName, 0, m_CameraName, FileName))
		{
			bool tmp = m_Xfer->Grab();
			return tmp;
		}
		else
		{
			return false;
		}

	}
	return false;
}
BOOL CCamera::InitCamera_CamLink(char *serverName, int resourceIndex, char *cameraName, QString ccfPath)
{
	SapLocation loc(serverName, resourceIndex);

	char pccfPath[256];
	strcpy(pccfPath, ccfPath.toLocal8Bit().data());
	m_pAcquisition = new SapAcquisition(loc, pccfPath);
	m_Buffers = new SapBufferWithTrash(2, m_pAcquisition);
	m_Xfer = new SapAcqToBuf(m_pAcquisition, m_Buffers, XferCallback, this);

	if (m_pAcquisition && !*m_pAcquisition && !m_pAcquisition->Create()) return FALSE;
	if (m_Buffers && !*m_Buffers && !m_Buffers->Create()) return FALSE;

	m_iImageWidth = m_Buffers->GetWidth();
	m_iImageHeight = m_Buffers->GetHeight();

	m_pBufferExtrack = new SapBuffer(2, m_iImageWidth, m_iImageHeight, SapFormatMono8);

	if (m_pBufferExtrack && !*m_pBufferExtrack && !m_pBufferExtrack->Create()) return FALSE;
	if (m_Xfer && !*m_Xfer && !m_Xfer->Create()) return FALSE;

	// 初始化相机
	SapLocation locCam(cameraName, 0);
	m_AcqDevice = new SapAcqDevice(locCam, FALSE);
	m_pFeature = new SapFeature(m_AcqDevice->GetLocation());
	m_AcqDevice->Create();
	m_pFeature->Create();

	return TRUE;
}

BOOL CCamera::LoadXML(char* lpPath)
{
//	m_myProDlg->m_progress.OffsetPos(10);
	int serverCount = 0;
	SapManager::SetDisplayStatusMode(SapManager::StatusCustom); //Gets/sets the global reporting mode for messages and errors. 
	serverCount = SapManager::GetServerCount();//Gets the number of available Sapera servers.


	int GenieIndex = 0;
	BOOL serverFound = FALSE;
	char serverName[CORSERVER_MAX_STRLEN];
	if (serverCount == 1)
	{	
		return false;
	}
	for (int serverIndex = 0; serverIndex <serverCount; serverIndex++)
	{
		//Gets the number of resources of a specified type on a Sapera server. 
		char userDefinedName[100]={0};
		if (SapManager::GetResourceCount(serverIndex, SapManager::ResourceAcqDevice) != 0)
		{	
			//Gets the name of a Sapera resource of a specified type.
			if(SapManager::GetResourceName(serverIndex,SapManager::ResourceAcqDevice,serverName,sizeof(serverName)))
			{

				if(strcmp(serverName,lpPath) == 0)
				{
					SapManager::GetServerName(serverIndex, serverName, sizeof(serverName));	//Gets the name of a Sapera server.
					m_AcqDevice = new SapAcqDevice(serverName);					
					return true;
				}
			}
		}

	}
	//m_myProDlg->m_progress.OffsetPos(50);
	return false;
}
 
bool CCamera::NewObjects()
{
	//	m_AcqDevice = new SapAcqDevice("左侧");
	//	m_AcqDevice	= new SapAcqDevice(dlg.GetLocation(), dlg.GetConfigFile());
	m_Buffers	= new SapBufferWithTrash(10, m_AcqDevice);
	m_Xfer		= new SapAcqDeviceToBuf(m_AcqDevice, m_Buffers, XferCallback, this);
	//	m_View      = new SapView(m_Buffers, m_viewWnd.GetSafeHwnd());
//	m_View      = new SapView(m_Buffers, GetDlgItem(IDC_LEFT_SHOW)->GetSafeHwnd());
//	m_View      = new SapView(m_Buffers, m_hWnd);

	// Create all objects
//	if (!CreateObjects()) { EndDialog(TRUE); return FALSE; }
	//m_myProDlg->m_progress.OffsetPos(80);
	CreateObjects();
	bool tmp = m_Xfer->Grab();
	//m_myProDlg->m_progress.OffsetPos(100);
	return tmp;
}
BOOL CCamera::CreateObjects()
{
 	// Create acquisition object
	if (m_AcqDevice && !*m_AcqDevice && !m_AcqDevice->Create())
	{
	//	WRITE_LOG("CreateObjects1",LOG_LEVEL_RUN);
		DestroyObjects();
		return FALSE;
	}


	
  	// Create buffer object
	if (m_Buffers && !*m_Buffers)
	{
		if( !m_Buffers->Create())
		{
		//	WRITE_LOG(_T("CreateObjects2"),LOG_LEVEL_RUN);
			DestroyObjects();
			return FALSE;
		}
		// Clear all buffers
		m_Buffers->Clear();
	}

	// Create view object
	// 	if (m_View && !*m_View && !m_View->Create())
	// 	{
	// 		WRITE_LOG(_T("CreateObjects3"),LOG_LEVEL_RUN);
	// 		DestroyObjects();
	// 		return FALSE;
	// 	}
	//	//图像适应现实控件大小
	//	m_View->SetScalingMode(SapView::ScalingFitToWindow);

	// Set next empty with trash cycle mode for transfer
	if (m_Xfer && m_Xfer->GetPair(0))
	{
		if (!m_Xfer->GetPair(0)->SetCycleMode(SapXferPair::CycleNextWithTrash))
		{
		//	WRITE_LOG(_T("CreateObjects4"),LOG_LEVEL_RUN);
			DestroyObjects();
			return FALSE;
		}
	}

	// Create transfer object
	if (m_Xfer && !*m_Xfer && !m_Xfer->Create())
	{
	//	WRITE_LOG(_T("CreateObjects5"),LOG_LEVEL_RUN);
		DestroyObjects();
		return FALSE;
	}

	return TRUE;
}
BOOL CCamera::DestroyObjects()
{
	// Destroy transfer object
	if ((m_Xfer != NULL) && *m_Xfer) m_Xfer->Destroy();

	// Destroy view object
	//	if ((m_View!=NULL)  && *m_View) m_View->Destroy();

	// Destroy buffer object
	if ((m_Buffers != NULL) && *m_Buffers) m_Buffers->Destroy();

	// Destroy acquisition object
	if ((m_AcqDevice != NULL) && *m_AcqDevice) m_AcqDevice->Destroy();


	// Delete all objects
	if (m_Xfer != NULL)
	{
		delete m_Xfer; m_Xfer = NULL;
	}
	//	if (m_ImageWnd)	delete m_ImageWnd;
	//	if (m_View!=NULL)			
	//	{delete m_View; m_View=NULL;}
	if (m_Buffers != NULL)
	{
		delete m_Buffers; m_Buffers = NULL;
	}
	if (m_AcqDevice != NULL)
	{
		delete m_AcqDevice; m_AcqDevice = NULL;
	}

	return TRUE;
}
BOOL CCamera::SetExpTime(double itime)
{
	// BOOL GetFeatureValue(const char *featureName, double *featureValue);
// 	double dtmp = -2;
//   	m_AcqDevice->GetFeatureValue("ExposureTime", &dtmp);
// 	double uitmp = 0;
// 	m_AcqDevice->GetFeatureValue("lineTriggerInputFrequency", &uitmp);

	//   BOOL SetFeatureValue(const char *featureName, double featureValue);
	// 设置曝光时间
	if (m_AcqDevice->SetFeatureValue("ExposureTime", (double)itime))
	{
		// 		CString str;
		// 		str. Format(_T("设置曝光时间：%f"),itime);
		//	WRITE_LOG(str,LOG_LEVEL_RUN);
		return TRUE;
 	}
	else
		return FALSE;
}
BOOL CCamera::SetCamGain(int iGain)
{
	// 设置增益
	if (m_AcqDevice->SetFeatureValue("Gain",(double)iGain)) 
	{
		// 		CString str;
		// 		str. Format(_T("设置增益：%d"),iGain);
		//	WRITE_LOG(str,LOG_LEVEL_RUN);
		return TRUE;
	}
	else
		return FALSE;
}
BOOL CCamera::GetExpTime(double &itime)
{
	// 获取曝光时间
	if (m_AcqDevice->GetFeatureValue("ExposureTime", &itime))
	{
 		return TRUE;
	}
	else
		return FALSE;
}
BOOL CCamera::GetCamGain(double &iGain)
{
	// 获取增益
	if (m_AcqDevice->GetFeatureValue("Gain", &iGain))
	{
		return TRUE;
	}
	else
		return false;
}
BOOL CCamera::GetCamFreg(double &iFreg)
{
	BOOL tmp = false;
	m_AcqDevice->GetFeatureValue("TriggerMode", &tmp); 
	if (tmp)
	{
		// 获取外触发行频
		if (m_AcqDevice->GetFeatureValue("lineTriggerInputFrequency", &iFreg))
 		{
			return true;
		}
		else
			return false;
 	}
	else
	{
		// 获取内触发行频
		if (m_AcqDevice->GetFeatureValue("AcquisitionLineRate", &iFreg))
		{
			return true;
		}
		else
			return false;
	}
	return false;
}
BOOL CCamera::SetCamFreg(double iFreg)
{
	BOOL tmp = false;
	m_AcqDevice->GetFeatureValue("TriggerMode", &tmp);
	if (tmp)
	{
		// 设置外触发行频
		return false;
	}
	else
	{
		// 设置内触发行频
		if (m_AcqDevice->SetFeatureValue("AcquisitionLineRate", (double)iFreg))
		{
			return true;
		}
		else
			return false;
	}
	return false;
}
void CCamera::XferCallback(SapXferCallbackInfo *pInfo)
{
	CCamera *pDlg= (CCamera *) pInfo->GetContext();
	pDlg->CallBackSub(pInfo);

	// If grabbing in trash buffer, do not display the image, update the
	// appropriate number of frames on the status bar instead
//	if (pInfo->IsTrash())
//	{
	//	CString str;
	//	str.Format("Frames acquired in trash buffer: %d", pInfo->GetEventCount());
	//	WRITE_LOG(str,LOG_LEVEL_RUN);
	//	pDlg->m_statusWnd.SetWindowText(str);
//	}

	// Refresh view
// 	else
// 		pDlg->m_View->Show();

	// 得到数据
}
void CCamera::CallBackSub(SapXferCallbackInfo *pInfo)
{
	void* pData = NULL;
	if(m_Buffers->GetAddress(&pData))
	{	
		if(!m_Buffers->ReleaseAddress(pData))
			return;

	//	WriteToShowDeque(pData);

#if 1

		if (!m_bEnable) // 相机已连接，未开始存储
		{
			WriteToShowDeque(pData);
		//	YaSuoDaoFile((BYTE *)pData,4096,m_camInfor.iHeight,100,_T("D:\\TunnelScanData\\Capture"));
		}
		else // 开始存储
		{
			WriteToSaveDeque(pData, pInfo);
			WriteToShowDeque(pData);
		}
#endif
		if (m_xgflg)
		{
			m_xgflg = false;
			ReleaseDeqDuiqi();
		}

	//	printf ("                                   DeqShowSize: = %zd\tDeqSaveSize: = %zd\n", m_ImgDeqShow.size(),m_ImgDeqSave.size());
		 
 	}
	else
	{
		QString tmpInfo;
		// 		tmpInfo =_T("获取缓存数据失败!");
		// 		WRITE_LOG(tmpInfo,LOG_LEVEL_RUN);
		return ;
	}	


}
void CCamera::FillFrameTimestamp(SapXferCallbackInfo *pInfo, FRAMEIMAGESTRUCT &fmst)
{
	if (m_Buffers != NULL)
	{
		UINT64 deviceTimestamp = 0;
		int bufferIndex = m_Buffers->GetIndex();
		BOOL hasDeviceTimestamp = FALSE;

		if (bufferIndex >= 0)
		{
			hasDeviceTimestamp = m_Buffers->GetDeviceTimeStamp(bufferIndex, &deviceTimestamp);
		}

		if (!hasDeviceTimestamp)
		{
			hasDeviceTimestamp = m_Buffers->GetDeviceTimeStamp(&deviceTimestamp);
		}

		if (hasDeviceTimestamp)
		{
			fmst.deviceTimestamp = deviceTimestamp;
			fmst.timestampFlags |= FRAME_TIMESTAMP_DEVICE_VALID;
		}
	}

	if (pInfo != NULL)
	{
		UINT64 hostTimestamp = 0;
		if (pInfo->GetHostTimeStamp(&hostTimestamp))
		{
			fmst.hostTimestamp = hostTimestamp;
			fmst.timestampFlags |= FRAME_TIMESTAMP_HOST_VALID;
		}

		UINT64 auxiliaryTimestamp = 0;
		if (pInfo->GetAuxiliaryTimeStamp(&auxiliaryTimestamp))
		{
			fmst.auxiliaryTimestamp = auxiliaryTimestamp;
			fmst.timestampFlags |= FRAME_TIMESTAMP_AUXILIARY_VALID;
		}
	}
}

void CCamera::YaSuoDaoFile(unsigned char* pBits,int w,int h,int q, QString strpath)
{
#if 0


	//文件名
	CTime t = CTime::GetCurrentTime();
	CString strgrab;
	strgrab.Format(_T("%d%02d%02d%02d%02d%02d"),
		t.GetYear(), t.GetMonth(), t.GetDay(),
		t.GetHour(), t.GetMinute(), t.GetSecond());

	USES_CONVERSION;
	CString str = _T("camera0");
	strgrab += str;
	strgrab += _T(".jpg");

	CString strFileName = strpath;
	strFileName += _T("\\");
	strFileName += strgrab;

	char * p = T2A(strFileName);   



	// TODO: 在此添加控件通知处理程序代码
    //定义一个压缩对象，这个对象用于处理主要的功能
	jpeg_compress_struct jpeg;
	//用于错误信息
	jpeg_error_mgr jerr;
	//错误输出在绑定
	jpeg.err = jpeg_std_error(&jerr);

	//初始化压缩对象
	jpeg_create_compress(&jpeg);
	//定义压缩后的输出，这里输出到一个文件！
	FILE* pFile = fopen(p, "wb");
	if (!pFile)
		return;
	//绑定输出
	jpeg_stdio_dest(&jpeg, pFile);

	// 	size = 512*512; 
	// 	p = (unsigned char *)malloc(size);
	// 	jpeg_mem_dest(&jpeg, &p, &size);

	//压缩参数设置。具体请到网上找相应的文档吧，参数很多，这里只设置主要的。
    //我设置为一个 24 位的 512　X　512大小的ＲＧＢ图片
	jpeg.image_width = w;
	jpeg.image_height = h;
	jpeg.input_components = 1;//3;
	jpeg.in_color_space = JCS_GRAYSCALE;//JCS_RGB;
	//参数设置为默认的
	//  jpeg_set_quality(&jpeg, 50, TRUE); /* limit to baseline-JPEG values */
	jpeg_set_defaults(&jpeg);
	//还可以设置些其他参数
	//// 指定亮度及色度质量
	//jpeg.q_scale_factor[0] = jpeg_quality_scaling(100);
	//jpeg.q_scale_factor[1] = jpeg_quality_scaling(100);
	//// 图像采样率，默认为2 * 2
	//jpeg.comp_info[0].v_samp_factor = 1;
	//jpeg.comp_info[0].h_samp_factor = 1;
	//// 设置jpeg压缩质量
	jpeg_set_quality(&jpeg, q, true);

	//开始压缩。执行这一行数据后，无法再设置参数了！
	jpeg_start_compress(&jpeg, TRUE);
	JSAMPROW row_pointer[1];
	while (jpeg.next_scanline < jpeg.image_height)
	{
		row_pointer[0] = &pBits[jpeg.next_scanline * (w)];
		jpeg_write_scanlines(&jpeg, row_pointer, 1);
	}

	//定义一个数组，代表图片每一行的数据。3　代表　jpeg.input_components
	//unsigned char oneRowImgData[1024];
	//for (int i = 0; i<1024; ++i)
	//	oneRowImgData[i] = 0;

	//JSAMPROW row_pointer[1];
	//row_pointer[0] = oneRowImgData;
	////从上到下，设置图片中每一行的像素值
	//for (int i = 0; i<jpeg.image_height; ++i)
	//{
	//	int data = 0;
	//	for (int j = 0; j<jpeg.image_width; j = j + 1)
	//	{
	//		oneRowImgData[j] = *fm.pData;//data%255;//R
	//									 //oneRowImgData[j+1] = data%255;//G
	//									 //oneRowImgData[j+2] = data%255;//B
	//		*fm.pData++;
	//	}

	//	//将一行数据写入！
	//	jpeg_write_scanlines(&jpeg, row_pointer, 1);
	//}
	//结束压缩
	jpeg_finish_compress(&jpeg);
	//清空对象
	jpeg_destroy_compress(&jpeg);

	fclose(pFile); pFile = NULL;

#endif
}

void CCamera::WriteToShowDeque(void* pData)
{
	FRAMEIMAGESTRUCT fmst;
	fmst.iHeight = m_camInfor.ImageHeight;
	fmst.iWeight = m_camInfor.ImageWidth;
	fmst.iSizeFrame = m_camInfor.iSizeFrame;
	fmst.pDataPoint = new unsigned char[m_camInfor.iSizeFrame]; // 是否可以放在结构体定义时new
	memcpy(fmst.pDataPoint, pData,m_camInfor.iSizeFrame);
	fmst.isShow = false;

	{
		std::lock_guard<std::mutex> lock(m_showMutex);
		m_ImgDeqShow.push_back(fmst);
	}
}
void CCamera::WriteToSaveDeque(void* pData, SapXferCallbackInfo *pInfo)
{

	// 在此得到里程信息
	FRAMEIMAGESTRUCT fmst;
	fmst.iHeight = m_camInfor.ImageHeight;
	fmst.iWeight = m_camInfor.ImageWidth;
	fmst.iSizeFrame = m_camInfor.iSizeFrame;
	fmst.pDataPoint = new unsigned char[m_camInfor.iSizeFrame]; // 是否可以放在结构体定义时new
	//	fmst.mil = g_milDirect.GetMileageInfor();
	fmst.mil = cam_milDetect->GetMileageInfor();

	// 1.时间戳
	auto now1 = std::chrono::system_clock::now();
	auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now1.time_since_epoch()).count();
	fmst.timems = timestamp/*clock()*/;
	FillFrameTimestamp(pInfo, fmst);
	double dtmp = -2;
	GetCamFreg(dtmp); // 2.行频
	fmst.dHFreqPrecision = dtmp;
	fmst.pulseCount = fmst.mil.iPulseCount; // 3.脉冲数
	fmst.gykMil = m_dmil_GYK;//4.gyk里程
 	memcpy(fmst.pDataPoint, pData,m_camInfor.iSizeFrame);

	{
		std::lock_guard<std::mutex> lock(m_saveMutex);
		m_ImgDeqSave.push_back(fmst);
	}
}

BOOL CCamera::IsSave(void)
{
	std::lock_guard<std::mutex> lock(m_saveMutex);
	return m_ImgDeqSave.size() >= MAXFRAME;

	// if (m_ImgDeqSave.size()>=MAXFRAME)
	// {
	// 	return TRUE;
	// }
	// else
	// 	return FALSE;
}
BOOL CCamera::IsShowII(void)
{
	std::lock_guard<std::mutex> lock(m_showMutex);
	return m_ImgDeqShow.size() >= MAXFRAME;

	// if (m_ImgDeqShow.size() >= MAXFRAME)
	// {
	// 	return TRUE;
	// }
	// else
	// 	return FALSE;
}
 
void CCamera::StartAcquisition(void)
{
	m_bEnable = TRUE;
}
void CCamera::StopAcquisition(void)
{
	m_bEnable = FALSE;
}
void CCamera::StopCamera()
{
	if (m_Xfer == NULL)
	{
		return;
	}
	if (m_Xfer !=NULL)
	{
		m_Xfer->Abort();
	}
	
}

BOOL CCamera::GetShowDeqDataII(unsigned char* pDataBuffer, int isize)
{
	{
		std::lock_guard<std::mutex> lock(m_showMutex);
		for (int j = 0; j < isize; j++)
		{
			FRAMEIMAGESTRUCT pft = m_ImgDeqShow.at(j);

			int src = 0;
			int dst = m_camInfor.iSizeFrame * j;
			memcpy(pDataBuffer + dst, pft.pDataPoint + src, m_camInfor.iSizeFrame);
		}
	}

	ClearShowDeg();
	return TRUE;


}
BOOL CCamera::ClearShowDeg()
{
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
	return TRUE;
}

BOOL CCamera::GetSaveDeqData(pPICTUREIMAGESTRUCT pPic)
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
			pPic->picInfor.timems[j] = pft.timems;					 // 1.时间戳
			pPic->picInfor.dHFreqPrecision[j] = pft.dHFreqPrecision; // 2.行频
			pPic->picInfor.pulseCount[j] = pft.pulseCount;			 // 3.脉冲数
			pPic->picInfor.gykMil[j] = pft.gykMil;					 // 4.gyk里程
			// 		if (j==0/*MAXFRAME/2*/)
			// 		{
			// 			pPic->picInfor.mil = pft.mil;
			// 			pPic->picInfor.timems = pft.timems;
			// 		}
			int src = 0;
			int dst = m_camInfor.iSizeFrame * j; // 128改16后，最上边16条黑的，(HEIGHT-1)改为(HEIGHT)就好了
			memcpy(pPic->pDataBuffer + dst, pft.pDataPoint + src, m_camInfor.iSizeFrame);
		}
	}
	ClearSaveDeg(isize);
 	return TRUE;
}
BOOL CCamera::ClearSaveDeg(int iSize)
{
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
	 return TRUE;
}

BOOL CCamera::ClearShowAndSaveDeque()
{
	ClearShowDeg();
	int isize = get_ImgDeqSave_Size();
	ClearSaveDeg(isize);
	return TRUE;
}
void CCamera::ReleaseDeqDuiqi(void)
{

	{
		std::lock_guard<std::mutex> lock(m_saveMutex);

		if (m_ImgDeqSave.size() > 0)
		{
			FRAMEIMAGESTRUCT jp_tmp = m_ImgDeqSave.front();
			delete[] jp_tmp.pDataPoint;
			jp_tmp.pDataPoint = NULL;
			m_ImgDeqSave.pop_front();
		}
	}

	{
		std::lock_guard<std::mutex> lock(m_showMutex);

		if (m_ImgDeqShow.size() > 0)
		{
			FRAMEIMAGESTRUCT jp_tmp = m_ImgDeqShow.front();
			delete[] jp_tmp.pDataPoint;
			jp_tmp.pDataPoint = NULL;
			m_ImgDeqShow.pop_front();
		}
	}
}

_int64 CCamera::get_ImgDeqShow_Size()
{
	std::lock_guard<std::mutex> lock(m_showMutex);
	return static_cast<_int64>(m_ImgDeqShow.size());

}
_int64 CCamera::get_ImgDeqSave_Size()
{
	std::lock_guard<std::mutex> lock(m_saveMutex);
	return static_cast<_int64>(m_ImgDeqSave.size());
}




#if 0

BOOL CCamera::IsFullCnt(void)
{
	if (m_IsFullCnt >= MAXFRAME*2)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

FRAMEIMAGESTRUCT CTunnelTestDlg::GetDeqFrontA(int &tmp,int incc)
{
	{CLock lock(&m_ccs_DeqA);

	FRAMEIMAGESTRUCT ft;
	int isize = g_ImgDeqA.size();
	if (incc < isize)
	{
		ft = g_ImgDeqA.at(incc);
		tmp = 1;
		return ft;
	}
	else
	{
		tmp = 0;
	}

	}

}
void CTunnelTestDlg::ClearDeqA_All(void)
{

	{CLock lock(&m_ccs_DeqA);

	int isize = g_ImgDeqA.size();

	for (int j=0;j<isize;j++)
	{
		FRAMEIMAGESTRUCT  jp_tmp = g_ImgDeqA.front();
		delete[] jp_tmp.pDataPoint;
		jp_tmp.pDataPoint = NULL;
		g_ImgDeqA.pop_front();
	}



	}
}



{

	m_Xfer->Abort()
	Sleep(100);


	Sleep(10);


}

#endif // 0
