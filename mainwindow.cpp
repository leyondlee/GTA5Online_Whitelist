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

    auto hotkey = new QHotkey(QKeySequence(Qt::CTRL + Qt::Key_F10), true, this);
    QObject::connect(hotkey, &QHotkey::activated, this, &MainWindow::onWhitelistHotkeyActivated);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":icons/icon_tray.png"));
    trayIcon->setToolTip(windowTitle());
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onIconActivated);

    QAction *quitAction = new QAction("Quit", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    QMenu *trayMenu = new QMenu(this);
    trayMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayMenu);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    #ifdef Q_OS_MACOS
    if (!event->spontaneous() || !isVisible()) {
        return;
    }
    #endif

    if (trayIcon->isVisible()) {
        trayIcon->showMessage(APP_NAME, "The program will keep running in the system tray.");
        hide();
        event->ignore();
    }
}

void MainWindow::onIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        break;
    case QSystemTrayIcon::DoubleClick:
        show();
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
    default:
        ;
    }
}

void MainWindow::onWhitelistHotkeyActivated()
{
    if (isWhitelistOn()) {
        if (turnWhitelistOff(false)) {
            QSound::play(":/sounds/WhitelistTurnedOff.wav");
        } else {
            QSound::play(":/sounds/SomethingWentWrong.wav");
        }
    } else {
        if (turnWhitelistOn(false)) {
            QSound::play(":/sounds/WhitelistTurnedOn.wav");
        } else {
            QSound::play(":/sounds/SomethingWentWrong.wav");
        }
    }
}

bool MainWindow::isWhitelistOn()
{
    return whitelistOffPushButton->isEnabled();
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
            onFailAddRules(true);
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
    if (isWhitelistOn()) {
        if (!addFirewallRules()) {
            onFailAddRules(true);
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

        if (isWhitelistOn() && !addFirewallRules()) {
            onFailAddRules(true);
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
    QString minAddress = MIN_ADDRESS;
    QString maxAddress = MAX_ADDRESS;

    QStringList addresses;
    for (int i = 0; i < addressListWidget->count(); i += 1) {
        QString address = addressListWidget->item(i)->text();
        addresses.append(address);
    }

    for (int i = 0; i < addresses.count(); i += 1) {
        QString address = addresses[i];
        if (QString::compare(address, minAddress, Qt::CaseSensitive) != 0) {
            break;
        }

        minAddress = IPTool::incrementAddress(address);
    }

    for (int i = addresses.count() - 1; i >= 0; i -= 1) {
        QString address = addresses[i];
        if (QString::compare(address, maxAddress, Qt::CaseSensitive) != 0) {
            break;
        }

        maxAddress = IPTool::decrementAddress(address);
    }

    QHostAddress minHostAddress = IPTool::getQHostAddress(minAddress);
    quint32 minIpv4Address = minHostAddress.toIPv4Address();

    QHostAddress maxHostAddress = IPTool::getQHostAddress(maxAddress);
    quint32 maxIpv4Address = maxHostAddress.toIPv4Address();

    if (minIpv4Address > maxIpv4Address) {
        return QString("0.0.0.0");
    }

    QString scope = QString("%1-").arg(minAddress);
    for (int i = 0; i < addressListWidget->count(); i += 1) {
        QString address = addressListWidget->item(i)->text();
        QHostAddress hostAddress = IPTool::getQHostAddress(address);
        quint32 ipv4Address = hostAddress.toIPv4Address();
        if (!hostAddress.isNull() && ipv4Address > minIpv4Address && ipv4Address < maxIpv4Address) {
            QString nextStartAddress = IPTool::incrementAddress(address);
            for (int j = i + 1; j < addressListWidget->count(); j += 1) {
                QString address = addressListWidget->item(j)->text();
                if (nextStartAddress != address) {
                    break;
                }

                nextStartAddress = IPTool::incrementAddress(address);
                i += 1;
            }

            scope = QString("%1%2,%3-").arg(scope, IPTool::decrementAddress(address), nextStartAddress);
        }
    }
    scope = QString("%1%2").arg(scope, maxAddress);

    return scope;
}

QString MainWindow::getInboundRuleName()
{
    return QString("%1 - Inbound").arg(APP_NAME);
}

QString MainWindow::getOutboundRuleName()
{
    return QString("%1 - Outbound").arg(APP_NAME);
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

    bool inboundSuccess = firewallTool->addRule(inboundRuleName, "", APP_NAME, "", NET_FW_IP_PROTOCOL_UDP, "", localPorts, remoteAddresses, "", NET_FW_RULE_DIR_IN, NET_FW_ACTION_BLOCK, true);
    bool outboundSuccess = firewallTool->addRule(outboundRuleName, "", APP_NAME, "", NET_FW_IP_PROTOCOL_UDP, "", localPorts, remoteAddresses, "", NET_FW_RULE_DIR_OUT, NET_FW_ACTION_BLOCK, true);

    return inboundSuccess && outboundSuccess;
}

void MainWindow::onWhitelistOnButtonClicked(bool checked)
{
    turnWhitelistOn(true);
}

void MainWindow::onWhitelistOffButtonClicked(bool checked)
{
    turnWhitelistOff(true);
}

QString MainWindow::getSettingsFilepath()
{
    return QDir(QGuiApplication::applicationDirPath()).filePath(SETTINGS_FILENAME);
}

void MainWindow::onFailAddRules(bool prompt)
{
    if (prompt) {
        QMessageBox::critical(this, "Error", QString("Fail to add inbound/outbound rule.\n\n%1").arg(firewallTool->getError()));
    }

    removeFirewallRules();
}

bool MainWindow::turnWhitelistOn(bool prompt)
{
    if (!firewallTool->isInitialised()) {
        return false;
    }

    if (!addFirewallRules()) {
        onFailAddRules(prompt);

        return false;
    }

    whitelistOnPushButton->setEnabled(false);
    whitelistOffPushButton->setEnabled(true);

    return true;
}

bool MainWindow::turnWhitelistOff(bool prompt)
{
    if (!firewallTool->isInitialised()) {
        return false;
    }

    if (!removeFirewallRules()) {
        if (prompt) {
            QMessageBox::critical(this, "Error", QString("Unable to remove inbound/outbound rules.\n\n%1").arg(firewallTool->getError()));
        }

        return false;
    }

    whitelistOnPushButton->setEnabled(true);
    whitelistOffPushButton->setEnabled(false);

    return true;
}
