#pragma once
#include "ui_TunnelDetection.h"
#include "Globals.h"
#include "QSimpleLed.h"
#include "QSetMileageDlg.h"
#include "QRrealTimeState.h"
#include "QInitThd.h"
#include "QShowThd.h"
#include "QSaveThd.h"
#include "QSetLightDlg.h"
#include "QSetCameraDlg.h"
#include "QSetParaDlg.h"
#include "QMilThd.h"

class TunnelDetection : public QMainWindow {
    Q_OBJECT
    
public:
    TunnelDetection(QWidget* parent = nullptr);
    ~TunnelDetection();

    /////动作&菜单栏&工具栏/////////////////////////////////////////////////////////////////////
    void createActions();
    void createMenus();
    void createToolbars();
    void createStatusBar();

    void ReadDefaultPara();
	void WriteDefaultPara();

    QMenu *myLoadMenu;         // 加载菜单栏
    QToolBar *loadToolBar;     // 加载工具栏
    QAction *loadTunnelAction; // 加载隧道简表

    QMenu *moduleMenu;           // 设置菜单栏
    QToolBar *moduleToolBar;     // 设置工具栏
    QAction *setMileageAction;   // 设置里程信息
    QAction *cameraAction;       // 相机
    QAction *lightAction;        // 照明
    QAction *limitAction;        // 限界
    QAction *compensationAction; // 补偿
    QAction *monitorAction;      // 监控相机
    QAction *dataRealTimeAction; // 数据实时状态
    QAction *paraAction;         // 设置程序参数

    QMenu *mysystemMenu;       // 系统菜单栏
    QToolBar *systemToolBar;   // 系统工具栏
    QAction *autoModelAction;  // 自动模式
    QAction *connectAction;    // 连接（相机等各个模块需要手动出发命令进行连接）
    QAction *disConnectAction; // 断开连接
    QAction *startAcqAction;   // 开始采集
    QAction *stopAcqAction;    // 停止采集

    QMenu *myhelpMenu;       // 帮助菜单栏
    QAction *viewHelpAction; // 查看帮助

    QStatusBar *statusbar; //
    QLabel *msgLable;

    QTimer *timerprogress_diskcapacity;
    QTimer *timerprogress_gykMil_accuracy;
    QTimer *timerprogress_auto;
    QTimer *timerprogress_gykMil;
    QTimer *timerprogress;
    QSimpleLed *camLed[MAXCAMNUM];

    QMilThd *m_milDirect; // 脉冲计数里程获取线程
    QSetMileageDlg *pSetMileageDlg; // 设置里程对话框
    QSetCameraDlg *pSetCameraDlg; // 设置相机参数对话框
    QSetLightDlg *pSetLightDlg;   // 设置照明和防水盖对话框
    QSetParaDlg *pSetParaDlg;     // 设置程序参数对话框
    QInitThd *m_QInitThd; // 初始化线程，未使用

    QShowThd *m_QShowThd; // 图像显示线程
    QSaveThd *m_pSaveImg[MAXCAMNUM];
    QRrealTimeState *pRrealTimeState;

     QString m_bureau_name; // 路局名称
     QString m_strLineName; // 线路名称
     QDateTime m_dateTime;  // 检测时间目录文件夹,和生成相机文件的时间
     QDateTime m_dateTime_ledgerFile; // 检测台账文件名的时间
     QString m_strXB;                 // 车辆运行的行别
     QString m_strMilDir;   // 里程增减
     QString m_strChetou;   // 车头朝向
     
     QString m_strStartMil; // 人为设置的起始里程
     QString m_strEndMil;   // 人为设置的终止里程

     imgName_struct m_ledgerFileName; // 台账表文件名
     QString m_csvPathFileName; // csv文件全路径名称，用于记录自动加载
     int iCurIndex;             // 待检测或正在检测的隧道索引
     std::vector<int> vec_curCamera; // 工作相机索引队列,比如上行双线工作相机为1、2，单线为1、2、5、6
     void loadledgerFile(QString filename);
     QLabel *labelShowImg[MAXCAMNUM];
     QLabel *Label_camName[MAXCAMNUM];   // 相机名称
     QLabel *label_isConnect[MAXCAMNUM]; // 是否连接
     QLabel *label_isSave[MAXCAMNUM];    // 是否正在存储数据
     QLineEdit *LineEdit_us[MAXCAMNUM];
     QLineEdit *LineEdit_gain[MAXCAMNUM];
     QLineEdit *LineEdit_smil;   // 人为设置的起始里程
     QLineEdit *LineEdit_emil;   // 人为设置的终止里程
     QLineEdit *LineEdit_cunchu; // 存储盘容量
     int m_tableWidget_cow;   // 列表控件的最大列数
     int m_tableWidget_width; // 列表控件的最小宽度

 public:
     void init_tableWidget();
     bool FindIniFile();
     void DefaultIniFile();
     bool LoadIniFile();

     void showListControl(std::vector<tunnel_data_struct> td_tmp);
     void get_tunnel_interval(); // 计算隧道间隔

     int icow_table_interval; // 间隔是第9列
     int icow_table_result;   // 检测状态结果是第10列
     int icow_table_smil;     // 开始存储里程是第11列
     int icow_table_emil;     // 停止存储里程是第12列

     //////////连接相机线程////////////////////////////////////////////////////////////////
     void startThd();
     void stopThd();
     void pauseThd();
     void disConnect();
     bool m_isBreakThd; // 线程是否中断
     bool m_isCanRun;
     std::thread genOrgThreads;
     static void runThdFun(TunnelDetection *pParameter);
     void sub_runThdFun();
     std::mutex my_lock;
     bool initCamera(int iSumNum);
     ICamera *m_pCamera[MAXCAMNUM];

     void startSaveImage(QString sdNumber = "1", QString sdName = "test"); // 开始存储数据
     void makeDestDir(QString strValue);
     void stopSaveImage(); // 开始存储数据

     //////////串口接收GYK里程////////////////////////////////////////////////////////////////
     QSerialPort *serialPort;
     void sp_gyk(void);
     void readData();
     void gyk_readData();
     QByteArray responseData;
     double m_dmil_GYK_his;
     __int64 m_pulse_save;
     double m_gyksave;
     double tmp_dmil_GYK;

     /////////自动模式///////////////////////////////////////////////////////////////
     int find_tunnel_idxII();                       //
     void get_active_camera(tunnel_data_struct td); // 判断工作相机
     void sub_get_active_camera(QString strtmp);
     void renew_active_camera(); // 激活相机标志位置零
     void dealjuli();            // 更新距下个隧道口的距离
     bool select_tunnel_flag;    // 查找待检隧道只进行1次
     bool save_start_flag;       // 存储数据只开始一次
     bool save_stop_flag;        // 存储数据只结束一次
     tunnel_data_struct m_td;    // 下一个要检测的隧道
     int m_idx; // 当前正在检测的隧道索引
     int m_idx_next; // 当前正在检测的隧道索引
     int m_count; // 已检测完毕的隧道计数
     bool openCsv();
     bool writeCsv();
     bool myflg; // 是否允许更新滚动条位置
     QFile m_csvLedgerFile;

     // 适配隧检小车，手动控制启停
     QMenu *originDataTableContextMenu;
     QAction *originDataTableActionDel;

 private slots:
     void loadTunnelActionSlot();   // 加载隧道简表槽函数
     void setMileageActionSlot();   // 设置里程信息的槽函数
     void cameraActionSlot();       // 配置相机模块
     void lightActionSlot();        // 配置照明模块
     void limitActionSlot();        // 配置限界模块
     void compensationActionSlot(); // 配置补偿模块
     void monitorActionSlot();      // 配置监控相机模块
     void paraActionSlot();         // 配置程序参数
     void dataRealTimeActionSlot(); // 数据实时状态
     void autoModelActionSlot();    // 自动模式槽函数
     void connectActionSlot();      // 连接动作槽函数
     void disConnectActionSlot();   // 断开连接动作槽函数
     void startAcqActionSlot();     // 开始采集槽函数
     void stopAcqActionSlot();      // 停止采集槽函数
     void viewHelpActionSlot();     // 槽函数

     void onRadioClickSlot();
     void onmilRadioClickSlot();
     void onchetouRadioClickSlot();
     void timerUpDateprogress();                   // 定时响应
     void timerUpDateGYKMilprogress();             // 模拟GYK里程测试用
     void timerUpDateAutoModeprogress();           // 自动模式
     void timerprogress_gykMil_accuracyprogress(); // GYK里程优化
     void timer_diskcapacity_progress();           // 硬盘容量实时更新

     void ShowThd_slots(QPixmap picQPixmap, int iChl); // 相机正在刷新显示的槽函数
     void save_thread_slots(int iChlV);                // 相机正在存储槽函数
     void stop_save_thread_slots(int iChlV);           // 相机停止存储槽函数
     void Mil_over_slots(QString ss, QString ee);      // 里程信息对话框设置完毕后的槽函数
     void Mil_time_slots(double dmil, double dspeed);  // 主界面更新脉冲计数的里程和速度信息  槽函数

     void auto_mode_slots(); // 自动模式下，更新列表控件
     void auto_start_acq_slots(int idx);
     void auto_stop_acq_slots(int idx);
     void auto_show_slots(int irow, QString strText);

     void tableContexMenuRequested(const QPoint &pos);
     void slotMenuClicked(QAction *act);

     void pushButton_GD_openzhaomingSlot();
     void pushButton_GD_closemingzhaoSlot();

 private:
     Ui::TunnelDetection *ui;
     void closeEvent(QCloseEvent *event);
};
