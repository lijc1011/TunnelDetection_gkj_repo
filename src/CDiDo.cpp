//#include "StdAfx.h"
#include "CDiDo.h"
//#include "bdaqctrl.h"
CDiDo::CDiDo(void)
{
	// Step 1: create 'InstantDoCtrl' object
	m_instantDoCtrl = Automation::BDaq::AdxInstantDoCtrlCreate();
	m_bOpenTrueOrCloseFalse = FALSE;
 	 
}


CDiDo::~CDiDo(void)
{
	// Step 4: close device and release any allocated resource.
	m_instantDoCtrl->Cleanup();

	// Step 5: destroy the object
	m_instantDoCtrl->Dispose();
}
BOOL CDiDo::InitDiDo(int devnum)
{
	// Step 2: select the device with 'ModeWriteWithReset' mode
	int deviceNumber = devnum;
	Automation::BDaq::DeviceInformation devInfoDO(deviceNumber);
	Automation::BDaq::ErrorCode errorCode = m_instantDoCtrl->setSelectedDevice(devInfoDO);
	if (!CheckError(errorCode)) return FALSE;
	return TRUE;
}
BOOL CDiDo::JieTongCmd(unsigned char bValue)
{
	unsigned char portData = 0;
	Automation::BDaq::int32 const rportStart = 0;
	Automation::BDaq::int32 const rportCount = 1;
	Automation::BDaq::ErrorCode rerrorCode = m_instantDoCtrl->Read(rportStart, rportCount, &portData);
	bValue = static_cast<unsigned char>(portData | bValue);
	WriteValue(bValue);
	return TRUE;
}
BOOL CDiDo::WriteValue(unsigned char bValue)
{
 	// Step 3: Scan the ports
	Automation::BDaq::int32 const portStart = 0;
	Automation::BDaq::int32 const portCount = 1;
	Automation::BDaq::ErrorCode errorCode = m_instantDoCtrl->Write(portStart, portCount, &bValue);
	if (!CheckError(errorCode)) return FALSE;
	Sleep(30);
	return TRUE;
}
BOOL CDiDo::CheckError(Automation::BDaq::ErrorCode errorCode)
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
BOOL CDiDo::ReadValue(unsigned char &bValue)
 {
		 Automation::BDaq::int32 const portStart = 0;
		 Automation::BDaq::int32 const portCount = 1;
		 Automation::BDaq::ErrorCode errorCode = m_instantDoCtrl->Read(portStart, portCount, &bValue);
		 if (!CheckError(errorCode)) return FALSE;
		 return TRUE;
 }

BOOL CDiDo::DuankaiCmd(unsigned char bCmd)
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
BOOL CDiDo::ClosePCI1761(void)
{
	DuankaiCmd(CDiDo::A_Camera00_01_02_R0_0x01);

	DuankaiCmd(CDiDo::A_Camera01_20_21_R3_0x08);
	DuankaiCmd(CDiDo::A_Camera02_23_24_R4_0x10);
	DuankaiCmd(CDiDo::A_Camera07_25_26_R5_0x20);
	DuankaiCmd(CDiDo::A_Camera08_27_28_R6_0x40);

	DuankaiCmd(CDiDo::B_Camera03_20_21_R3_0x08);
	DuankaiCmd(CDiDo::B_Camera04_23_24_R4_0x10);
	DuankaiCmd(CDiDo::B_Camera05_25_26_R5_0x20);
	DuankaiCmd(CDiDo::B_Camera06_27_28_R6_0x40);

	return TRUE;
}

void CDiDo::SetOpenTrueOrCloseFalse(BOOL bflg)
{
	m_bOpenTrueOrCloseFalse = bflg;
}
BOOL CDiDo::StartMonitor(void)
{
 	m_bFlag = true;
	hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if(hEvent == INVALID_HANDLE_VALUE) return FALSE;
//	m_hThread = (HANDLE)_beginthreadex(NULL, 0, PreLoadThread, this, 0, NULL);
	return TRUE;
}
unsigned int WINAPI CDiDo::PreLoadThread(void* pParameter)
{
	CDiDo* pThis = (CDiDo*)pParameter;
	pThis->PreLoad();
//	_endthreadex(0);
	return 0;
}
void CDiDo::PreLoad()
{
	while(m_bFlag)
	{
		WaitForSingleObject(hEvent, TIMEINTERVAL);

		if (m_bOpenTrueOrCloseFalse)
		{
			//�Ͽ��̵���
		//	int
		}
		else
		{
			//�պϼ̵���
		}

	 
	}
}

void CDiDo::get1761xuhao()
{
	Automation::BDaq::Array<Automation::BDaq::DeviceTreeNode>* sptedDevices = m_instantDoCtrl->getSupportedDevices();
	if (sptedDevices->getCount() == 0)
	{
		return;
 	}
	else 
	{
		int deviceCount = 0;
		for (int i = 0; i < sptedDevices->getCount(); ++i)
		{
			Automation::BDaq::DeviceTreeNode const & node = sptedDevices->getItem(i);
			qDebug("%d, %s\n", node.DeviceNumber, node.Description);

			Automation::BDaq::DeviceInformation devInfo(node.DeviceNumber);
		 
 
		}
	 
	}
 

 
}
