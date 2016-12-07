#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>
// http://doc.qt.io/qt-4.8/qmessagebox.html

namespace Ui {
	class HelpDialog;
}

class HelpDialog : public QDialog
{
	Q_OBJECT

public:
	explicit HelpDialog(QWidget* parent = nullptr);
	~HelpDialog();

public slots:
	void setupText(const char** text);

private:
	Ui::HelpDialog *ui;
};

#endif // HELPDIALOG_H
