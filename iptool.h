#include <QObject>
#include <QRegularExpression>
#include <QHostAddress>

#ifndef IPTOOL_H
#define IPTOOL_H

#define IP_PATTERN "^((25[0-5]|(2[0-4]|1[0-9]|[1-9]|)[0-9])(\\.(?!$)|$)){4}$"

class IPTool : public QObject
{
    Q_OBJECT

public:
    explicit IPTool(QObject *parent = nullptr);
    static bool isValidAddress(QString address);
    static QHostAddress getQHostAddress(QString address);
    static QHostAddress getQHostAddress(quint32 ipv4Address);
    static QString incrementAddress(QString address);
    static QString decrementAddress(QString address);

signals:

};

#endif // IPTOOL_H
