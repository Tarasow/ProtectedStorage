#include "HelpDialog.h"
#include "ui_HelpDialog.h"

HelpDialog::HelpDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::HelpDialog)
{
	ui->setupUi(this);
}

HelpDialog::~HelpDialog()
{
	delete ui;
}

void HelpDialog::setupText(const char** text) {
	// сильно error-prone, не надо так
	this->setWindowTitle(text[0]);
	ui->label->setText(text[0]);
	ui->label_2->setText(text[1]);
	ui->label_3->setText(text[2]);
	ui->label_4->setText(text[3]);
	ui->label_5->setText(text[4]);
}
