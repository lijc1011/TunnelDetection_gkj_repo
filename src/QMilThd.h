#ifndef QMILTHD_H
#define QMILTHD_H

#include "Globals.h"
#include "DataType.h"

//#include	"Lock.h"
// avoid importing entire std namespace in headers to prevent symbol conflicts (e.g. std::byte vs global byte)
#define TIMEINTERVAL 10


class QMilThd : public QObject
{
	Q_OBJECT

public:

	enum cmd
	{
		A_Camera01_20_21_R3_0x08 = 0x08,//���1
		A_Camera02_23_24_R4_0x10 = 0x01,//���2
		A_Camera03_25_26_R5_0x20 = 0x04,//���3
		A_Camera04_27_28_R6_0x40 = 0x02,//���4

	};

	QMilThd(QObject *parent);
	~QMilThd();
public:
	BOOL SetDevNum(int iNum, double dUnitMil);
	BOOL InitMil(void);
	BOOL StartCount(void);//3��������
	BOOL StopCount(void);
	MILEAGEINFORSTRUCT GetMileageInfor(void);//4��ȡ�����Ϣ
	void SetUnitMil(float fValue);//2���õ�λ����
	void SetZero(void);
	void SetNowMileage(double dValue);
	void SetZengjiaOrJianshao(int dValue);

	void SetCorrectionfactor(double dFactor);

private:
	int m_iDevNum;
	MILEAGEINFORSTRUCT m_mileageInfor;
	std::mutex m_milMutex;
	double m_fUnitMil;
	int m_iPulsSave;
	clock_t m_iTimeSave;
	double m_fSetMileage;
	double m_fFactor;
	int m_iZengOrJian;

private:
	Automation::BDaq::UdCounterCtrl* m_udCounterCtrl;
	Automation::BDaq::InstantDoCtrl * m_instantDoCtrl;
	BOOL CheckError(Automation::BDaq::ErrorCode errorCode);


	bool m_isBreakThd;
	bool m_isCanRun;
	std::thread genOrgThreads;
	static void runThdFun(QMilThd* pParameter);
	void sub_runThdFun();
	std::mutex my_lock;

	
private:
	
public:
    bool InitDiDo(int devnum);
	bool WriteValue(unsigned char bValue);//3��������
	bool ReadValue(unsigned char &bValue);//
	bool JieTongCmd(unsigned char bValue);//AC�̵������������ͨ
	bool DuankaiCmd(unsigned char bCmd);//AC�̵�����������Ͽ�

};

#endif // QMILTHD_H
