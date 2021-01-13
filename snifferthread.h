#include <QObject>
#include <QThread>
#include <QDebug>

#include <pcap.h>
#include <Winsock2.h>

#ifndef SNIFFERTHREAD_H
#define SNIFFERTHREAD_H

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

#endif // SNIFFERTHREAD_H
