#include <QObject>
#include <QDebug>
#include <QVariant>
#include <QtWin>
#include <QByteArray>

#include <windows.h>
#include <netfw.h>
#include <comutil.h>
#include <atlcomcli.h>

#ifndef FIREWALLTOOL_H
#define FIREWALLTOOL_H

class FirewallTool : public QObject
{
    Q_OBJECT

public:
    explicit FirewallTool(QObject *parent = nullptr);
    long getCurrentProfiles();
    bool isProfileEnabled(NET_FW_PROFILE_TYPE2 profileType);
    bool hasError();
    QString getError();
    bool isInitialised();
    bool removeRule(QString name);
    bool addRule(QString name, QString description, QString group, QString application, NET_FW_IP_PROTOCOL_ protocol, QString laddresses, QString lports, QString raddresses, QString rports, NET_FW_RULE_DIRECTION direction, NET_FW_ACTION_ action, bool enabled);
    QList<QMap<QString, QVariant>> getRules();
    bool hasRule(QString name);

private:
    bool initSuccess;
    QString error;
    HRESULT hrComInit = S_OK;
    INetFwPolicy2 *pNetFwPolicy2 = NULL;

    HRESULT WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2);
    void onDestroyed();
    bool init();
    void cleanup();
    QString formatHResult(HRESULT hr);
    QMap<QString, QVariant> getRuleInfo(INetFwRule* FwRule);
    QString bStrToQString(BSTR bstr);

signals:

};

#endif // FIREWALLTOOL_H
