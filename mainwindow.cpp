#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    statusLabel = ui->statusLabel;
    profileLabel = ui->profileLabel;
    whitelistOnPushButton = ui->whitelistOnPushButton;
    whitelistOffPushButton = ui->whitelistOffPushButton;
    addPushButton = ui->addPushButton;
    addressListWidget = ui->addressListWidget;
    selectCountLabel = ui->selectCountLabel;

    customAddressListWidget = new CustomAddressListWidget(addressListWidget, selectCountLabel, true, this);

    firewallTool = new FirewallTool(this);
    if (firewallTool->hasError()) {
        displayFirewallError();
    }

    statusLabel->setText("-");
    profileLabel->setText("-");
    setFirewallStatus();

    connect(addPushButton, &QPushButton::clicked, this, &MainWindow::onAddButtonClicked);
    connect(whitelistOnPushButton, &QPushButton::clicked, this, &MainWindow::onWhitelistOnButtonClicked);
    connect(whitelistOffPushButton, &QPushButton::clicked, this, &MainWindow::onWhitelistOffButtonClicked);
    connect(customAddressListWidget, &CustomAddressListWidget::selectionRemoved, this, &MainWindow::onSelectionRemoved);

    initWhitelist();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initWhitelist()
{
    QStringList addresses = getSavedAddresses(true);
    for (int i = 0; i < addresses.count(); i += 1) {
        QString address = addresses[i];
        if (IPTool::isValidAddress(address)) {
            customAddressListWidget->addAddressToList(address);
        }
    }

    if (!firewallTool->isInitialised()) {
        return;
    }

    if (firewallTool->hasRule(getInboundRuleName()) && firewallTool->hasRule(getOutboundRuleName())) {
        if (!addFirewallRules()) {
            onFailAddRules();
        }

        whitelistOnPushButton->setEnabled(false);
        whitelistOffPushButton->setEnabled(true);
    } else {
        whitelistOnPushButton->setEnabled(true);
        whitelistOffPushButton->setEnabled(false);
    }
}

bool MainWindow::isAddressInList(QString address)
{
    QStringList addresses = customAddressListWidget->getAddresses();
    for (int i = 0; i < addresses.count(); i += 1) {
        if (QString::compare(addresses[i], address, Qt::CaseSensitive) == 0) {
            return true;
        }
    }

    return false;
}

void MainWindow::onSelectionRemoved(QMap<QString, QVariant> itemsRemoved)
{
    if (whitelistOffPushButton->isEnabled()) {
        if (!addFirewallRules()) {
            onFailAddRules();
        }
    }

    saveAddresses(true);
}

void MainWindow::onAddButtonClicked(bool checked)
{
    AddAddressDialog addAddressDialog(this);
    if (addAddressDialog.exec() == QDialog::Accepted) {
        QStringList addresses = addAddressDialog.getAddresses();
        for (int i = 0; i < addresses.count(); i += 1) {
            QString address = addresses[i];

            if (isAddressInList(address)) {
                QString text = QString("IP Address already exists - %1").arg(address);
                QMessageBox::information(this, "Information", text);
            } else {
                if (customAddressListWidget->addAddressToList(address) == -1) {
                    QString text = QString("Fail to add IP Address - %1").arg(address);
                    QMessageBox::critical(this, "Error", text);
                }
            }
        }

        if (whitelistOffPushButton->isEnabled() && !addFirewallRules()) {
            onFailAddRules();
        }

        saveAddresses(true);
    }
}

bool MainWindow::saveAddresses(bool prompt)
{
    QString filename = SETTINGS_FILENAME;
    QFile saveFile(filename);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        if (prompt) {
            QMessageBox::warning(this, "Warning", QString("Unable to write to file\n%1").arg(filename));
        }

        return false;
    }

    QStringList addresses;
    for (int i = 0; i < addressListWidget->count(); i += 1) {
        QListWidgetItem *item = addressListWidget->item(i);
        addresses.append(item->text());
    }

    QJsonObject jsonObject;
    jsonObject["Addresses"] = QJsonArray::fromStringList(addresses);

    QJsonDocument saveDoc(jsonObject);
    saveFile.write(saveDoc.toJson());

    return true;
}

QStringList MainWindow::getSavedAddresses(bool prompt)
{
    QString filename = SETTINGS_FILENAME;
    QFile loadFile(filename);
    if (!loadFile.exists()) {
        return QStringList();
    }

    if (!loadFile.open(QIODevice::ReadOnly)) {
        if (prompt) {
            QMessageBox::warning(this, "Warning", QString("Unable to read file\n%1").arg(filename));
        }

        return QStringList();
    }

    QByteArray saveData = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    QJsonObject jsonObject = loadDoc.object();

    if (!jsonObject.contains("Addresses")) {
        return QStringList();
    }

    QStringList addresses;
    QJsonArray jsonArray = jsonObject["Addresses"].toArray();
    for (int i = 0; i < jsonArray.count(); i += 1) {
        addresses.append(jsonArray[i].toString());
    }

    return addresses;
}

void MainWindow::displayFirewallError()
{
    QString error = firewallTool->getError();
    if (error.isEmpty()) {
        return;
    }

    QMessageBox::critical(this, "Error", error);
}

void MainWindow::setFirewallStatus()
{
    if (!firewallTool->isInitialised()) {
        return;
    }

    long currentProfiles = firewallTool->getCurrentProfiles();
    if (firewallTool->hasError()) {
        displayFirewallError();
        return;
    }

    QMap<QString, bool> profiles;
    if (currentProfiles & NET_FW_PROFILE2_DOMAIN) {
        profiles["Domain"] = firewallTool->isProfileEnabled(NET_FW_PROFILE2_DOMAIN);
    }
    if (currentProfiles & NET_FW_PROFILE2_PRIVATE) {
        profiles["Private"] = firewallTool->isProfileEnabled(NET_FW_PROFILE2_PRIVATE);
    }
    if (currentProfiles & NET_FW_PROFILE2_PUBLIC) {
        profiles["Public"] = firewallTool->isProfileEnabled(NET_FW_PROFILE2_PUBLIC);
    }

    if (!profiles.isEmpty()) {
        profileLabel->setText(profiles.keys().join("/"));

        if (profiles.count() == 1) {
            if (profiles[profiles.keys()[0]]) {
                statusLabel->setText("Enabled");
            } else {
                statusLabel->setText("Disabled");
            }
        } else {
            QString status;
            for (int i = 0; i < profiles.count(); i += 1) {
                if (!status.isEmpty()) {
                    status = QString("%1, ").arg(status);
                }

                QString key = profiles.keys()[i];
                status = QString("%1%2 - ").arg(status, key);
                if (profiles[key]) {
                    status = QString("%1%2").arg(status, "Enabled");
                } else {
                    status = QString("%1%2").arg(status, "Disabled");
                }
            }

            statusLabel->setText(status);
        }
    }
}

QString MainWindow::getAddressScope()
{
    QHostAddress minHostAddress = IPTool::getQHostAddress(MIN_ADDRESS);
    quint32 minIpv4Address = minHostAddress.toIPv4Address();

    QHostAddress maxHostAddress = IPTool::getQHostAddress(MAX_ADDRESS);
    quint32 maxIpv4Address = maxHostAddress.toIPv4Address();

    QString scope = QString("%1-").arg(MIN_ADDRESS);
    for (int i = 0; i < addressListWidget->count(); i += 1) {
        QString address = addressListWidget->item(i)->text();
        QHostAddress hostAddress = IPTool::getQHostAddress(address);
        quint32 ipv4Address = hostAddress.toIPv4Address();
        if (!hostAddress.isNull() && ipv4Address > minIpv4Address && ipv4Address < maxIpv4Address) {
            scope = QString("%1%2,%3-").arg(scope, IPTool::decrementAddress(address), IPTool::incrementAddress(address));
        }
    }
    scope = QString("%1%2").arg(scope, MAX_ADDRESS);

    return scope;
}

QString MainWindow::getInboundRuleName()
{
    return QString("%1 - Inbound").arg(APPNAME);
}

QString MainWindow::getOutboundRuleName()
{
    return QString("%1 - Outbound").arg(APPNAME);
}

bool MainWindow::removeFirewallRules()
{
    QString inboundRuleName = getInboundRuleName();
    QString outboundRuleName = getOutboundRuleName();

    bool inboundSuccess = true;
    if (firewallTool->hasRule(inboundRuleName)) {
        inboundSuccess = firewallTool->removeRule(inboundRuleName);
    }

    bool outboundSuccess = true;
    if (firewallTool->hasRule(outboundRuleName)) {
        inboundSuccess = firewallTool->removeRule(outboundRuleName);
    }

    return inboundSuccess && outboundSuccess;
}

bool MainWindow::addFirewallRules()
{
    removeFirewallRules();

    QString inboundRuleName = getInboundRuleName();
    QString outboundRuleName = getOutboundRuleName();

    QString localPorts = QString("%1").arg(GTA5ONLINE_PORT);
    QString remoteAddresses = getAddressScope();

    bool inboundSuccess = firewallTool->addRule(inboundRuleName, "", APPNAME, "", NET_FW_IP_PROTOCOL_UDP, "", localPorts, remoteAddresses, "", NET_FW_RULE_DIR_IN, NET_FW_ACTION_BLOCK, true);
    bool outboundSuccess = firewallTool->addRule(outboundRuleName, "", APPNAME, "", NET_FW_IP_PROTOCOL_UDP, "", localPorts, remoteAddresses, "", NET_FW_RULE_DIR_OUT, NET_FW_ACTION_BLOCK, true);

    return inboundSuccess && outboundSuccess;
}

void MainWindow::onWhitelistOnButtonClicked(bool checked)
{
    if (!firewallTool->isInitialised()) {
        return;
    }

    if (!addFirewallRules()) {
        onFailAddRules();
        return;
    }

    whitelistOnPushButton->setEnabled(false);
    whitelistOffPushButton->setEnabled(true);
}

void MainWindow::onWhitelistOffButtonClicked(bool checked)
{
    if (!firewallTool->isInitialised()) {
        return;
    }

    if (!removeFirewallRules()) {
        QMessageBox::critical(this, "Error", QString("Unable to remove inbound/outbound rules.\n\n%1").arg(firewallTool->getError()));
        return;
    }

    whitelistOnPushButton->setEnabled(true);
    whitelistOffPushButton->setEnabled(false);
}

QString MainWindow::getSettingsFilepath()
{
    return QDir(QGuiApplication::applicationDirPath()).filePath(SETTINGS_FILENAME);
}

void MainWindow::onFailAddRules()
{
    QMessageBox::critical(this, "Error", QString("Fail to add inbound/outbound rule.\n\n%1").arg(firewallTool->getError()));
    removeFirewallRules();
}
