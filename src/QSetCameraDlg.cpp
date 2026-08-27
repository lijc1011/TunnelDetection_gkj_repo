#include "QSetCameraDlg.h"
#include "ui_QSetCameraDlg.h"
#include <QCoreApplication>
#include <QSaveFile>

QSetCameraDlg::QSetCameraDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QSetCameraDlg)
{
    ui->setupUi(this);

    	for (int i = 0; i < MAXCAMNUM; i++)
	{
		m_cam[i] = NULL;
	}

	for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
	{
		//gridlayout，每个相机的曝光和增益控制
		QGridLayout *Layout_cam = new QGridLayout();
		Layout_cam->setContentsMargins(0, 0, 0, 0);
		Layout_cam->setContentsMargins(1, 1, 1, 1);    //对话框左右边距10
	
		Layout_cam->setSpacing(1);     //上下边距

		//相机名称
		QString strshow = QStringLiteral("相机：");

		int CPos = g_cameraInfor[i].Name.indexOf('C', 0);
		strshow += g_cameraInfor[i].Name.mid(0, CPos);
		QLabel *label_cn = new QLabel(strshow);
		Layout_cam->addWidget(label_cn, 0, 0, 1, 3);
		label_cn->setAlignment(Qt::AlignCenter);
		label_cn->setMinimumHeight(40);
		label_cn->setMaximumHeight(40);
		//相机曝光建议值
// 		QString strus = QStringLiteral("曝光调整建议：" );
// 		strus += QStringLiteral("(1)如非必要，不建议调整曝光值。");
// 		strus += QStringLiteral("(2)曝光调整范围：5≤u≤30。");
// 		QLabel *label_us_infor1 = new QLabel(QStringLiteral("曝光调整建议："));
// 		Layout_cam->addWidget(label_us_infor1, 1, 0);
// 		QLabel *label_us_infor2 = new QLabel(QStringLiteral("(1)如非必要，不建议调整曝光值。"));
// 		Layout_cam->addWidget(label_us_infor2, 2, 0);
		QString strcontext = QString(QStringLiteral("曝光调整范围:%1≤u≤%2范围")).arg(g_cameraInfor[i].ExposureTimeMin).arg(g_cameraInfor[i].ExposureTimeMax);
  		QLabel *label_us_infor = new QLabel(strcontext);
 		Layout_cam->addWidget(label_us_infor, 1, 0,1,3);
		label_us_infor->setAlignment(Qt::AlignCenter);
		label_us_infor->setMaximumHeight(40);
		
		QString strcontext1 = QString(QStringLiteral("增益调整范围:%1≤u≤%2范围")).arg(g_cameraInfor[i].GainMin).arg(g_cameraInfor[i].GainMax);
		QLabel *label_gain_infor = new QLabel(strcontext1);
		Layout_cam->addWidget(label_gain_infor, 2, 0, 1, 3);
		label_gain_infor->setAlignment(Qt::AlignCenter);
		label_gain_infor->setMaximumHeight(40);

		QLabel *label_us = new QLabel(QStringLiteral("曝光(us)："));
		LineEdit_us[i] = new QLineEdit;
		Layout_cam->addWidget(label_us, 3, 0);
		Layout_cam->addWidget(LineEdit_us[i], 3, 1);
		label_us->setAlignment(Qt::AlignCenter);
		LineEdit_us[i]->setFixedWidth(60);
		QPushButton *pb_setus = new QPushButton(QStringLiteral("设置"));
		pb_setus->setObjectName(QString::number(i, 10));
		connect(pb_setus, &QPushButton::clicked, this, &QSetCameraDlg::pb_setus_Button);
		Layout_cam->addWidget(pb_setus, 3, 2, 1, 1);
		pb_setus->setMinimumWidth(120);
		pb_setus->setMinimumHeight(40);

		QLabel *label_gain = new QLabel(QStringLiteral("增益："));
		LineEdit_gain[i] = new QLineEdit;
		Layout_cam->addWidget(label_gain, 4, 0);
		Layout_cam->addWidget(LineEdit_gain[i], 4, 1);
		label_gain->setAlignment(Qt::AlignCenter);
		LineEdit_gain[i]->setFixedWidth(60);
		QPushButton *pb_setgain = new QPushButton(QStringLiteral("设置"));
		pb_setgain->setObjectName(QString::number(i, 10));
		connect(pb_setgain, &QPushButton::clicked, this, &QSetCameraDlg::pb_setgain_Button);
		Layout_cam->addWidget(pb_setgain, 4, 2, 1, 1);
		pb_setgain->setMinimumWidth(120);
		pb_setgain->setMinimumHeight(40);

		QLabel *label_Frequency = new QLabel(QStringLiteral("行频："));
		LineEdit_Frequency[i] = new QLineEdit;
		Layout_cam->addWidget(label_Frequency,5, 0);
		Layout_cam->addWidget(LineEdit_Frequency[i], 5, 1,1,2);
		label_Frequency->setAlignment(Qt::AlignCenter);
	//	LineEdit_Frequency[i]->setFixedWidth(60);

		QLabel *label_DeqShow = new QLabel(QStringLiteral("显示队列："));
		LineEdit_DeqShow[i] = new QLineEdit;
		Layout_cam->addWidget(label_DeqShow, 6, 0);
		Layout_cam->addWidget(LineEdit_DeqShow[i], 6, 1, 1, 2);
		label_DeqShow->setAlignment(Qt::AlignCenter);

		QLabel *label_DeqSave = new QLabel(QStringLiteral("存储队列："));
		LineEdit_DeqSave[i] = new QLineEdit;
		Layout_cam->addWidget(label_DeqSave, 7, 0);
		Layout_cam->addWidget(LineEdit_DeqSave[i], 7, 1, 1, 2);
		label_DeqSave->setAlignment(Qt::AlignCenter);

		QLabel *label_DeqSet = new QLabel(QStringLiteral("内触发行频："));
		LineEdit_DeqSet[i] = new QLineEdit;
		Layout_cam->addWidget(label_DeqSet, 8, 0);
		Layout_cam->addWidget(LineEdit_DeqSet[i], 8, 1);
		label_DeqSet->setAlignment(Qt::AlignCenter);
		LineEdit_DeqSet[i]->setFixedWidth(60);
		QPushButton *pb_setfreg = new QPushButton(QStringLiteral("设置"));
		pb_setfreg->setObjectName(QString::number(i, 10));
		connect(pb_setfreg, &QPushButton::clicked, this, &QSetCameraDlg::pb_setfreg_Button);
		Layout_cam->addWidget(pb_setfreg, 8, 2, 1, 1);
		pb_setfreg->setMinimumWidth(120);
		pb_setfreg->setMinimumHeight(40);


		ui->horizontalLayout->addLayout(Layout_cam);

	}
	//更新显示行频
	timerprogress_freg = new QTimer(this);
	connect(timerprogress_freg, SIGNAL(timeout()), this, SLOT(timerUpDate_freg()));
	timerprogress_freg->setInterval(10);
	timerprogress_freg->start();

	//更新显示2个队列的长度
	timerprogress_deque = new QTimer(this);
	connect(timerprogress_deque, SIGNAL(timeout()), this, SLOT(timerUpDate_deque()));
	timerprogress_deque->setInterval(10);//更新显示队列
	timerprogress_deque->start();

	// ===== 上下结构：上=亮度模式开关，下=相机参数 =====
	// 下方 GroupBox：容纳原有 per-camera 控件（曝光/增益/行频/队列）
	QGroupBox *bottomGroup = new QGroupBox(QStringLiteral("相机参数"), this);
	bottomGroup->setLayout(ui->horizontalLayout);

	// 上方 GroupBox：手动亮度 / 自动亮度 单选，默认手动
	QGroupBox *topGroup = new QGroupBox(QStringLiteral("亮度模式"), this);
	QHBoxLayout *topLayout = new QHBoxLayout();
	m_radioManual = new QRadioButton(QStringLiteral("手动亮度"));
	m_radioAuto = new QRadioButton(QStringLiteral("自动亮度"));
	m_radioManual->setChecked(true); // 默认手动亮度
	QButtonGroup *modeGroup = new QButtonGroup(this);
	modeGroup->addButton(m_radioManual);
	modeGroup->addButton(m_radioAuto);
	topLayout->addWidget(m_radioManual);
	topLayout->addWidget(m_radioAuto);
	topLayout->addStretch();
	topGroup->setLayout(topLayout);
	topGroup->setMaximumHeight(60);
	// 与全局总开关联动（QShowThd 负责边沿检测/复位/重播种）
	connect(m_radioAuto, &QRadioButton::toggled, this, &QSetCameraDlg::onBrightnessModeChanged);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addWidget(topGroup);
	mainLayout->addWidget(bottomGroup);
	this->setLayout(mainLayout);

	// 初值与界面一致：默认手动
	g_autoBrightnessEnabled.store(false);
}
// 手动/自动亮度切换：仅翻转全局总开关，其余逻辑全在 QShowThd 门控内，
// 关闭时对既有流程零影响。
void QSetCameraDlg::onBrightnessModeChanged()
{
	g_autoBrightnessEnabled.store(m_radioAuto->isChecked());
}

QSetCameraDlg::~QSetCameraDlg()
{
    delete ui;
}

void QSetCameraDlg::show_us()
{
	//get所有相机的曝光
	for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
	{
		if (m_cam[i] != NULL)
		{
			double dtmp = -2;
 			if (m_cam[i]->GetExpTime(dtmp))
			{
				qDebug() << g_cameraInfor[i].Name << QStringLiteral("获取曝光：") << dtmp;
 				LineEdit_us[i]->setText(QString::number(dtmp, 'f', 0));
				
			}
			else
				qDebug() << g_cameraInfor[i].Name << QStringLiteral("获取曝光失败：") << dtmp;
		}
	}
}
void QSetCameraDlg::show_gain()
{
	//get所有相机的增益
	for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
	{
		if (m_cam[i] != NULL)
		{
			double dtmp = -2;
			if (m_cam[i]->GetCamGain(dtmp))
			{
				qDebug() << g_cameraInfor[i].Name << QStringLiteral("获取增益：") << dtmp;
				LineEdit_gain[i]->setText(QString::number(dtmp, 'f', 0));

			}
			else
				qDebug() << g_cameraInfor[i].Name << QStringLiteral("获取增益失败：") << dtmp;
		}
	}
}
void QSetCameraDlg::show_Frequency()
{
	//get所有相机的增益
	for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
	{
		if (m_cam[i] != NULL)
		{
			double dtmp = -2;
			if (m_cam[i]->GetCamFreg(dtmp))
			{
			//	qDebug() << g_cameraInfor[i].Name << QStringLiteral("获取行频：") << dtmp;
				LineEdit_Frequency[i]->setText(QString::number(dtmp, 'f', 6));

			}
			else
				qDebug() << g_cameraInfor[i].Name << QStringLiteral("获取行频失败：") << dtmp;
		}
	}
}
void QSetCameraDlg::timerUpDate_freg()
{
	show_Frequency();
}
void QSetCameraDlg::setCamera(ICamera *pDel[])
{
	for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
	{
		m_cam[i] = pDel[i];
	}
}
void QSetCameraDlg::pb_setus_Button()
{
	//取得相机序号
	QObject *object = QObject::sender();
	QPushButton *push_button = qobject_cast<QPushButton *>(object);
	QString object_name = push_button->objectName();
	int iChl_num = object_name.toInt();

	//取得具体曝光值
  	int iValue_us = LineEdit_us[iChl_num]->text().toInt();
	if ((iValue_us < g_cameraInfor[iChl_num].ExposureTimeMin) || (iValue_us > g_cameraInfor[iChl_num].ExposureTimeMax))
	{
		//提示超限，重新设置
		QString strcontext = QString(QStringLiteral("请将曝光值设置为%1≤u≤%2范围之内")).arg(g_cameraInfor[iChl_num].ExposureTimeMin).arg(g_cameraInfor[iChl_num].ExposureTimeMax);
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, QStringLiteral("敬请注意"),
			strcontext,
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		return;
	}

	set_camera_us(iChl_num, iValue_us);
}
void QSetCameraDlg::set_camera_us(int icam,int ivalue)
{
	if (m_cam[icam] != NULL)
	{
 		if (m_cam[icam]->SetExpTime(ivalue))
		{
			double actualValue = 0.0;
			if (!m_cam[icam]->GetExpTime(actualValue))
			{
				qDebug() << g_cameraInfor[icam].Name << QStringLiteral("曝光回读失败，未保存配置：") << ivalue;
				return;
			}

			g_cameraInfor[icam].ExposureTime = actualValue;
			const QString value = QString::number(actualValue, 'f', 2);
			if (!saveManualCameraValue(icam, "ExposureTime", value))
			{
				QMessageBox::warning(this, QStringLiteral("保存失败"),
					QStringLiteral("曝光已写入相机，但未能保存到 Config.xml。"));
				return;
			}
			LineEdit_us[icam]->setText(QString::number(actualValue, 'f', 0));
			qDebug() << g_cameraInfor[icam].Name << QStringLiteral("曝光已设置并保存：") << actualValue;
 	 	}
		else
			qDebug() << g_cameraInfor[icam].Name << QStringLiteral("设置曝光失败：") << ivalue;
	}
}
 
void QSetCameraDlg::pb_setgain_Button()
{
	//取得相机序号
	QObject *object = QObject::sender();
	QPushButton *push_button = qobject_cast<QPushButton *>(object);
	QString object_name = push_button->objectName();
	int iChl_num = object_name.toInt();

	//取得具体增益值
	int iValue_gain = LineEdit_gain[iChl_num]->text().toInt();
	if ((iValue_gain < g_cameraInfor[iChl_num].GainMin) || (iValue_gain > g_cameraInfor[iChl_num].GainMax))
	{
		//提示超限，重新设置
		QString strcontext = QString(QStringLiteral("请将增益值设置为%1≤u≤%2范围之内")).arg(g_cameraInfor[iChl_num].GainMin).arg(g_cameraInfor[iChl_num].GainMax);
 		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, QStringLiteral("敬请注意"),
			strcontext,
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		return;
	}
	set_camera_gain(iChl_num, iValue_gain);
}
void QSetCameraDlg::set_camera_gain(int icam, int ivalue)
{
	if (m_cam[icam] != NULL)
	{
		if (m_cam[icam]->SetCamGain(ivalue))
		{
			double actualValue = 0.0;
			if (!m_cam[icam]->GetCamGain(actualValue))
			{
				qDebug() << g_cameraInfor[icam].Name << QStringLiteral("增益回读失败，未保存配置：") << ivalue;
				return;
			}

			const int storedValue = qRound(actualValue);
			g_cameraInfor[icam].Gain = storedValue;
			if (!saveManualCameraValue(icam, "Gain", QString::number(storedValue)))
			{
				QMessageBox::warning(this, QStringLiteral("保存失败"),
					QStringLiteral("增益已写入相机，但未能保存到 Config.xml。"));
				return;
			}
			LineEdit_gain[icam]->setText(QString::number(actualValue, 'f', 0));
			qDebug() << g_cameraInfor[icam].Name << QStringLiteral("增益已设置并保存：") << actualValue;
		}
		else
			qDebug() << g_cameraInfor[icam].Name << QStringLiteral("设置增益失败：") << ivalue;
	}
}

bool QSetCameraDlg::saveManualCameraValue(int cameraIndex, const char *valueName, const QString &value)
{
	const QString configPath = QCoreApplication::applicationDirPath() + "/Config.xml";
	TiXmlDocument document(configPath.toLocal8Bit().constData());
	if (!document.LoadFile())
		return false;

	TiXmlElement *root = document.RootElement();
	TiXmlElement *cameraData = root ? root->FirstChildElement("camera_data") : NULL;
	const QString cameraName = QString("camera%1").arg(cameraIndex + 1, 2, 10, QChar('0'));
	TiXmlElement *camera = cameraData ? cameraData->FirstChildElement(cameraName.toLocal8Bit().constData()) : NULL;
	if (camera == NULL)
		return false;

	TiXmlElement *element = camera->FirstChildElement(valueName);
	if (element == NULL)
	{
		element = new TiXmlElement(valueName);
		camera->LinkEndChild(element);
	}
	if (element->FirstChild() != NULL)
		element->FirstChild()->SetValue(value.toLocal8Bit().constData());
	else
		element->LinkEndChild(new TiXmlText(value.toLocal8Bit().constData()));

	TiXmlPrinter printer;
	document.Accept(&printer);
	QSaveFile output(configPath);
	if (!output.open(QIODevice::WriteOnly))
		return false;
	if (output.write(printer.CStr(), static_cast<qint64>(printer.Size())) != static_cast<qint64>(printer.Size()))
	{
		output.cancelWriting();
		return false;
	}
	return output.commit();
}

void QSetCameraDlg::pb_setfreg_Button()
{
	//取得相机序号
	QObject *object = QObject::sender();
	QPushButton *push_button = qobject_cast<QPushButton *>(object);
	QString object_name = push_button->objectName();
	int iChl_num = object_name.toInt();

	//取得具体内触发行频值
	int iValue_freg= LineEdit_DeqSet[iChl_num]->text().toInt();
// 	if ((iValue_gain < g_cameraInfor[iChl_num].GainMin) || (iValue_gain > g_cameraInfor[iChl_num].GainMax))
// 	{
// 		//提示超限，重新设置
// 		QString strcontext = QString(QStringLiteral("请将增益值设置为%1≤u≤%2范围之内")).arg(g_cameraInfor[iChl_num].GainMin).arg(g_cameraInfor[iChl_num].GainMax);
// 		QMessageBox::StandardButton reply;
// 		reply = QMessageBox::question(this, QStringLiteral("敬请注意"),
// 			strcontext,
// 			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
// 		return;
// 	}
	set_camera_freg(iChl_num, iValue_freg);
}
void QSetCameraDlg::set_camera_freg(int icam, int ivalue)
{
	if (m_cam[icam] != NULL)
	{
		if (m_cam[icam]->SetCamFreg(ivalue))
		{
			qDebug() << g_cameraInfor[icam].Name << QStringLiteral("内触发行频：") << ivalue;
		}
		else
			qDebug() << g_cameraInfor[icam].Name << QStringLiteral("设置内触发行频失败：") << ivalue;
	}
}

void QSetCameraDlg::timerUpDate_deque()
{
	//get所有相机的显示和存储队列大小
	for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
	{
		if (m_cam[i] != NULL)
		{
			LineEdit_DeqShow[i]->setText(QString::number(m_cam[i]->get_ImgDeqShow_Size(), 'f', 0));
			LineEdit_DeqSave[i]->setText(QString::number(m_cam[i]->get_ImgDeqSave_Size(), 'f', 0));
 		}
	}

	// C1：自动亮度模式下，把相机实际曝光/增益实时回显到界面，
	// 让用户看到 AE 动态调整过程。手动模式不刷新，保留用户手填值。
	if (g_autoBrightnessEnabled.load())
	{
		// 队列显示仍保持原来的10ms刷新；相机参数回读在自动模式下降到500ms一次，
		// 避免UI线程以100Hz并发访问海康/DALSA参数节点。
		if (++m_autoStatusRefreshCounter >= 50)
		{
			m_autoStatusRefreshCounter = 0;
			show_us();
			show_gain();
		}
	}
	else
	{
		m_autoStatusRefreshCounter = 0;
	}
}
