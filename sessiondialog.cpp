#include "sessiondialog.h"
#include "ui_sessiondialog.h"

SessionDialog::SessionDialog(Sniffer *sniffer, QString deviceName, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SessionDialog)
{
    ui->setupUi(this);

    addressTableWidget = ui->addressTableWidget;
    selectCountLabel = ui->selectCountLabel;
    foundCountLabel = ui->foundCountLabel;

    QStringList headerLabels;
    headerLabels.append("IP Address");
    headerLabels.append("Country");
    addressTableWidget->setColumnCount(headerLabels.count());
    addressTableWidget->setHorizontalHeaderLabels(headerLabels);
    addressTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    addressTableWidget->verticalHeader()->setVisible(false);
    addressTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    selectCountLabel->setText("");
    foundCountLabel->setText("Loading...");

    this->sniffer = sniffer;
    addresses = sniffer->getDeviceAddresses(deviceName);

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &SessionDialog::updateAddressTable);
    updateTimer->start(1000);

    connect(sniffer, &Sniffer::sniffTimeout, this, [=]() {
        setFoundCount();
    });
    connect(sniffer, &Sniffer::newSniffResult, this, &SessionDialog::onNewSniffResult);
    if (!sniffer->startSniffing(deviceName)) {
        QMessageBox::critical(this, "Error", "Something went wrong.");
    }

    manager = new QNetworkAccessManager(this);

    connect(addressTableWidget, &QTableWidget::itemSelectionChanged, this, &SessionDialog::onAddressTableItemSelectionChanged);
    connect(this, &QDialog::finished, this, &SessionDialog::onFinished);
}

SessionDialog::~SessionDialog()
{
    delete ui;
}

void SessionDialog::onAddressTableItemSelectionChanged()
{
    int count = 0;
    QList<QTableWidgetSelectionRange> selectedRanges = addressTableWidget->selectedRanges();
    for (int i = 0; i < selectedRanges.count(); i += 1) {
        count += selectedRanges[i].rowCount();
    }


    selectCountLabel->setText(QString("%1 selected").arg(count));
}

void SessionDialog::onFinished(int result)
{
    sniffer->stopSniffing();
}

bool SessionDialog::isValidSource(QString address)
{
    for (int i = 0; i < addresses.count(); i += 1) {
        if (QString::compare(address, addresses[i], Qt::CaseSensitive) == 0) {
            return true;
        }
    }

    return false;
}

QTableWidgetItem *SessionDialog::getAddressTableWidgetItem(QString address)
{
    for (int i = 0; i < addressTableWidget->rowCount(); i += 1) {
        QTableWidgetItem *item = addressTableWidget->item(i, 0);

        if (QString::compare(address, item->text(), Qt::CaseSensitive) == 0) {
            return item;
        }
    }

    return NULL;
}

int SessionDialog::addAddressToTable(QString address)
{
    QHostAddress hostAddress = IPTool::getQHostAddress(address);
    if (hostAddress.isNull()) {
        return -1;
    }

    int row = 0;
    for (int i = 0; i < addressTableWidget->rowCount(); i += 1) {
        QHostAddress hostAddress2 = QHostAddress(addressTableWidget->item(i, 0)->text());
        if (!hostAddress2.isNull() && hostAddress.toIPv4Address() < hostAddress2.toIPv4Address()) {
            break;
        }

        row += 1;
    }

    QTableWidgetItem *item = new QTableWidgetItem(address);
    addressTableWidget->insertRow(row);
    addressTableWidget->setItem(row, 0, item);

    QString url = QString(IPLOOKUP_SERVER).replace("{address}", address);
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [=]() {
        if (item == getAddressTableWidgetItem(address)) {
            QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll());
            QJsonObject jsonObject = jsonDocument.object();

            if (jsonObject.contains("geoplugin_countryName")) {
                QTableWidgetItem *itemCountry = new QTableWidgetItem(jsonObject["geoplugin_countryName"].toString());
                addressTableWidget->setItem(item->row(), 1, itemCountry);
            }
        }

        reply->deleteLater();
    });

    return row;
}

void SessionDialog::onNewSniffResult(QMap<QString, QVariant> result)
{
    QString sourceAddresss = result["saddr"].toString();
    if (!isValidSource(sourceAddresss)) {
        return;
    }

    int sourcePort = result["sport"].toInt();
    if (sourcePort != SNIFF_PORT) {
        return;
    }

    QString destinationAddress = result["daddr"].toString();

    QTableWidgetItem *item = getAddressTableWidgetItem(destinationAddress);
    if (item == NULL) {
        int row = addAddressToTable(destinationAddress);
        if (row != -1) {
            item = addressTableWidget->item(row, 0);
            setFoundCount();
        }
    }

    item->setData(Qt::UserRole, QDateTime::currentSecsSinceEpoch());
}

void SessionDialog::updateAddressTable()
{
    int rowCount = addressTableWidget->rowCount();
    if (rowCount == 0) {
        return;
    }

    qint64 epochTimeNow = QDateTime::currentSecsSinceEpoch();

    QList<QTableWidgetItem *> itemsToRemove;

    for (int i = 0; i < rowCount; i += 1) {
        QTableWidgetItem *item = addressTableWidget->item(i, 0);
        if (epochTimeNow - item->data(Qt::UserRole).toLongLong() > REMOVE_THRESHOLD) {
            itemsToRemove.append(item);
        }
    }

    for (int i = 0; i < itemsToRemove.count(); i += 1) {
        QTableWidgetItem *item = itemsToRemove[i];
        addressTableWidget->removeRow(item->row());
    }

    setFoundCount();
}

void SessionDialog::setFoundCount()
{
    int rowCount = addressTableWidget->rowCount();
    QString text = QString("%1 found").arg(rowCount);

    foundCountLabel->setText(text);
}

QStringList SessionDialog::getSelectedAddresses()
{
    QStringList addresses;
    QList<QTableWidgetSelectionRange> selectedRanges = addressTableWidget->selectedRanges();
    for (int i = 0; i < selectedRanges.count(); i += 1) {
        QTableWidgetSelectionRange selectedRange = selectedRanges[i];
        for (int j = selectedRange.topRow(); j <= selectedRange.bottomRow(); j += 1) {
            QTableWidgetItem *item = addressTableWidget->item(j, 0);
            addresses.append(item->text());
        }
    }

    return addresses;
}
