#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <QDateTime>
#include <QTimer>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

#include "sniffer.h"
#include "customaddresslistwidget.h"

#ifndef SESSIONDIALOG_H
#define SESSIONDIALOG_H

#define REMOVE_THRESHOLD 5
#define IPLOOKUP_SERVER "http://www.geoplugin.net/json.gp?ip={address}"

class SessionDialogThread;

namespace Ui {
class SessionDialog;
}

class SessionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SessionDialog(Sniffer *sniffer, QString deviceName, QWidget *parent = nullptr);
    ~SessionDialog();
    QStringList getSelectedAddresses();

private:
    Ui::SessionDialog *ui;
    QTableWidget *addressTableWidget;
    QLabel *selectCountLabel;
    QLabel *foundCountLabel;
    Sniffer *sniffer;
    QStringList addresses;
    QTimer *updateTimer;
    QNetworkAccessManager *manager;

    void onFinished(int result);
    bool isValidSource(QString address);
    QTableWidgetItem *getAddressTableWidgetItem(QString address);
    void onNewSniffResult(QMap<QString, QVariant> result);
    void updateAddressTable();
    void setFoundCount();
    int addAddressToTable(QString address);
    void onAddressTableItemSelectionChanged();
};

#endif // SESSIONDIALOG_H
