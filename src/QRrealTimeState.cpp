#include "QRrealTimeState.h"
#include "ui_QRrealTimeState.h"

QRrealTimeState::QRrealTimeState(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QRrealTimeState)
{
    ui->setupUi(this);
}

QRrealTimeState::~QRrealTimeState()
{
    delete ui;
}

void QRrealTimeState::runThd_slots(int iChl)
{
    ui->lineEdit->setText(QString::number(iChl, 10)); //
}