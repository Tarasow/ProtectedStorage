#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#include <QDialog>
// http://doc.qt.io/qt-4.8/qerrormessage.html -- go standard dialogs!

namespace Ui {
	class ErrorDialog;
}

class ErrorDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ErrorDialog(const char* reason, QWidget *parent = nullptr);
	~ErrorDialog();

private slots:

private:
	Ui::ErrorDialog *ui;
};

#endif // ERRORDIALOG_H
