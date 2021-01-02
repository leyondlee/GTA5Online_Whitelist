#include "addaddressdialog.h"
#include "ui_addaddressdialog.h"

AddAddressDialog::AddAddressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddAddressDialog)
{
    ui->setupUi(this);

    insertLineEdit = ui->insertLineEdit;
    insertPushButton = ui->insertPushButton;
    sessionPushButton = ui->sessionPushButton;
    addressListWidget = ui->addressListWidget;
    selectCountLabel = ui->selectCountLabel;

    customAddressListWidget = new CustomAddressListWidget(addressListWidget, selectCountLabel, true, this);

    QRegularExpression re(IP_PATTERN);
    QValidator *validator = new QRegularExpressionValidator(re, this);
    insertLineEdit->setValidator(validator);

    connect(insertPushButton, &QPushButton::clicked, this, &AddAddressDialog::onInsertButtonClicked);
    connect(sessionPushButton, &QPushButton::clicked, this, &AddAddressDialog::onSessionButtonClicked);
    connect(insertLineEdit, &QLineEdit::returnPressed, this, &AddAddressDialog::onInsertEditReturnPressed);
}

AddAddressDialog::~AddAddressDialog()
{
    delete ui;
}

void AddAddressDialog::keyPressEvent(QKeyEvent *e)
{
    int key = e->key();

    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        e->ignore();
    } else {
        QDialog::keyPressEvent(e);
    }
}

void AddAddressDialog::onInsertEditReturnPressed()
{
    QString address = insertLineEdit->text();
    insertAddressToList(address);

    insertLineEdit->clear();
}

void AddAddressDialog::onInsertButtonClicked(bool checked)
{
    onInsertEditReturnPressed();
}

void AddAddressDialog::onSessionButtonClicked(bool checked)
{
    Sniffer *sniffer = new Sniffer(this);

    SelectDeviceDialog selectDeviceDialog(sniffer, this);
    if (selectDeviceDialog.exec() == QDialog::Accepted) {
        QString deviceName = selectDeviceDialog.getSelectedDevice();

        if (!deviceName.isEmpty()) {
            SessionDialog sessionDialog(sniffer, deviceName, this);
            if (sessionDialog.exec() == QDialog::Accepted) {
                QStringList addresses = sessionDialog.getSelectedAddresses();
                for (int i = 0; i < addresses.count(); i += 1) {
                    insertAddressToList(addresses[i]);
                }
            }
        }
    }

    sniffer->deleteLater();
}

bool AddAddressDialog::isAddressInList(QString address)
{
    for (int i = 0; i < addressListWidget->count(); i += 1) {
        if (QString::compare(addressListWidget->item(i)->text(), address, Qt::CaseSensitive) == 0) {
            return true;
        }
    }

    return false;
}

void AddAddressDialog::insertAddressToList(QString address)
{
    if (address.isEmpty()) {
        QMessageBox::information(this, "Information", "Please enter an IP address");
        return;
    }

    if (!IPTool::isValidAddress(address)) {
        QString text = QString("Invalid IP Address - %1").arg(address);
        QMessageBox::information(this, "Information", text);
        return;
    }

    if (isAddressInList(address)) {
        QString text = QString("IP Address already exists - %1").arg(address);
        QMessageBox::information(this, "Information", text);
        return;
    }

    customAddressListWidget->addAddressToList(address);
}

QStringList AddAddressDialog::getAddresses()
{
    return customAddressListWidget->getAddresses();
}
