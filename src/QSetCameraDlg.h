#ifndef QSETCAMERADLG_H
#define QSETCAMERADLG_H

#include <QDialog>
#include "Globals.h"
#include "Camera.h"

namespace Ui {
class QSetCameraDlg;
}

class QSetCameraDlg : public QDialog
{
    Q_OBJECT

public:
    explicit QSetCameraDlg(QWidget *parent = nullptr);
    ~QSetCameraDlg();

    	QLineEdit *LineEdit_us[MAXCAMNUM];
	QLineEdit *LineEdit_gain[MAXCAMNUM];
	QLineEdit *LineEdit_Frequency[MAXCAMNUM];
	QLineEdit *LineEdit_DeqShow[MAXCAMNUM]; 
	QLineEdit *LineEdit_DeqSave[MAXCAMNUM];
	QLineEdit *LineEdit_DeqSet[MAXCAMNUM];
	QTimer *timerprogress_freg;
	QTimer *timerprogress_deque;

	void show_us();
	void show_gain();
	void show_Frequency();
	void setCamera(ICamera *pDel[]);
	ICamera* m_cam[MAXCAMNUM];
	void set_camera_us(int icam, int ivalue);
	void set_camera_gain(int icam, int ivalue);
	void set_camera_freg(int icam, int ivalue);

private slots :
	void pb_setus_Button();//
	void pb_setgain_Button();//
	void pb_setfreg_Button();//
	void timerUpDate_freg(); //自动更新行频
	void timerUpDate_deque(); //自动更新2个队列
    
private:
    Ui::QSetCameraDlg *ui;
};

#endif // QSETCAMERADLG_H
