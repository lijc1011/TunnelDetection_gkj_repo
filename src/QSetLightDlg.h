#ifndef QSETLIGHTDLG_H
#define QSETLIGHTDLG_H

#include <QDialog>
#include "Globals.h"
#include "CDiDo.h"
#include "QMilThd.h"

namespace Ui {
class QSetLightDlg;
}

class QSetLightDlg : public QDialog
{
    Q_OBJECT

public:
    explicit QSetLightDlg(QMilThd *milDetect, QWidget *parent = nullptr);
    ~QSetLightDlg();

    CDiDo m_dido;
    QMilThd *set_milDetect;

    //////防水盖动作////////////////////////////////////////////////////////////////////
    bool m_isStartOrStop; // 开或关

    void startThd();
    void stopThd();
    void pauseThd();

    bool m_isBreakThd;
    bool m_isCanRun;
    std::thread genOrgThreads;
    static void runThdFun(QSetLightDlg *pParameter);
    void sub_runThdFun();
    std::mutex my_lock;

    //////开闭照明动作////////////////////////////////////////////////////////////////////
    void open_camera_light(int icamNum);
    void close_camera_light(int icamNum);

    //////1761卡每个通道继电器指示灯////////////////////////////////////////////////////////////////////
    void setLED(QLabel *label, int color, int size);
    QLabel *label_led[MAXCAMNUM];
    void freshLedStatus();

    void get1761xuhao();

public slots:
    void pushButton_openGaizieSlot();
    void pushButton_closeGaiziSlot();
    void pushButton_GD_openzhaomingSlot();
    void pushButton_GD_closemingzhaoSlot();

    void open_zhaoming_Slots();
    void close_zhaoming_Slots();


private:
    Ui::QSetLightDlg *ui;
};

#endif // QSETLIGHTDLG_H
