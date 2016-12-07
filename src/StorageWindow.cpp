#include "StorageWindow.h"
#include "ui_StorageWindow.h"
#include "ErrorDialog.h"
#include <QFileDialog>

StorageWindow::StorageWindow(const char* path, QWidget* parent) :
	QMainWindow(parent),
	ui(std::make_unique<Ui::StorageWindow>()),
	storage(ProtectedStorage::getInstance()),
	appPath(path),
	howToDialog(std::make_unique<HelpDialog>()),
	aboutDialog(std::make_unique<HelpDialog>())
{
	ui->setupUi(this);
	this->setupMenu();
	this->setupHelp();
	this->setupConnections();
	this->setupButtons(true);
	ui->pushButton_2->setDisabled(true);
}


void StorageWindow::setupMenu() {
	QAction* createAction = new QAction("Create storage", this);
	connect(createAction, SIGNAL(triggered()), this, SLOT(createClicked()));
	QAction* destroyAction = new QAction("Destroy storage", this);
	connect(destroyAction, SIGNAL(triggered()), this, SLOT(destroyClicked()));

	QMenu* menu = new QMenu("Menu", this);
	menu->addActions( { createAction, destroyAction } );

	QAction* howToAction = new QAction("How to use", this);
	connect(howToAction, SIGNAL(triggered()), this, SLOT(howToUse()));
	QAction* aboutAction = new QAction("About", this);
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

	QMenu* help = new QMenu("Help", this);
	help->addActions( { howToAction, aboutAction} );

	this->menuBar()->addMenu(menu);
	this->menuBar()->addMenu(help);
}

void StorageWindow::setupConnections() {
	connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(createClicked()));
	connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(destroyClicked()));
	connect(ui->toolButton, SIGNAL(clicked()), this, SLOT(rootFileClicked()));
	connect(ui->toolButton_2, SIGNAL(clicked()), this, SLOT(mountFileClicked()));
	connect(ui->pushButton_3, SIGNAL(pressed()), this, SLOT(showPassword()));
	connect(ui->pushButton_3, SIGNAL(released()), this, SLOT(dontShowPassword()));
}


void StorageWindow::setupHelp() {
	size_t textSize = 5;
	auto text = std::make_unique<const char*[]>(textSize); // std::array<const char*>?
	// QLabel умеет html
	text[0] = "How to use the program";
	text[1] = "At first, fill in all the fields:\n"
			  "Root directory - where you want to store\nyour encrypted data.\n"
			  "Mount - where you want to work with it.";
	text[2] = "Password - need to encrypt and decrypt.\n"
			  "Then, push the button \"Create storage\"\n"
			  "to create it.\n"
			  "To destroy the storage, push the button";
	text[3] = "\"Destroy storage\".\n\n"
			  "Remember: you can't create more than one\n"
			  "storage at the same time.";
	text[4] = "If an error occurs, the program will\n"
			  "report about it and offer some solutions.";

	howToDialog->setupText(text.get());

	text[0] = "About";
	text[1] = "Protected storage 0.9 beta\n\n"
			  "Created on C++ using FUSE library and Qt\n";
	text[2] = "Provides the functionality of creating\n"
			  "and using protected storage with any data.\n"
			  "The mounted directory is similar to an\n"
			  "regular logical disk.";
	text[3] = "\nWish you will find the using of this\n"
			  "program convenient and useful.\n";
	text[4] = "2016, Tarasov Kirill";

	aboutDialog->setupText(text.get());
}


void StorageWindow::setupButtons(bool createAvailable) {
	ui->pushButton->setEnabled(createAvailable);
	ui->toolButton->setEnabled(createAvailable);
	ui->toolButton_2->setEnabled(createAvailable);
	ui->lineEdit->setEnabled(createAvailable);
	ui->lineEdit_2->setEnabled(createAvailable);
	ui->lineEdit_3->setEnabled(createAvailable);

	ui->pushButton_2->setEnabled(!createAvailable);
}


StorageWindow::~StorageWindow() {
}


void StorageWindow::closeEvent(QCloseEvent* event) {
	howToDialog->close();
	aboutDialog->close();
	if (storage.get()->isMounted())
		while(!this->destroyStorage()) {};
	QMainWindow::closeEvent(event);
}


bool StorageWindow::createStorage() {
	auto root = ui->lineEdit->text().toStdString();  // std::array<const char*>?
	if (root.empty()) {
		this->error("Root directory field is empty.\nPlease, fill in all the fields.");
		return false;
	}
	auto mount = ui->lineEdit_2->text().toStdString();
	if (mount.empty()) {
		this->error("Mount directory field is empty.\nPlease, fill in all the fields.");
		return false;
	}
	auto pass = ui->lineEdit_3->text().toStdString();
	if (pass.empty()) {
		this->error("Password field is empty.\nPlease, fill in all the fields.");
		return false;
	}

	int size = 5;
	auto arr = std::make_unique<char*[]>(size);
	arr[0] = const_cast<char*>(appPath.c_str());
	arr[1] = const_cast<char*>("-f");
	arr[2] = const_cast<char*>(root.c_str());
	arr[3] = const_cast<char*>(mount.c_str());
	arr[4] = const_cast<char*>(pass.c_str());

	auto a = storage->createStorage(arr.get(), size);

	if (a == storageCreateStatus::alreadyCreated) {
		this->error("Protected storage is already created.\nDestroy it and then create a new.");
		return false;
	}
	if (a == storageCreateStatus::errorInCreating) {
		this->error("Something went wrong when creating\nthe storage."
					"\nCheck the fields of root and\nmount dirictories.");
		return false;
	}

	return true;
}


bool StorageWindow::destroyStorage() {
	auto status = storage->destroyStorage();
	if (status == storageDestroyStatus::nothingToDestroy) {
		this->error("There is no storage to destroy.");
		return false;
	}
	if (status == storageDestroyStatus::errorInDestroying) {
		this->error("Can't destroy storage.\nPlease, close all files in it.");
		return false;
	}
	return true;
}


void StorageWindow::howToUse() {
	howToDialog->show();
}


void StorageWindow::about() {
	aboutDialog->show();
}


void StorageWindow::error(const char* reason) {
	ErrorDialog e(reason);
	e.exec();
}


void StorageWindow::rootFileClicked() {
	QFileDialog qfd;
	qfd.exec(); // тут может быть cancel?
	ui->lineEdit->setText(qfd.directory().absolutePath()); // тут не должна быть директория?
}


void StorageWindow::mountFileClicked() {
	QFileDialog qfd;
	qfd.exec(); // тут может быть cancel?
	ui->lineEdit_2->setText(qfd.directory().path());
}


void StorageWindow::createClicked() {
	if (this->createStorage()) {
		this->setupButtons(false);
	}
}


void StorageWindow::destroyClicked() {
	if (this->destroyStorage()) {
		this->setupButtons(true);
	}
}


void StorageWindow::showPassword() {
	ui->lineEdit_3->setEchoMode(QLineEdit::EchoMode::Normal);
}


void StorageWindow::dontShowPassword() {
	ui->lineEdit_3->setEchoMode(QLineEdit::EchoMode::Password);
}
