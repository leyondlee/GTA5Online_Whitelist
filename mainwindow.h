#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QDir>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QShortcut>
#include <QSound>
#include <QHotkey>
#include <QSystemTrayIcon>
#include <QMenu>

#include "addaddressdialog.h"
#include "firewalltool.h"
#include "customaddresslistwidget.h"

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define APP_NAME "GTA5Online_Whitelist"
#define GTA5ONLINE_PORT 6672
#define MIN_ADDRESS "1.1.1.1"
#define MAX_ADDRESS "255.255.255.254"
#define SETTINGS_FILENAME "settings.json"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    QLabel *statusLabel;
    QLabel *profileLabel;
    QPushButton *whitelistOnPushButton;
    QPushButton *whitelistOffPushButton;
    QPushButton *addPushButton;
    QListWidget *addressListWidget;
    QLabel *selectCountLabel;
    FirewallTool *firewallTool;
    CustomAddressListWidget *customAddressListWidget;
    QHotkey *hotkey;
    QSystemTrayIcon *trayIcon;

    void onAddButtonClicked(bool checked);
    void setFirewallStatus();
    void displayFirewallError();
    void onWhitelistOnButtonClicked(bool checked);
    void onWhitelistOffButtonClicked(bool checked);
    void initWhitelist();
    bool saveAddresses(bool prompt = false);
    void onSelectionRemoved(QMap<QString, QVariant> itemsRemoved);
    QStringList getSavedAddresses(bool prompt = false);
    bool isAddressInList(QString address);
    bool removeFirewallRules();
    bool addFirewallRules();
    QString getAddressScope();
    QString getInboundRuleName();
    QString getOutboundRuleName();
    QString getSettingsFilepath();
    void onFailAddRules(bool prompt = false);
    void onWhitelistToggleShortcutActivated();
    bool isWhitelistOn();
    void onWhitelistHotkeyActivated();
    bool turnWhitelistOn(bool prompt = false);
    bool turnWhitelistOff(bool prompt = false);
    void onIconActivated(QSystemTrayIcon::ActivationReason reason);
    void initHotkey();
    void initTrayIcon();
    void setTrayIcon();
};
#endif // MAINWINDOW_H
