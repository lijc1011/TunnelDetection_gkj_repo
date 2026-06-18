//#include "stdafx.h"
#include "QMilThd.h"

QMilThd::QMilThd(QObject *parent)
	: QObject(parent)
{
	m_udCounterCtrl = NULL;

	m_iPulsSave = 0;
	m_iTimeSave = 0;
	m_fSetMileage = 0;
	m_fFactor = 1;
	m_iZengOrJian = 0;//Ĭ������
}

QMilThd::~QMilThd()
{
	StopCount();
	// Step 7: close device and release any allocated resource.
	m_udCounterCtrl->Cleanup();

	// Step 8: destroy the object
	m_udCounterCtrl->Dispose();
}
BOOL QMilThd::SetDevNum(int iNum, double dUnitMil)
{
	m_iDevNum = iNum;
	m_fUnitMil = dUnitMil;//0.8096;//1.1530;//1.055;//0.2650;// 0.2654;//0.2638;//0.2638;//0.5749;//
	return TRUE;
}
BOOL QMilThd::InitMil(void)
{
	// Step 1: create 'UdCounterCtrl' object
	m_udCounterCtrl = Automation::BDaq::UdCounterCtrl::Create();

	// Step 2: select the device with 'ModeWrite' mode
	int deviceNumber = m_iDevNum;
	Automation::BDaq::DeviceInformation devInfo(deviceNumber);
	Automation::BDaq::ErrorCode errorCode = m_udCounterCtrl->setSelectedDevice(devInfo);
	if (!CheckError(errorCode)) return FALSE;

	// 	CString   m_profilePath = _T(".\\PCI-1784.xml");
	// 	WCHAR* profilePath = (WCHAR*)m_profilePath.GetBuffer(m_profilePath.GetLength());
	// 	errorCode = m_udCounterCtrl->LoadProfile(profilePath);


#if 1 //��Ͳ������
	// Step 3: select the counter channel
	int channelStart = 0;
	int channelCount = 1;
	errorCode = m_udCounterCtrl->setChannelStart(channelStart);
	if (!CheckError(errorCode)) return FALSE;
	errorCode = m_udCounterCtrl->setChannelCount(channelCount);
	if (!CheckError(errorCode)) return FALSE;

	// Step 4: set counting type
	for (int i = m_udCounterCtrl->getChannelStart(); i < m_udCounterCtrl->getChannelStart() + m_udCounterCtrl->getChannelCount(); i++)
	{
		errorCode = m_udCounterCtrl->getChannels()->getItem(i).setCountingType(Automation::BDaq::AbPhaseX1);
		if (!CheckError(errorCode)) return FALSE;
	}
#endif


	SetZero();
	return TRUE;
}
BOOL QMilThd::CheckError(Automation::BDaq::ErrorCode errorCode)
{
	if (errorCode != Automation::BDaq::Success)
	{
		// 		CString str;
		// 		str.Format(_T("Some errors happened, the error code is: 0x%X !\n"),errorCode);
		// 		AfxMessageBox(str);
		return FALSE;
	}
	else
		return TRUE;
}
BOOL QMilThd::StartCount(void)
{
	// Step 5: start up-down counting
	Automation::BDaq::ErrorCode errorCode = m_udCounterCtrl->setEnabled(true);
	if (!CheckError(errorCode)) return false;

	m_isBreakThd = true;
	m_isCanRun = true;
	std::thread *pt1(new std::thread(runThdFun, this));
	return TRUE;
}
BOOL QMilThd::StopCount(void)
{
	// Step 6: stop up-down counting
 	Automation::BDaq::ErrorCode errorCode = m_udCounterCtrl->setEnabled(FALSE);
 	if (!CheckError(errorCode)) return FALSE;
	m_isBreakThd = false;
	m_isCanRun = false;
	return TRUE;
}
void QMilThd::runThdFun(QMilThd* pParameter)
{
	pParameter->sub_runThdFun();
}
void QMilThd::sub_runThdFun()
{
	while (m_isBreakThd)
	{
		//	std::lock_guard<std::mutex> lockGuard(my_lock);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		if (m_isCanRun)
		{


			//	int inum = timemE - timems;
			// 		double value = 0.0;
			// 		m_freqMeterCtrl->Read(1, &value)

			Automation::BDaq::int32 udValue = 0;
			clock_t timems = clock();
			/*	Sleep(100);*/
			m_udCounterCtrl->Read(1, &udValue);

			//	udValue = 0-udValue;



			{//CLock lock(&m_ccs_Show);
			//�����Ϣ�ṹ�帳ֵ
			//������
			m_mileageInfor.iPulseCount = udValue;
#if 1
			//����ת
			if (udValue > m_iPulsSave)
			{
				m_mileageInfor.uiIncOrDec = 0;//0��ת,����
			}
			else if (udValue < m_iPulsSave)
			{
				m_mileageInfor.uiIncOrDec = 1;
			}

			//����
			if (udValue > 0)
			{
				m_mileageInfor.uiPositiveOrNegative = 0;//0����
			}
			else if (udValue < 0)
			{
				m_mileageInfor.uiPositiveOrNegative = 1;
			}
#endif


			//���
			udValue = abs(udValue);//ȡ����ֵ��û�и����
			if (m_iZengOrJian == 0)
			{
				//����
				m_mileageInfor.fMileage = m_fSetMileage + udValue*m_fUnitMil / 1000;//��λ��
			}
			else if (m_iZengOrJian == 1)
			{
				//����
				m_mileageInfor.fMileage = m_fSetMileage - udValue*m_fUnitMil / 1000;//��λ��
			}
			//	m_mileageInfor.fMileage = udValue*m_fUnitMil/1000;//��λ��

			//�ٶ�
#if 1
			if (abs(m_iTimeSave - timems) >= 1000)
			{
				int tmp = abs(m_iPulsSave - udValue);
				int itim = abs(m_iTimeSave - timems);
				float maichong = (float)tmp / itim;
				m_mileageInfor.fSpeed = maichong*m_fUnitMil*3.6*m_fFactor;	//mm/ms����m/s

																			// 			USES_CONVERSION;
																			// 			CString str;
																			// 			str.Format(_T("%f = %d / %d����%f\n"),maichong,abs(m_iPulsSave-udValue),abs(m_iTimeSave-timems),m_mileageInfor.fSpeed);
																			// 			char * p1 = T2A(str);   
																			// 			printf("%f = %d / %d����%f\n",maichong,abs(m_iPulsSave-udValue),abs(m_iTimeSave-timems),m_mileageInfor.fSpeed);


				m_iPulsSave = udValue;
				m_iTimeSave = timems;
			}
			}
#endif
			// 		m_iPulsSave = udValue;
			// 		m_iTimeSave = timems;


			

		}
	}
}



MILEAGEINFORSTRUCT QMilThd::GetMileageInfor(void)
{
	MILEAGEINFORSTRUCT tmp;
	{//CLock lock(&m_ccs_Show);


	tmp.iPulseCount = m_mileageInfor.iPulseCount;
	tmp.uiIncOrDec = m_mileageInfor.uiIncOrDec;
	tmp.uiPositiveOrNegative = m_mileageInfor.uiPositiveOrNegative;
	// 	tmp.fMileage = (m_mileageInfor.fMileage + m_fSetMileage)*m_fFactor;
	tmp.fMileage = m_mileageInfor.fMileage;//+ m_fSetMileage;
	tmp.fSpeed = m_mileageInfor.fSpeed;//
	tmp.uiIncOrDecMil = m_iZengOrJian; }
	return tmp;
}

void QMilThd::SetUnitMil(float fValue)
{
	m_fUnitMil = fValue;
}
void QMilThd::SetZero(void)
{
	m_udCounterCtrl->ValueReset();
	m_fSetMileage = 0;
}

void QMilThd::SetNowMileage(double dValue)
{
	//	m_fSetMileage = dValue - m_mileageInfor.fMileage;
	m_fSetMileage = dValue;// - m_mileageInfor.fMileage;
	m_udCounterCtrl->ValueReset();
}

void QMilThd::SetZengjiaOrJianshao(int dValue)
{
	m_iZengOrJian = dValue;
}

void QMilThd::SetCorrectionfactor(double dFactor)
{
	m_fFactor = dFactor;
}

/////////1784do����///////////////////////////////////////////////////////////////////
bool QMilThd::InitDiDo(int devnum)
{
	//1784do����
	m_instantDoCtrl = Automation::BDaq::AdxInstantDoCtrlCreate();

	// Step 2: select the device with 'ModeWriteWithReset' mode
	int deviceNumber = devnum;
	Automation::BDaq::DeviceInformation devInfoDO(deviceNumber);
	Automation::BDaq::ErrorCode errorCode = m_instantDoCtrl->setSelectedDevice(devInfoDO);
	if (!CheckError(errorCode)) return FALSE;
	return TRUE;
}
bool QMilThd::JieTongCmd(unsigned char bValue)
{
    unsigned char portData = 0;
	Automation::BDaq::int32 const rportStart = 0;
	Automation::BDaq::int32 const rportCount = 1;
	Automation::BDaq::ErrorCode rerrorCode = m_instantDoCtrl->Read(rportStart, rportCount, &portData);
	bValue = portData | bValue;
	WriteValue(bValue);
	return TRUE;
}
bool QMilThd::WriteValue(byte bValue)
{
	// Step 3: Scan the ports
    Automation::BDaq::int32 const portStart = 0;
	Automation::BDaq::int32 const portCount = 1;
	Automation::BDaq::ErrorCode errorCode = m_instantDoCtrl->Write(portStart, portCount, &bValue);
	if (!CheckError(errorCode)) return FALSE;
	Sleep(30);
	return TRUE;
}
bool QMilThd::ReadValue(unsigned char &bValue)
{
	Automation::BDaq::int32 const portStart = 0;
	Automation::BDaq::int32 const portCount = 1;
	Automation::BDaq::ErrorCode errorCode = m_instantDoCtrl->Read(portStart, portCount, &bValue);
	if (!CheckError(errorCode)) return FALSE;
	return TRUE;
}
bool QMilThd::DuankaiCmd(unsigned char bCmd)
{
    unsigned char portDataRead = 0;
	Automation::BDaq::int32 const portStart = 0;
	Automation::BDaq::int32 const portCount = 1;
	Automation::BDaq::ErrorCode errorCode = m_instantDoCtrl->Read(portStart, portCount, &portDataRead);
	CheckError(errorCode);
	unsigned char bValue = static_cast<unsigned char>(~bCmd);
	bValue = static_cast<unsigned char>(bValue & portDataRead);

	WriteValue(bValue);
	return TRUE;
}