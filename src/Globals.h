#include <QMainWindow>
#include <QScreen>
#include <QSettings>
#include <QToolBar>
#include <QMenu>
#include <QStatusBar>
#include <QAction>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QButtonGroup>
#include <QFile>
#include <QMessageBox>
#include <QTimer>
#include <QObject>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStorageInfo>
#include <QHeaderView>



#include <iostream>                 // 解决 std::cout 报错
#include <filesystem>               // 解决 std::filesystem::current_path 报错
#include <string>
#include <vector>
#include <array>     
#include <thread>
#include <mutex>
#include <atomic>
#include <numeric>
#include <algorithm>
#include <cmath>

#include <opencv2\opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "DataType.h"

#include "tinyxml.h"
#include "tinystr.h"

#include "SapClassBasic.h"

#include "bdaqctrl.h"

#include "image_encryptor.hpp"

using namespace cv;
//using namespace std;

extern program_temp_struct gprogram_tempInfor; // 程序临时配置信息
extern program_set_struct g_program_setInfor; // 程序配置参数
extern CAMERAINFORSTRUCT g_cameraInfor[MAXCAMNUM]; // 相机参数
extern std::vector<tunnel_data_struct> gtunnel_ledger_struct; // 隧道台账信息

extern bool g_isConnect[MAXCAMNUM]; // 相机是否连接
extern bool g_isShow[MAXCAMNUM];    // 相机是否刷新显示
extern std::atomic<bool> g_isSave[MAXCAMNUM]; // 相机是否存储数据

// 自动亮度自适应总开关：true=自动亮度(启用AE)，false=手动亮度(默认)。
// 关闭时采集/显示/存储流程与未加此功能时逐字节一致，见 QShowThd 热路径的 if 门控。
extern std::atomic<bool> g_autoBrightnessEnabled;

extern double m_dmil_GYK; // GYK里程
extern int m_dspeed_GYK;  // GYK速度
extern double m_dmil_GYK_Cor; // GYK修正后的里程

void writeToXmlElement(TiXmlElement *itemElement, const char *childNm, const char *childText);
TiXmlElement *getXmlElement(TiXmlElement *itemElement, const char *nm);
template <typename T>std::string toString(const T &t); // 加上 std::

QStringList kuohao_to_list(QString str);
void kuohao_to_list(QString str, QStringList &xStringList, QStringList &yStringList);

imgName_struct Get_imgName_infor(QString imgName);
double k_mil_to_km(QString sMil);
