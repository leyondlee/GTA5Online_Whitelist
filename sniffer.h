#include <QObject>
#include <QDebug>
#include <QVariant>
#include <QThread>

#include <tchar.h>

#include "snifferthread.h"

#ifndef SNIFFER_H
#define SNIFFER_H

#define IPTOSBUFFERS 12
#define SNIFF_PORT 6672

class Sniffer : public QObject
{
    Q_OBJECT

public:
    explicit Sniffer(QObject *parent = nullptr);

    bool isDllLoaded();
    QStringList getDeviceNames();
    int getDeviceIndex(QString name);
    QMap<QString, QVariant> getDeviceInfo(QString name);
    QList<QMap<QString, QVariant>> getDeviceAddressesWithInfo(QString name);
    QStringList getDeviceAddresses(QString name);
    bool startSniffing(QString name);
    void stopSniffing();

private:
    bool dllLoaded = false;
    pcap_if_t *devices = NULL;
    SnifferThread *snifferThread = NULL;

    BOOL LoadNpcapDlls();
    char *iptos(u_long in);
    bool loadDevices();
    void onDestroyed();
    void freeDevices();

signals:
    void newSniffResult(QMap<QString, QVariant> result);
    void sniffTimeout();
};

#endif // SNIFFER_H
