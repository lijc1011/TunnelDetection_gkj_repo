#ifndef QSETPARADLG_H
#define QSETPARADLG_H

#include <QDialog>
#include "Globals.h"

namespace Ui {
class QSetParaDlg;
}

class QSetParaDlg : public QDialog
{
    Q_OBJECT

public:
    explicit QSetParaDlg(QWidget *parent = nullptr);
    ~QSetParaDlg();

    void showInitPara();

private slots:
    void pushButtonSelect_slots();


private:
    Ui::QSetParaDlg *ui;
};

#endif // QSETPARADLG_H
