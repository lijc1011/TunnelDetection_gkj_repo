#pragma once
#define MAXCAMNUM 10 // 最大连接相机数量
#define RouteInfoSize 256
#define MAXFRAME 16 // 2K128,4K256

#define NODETECTION 0       // 未检测
#define BEINGDETECTION 1    // 正在检测
#define COMPLETEDETECTION 2 // 已检测

#define FRAME_TIMESTAMP_DEVICE_VALID 0x00000001
#define FRAME_TIMESTAMP_HOST_VALID 0x00000002
#define FRAME_TIMESTAMP_AUXILIARY_VALID 0x00000004
#define PICTURE_INDEX_MAGIC 0x58444950
#define PICTURE_INDEX_VERSION_WITH_TIMESTAMPS 2

////临时配置信息结构体//////////////////////////////////////////////////////////////////////
typedef struct program_temp_struct
{
    QString tmp_path;            // 临时配置文件路径
    QString tmp_file;            // 临时配置文件名称
    QString tmp_path_file;       // 隧道简表文件全路径
    QString tmp_path_file_sound; // 声屏障简表文件全路径

    program_temp_struct()
    {
    }
} program_temp_struct, *pprogram_temp_struct;

////程序配置信息结构体//////////////////////////////////////////////////////////////////////
typedef struct program_set_struct
{
    QString ObjectType;  // 检测对象类型，用于设置界面，取值：隧道，无砟轨道，等
    int ZhuCong;         // 主机或从机
    QString ZhuServerIp; // 主机IP

    int Pci1710Num; //
    int Pci1761Num; //
    int Pci1784Num; //
    int onoff1784;  // 无砟轨道小车使用1784控制激光，1起作用
    double PulseWidth;

    QString DataDestPath;  // 数据存储默认路径
    int MilType;           // 里程定位厂家，0指的是GYK，1指的是朔黄随检车
    int MilComPort;        // GYK串口号
    int MilComBaud;        // GYK串口波特率
    int CameraTotalNumber; // 本系统相机总数量
    int ViewTotalNumber;   // 本系统视野通道总数量
    int CameraNumber;      // 相机数量
    int CameraType;        // 相机接口类型,0是网口，1是CamLink口
    int UncoverTime;       // 防水盖动作时长
    int ShowThdTime;       // 显示线程刷新周期，默认10ms，可以增加以降低显示所占用的系统资源
    int rukouThreshold;    // GYK里程和检测进口比较时的阈值
    int chukouThreshold;   // GYK里程和检测出口比较时的阈值

    int GYKPosValue;        // GYK设在车辆的位置，修正GYK里程
    int GYK0_or_COR1;       // 使用GYK里程，还是优化后的里程进行出入口判断
    int DebugTimmerSwitch;  // 调试时，模拟GYK里程的开关
    double DebugStartMil;   // 调试时，初始里程
    int DebugMilInc1Dec0;   // 调试时，里程增1减0
    int DebugMilStepLength; // 调试时，里程变化步长

    QString danxian;           // 检测单线隧道，所有相机工作，与走形行别，面向大小里程无关
    QString shuang_xia_da;     // 检测双线隧道，走下行，面向大里程
    QString shuang_xia_xiao;   // 检测双线隧道，走下行，面向小里程
    QString shuang_shang_da;   // 检测双线隧道，走上行，面向大里程
    QString shuang_shang_xiao; // 检测双线隧道，走上行，面向小里程

    QString xia_da_zuo;     // 检测左侧声屏障，走下行，面向大里程
    QString xia_xiao_zuo;   // 检测左侧声屏障，走下行，面向小里程
    QString shang_da_you;   // 检测右侧声屏障，走上行，面向大里程
    QString shang_xiao_you; // 检测右侧声屏障，走上行，面向小里程

    QString pictureWH; // 图片宽高

    int Set_show_WH;   // 手动设置图像显示区域的开关，1开0关
    int Set_show_W;    // 宽度
    int Set_show_H;    // 高度
    int Table_rows;    // 表格行数
    int big_mil_onoff; // 是否打开异常里程屏蔽功能，1开0关
    int mil_threshold; // 最大里程异常值，比如朔黄可设置成250
    int SaveZarr;           // 存储格式：1=Zarr v3(TensorStore)，0=旧 camdatx(fwrite)
    int ZarrVerifyOnClose;  // Zarr 收口前抽样回读：1开启，0关闭
    int ZarrMinFreeGB;      // 开始写入及写入过程允许的最小剩余空间（GB）
    int ZarrNormalize;      // 图像转正开关：1按现场规则转正，0严格按相机原始方向写 Zarr
    QString ZarrMachineId;  // 转正规则机器编号：A/B；raw 模式可为空
    program_set_struct()
    {
        ZhuCong = 0;
        Pci1710Num = 0;
        Pci1761Num = 0;
        Pci1784Num = 0;
        onoff1784 = 0;
        PulseWidth = 0;
        MilType = 0;
        MilComPort = 1;
        MilComBaud = 9600;
        CameraNumber = 4;
        CameraType = 0;
        UncoverTime = 10;
        danxian = "(1,0)(2,0)(3,0)(4,0)(5,0)(6,0)(7,0)(8,0)(9,0)(10,0)";
        shuang_xia_da = "(1,0)(2,0)(3,0)(4,0)(5,0)(6,0)(7,0)(8,0)(9,0)(10,0)";
        shuang_xia_xiao = "(1,0)(2,0)(3,0)(4,0)(5,0)(6,0)(7,0)(8,0)(9,0)(10,0)";
        shuang_shang_da = "(1,0)(2,0)(3,0)(4,0)(5,0)(6,0)(7,0)(8,0)(9,0)(10,0)";
        shuang_shang_xiao = "(1,0)(2,0)(3,0)(4,0)(5,0)(6,0)(7,0)(8,0)(9,0)(10,0)";
        pictureWH = "(4096,4096)(4096,4096)(4096,4096)(4096,4096)(4096,4096)(4096,4096)(4096,4096)(4096,4096)(0,0)(0,0)";
        Table_rows = 1000;
        big_mil_onoff = 0;
        mil_threshold = 0;
        SaveZarr = 0;
        ZarrVerifyOnClose = 1;
        ZarrMinFreeGB = 10;
        ZarrNormalize = 0;
        ZarrMachineId = "";
    }
} program_set_struct, *pprogram_set_struct;

////相机信息结构体//////////////////////////////////////////////////////////////////////
typedef struct CAMERAINFORSTRUCT
{
    QString Name;        // 相机名称
    QString CamLinkName; // CamLink相机名称
    int ExposureMode;    // 曝光模式
    double ExposureTime; // 曝光时间
    int PictureHeight;   // 图片高度
    int ImageHeight;     // 图像高度
    int ImageWidth;      // 图像宽度
    int TriggerMode;     // 触发模式,1外触发，0内触发
    int LineFormat;      // LVDS信号
    int LineFreq;
    int IsFlip;
    int FlipMode;
    int SaveTransform;   // 旧 Zarr 原型遗留值；ZarrNormalize=0 时不应用，避免改变原始方向
    int Gain;
    int isSingle;      // 标识相机是否用于检测单线隧道
    int isDouble;      // 标识相机是否用于检测双线隧道
    int isColorCamera; // 是否是彩色相机，1是彩色，0是黑白
    int isWork;        // 标识相机是否工作

    int ExposureTimeMin; // 最小曝光时间5us
    int ExposureTimeMax; // 最大曝光时间30us
    int GainMin;         // 最小增益1
    int GainMax;         // 最大增益3

    // 软件自动亮度参数。每个cameraXX独立保存；旧Config缺少字段时使用初始经验范围。
    // 这些字段只在“自动亮度”模式读取，手动模式完全不使用。
    double AutoBrightnessMin;
    double AutoBrightnessMax;
    double AutoBrightnessMaxStepRatio;
    double AutoBrightnessTrimPercent;
    int AutoBrightnessInterval;

    // 	char cCamDataName[RouteInfoSize];  //相机数据文件名称
    // 	char cCamIndexName[RouteInfoSize];  //相机索引文件名称
    // 	char cCameraName[RouteInfoSize];  //相机名称

    __int64 iSizeFrame;

    double dPixPrecision;

    int iReserve1;
    int iReserve2;
    double dReserve1;
    double dReserve2;
    char cReserve[RouteInfoSize];

    CAMERAINFORSTRUCT()
    {
        // 		memset(cCamDataName, 0, RouteInfoSize);
        // 		memset(cCamIndexName, 0, RouteInfoSize);
        // 		memset(cCameraName, 0, RouteInfoSize);
        ExposureMode = 1;   // 曝光模式
        ExposureTime = 100; // 曝光时间
        PictureHeight = 0;  // 图片高度
        ImageHeight = 0;    // 图像高度
        ImageWidth = 0;     // 图像宽度
        LineFormat = 3;     // LVDS信号
        iSizeFrame = 0;     // 一帧图像大小
        isColorCamera = 0;  // 默认是黑白相机
        isWork = 1;

        AutoBrightnessMin = 75.0;
        AutoBrightnessMax = 175.0;
        AutoBrightnessMaxStepRatio = 0.08;
        AutoBrightnessTrimPercent = 5.0;
        AutoBrightnessInterval = 1;

        IsFlip = 0;
        FlipMode = 0;
        SaveTransform = 0;
        iReserve1 = 0;
        iReserve2 = 0;
        dReserve1 = 0;
        dReserve2 = 0;
        memset(cReserve, 0, RouteInfoSize);
    }
} CAMERAINFORSTRUCT;

////设备（隧道/声屏障）信息结构体//////////////////////////////////////////////////////////////////////
typedef struct tunnel_data_struct
{
    int ixh;
    QString object_type;   // 检测对象类型：隧道、声屏障
    QString bureau_name;   // 局名_新增
    QString division_name; // 段名_新增
    QString section_name;  // 区间_新增

    QString lineName;      // 线路名称
    QString lineLevel;     // 线路等级
    QString tunnelName;    // 隧道名称，改为设备名称
    QString tunnelType;    // 隧道单双线   0单线，1双线；声屏障指的是左右侧：左侧，右侧
    QString tunnelXingbie; // 隧道行别,0上行，1下行，2上下行
    QString tunnelCode;    // 隧道编码，改为设备编码
    QString tunnelNumber;  // 隧道号，改为设备号
    QString tunnelInType;  // 隧道进口类型
    QString tunnelOutType; // 隧道出口类型

    double tunnel_startMil;  // 起始里程
    double tunnel_endMil;    // 终止里程
    double tunnel_centreMil; // 中心里程
    int tunnel_length;       // 长度 //以前是int，有2位小数的情况
    double object_height;    // 高度

    QString equipment_name; // 检测设备,这两个采集程序增加的，地面数据处理程序没有
    QString person_name;    // 检测人员

    int iX_Line_cnt; // 沿轨道方向扫描线总数 ，从detection_data_struct读回，计算，不体现在xml里，只给展开图用
    int iY_Line_cnt; // 沿隧道衬砌表面环向方向，扫描线总数从detection_data_struct读回，计算，不体现在xml里，只给展开图用

    int status_flag; // 0表示未检测，1表示正在检测，2表示检测完（检测指的是动态里程是否经过，不是真正存储数据的意思）
    tunnel_data_struct()
    {
        ixh = 0;
        status_flag = 0;
    }
} tunnel_data_struct, *ptunnel_data_struct;

typedef unsigned int UINT; // 加上这一行，下面就不用改了
typedef struct MILEAGEINFORSTRUCT
{
    __int64 iPulseCount;
    double fMileage;
    double fSpeed;
    UINT uiPositiveOrNegative;
    UINT uiIncOrDec;

    double fMileageStart;
    UINT uiIncOrDecMil;

    int iReserve;
    double dReserve;

    MILEAGEINFORSTRUCT()
    {
        fMileageStart = 0;
        iPulseCount = 0;
        fMileage = 0;
        fSpeed = 0;
        uiPositiveOrNegative = 0;
        uiIncOrDec = 0;
        uiIncOrDecMil = 0;
        iReserve = 0;
        dReserve = 0;
    }
} MILEAGEINFORSTRUCT, *pMILEAGEINFORSTRUCT;


typedef struct FRAMEIMAGESTRUCT
{
    unsigned char *pDataPoint;

    MILEAGEINFORSTRUCT mil;
    int iHeight;        // 一帧高度
    int iWeight;        // 一帧宽度
    __int64 iSizeFrame; // 一帧大小
    bool isShow;        // 用于标识是否已显示过

    __int64 timems;         // 每256条线1个时间戳
    unsigned __int64 deviceTimestamp;
    unsigned __int64 hostTimestamp;
    unsigned __int64 auxiliaryTimestamp;
    UINT timestampFlags;
    double dHFreqPrecision; // 每256条线1个行频
    __int64 pulseCount;     // 每256条线1个脉冲数
    double gykMil;          // 每256条线1个GYK里程

    FRAMEIMAGESTRUCT()
    {
        pDataPoint = NULL;
        timems = 0;
        deviceTimestamp = 0;
        hostTimestamp = 0;
        auxiliaryTimestamp = 0;
        timestampFlags = 0;
        iHeight = 0;
        iWeight = 0;
        iSizeFrame = 0;
        isShow = false;
    }

} FRAMEIMAGESTRUCT, *pFRAMEIMAGESTRUCT;

typedef struct PICTUREINFORSTRUCT
{
    MILEAGEINFORSTRUCT mil;
    int iHeight;          // 一幅高度
    int iWeight;          // 一幅宽度
    __int64 iSizePicture; // 一幅大小
    int iFrameNum;        // 一幅帧数MAXFRAME=16

    UINT iPicCnt;
    __int64 startPosition;
    __int64 recordDimension;

    int iLeftLine;
    int iRightLine;
    int iUpLine;
    int iDownLine;
    int iIsValid;
    int iMilLongOrShort;

    __int64 timems[MAXFRAME];         // 每幅图有16个时间戳
    double dHFreqPrecision[MAXFRAME]; // 每幅图有16个行频
    __int64 pulseCount[MAXFRAME];     // 每幅图有16个脉冲数
    double gykMil[MAXFRAME];          // 每幅图有16个GYK里程

    UINT indexMagic;
    UINT indexVersion;
    UINT indexRecordSize;
    UINT timestampExtensionSize;
    unsigned __int64 deviceTimestamp[MAXFRAME];
    unsigned __int64 hostTimestamp[MAXFRAME];
    unsigned __int64 auxiliaryTimestamp[MAXFRAME];
    UINT timestampFlags[MAXFRAME];

    PICTUREINFORSTRUCT()
    {
        memset(&mil, 0, sizeof(MILEAGEINFORSTRUCT));
        iHeight = 0;
        iWeight = 0;
        iSizePicture = 0;
        iFrameNum = MAXFRAME;

        iPicCnt = 0;
        startPosition = 0;
        recordDimension = 0;

        iLeftLine = 0;
        iRightLine = 0;
        iUpLine = 0;
        iDownLine = 0;
        iIsValid = 0;
        iMilLongOrShort = 0;
        indexMagic = PICTURE_INDEX_MAGIC;
        indexVersion = PICTURE_INDEX_VERSION_WITH_TIMESTAMPS;
        indexRecordSize = sizeof(PICTUREINFORSTRUCT);
        timestampExtensionSize = sizeof(deviceTimestamp) + sizeof(hostTimestamp) + sizeof(auxiliaryTimestamp) + sizeof(timestampFlags);

        for (int i = 0; i < MAXFRAME; i++)
        {
            timems[i] = 0;
            dHFreqPrecision[i] = 0;
            pulseCount[i] = 0;
            gykMil[i] = 0;
            deviceTimestamp[i] = 0;
            hostTimestamp[i] = 0;
            auxiliaryTimestamp[i] = 0;
            timestampFlags[i] = 0;
        }
    }
} PICTUREINFORSTRUCT, *pPICTUREINFORSTRUCT;

typedef struct PICTUREIMAGESTRUCT
{
    unsigned char *pDataBuffer;
    __int64 iBufferSize;
    PICTUREINFORSTRUCT picInfor;

    PICTUREIMAGESTRUCT()
    {
        pDataBuffer = NULL;
        iBufferSize = 0;
    }
} PICTUREIMAGESTRUCT, *pPICTUREIMAGESTRUCT;

typedef struct imgName_struct
{
    QString object_Type;   // 检测对象类型，隧道，声屏障
    QString bureau_name;   // 路局名称
    QString line_name;     // 线路名称
    QString xingbie;       // 行别
    QString start_station; // 起始站名
    QString end_station;   // 终到站名
    QString set_time;      // 时间，年月日20240510
    QString com_flg;       // 计算机标识01,02,03
    QString file_Suffix;   // 后缀

    imgName_struct()
    {
    }
} imgName_struct, *pImgName_struct;

#if 0
////检测信息结构体//////////////////////////////////////////////////////////////////////
typedef struct JIANCEINFORSTRUCT
{
	char cLineName[RouteInfoSize];  //线路名称
	char cTunnelName[RouteInfoSize];
	int iTunnelType;
	int iLineDir;
	int	iRunDir;
	int iMileageChange;
	int iWorkMod;

	int iReserve1;
	int iReserve2;
	double dReserve1;
	double dReserve2;
	char cReserve[RouteInfoSize];

	JIANCEINFORSTRUCT()
	{
		memset(cLineName, 0, RouteInfoSize);
		memset(cTunnelName, 0, RouteInfoSize);
		iTunnelType = 0;
		iLineDir = 1;
		iRunDir = 1;
		iMileageChange = 0;
		iWorkMod = WORKMOD0;

		iReserve1 = 0;
		iReserve2 = 0;
		dReserve1 = 0;
		dReserve2 = 0;
		memset(cReserve, 0, RouteInfoSize);

	}
}JIANCEINFORSTRUCT, *pJIANCEINFORSTRUCT;
 
typedef struct LOCALINFORSTRUCT
{
	int iComputerSn;
	int iZhuBei;//没有用
	int iPci1710Num;
	int iPci1761Num;
	int iPci1784Num;
	char cServerIp[RouteInfoSize];  //主控端ip
	char cDestPath[RouteInfoSize];  //数据存储路径

	int iReserve1;
	int iReserve2;
	double dReserve1;
	double dReserve2;
	char cReserve[RouteInfoSize];  //数据存储路径


	LOCALINFORSTRUCT()
	{
 		iComputerSn = 0;
		iZhuBei = 1;
		iPci1710Num = 1;
		iPci1761Num = 1;
		iPci1784Num = 1;
		memset(cServerIp,0,RouteInfoSize);
		memset(cDestPath,0,RouteInfoSize);

		iReserve1 = 0;
		iReserve2 = 0;
		dReserve1 = 0;
		dReserve2 = 0;
	    memset(cReserve,0,RouteInfoSize);
	}
}LOCALINFORSTRUCT,*pLOCALINFORSTRUCT;





typedef struct DATAINFORSTRUCT
{
	int iStartLineCount;
	int iPreImgNum;

	char cReserve1[RouteInfoSize];  //线路名称
	char cReserve2[RouteInfoSize];   
	 
	int iReserve1;
	int iReserve2;
	int iReserve3;
	int iReserve4;
	double dReserve1;
	double dReserve2;
	double dReserve3;
	double dReserve4;
	 

	DATAINFORSTRUCT()
	{
		iStartLineCount = 0;
		iPreImgNum = 0;
		memset(cReserve1,0,RouteInfoSize);
		memset(cReserve2,0,RouteInfoSize);
		 

		iReserve1 = 0;
		iReserve2 = 0;
		iReserve3 = 0;
		iReserve4 = 0;
		dReserve1 = 0;
		dReserve2 = 0;
		dReserve3 = 0;
		dReserve4 = 0;
	 

	}
}DATAINFORSTRUCT,*pDATAINFORSTRUCT;


typedef struct OFFSETINFORSTRUCT
{
	MILEAGEINFORSTRUCT mil;
	double leftData;
	double rightData;

	USHORT usValue;
	double fMileage;
	double fSpeed;

	double fMileageStart;
	UINT uiIncOrDecMil;

	int iReserve;
	double dReserve;

	OFFSETINFORSTRUCT()
	{
		usValue = 0;
		fMileage = 0;
		fMileage = 0;
		fSpeed = 0;
		fMileageStart = 0;
		uiIncOrDecMil = 0;
		iReserve = 0;
		dReserve = 0;

	}
}OFFSETINFORSTRUCT,*pOFFSETINFORSTRUCT;
#endif
