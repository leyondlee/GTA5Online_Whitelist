#include "selectdevicedialog.h"
#include "ui_selectdevicedialog.h"

#include <QDebug>

SelectDeviceDialog::SelectDeviceDialog(Sniffer *sniffer, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SelectDeviceDialog)
{
    ui->setupUi(this);

    deviceTableWidget = ui->deviceTableWidget;

    QStringList headerLabels;
    headerLabels.append("Name");
    headerLabels.append("Description");
    headerLabels.append("Address");
    deviceTableWidget->setColumnCount(headerLabels.count());
    deviceTableWidget->setHorizontalHeaderLabels(headerLabels);
    deviceTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    deviceTableWidget->verticalHeader()->setVisible(false);
    deviceTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QStringList deviceNames = sniffer->getDeviceNames();
    for (int i = 0; i < deviceNames.count(); i += 1) {
        QString name = deviceNames[i];

        QMap<QString, QVariant> deviceInfo = sniffer->getDeviceInfo(name);

        if (!deviceInfo.isEmpty()) {
            QString address;
            QStringList addresses = deviceInfo["Addresses"].toStringList();

            if (!addresses.isEmpty()) {\
                int rowCount = deviceTableWidget->rowCount();
                deviceTableWidget->insertRow(rowCount);
                QTableWidgetItem *itemName = new QTableWidgetItem(deviceInfo["Name"].toString());
                deviceTableWidget->setItem(rowCount, 0, itemName);
                QTableWidgetItem *itemDescription = new QTableWidgetItem(deviceInfo["Description"].toString());
                deviceTableWidget->setItem(rowCount, 1, itemDescription);

                for (int i = 0; i < addresses.count(); i += 1) {
                    if (address.isEmpty()) {
                        address = addresses[i];
                    } else {
                        address = QString("%1\n%2").arg(address, addresses[i]);
                    }
                }
                QTableWidgetItem *itemAddress = new QTableWidgetItem(address);
                deviceTableWidget->setItem(rowCount, 2, itemAddress);
            }
        }
    }

    deviceTableWidget->resizeRowsToContents();
}

SelectDeviceDialog::~SelectDeviceDialog()
{
    delete ui;
}

QString SelectDeviceDialog::getSelectedDevice()
{
    QList<QTableWidgetItem *> selectedItems = deviceTableWidget->selectedItems();

    if (selectedItems.isEmpty()) {
        return QString();
    }

    return selectedItems.at(0)->text();
}
