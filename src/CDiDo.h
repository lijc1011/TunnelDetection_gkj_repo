#pragma once
//#include "bdaqctrl.h"
#include "Globals.h" 


// avoid importing entire std namespace in headers to prevent symbol conflicts (e.g. std::byte vs global byte)
//using namespace Automation::BDaq;
#define TIMEINTERVAL 10

class CDiDo
{
public:
	enum cmd
	{	
		A_Camera00_01_02_R0_0x01 = 0x01,//���5
		A_Camera00_04_05_R1_0x02 = 0x02,//���6
		A_Camera01_20_21_R3_0x08 = 0x08,//���1
		A_Camera02_23_24_R4_0x10 = 0x10,//���2
		A_Camera07_25_26_R5_0x20 = 0x20,//���3
		A_Camera08_27_28_R6_0x40 = 0x40,//���4

		A_AType_7_8_R2_0x04 = 0x04,
		A_CType_10_11_R7_0x80 = 0x80,

		B_Camera03_20_21_R3_0x08 = 0x08,
		B_Camera04_23_24_R4_0x10 = 0x10,
		B_Camera05_25_26_R5_0x20 = 0x20,
		B_Camera06_27_28_R6_0x40 = 0x40,

 		B_AType_7_8_R2_0x04 = 0x04,
 		B_CType_10_11_R7_0x80 = 0x80
	};
public:
	CDiDo(void);//1����
	~CDiDo(void);

public:
    BOOL InitDiDo(int devnum);
	BOOL WriteValue(unsigned char bValue);//3��������
	BOOL ReadValue(unsigned char &bValue);//

	BOOL JieTongCmd(unsigned char bValue);//AC�̵������������ͨ

	BOOL DuankaiCmd(unsigned char bCmd);//AC�̵�����������Ͽ�
	BOOL ClosePCI1761(void);

	void SetOpenTrueOrCloseFalse(BOOL bflg);
 
	BOOL m_bFlag;
	HANDLE m_hThread;
	HANDLE hEvent;
	BOOL StartMonitor(void);
 	static unsigned int WINAPI PreLoadThread(void* pParameter);
	void PreLoad();
	 

	void get1761xuhao();
private:
	Automation::BDaq::InstantDoCtrl * m_instantDoCtrl;
	BOOL CheckError(Automation::BDaq::ErrorCode errorCode);
	BOOL m_bOpenTrueOrCloseFalse;
	 
};

