#ifndef QRREALTIMESTATE_H
#define QRREALTIMESTATE_H

#include <QDialog>

namespace Ui {
class QRrealTimeState;
}

class QRrealTimeState : public QDialog
{
    Q_OBJECT

public:
    explicit QRrealTimeState(QWidget *parent = nullptr);
    ~QRrealTimeState();

private slots:
    void runThd_slots(int iChl);

    
private:
    Ui::QRrealTimeState *ui;
};

#endif // QRREALTIMESTATE_H
