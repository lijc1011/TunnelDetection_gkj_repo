#include "QSetLightDlg.h"
#include "ui_QSetLightDlg.h"

QSetLightDlg::QSetLightDlg(QMilThd *milDetect, QWidget *parent)
    : QDialog(parent), ui(new Ui::QSetLightDlg)
{
    ui->setupUi(this);

    set_milDetect = milDetect;
    for (int i = 0; i < MAXCAMNUM; i++)
    {
        label_led[i] = NULL;
    }

    // 界面整体是上下结构
    QVBoxLayout *fullLayout = new QVBoxLayout();
    fullLayout->setContentsMargins(20, 20, 20, 20);
    fullLayout->setSpacing(20); // 上下边距

    // 1.上边是Hlayout,防水盖设置
    ui->upLayout->setContentsMargins(20, 20, 20, 20);
    ui->upLayout->setSpacing(20); // 上下边距

    ui->pushButton_openGaizi->setMinimumWidth(260);
    ui->pushButton_openGaizi->setMinimumHeight(80);
    connect(ui->pushButton_openGaizi, &QPushButton::clicked, this, &QSetLightDlg::pushButton_openGaizieSlot);
    ui->upLayout->addWidget(ui->pushButton_openGaizi);

    ui->pushButton_closeGaizi->setMinimumWidth(260);
    ui->pushButton_closeGaizi->setMinimumHeight(80);
    connect(ui->pushButton_closeGaizi, &QPushButton::clicked, this, &QSetLightDlg::pushButton_closeGaiziSlot);
    ui->upLayout->addWidget(ui->pushButton_closeGaizi);
    ui->upLayout->addStretch(1);

    ui->pushButton_GD_openzhaoming->setMinimumWidth(260);
    ui->pushButton_GD_openzhaoming->setMinimumHeight(80);
    connect(ui->pushButton_GD_openzhaoming, &QPushButton::clicked, this, &QSetLightDlg::pushButton_GD_openzhaomingSlot);
    ui->upLayout->addWidget(ui->pushButton_GD_openzhaoming);
    ui->upLayout->addStretch(1);

    ui->pushButton_GD_closemingzhao->setMinimumWidth(260);
    ui->pushButton_GD_closemingzhao->setMinimumHeight(80);
    connect(ui->pushButton_GD_closemingzhao, &QPushButton::clicked, this, &QSetLightDlg::pushButton_GD_closemingzhaoSlot);
    ui->upLayout->addWidget(ui->pushButton_GD_closemingzhao);
    ui->upLayout->addStretch(1);
    if (g_program_setInfor.onoff1784 == 0)
    {
        ui->pushButton_GD_openzhaoming->setEnabled(false);
        ui->pushButton_GD_closemingzhao->setEnabled(false);
    }
    else if (g_program_setInfor.onoff1784 == 1)
    {
        ui->pushButton_GD_openzhaoming->setEnabled(true);
        ui->pushButton_GD_closemingzhao->setEnabled(true);
    }

    ui->groupBox_fangshuigai->setLayout(ui->upLayout);
    fullLayout->addWidget(ui->groupBox_fangshuigai);

    // 2.中间是Hlayout,分4段，看相机数量
    ui->downLayout->setContentsMargins(20, 20, 20, 20);
    ui->downLayout->setSpacing(20); // 上下边距
    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        // gridlayout，每个相机的照明控制
        QGridLayout *Layout_cam = new QGridLayout();
        Layout_cam->setContentsMargins(0, 0, 0, 0);
        Layout_cam->setContentsMargins(20, 20, 20, 20);
        Layout_cam->setSpacing(20); // 上下边距

        // 相机名称
        QString strshow = QStringLiteral("相机：");
        int CPos = g_cameraInfor[i].Name.indexOf('C', 0);
        strshow += g_cameraInfor[i].Name.mid(0, CPos);

        QLabel *label_cn = new QLabel(strshow);
        Layout_cam->addWidget(label_cn, 0, 0);
        label_cn->setAlignment(Qt::AlignCenter);

        QPushButton *open_zhaoming = new QPushButton(QStringLiteral("打开照明"));
        open_zhaoming->setObjectName(QString::number(i, 10));
        connect(open_zhaoming, &QPushButton::clicked, this, &QSetLightDlg::open_zhaoming_Slots);
        Layout_cam->addWidget(open_zhaoming, 1, 0, 1, 2);
        open_zhaoming->setMinimumWidth(120);
        open_zhaoming->setMinimumHeight(40);

        QPushButton *close_zhaoming = new QPushButton(QStringLiteral("关闭照明"));
        close_zhaoming->setObjectName(QString::number(i, 10));
        connect(close_zhaoming, &QPushButton::clicked, this, &QSetLightDlg::close_zhaoming_Slots);
        Layout_cam->addWidget(close_zhaoming, 2, 0, 1, 2);
        close_zhaoming->setMinimumWidth(120);
        close_zhaoming->setMinimumHeight(40);

        // led
        QString strled = strshow;
        strled += ("led");
        label_led[i] = new QLabel(strled);
        Layout_cam->addWidget(label_led[i], 0, 1);
        label_led[i]->setAlignment(Qt::AlignCenter);
        setLED(label_led[i], 2, 40); // color 0:grey 1:red 2:green 3:yellow
        ui->downLayout->addLayout(Layout_cam);
    }

    ui->groupBox_light->setLayout(ui->downLayout);
    fullLayout->addWidget(ui->groupBox_light);

    this->setLayout(fullLayout);

    m_dido.InitDiDo(g_program_setInfor.Pci1761Num);
    freshLedStatus();
    get1761xuhao();
}

QSetLightDlg::~QSetLightDlg()
{
    delete ui;
}

//////防水盖动作////////////////////////////////////////////////////////////////////
void QSetLightDlg::pushButton_openGaizieSlot()
{
    m_isStartOrStop = true;
    startThd();
}
void QSetLightDlg::pushButton_closeGaiziSlot()
{
    m_isStartOrStop = false;
    startThd();
}
void QSetLightDlg::startThd()
{
    m_isBreakThd = true;
    m_isCanRun = false;
    std::thread *pt1(new std::thread(runThdFun, this));
}
void QSetLightDlg::stopThd()
{
    m_isBreakThd = false;
    m_isCanRun = false;
}
void QSetLightDlg::pauseThd()
{
    m_isCanRun = false;
}
void QSetLightDlg::runThdFun(QSetLightDlg *pParameter)
{
    pParameter->sub_runThdFun();
}
void QSetLightDlg::sub_runThdFun()
{
    while (m_isBreakThd)
    {
        //	std::lock_guard<std::mutex> lockGuard(my_lock);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (m_isStartOrStop == true) // 开启照明
        {
            // 防水盖动作
            m_dido.JieTongCmd(CDiDo::A_AType_7_8_R2_0x04); // 先R2，即先选择-24V,r2常闭状态即接通了-24V

            m_dido.JieTongCmd(CDiDo::A_CType_10_11_R7_0x80); // 后R7，即接通

            Sleep(1000 * g_program_setInfor.UncoverTime);

            // 防水盖动作完毕,断开正24v
            m_dido.DuankaiCmd(CDiDo::A_CType_10_11_R7_0x80); // 断开R7
            Sleep(1000);
            m_dido.DuankaiCmd(CDiDo::A_AType_7_8_R2_0x04); //-24V恢复
        }
        else if (m_isStartOrStop == false) // 关闭照明
        {
            // 防水盖动作
            m_dido.JieTongCmd(CDiDo::A_CType_10_11_R7_0x80); // 后R7，即接通

            Sleep(1000 * g_program_setInfor.UncoverTime);

            // 防水盖动作完毕,断开正24v
            m_dido.DuankaiCmd(CDiDo::A_CType_10_11_R7_0x80); // 断开R7
            Sleep(1000);
        }
        break;
    }
}

//////开闭照明动作////////////////////////////////////////////////////////////////////
void QSetLightDlg::open_zhaoming_Slots()
{
    // 取得相机序号
    QObject *object = QObject::sender();
    QPushButton *push_button = qobject_cast<QPushButton *>(object);
    QString object_name = push_button->objectName();
    int iChl_num = object_name.toInt();
    open_camera_light(iChl_num);
}
void QSetLightDlg::open_camera_light(int icamNum)
{
    if ((icamNum < 0) || (icamNum > 9))
    {
        return;
    }

    switch (icamNum)
    {
    case 0:
        m_dido.JieTongCmd(CDiDo::A_Camera01_20_21_R3_0x08); // 相机1
        break;
    case 1:
        m_dido.JieTongCmd(CDiDo::A_Camera02_23_24_R4_0x10); // 相机2
        break;
    case 2:
        m_dido.JieTongCmd(CDiDo::A_Camera07_25_26_R5_0x20); // 相机3
        break;
    case 3:
        m_dido.JieTongCmd(CDiDo::A_Camera08_27_28_R6_0x40); // 相机4
        break;
    case 4:
        m_dido.JieTongCmd(CDiDo::A_Camera00_01_02_R0_0x01); // 相机5
        break;
    case 5:
        m_dido.JieTongCmd(CDiDo::A_Camera00_04_05_R1_0x02); // 相机6
        break;
    case 6:
        break;
    case 7:
        break;
    case 8:
        break;
    case 9:
        break;
    }
    freshLedStatus();
}
void QSetLightDlg::close_zhaoming_Slots()
{
    // 取得相机序号
    QObject *object = QObject::sender();
    QPushButton *push_button = qobject_cast<QPushButton *>(object);
    QString object_name = push_button->objectName();
    int iChl_num = object_name.toInt();
    close_camera_light(iChl_num);
}
void QSetLightDlg::close_camera_light(int icamNum)
{
    if ((icamNum < 0) || (icamNum > 9))
    {
        return;
    }

    switch (icamNum)
    {
    case 0:
        m_dido.DuankaiCmd(CDiDo::A_Camera01_20_21_R3_0x08); // 相机1
        break;
    case 1:
        m_dido.DuankaiCmd(CDiDo::A_Camera02_23_24_R4_0x10); // 相机2
        break;
    case 2:
        m_dido.DuankaiCmd(CDiDo::A_Camera07_25_26_R5_0x20); // 相机3
        break;
    case 3:
        m_dido.DuankaiCmd(CDiDo::A_Camera08_27_28_R6_0x40); // 相机4
        break;
    case 4:
        m_dido.DuankaiCmd(CDiDo::A_Camera00_01_02_R0_0x01); // 相机5
        break;
    case 5:
        m_dido.DuankaiCmd(CDiDo::A_Camera00_04_05_R1_0x02); // 相机6
        break;
    case 6:
        break;
    case 7:
        break;
    case 8:
        break;
    case 9:
        break;
    }
    freshLedStatus();
}

//////无砟轨道小车开闭照明动作////////////////////////////////////////////////////////////////////
void QSetLightDlg::pushButton_GD_openzhaomingSlot()
{
    set_milDetect->JieTongCmd(QMilThd::A_Camera01_20_21_R3_0x08); // 相机1
    set_milDetect->JieTongCmd(QMilThd::A_Camera02_23_24_R4_0x10); // 相机2
    set_milDetect->JieTongCmd(QMilThd::A_Camera03_25_26_R5_0x20); // 相机3
    set_milDetect->JieTongCmd(QMilThd::A_Camera04_27_28_R6_0x40); // 相机4
}
void QSetLightDlg::pushButton_GD_closemingzhaoSlot()
{
    set_milDetect->DuankaiCmd(QMilThd::A_Camera01_20_21_R3_0x08); // 相机1
    set_milDetect->DuankaiCmd(QMilThd::A_Camera02_23_24_R4_0x10); // 相机2
    set_milDetect->DuankaiCmd(QMilThd::A_Camera03_25_26_R5_0x20); // 相机3
    set_milDetect->DuankaiCmd(QMilThd::A_Camera04_27_28_R6_0x40); // 相机4
}

////1761卡每个通道继电器指示灯//////////////////////////////////////////////////////////////////////
// 该函数将label控件变成一个圆形指示灯，需要指定颜色color以及直径size
// color 0:grey 1:red 2:green 3:yellow
// size  单位是像素
void QSetLightDlg::setLED(QLabel *label, int color, int size)
{
    // 将label中的文字清空
    label->setText("");
    // 先设置矩形大小
    // 如果ui界面设置的label大小比最小宽度和高度小，矩形将被设置为最小宽度和最小高度；
    // 如果ui界面设置的label大小比最小宽度和高度大，矩形将被设置为最大宽度和最大高度；
    QString min_width = QString("min-width: %1px;").arg(size);             // 最小宽度：size
    QString min_height = QString("min-height: %1px;").arg(size);           // 最小高度：size
    QString max_width = QString("max-width: %1px;").arg(size);             // 最小宽度：size
    QString max_height = QString("max-height: %1px;").arg(size);           // 最小高度：size
                                                                           // 再设置边界形状及边框
    QString border_radius = QString("border-radius: %1px;").arg(size / 2); // 边框是圆角，半径为size/2
    QString border = QString("border:1px solid black;");                   // 边框为1px黑色
                                                                           // 最后设置背景颜色
    QString background = "background-color:";
    switch (color)
    {
    case 0:
        // 灰色
        background += "rgb(190,190,190)";
        break;
    case 1:
        // 红色
        background += "rgb(255,0,0)";
        break;
    case 2:
        // 绿色
        background += "rgb(0,255,0)";
        break;
    case 3:
        // 黄色
        background += "rgb(255,255,0)";
        break;
    default:
        break;
    }

    const QString SheetStyle = min_width + min_height + max_width + max_height + border_radius + border + background;
    label->setStyleSheet(SheetStyle);
}
void QSetLightDlg::freshLedStatus()
{
    // 读回1761状态，刷新led状态
    unsigned char portData = 0;
    m_dido.ReadValue(portData);

    int i0 = portData >> (0) & 1;
    int i1 = portData >> (1) & 1;
    int i2 = portData >> (2) & 1;
    int i3 = portData >> (3) & 1; // 第1个相机
    int i4 = portData >> (4) & 1; // 第2个相机
    int i5 = portData >> (5) & 1; // 第3个相机
    int i6 = portData >> (6) & 1; // 第4个相机
    int i7 = portData >> (7) & 1;

    // 第1个相机,0:grey 1:red 2:green 3:yellow
    if (label_led[0] != NULL)
    {
        if (i3 == 1)
            setLED(label_led[0], 2, 40);
        else if (i3 == 0)
            setLED(label_led[0], 0, 40);
    }

    // 第2个相机,0:grey 1:red 2:green 3:yellow
    if (label_led[1] != NULL)
    {
        if (i4 == 1)
            setLED(label_led[1], 2, 40);
        else if (i4 == 0)
            setLED(label_led[1], 0, 40);
    }

    // 第3个相机,0:grey 1:red 2:green 3:yellow
    if (label_led[2] != NULL)
    {
        if (i5 == 1)
            setLED(label_led[2], 2, 40);
        else if (i5 == 0)
            setLED(label_led[2], 0, 40);
    }

    // 第4个相机,0:grey 1:red 2:green 3:yellow
    if (label_led[3] != NULL)
    {
        if (i6 == 1)
            setLED(label_led[3], 2, 40);
        else if (i6 == 0)
            setLED(label_led[3], 0, 40);
    }
}

//////////////////////////////////////////////////////////////////////////
void QSetLightDlg::get1761xuhao()
{
    m_dido.get1761xuhao();
}