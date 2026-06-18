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
	if (g_program_setInfor.CameraType == 0)//�������
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
	else if (g_program_setInfor.CameraType == 1)//CamLink�����
	{
// 		if (m_camInfor.iExposureMode == 0)//0��ʾ�����������
// 		{
// 			return false;
// 		}

 		m_ServerCount = m_pManger->GetServerCount(SapManager::ResourceAcq);               ///< ��ȡ�ɼ�������
		m_CameraCount = m_pManger->GetServerCount(SapManager::ResourceAcqDevice);		  ///< ��ȡ�������

		if (m_ServerCount > 0)
		{
			m_ServerName = new char[256];
			m_CameraName = new char[256];

			for (int serverIndex = 0; serverIndex < m_ServerCount; serverIndex++)
			{
 				m_pManger->GetServerName(serverIndex, SapManager::ResourceAcq, m_ServerName);  ///< ����������ȡ�ɼ�������
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

	//��ʼ�����
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
	//	m_AcqDevice = new SapAcqDevice("���");
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
//	//ͼ����Ӧ��ʵ�ؼ���С
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
	//�����ع�ʱ��
	if (m_AcqDevice->SetFeatureValue("ExposureTime", (double)itime))
	{
// 		CString str;
// 		str. Format(_T("�����ع�ʱ�䣺%f"),itime);
	//	WRITE_LOG(str,LOG_LEVEL_RUN);
		return TRUE;
 	}
	else
		return FALSE;
}
BOOL CCamera::SetCamGain(int iGain)
{
	//��������
	if (m_AcqDevice->SetFeatureValue("Gain",(double)iGain)) 
	{
// 		CString str;
// 		str. Format(_T("�������棺%d"),iGain);
	//	WRITE_LOG(str,LOG_LEVEL_RUN);
		return TRUE;
	}
	else
		return FALSE;
}
BOOL CCamera::GetExpTime(double &itime)
{
 	//��ȡ�ع�ʱ��
 	if (m_AcqDevice->GetFeatureValue("ExposureTime", &itime))
	{
 		return TRUE;
	}
	else
		return FALSE;
}
BOOL CCamera::GetCamGain(double &iGain)
{
	//��ȡ����
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
		//��ȡ�ⴥ����Ƶ
		if (m_AcqDevice->GetFeatureValue("lineTriggerInputFrequency", &iFreg))
 		{
			return true;
		}
		else
			return false;
 	}
	else
	{
		//��ȡ�ڴ�����Ƶ
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
		//�����ⴥ����Ƶ
		return false;
	}
	else
	{
		//�����ڴ�����Ƶ
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
	pDlg->CallBackSub();

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

	//�õ�����
	

}
void CCamera::CallBackSub(void)
{
	void* pData = NULL;
	if(m_Buffers->GetAddress(&pData))
	{	
		if(!m_Buffers->ReleaseAddress(pData))
			return;

	//	WriteToShowDeque(pData);

#if 1


		if (!m_bEnable)//��������ӣ�δ��ʼ�洢
		{
			WriteToShowDeque(pData);
		//	YaSuoDaoFile((BYTE *)pData,4096,m_camInfor.iHeight,100,_T("D:\\TunnelScanData\\Capture"));
		}
		else //��ʼ�洢
		{
			WriteToSaveDeque(pData);
			WriteToShowDeque(pData);
		}
#endif
		if (m_xgflg)
		{
			m_xgflg = false;
			ReleaseDeqDuiqi();
		}

		printf ("                                   DeqShowSize: = %zd\tDeqSaveSize: = %zd\n", m_ImgDeqShow.size(),m_ImgDeqSave.size());
		 
 	}
	else
	{
		QString tmpInfo;
// 		tmpInfo =_T("��ȡ��������ʧ��!");
// 		WRITE_LOG(tmpInfo,LOG_LEVEL_RUN);
		return ;
	}	


}
void CCamera::YaSuoDaoFile(unsigned char* pBits,int w,int h,int q, QString strpath)
{
#if 0


	//�ļ���
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



	// TODO: �ڴ����ӿؼ�֪ͨ�����������
	//����һ��ѹ����������������ڴ�����Ҫ�Ĺ���
	jpeg_compress_struct jpeg;
	//���ڴ�����Ϣ
	jpeg_error_mgr jerr;
	//��������ڰ�
	jpeg.err = jpeg_std_error(&jerr);

	//��ʼ��ѹ������
	jpeg_create_compress(&jpeg);
	//����ѹ�������������������һ���ļ���
	FILE* pFile = fopen(p, "wb");
	if (!pFile)
		return;
	//�����
	jpeg_stdio_dest(&jpeg, pFile);

	// 	size = 512*512; 
	// 	p = (unsigned char *)malloc(size);
	// 	jpeg_mem_dest(&jpeg, &p, &size);

	//ѹ���������á������뵽��������Ӧ���ĵ��ɣ������ܶ࣬����ֻ������Ҫ�ġ�
	//������Ϊһ�� 24 λ�� 512��X��512��С�ģңǣ�ͼƬ
	jpeg.image_width = w;
	jpeg.image_height = h;
	jpeg.input_components = 1;//3;
	jpeg.in_color_space = JCS_GRAYSCALE;//JCS_RGB;
	//��������ΪĬ�ϵ�
	//  jpeg_set_quality(&jpeg, 50, TRUE); /* limit to baseline-JPEG values */
	jpeg_set_defaults(&jpeg);
	//����������Щ����������
	//// ָ�����ȼ�ɫ������
	//jpeg.q_scale_factor[0] = jpeg_quality_scaling(100);
	//jpeg.q_scale_factor[1] = jpeg_quality_scaling(100);
	//// ͼ������ʣ�Ĭ��Ϊ2 * 2
	//jpeg.comp_info[0].v_samp_factor = 1;
	//jpeg.comp_info[0].h_samp_factor = 1;
	//// �趨����jpegѹ������
	jpeg_set_quality(&jpeg, q, true);

	//��ʼѹ����ִ����һ�����ݺ��޷������ò����ˣ�
	jpeg_start_compress(&jpeg, TRUE);
	JSAMPROW row_pointer[1];
	while (jpeg.next_scanline < jpeg.image_height)
	{
		row_pointer[0] = &pBits[jpeg.next_scanline * (w)];
		jpeg_write_scanlines(&jpeg, row_pointer, 1);
	}

	//����һ�����飬����ͼƬÿһ�е����ݡ�3��������jpeg.input_components
	//unsigned char oneRowImgData[1024];
	//for (int i = 0; i<1024; ++i)
	//	oneRowImgData[i] = 0;

	//JSAMPROW row_pointer[1];
	//row_pointer[0] = oneRowImgData;
	////���ϵ��£�����ͼƬ��ÿһ�е�����ֵ
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

	//	//��һ������д�룡
	//	jpeg_write_scanlines(&jpeg, row_pointer, 1);
	//}
	//����ѹ��
	jpeg_finish_compress(&jpeg);
	//��ն���
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
	fmst.pDataPoint = new unsigned char[m_camInfor.iSizeFrame];//�Ƿ���Է��ڽṹ�嶨��ʱnew
	memcpy(fmst.pDataPoint, pData,m_camInfor.iSizeFrame);
	fmst.isShow = false;

	// {CLock lock(&m_ccs_Show);
	// m_ImgDeqShow.push_back(fmst);
	// }
}
void CCamera::WriteToSaveDeque(void* pData)
{

// 	//�ڴ˵õ������Ϣ
// 	FRAMEIMAGESTRUCT fmst;
// 	fmst.iHeight = m_camInfor.ImageHeight;
// 	fmst.iWeight = m_camInfor.ImageWidth;
// 	fmst.iSizeFrame = m_camInfor.iSizeFrame;
// 	fmst.pDataPoint = new unsigned char[m_camInfor.iSizeFrame];//�Ƿ���Է��ڽṹ�嶨��ʱnew
// //	fmst.mil = g_milDirect.GetMileageInfor();
// 	fmst.mil = cam_milDetect->GetMileageInfor();
	
// 	//1.ʱ���
// 	auto now1 = std::chrono::system_clock::now();
// 	auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now1.time_since_epoch()).count();
// 	fmst.timems = timestamp/*clock()*/;
// 	double dtmp = -2;
// 	GetCamFreg(dtmp);//2.��Ƶ
//  	fmst.dHFreqPrecision = dtmp;
// 	fmst.pulseCount = fmst.mil.iPulseCount;//3.������
// 	fmst.gykMil = m_dmil_GYK;//4.gyk���
//  	memcpy(fmst.pDataPoint, pData,m_camInfor.iSizeFrame);

// 	{CLock lock(&m_ccs_Save);
// 	m_ImgDeqSave.push_back(fmst);
// 	}
}

BOOL CCamera::IsSave(void)
{
	// if (m_ImgDeqSave.size()>=MAXFRAME)
	// {
	// 	return TRUE;
	// }
	// else
		return FALSE;
}
BOOL CCamera::IsShowII(void)
{
	// if (m_ImgDeqShow.size() >= MAXFRAME)
	// {
	// 	return TRUE;
	// }
	// else
		return FALSE;
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
	// {CLock lock(&m_ccs_Show);
	// for (int j = 0; j < isize; j++)
	// {
	// 	FRAMEIMAGESTRUCT pft = m_ImgDeqShow.at(j);

	// 	int src = 0;
	// 	int dst = m_camInfor.iSizeFrame*j;
	// 	memcpy(pDataBuffer + dst, pft.pDataPoint + src, m_camInfor.iSizeFrame);
		
	// }

	// }

	// ClearShowDeg();
	return TRUE;


}
BOOL CCamera::ClearShowDeg()
{
	// {CLock lock(&m_ccs_Show);

	// int isize = m_ImgDeqShow.size();
	// for (int j=0;j<isize;j++)
	// {
	// 	if (m_ImgDeqShow.size() >0)
	// 	{
	// 		FRAMEIMAGESTRUCT  jp_tmp = m_ImgDeqShow.front();
	// 		delete[] jp_tmp.pDataPoint;
	// 		jp_tmp.pDataPoint = NULL;
	// 		m_ImgDeqShow.pop_front();
	// 	}
 	// }
 	// }
	return TRUE;
}

BOOL CCamera::GetSaveDeqData(pPICTUREIMAGESTRUCT pPic)
{
//  	int isize = pPic->picInfor.iFrameNum;
// 	{CLock lock(&m_ccs_Save);
// 	for (int j=0;j<isize;j++)
// 	{
//  		FRAMEIMAGESTRUCT pft = m_ImgDeqSave.at(j);
// 		pPic->picInfor.mil = pft.mil;
// 		pPic->picInfor.timems[j] = pft.timems;//1.ʱ���
// 		pPic->picInfor.dHFreqPrecision[j] = pft.dHFreqPrecision;//2.��Ƶ
// 		pPic->picInfor.pulseCount[j] = pft.pulseCount;//3.������
// 		pPic->picInfor.gykMil[j] = pft.gykMil;//4.������
// // 		if (j==0/*MAXFRAME/2*/)
// // 		{
// // 			pPic->picInfor.mil = pft.mil;
// // 			pPic->picInfor.timems = pft.timems;
// // 		}
//  		int src = 0;
// 		int dst = m_camInfor.iSizeFrame*j;//128��16�����ϱ�16���ڵģ�(HEIGHT-1)��Ϊ(HEIGHT)�ͺ���
// 		memcpy(pPic->pDataBuffer+dst,pft.pDataPoint+src, m_camInfor.iSizeFrame);

 
// 	}
// 	}
//   	ClearSaveDeg(isize);
 	return TRUE;
}
BOOL CCamera::ClearSaveDeg(int iSize)
{
	// {CLock lock(&m_ccs_Save);

 	// for (int j=0;j<iSize;j++)
	// {
	// 	if (m_ImgDeqSave.size() >0)
	// 	{
	// 		FRAMEIMAGESTRUCT  jp_tmp = m_ImgDeqSave.front();
	// 		delete[] jp_tmp.pDataPoint;
	// 		jp_tmp.pDataPoint = NULL;
	// 		m_ImgDeqSave.pop_front();
	// 	}
	// }
	// }
	 return TRUE;
}

BOOL CCamera::ClearShowAndSaveDeque()
{
	// ClearShowDeg();
	// int isize = m_ImgDeqSave.size();
	// ClearSaveDeg(isize);
	return TRUE;
}
void CCamera::ReleaseDeqDuiqi(void)
{

	// {CLock lock(&m_ccs_Save);

	// if(m_ImgDeqSave.size()>0)
	// {
	// 	FRAMEIMAGESTRUCT  jp_tmp = m_ImgDeqSave.front();
	// 	delete[] jp_tmp.pDataPoint;
	// 	jp_tmp.pDataPoint = NULL;
	// 	m_ImgDeqSave.pop_front();
	// }

	// }

	// {CLock lock(&m_ccs_Show);

	// if(m_ImgDeqShow.size()>0)
	// {
	// 	FRAMEIMAGESTRUCT  jp_tmp = m_ImgDeqShow.front();
	// 	delete[] jp_tmp.pDataPoint;
	// 	jp_tmp.pDataPoint = NULL;
	// 	m_ImgDeqShow.pop_front();
	// }

	// }



}

_int64 CCamera::get_ImgDeqShow_Size()
{
	return m_ImgDeqShow.size();
}
_int64 CCamera::get_ImgDeqSave_Size()
{
	return m_ImgDeqSave.size();
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
