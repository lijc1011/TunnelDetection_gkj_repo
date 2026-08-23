#include "TunnelDetection.h"
#include "HikCamera.h"

TunnelDetection::TunnelDetection(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::TunnelDetection)
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("隧道图像采集程序V2.0"));

    m_bureau_name = QStringLiteral("路局名称");
    m_strLineName = QStringLiteral("线路名称");

    m_isBreakThd = false;
    select_tunnel_flag = true;
    save_start_flag = true;
    save_stop_flag = true;
    m_count = 0;
    myflg = true;

    icow_table_interval = 9;
    icow_table_result = 10;
    icow_table_smil = 11;
    icow_table_emil = 12;

    ReadDefaultPara();
    LineEdit_GD_smil = NULL;
    LineEdit_GD_emil = NULL;

    for (int i = 0; i < MAXCAMNUM; i++)
    {
        // 相机初始状态
        g_isConnect[i] = false; // 未连接
        g_isShow[i] = true;     // 只显示
        g_isSave[i].store(false); // 先不存储

        m_pSaveImg[i] = NULL;
        m_pCamera[i] = NULL;
    }

    createActions();   // 动作
    createMenus();     // 菜单
    createToolbars();  // 工具栏
    createStatusBar(); // 状态栏

    // 查找配置文件
    if (!FindIniFile())
    {
        // 生成，读入
        DefaultIniFile();
        LoadIniFile();
    }
    else
    {
        // 读入
        LoadIniFile();
    }
    if (g_program_setInfor.ObjectType == QStringLiteral("无砟轨道"))
    {
        m_idx = 0;
    }
    else
    {
        m_idx = -1;
    }

    QRect mRect;
    mRect = QGuiApplication::primaryScreen()->geometry();
    qDebug() << "width:" << mRect.width() << "  height:" << mRect.height();

    // 主界面整体是上中下结构
    QVBoxLayout *fullLayout = new QVBoxLayout();
    fullLayout->setContentsMargins(1, 1, 1, 1);  // 对话框左右边距10
    fullLayout->setSpacing(1); // 上下边距

    // 最上边是Hlayout,大按钮
    QHBoxLayout *upupLayout = new QHBoxLayout();
    upupLayout->setContentsMargins(1, 1, 1, 1);  // 对话框左右边距10
    upupLayout->setSpacing(1); // 上下边距

    // 上边是Hlayout,分4段，看相机数量
    QHBoxLayout *upLayout = new QHBoxLayout();
    upLayout->setContentsMargins(1, 1, 1, 1);  // 对话框左右边距10
    upLayout->setSpacing(1); // 上下边距

    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        QGridLayout *Layout_cam = new QGridLayout();
        Layout_cam->setContentsMargins(1, 0, 1, 0); // 左右边距1，上下边距0
        Layout_cam->setSpacing(2); // 控件间距

        // 相机名称
        QString strshow = QStringLiteral("相机：");
        int CPos = g_cameraInfor[i].Name.indexOf('C', 0);
        QString strCameraNameSN = g_cameraInfor[i].Name.mid(0, CPos);
        strshow += strCameraNameSN;

        QLabel *label_cn = new QLabel(strshow);
        Label_camName[i] = new QLabel;
        Layout_cam->addWidget(label_cn, 0, 0);
        //	Layout_cam->addWidget(Label_camName[i], 0, 1);
        label_cn->setStyleSheet("font: bold 15px;");
        label_cn->setAlignment(Qt::AlignCenter);
        label_cn->setMaximumHeight(22);
        Label_camName[i]->setAlignment(Qt::AlignCenter);
        Label_camName[i]->setText(strCameraNameSN);
        Label_camName[i]->setMaximumHeight(22);

        // 是否连接
        label_isConnect[i] = new QLabel(QStringLiteral("未连接"));
        Layout_cam->addWidget(label_isConnect[i], 0, 2);
        label_isConnect[i]->setAlignment(Qt::AlignCenter);
        label_isConnect[i]->setStyleSheet("font: bold 15px;");
        label_isConnect[i]->setMaximumHeight(22);

        // 是否存储
        label_isSave[i] = new QLabel(QStringLiteral("未存储"));
        Layout_cam->addWidget(label_isSave[i], 0, 3);
        label_isSave[i]->setAlignment(Qt::AlignCenter);
        label_isSave[i]->setStyleSheet("font: bold 15px;");
        label_isSave[i]->setMaximumHeight(22);

        upLayout->addLayout(Layout_cam);
    }

    fullLayout->addLayout(upLayout, 0);     // stretch=0，不拉伸

    // 中间是Hlayout,分4段，看相机数量
    QHBoxLayout *midLayout = new QHBoxLayout();
    midLayout->setContentsMargins(1, 1, 1, 1); // 对话框左右边距10
    midLayout->setSpacing(1); // 上下边距

    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        QString strXH = QString(QStringLiteral("显示区域%1")).arg(i + 1, 0, 10);
        labelShowImg[i] = new QLabel(strXH);
        labelShowImg[i]->setStyleSheet("border: 1px solid black;");
        labelShowImg[i]->setAlignment(Qt::AlignCenter);

        if (g_program_setInfor.ObjectType == QStringLiteral("无砟轨道"))
        {
            int iwidth = mRect.width() / 118; // 屏幕宽度按128分成138份，只有肥东站那事改成了118
            if (g_cameraInfor[i].ImageWidth == 4096)
            {
                labelShowImg[i]->setFixedWidth(iwidth * 32);
                labelShowImg[i]->setFixedHeight(iwidth * 32);
            }
            if (g_cameraInfor[i].ImageWidth == 2048)
            {
                labelShowImg[i]->setFixedWidth(iwidth * 16);
                labelShowImg[i]->setFixedHeight(iwidth * 32);
            }
            if (g_cameraInfor[i].ImageWidth == 2688)
            {
                labelShowImg[i]->setFixedWidth(iwidth * 21);
                labelShowImg[i]->setFixedHeight(iwidth * 32);
            }
        }
        else if (g_program_setInfor.ObjectType != QStringLiteral("无砟轨道"))
        {
            int iwidth = mRect.width() / g_program_setInfor.CameraNumber - 1;
            int iheigth = iwidth;
            if (g_program_setInfor.Set_show_WH == 0) // 1代表手动设置，0代表自动设置
            {
                if (g_cameraInfor[i].ImageWidth == 4096)
                {
                    iheigth = iwidth;
                }
                if (g_cameraInfor[i].ImageWidth == 2048)
                {
                    iheigth = iwidth * 2;
                }
            }
            else if (g_program_setInfor.Set_show_WH == 1)
            {
                iheigth = g_program_setInfor.Set_show_H;
                iwidth = g_program_setInfor.Set_show_W;
            }
            labelShowImg[i]->setFixedHeight(iheigth);
            labelShowImg[i]->setFixedWidth(iwidth);
        }
        midLayout->addWidget(labelShowImg[i]);
    }

    fullLayout->addLayout(midLayout, 0);    // stretch=0，不拉伸

    // 下边是Hlayout,分多段
    QHBoxLayout *downLayout = new QHBoxLayout();
    downLayout->setContentsMargins(1, 1, 1, 1);  // 对话框左右边距10
    downLayout->setSpacing(1); // 上下边距

    // 下.1是行车信息设置
    // 放到QGridLayout
    QGridLayout *CamLayout1 = new QGridLayout();
    CamLayout1->setContentsMargins(0, 0, 0, 0);
    CamLayout1->setHorizontalSpacing(10);
    CamLayout1->setContentsMargins(10, 10, 10, 10);  // 对话框左右边距10
    CamLayout1->setSpacing(10); // 上下边距

    // 下.x是铁科院标识
#if 0
 	QLabel *tky_label = new QLabel();
	tky_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	QImage img("images/tky.jpg");
	tky_label->setPixmap(QPixmap::fromImage(img).scaled(tky_label->size()));//setPixelmap(QPixmap::fromImage(img));  
	tky_label->resize(tky_label->pixmap()->size());//resize(ui->label->pixmap()->size());  
	tky_label->adjustSize();
	tky_label->setScaledContents(true);
	tky_label->setMaximumHeight(70);
//	tky_label->setMaximumWidth(iwidth);
 	CamLayout1->addWidget(tky_label, 0, 0, 1, 3);
#endif

    CamLayout1->addWidget(ui->label_lineName, 0, 0);
    CamLayout1->setAlignment(Qt::AlignCenter);
    CamLayout1->addWidget(ui->lineEdit_lineName, 0, 1, 1, 2);
    CamLayout1->addWidget(ui->label_decTime, 1, 0);
    CamLayout1->addWidget(ui->lineEdit_decTime, 1, 1, 1, 2);

    CamLayout1->addWidget(ui->label_lineXB, 2, 0);
    CamLayout1->addWidget(ui->radioButton_shang, 2, 1);
    CamLayout1->addWidget(ui->radioButton_xia, 2, 2);
    connect(ui->radioButton_shang, SIGNAL(clicked()), this, SLOT(onRadioClickSlot()));
    connect(ui->radioButton_xia, SIGNAL(clicked()), this, SLOT(onRadioClickSlot()));
    ui->radioButton_shang->setCheckable(true);
    ui->radioButton_shang->setChecked(false);
    ui->radioButton_xia->setCheckable(true);
    ui->radioButton_xia->setChecked(true);
    if (m_strXB == QStringLiteral("上行"))
    {
        ui->radioButton_shang->setChecked(true);
        ui->radioButton_xia->setChecked(false);
    }
    else if (m_strXB == QStringLiteral("下行"))
    {
        ui->radioButton_shang->setChecked(false);
        ui->radioButton_xia->setChecked(true);
    }
    QButtonGroup *group1 = new QButtonGroup();
    group1->addButton(ui->radioButton_shang, 0);
    group1->addButton(ui->radioButton_xia, 1);

    CamLayout1->addWidget(ui->label_milDir, 3, 0);
    CamLayout1->addWidget(ui->radioButton_milInc, 3, 1);
    CamLayout1->addWidget(ui->radioButton_milDec, 3, 2);
    connect(ui->radioButton_milInc, SIGNAL(clicked()), this, SLOT(onmilRadioClickSlot()));
    connect(ui->radioButton_milDec, SIGNAL(clicked()), this, SLOT(onmilRadioClickSlot()));
    ui->radioButton_milInc->setCheckable(true);
    ui->radioButton_milInc->setChecked(true);
    ui->radioButton_milDec->setCheckable(true);
    ui->radioButton_milDec->setChecked(false);
    if (m_strMilDir == QStringLiteral("增加"))
    {
        ui->radioButton_milInc->setChecked(true);
        ui->radioButton_milDec->setChecked(false);
    }
    else if (m_strMilDir == QStringLiteral("减小"))
    {
        ui->radioButton_milInc->setChecked(false);
        ui->radioButton_milDec->setChecked(true);
    }
    QButtonGroup *group2 = new QButtonGroup();
    group2->addButton(ui->radioButton_milInc, 0);
    group2->addButton(ui->radioButton_milDec, 1);

    CamLayout1->addWidget(ui->label_chetouDir, 4, 0);
    CamLayout1->addWidget(ui->radioButton_chetouBig, 4, 1);
    CamLayout1->addWidget(ui->radioButton_chetouSmall, 4, 2);
    connect(ui->radioButton_chetouBig, SIGNAL(clicked()), this, SLOT(onchetouRadioClickSlot()));
    connect(ui->radioButton_chetouSmall, SIGNAL(clicked()), this, SLOT(onchetouRadioClickSlot()));
    ui->radioButton_chetouBig->setCheckable(true);
    ui->radioButton_chetouBig->setChecked(true);
    ui->radioButton_chetouSmall->setCheckable(true);
    ui->radioButton_chetouSmall->setChecked(false);
    if (m_strChetou == QStringLiteral("大里程"))
    {
        ui->radioButton_chetouBig->setChecked(true);
        ui->radioButton_chetouSmall->setChecked(false);
    }
    else if (m_strChetou == QStringLiteral("小里程"))
    {
        ui->radioButton_chetouBig->setChecked(false);
        ui->radioButton_chetouSmall->setChecked(true);
    }
    QButtonGroup *group5 = new QButtonGroup();
    group5->addButton(ui->radioButton_chetouBig, 0);
    group5->addButton(ui->radioButton_chetouSmall, 1);

    ui->label_lineName->setMinimumWidth(70);
    ui->label_decTime->setMinimumWidth(70);
    ui->label_lineXB->setMinimumWidth(70);
    ui->label_milDir->setMinimumWidth(70);
    ui->label_chetouDir->setMinimumWidth(70);
    ui->lineEdit_lineName->setMinimumWidth(120);
    ui->lineEdit_decTime->setMinimumWidth(120);

    const int screenWidth = mRect.width();
    int CamLayout_width = qBound(220, screenWidth * 13 / 100, 320);
    if (g_program_setInfor.ObjectType == QStringLiteral("无砟轨道"))
    {
        CamLayout_width = qBound(220, screenWidth * 23 / 100, 300);
    }
    m_tableWidget_width = qMax(760, screenWidth - CamLayout_width * 2 - 20);
    init_tableWidget();

    QGroupBox *CamLayout1_groupBox = new QGroupBox(QStringLiteral("行车信息"));
    CamLayout1_groupBox->setAlignment(Qt::AlignCenter);
    CamLayout1_groupBox->setMaximumWidth(CamLayout_width);
    // 用外层布局上下加 stretch 使内部控件整体上下居中
    QVBoxLayout *wrapper1 = new QVBoxLayout();
    wrapper1->addStretch(1);
    wrapper1->addLayout(CamLayout1);
    wrapper1->addStretch(1);
    CamLayout1_groupBox->setLayout(wrapper1);

    // 下.2是位置信息设置
    QGridLayout *CamLayout2 = new QGridLayout();
    CamLayout2->setContentsMargins(0, 0, 0, 0);
    CamLayout2->setHorizontalSpacing(10);
    CamLayout2->setContentsMargins(10, 10, 10, 10);  // 对话框左右边距10
    CamLayout2->setSpacing(10); // 上下边距

    ui->lcdNumber_nmil->setDigitCount(8); // 设置数字位数为6位
    ui->lcdNumber_spe->setDigitCount(8);  // 设置数字位数为6位

    QColor color = QColor(0, 255, 127);
    QString winColor = color.name();

    ui->lcdNumber_nmil->setStyleSheet("QLCDNumber {background-color:" + winColor + ";color:black;border: 1px solid black;font: bold 28px;}");
    ui->lcdNumber_spe->setStyleSheet("QLCDNumber {background-color:" + winColor + ";color:black;border: 1px solid black;font: bold 24px;}");
    ui->lcdNumber_nmil->setSegmentStyle(QLCDNumber::Flat); // 设置数字显示风格为扁平风格
    ui->lcdNumber_spe->setSegmentStyle(QLCDNumber::Flat);  // 设置数字显示风格为扁平风格
    ui->lcdNumber_nmil->display(1234.56);                  // 显示数字123456
    ui->lcdNumber_spe->display(78.56);                     // 显示数字123456

    CamLayout2->addWidget(ui->label_nmil, 0, 0);
    CamLayout2->addWidget(ui->lcdNumber_nmil, 0, 1);
    CamLayout2->addWidget(ui->label_nmilk, 0, 2);
    ui->lcdNumber_nmil->setMaximumHeight(30);
    CamLayout2->addWidget(ui->label_spe, 1, 0);
    CamLayout2->addWidget(ui->lcdNumber_spe, 1, 1);
    CamLayout2->addWidget(ui->label_spekh, 1, 2);
    ui->lcdNumber_spe->setMaximumHeight(30);

    // 起止里程
    QLabel *label_smil = new QLabel(QStringLiteral("起始里程："));
    LineEdit_smil = new QLineEdit;
    LineEdit_smil->setReadOnly(true);
    label_smil->setAlignment(Qt::AlignRight);
    CamLayout2->addWidget(label_smil, 2, 0);
    CamLayout2->addWidget(LineEdit_smil, 2, 1, 1, 2);
    QLabel *label_emil = new QLabel(QStringLiteral("终止里程："));
    LineEdit_emil = new QLineEdit;
    LineEdit_emil->setReadOnly(true);
    label_emil->setAlignment(Qt::AlignRight);
    CamLayout2->addWidget(label_emil, 3, 0);
    CamLayout2->addWidget(LineEdit_emil, 3, 1, 1, 2);
    Mil_over_slots(m_strStartMil, m_strEndMil);

    QLabel *label_cunchu = new QLabel(QStringLiteral("磁盘容量："));
    LineEdit_cunchu = new QLineEdit;
    LineEdit_cunchu->setReadOnly(true);
    label_cunchu->setAlignment(Qt::AlignRight);
    CamLayout2->addWidget(label_cunchu, 4, 0);
    CamLayout2->addWidget(LineEdit_cunchu, 4, 1, 1, 2);

    QWidget *actWidget = new QWidget();
    actWidget->setMaximumWidth(CamLayout_width);
    actWidget->setLayout(CamLayout2);

    QHBoxLayout *myLayout = new QHBoxLayout();
    myLayout->setContentsMargins(1, 1, 1, 1);  // 对话框左右边距10
    myLayout->setSpacing(1); // 上下边距
    myLayout->addWidget(actWidget);

    QGroupBox *CamLayout2_groupBox = new QGroupBox(QStringLiteral("位置信息"));
    CamLayout2_groupBox->setAlignment(Qt::AlignCenter);
    CamLayout2_groupBox->setMaximumWidth(CamLayout_width);
    // 用外层布局上下加 stretch 使内部控件整体上下居中
    QVBoxLayout *wrapper2 = new QVBoxLayout();
    wrapper2->addStretch(1);
    wrapper2->addLayout(myLayout);
    wrapper2->addStretch(1);
    CamLayout2_groupBox->setLayout(wrapper2);

    // 将两个 GroupBox 包在同一个水平容器中，并排横放后整体居中
    QHBoxLayout *leftPanelLayout = new QHBoxLayout();
    leftPanelLayout->setContentsMargins(0, 0, 0, 0);
    leftPanelLayout->setSpacing(6);
    leftPanelLayout->addWidget(CamLayout1_groupBox);
    leftPanelLayout->addWidget(CamLayout2_groupBox);

    QWidget *leftPanelWidget = new QWidget();
    leftPanelWidget->setMaximumWidth(CamLayout_width * 2 + 6);
    leftPanelWidget->setLayout(leftPanelLayout);

    downLayout->addWidget(leftPanelWidget);

    // 下.3是正在检测的隧道列表
    ui->tableWidget->setMinimumWidth(760); //
    ui->tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    // 添加表格右键菜单
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    originDataTableContextMenu = new QMenu(this);
    originDataTableActionDel = new QAction(this);
    originDataTableActionDel->setText(QStringLiteral("选中该隧道，进行手动控制采集"));
    originDataTableContextMenu->addAction(originDataTableActionDel);

    // 表格右键菜单项的信号槽连接
    connect(originDataTableContextMenu, SIGNAL(triggered(QAction *)), this, SLOT(slotMenuClicked(QAction *)));
    connect(ui->tableWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(tableContexMenuRequested(const QPoint &)));
    downLayout->addWidget(ui->tableWidget);
    if (g_program_setInfor.ObjectType == QStringLiteral("无砟轨道"))
    {
        ui->tableWidget->hide();

        QGridLayout *gdMileageLayout = new QGridLayout();
        gdMileageLayout->setContentsMargins(10, 5, 10, 5);
        gdMileageLayout->setHorizontalSpacing(8);
        gdMileageLayout->setVerticalSpacing(8);

        QLabel *label_GD_smil = new QLabel(QStringLiteral("起始里程："));
        QLabel *label_GD_emil = new QLabel(QStringLiteral("终止里程："));
        QLabel *label_GD_smil_unit = new QLabel(QStringLiteral("公里"));
        QLabel *label_GD_emil_unit = new QLabel(QStringLiteral("公里"));
        LineEdit_GD_smil = new QLineEdit(m_strStartMil);
        LineEdit_GD_emil = new QLineEdit(m_strEndMil);
        QPushButton *pushButton_GD_setStartMileage = new QPushButton(QStringLiteral("设置"));
        QPushButton *pushButton_GD_setEndMileage = new QPushButton(QStringLiteral("设置"));

        label_GD_smil->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label_GD_emil->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        LineEdit_GD_smil->setMinimumWidth(120);
        LineEdit_GD_emil->setMinimumWidth(120);
        LineEdit_GD_smil->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        LineEdit_GD_emil->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        pushButton_GD_setStartMileage->setMinimumWidth(70);
        pushButton_GD_setEndMileage->setMinimumWidth(70);

        gdMileageLayout->addWidget(label_GD_smil, 0, 0);
        gdMileageLayout->addWidget(LineEdit_GD_smil, 0, 1);
        gdMileageLayout->addWidget(label_GD_smil_unit, 0, 2);
        gdMileageLayout->addWidget(pushButton_GD_setStartMileage, 0, 3);
        gdMileageLayout->addWidget(label_GD_emil, 1, 0);
        gdMileageLayout->addWidget(LineEdit_GD_emil, 1, 1);
        gdMileageLayout->addWidget(label_GD_emil_unit, 1, 2);
        gdMileageLayout->addWidget(pushButton_GD_setEndMileage, 1, 3);
        connect(pushButton_GD_setStartMileage, &QPushButton::clicked, this, [this]() {
            Mil_over_slots(LineEdit_GD_smil->text(), m_strEndMil);
        });
        connect(pushButton_GD_setEndMileage, &QPushButton::clicked, this, [this]() {
            Mil_over_slots(m_strStartMil, LineEdit_GD_emil->text());
        });

        QHBoxLayout *gdLayout = new QHBoxLayout();
        gdLayout->setContentsMargins(10, 0, 10, 0);
        gdLayout->setSpacing(0);

        ui->pushButton_GD_openzhaoming->setText(QStringLiteral("打开光源"));
        ui->pushButton_GD_openzhaoming->setMinimumWidth(130);
        ui->pushButton_GD_openzhaoming->setMaximumWidth(200);
        ui->pushButton_GD_openzhaoming->setMinimumHeight(56);
        ui->pushButton_GD_openzhaoming->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(ui->pushButton_GD_openzhaoming, &QPushButton::clicked, this, &TunnelDetection::pushButton_GD_openzhaomingSlot);
        gdLayout->addStretch(1);
        gdLayout->addWidget(ui->pushButton_GD_openzhaoming);
        gdLayout->addSpacing(qBound(20, screenWidth / 50, 40));

        ui->pushButton_GD_closemingzhao->setText(QStringLiteral("关闭光源"));
        ui->pushButton_GD_closemingzhao->setMinimumWidth(130);
        ui->pushButton_GD_closemingzhao->setMaximumWidth(200);
        ui->pushButton_GD_closemingzhao->setMinimumHeight(56);
        ui->pushButton_GD_closemingzhao->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(ui->pushButton_GD_closemingzhao, &QPushButton::clicked, this, &TunnelDetection::pushButton_GD_closemingzhaoSlot);
        gdLayout->addWidget(ui->pushButton_GD_closemingzhao);
        gdLayout->addStretch(1);

        QVBoxLayout *gd2Layout = new QVBoxLayout();
        gd2Layout->setContentsMargins(8, 8, 8, 8);
        gd2Layout->setSpacing(14);
        gd2Layout->addStretch(6);
        gd2Layout->addLayout(gdMileageLayout);
        gd2Layout->addLayout(gdLayout);
        gd2Layout->addStretch(4);

        QGroupBox *gd_groupBox = new QGroupBox(QStringLiteral("无砟轨道小车检测控制"));
        gd_groupBox->setAlignment(Qt::AlignCenter);
        const int gdPanelWidth = qBound(400, screenWidth - CamLayout_width * 2 - 36, 650);
        gd_groupBox->setMaximumWidth(gdPanelWidth);
        gd_groupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        gd_groupBox->setMinimumHeight(330);
        gd_groupBox->setLayout(gd2Layout);
        downLayout->addWidget(gd_groupBox);
    }

    // 下.3是限界区域
    //  	ui->graphicsView->setFixedWidth(iwidth);
    ui->graphicsView->setFixedWidth(500); // 1200是个固定数
    //	ui->graphicsView->setMinimumWidth(1200);//
    //	downLayout->addWidget(ui->graphicsView);

    fullLayout->addLayout(downLayout, 1);   // stretch=1，占据所有剩余空间

    QWidget *widget = new QWidget();
    widget->setLayout(fullLayout);
    this->setCentralWidget(widget);

    // 定时器，系统时间
    timerprogress = new QTimer(this);
    connect(timerprogress, SIGNAL(timeout()), this, SLOT(timerUpDateprogress()));
    timerprogress->setInterval(100);
    timerprogress->start();

    // 定时器，调试状态下，模拟GYK发数
    timerprogress_gykMil = new QTimer(this);
    connect(timerprogress_gykMil, SIGNAL(timeout()), this, SLOT(timerUpDateGYKMilprogress()));
    timerprogress_gykMil->setInterval(3000); // 模拟GYK给数，1秒走20m，3秒走60m
    if (g_program_setInfor.DebugTimmerSwitch == 1)
    {
        m_dmil_GYK = g_program_setInfor.DebugStartMil; // 模拟的gyk初始里程值
        timerprogress_gykMil->start();
    }
    // 定时器，自动模式判断里程
    timerprogress_auto = new QTimer(this);
    connect(timerprogress_auto, SIGNAL(timeout()), this, SLOT(timerUpDateAutoModeprogress()));
    timerprogress_auto->setInterval(100);

    // 定时器，用于优化GYK里程，每10ms一次
    timerprogress_gykMil_accuracy = new QTimer(this);
    connect(timerprogress_gykMil_accuracy, SIGNAL(timeout()), this, SLOT(timerprogress_gykMil_accuracyprogress()));
    timerprogress_gykMil_accuracy->setInterval(10);
    timerprogress_gykMil_accuracy->start();

    // 定时器，硬盘容量实时更新
    timerprogress_diskcapacity = new QTimer(this);
    connect(timerprogress_diskcapacity, SIGNAL(timeout()), this, SLOT(timer_diskcapacity_progress()));
    timerprogress_diskcapacity->setInterval(1000);
    timerprogress_diskcapacity->start();

    //脉冲计数线程
    m_milDirect = NULL;
    m_milDirect = new QMilThd(this);

    //里程信息展示对话框
    pSetMileageDlg = NULL;
    pSetMileageDlg = new QSetMileageDlg(m_milDirect, this);
    pSetMileageDlg->setWindowTitle(QStringLiteral("设置里程信息对话框"));
    QObject::connect(pSetMileageDlg, SIGNAL(Mil_over_signals(QString, QString)), this, SLOT(Mil_over_slots(QString, QString)));
    QObject::connect(pSetMileageDlg, SIGNAL(Mil_time_signals(double, double)), this, SLOT(Mil_time_slots(double, double)));
    pSetMileageDlg->setMilDir(m_strMilDir);

    // 设置相机参数对话框
    pSetCameraDlg = NULL;
    pSetCameraDlg = new QSetCameraDlg(this);
    pSetCameraDlg->setWindowTitle(QStringLiteral("设置相机参数对话框"));

    // 设置照明（防水盖）开闭
    pSetLightDlg = NULL;
    pSetLightDlg = new QSetLightDlg(m_milDirect, this);
    pSetLightDlg->setWindowTitle(QStringLiteral("设置照明（防水盖）开闭"));

    // 设置程序参数对话框
    pSetParaDlg = NULL;
    pSetParaDlg = new QSetParaDlg(this);
    pSetParaDlg->setWindowTitle(QStringLiteral("设置程序参数"));
    pSetParaDlg->showInitPara();

    pRrealTimeState = NULL;

    m_QShowThd = NULL;
    m_QShowThd = new QShowThd(this);

    // 程序启动时就自动执行的功能模块
    // GYK里程获取，串口接收
    sp_gyk();

    // 连接相机
    startThd();

    // 提示是否自动加载台账
    if (!m_csvPathFileName.isEmpty())
    {
        QString strText = QStringLiteral("是否自动加载上次检测的设备简表？");
        strText += "\r\n";
        QFileInfo fi(m_csvPathFileName);
        strText += fi.fileName();
        QMessageBox::StandardButton reply;
        reply = QMessageBox::information(this, QStringLiteral("敬请注意"),
                                         strText,
                                         QMessageBox::Yes | QMessageBox::Cancel);
        if (reply == QMessageBox::Yes)
        {
            loadledgerFile(m_csvPathFileName);
        }
    }
}

TunnelDetection::~TunnelDetection()
{
    delete ui; 
}

/////获取本地参数文件/////////////////////////////////////////////////////////////////////
void TunnelDetection::ReadDefaultPara()
{
    // 读配置文件
    QString curPath = QCoreApplication::applicationDirPath();
    QString filePath = curPath + "/setting/config.ini";
    QSettings setting(filePath, QSettings::IniFormat);
    setting.beginGroup("config");
    gprogram_tempInfor.tmp_path_file = setting.value("openPath").toString();
    m_csvPathFileName = setting.value("openPathFile").toString();
    m_strXB = setting.value("runxingbie").toString();
    m_strMilDir = setting.value("milIncDec").toString();
    m_strChetou = setting.value("chetouDir").toString();
    // 起止里程属于本次作业数据，每次启动时都由操作人员重新设置，
    // 不恢复上一次运行保存的值。
    m_strStartMil.clear();
    m_strEndMil.clear();

    setting.endGroup();
}
void TunnelDetection::WriteDefaultPara()
{
    QString curPath = QCoreApplication::applicationDirPath();
    QString filePath = curPath + "/setting/config.ini";
    QSettings setting(filePath, QSettings::IniFormat);                             // 生成配置文件
    setting.beginGroup("config");                                                  // beginGroup与下面endGroup 相对应，“config”是标记
    setting.setValue("openPath", gprogram_tempInfor.tmp_path_file);
    setting.setValue("openPathFile", m_csvPathFileName);
    setting.setValue("runxingbie", m_strXB);
    setting.setValue("milIncDec", m_strMilDir);
    setting.setValue("chetouDir", m_strChetou);
    setting.setValue("StartMil", m_strStartMil);
    setting.setValue("EndMil", m_strEndMil);
    setting.endGroup();
}
bool TunnelDetection::FindIniFile()
{
    QString  curPath = QCoreApplication::applicationDirPath();
    QString filePath = curPath + "/Config.xml";
    QFile file(filePath);
    if (!file.exists())
    {
        ///在此弹出消息框
        // QMessageBox::warning(this, QStringLiteral("提示"),
        //     QStringLiteral("未找到配置文件 Config.xml！\n路径：%1").arg(filePath));
        return false;
    }

    // QMessageBox::warning(this, QStringLiteral("提示"),
    //                      QStringLiteral("已找到配置文件 Config.xml！\n路径：%1").arg(filePath));
    return true;

    
}
void TunnelDetection::DefaultIniFile()
{
    QString filePath = QCoreApplication::applicationDirPath() + "/Config.xml";
    char xmlPath[RouteInfoSize];
    strcpy(xmlPath, filePath.toLocal8Bit().data());

    TiXmlDocument *doc = new TiXmlDocument(); // 创建xml文档对象
    TiXmlDeclaration *pDeclaration = new TiXmlDeclaration("1.0", "UTF-8", "");
    doc->LinkEndChild(pDeclaration);

    TiXmlElement *RootElement = new TiXmlElement("body"); // 创建一个根结点
    doc->LinkEndChild(RootElement);

    ////程序配置信息PROINFOR///////program_set_struct/////////////////////////////
    TiXmlElement *ProInforElement = new TiXmlElement("program_infor");
    RootElement->LinkEndChild(ProInforElement);

    writeToXmlElement(ProInforElement, "ObjectType", g_program_setInfor.ObjectType.toStdString().c_str());
    writeToXmlElement(ProInforElement, "ZhuCong", QString::number(g_program_setInfor.ZhuCong, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "ZhuServerIp", g_program_setInfor.ZhuServerIp.toStdString().c_str());

    writeToXmlElement(ProInforElement, "Pci1710Num", QString::number(g_program_setInfor.Pci1710Num, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "Pci1761Num", QString::number(g_program_setInfor.Pci1761Num, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "Pci1784Num", QString::number(g_program_setInfor.Pci1784Num, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "onoff1784", QString::number(g_program_setInfor.onoff1784, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "PulseWidth", QString::number(g_program_setInfor.PulseWidth, 'f', 8).toStdString().c_str());

    writeToXmlElement(ProInforElement, "DataDestPath", g_program_setInfor.DataDestPath.toStdString().c_str());
    writeToXmlElement(ProInforElement, "MilType", QString::number(g_program_setInfor.MilType, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "MilComPort", QString::number(g_program_setInfor.MilComPort, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "MilComBaud", QString::number(g_program_setInfor.MilComBaud, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "CameraTotalNumber", QString::number(g_program_setInfor.CameraTotalNumber, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "ViewTotalNumber", QString::number(g_program_setInfor.ViewTotalNumber, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "CameraNumber", QString::number(g_program_setInfor.CameraNumber, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "CameraType", QString::number(g_program_setInfor.CameraType, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "UncoverTime", QString::number(g_program_setInfor.UncoverTime, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "ShowThdTime", QString::number(g_program_setInfor.ShowThdTime, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "rukouThreshold", QString::number(g_program_setInfor.rukouThreshold, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "chukouThreshold", QString::number(g_program_setInfor.chukouThreshold, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "GYKPosValue", QString::number(g_program_setInfor.GYKPosValue, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "GYK0_or_COR1", QString::number(g_program_setInfor.GYK0_or_COR1, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "DebugTimmerSwitch", QString::number(g_program_setInfor.DebugTimmerSwitch, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "DebugStartMil", QString::number(g_program_setInfor.DebugStartMil, 'f', 3).toStdString().c_str());
    writeToXmlElement(ProInforElement, "DebugMilInc1Dec0", QString::number(g_program_setInfor.DebugMilInc1Dec0, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "DebugMilStepLength", QString::number(g_program_setInfor.DebugMilStepLength, 10).toStdString().c_str());

    writeToXmlElement(ProInforElement, "danxian", g_program_setInfor.danxian.toStdString().c_str());
    writeToXmlElement(ProInforElement, "shuang_xia_da", g_program_setInfor.shuang_xia_da.toStdString().c_str());
    writeToXmlElement(ProInforElement, "shuang_xia_xiao", g_program_setInfor.shuang_xia_xiao.toStdString().c_str());
    writeToXmlElement(ProInforElement, "shuang_shang_da", g_program_setInfor.shuang_shang_da.toStdString().c_str());
    writeToXmlElement(ProInforElement, "shuang_shang_xiao", g_program_setInfor.shuang_shang_xiao.toStdString().c_str());

    writeToXmlElement(ProInforElement, "xia_da_zuo", g_program_setInfor.xia_da_zuo.toStdString().c_str());
    writeToXmlElement(ProInforElement, "xia_xiao_zuo", g_program_setInfor.xia_xiao_zuo.toStdString().c_str());
    writeToXmlElement(ProInforElement, "shang_da_you", g_program_setInfor.shang_da_you.toStdString().c_str());
    writeToXmlElement(ProInforElement, "shang_xiao_you", g_program_setInfor.shang_xiao_you.toStdString().c_str());

    writeToXmlElement(ProInforElement, "pictureWH", g_program_setInfor.pictureWH.toStdString().c_str());

    writeToXmlElement(ProInforElement, "Set_show_WH", QString::number(g_program_setInfor.Set_show_WH, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "Set_show_W", QString::number(g_program_setInfor.Set_show_W, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "Set_show_H", QString::number(g_program_setInfor.Set_show_H, 10).toStdString().c_str());

    writeToXmlElement(ProInforElement, "Table_rows", QString::number(g_program_setInfor.Table_rows, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "big_mil_onoff", QString::number(g_program_setInfor.big_mil_onoff, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "mil_threshold", QString::number(g_program_setInfor.mil_threshold, 10).toStdString().c_str());
    writeToXmlElement(ProInforElement, "SaveZarr", QString::number(g_program_setInfor.SaveZarr, 10).toStdString().c_str());

    ////相机信息CameraInfor///////preprocess_data_struct/////////////////////////////
    TiXmlElement *camera_dataElement = new TiXmlElement("camera_data");
    RootElement->LinkEndChild(camera_dataElement);

    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        QString strItem = "camera";
        QString cam_xh = QString("%1").arg(i + 1, 2, 10, QChar('0'));
        strItem += cam_xh;
        char cPath[RouteInfoSize];
        strcpy(cPath, strItem.toLocal8Bit().data());

        TiXmlElement *Camera_ITEM = new TiXmlElement(cPath);
        camera_dataElement->LinkEndChild(Camera_ITEM);
        writeToXmlElement(Camera_ITEM, "Name", g_cameraInfor[i].Name.toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "CamLinkName", g_cameraInfor[i].CamLinkName.toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "ExposureMode", QString::number(g_cameraInfor[i].ExposureMode, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "ExposureTime", QString::number(g_cameraInfor[i].ExposureTime, 'f', 2).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "PictureHeight", QString::number(g_cameraInfor[i].PictureHeight, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "ImageHeight", QString::number(g_cameraInfor[i].ImageHeight, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "ImageWidth", QString::number(g_cameraInfor[i].ImageWidth, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "TriggerMode", QString::number(g_cameraInfor[i].TriggerMode, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "LineFormat", QString::number(g_cameraInfor[i].LineFormat, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "LineFreq", QString::number(g_cameraInfor[i].LineFreq, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "IsFlip", QString::number(g_cameraInfor[i].IsFlip, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "FlipMode", QString::number(g_cameraInfor[i].FlipMode, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "SaveTransform", QString::number(g_cameraInfor[i].SaveTransform, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "Gain", QString::number(g_cameraInfor[i].Gain, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "isSingle", QString::number(g_cameraInfor[i].isSingle, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "isDouble", QString::number(g_cameraInfor[i].isDouble, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "isColorCamera", QString::number(g_cameraInfor[i].isColorCamera, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "ExposureTimeMin", QString::number(g_cameraInfor[i].ExposureTimeMin, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "ExposureTimeMax", QString::number(g_cameraInfor[i].ExposureTimeMax, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "GainMin", QString::number(g_cameraInfor[i].GainMin, 10).toStdString().c_str());
        writeToXmlElement(Camera_ITEM, "GainMax", QString::number(g_cameraInfor[i].GainMax, 10).toStdString().c_str());
    }

    doc->SaveFile(xmlPath);
    return;
}
bool TunnelDetection::LoadIniFile()
{
    // 设置输入文件
    QString filePath = QCoreApplication::applicationDirPath() + "/Config.xml";
    char xmlPath[RouteInfoSize];
    strcpy(xmlPath, filePath.toLocal8Bit().data());

    TiXmlDocument *mydoc = new TiXmlDocument(xmlPath);
    if (!mydoc->LoadFile())
        return false;
    TiXmlElement *rootElement = mydoc->RootElement();

    ////程序配置信息PROINFOR///////program_set_struct/////////////////////////////
    TiXmlElement *genElement = rootElement->FirstChildElement("program_infor");
    if ((genElement->FirstChildElement("ObjectType") != NULL) && (genElement->FirstChildElement("ObjectType")->FirstChild() != NULL))
        g_program_setInfor.ObjectType = QString(genElement->FirstChildElement("ObjectType")->FirstChild()->Value());
    if ((genElement->FirstChildElement("ZhuCong") != NULL) && (genElement->FirstChildElement("ZhuCong")->FirstChild() != NULL))
        g_program_setInfor.ZhuCong = QString(genElement->FirstChildElement("ZhuCong")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("Pci1710Num") != NULL) && (genElement->FirstChildElement("Pci1710Num")->FirstChild() != NULL))
        g_program_setInfor.Pci1710Num = QString(genElement->FirstChildElement("Pci1710Num")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("Pci1761Num") != NULL) && (genElement->FirstChildElement("Pci1761Num")->FirstChild() != NULL))
        g_program_setInfor.Pci1761Num = QString(genElement->FirstChildElement("Pci1761Num")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("Pci1784Num") != NULL) && (genElement->FirstChildElement("Pci1784Num")->FirstChild() != NULL))
        g_program_setInfor.Pci1784Num = QString(genElement->FirstChildElement("Pci1784Num")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("onoff1784") != NULL) && (genElement->FirstChildElement("onoff1784")->FirstChild() != NULL))
        g_program_setInfor.onoff1784 = QString(genElement->FirstChildElement("onoff1784")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("PulseWidth") != NULL) && (genElement->FirstChildElement("PulseWidth")->FirstChild() != NULL))
        g_program_setInfor.PulseWidth = QString(genElement->FirstChildElement("PulseWidth")->FirstChild()->Value()).toDouble();
    if ((genElement->FirstChildElement("DataDestPath") != NULL) && (genElement->FirstChildElement("DataDestPath")->FirstChild() != NULL))
        g_program_setInfor.DataDestPath = QString(genElement->FirstChildElement("DataDestPath")->FirstChild()->Value());
    if ((genElement->FirstChildElement("MilType") != NULL) && (genElement->FirstChildElement("MilType")->FirstChild() != NULL))
        g_program_setInfor.MilType = QString(genElement->FirstChildElement("MilType")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("MilComPort") != NULL) && (genElement->FirstChildElement("MilComPort")->FirstChild() != NULL))
        g_program_setInfor.MilComPort = QString(genElement->FirstChildElement("MilComPort")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("MilComBaud") != NULL) && (genElement->FirstChildElement("MilComBaud")->FirstChild() != NULL))
        g_program_setInfor.MilComBaud = QString(genElement->FirstChildElement("MilComBaud")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("CameraTotalNumber") != NULL) && (genElement->FirstChildElement("CameraTotalNumber")->FirstChild() != NULL))
        g_program_setInfor.CameraTotalNumber = QString(genElement->FirstChildElement("CameraTotalNumber")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("ViewTotalNumber") != NULL) && (genElement->FirstChildElement("ViewTotalNumber")->FirstChild() != NULL))
        g_program_setInfor.ViewTotalNumber = QString(genElement->FirstChildElement("ViewTotalNumber")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("CameraNumber") != NULL) && (genElement->FirstChildElement("CameraNumber")->FirstChild() != NULL))
        g_program_setInfor.CameraNumber = QString(genElement->FirstChildElement("CameraNumber")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("CameraType") != NULL) && (genElement->FirstChildElement("CameraType")->FirstChild() != NULL))
        g_program_setInfor.CameraType = QString(genElement->FirstChildElement("CameraType")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("UncoverTime") != NULL) && (genElement->FirstChildElement("UncoverTime")->FirstChild() != NULL))
        g_program_setInfor.UncoverTime = QString(genElement->FirstChildElement("UncoverTime")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("ShowThdTime") != NULL) && (genElement->FirstChildElement("ShowThdTime")->FirstChild() != NULL))
        g_program_setInfor.ShowThdTime = QString(genElement->FirstChildElement("ShowThdTime")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("rukouThreshold") != NULL) && (genElement->FirstChildElement("rukouThreshold")->FirstChild() != NULL))
        g_program_setInfor.rukouThreshold = QString(genElement->FirstChildElement("rukouThreshold")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("chukouThreshold") != NULL) && (genElement->FirstChildElement("chukouThreshold")->FirstChild() != NULL))
        g_program_setInfor.chukouThreshold = QString(genElement->FirstChildElement("chukouThreshold")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("GYKPosValue") != NULL) && (genElement->FirstChildElement("GYKPosValue")->FirstChild() != NULL))
        g_program_setInfor.GYKPosValue = QString(genElement->FirstChildElement("GYKPosValue")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("GYK0_or_COR1") != NULL) && (genElement->FirstChildElement("GYK0_or_COR1")->FirstChild() != NULL))
        g_program_setInfor.GYK0_or_COR1 = QString(genElement->FirstChildElement("GYK0_or_COR1")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("DebugTimmerSwitch") != NULL) && (genElement->FirstChildElement("DebugTimmerSwitch")->FirstChild() != NULL))
        g_program_setInfor.DebugTimmerSwitch = QString(genElement->FirstChildElement("DebugTimmerSwitch")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("DebugStartMil") != NULL) && (genElement->FirstChildElement("DebugStartMil")->FirstChild() != NULL))
        g_program_setInfor.DebugStartMil = QString(genElement->FirstChildElement("DebugStartMil")->FirstChild()->Value()).toDouble();
    if ((genElement->FirstChildElement("DebugMilInc1Dec0") != NULL) && (genElement->FirstChildElement("DebugMilInc1Dec0")->FirstChild() != NULL))
        g_program_setInfor.DebugMilInc1Dec0 = QString(genElement->FirstChildElement("DebugMilInc1Dec0")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("DebugMilStepLength") != NULL) && (genElement->FirstChildElement("DebugMilStepLength")->FirstChild() != NULL))
        g_program_setInfor.DebugMilStepLength = QString(genElement->FirstChildElement("DebugMilStepLength")->FirstChild()->Value()).toInt();

    if ((genElement->FirstChildElement("danxian") != NULL) && (genElement->FirstChildElement("danxian")->FirstChild() != NULL))
        g_program_setInfor.danxian = QString(genElement->FirstChildElement("danxian")->FirstChild()->Value());
    if ((genElement->FirstChildElement("shuang_xia_da") != NULL) && (genElement->FirstChildElement("shuang_xia_da")->FirstChild() != NULL))
        g_program_setInfor.shuang_xia_da = QString(genElement->FirstChildElement("shuang_xia_da")->FirstChild()->Value());
    if ((genElement->FirstChildElement("shuang_xia_xiao") != NULL) && (genElement->FirstChildElement("shuang_xia_xiao")->FirstChild() != NULL))
        g_program_setInfor.shuang_xia_xiao = QString(genElement->FirstChildElement("shuang_xia_xiao")->FirstChild()->Value());
    if ((genElement->FirstChildElement("shuang_shang_da") != NULL) && (genElement->FirstChildElement("shuang_shang_da")->FirstChild() != NULL))
        g_program_setInfor.shuang_shang_da = QString(genElement->FirstChildElement("shuang_shang_da")->FirstChild()->Value());
    if ((genElement->FirstChildElement("shuang_shang_xiao") != NULL) && (genElement->FirstChildElement("shuang_shang_xiao")->FirstChild() != NULL))
        g_program_setInfor.shuang_shang_xiao = QString(genElement->FirstChildElement("shuang_shang_xiao")->FirstChild()->Value());

    if ((genElement->FirstChildElement("xia_da_zuo") != NULL) && (genElement->FirstChildElement("xia_da_zuo")->FirstChild() != NULL))
        g_program_setInfor.xia_da_zuo = QString(genElement->FirstChildElement("xia_da_zuo")->FirstChild()->Value());
    if ((genElement->FirstChildElement("xia_xiao_zuo") != NULL) && (genElement->FirstChildElement("xia_xiao_zuo")->FirstChild() != NULL))
        g_program_setInfor.xia_xiao_zuo = QString(genElement->FirstChildElement("xia_xiao_zuo")->FirstChild()->Value());
    if ((genElement->FirstChildElement("shang_da_you") != NULL) && (genElement->FirstChildElement("shang_da_you")->FirstChild() != NULL))
        g_program_setInfor.shang_da_you = QString(genElement->FirstChildElement("shang_da_you")->FirstChild()->Value());
    if ((genElement->FirstChildElement("shang_xiao_you") != NULL) && (genElement->FirstChildElement("shang_xiao_you")->FirstChild() != NULL))
        g_program_setInfor.shang_xiao_you = QString(genElement->FirstChildElement("shang_xiao_you")->FirstChild()->Value());

    if ((genElement->FirstChildElement("pictureWH") != NULL) && (genElement->FirstChildElement("pictureWH")->FirstChild() != NULL))
        g_program_setInfor.pictureWH = QString(genElement->FirstChildElement("pictureWH")->FirstChild()->Value());
    if ((genElement->FirstChildElement("Set_show_WH") != NULL) && (genElement->FirstChildElement("Set_show_WH")->FirstChild() != NULL))
        g_program_setInfor.Set_show_WH = QString(genElement->FirstChildElement("Set_show_WH")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("Set_show_W") != NULL) && (genElement->FirstChildElement("Set_show_W")->FirstChild() != NULL))
        g_program_setInfor.Set_show_W = QString(genElement->FirstChildElement("Set_show_W")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("Set_show_H") != NULL) && (genElement->FirstChildElement("Set_show_H")->FirstChild() != NULL))
        g_program_setInfor.Set_show_H = QString(genElement->FirstChildElement("Set_show_H")->FirstChild()->Value()).toInt();

    if ((genElement->FirstChildElement("Table_rows") != NULL) && (genElement->FirstChildElement("Table_rows")->FirstChild() != NULL))
        g_program_setInfor.Table_rows = QString(genElement->FirstChildElement("Table_rows")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("big_mil_onoff") != NULL) && (genElement->FirstChildElement("big_mil_onoff")->FirstChild() != NULL))
        g_program_setInfor.big_mil_onoff = QString(genElement->FirstChildElement("big_mil_onoff")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("mil_threshold") != NULL) && (genElement->FirstChildElement("mil_threshold")->FirstChild() != NULL))
        g_program_setInfor.mil_threshold = QString(genElement->FirstChildElement("mil_threshold")->FirstChild()->Value()).toInt();
    if ((genElement->FirstChildElement("SaveZarr") != NULL) && (genElement->FirstChildElement("SaveZarr")->FirstChild() != NULL))
        g_program_setInfor.SaveZarr = QString(genElement->FirstChildElement("SaveZarr")->FirstChild()->Value()).toInt();

    ////相机信息CAMERAINFORSTRUCT//////////////////////////////////////////////////////////////
    TiXmlElement *preprocess_mark_dataElement = rootElement->FirstChildElement("camera_data");
    if (preprocess_mark_dataElement == NULL)
        return false;
    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        QString strItem = "camera";
        QString cam_xh = QString("%1").arg(i + 1, 2, 10, QChar('0'));
        strItem += cam_xh;
        char cPath[RouteInfoSize];
        strcpy(cPath, strItem.toLocal8Bit().data());

        if (preprocess_mark_dataElement->FirstChildElement(cPath) != NULL)
        {
            TiXmlElement *preprocess_ITEM = preprocess_mark_dataElement->FirstChildElement(cPath);
            if ((preprocess_ITEM->FirstChildElement("Name") != NULL) && (preprocess_ITEM->FirstChildElement("Name")->FirstChild() != NULL))
                g_cameraInfor[i].Name = QString(preprocess_ITEM->FirstChildElement("Name")->FirstChild()->Value());
            if ((preprocess_ITEM->FirstChildElement("CamLinkName") != NULL) && (preprocess_ITEM->FirstChildElement("CamLinkName")->FirstChild() != NULL))
                g_cameraInfor[i].CamLinkName = QString(preprocess_ITEM->FirstChildElement("CamLinkName")->FirstChild()->Value());
            if ((preprocess_ITEM->FirstChildElement("ExposureMode") != NULL) && (preprocess_ITEM->FirstChildElement("ExposureMode")->FirstChild() != NULL))
                g_cameraInfor[i].ExposureMode = QString(preprocess_ITEM->FirstChildElement("ExposureMode")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("ExposureTime") != NULL) && (preprocess_ITEM->FirstChildElement("ExposureTime")->FirstChild() != NULL))
                g_cameraInfor[i].ExposureTime = QString(preprocess_ITEM->FirstChildElement("ExposureTime")->FirstChild()->Value()).toDouble();

            if ((preprocess_ITEM->FirstChildElement("PictureHeight") != NULL) && (preprocess_ITEM->FirstChildElement("PictureHeight")->FirstChild() != NULL))
                g_cameraInfor[i].PictureHeight = QString(preprocess_ITEM->FirstChildElement("PictureHeight")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("ImageHeight") != NULL) && (preprocess_ITEM->FirstChildElement("ImageHeight")->FirstChild() != NULL))
                g_cameraInfor[i].ImageHeight = QString(preprocess_ITEM->FirstChildElement("ImageHeight")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("ImageWidth") != NULL) && (preprocess_ITEM->FirstChildElement("ImageWidth")->FirstChild() != NULL))
                g_cameraInfor[i].ImageWidth = QString(preprocess_ITEM->FirstChildElement("ImageWidth")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("TriggerMode") != NULL) && (preprocess_ITEM->FirstChildElement("TriggerMode")->FirstChild() != NULL))
                g_cameraInfor[i].TriggerMode = QString(preprocess_ITEM->FirstChildElement("TriggerMode")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("LineFormat") != NULL) && (preprocess_ITEM->FirstChildElement("LineFormat")->FirstChild() != NULL))
                g_cameraInfor[i].LineFormat = QString(preprocess_ITEM->FirstChildElement("LineFormat")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("LineFreq") != NULL) && (preprocess_ITEM->FirstChildElement("LineFreq")->FirstChild() != NULL))
                g_cameraInfor[i].LineFreq = QString(preprocess_ITEM->FirstChildElement("LineFreq")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("IsFlip") != NULL) && (preprocess_ITEM->FirstChildElement("IsFlip")->FirstChild() != NULL))
                g_cameraInfor[i].IsFlip = QString(preprocess_ITEM->FirstChildElement("IsFlip")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("FlipMode") != NULL) && (preprocess_ITEM->FirstChildElement("FlipMode")->FirstChild() != NULL))
                g_cameraInfor[i].FlipMode = QString(preprocess_ITEM->FirstChildElement("FlipMode")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("SaveTransform") != NULL) && (preprocess_ITEM->FirstChildElement("SaveTransform")->FirstChild() != NULL))
                g_cameraInfor[i].SaveTransform = QString(preprocess_ITEM->FirstChildElement("SaveTransform")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("Gain") != NULL) && (preprocess_ITEM->FirstChildElement("Gain")->FirstChild() != NULL))
                g_cameraInfor[i].Gain = QString(preprocess_ITEM->FirstChildElement("Gain")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("isSingle") != NULL) && (preprocess_ITEM->FirstChildElement("isSingle")->FirstChild() != NULL))
                g_cameraInfor[i].isSingle = QString(preprocess_ITEM->FirstChildElement("isSingle")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("isDouble") != NULL) && (preprocess_ITEM->FirstChildElement("isDouble")->FirstChild() != NULL))
                g_cameraInfor[i].isDouble = QString(preprocess_ITEM->FirstChildElement("isDouble")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("isColorCamera") != NULL) && (preprocess_ITEM->FirstChildElement("isColorCamera")->FirstChild() != NULL))
                g_cameraInfor[i].isColorCamera = QString(preprocess_ITEM->FirstChildElement("isColorCamera")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("ExposureTimeMin") != NULL) && (preprocess_ITEM->FirstChildElement("ExposureTimeMin")->FirstChild() != NULL))
                g_cameraInfor[i].ExposureTimeMin = QString(preprocess_ITEM->FirstChildElement("ExposureTimeMin")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("ExposureTimeMax") != NULL) && (preprocess_ITEM->FirstChildElement("ExposureTimeMax")->FirstChild() != NULL))
                g_cameraInfor[i].ExposureTimeMax = QString(preprocess_ITEM->FirstChildElement("ExposureTimeMax")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("GainMin") != NULL) && (preprocess_ITEM->FirstChildElement("GainMin")->FirstChild() != NULL))
                g_cameraInfor[i].GainMin = QString(preprocess_ITEM->FirstChildElement("GainMin")->FirstChild()->Value()).toInt();
            if ((preprocess_ITEM->FirstChildElement("GainMax") != NULL) && (preprocess_ITEM->FirstChildElement("GainMax")->FirstChild() != NULL))
                g_cameraInfor[i].GainMax = QString(preprocess_ITEM->FirstChildElement("GainMax")->FirstChild()->Value()).toInt();
        }
    }
    return true;
}
/////动作&菜单栏&工具栏/////////////////////////////////////////////////////////////////////
void TunnelDetection::createActions()
{
    // 1加载菜单
    // 1.1加载隧道简表
    loadTunnelAction = new QAction(QIcon(":/images/loadtunnel.png"), QStringLiteral("&设备简表"), this);
    loadTunnelAction->setStatusTip(QStringLiteral("加载设备简表"));
    connect(loadTunnelAction, SIGNAL(triggered()), this, SLOT(loadTunnelActionSlot()));

    // 2设置菜单
    // 1.2设置里程信息
    setMileageAction = new QAction(QIcon(":/images/mil.png"), QStringLiteral("&里程信息"), this);
    setMileageAction->setStatusTip(QStringLiteral("设置里程信息"));
    connect(setMileageAction, SIGNAL(triggered()), this, SLOT(setMileageActionSlot()));

    // 2.1相机配置
    cameraAction = new QAction(QIcon(":/images/camera.png"), QStringLiteral("&相机"), this);
    connect(cameraAction, SIGNAL(triggered()), this, SLOT(cameraActionSlot()));

    // 2.2照明配置
    lightAction = new QAction(QIcon(":/images/zhaoming.png"), QStringLiteral("&照明（防水盖）"), this);
    connect(lightAction, SIGNAL(triggered()), this, SLOT(lightActionSlot()));

    // 2.3限界配置
    limitAction = new QAction(QStringLiteral("&限界"), this);
    connect(limitAction, SIGNAL(triggered()), this, SLOT(limitActionSlot()));

    // 2.4补偿配置
    compensationAction = new QAction(QStringLiteral("&补偿"), this);
    connect(compensationAction, SIGNAL(triggered()), this, SLOT(compensationActionSlot()));

    // 2.5监控相机配置
    monitorAction = new QAction(QStringLiteral("&监控相机"), this);
    connect(monitorAction, SIGNAL(triggered()), this, SLOT(monitorActionSlot()));

    // 2.6数据实时状态
    dataRealTimeAction = new QAction(QStringLiteral("&数据实时状态"), this);
    connect(dataRealTimeAction, SIGNAL(triggered()), this, SLOT(dataRealTimeActionSlot()));

    // 2.7设置程序参数
    paraAction = new QAction(QIcon(""), QStringLiteral("&程序参数"), this);
    connect(paraAction, SIGNAL(triggered()), this, SLOT(paraActionSlot()));

    // 3系统菜单
    // 3.1自动模式
    autoModelAction = new QAction(QIcon(":/images/auto.png"), QStringLiteral("&自动模式"), this);
    autoModelAction->setStatusTip(QStringLiteral("自动模式"));
    connect(autoModelAction, SIGNAL(triggered()), this, SLOT(autoModelActionSlot()));

    // 3.2初始化
    connectAction = new QAction(QIcon(":/images/connect.ico"), QStringLiteral("&连接"), this);
    connectAction->setStatusTip(QStringLiteral("连接"));
    connect(connectAction, SIGNAL(triggered()), this, SLOT(connectActionSlot()));

    // 3.3断开连接
    disConnectAction = new QAction(QIcon(":/images/disconnect.png"), QStringLiteral("&断开连接"), this);
    disConnectAction->setStatusTip(QStringLiteral("断开连接"));
    connect(disConnectAction, SIGNAL(triggered()), this, SLOT(disConnectActionSlot()));

    // 3.4开始采集
    startAcqAction = new QAction(QIcon(":/images/start.png"), QStringLiteral("&开始采集"), this);
    startAcqAction->setStatusTip(QStringLiteral("开始采集"));
    connect(startAcqAction, SIGNAL(triggered()), this, SLOT(startAcqActionSlot()));

    // 3.5停止采集
    stopAcqAction = new QAction(QIcon(":/images/stop.png"), QStringLiteral("&停止采集"), this);
    stopAcqAction->setStatusTip(QStringLiteral("停止采集"));
    connect(stopAcqAction, SIGNAL(triggered()), this, SLOT(stopAcqActionSlot()));

    // 4帮助菜单
    // 4.1查看帮助
    viewHelpAction = new QAction(QStringLiteral("&查看帮助"), this);
    viewHelpAction->setStatusTip(QStringLiteral("查看帮助"));
    connect(viewHelpAction, SIGNAL(triggered()), this, SLOT(viewHelpActionSlot()));
}
void TunnelDetection::createMenus()
{
    // 加载菜单
    myLoadMenu = menuBar()->addMenu(QStringLiteral("&加载"));
    myLoadMenu->addAction(loadTunnelAction);

    // 设置菜单
    moduleMenu = menuBar()->addMenu(QStringLiteral("&设置"));
    moduleMenu->addAction(setMileageAction);
    moduleMenu->addAction(cameraAction);
    moduleMenu->addAction(lightAction);
    // 	moduleMenu->addAction(limitAction);
    // 	moduleMenu->addAction(compensationAction);
    moduleMenu->addAction(monitorAction);
    //	moduleMenu->addAction(dataRealTimeAction);
    moduleMenu->addAction(paraAction);

    // 系统菜单
    mysystemMenu = menuBar()->addMenu(QStringLiteral("&系统"));
    mysystemMenu->addAction(autoModelAction);
    mysystemMenu->addSeparator();
    // 	mysystemMenu->addAction(connectAction);
    // 	mysystemMenu->addAction(disConnectAction);
    // 	mysystemMenu->addSeparator();
    mysystemMenu->addAction(startAcqAction);
    mysystemMenu->addAction(stopAcqAction);

    // 帮助菜单
    myhelpMenu = menuBar()->addMenu(QStringLiteral("&帮助"));
    myhelpMenu->addAction(viewHelpAction);
}
void TunnelDetection::createToolbars()
{
    //ui->menubar->hide();
    // 加载菜单
    loadToolBar = addToolBar(QStringLiteral("设置")); //
    loadToolBar->setMovable(false);                   // 设置工具栏不可移动
    loadToolBar->setIconSize(QSize(64, 64));          // 设置图标大小
    loadToolBar->addAction(loadTunnelAction);

    // 设置菜单
    moduleToolBar = addToolBar(QStringLiteral("设置")); //
    moduleToolBar->setMovable(false);                   // 设置工具栏不可移动
    moduleToolBar->setIconSize(QSize(64, 64));          // 设置图标大小
    moduleToolBar->addAction(setMileageAction);
    moduleToolBar->addAction(cameraAction);
    moduleToolBar->addAction(lightAction);

    // 系统菜单
    systemToolBar = addToolBar(QStringLiteral("系统")); //
    systemToolBar->setMovable(false);                   // 设置工具栏不可移动
    systemToolBar->setIconSize(QSize(64, 64));          // 设置图标大小
    systemToolBar->addAction(autoModelAction);
    //	systemToolBar->addAction(connectAction);
    //	systemToolBar->addAction(disConnectAction);
    systemToolBar->addAction(startAcqAction);
    systemToolBar->addAction(stopAcqAction);
}
void TunnelDetection::createStatusBar()
{
    statusbar = statusBar(); // 获取状态栏
    //	QStatusBar* statusbar = new QStatusBar(this);
    //	QPushButton* home = new QPushButton(statusbar);
    //	home->setText("HOME");
    //	statusbar->addPermanentWidget(home, 0);
    statusbar->show();
    QPushButton *mulPageBtn = new QPushButton;
    mulPageBtn->setText("页面浏览");
    this->setStatusBar(statusbar);
    QLabel *pageName = new QLabel;
    pageName->setText("");
    pageName->setAlignment(Qt::AlignCenter);
    statusbar->addWidget(pageName, 5);
    QLabel *spaceHolder = new QLabel;
    spaceHolder->setText(QStringLiteral("主界面"));
    spaceHolder->setAlignment(Qt::AlignCenter);
    //	statusbar->addWidget(spaceHolder, 1);

    statusbar->showMessage("Ready!");

#if 0
	/*2、状态栏的临时信息、一般信息、永久信息的显示*/
	QStatusBar* bar = statusBar(); //获取状态栏

	msgLable = new QLabel;
	msgLable->setMinimumSize(200, 20/*msgLable->sizeHint()*/);
	msgLable->setAlignment(Qt::AlignHCenter);
	QString strmsg = QStringLiteral("当前选中的隧道是：");
	 
	msgLable->setText(strmsg);
	bar->addPermanentWidget(msgLable);

//	bar->show();

#endif
}
/////主界面初始化/////////////////////////////////////////////////////////////////////
void TunnelDetection::onRadioClickSlot()
{
    if ((ui->radioButton_shang->isChecked()) && (!ui->radioButton_xia->isChecked()))
    {
        m_strXB = QStringLiteral("上行");
    }
    else if ((!ui->radioButton_shang->isChecked()) && (ui->radioButton_xia->isChecked()))
    {
        m_strXB = QStringLiteral("下行");
    }

    WriteDefaultPara();
}
void TunnelDetection::onmilRadioClickSlot()
{
    if ((ui->radioButton_milInc->isChecked()) && (!ui->radioButton_milDec->isChecked()))
    {
        m_strMilDir = QStringLiteral("增加");
    }
    else if ((!ui->radioButton_milInc->isChecked()) && (ui->radioButton_milDec->isChecked()))
    {
        m_strMilDir = QStringLiteral("减小");
    }
    WriteDefaultPara();
    if (pSetMileageDlg != NULL)
    {
        pSetMileageDlg->setMilDir(m_strMilDir);
    }
}
void TunnelDetection::onchetouRadioClickSlot()
{
    if ((ui->radioButton_chetouBig->isChecked()) && (!ui->radioButton_chetouSmall->isChecked()))
    {
        m_strChetou = QStringLiteral("大里程");
    }
    else if ((!ui->radioButton_chetouBig->isChecked()) && (ui->radioButton_chetouSmall->isChecked()))
    {
        m_strChetou = QStringLiteral("小里程");
    }
    WriteDefaultPara();
}
/////动作的槽函数/////////////////////////////////////////////////////////////////////
void TunnelDetection::loadTunnelActionSlot()
{
    QString filename = QFileDialog::getOpenFileName(this, "Choose csv File", gprogram_tempInfor.tmp_path_file, ("csv File(*.*)"));
    if (filename.isEmpty())
        return;
    m_csvPathFileName = filename;
    loadledgerFile(filename);
}
void TunnelDetection::loadledgerFile(QString filename)
{
    // 取出文件名中的信息
    QFileInfo qfinfor(filename);
    QString strTmp = qfinfor.fileName();
    m_ledgerFileName = Get_imgName_infor(strTmp);

    // 将excel里的台账数据，读入到全局变量gtunnel_data_struct
    QFile csvfile;
    QString strname = filename; // QStringLiteral("高培1号.csv");
    csvfile.setFileName(strname);
    //csvfile.open(QIODevice::ReadOnly);
    //QTextStream txtInPut(&csvfile);

    csvfile.open(QIODevice::ReadOnly);
    QByteArray data = csvfile.readAll();

    QString csvText = QString::fromUtf8(data);
    if (csvText.contains(QChar::ReplacementCharacter))
    {
        csvText = QString::fromLocal8Bit(data);
    }

    QStringList tempOption = csvText.split("\n");

    //QStringList tempOption = txtInPut.readAll().split("\n"); // 每行以\n区分
    QStringList tempbarbiaotou;
    if (!tempOption.isEmpty())
    {
        tempbarbiaotou = tempOption.at(0).split(","); // 取回表头
    }

    // 取回检测结果，内容
    for (int i = 1; i < tempOption.count(); i++)
    {
        QStringList tempbar = tempOption.at(i).split(","); // 一行中的单元格以，区分
        if (tempbar.length() != tempbarbiaotou.length())
        {
            break;
        }
        tunnel_data_struct tunnel_data;
        tunnel_data.ixh = tempbar[0].toInt();    // 序号
        tunnel_data.object_type = tempbar[1];    // 检测对象类型：隧道、声屏障
        tunnel_data.bureau_name = tempbar[2];    // 局名
        tunnel_data.division_name = tempbar[3];  // 段名
        tunnel_data.lineName = tempbar[4];       // 线路名称
        tunnel_data.lineLevel = tempbar[5];      // 线路等级
        tunnel_data.section_name = tempbar[6];   // 区间
        tunnel_data.tunnelNumber = tempbar[7];   // 设备号
        tunnel_data.tunnelName = tempbar[8];     // 设备名称
        tunnel_data.tunnelCode = tempbar[9];     // 设备编码
        tunnel_data.tunnelXingbie = tempbar[10]; // excel中的行别，指的是程序和comInfo中的隧道单双线，（原来是这样，新版程序改成和意思相对应了，这就需要新的处理程序重新适配，或兼容）
        tunnel_data.tunnelType = tempbar[11];    // excel中的线别,指的是程序和comInfo中的隧道上下行

        // 起始里程有时是double，单位是km，有时是k+
        int KPos_s = tempbar[12].indexOf('K', 0);
        if (KPos_s == -1)
        {
            tunnel_data.tunnel_startMil = tempbar[12].toFloat();
        }
        else
        {
            tunnel_data.tunnel_startMil = k_mil_to_km(tempbar[12]);
        }
        // 中心里程有时是double，单位是km，有时是k+
        int KPos_c = tempbar[13].indexOf('K', 0);
        if (KPos_c == -1)
        {
            tunnel_data.tunnel_centreMil = tempbar[13].toFloat(); //
        }
        else
        {
            tunnel_data.tunnel_centreMil = k_mil_to_km(tempbar[13]);
        }
        // 终止里程有时是double，单位是km，有时是k+
        int KPos_e = tempbar[14].indexOf('K', 0);
        if (KPos_e == -1)
        {
            tunnel_data.tunnel_endMil = tempbar[14].toFloat(); //
        }
        else
        {
            tunnel_data.tunnel_endMil = k_mil_to_km(tempbar[14]);
        }

        tunnel_data.tunnel_length = qRound(tempbar[15].toFloat());                      // 设备长度
        tunnel_data.object_height = tempbar[16].toFloat();                              // 高度
        tunnel_data.equipment_name = tempbar[17];                                       // 检测设备
        tunnel_data.person_name = tempbar[18].remove(QChar('\r'), Qt::CaseInsensitive); // 检测人员

        gtunnel_ledger_struct.push_back(tunnel_data);

        if (gtunnel_ledger_struct.size() > 0)
        {
            // 显示线路名称
            m_strLineName = gtunnel_ledger_struct.at(0).lineName;
            ui->lineEdit_lineName->setText(m_strLineName);

            m_bureau_name = gtunnel_ledger_struct.at(0).bureau_name;
        }

        // 显示隧道简表
        showListControl(gtunnel_ledger_struct);

        QFileInfo qfi(filename);
        gprogram_tempInfor.tmp_path_file = qfi.absolutePath();
    }
    csvfile.close();
    WriteDefaultPara();
}
void TunnelDetection::init_tableWidget()
{
    m_tableWidget_cow = 13;
    ui->tableWidget->setColumnCount(m_tableWidget_cow);
    ui->tableWidget->setRowCount(g_program_setInfor.Table_rows);

    QStringList headerlist;
    headerlist << QStringLiteral("序号") << QStringLiteral("类型") << QStringLiteral("设备号") << QStringLiteral("设备名称")
               << QStringLiteral("线别") << QStringLiteral("起点里程") << QStringLiteral("终点里程")
               << QStringLiteral("全长") << QStringLiteral("高度") << QStringLiteral("设备间隔")
               << QStringLiteral("检测状态") << QStringLiteral("开始存储里程")
               << QStringLiteral("停止存储里程");
    ui->tableWidget->setHorizontalHeaderLabels(headerlist);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(false);

    const int tableWidth = qMax(760, m_tableWidget_width - 24);
    const std::array<int, 13> columnWidths = {
        tableWidth * 4 / 100,  // 序号
        tableWidth * 6 / 100,  // 类型
        tableWidth * 6 / 100,  // 设备号
        tableWidth * 14 / 100, // 设备名称
        tableWidth * 5 / 100,  // 线别
        tableWidth * 9 / 100,  // 起点里程
        tableWidth * 9 / 100,  // 终点里程
        tableWidth * 5 / 100,  // 全长
        tableWidth * 5 / 100,  // 高度
        tableWidth * 7 / 100,  // 设备间隔
        tableWidth * 12 / 100, // 检测结果
        tableWidth * 9 / 100,  // 开始存储里程
        tableWidth * 9 / 100   // 停止存储里程
    };

    int usedWidth = 0;
    for (int i = 0; i < m_tableWidget_cow - 1; ++i)
    {
        ui->tableWidget->setColumnWidth(i, columnWidths[i]);
        usedWidth += columnWidths[i];
    }
    ui->tableWidget->setColumnWidth(m_tableWidget_cow - 1, qMax(70, tableWidth - usedWidth));
}
void TunnelDetection::showListControl(std::vector<tunnel_data_struct> td_tmp)
{
    ui->tableWidget->clear();
    init_tableWidget();
    for (int i = 0; i < td_tmp.size(); i++)
    {
        for (int j = 0; j < m_tableWidget_cow; j++)
        {
            QTableWidgetItem *mil_Item = new QTableWidgetItem("");
            ui->tableWidget->setItem(i, j, mil_Item);
            ui->tableWidget->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        }
    }
    for (int i = 0; i < td_tmp.size(); i++)
    {
        tunnel_data_struct td = td_tmp.at(i);

        ui->tableWidget->item(i, 0)->setText(QString::number(i + 1, 10));
        ui->tableWidget->item(i, 1)->setText(td.object_type);
        ui->tableWidget->item(i, 2)->setText(td.tunnelNumber);
        ui->tableWidget->item(i, 3)->setText(td.tunnelName);
        ui->tableWidget->item(i, 4)->setText(td.tunnelType);
        ui->tableWidget->item(i, 5)->setText(QString::number(td.tunnel_startMil, 'f', 3));
        ui->tableWidget->item(i, 6)->setText(QString::number(td.tunnel_endMil, 'f', 3));
        ui->tableWidget->item(i, 7)->setText(QString::number(td.tunnel_length, 10));
        // 高度
        if (td.object_type == QStringLiteral("声屏障"))
        {
            ui->tableWidget->item(i, 8)->setText(QString::number(td.object_height, 'f', 2));
        }
        else if (td.object_type == QStringLiteral("隧道"))
        {
            ui->tableWidget->item(i, 8)->setText(QStringLiteral("/"));
        }

        ui->tableWidget->item(i, 9)->setText(QStringLiteral("0"));  // 间隔距离
        ui->tableWidget->item(i, 10)->setText(QStringLiteral("/")); // 检测结果
        ui->tableWidget->item(i, 11)->setText(QStringLiteral(""));  // 开始存储里程
        ui->tableWidget->item(i, 12)->setText(QStringLiteral(""));  // 停止存储里程
    }
}
void TunnelDetection::tableContexMenuRequested(const QPoint &pos)
{
    QTableWidgetItem *selectedItem = ui->tableWidget->itemAt(pos);
    if (selectedItem != nullptr)
    {
        originDataTableContextMenu->exec(QCursor::pos());
    }
}
void TunnelDetection::slotMenuClicked(QAction *act)
{
    if (act == originDataTableActionDel)
    {
        int currentRow = ui->tableWidget->currentIndex().row();
        // 		QString str1 = ui->tableWidget->item(currentRow, 0)->text();//取出当前行第1列内容
        // 		QString str2 = ui->tableWidget->item(currentRow, 7)->text();//取出当前行第2列内容
        // 		QString str = ui->tableWidget->item(currentRow, 3)->text();//取出字符串

        // 																  //	int str3 = ui->tableWidget->item(currentRow, 2)->text.toInt();//取出当前行第3列内容

        // 		int i = 9;
        //
        // 		int num = str.toInt();//转成int

        m_idx = currentRow;
        QString strmsg = QStringLiteral("当前选中的隧道是：%1").arg(gtunnel_ledger_struct.at(m_idx).tunnelNumber);
        strmsg += gtunnel_ledger_struct.at(m_idx).tunnelName;

         //	msgLable->setText(strmsg);
        statusbar->showMessage(strmsg);
    }
}
void TunnelDetection::get_tunnel_interval()
{
    if (m_strMilDir == QStringLiteral("增加"))
    {
        for (int i = 1; i < gtunnel_ledger_struct.size(); i++)
        {
            tunnel_data_struct td1 = gtunnel_ledger_struct.at(i - 1);
            tunnel_data_struct td2 = gtunnel_ledger_struct.at(i);
            double dinterval = td2.tunnel_startMil - td1.tunnel_endMil;
            dinterval = dinterval * 1000;
            ui->tableWidget->item(i, icow_table_interval)->setText(QString::number((int)dinterval, 10));
        }
    }
    else if (m_strMilDir == QStringLiteral("减小"))
    {
        for (int i = gtunnel_ledger_struct.size() - 1 - 1; i > 0; i--)
        {
            tunnel_data_struct td1 = gtunnel_ledger_struct.at(i);
            tunnel_data_struct td2 = gtunnel_ledger_struct.at(i + 1);
            double dinterval = td2.tunnel_startMil - td1.tunnel_endMil;
            dinterval = dinterval * 1000;
            ui->tableWidget->item(i, icow_table_interval)->setText(QString::number((int)dinterval, 10));
        }
    }
}
void TunnelDetection::setMileageActionSlot()
{
    pSetMileageDlg->show();
}
void TunnelDetection::Mil_over_slots(QString ss, QString ee)
{
    m_strStartMil = ss;
    m_strEndMil = ee;
    LineEdit_smil->setText(ss);
    LineEdit_emil->setText(ee);
    if (LineEdit_GD_smil != NULL && LineEdit_GD_emil != NULL)
    {
        LineEdit_GD_smil->setText(ss);
        LineEdit_GD_emil->setText(ee);
    }

    if (g_program_setInfor.ObjectType == QStringLiteral("无砟轨道"))
    {
        if ((!gtunnel_ledger_struct.empty()) && (m_idx == 0))
        {
            gtunnel_ledger_struct.at(m_idx).tunnel_startMil = ss.toDouble();
            gtunnel_ledger_struct.at(m_idx).tunnel_endMil = ee.toDouble();
            double dlength = 1000 * abs(ss.toDouble() - ee.toDouble());
            int ilength = (int)(dlength);
            gtunnel_ledger_struct.at(m_idx).tunnel_length = ilength;
        }
    }

    qDebug() << QStringLiteral("设置起始里程：") << m_strStartMil;
    qDebug() << QStringLiteral("设置终止里程：") << m_strEndMil;
    WriteDefaultPara();
}
void TunnelDetection::Mil_time_slots(double dmil, double dspeed)
{
    if (g_program_setInfor.ObjectType == QStringLiteral("无砟轨道"))
    {
        const MILEAGEINFORSTRUCT ms = m_milDirect->GetMileageInfor();
        const double pulseMileageKm = static_cast<double>(ms.iPulseCount) * g_program_setInfor.PulseWidth / 1000.0 / 1000.0;
        const double showmil = m_strStartMil.toDouble() + pulseMileageKm;
        ui->lcdNumber_nmil->display(showmil); // 显示数字123456
        ui->lcdNumber_spe->display(dspeed);   // 显示数字123456
    }
}
void TunnelDetection::cameraActionSlot()
{
    pSetCameraDlg->show_us();
    pSetCameraDlg->show_gain();
    pSetCameraDlg->show();
}
//////无砟轨道小车开闭照明动作////////////////////////////////////////////////////////////////////
void TunnelDetection::pushButton_GD_openzhaomingSlot()
{
    m_milDirect->JieTongCmd(QMilThd::A_Camera01_20_21_R3_0x08); // 相机1
    m_milDirect->JieTongCmd(QMilThd::A_Camera02_23_24_R4_0x10); // 相机2
    m_milDirect->JieTongCmd(QMilThd::A_Camera03_25_26_R5_0x20); // 相机3
    m_milDirect->JieTongCmd(QMilThd::A_Camera04_27_28_R6_0x40); // 相机4
}
void TunnelDetection::pushButton_GD_closemingzhaoSlot()
{
    m_milDirect->DuankaiCmd(QMilThd::A_Camera01_20_21_R3_0x08); // 相机1
    m_milDirect->DuankaiCmd(QMilThd::A_Camera02_23_24_R4_0x10); // 相机2
    m_milDirect->DuankaiCmd(QMilThd::A_Camera03_25_26_R5_0x20); // 相机3
    m_milDirect->DuankaiCmd(QMilThd::A_Camera04_27_28_R6_0x40); // 相机4
}
void TunnelDetection::lightActionSlot()
{
    pSetLightDlg->show();
}
void TunnelDetection::limitActionSlot()
{
}
void TunnelDetection::compensationActionSlot()
{
}
void TunnelDetection::monitorActionSlot()
{
}
void TunnelDetection::paraActionSlot()
{
    pSetParaDlg->show();
}
void TunnelDetection::dataRealTimeActionSlot()
{
    if (pRrealTimeState == NULL)
    {
        pRrealTimeState = new QRrealTimeState(this);
        pRrealTimeState->setWindowTitle(QStringLiteral("数据实时状态对话框"));
    }
    pRrealTimeState->show();
}
//////串口接收GYK里程////////////////////////////////////////////////////////////////////
void TunnelDetection::sp_gyk(void)
{
    // 创建QSerialPort对象
    serialPort = new QSerialPort(this);

    // 寻找并配置串口
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QString strPort = QStringLiteral("COM") + QString::number(g_program_setInfor.MilComPort, 10);
        if (info.portName() == strPort)
        { // 替换为你的串口名称
            serialPort->setPort(info);
            serialPort->setBaudRate(g_program_setInfor.MilComBaud);
            serialPort->setDataBits(QSerialPort::Data8);
            serialPort->setParity(QSerialPort::NoParity);
            serialPort->setStopBits(QSerialPort::OneStop);
            serialPort->setFlowControl(QSerialPort::NoFlowControl);
            break;
        }
    }

    // 打开串口
    if (!serialPort->open(QIODevice::ReadWrite))
    {
        qDebug() << "Error opening serial port";
        return;
    }

    // 连接信号槽来接收数据
    // 	connect(serialPort, &QSerialPort::readyRead, [&]() {
    // 		QByteArray data = serialPort->readAll();
    // 		// 处理接收到的数据
    // 		qDebug() << "Received:" << data.toHex();
    // 	});
    connect(serialPort, &QSerialPort::readyRead, this, &TunnelDetection::readData);
    //	connect(serialPort, SIGNAL(readyRead()), this, SLOT(readData()));
}
void TunnelDetection::readData()
{
    if (g_program_setInfor.MilType == 0) // 大铁GYK
    {
        gyk_readData();
    }
    else if (g_program_setInfor.MilType == 1) // 朔黄
    {
        // 处理接收到的数据
        // 解析出实时里程和速度
        // 正常数据帧长20字节
        // AA 55 3A 65 ED 24 65 00 00 00 03 B3 1F 02 01 6F 1A 00 01 D4
        QByteArray recvData = serialPort->readAll();
        //	qDebug() << "Received1:" << recvData.toHex();
        int recv_length = recvData.size();
        // 	qDebug() << "Length using size1(): " << recv_length;
        responseData += recvData;
        int response_length = responseData.size();
        // 	qDebug() << "Received2:" << responseData.toHex();
        // 	qDebug() << "Length using size2(): " << response_length;

        QByteArray ba; // 数据帧头1002
        ba.resize(3);  // 设置字节数组的大小
        ba[0] = 0xAA;  // 赋值
        ba[1] = 0x55;
        ba[2] = 0x3A;

        int startPos = responseData.indexOf(ba);
        if (startPos == -1) // 没有帧头
        {
            responseData.clear();
            return;
        }

        // 此时有帧头
        // 再通过长度判断一下，不符合的返回，重新累加接收
        if (responseData.size() < 15) //
        {
            return;
        }

        {
            QByteArray baohan_tou_Data = responseData; // responseData.mid(startPos, responseData.length() - startPos);
            qDebug() << "Received lijc:" << baohan_tou_Data.toHex();
            qDebug() << "Length using size(): " << baohan_tou_Data;

            QByteArray shijian = baohan_tou_Data.mid(3, 4);     // 第3字节，长度4个
            QByteArray sudu = baohan_tou_Data.mid(7, 2);        // 第7字节，长度2个
            QByteArray gonglibiao = baohan_tou_Data.mid(11, 3); // 第11字节，长度4个

            qDebug() << "Received time:" << shijian.toHex();
            qDebug() << "Received speed:" << sudu.toHex();
            qDebug() << "Received mil:" << gonglibiao.toHex();

            // 恢复公里标数值
            QByteArray milByteArray;
            milByteArray = gonglibiao.mid(2, 1) + gonglibiao.mid(1, 1) + gonglibiao.mid(0, 1);
            qDebug() << "Received:" << gonglibiao.toHex();

            // 界面刷新显示里程
            bool ok;
            int value1 = milByteArray.toHex().toInt(&ok, 16);
            double m_dmil_GYK_tmp = value1 / 1000.0;
            //	qDebug() << "Received:" << m_dmil_GYK_tmp;

            // 过滤不正常的gyk里程
            if (!gtunnel_ledger_struct.empty())
            {
                // 如果里程是正常的，在此赋值更新
                m_dmil_GYK = m_dmil_GYK_tmp;
            }
            else
            {
                m_dmil_GYK = m_dmil_GYK_tmp;
            }

            // 		QString tmp = QString::number(m_dmil_GYK, 'f', 3);
            // 		ui.lcdNumber_nmil->display(tmp);
            //	qDebug() << "Received mil:" << tmp;
            m_dmil_GYK_his = m_dmil_GYK;

            // 恢复速度数值
            QByteArray speedByteArray;
            speedByteArray = sudu.mid(1, 1) + sudu.mid(0, 1);

            // 界面刷新显示速度
            bool ok1;
            m_dspeed_GYK = speedByteArray.toHex().toInt(&ok1, 16);
            //	ui.lcdNumber_spe->display(m_dspeed_GYK);
            qDebug() << "Received speed :" << m_dspeed_GYK;
        }
        responseData.clear();
        //	serialPort->clear(); //清空缓存区
        m_milDirect->SetZero();
    }
}
void TunnelDetection::gyk_readData()
{
    // 处理接收到的数据
    // 解析出实时里程和速度
    // 正常数据帧长172字节
    // 1002005011000100050038006701000000000000000000000000000017000000000000000000010000cf39c005129c526100000006040000ff5d19115f002000020000000000002bfd2a082232630100007efc0b1003
    // 100200501100010005003800670080002020202000000bd50309a500530005000000000000002ee200683930009f20a86146000020040c1010048748065f0020000200000009c97a0715071c3109e901000040517c1003100200129100a500020122ea0000033e0009a5001010781003
    QByteArray recvData = serialPort->readAll();
    //	qDebug() << "Received1:" << recvData.toHex();
    int recv_length = recvData.size();
    // 	qDebug() << "Length using size1(): " << recv_length;
    responseData += recvData;
    int response_length = responseData.size();
    // 	qDebug() << "Received2:" << responseData.toHex();
    // 	qDebug() << "Length using size2(): " << response_length;

    QByteArray ba; // 数据帧头1002
    ba.resize(2);  // 设置字节数组的大小
    ba[0] = 0x10;  // 赋值
    ba[1] = 0x02;

    int startPos = responseData.indexOf(ba);
    if (startPos == -1) // 没有帧头
    {
        responseData.clear();
        return;
    }
    QByteArray wei; // 尾巴1003
    wei.resize(2);  // 设置字节数组的大小
    wei[0] = 0x10;  // 赋值
    wei[1] = 0x03;

    int stopPos = responseData.indexOf(wei);
    if (stopPos == -1) // 没有帧尾
    {
        return;
    }

    // 此时既有有帧头，也有帧尾
    qDebug() << "Received org:" << responseData.toHex();
    qDebug() << "Length using size(): " << response_length;

    // 有头有尾，就截取
    QByteArray baohan_tou_wei_Data = responseData.mid(startPos, stopPos + 1 + 3);
    responseData = baohan_tou_wei_Data;
    qDebug() << "Received jiequ:" << responseData.toHex();
    qDebug() << "Length using size(): " << responseData.size();

    // 通过帧定义长度判断一下，不符合的清除，重新接收
    if (responseData.size() < 80) // 协议给的帧85,86？
    {
        responseData.clear();
        return;
    }
    QByteArray dle; // DLE判断1010
    dle.resize(2);
    dle[0] = 0x10;
    dle[1] = 0x10;

    int fromStartPos = 0;
    while (1)
    {
        int startPos_dle = responseData.indexOf(dle, fromStartPos);
        if (startPos_dle == -1)
        {
            break;
        }
        // 去掉一个DLE（10）
        responseData.remove(startPos_dle, 1);
        qDebug() << "Received del DLE:" << responseData.toHex();
        qDebug() << "Length using size(): " << responseData.size();

        fromStartPos = startPos_dle + 1;
    }
    //	if (startPos != -1)
    {
        //	startPos = responseData.indexOf(ba);
        QByteArray baohan_tou_Data = responseData; // responseData.mid(startPos, responseData.length() - startPos);
        qDebug() << "Received lijc:" << baohan_tou_Data.toHex();
        qDebug() << "Length using size(): " << baohan_tou_Data;

        QByteArray shijian = baohan_tou_Data.mid(45, 4);    // 第46字节，长度4个
        QByteArray sudu = baohan_tou_Data.mid(49, 3);       // 第50字节，长度3个
        QByteArray gonglibiao = baohan_tou_Data.mid(57, 3); // 第58字节，长度3个

        qDebug() << "Received time:" << shijian.toHex();
        qDebug() << "Received speed:" << sudu.toHex();
        qDebug() << "Received mil:" << gonglibiao.toHex();

        // 恢复公里标数值
        QByteArray milByteArray;
        milByteArray = gonglibiao.mid(2, 1) + gonglibiao.mid(1, 1) + gonglibiao.mid(0, 1);
        qDebug() << "Received:" << gonglibiao.toHex();

        // 去掉B23和B22
        milByteArray[0] = milByteArray[0] & 0x3F;
        milByteArray[1] = milByteArray[1] & 0xFF;
        milByteArray[2] = milByteArray[2] & 0xFF;
        qDebug() << "Received:" << milByteArray.toHex();

        // 界面刷新显示里程
        bool ok;
        int value1 = milByteArray.toHex().toInt(&ok, 16);
        double m_dmil_GYK_tmp = value1 / 1000.0;
        //	qDebug() << "Received:" << m_dmil_GYK_tmp;

        // 过滤不正常的gyk里程
        if (!gtunnel_ledger_struct.empty())
        {
            // 如果里程是正常的，在此赋值更新
            m_dmil_GYK = m_dmil_GYK_tmp;
        }
        else
        {
            m_dmil_GYK = m_dmil_GYK_tmp;
        }

        m_dmil_GYK_his = m_dmil_GYK;

        // 恢复速度数值
        QByteArray speedByteArray;
        speedByteArray = sudu.mid(2, 1) + sudu.mid(1, 1) + sudu.mid(0, 1);
        speedByteArray[0] = speedByteArray[0] & 0x00;
        speedByteArray[1] = speedByteArray[1] & 0x03;
        speedByteArray[2] = speedByteArray[2] & 0xFF;

        // 界面刷新显示速度
        bool ok1;
        m_dspeed_GYK = speedByteArray.toHex().toInt(&ok1, 16);
        //	ui.lcdNumber_spe->display(m_dspeed_GYK);
        qDebug() << "Received speed :" << m_dspeed_GYK;
    }
    responseData.clear();
    //	serialPort->clear(); //清空缓存区
    m_milDirect->SetZero();
}
void TunnelDetection::timerUpDateprogress()
{
    // 获取系统现在的时间
    QDateTime dateTime = QDateTime::currentDateTime();
    ui->lineEdit_decTime->setText(dateTime.toString("yyyy-MM-dd HH:mm:ss"));

    // 刷新显示里程+速度
    if (pSetMileageDlg->m_milMode == 0)
    {
        ui->label_nmil->setText(QStringLiteral("GYK里程"));
        ui->label_spe->setText(QStringLiteral("GYK速度"));
        ui->lcdNumber_nmil->display(QString::number(m_dmil_GYK_Cor, 'f', 3));
        ui->lcdNumber_spe->display(m_dspeed_GYK);
    }
    else if (pSetMileageDlg->m_milMode == 1)
    {
        ui->label_nmil->setText(QStringLiteral("脉冲计数里程"));
        ui->label_spe->setText(QStringLiteral("脉冲计数速度"));
        if (g_program_setInfor.ObjectType != QStringLiteral("无砟轨道"))
        {
            MILEAGEINFORSTRUCT ms = m_milDirect->GetMileageInfor();
            ui->lcdNumber_nmil->display(QString::number(m_dmil_GYK_Cor, 'f', 2)); // 里程，单位是米
            ui->lcdNumber_spe->display(QString::number(m_dspeed_GYK, 'f', 2));    // 速度,公里/小时
        }
    }
}
void TunnelDetection::timerUpDateGYKMilprogress()
{
    if (g_program_setInfor.DebugMilInc1Dec0 == 1)
    {
        m_dmil_GYK += g_program_setInfor.DebugMilStepLength / 1000.0; // 每秒增加20m
    }
    else if (g_program_setInfor.DebugMilInc1Dec0 == 0)
    {
        m_dmil_GYK -= g_program_setInfor.DebugMilStepLength / 1000.0; // 每秒减少20m
    }

    QString tmp = QString::number(m_dmil_GYK, 'f', 3);
    ui->lcdNumber_nmil->display(tmp);
    m_milDirect->SetZero();
}
void TunnelDetection::timerprogress_gykMil_accuracyprogress()
{
    MILEAGEINFORSTRUCT m_mileageInfor = m_milDirect->GetMileageInfor();

#if 0


	//m_dmil_GYK, m_pulse_save,g_milDirect.iPulseCount,g_program_setInfor.PulseWidth
	double mytmp = 0;
	if (abs(m_gyksave - m_dmil_GYK) == 0)
	{
		mytmp = m_gyksave;
	}
	else
	{
		mytmp = m_dmil_GYK;
		MILEAGEINFORSTRUCT ms = g_milDirect.GetMileageInfor();
		m_pulse_save = ms.iPulseCount;
	}
	if (m_strMilDir == QStringLiteral("增加"))
	{
	 //给当前m_dmil_GYK+脉冲数*脉冲宽度
		MILEAGEINFORSTRUCT ms = g_milDirect.GetMileageInfor();
		__int64 pchazhi = ms.iPulseCount - m_pulse_save;
		tmp_dmil_GYK = mytmp + pchazhi*g_program_setInfor.PulseWidth / 1000 / 1000;
		QString tmp = QString::number(m_dmil_GYK, 'f', 3);
	//	ui.lcdNumber_spe->display(tmp);

	}
	else if (m_strMilDir == QStringLiteral("减小"))
	{
		MILEAGEINFORSTRUCT ms = g_milDirect.GetMileageInfor();
		__int64 pchazhi = ms.iPulseCount - m_pulse_save;
		tmp_dmil_GYK = mytmp - pchazhi*g_program_setInfor.PulseWidth / 1000 / 1000;
		QString tmp = QString::number(tmp_dmil_GYK, 'f', 3);
	// 	ui.lcdNumber_spe->display(tmp);

		m_pulse_save = ms.iPulseCount;
		m_gyksave = mytmp;
	
	}
#endif
}
void TunnelDetection::timer_diskcapacity_progress()
{
    QString RootPath = g_program_setInfor.DataDestPath;
    int nPos1 = RootPath.indexOf('\\');
    int nPos2 = RootPath.indexOf('/');
    QString strRootPath;
    if (nPos1 > 0)
    {
        strRootPath = RootPath.left(nPos1 + 1);
    }
    if (nPos2 > 0)
    {
        strRootPath = RootPath.left(nPos2 + 1);
    }
    qDebug() << RootPath;
    qDebug() << strRootPath;
    QStorageInfo *storage = new QStorageInfo(strRootPath);
    qDebug() << "size:" << storage->bytesFree() / 1024 / 1024 / 1024 << "GB";
    qDebug() << "size:" << storage->bytesTotal() / 1024 / 1024 / 1024 << "GB";

    qint64 BFree = storage->bytesFree() / 1024 / 1024 / 1024;
    qint64 BTotal = storage->bytesTotal() / 1024 / 1024 / 1024;

    QString strcontext = QString(QStringLiteral("%1盘，剩余%2GB，总量%3GB")).arg(strRootPath.left(1)).arg(BFree).arg(BTotal);
    LineEdit_cunchu->setText(strcontext);
}

/////连接相机/////////////////////////////////////////////////////////////////////
void TunnelDetection::connectActionSlot()
{
    if (m_isBreakThd == true)
    {
        return;
    }
    // 连接相机
    startThd();
}
void TunnelDetection::startThd()
{
    m_isBreakThd = true;
    m_isCanRun = false;
    std::thread *pt1(new std::thread(runThdFun, this));
}
void TunnelDetection::stopThd()
{
    m_isBreakThd = false;
    m_isCanRun = true;
}
void TunnelDetection::pauseThd()
{
    m_isCanRun = false;
}
void TunnelDetection::runThdFun(TunnelDetection *pParameter)
{
    pParameter->sub_runThdFun();
}
void TunnelDetection::sub_runThdFun()
{
    while (m_isBreakThd)
    {
        //	std::lock_guard<std::mutex> lockGuard(my_lock);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (m_isCanRun == TRUE)
        {
            continue;
        }
        bool res = initCamera(g_program_setInfor.CameraNumber);
        if (res == true)
        {
            // 相机都连接成功了，启动显示和存储
            m_QShowThd->setCamera(m_pCamera, g_cameraInfor);
            bool res = QObject::connect(m_QShowThd, SIGNAL(threadover_Camdat_std(QPixmap, int)), this, SLOT(ShowThd_slots(QPixmap, int)));
            m_QShowThd->startThd();
            pSetCameraDlg->setCamera(m_pCamera);
#if 1
            // 启动存储线程
            //	int i = 0;
            for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
            {
                m_pSaveImg[i] = NULL;
                if (m_pSaveImg[i] != NULL)
                    delete m_pSaveImg[i];
                m_pSaveImg[i] = new QSaveThd(this, m_pCamera[i], g_cameraInfor[i], i);
                QObject::connect(m_pSaveImg[i], SIGNAL(thread_signal(int)), this, SLOT(save_thread_slots(int)));
                QObject::connect(m_pSaveImg[i], SIGNAL(thread_signal_stopsave(int)), this, SLOT(stop_save_thread_slots(int)));
                m_pSaveImg[i]->startThd();
            }
#endif

            m_isCanRun = TRUE;
        }
        else
        {
            m_isCanRun = FALSE;
            continue;
        }
    }
}
bool TunnelDetection::initCamera(int iSumNum)
{
    // 海康相机模式(CameraType==2)：使用前初始化一次 MVS SDK
    if (g_program_setInfor.CameraType == 2)
    {
        MV_CC_Initialize();
    }

    bool res[MAXCAMNUM] = {0};
    for (int i = 0; i < iSumNum; i++)
    {
        m_pCamera[i] = NULL;
        if (m_pCamera[i] != NULL)
            delete m_pCamera[i];
        // 根据黑白/彩色定义每帧图像的字节大小，256*2048或256*4096
        if (g_cameraInfor[i].isColorCamera == 0) // 黑白
        {
            g_cameraInfor[i].iSizeFrame = g_cameraInfor[i].ImageWidth * g_cameraInfor[i].ImageHeight;
        }
        else if (g_cameraInfor[i].isColorCamera == 1) // 彩色
        {
            g_cameraInfor[i].iSizeFrame = g_cameraInfor[i].ImageWidth * g_cameraInfor[i].ImageHeight * 3;
        }

        if (g_program_setInfor.CameraType == 2) // 海康网口相机
        {
            m_pCamera[i] = new CHikCamera(g_cameraInfor[i], m_milDirect);
        }
        else // 0=DALSA网口, 1=DALSA CamLink，沿用原 CCamera 通路
        {
            m_pCamera[i] = new CCamera(g_cameraInfor[i], m_milDirect);
        }
        res[i] = m_pCamera[i]->ConnectCamera();
        if (res[i] == false)
            continue;
        else
            //	emit runThd_signals(i);
            label_isConnect[i]->setText(QStringLiteral("已连接"));
        g_isConnect[i] = true;

		// Config.xml 是手动参数的权威来源：各品牌相机连接后统一下发并回读。
		const bool exposureInRange = g_cameraInfor[i].ExposureTime >= g_cameraInfor[i].ExposureTimeMin &&
			g_cameraInfor[i].ExposureTime <= g_cameraInfor[i].ExposureTimeMax;
		const bool gainInRange = g_cameraInfor[i].Gain >= g_cameraInfor[i].GainMin &&
			g_cameraInfor[i].Gain <= g_cameraInfor[i].GainMax;
		double actualExposure = 0.0;
		double actualGain = 0.0;
		const bool exposureOk = exposureInRange &&
			m_pCamera[i]->SetExpTime(g_cameraInfor[i].ExposureTime) &&
			m_pCamera[i]->GetExpTime(actualExposure);
		const bool gainOk = gainInRange &&
			m_pCamera[i]->SetCamGain(g_cameraInfor[i].Gain) &&
			m_pCamera[i]->GetCamGain(actualGain);
		if (!exposureOk)
			qDebug() << g_cameraInfor[i].Name << QStringLiteral("Config 曝光下发或回读失败：") << g_cameraInfor[i].ExposureTime;
		else
			qDebug() << g_cameraInfor[i].Name << QStringLiteral("Config 曝光已生效：") << actualExposure;
		if (!gainOk)
			qDebug() << g_cameraInfor[i].Name << QStringLiteral("Config 增益下发或回读失败：") << g_cameraInfor[i].Gain;
		else
			qDebug() << g_cameraInfor[i].Name << QStringLiteral("Config 增益已生效：") << actualGain;
    }

#if 1

    int icon_num = 0;
    for (int i = 0; i < MAXCAMNUM; i++)
    {
        if (res[i])
            icon_num++;
    }
    if (icon_num == iSumNum)
    {
        return true;
    }
    else
    {
        return false;
    }

#endif
    return true;
}
//////自动模式工作////////////////////////////////////////////////////////////////////
void TunnelDetection::autoModelActionSlot()
{
    // 此时相机都已处于连接状态，正在刷新显示图像
    // 已得到待检测隧道列表，gtunnel_ledger_struct
    // 已得到GYK里程,m_dmil_GYK
    // 已得到行车状态,m_strXB,m_strMilDir,m_strChetou

    // 提示已正确设置检测信息；加载设备台账；连接相机；打开防水盖；打开照明
    QString strText = QStringLiteral("请确认启动自动采集模式之前，务必确认完成以下操作：");
    strText += "\r\n";
    strText += QStringLiteral("1、设置正确的运行行别、里程增减、车头朝向；");
    strText += "\r\n";
    strText += QStringLiteral("2、加载设备台账；");
    strText += "\r\n";
    strText += QStringLiteral("3、连接相机；");
    strText += "\r\n";
    strText += QStringLiteral("4、打开防水盖；");
    strText += "\r\n";
    strText += QStringLiteral("5、打开照明。");
    QMessageBox::StandardButton reply;
    reply = QMessageBox::information(this, QStringLiteral("请注意"),
                                     strText,
                                     QMessageBox::Yes | QMessageBox::Cancel);
    if (reply == QMessageBox::Cancel)
    {
        return;
    }

    if (gtunnel_ledger_struct.empty())
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::information(this, QStringLiteral("请注意"),
                                         QStringLiteral("未加载设备台账"),
                                         QMessageBox::Yes | QMessageBox::Cancel);
        if (reply == QMessageBox::Cancel)
        {
            return;
        }
    }
    // 计算隧道间隔
    get_tunnel_interval();

    // 建立检测台账
    openCsv();

#if 0
	//打开防水盖
	pSetLightDlg->pushButton_openGaizieSlot();
	//打开照明
	for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
	{
		pSetLightDlg->open_camera_light(i);
	}

#endif

    // 5.自动匹配实时里程和台账里程，
    timerprogress_auto->start();
#if 0

	//1.连接相机，显示但不存储
	//所有相机都连接，但是满足条件的才工作

	//2.打开防水盖，测试照明，亮5秒灭

	//3.限界连接，显示但不存储

	//4.补偿连接，显示但不存储

	//5.自动匹配实时里程和台账里程，若满足条件，则开始存储数据，自动启停。直到待检测隧道检测完毕
	//（1）根据里程推断待检测的隧道
	//（2）根据单双线和上下行，推断工作相机

#endif
}
bool TunnelDetection::openCsv()
{
    // 准备路径
    // 第1级固定根目录
    QString str_path = g_program_setInfor.DataDestPath;
    makeDestDir(str_path);
    // 第2级路局名称
    str_path += "\\";
    str_path += m_bureau_name;
    makeDestDir(str_path);
    // 第3级线路名称
    str_path += "\\";
    str_path += QStringLiteral("检测台账");
    makeDestDir(str_path);

    // 准备文件名
    // 检测台账_隧道_成都局_沪昆高铁_站1-站2_检测时间_计算机标识.xls
    QString strdectfilename = QStringLiteral("检测台账_");
    strdectfilename += m_ledgerFileName.object_Type;
    strdectfilename += "_";
    strdectfilename += m_bureau_name;
    strdectfilename += "_";
    strdectfilename += m_strLineName;
    strdectfilename += "_";
    strdectfilename += m_ledgerFileName.xingbie;
    strdectfilename += "_";
    strdectfilename += m_ledgerFileName.start_station;
    strdectfilename += "_";
    strdectfilename += m_ledgerFileName.end_station;
    m_dateTime_ledgerFile = QDateTime::currentDateTime();
    strdectfilename += m_dateTime_ledgerFile.toString("_yyyyMMddHHmmss");
    strdectfilename += "_";
    strdectfilename += m_ledgerFileName.com_flg;
    strdectfilename += "_";
    strdectfilename += ".csv";

    str_path += "\\";
    str_path += strdectfilename;

    m_csvLedgerFile.setFileName(str_path);

    if (m_csvLedgerFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&m_csvLedgerFile);
        // Excel/WPS on Windows may treat a BOM-less CSV as ANSI/GBK.
        // Write an explicit UTF-8 BOM when the ledger is created.
        stream.setEncoding(QStringConverter::Utf8);
        stream.setGenerateByteOrderMark(true);
        stream << QStringLiteral("序号,") << QStringLiteral("设备类型,") << QStringLiteral("路局,") << QStringLiteral("段名,") << QStringLiteral("线名,") << QStringLiteral("线路等级,")
               << QStringLiteral("所在区间,") << QStringLiteral("设备号,") << QStringLiteral("设备名,") << QStringLiteral("设备编码,") << QStringLiteral("行别,") << QStringLiteral("线别,")
               << QStringLiteral("起点里程,") << QStringLiteral("中心里程,") << QStringLiteral("终点里程,") << QStringLiteral("全长,") << QStringLiteral("高度,") << QStringLiteral("检测设备,")
               << QStringLiteral("检测人员,") << QStringLiteral("相机总数,") << QStringLiteral("视野总数,") << QStringLiteral("当前通道数量,") << QStringLiteral("里程增减,")
               << QStringLiteral("运行行别,") << QStringLiteral("车头朝向,") << QStringLiteral("图像文件,") << QStringLiteral("图像索引文件,")
               << QStringLiteral("检测时间,") << QStringLiteral("限界文件,") << QStringLiteral("是否彩色,") << QStringLiteral("分辨率,") << "\n";

        m_csvLedgerFile.close();
    }
    return true;
}
bool TunnelDetection::writeCsv()
{
    if (m_csvLedgerFile.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream oneStream(&m_csvLedgerFile);
        oneStream.setEncoding(QStringConverter::Utf8);

        // 当前隧道工作相机的图像文件
        QString imgFileName, idxFileName;
        for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
        {
            if (g_cameraInfor[i].isWork == 1)
            {
                if (m_pSaveImg[i] != NULL)
                {
                    imgFileName += m_pSaveImg[i]->m_str_ImgFileName;
                    imgFileName += ";";
                }
                if (m_pSaveImg[i] != NULL)
                {
                    idxFileName += m_pSaveImg[i]->m_str_IdxFileName;
                    idxFileName += ";";
                }
            }
        }
        // 单张图片宽高
        QString strWH;
        for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
        {
            if (g_cameraInfor[i].isWork == 1)
            {
                if (m_pSaveImg[i] != NULL)
                {
                    strWH += QString::number(g_cameraInfor[i].ImageWidth, 10);
                    strWH += ";";
                }
            }
        }
#if 0


		QStringList WStringList;
		QStringList HStringList;
		kuohao_to_list(g_program_setInfor.pictureWH,WStringList, HStringList);
		QString strWH = "\"";
		for (int i = 0; i < MAXCAMNUM; i++)
		{
			strWH += "(";
			strWH += WStringList.at(i);
			strWH += ",";
			strWH += HStringList.at(i);
			strWH += ")";
		}
		strWH += "\"";
#endif
        oneStream.setRealNumberPrecision(10);

        // 是否是彩色
        QString strIsColor;
        if (g_cameraInfor[0].isColorCamera == 0)
        {
            strIsColor = QStringLiteral("否");
        }
        else if (g_cameraInfor[0].isColorCamera == 1)
        {
            strIsColor = QStringLiteral("是");
        }

        // 检测时间
        QString strTime = m_dateTime.toString("yyyyMMddHHmmss\t");
        if ((!gtunnel_ledger_struct.empty()) && (m_idx >= 0)) // 为了适配无砟轨道小车的检测台账，加了=0
        {
            tunnel_data_struct td = gtunnel_ledger_struct.at(m_idx);
            oneStream << td.ixh << "," << td.object_type << "," << td.bureau_name << "," << td.division_name << "," << td.lineName << "," << td.lineLevel << ","
                      << td.section_name << "," << td.tunnelNumber << "," << td.tunnelName << "," << td.tunnelCode << "," << td.tunnelXingbie << ","
                      << td.tunnelType << "," << QString::number(td.tunnel_startMil, 'f', 3) << "," << QString::number(td.tunnel_centreMil, 'f', 3) << ","
                      << QString::number(td.tunnel_endMil, 'f', 3) << "," << td.tunnel_length << "," << td.object_height << "," << td.equipment_name << "," << td.person_name << ","
                      << g_program_setInfor.CameraTotalNumber << "," << g_program_setInfor.ViewTotalNumber << "," << g_program_setInfor.CameraNumber << ","
                      << m_strMilDir << "," << m_strXB << "," << m_strChetou << "," << imgFileName << "," << idxFileName << ","
                      << strTime << "," << QStringLiteral(" ") << "," << strIsColor << "," << strWH << "," << "\n";
        }

        m_csvLedgerFile.close();
    }
    return true;
}
void TunnelDetection::get_active_camera(tunnel_data_struct td)
{
    // 相机工作状态置零
    renew_active_camera();

    // 隧道类型strTunnelType单双线；或者声屏障左右侧
    // 已得到行车状态,m_strXB,m_strMilDir,m_strChetou
    if (td.object_type == QStringLiteral("隧道"))
    {
        if ((td.tunnelType == QStringLiteral("单线")) || (td.tunnelType == QStringLiteral("单")))
        {
            // 检测单线隧道，所有相机工作，与走形行别，面向大小里程无关
            sub_get_active_camera(g_program_setInfor.danxian);
        }
        else if ((td.tunnelType == QStringLiteral("双线")) || (td.tunnelType == QStringLiteral("双")))
        {
            if (m_strXB == QStringLiteral("下行"))
            {
                if (m_strChetou == QStringLiteral("大里程"))
                {
                    // 检测双线隧道，走下行，面向大里程
                    sub_get_active_camera(g_program_setInfor.shuang_xia_da);
                }
                else if (m_strChetou == QStringLiteral("小里程"))
                {
                    // 检测双线隧道，走下行，面向小里程
                    sub_get_active_camera(g_program_setInfor.shuang_xia_xiao);
                }
            }
            else if (m_strXB == QStringLiteral("上行"))
            {
                if (m_strChetou == QStringLiteral("大里程"))
                {
                    // 检测双线隧道，走上行，面向大里程
                    sub_get_active_camera(g_program_setInfor.shuang_shang_da);
                }
                else if (m_strChetou == QStringLiteral("小里程"))
                {
                    // 检测双线隧道，走上行，面向小里程
                    sub_get_active_camera(g_program_setInfor.shuang_shang_xiao);
                }
            }
        }
    }
    else if (td.object_type == QStringLiteral("声屏障"))
    {
        if (m_strXB == QStringLiteral("下行"))
        {
            if (m_strChetou == QStringLiteral("大里程"))
            {
                // 检测左侧声屏障，走下行，面向大里程
                sub_get_active_camera(g_program_setInfor.xia_da_zuo);
            }
            else if (m_strChetou == QStringLiteral("小里程"))
            {
                // 检测左侧声屏障，走下行，面向小里程
                sub_get_active_camera(g_program_setInfor.xia_xiao_zuo);
            }
        }
        else if (m_strXB == QStringLiteral("上行"))
        {
            if (m_strChetou == QStringLiteral("大里程"))
            {
                // 检测右侧声屏障，走上行，面向大里程
                sub_get_active_camera(g_program_setInfor.shang_da_you);
            }
            else if (m_strChetou == QStringLiteral("小里程"))
            {
                // 检测右侧声屏障，走上行，面向小里程
                sub_get_active_camera(g_program_setInfor.shang_xiao_you);
            }
        }
    }
}
void TunnelDetection::sub_get_active_camera(QString strtmp)
{
    QStringList strListTmp = kuohao_to_list(strtmp);
    vector<int> vec_tmp;
    for (int i = 0; i < strListTmp.size(); i++)
    {
        if (strListTmp.at(i).toInt() == 1)
        {
            vec_tmp.push_back(i + 1);
        }
    }
    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        int CPos = g_cameraInfor[i].Name.indexOf('C', 0);
        int ids = g_cameraInfor[i].Name.mid(0, CPos).toInt();
        for (int j = 0; j < vec_tmp.size(); j++)
        {
            if (vec_tmp.at(j) == ids)
            {
                g_cameraInfor[i].isWork = 1;
                qDebug() << g_cameraInfor[i].Name << "flag:" << QString::number(g_cameraInfor[i].isWork, 10);
                break;
            }
        }
    }
}
void TunnelDetection::renew_active_camera()
{
    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        g_cameraInfor[i].isWork = 0;
    }
}
void TunnelDetection::timerUpDateAutoModeprogress()
{
    // 每一个m_dmil_GYK，都进行一次全面判断
    // 1.先判断正在检测、已检测、未检测、检测完成
    // 2.开始存储、停止存储

    if (gtunnel_ledger_struct.empty())
        return;

    double m_dmil = m_dmil_GYK;
    if (g_program_setInfor.GYK0_or_COR1 == 0)
    {
        m_dmil = m_dmil_GYK;
    }
    else if (g_program_setInfor.GYK0_or_COR1 == 1)
    {
        m_dmil = m_dmil_GYK_Cor;
    }

    // 3.屏蔽异常里程
    if (g_program_setInfor.big_mil_onoff == 1)
    {
        if ((m_dmil - 0) < 0 || (m_dmil - g_program_setInfor.mil_threshold) > 0)
        {
            return;
        }
    }

    if (m_strMilDir == QStringLiteral("增加"))
    {
        for (int i = 0; i < gtunnel_ledger_struct.size(); i++)
        {
            tunnel_data_struct td = gtunnel_ledger_struct.at(i);
            double dvalue_jin = m_dmil - (td.tunnel_startMil - (double)g_program_setInfor.rukouThreshold / 1000);
            double dvalue_chu = m_dmil - (td.tunnel_endMil + (double)g_program_setInfor.chukouThreshold / 1000);
            if ((dvalue_jin > 0) && (dvalue_chu < 0))
            {
                // 当前隧道正在检测
                gtunnel_ledger_struct.at(i).status_flag = BEINGDETECTION;
                //	auto_show_slots(i, QStringLiteral("正在检测"));
            }
            if (dvalue_jin < 0)
            {
                // 当前隧道未检测
                gtunnel_ledger_struct.at(i).status_flag = NODETECTION;
                auto_show_slots(i, QStringLiteral("未检测"));
            }
            if (dvalue_chu > 0)
            {
                // 当前隧道已检测
                gtunnel_ledger_struct.at(i).status_flag = COMPLETEDETECTION;
                auto_show_slots(i, QStringLiteral("已检测"));
            }
        }
        for (int i = 0; i < gtunnel_ledger_struct.size(); i++)
        {
            tunnel_data_struct td = gtunnel_ledger_struct.at(i);
            // 还没到入口，入口判断
            // 			double dmil_s1 = td.tunnel_startMil - (double)g_program_setInfor.rukouThreshold/1000;
            // 			double dmil_s2 = m_dmil_GYK - dmil_s1;//注意单位m，默认是20m
            // 			if (dmil_s2 < 0)
            double dmil_s1 = m_dmil - td.tunnel_startMil;                             // 必须小于0
            double dmil_s2 = abs(dmil_s1 * 1000) - g_program_setInfor.rukouThreshold; // 注意单位m，默认是20m
            if ((dmil_s1 < 0) && (dmil_s2 < 0))
            {
                // 开始存储，动作一次
                if (save_start_flag == true)
                {
                    m_idx = i;
                    get_active_camera(td);
                    qDebug() << "gyk:" << QString::number(m_dmil, 'f', 3) << "startMil:" << QString::number(td.tunnel_startMil, 'f', 3) << "juli:" << dmil_s1;
                    auto_start_acq_slots(i);
                    ui->tableWidget->item(td.ixh - 1, icow_table_smil)->setText(QString::number(m_dmil, 'f', 3));
                    save_start_flag = false;
                    save_stop_flag = true;
                }
            }
            // 已经进入隧道
            double dvalue1 = m_dmil - td.tunnel_startMil;
            double dvalue2 = m_dmil - td.tunnel_endMil;
            if ((dvalue1 > 0) && (dvalue2 < 0))
            {
                // 当前隧道正在检测
                // 判断是否已开始存储数据
                if (save_start_flag == true) // 假意味着正在存储
                {
                    m_idx = i;
                    get_active_camera(td);
                    qDebug() << "startMil:" << QString::number(td.tunnel_startMil, 'f', 3) << "gyk:" << QString::number(m_dmil, 'f', 3) << "endMil:" << QString::number(td.tunnel_endMil, 'f', 3);
                    auto_start_acq_slots(i);
                    save_start_flag = false;
                    save_stop_flag = true;
                    ui->tableWidget->item(td.ixh - 1, icow_table_smil)->setText(QString::number(m_dmil, 'f', 3));
                }
            }
            // 出口判断
            // 			double dmil_e1 = td.tunnel_endMil + (double)g_program_setInfor.chukouThreshold/1000;//必须大于0
            // 			double dmil_e2 = m_dmil_GYK - dmil_e1;
            // 			if (dmil_e2 > 0)
            double dmil_e1 = m_dmil - (td.tunnel_endMil + (double)g_program_setInfor.chukouThreshold / 1000); // 必须大于0
            double dmil_e2 = abs(dmil_e1 * 1000) - g_program_setInfor.chukouThreshold;
            if ((dmil_e1 > 0) && (dmil_e2 < 0))
            {
                if (save_stop_flag == true)
                {
                    // 停止存储
                    qDebug() << "endMil:" << QString::number(td.tunnel_endMil, 'f', 3) << "gyk:" << QString::number(m_dmil, 'f', 3) << "juli:" << dmil_e1;
                    save_stop_flag = false;
                    auto_stop_acq_slots(i);
                    ui->tableWidget->item(td.ixh - 1, icow_table_emil)->setText(QString::number(m_dmil, 'f', 3));
                    renew_active_camera();
                    select_tunnel_flag = true; // 允许再次查找隧道
                    save_start_flag = true;
                    myflg = true;
                }
            }
        }
    }
    else if (m_strMilDir == QStringLiteral("减小"))
    {
        for (int i = gtunnel_ledger_struct.size() - 1; i >= 0; i--)
        {
            tunnel_data_struct td = gtunnel_ledger_struct.at(i);
            double dvalue_jin = m_dmil - (td.tunnel_endMil + (double)g_program_setInfor.rukouThreshold / 1000);
            double dvalue_chu = m_dmil - (td.tunnel_startMil - (double)g_program_setInfor.chukouThreshold / 1000);
            if ((dvalue_jin < 0) && (dvalue_chu > 0))
            {
                // 当前隧道正在检测
                gtunnel_ledger_struct.at(i).status_flag = BEINGDETECTION;
                auto_show_slots(i, QStringLiteral("正在检测"));
            }
            if (dvalue_jin > 0)
            {
                // 当前隧道未检测
                gtunnel_ledger_struct.at(i).status_flag = NODETECTION;
                auto_show_slots(i, QStringLiteral("未检测"));
            }
            if (dvalue_chu < 0)
            {
                // 当前隧道已检测
                gtunnel_ledger_struct.at(i).status_flag = COMPLETEDETECTION;
                auto_show_slots(i, QStringLiteral("已检测"));
            }
        }
        for (int i = gtunnel_ledger_struct.size() - 1; i >= 0; i--)
        {
            tunnel_data_struct td = gtunnel_ledger_struct.at(i);
            // 还没到入口，入口判断
            double dmil_s1 = m_dmil - td.tunnel_endMil;
            double dmil_s2 = abs(dmil_s1 * 1000) - g_program_setInfor.rukouThreshold; // 注意单位m,默认是20m
            if ((dmil_s1 > 0) && (dmil_s2 < 0))
            {
                // 开始存储，动作一次
                if (save_start_flag == true)
                {
                    m_idx = i;
                    get_active_camera(td);
                    qDebug() << "gyk:" << QString::number(m_dmil, 'f', 3) << "startMil:" << QString::number(td.tunnel_endMil, 'f', 3) << "juli:" << dmil_s1;
                    auto_start_acq_slots(i);
                    ui->tableWidget->item(td.ixh - 1, icow_table_smil)->setText(QString::number(m_dmil, 'f', 3));
                    save_start_flag = false;
                    save_stop_flag = true;
                }
            }
            // 已经进入隧道
            double dvalue1 = m_dmil - td.tunnel_endMil;
            double dvalue2 = m_dmil - td.tunnel_startMil;
            if ((dvalue1 < 0) && (dvalue2 > 0))
            {
                // 当前隧道正在检测
                // 判断是否已开始存储数据
                if (save_start_flag == true) // 假意味着正在存储
                {
                    m_idx = i;
                    get_active_camera(td);
                    qDebug() << "endMil:" << QString::number(td.tunnel_endMil, 'f', 3) << "gyk:" << QString::number(m_dmil, 'f', 3) << "startMil:" << QString::number(td.tunnel_startMil, 'f', 3);
                    auto_start_acq_slots(i);
                    save_start_flag = false;
                    save_stop_flag = true;
                    ui->tableWidget->item(td.ixh - 1, icow_table_smil)->setText(QString::number(m_dmil, 'f', 3));
                }
            }
            // 出口判断
            double dmil_e1 = m_dmil - (td.tunnel_startMil - (double)g_program_setInfor.chukouThreshold / 1000); // 必须大于0
            double dmil_e2 = abs(dmil_e1 * 1000) - g_program_setInfor.chukouThreshold;
            if ((dmil_e1 < 0) && (dmil_e2 < 0))
            {
                if (save_stop_flag == true)
                {
                    // 停止存储
                    qDebug() << "tunnel_startMil:" << QString::number(td.tunnel_startMil, 'f', 3) << "gyk:" << QString::number(m_dmil, 'f', 3) << "juli:" << dmil_e1;
                    save_stop_flag = false;
                    auto_stop_acq_slots(i);
                    ui->tableWidget->item(td.ixh - 1, icow_table_emil)->setText(QString::number(m_dmil, 'f', 3));
                    renew_active_camera();
                    select_tunnel_flag = true; // 允许再次查找隧道
                    save_start_flag = true;
                    myflg = true;
                }
            }
        }
    }

    // 下一个隧道距离更新
    int idx = find_tunnel_idxII();
    if ((idx >= 0) && (idx < gtunnel_ledger_struct.size()))
    {
        m_td = gtunnel_ledger_struct.at(idx);
        auto_mode_slots();
    }

    // 当前隧道距离更新
    dealjuli();

    // gyk里程大于最后一个隧道出口里程, 则检测完毕
    if (m_strMilDir == QStringLiteral("增加"))
    {
        if (!gtunnel_ledger_struct.empty())
        {
            double dtmp = m_dmil - (gtunnel_ledger_struct.at(gtunnel_ledger_struct.size() - 1).tunnel_endMil + 0.1);
            if (dtmp > 0)
            {
                timerprogress_auto->stop();
                QString strText = QStringLiteral("已完成全部检测！");
                strText += QString::number(m_dmil, 'f', 3);
                QMessageBox::StandardButton reply;
                reply = QMessageBox::information(this, QStringLiteral("敬请注意"),
                                                 strText,
                                                 QMessageBox::Yes | QMessageBox::Cancel);
            }
        }
    }
    else if (m_strMilDir == QStringLiteral("减小"))
    {
        if (!gtunnel_ledger_struct.empty())
        {
            double dtmp = m_dmil - (gtunnel_ledger_struct.at(0).tunnel_startMil - 0.1);
            if (dtmp < 0)
            {
                timerprogress_auto->stop();
                QString strText = QStringLiteral("已完成全部检测！");
                strText += QString::number(m_dmil, 'f', 3);
                QMessageBox::StandardButton reply;
                reply = QMessageBox::information(this, QStringLiteral("敬请注意"),
                                                 strText,
                                                 QMessageBox::Yes | QMessageBox::Cancel);
            }
        }
    }
}
int TunnelDetection::find_tunnel_idxII()
{
    // 0表示未检测，1表示正在检测，2表示检测完（检测指的是动态里程是否经过，不是真正存储数据的意思）
    int idest = -1; //-1表示没有要检测的隧道
    if (m_strMilDir == QStringLiteral("增加"))
    {
        for (int i = 0; i < gtunnel_ledger_struct.size(); i++)
        {
            tunnel_data_struct td = gtunnel_ledger_struct.at(i);
            if (td.status_flag == -1)
            {
                continue;
            }
            else if (td.status_flag == COMPLETEDETECTION)
            {
                continue;
            }
            else if (td.status_flag == BEINGDETECTION)
            {
                idest = i + 1; // 下一个隧道是即将检测，序号从1开始
                break;
            }
            else if (td.status_flag == NODETECTION)
            {
                idest = i; // 第一个未检测的隧道就是下一个即将检测的隧道
                break;
            }
        }
    }
    else if (m_strMilDir == QStringLiteral("减小"))
    {
        for (int i = gtunnel_ledger_struct.size() - 1; i >= 0; i--)
        {
            tunnel_data_struct td = gtunnel_ledger_struct.at(i);
            if (td.status_flag == -1)
            {
                continue;
            }
            else if (td.status_flag == COMPLETEDETECTION)
            {
                continue;
            }
            else if (td.status_flag == BEINGDETECTION)
            {
                idest = i - 1; // 下一个隧道是即将检测，序号从1开始
                break;
            }
            else if (td.status_flag == NODETECTION)
            {
                idest = i; // 第一个未检测的隧道就是下一个即将检测的隧道
                break;
            }
        }
    }

    return idest;
}
void TunnelDetection::auto_mode_slots()
{
    if (gtunnel_ledger_struct.empty())
    {
        return;
    }

    double m_dmil = m_dmil_GYK;
    if (g_program_setInfor.GYK0_or_COR1 == 0)
    {
        m_dmil = m_dmil_GYK;
    }
    else if (g_program_setInfor.GYK0_or_COR1 == 1)
    {
        m_dmil = m_dmil_GYK_Cor;
    }

    if (m_strMilDir == QStringLiteral("增加"))
    {
        // 标识即将检测隧道；还剩多少米开始，正在检测隧道，还剩多少米完成
        double djuli = m_td.tunnel_startMil - m_dmil;
        djuli = djuli * 1000;
        QString strtext = QStringLiteral("即将检测，还剩");
        QString tmp = QString::number((int)djuli, 10);
        strtext += tmp;
        strtext += QStringLiteral("米开始");
        ui->tableWidget->item(m_td.ixh - 1, icow_table_result)->setText(strtext);
    }
    else if (m_strMilDir == QStringLiteral("减小"))
    {
        // 标识即将检测隧道；还剩多少米开始，正在检测隧道，还剩多少米完成
        double djuli = m_dmil - m_td.tunnel_endMil;
        djuli = djuli * 1000;
        QString strtext = QStringLiteral("即将检测，还剩");
        QString tmp = QString::number((int)djuli, 10);
        strtext += tmp;
        strtext += QStringLiteral("米开始");
        ui->tableWidget->item(m_td.ixh - 1, icow_table_result)->setText(strtext);
    }
}
void TunnelDetection::dealjuli()
{

    double m_dmil = m_dmil_GYK;
    if (g_program_setInfor.GYK0_or_COR1 == 0)
    {
        m_dmil = m_dmil_GYK;
    }
    else if (g_program_setInfor.GYK0_or_COR1 == 1)
    {
        m_dmil = m_dmil_GYK_Cor;
    }

    if (m_strMilDir == QStringLiteral("增加"))
    {
        for (int i = 0; i < gtunnel_ledger_struct.size(); i++)
        {
            if (gtunnel_ledger_struct.at(i).status_flag == BEINGDETECTION)
            {
                // 正在检测设置成绿色
                tunnel_data_struct td = gtunnel_ledger_struct.at(i);
                for (int j = 0; j < m_tableWidget_cow; j++)
                {
                    ui->tableWidget->item(i, j)->setBackground(QBrush(QColor(0, 255, 0)));
                }

                // 列表控件的滚动条自动跟随
                if (myflg == true)
                {
                    ui->tableWidget->scrollToItem(ui->tableWidget->item(td.ixh - 1, 0), QAbstractItemView::PositionAtCenter);
                    myflg = false;
                }

                // 动态提示正在检测，还剩*米完成
                double djuli = td.tunnel_endMil - m_dmil;
                if (djuli < 0)
                {
                    djuli = 0;
                }
                djuli = djuli * 1000;
                QString strtext = QStringLiteral("正在检测，还剩");
                QString tmp = QString::number((int)djuli, 10);
                strtext += tmp;
                strtext += QStringLiteral("米完成");
                ui->tableWidget->item(i, icow_table_result)->setText(strtext);
            }
            // 检测完成设置成浅黄色
            if (gtunnel_ledger_struct.at(i).status_flag == COMPLETEDETECTION)
            {
                for (int j = 0; j < m_tableWidget_cow; j++)
                {
                    ui->tableWidget->item(i, j)->setBackground(QBrush(QColor(255, 255, 0)));
                }
            }
        }
    }
    else if (m_strMilDir == QStringLiteral("减小"))
    {
        for (int i = gtunnel_ledger_struct.size() - 1; i >= 0; i--)
        {
            if (gtunnel_ledger_struct.at(i).status_flag == BEINGDETECTION)
            {
                // 正在检测设置成绿色
                tunnel_data_struct td = gtunnel_ledger_struct.at(i);
                for (int j = 0; j < m_tableWidget_cow; j++)
                {
                    ui->tableWidget->item(i, j)->setBackground(QBrush(QColor(0, 255, 0)));
                }

                // 列表控件的滚动条自动跟随
                if (myflg == true)
                {
                    ui->tableWidget->scrollToItem(ui->tableWidget->item(td.ixh - 1, 0), QAbstractItemView::PositionAtCenter);
                    myflg = false;
                }

                // 动态提示正在检测，还剩*米完成
                double djuli = m_dmil - td.tunnel_startMil;
                if (djuli < 0)
                {
                    djuli = 0;
                }
                djuli = djuli * 1000;
                QString strtext = QStringLiteral("正在检测，还剩");
                QString tmp = QString::number((int)djuli, 10);
                strtext += tmp;
                strtext += QStringLiteral("米完成");
                ui->tableWidget->item(td.ixh - 1, icow_table_result)->setText(strtext);
            }
            // 检测完成设置成浅黄色
            if (gtunnel_ledger_struct.at(i).status_flag == COMPLETEDETECTION)
            {
                for (int j = 0; j < m_tableWidget_cow; j++)
                {
                    ui->tableWidget->item(i, j)->setBackground(QBrush(QColor(255, 255, 0)));
                }
            }
        }
    }
}
void TunnelDetection::auto_start_acq_slots(int idx)
{
    tunnel_data_struct td = gtunnel_ledger_struct.at(m_idx);
    startSaveImage(td.tunnelNumber, td.tunnelName);
}
void TunnelDetection::auto_stop_acq_slots(int idx)
{
    stopSaveImage();

    // 写一行csv
    writeCsv();
}
void TunnelDetection::auto_show_slots(int irow, QString strText)
{
    ui->tableWidget->item(irow, icow_table_result)->setText(strText);
}
void TunnelDetection::ShowThd_slots(QPixmap picQPixmap, int iChl)
{
    labelShowImg[iChl]->setPixmap(picQPixmap.scaled(labelShowImg[iChl]->size()));
}
void TunnelDetection::disConnectActionSlot()
{
    disConnect();
}
void TunnelDetection::disConnect()
{
    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        if (m_pSaveImg[i] != NULL)
        {
            m_pSaveImg[i]->stopThd();
        }
    }

    if (m_QShowThd != NULL)
    {
        m_QShowThd->stopThd();
    }
    Sleep(500);
    stopThd();
    // 断开相机
    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        if (m_pCamera[i] != NULL)
        {
            m_pCamera[i]->StopCamera();
            m_pCamera[i]->DestroyObjects();
        }
    }
    // 海康相机模式：反初始化 MVS SDK
    if (g_program_setInfor.CameraType == 2)
    {
        MV_CC_Finalize();
    }
}
//////图像数据存储////////////////////////////////////////////////////////////////////
void TunnelDetection::startAcqActionSlot()
{
    if (m_idx == -1)
    {
        QString sdNumber = "01";
        QString sdName = QStringLiteral("测试隧道");
        // 建立文件夹
        // 第1级固定根目录
        QString str_path = g_program_setInfor.DataDestPath;
        makeDestDir(str_path);
        // 第2级路局名称
        str_path += "\\";
        str_path += m_bureau_name;
        makeDestDir(str_path);
        // 第3级线路名称
        str_path += "\\";
        str_path += m_strLineName;
        makeDestDir(str_path);
        // 第4级隧道号-隧道名称
        str_path += "\\";
        QString sdhao_sdName = sdNumber;
        sdhao_sdName += "-";
        sdhao_sdName += sdName;
        str_path += sdhao_sdName;
        makeDestDir(str_path);
        // 第5级总的检测开始的时间
        str_path += "\\";
        m_dateTime = QDateTime::currentDateTime();
        str_path += m_dateTime.toString("yyyyMMddHHmmss");
        makeDestDir(str_path);
        // 第6级acq
        str_path += "\\";
        str_path += "acq";
        makeDestDir(str_path);

        // 每个相机开始存储
        for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
        {
            // 文件名:年月日时分秒_相机名称_运行行别_里程增减_车头朝向.camdat
            QString str_FileName = m_dateTime.toString("yyyyMMddHHmmss");
            str_FileName += "-";
            str_FileName += g_cameraInfor[i].Name;
            str_FileName += "-";
            str_FileName += m_strXB;
            str_FileName += "-";
            str_FileName += m_strMilDir;
            str_FileName += "-";
            str_FileName += m_strChetou;
            str_FileName += QStringLiteral(".camdatx.tmp");

            if (m_pSaveImg[i] != NULL)
            {
                m_pSaveImg[i]->StartSave(str_path, str_FileName);
            }
            if (m_pCamera[i] != NULL && g_isSave[i].load())
            {
                m_pCamera[i]->StartAcquisition();
            }

            qDebug() << str_FileName;
        }
    }
    else
    {
        tunnel_data_struct td = gtunnel_ledger_struct.at(m_idx);

        // 建立检测台账
        openCsv();
        startSaveImage(td.tunnelNumber, td.tunnelName);
    }
}
void TunnelDetection::startSaveImage(QString sdNumber, QString sdName)
{
    // 建立文件夹
    // 第1级固定根目录
    QString str_path = g_program_setInfor.DataDestPath;
    makeDestDir(str_path);
    // 第2级路局名称
    str_path += "\\";
    str_path += m_bureau_name;
    makeDestDir(str_path);
    // 第3级线路名称
    str_path += "\\";
    str_path += m_strLineName;
    makeDestDir(str_path);
    // 第4级隧道号-隧道名称
    str_path += "\\";
    QString sdhao_sdName = sdNumber;
    sdhao_sdName += "-";
    sdhao_sdName += sdName;
    str_path += sdhao_sdName;
    makeDestDir(str_path);
    // 第5级总的检测开始的时间
    str_path += "\\";
    m_dateTime = QDateTime::currentDateTime();
    str_path += m_dateTime.toString("yyyyMMddHHmmss");
    makeDestDir(str_path);
    // 第6级acq
    str_path += "\\";
    str_path += "acq";
    makeDestDir(str_path);

    // 每个相机开始存储
    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        if (g_cameraInfor[i].isWork == 1)
        {
            // 文件名:年月日时分秒_相机名称_运行行别_里程增减_车头朝向.camdat
            QString str_FileName = m_dateTime.toString("yyyyMMddHHmmss");
            str_FileName += "-";
            str_FileName += g_cameraInfor[i].Name;
            str_FileName += "-";
            str_FileName += m_strXB;
            str_FileName += "-";
            str_FileName += m_strMilDir;
            str_FileName += "-";
            str_FileName += m_strChetou;
            str_FileName += QStringLiteral(".camdatx.tmp");

            if (m_pSaveImg[i] != NULL)
            {
                m_pSaveImg[i]->StartSave(str_path, str_FileName);
            }
            if (m_pCamera[i] != NULL && g_isSave[i].load())
            {
                m_pCamera[i]->StartAcquisition();
            }

            qDebug() << str_FileName;
        }
    }
}
void TunnelDetection::makeDestDir(QString strValue)
{
    QDir dir(strValue);
    if (!dir.exists())
    {
        bool res = dir.mkdir(strValue);
        if (res == false)
        {
            QString strText = QStringLiteral("建立路径：");
            strText += strValue;
            strText += QStringLiteral("  失败");
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, QStringLiteral("敬请注意"),
                                          strText,
                                          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        }
    }
}
void TunnelDetection::stopAcqActionSlot()
{
    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        if (m_pCamera[i] != NULL)
        {
            m_pCamera[i]->StopAcquisition();
        }

        if (m_pSaveImg[i] != NULL)
        {
            m_pSaveImg[i]->StopSave();
        }
    }

    // 写一行csv
    writeCsv();
}
void TunnelDetection::stopSaveImage()
{
    for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
    {
        if (g_cameraInfor[i].isWork == 1)
        {
            if (m_pCamera[i] != NULL)
            {
                m_pCamera[i]->StopAcquisition();
            }

            if (m_pSaveImg[i] != NULL)
            {
                m_pSaveImg[i]->StopSave();
            }
        }
    }
}
void TunnelDetection::viewHelpActionSlot()
{
}
void TunnelDetection::save_thread_slots(int iChlV)
{
    label_isSave[iChlV]->setText(QStringLiteral("正在存储"));
}
void TunnelDetection::stop_save_thread_slots(int iChlV)
{
    label_isSave[iChlV]->setText(QStringLiteral("停止存储"));
}
void TunnelDetection::closeEvent(QCloseEvent *event)
{
    m_milDirect->StopCount();
}
