#include "login_dialog.h"
#include "ui_login_dialog.h"
#include <QtWidgets>
#include <QHostAddress>
#include <qlabel.h>
#include <QLabel>
#include <QIntValidator>

login_dialog::login_dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::login_dialog)
{
    ui->setupUi(this);
    ui->PortInput->setValidator(new QIntValidator(1,10900,this));
    ui->AddressInput->setFocus();
}

login_dialog::~login_dialog()
{
    delete ui;
}

QString login_dialog::getAddress()
{
    return ui->AddressInput->text();
}

int login_dialog::getPort()
{
    return ui->PortInput->text().toInt();
}
