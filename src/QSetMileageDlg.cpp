#include "QSetMileageDlg.h"
#include "ui_QSetMileageDlg.h"

QSetMileageDlg::QSetMileageDlg(QMilThd *milDetect, QWidget *parent)
    : QDialog(parent), ui(new Ui::QSetMileageDlg)
{
    ui->setupUi(this);

    set_milDetect = milDetect;
    setWindowFlags((windowFlags() & ~Qt::WindowCloseButtonHint));
    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags(flags);

    ui->lineEdit_smil->setMinimumWidth(120);
    ui->lineEdit_smil->setMinimumHeight(40);
    ui->lineEdit_emil->setMinimumWidth(120);
    ui->lineEdit_emil->setMinimumHeight(40);

    ui->lineEdit_smil->setStyleSheet("font: bold 24px;");
    ui->lineEdit_emil->setStyleSheet("font: bold 24px;");

    ui->label_smil->setStyleSheet("font: bold 24px;");
    ui->label_emil->setStyleSheet("font: bold 24px;");
    ui->label_smil_unit->setStyleSheet("font: bold 24px;");
    ui->label_mil_emil_unit->setStyleSheet("font: bold 24px;");

    connect(ui->pushButton_set, &QPushButton::clicked, this, &QSetMileageDlg::on_pushButton_set);
    connect(ui->pushButton_QingLing, &QPushButton::clicked, this, &QSetMileageDlg::on_pushButton_QingLing);
    ui->pushButton_QingLing->hide();

    m_pButtonGroup = new QButtonGroup(this);
    m_pButtonGroup->setExclusive(true); // 设置互斥
    m_pButtonGroup->addButton(ui->radioButton_gyk);
    m_pButtonGroup->addButton(ui->radioButton_pulse);

    ui->radioButton_gyk->setCheckable(true);
    ui->radioButton_gyk->setChecked(true);
    connect(m_pButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(onpButtonGroupClicked(QAbstractButton *)));
    //	ui->groupBox->hide();

    ui->lineEdit_pulse_width->setText(QString::number(g_program_setInfor.PulseWidth, 'f', 8)); // 里程，单位是米

    timerprogress = NULL;
    m_milMode = 0;

    initStart();
#if 0
 	//上中下结构
	QVBoxLayout *fullLayout = new QVBoxLayout();
	fullLayout->setMargin(10);     //对话框左右边距10
	fullLayout->setSpacing(10);    //上下边距

	//上边
	QHBoxLayout *upLayout = new QHBoxLayout();


	//上边.左边是脉冲计数里程
	upLayout->addWidget(ui.groupBox_pulse);


	//上边.右边是车载定位系统里程比如GYK
	upLayout->addWidget(ui.groupBox_GYK);
	
	fullLayout->addLayout(upLayout);

	this->setLayout(fullLayout);
#endif
}

QSetMileageDlg::~QSetMileageDlg()
{
    delete ui;
}

void QSetMileageDlg::initStart()
{
    set_milDetect->SetDevNum(g_program_setInfor.Pci1784Num, g_program_setInfor.PulseWidth);
    set_milDetect->InitMil();
    set_milDetect->InitDiDo(g_program_setInfor.Pci1784Num);
    timerprogress = new QTimer(this); // 新建定时器
    connect(timerprogress, SIGNAL(timeout()), this, SLOT(timerUpDateprogress()));
    timerprogress->setInterval(100);
    timerprogress->start();

    set_milDetect->StartCount();
}
void QSetMileageDlg::setMilDir(QString strMilDir)
{
    m_setMilDir = strMilDir;
}
void QSetMileageDlg::timerUpDateprogress()
{
    MILEAGEINFORSTRUCT ms = set_milDetect->GetMileageInfor();
    double dtmp = ms.fMileage / 1000;
    ui->lineEdit_mil->setText(QString::number(dtmp, 'f', 6));           // 里程，单位是公里
    ui->lineEdit_speed->setText(QString::number(ms.fSpeed, 'f', 2));    // 速度,公里/小时
    ui->lineEdit_pulse->setText(QString::number(ms.iPulseCount, 10));   //
    ui->lineEdit_mil_gyk->setText(QString::number(m_dmil_GYK, 'f', 3)); //
    ui->lineEdit_speed_gyk->setText(QString::number(m_dspeed_GYK, 10)); //
    if (m_setMilDir == QStringLiteral("增加"))
    {
        m_dmil_GYK_Cor = m_dmil_GYK + ms.fMileage / 1000;
    }
    else if (m_setMilDir == QStringLiteral("减小"))
    {
        m_dmil_GYK_Cor = m_dmil_GYK - ms.fMileage / 1000;
    }

    ui->lineEdit_mil_acc->setText(QString::number(m_dmil_GYK_Cor, 'f', 3)); //

    emit Mil_time_signals(ms.fMileage, ms.fSpeed);
    //	ui->lineEdit_inc_dec->setText(QString::number(ms.uiIncOrDec, 10));
    // 	if (ms.uiIncOrDec == 0)
    // 	{
    // 		ui->lineEdit_inc_dec->setText(QStringLiteral("增加、正转"));
    // 	}
    // 	else if (ms.uiIncOrDec == 1)
    // 	{
    // 		ui.lineEdit_inc_dec->setText(QStringLiteral("减小、反转"));
    // 	}

    //	ui.lineEdit_P_N->setText(QString::number(ms.uiPositiveOrNegative, 10));

    // 	if (ms.uiPositiveOrNegative == 0)
    // 	{
    // 		ui.uiPositiveOrNegative->setText(QStringLiteral("正数"));
    // 	}
    // 	else if (ms.uiPositiveOrNegative == 1)
    // 	{
    // 		ui.uiPositiveOrNegative->setText(QStringLiteral("负数"));
    // 	}
}
void QSetMileageDlg::onpButtonGroupClicked(QAbstractButton *button)
{
    // 当前点击的按钮
    if (button->text() == QStringLiteral("使用车载定位系统里程"))
    {
        m_milMode = 0;
    }
    else if (button->text() == QStringLiteral("使用脉冲计数里程"))
    {
        m_milMode = 1;
    }
}

void QSetMileageDlg::on_pushButton_set()
{
    QString ss = ui->lineEdit_smil->text();
    QString ee = ui->lineEdit_emil->text();
    emit Mil_over_signals(ss, ee);
    this->hide();
}

void QSetMileageDlg::on_pushButton_QingLing()
{
    set_milDetect->SetZero();
}