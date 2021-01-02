#include <QObject>
#include <QDebug>
#include <QVariant>
#include <QThread>

#include <pcap.h>
#include <Winsock2.h>
#include <time.h>
#include <tchar.h>

#ifndef SNIFFER_H
#define SNIFFER_H

#define IPTOSBUFFERS 12
#define SNIFF_PORT 6672

class SnifferThread;

/* 4 bytes IP address */
typedef struct ip_address {
    u_char byte1;
    u_char byte2;
    u_char byte3;
    u_char byte4;
} ip_address;

/* IPv4 header */
typedef struct ip_header {
    u_char	ver_ihl;		// Version (4 bits) + Internet header length (4 bits)
    u_char	tos;			// Type of service
    u_short tlen;			// Total length
    u_short identification; // Identification
    u_short flags_fo;		// Flags (3 bits) + Fragment offset (13 bits)
    u_char	ttl;			// Time to live
    u_char	proto;			// Protocol
    u_short crc;			// Header checksum
    ip_address	saddr;		// Source address
    ip_address	daddr;		// Destination address
    u_int	op_pad;			// Option + Padding
} ip_header;

/* UDP header*/
typedef struct udp_header {
    u_short sport;			// Source port
    u_short dport;			// Destination port
    u_short len;			// Datagram length
    u_short crc;			// Checksum
} udp_header;

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

class SnifferThread : public QThread
{
    Q_OBJECT
public:
    SnifferThread(pcap_t *adhandle, QObject *parent = nullptr);

    void stop();

private:
    pcap_t *adhandle;
    bool loop = true;

    void run() override;

signals:
    void newResult(QMap<QString, QVariant> result);
    void timeout();
};

#endif // SNIFFER_H
