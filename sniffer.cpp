#include "sniffer.h"

Sniffer::Sniffer(QObject *parent) : QObject(parent)
{
    if (LoadNpcapDlls()) {
        dllLoaded = true;
    }

    loadDevices();

    connect(this, &QObject::destroyed, this, &Sniffer::onDestroyed);
}

void Sniffer::onDestroyed()
{
    freeDevices();
    qDebug() << "Sniffer Destroyed";
}

BOOL Sniffer::LoadNpcapDlls()
{
    _TCHAR npcap_dir[512];
    UINT len;
    len = GetSystemDirectory(npcap_dir, 480);
    if (!len) {
        qDebug() << "Error in GetSystemDirectory: %x" << GetLastError();
        return FALSE;
    }

    _tcscat_s(npcap_dir, 512, _T("\\Npcap"));
    if (SetDllDirectory(npcap_dir) == 0) {
        qDebug() << "Error in SetDllDirectory: %x" << GetLastError();
        return FALSE;
    }

    return TRUE;
}

char *Sniffer::iptos(u_long in)
{
    static char output[IPTOSBUFFERS][3*4+3+1];
    static short which;
    u_char *p;

    p = (u_char *)&in;
    which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
    sprintf(output[which], "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
    return output[which];
}

void Sniffer::freeDevices()
{
    if (devices != NULL) {
        pcap_freealldevs(devices);
    }

    devices = NULL;
}

bool Sniffer::isDllLoaded()
{
    return dllLoaded;
}

bool Sniffer::loadDevices()
{
    freeDevices();

    if (!dllLoaded) {
        return false;
    }

    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &devices, errbuf) == -1)
    {
        qDebug() << "Error in pcap_findalldevs: " << QString(errbuf);
        return false;
    }

    return true;
}

QStringList Sniffer::getDeviceNames()
{
    if (devices == NULL) {
        return QStringList();
    }

    QStringList deviceNames;
    for (pcap_if_t *d = devices; d != NULL; d = d->next) {
        deviceNames.append(QString(d->name));
    }

    return deviceNames;
}

int Sniffer::getDeviceIndex(QString name)
{
    if (devices == NULL) {
        return -1;
    }

    int i = 0;
    for (pcap_if_t *d = devices; d != NULL; d = d->next) {
        if (QString::compare(QString(d->name), name, Qt::CaseSensitive) == 0) {
            return i;
        }

        i += 1;
    }

    return -1;
}

QMap<QString, QVariant> Sniffer::getDeviceInfo(QString name)
{
    int index = getDeviceIndex(name);
    if (index == -1) {
        return QMap<QString, QVariant>();
    }

    pcap_if_t *device;
    int i;
    for (device = devices, i = 0; i < index; device = device->next, i += 1);

    QMap<QString, QVariant> info;
    info["Name"] = QString(device->name);
    info["Description"] = QString(device->description);

    if (device->flags & PCAP_IF_LOOPBACK) {
        info["Loopback"] = true;
    } else {
        info["Loopback"] = false;
    }

    info["Addresses"] = getDeviceAddresses(name);

    return info;
}

QList<QMap<QString, QVariant>> Sniffer::getDeviceAddressesWithInfo(QString name)
{
    int index = getDeviceIndex(name);
    if (index == -1) {
        return QList<QMap<QString, QVariant>>();
    }

    QList<QMap<QString, QVariant>> addresses;

    pcap_if_t *device;
    int i;
    for (device = devices, i = 0; i < index; device = device->next, i += 1);

    for (pcap_addr_t *a = device->addresses; a; a = a->next) {
        if (a->addr->sa_family == AF_INET) {
            QMap<QString, QVariant> addressInfo;

            if (a->addr) {
                addressInfo["Address"] = QString(iptos(((struct sockaddr_in *)a->addr)->sin_addr.s_addr));
            }

            if (a->netmask) {
                addressInfo["Netmask"] = QString(iptos(((struct sockaddr_in *)a->netmask)->sin_addr.s_addr));
            }

            if (a->broadaddr) {
                addressInfo["Broadcast"] = QString(iptos(((struct sockaddr_in *)a->broadaddr)->sin_addr.s_addr));
            }

            if (a->dstaddr) {
                addressInfo["Destination"] = QString(iptos(((struct sockaddr_in *)a->dstaddr)->sin_addr.s_addr));
            }

            addresses.append(addressInfo);
        }
    }

    return addresses;
}

QStringList Sniffer::getDeviceAddresses(QString name)
{
    QList<QMap<QString, QVariant>> addressesWithInfo = getDeviceAddressesWithInfo(name);
    if (addressesWithInfo.isEmpty()) {
        return QStringList();
    }

    QStringList addresses;
    for (int i = 0; i < addressesWithInfo.count(); i += 1) {
        QMap<QString, QVariant> addressInfo = addressesWithInfo[i];

        if (addressInfo.contains("Address")) {
            addresses.append(addressInfo["Address"].toString());
        }
    }

    return addresses;
}

bool Sniffer::startSniffing(QString name)
{
    int index = getDeviceIndex(name);
    if (index == -1) {
        return false;
    }

    pcap_if_t *device;
    int i;
    for (device = devices, i = 0; i < index; device = device->next, i += 1);

    pcap_t *adhandle;
    char errbuf[PCAP_ERRBUF_SIZE];
    if ((adhandle = pcap_open(device->name,	65536, 0, 1000,	NULL, errbuf)) == NULL) {
        qDebug() << "Unable to open the adapter. " << device->name << " is not supported by Npcap";
        return false;
    }

    /* Check the link layer. We support only Ethernet for simplicity. */
    if (pcap_datalink(adhandle) != DLT_EN10MB)
    {
        qDebug() << "This program works only on Ethernet networks.";
        return false;
    }

    u_int netmask;
    if (device->addresses != NULL) {
        /* Retrieve the mask of the first address of the interface */
        netmask = ((struct sockaddr_in *)(device->addresses->netmask))->sin_addr.S_un.S_addr;
    } else {
        /* If the interface is without addresses we suppose to be in a C class network */
        netmask=0xffffff;
    }

    QString packet_filter = QString("ip and udp port %1").arg(SNIFF_PORT);
    struct bpf_program fcode;
    if (pcap_compile(adhandle, &fcode, packet_filter.toLocal8Bit().data(), 1, netmask) < 0) {
        qDebug() << "Unable to compile the packet filter. Check the syntax.";
        return false;
    }

    if (pcap_setfilter(adhandle, &fcode) < 0) {
        qDebug() << "Error setting the filter.";
        return false;
    }

    snifferThread = new SnifferThread(adhandle, this);
    snifferThread->start();

    connect(snifferThread, &SnifferThread::newResult, this, [=](QMap<QString, QVariant> result) {
        emit newSniffResult(result);
    });
    connect(snifferThread, &SnifferThread::timeout, this, [=]() {
        emit sniffTimeout();
    });

    return true;
}

void Sniffer::stopSniffing()
{
    if (snifferThread != NULL) {
        snifferThread->stop();
        snifferThread->quit();
        snifferThread->wait();
    }
}
