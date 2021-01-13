#include "snifferthread.h"

SnifferThread::SnifferThread(pcap_t *adhandle, QObject *parent): QThread(parent)
{
    this->adhandle = adhandle;

    connect(this, &QObject::destroyed, this, [=]() {
        qDebug() << "SnifferThread Destroyed";
    });
}

void SnifferThread::run()
{
    int res;
    struct pcap_pkthdr *header;
    const u_char *pkt_data;

    while (loop && (res = pcap_next_ex(adhandle, &header, &pkt_data)) >= 0) {
        if (!loop) {
            break;
        }

        if (res == 0) {
            /* Timeout elapsed */
            emit timeout();
            continue;
        }

        struct tm ltime;
        char timestr[16];
        ip_header *ih;
        udp_header *uh;
        u_int ip_len;
        u_short sport,dport;
        time_t local_tv_sec;

        /* convert the timestamp to readable format */
        local_tv_sec = header->ts.tv_sec;
        localtime_s(&ltime, &local_tv_sec);
        strftime(timestr, sizeof timestr, "%H:%M:%S", &ltime);

        /* print timestamp and length of the packet */
        //printf("%s.%.6d len:%d ", timestr, header->ts.tv_usec, header->len);

        /* retireve the position of the ip header */
        ih = (ip_header *) (pkt_data + 14); //length of ethernet header

        /* retireve the position of the udp header */
        ip_len = (ih->ver_ihl & 0xf) * 4;
        uh = (udp_header *) ((u_char*)ih + ip_len);

        /* convert from network byte order to host byte order */
        sport = ntohs(uh->sport);
        dport = ntohs(uh->dport);

        /* print ip addresses and udp ports */
        /*printf("%d.%d.%d.%d.%d -> %d.%d.%d.%d.%d\n",
            ih->saddr.byte1,
            ih->saddr.byte2,
            ih->saddr.byte3,
            ih->saddr.byte4,
            sport,
            ih->daddr.byte1,
            ih->daddr.byte2,
            ih->daddr.byte3,
            ih->daddr.byte4,
            dport);*/

        QMap<QString, QVariant> result;
        result["saddr"] = QString("%1.%2.%3.%4").arg(ih->saddr.byte1).arg(ih->saddr.byte2).arg(ih->saddr.byte3).arg(ih->saddr.byte4);
        result["sport"] = sport;
        result["daddr"] = QString("%1.%2.%3.%4").arg(ih->daddr.byte1).arg(ih->daddr.byte2).arg(ih->daddr.byte3).arg(ih->daddr.byte4);
        result["dport"] = dport;

        emit newResult(result);
    }
}

void SnifferThread::stop()
{
    loop = false;
}
