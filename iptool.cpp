#include "iptool.h"

IPTool::IPTool(QObject *parent) : QObject(parent)
{

}

bool IPTool::isValidAddress(QString address)
{
    QRegularExpression re(IP_PATTERN);
    QRegularExpressionMatch match = re.match(address);
    if (match.hasMatch()) {
        return true;
    }

    return false;
}

QHostAddress IPTool::getQHostAddress(QString address)
{
    QHostAddress hostAddress;
    if (!hostAddress.setAddress(address)) {
        return QHostAddress();
    }

    if (hostAddress.protocol() != QAbstractSocket::IPv4Protocol) {
        return QHostAddress();
    }

    return hostAddress;
}

QHostAddress IPTool::getQHostAddress(quint32 ipv4Address)
{
    QHostAddress hostAddress(ipv4Address);

    if (hostAddress.protocol() != QAbstractSocket::IPv4Protocol) {
        return QHostAddress();
    }

    return hostAddress;
}

QString IPTool::incrementAddress(QString address)
{
    QHostAddress hostAddress = getQHostAddress(address);
    if (hostAddress.isNull()) {
        return QString();
    }

    quint32 ipv4Address = hostAddress.toIPv4Address();
    ipv4Address += 1;
    QHostAddress newHostAddress = getQHostAddress(ipv4Address);
    if (newHostAddress.isNull()) {
        return QString();
    }

    return newHostAddress.toString();
}

QString IPTool::decrementAddress(QString address)
{
    QHostAddress hostAddress = getQHostAddress(address);
    if (hostAddress.isNull()) {
        return QString();
    }

    quint32 ipv4Address = hostAddress.toIPv4Address();
    ipv4Address -= 1;
    QHostAddress newHostAddress = getQHostAddress(ipv4Address);
    if (newHostAddress.isNull()) {
        return QString();
    }

    return newHostAddress.toString();
}
