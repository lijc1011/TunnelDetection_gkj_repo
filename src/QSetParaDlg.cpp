#include "QSetParaDlg.h"
#include "ui_QSetParaDlg.h"

QSetParaDlg::QSetParaDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QSetParaDlg)
{
    ui->setupUi(this);
    connect(ui->pushButtonSelect, &QPushButton::clicked, this, &QSetParaDlg::pushButtonSelect_slots);
}

QSetParaDlg::~QSetParaDlg()
{
    delete ui;
}
void QSetParaDlg::showInitPara()
{
    // 已读回Config.xml参数并显示
    ui->lineEditPath->setText(g_program_setInfor.DataDestPath);
}
void QSetParaDlg::pushButtonSelect_slots()
{
    // 获取选择的目录路径
    QString strRoot = QFileDialog::getExistingDirectory(nullptr, QObject::tr("Open Directory"), g_program_setInfor.DataDestPath);

    // 更新内存
    g_program_setInfor.DataDestPath = strRoot;

    // 界面显示
    ui->lineEditPath->setText(g_program_setInfor.DataDestPath);

    // 写入Config.xml
    QString curPath = QCoreApplication::applicationDirPath();
    QString filePath = curPath + "/Config.xml";

    // 准备路径
    char xmlPath[RouteInfoSize];
    strcpy(xmlPath, filePath.toLocal8Bit().data());

    TiXmlDocument *doc = new TiXmlDocument(xmlPath);
    if (!doc->LoadFile())
        return;
    TiXmlElement *bodyElement = doc->RootElement();
    TiXmlElement *program_inforElement = bodyElement->FirstChildElement("program_infor");

    writeToXmlElement(program_inforElement, "DataDestPath", g_program_setInfor.DataDestPath.toStdString().c_str());
    doc->SaveFile(xmlPath);
}