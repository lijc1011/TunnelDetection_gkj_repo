#ifndef QSETMILEAGEDLG_H
#define QSETMILEAGEDLG_H

#include <QDialog>
#include "Globals.h"
#include "QMilThd.h"

namespace Ui {
class QSetMileageDlg;
}

class QSetMileageDlg : public QDialog
{
    Q_OBJECT

public:
    explicit QSetMileageDlg(QMilThd *milDetect, QWidget *parent = nullptr);
    ~QSetMileageDlg();

    QMilThd *set_milDetect;
    QTimer *timerprogress;
    QButtonGroup *m_pButtonGroup;
    int m_milMode;               // 里程模式
    QString m_zengjiaOrjianshao; // 里程增加减少
    double m_dStartMil;          // 带设定的初始里程值

    void initStart();
    void setMilDir(QString strMilDir);
    QString m_setMilDir;
private slots:
    void timerUpDateprogress(); // 定时响应
    void on_pushButton_set();
    void on_pushButton_QingLing();
    void onpButtonGroupClicked(QAbstractButton *button);

signals:
    void Mil_over_signals(QString ss, QString ee);
    void Mil_time_signals(double dmil, double dspeed);

private:
    Ui::QSetMileageDlg *ui;
};

#endif // QSETMILEAGEDLG_H
