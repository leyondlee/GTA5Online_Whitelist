#include "firewalltool.h"

FirewallTool::FirewallTool(QObject *parent) : QObject(parent)
{
    initSuccess = init();

    connect(this, &QObject::destroyed, this, &FirewallTool::onDestroyed);
}

void FirewallTool::onDestroyed()
{
    cleanup();
    qDebug() << "FirewallTool Destroyed";
}

bool FirewallTool::hasError()
{
    return !error.isEmpty();
}

QString FirewallTool::getError()
{
    return error;
}

bool FirewallTool::isInitialised()
{
    return initSuccess;
}

bool FirewallTool::init()
{
    HRESULT hr = S_OK;

    // Initialize COM.
    hrComInit = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    // Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
    // initialized with a different mode. Since we don't care what the mode is,
    // we'll just use the existing mode.
    if (hrComInit != RPC_E_CHANGED_MODE) {
        if (FAILED(hrComInit)) {
            if (error != nullptr) {
                error = QString("CoInitializeEx failed: %1").arg(formatHResult(hrComInit));
            }

            cleanup();
            return false;
        }
    }

    // Retrieve INetFwPolicy2
    hr = WFCOMInitialize(&pNetFwPolicy2);
    if (FAILED(hr)) {
        cleanup();
        return false;
    }

    return true;
}

QString FirewallTool::formatHResult(HRESULT hr)
{
    QString str = QtWin::errorStringFromHresult(hr);
    if (str.isEmpty()) {
        QString hex = QByteArray::number((qint32) hr, 16);
        for (int i = 0; i < hex.count() - 8; i += 1) {
            hex.prepend('0');
        }
        str = QString("0x%1").arg(hex);
    }

    return str;
}

void FirewallTool::cleanup()
{
    // Release INetFwPolicy2
    if (pNetFwPolicy2 != NULL) {
        pNetFwPolicy2->Release();
    }

    // Uninitialize COM.
    if (SUCCEEDED(hrComInit)) {
        CoUninitialize();
    }
}

long FirewallTool::getCurrentProfiles()
{
    error.clear();

    if (!initSuccess) {
        return 0;
    }

    QString profile;

    HRESULT hr = S_OK;

    long CurrentProfilesBitMask = 0;

    // Retrieve Current Profiles bitmask
    hr = pNetFwPolicy2->get_CurrentProfileTypes(&CurrentProfilesBitMask);
    if (FAILED(hr)) {
        error = QString("get_CurrentProfileTypes failed: %1").arg(formatHResult(hr));
    }

    return CurrentProfilesBitMask;
}

HRESULT FirewallTool::WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2)
{
    HRESULT hr = S_OK;

    hr = CoCreateInstance(
        __uuidof(NetFwPolicy2),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(INetFwPolicy2),
        (void**)ppNetFwPolicy2);

    if (FAILED(hr)) {
        error = QString("CoCreateInstance for INetFwPolicy2 failed: %1").arg(formatHResult(hr));
    }

    return hr;
}

bool FirewallTool::isProfileEnabled(NET_FW_PROFILE_TYPE2 profileType)
{
    error.clear();

    if (!initSuccess) {
        return false;
    }

    VARIANT_BOOL bIsEnabled = FALSE;

    if (SUCCEEDED(pNetFwPolicy2->get_FirewallEnabled(profileType, &bIsEnabled))) {
        return bIsEnabled;
    }

    return false;
}

bool FirewallTool::hasRule(QString name)
{
    error.clear();

    bool found = false;

    HRESULT hr = S_OK;
    INetFwRules *pFwRules = NULL;
    INetFwRule *pFwRule = NULL;

    BSTR bstrRuleName = SysAllocString(name.toStdWString().c_str());

    hr = pNetFwPolicy2->get_Rules(&pFwRules);
    if (FAILED(hr)) {
        error = QString("get_Rules failed: %1").arg(formatHResult(hr));
        goto Cleanup;
    }

    hr = pFwRules->Item(bstrRuleName, &pFwRule);
    if (FAILED(hr)) {
        error = QString("Firewall Rule Item failed: %1").arg(formatHResult(hr));
        goto Cleanup;
    }

    found = (pFwRule != NULL);

Cleanup:
    // Free BSTR's
    SysFreeString(bstrRuleName);

    // Release the INetFwRule object
    if (pFwRule != NULL) {
        pFwRule->Release();
    }

    // Release the INetFwRules object
    if (pFwRules != NULL) {
        pFwRules->Release();
    }

    return found;
}

bool FirewallTool::removeRule(QString name)
{
    error.clear();

    bool success = false;

    HRESULT hr = S_OK;
    INetFwRules *pFwRules = NULL;

    BSTR bstrRuleName = SysAllocString(name.toStdWString().c_str());

    hr = pNetFwPolicy2->get_Rules(&pFwRules);
    if (FAILED(hr)) {
        error = QString("get_Rules failed: %1").arg(formatHResult(hr));
        goto Cleanup;
    }

    hr = pFwRules->Remove(bstrRuleName);
    if (FAILED(hr)) {
        error = QString("Firewall Rule Remove failed: %1").arg(formatHResult(hr));
        goto Cleanup;
    }

    success = true;

Cleanup:
    // Free BSTR's
    SysFreeString(bstrRuleName);

    // Release the INetFwRules object
    if (pFwRules != NULL) {
        pFwRules->Release();
    }

    return success;
}

bool FirewallTool::addRule(QString name, QString description, QString group, QString application, NET_FW_IP_PROTOCOL_ protocol, QString laddresses, QString lports, QString raddresses, QString rports, NET_FW_RULE_DIRECTION direction, NET_FW_ACTION_ action, bool enabled)
{
    error.clear();

    bool success = false;

    HRESULT hr = S_OK;
    INetFwRules *pFwRules = NULL;
    INetFwRule *pFwRule = NULL;

    BSTR bstrRuleName = SysAllocString(name.toStdWString().c_str());
    BSTR bstrRuleDescription = SysAllocString(description.toStdWString().c_str());
    BSTR bstrRuleGroup = SysAllocString(group.toStdWString().c_str());
    BSTR bstrRuleApplication = SysAllocString(application.toStdWString().c_str());
    BSTR bstrRuleLAddresses = SysAllocString(laddresses.toStdWString().c_str());
    BSTR bstrRuleLPorts = SysAllocString(lports.toStdWString().c_str());
    BSTR bstrRuleRAddresses = SysAllocString(raddresses.toStdWString().c_str());
    BSTR bstrRuleRPorts = SysAllocString(rports.toStdWString().c_str());

    long CurrentProfilesBitMask = getCurrentProfiles();

    // When possible we avoid adding firewall rules to the Public profile.
    // If Public is currently active and it is not the only active profile, we remove it from the bitmask
    if ((CurrentProfilesBitMask & NET_FW_PROFILE2_PUBLIC) && (CurrentProfilesBitMask != NET_FW_PROFILE2_PUBLIC)) {
        CurrentProfilesBitMask ^= NET_FW_PROFILE2_PUBLIC;
    }

    // Retrieve INetFwRules
    hr = pNetFwPolicy2->get_Rules(&pFwRules);
    if (FAILED(hr)) {
        error = QString("get_Rules failed: %1").arg(formatHResult(hr));
        goto Cleanup;
    }

    // Create a new Firewall Rule object.
    hr = CoCreateInstance(
        __uuidof(NetFwRule),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(INetFwRule),
        (void**)&pFwRule);
    if (FAILED(hr)) {
        error = QString("CoCreateInstance for Firewall Rule failed: %1").arg(formatHResult(hr));
        goto Cleanup;
    }

    // Populate the Firewall Rule object
    pFwRule->put_Name(bstrRuleName);
    pFwRule->put_Description(bstrRuleDescription);
    if (!application.isEmpty()) {
        pFwRule->put_ApplicationName(bstrRuleApplication);
    }
    pFwRule->put_Protocol(protocol);
    pFwRule->put_LocalAddresses(bstrRuleLAddresses);
    pFwRule->put_LocalPorts(bstrRuleLPorts);
    pFwRule->put_RemoteAddresses(bstrRuleRAddresses);
    pFwRule->put_RemotePorts(bstrRuleRPorts);
    pFwRule->put_Direction(direction);
    pFwRule->put_Grouping(bstrRuleGroup);
    pFwRule->put_Profiles(CurrentProfilesBitMask);
    pFwRule->put_Action(action);

    if (enabled) {
        pFwRule->put_Enabled(VARIANT_TRUE);
    } else {
        pFwRule->put_Enabled(VARIANT_FALSE);
    }

    // Add the Firewall Rule
    hr = pFwRules->Add(pFwRule);
    if (FAILED(hr)) {
        error = QString("Firewall Rule Add failed: %1").arg(formatHResult(hr));
        goto Cleanup;
    }

    success = true;

Cleanup:
    // Free BSTR's
    SysFreeString(bstrRuleName);
    SysFreeString(bstrRuleDescription);
    SysFreeString(bstrRuleGroup);
    SysFreeString(bstrRuleApplication);
    SysFreeString(bstrRuleLAddresses);
    SysFreeString(bstrRuleLPorts);
    SysFreeString(bstrRuleRAddresses);
    SysFreeString(bstrRuleRPorts);

    // Release the INetFwRule object
    if (pFwRule != NULL) {
        pFwRule->Release();
    }

    // Release the INetFwRules object
    if (pFwRules != NULL) {
        pFwRules->Release();
    }

    return success;
}

QList<QMap<QString, QVariant>> FirewallTool::getRules()
{
    error.clear();

    QList<QMap<QString, QVariant>> rules;

    HRESULT hr = S_OK;

    ULONG cFetched = 0;
    CComVariant var;

    IUnknown *pEnumerator = NULL;
    IEnumVARIANT *pVariant = NULL;

    INetFwRules *pFwRules = NULL;
    INetFwRule *pFwRule = NULL;

    // Retrieve INetFwRules
    hr = pNetFwPolicy2->get_Rules(&pFwRules);
    if (FAILED(hr)) {
        error = QString("get_Rules failed: %1").arg(formatHResult(hr));
        goto Cleanup;
    }

    // Iterate through all of the rules in pFwRules
    pFwRules->get__NewEnum(&pEnumerator);
    if (pEnumerator) {
        hr = pEnumerator->QueryInterface(__uuidof(IEnumVARIANT), (void **) &pVariant);
    }

    while(SUCCEEDED(hr) && hr != S_FALSE) {
        var.Clear();
        hr = pVariant->Next(1, &var, &cFetched);

        if (S_FALSE != hr) {
            if (SUCCEEDED(hr)) {
                hr = var.ChangeType(VT_DISPATCH);
            }

            if (SUCCEEDED(hr)) {
                hr = (V_DISPATCH(&var))->QueryInterface(__uuidof(INetFwRule), reinterpret_cast<void**>(&pFwRule));
            }

            if (SUCCEEDED(hr)) {
                // Output the properties of this rule
                QMap<QString, QVariant> ruleInfo = getRuleInfo(pFwRule);
                if (!ruleInfo.isEmpty()) {
                    rules.append(ruleInfo);
                }
            }
        }
    }

Cleanup:
    if (pEnumerator != NULL) {
        pEnumerator->Release();
    }

    if (pVariant != NULL) {
        pVariant->Release();
    }

    // Release pFwRule
    if (pFwRule != NULL) {
        pFwRule->Release();
    }

    // Release the INetFwRules object
    if (pFwRules != NULL) {
        pFwRules->Release();
    }

    return rules;
}

QMap<QString, QVariant> FirewallTool::getRuleInfo(INetFwRule* FwRule)
{
    QMap<QString, QVariant> ruleInfo;

    variant_t InterfaceArray;
    variant_t InterfaceString;

    VARIANT_BOOL bEnabled;
    BSTR bstrVal;

    long lVal = 0;
    long lProfileBitmask = 0;

    NET_FW_RULE_DIRECTION fwDirection;
    NET_FW_ACTION fwAction;

    if (SUCCEEDED(FwRule->get_Name(&bstrVal))) {
        ruleInfo["Name"] = bStrToQString(bstrVal);
    }

    if (SUCCEEDED(FwRule->get_Description(&bstrVal))) {
        ruleInfo["Description"] = bStrToQString(bstrVal);
    }

    if (SUCCEEDED(FwRule->get_ApplicationName(&bstrVal))) {
        ruleInfo["ApplicationName"] = bStrToQString(bstrVal);
    }

    if (SUCCEEDED(FwRule->get_ServiceName(&bstrVal))) {
        ruleInfo["ServiceName"] = bStrToQString(bstrVal);
    }

    if (SUCCEEDED(FwRule->get_Protocol(&lVal))) {
        ruleInfo["Protocol"] = QVariant::fromValue(lVal);

        if (lVal != NET_FW_IP_VERSION_V4 && lVal != NET_FW_IP_VERSION_V6) {
            if (SUCCEEDED(FwRule->get_LocalPorts(&bstrVal))) {
                ruleInfo["LocalPorts"] = bStrToQString(bstrVal);
            }

            if (SUCCEEDED(FwRule->get_RemotePorts(&bstrVal))) {
                ruleInfo["RemotePorts"] = bStrToQString(bstrVal);
            }
        } else {
            if (SUCCEEDED(FwRule->get_IcmpTypesAndCodes(&bstrVal))) {
                ruleInfo["IcmpTypesAndCodes"] = bStrToQString(bstrVal);
            }
        }
    }

    if (SUCCEEDED(FwRule->get_LocalAddresses(&bstrVal))) {
        ruleInfo["LocalAddresses"] = bStrToQString(bstrVal);
    }

    if (SUCCEEDED(FwRule->get_RemoteAddresses(&bstrVal))) {
        ruleInfo["RemoteAddresses"] = bStrToQString(bstrVal);
    }

    if (SUCCEEDED(FwRule->get_Profiles(&lProfileBitmask))) {
        ruleInfo["Profiles"] = QVariant::fromValue(lProfileBitmask);
    }

    if (SUCCEEDED(FwRule->get_Direction(&fwDirection))) {
        ruleInfo["Direction"] = fwDirection;
    }

    if (SUCCEEDED(FwRule->get_Action(&fwAction))) {
        ruleInfo["Action"] = fwAction;
    }

    if (SUCCEEDED(FwRule->get_Interfaces(&InterfaceArray))) {
        if (InterfaceArray.vt != VT_EMPTY) {
            SAFEARRAY *pSa = NULL;

            QStringList interfaces;

            pSa = InterfaceArray.parray;
            for(long index= pSa->rgsabound->lLbound; index < (long)pSa->rgsabound->cElements; index++) {
                SafeArrayGetElement(pSa, &index, &InterfaceString);
                interfaces.append(bStrToQString((BSTR) InterfaceString.bstrVal));
            }

            ruleInfo["Interfaces"] = interfaces;
        }
    }

    if (SUCCEEDED(FwRule->get_InterfaceTypes(&bstrVal))) {
        ruleInfo["InterfaceTypes"] = bStrToQString(bstrVal);
    }

    if (SUCCEEDED(FwRule->get_Enabled(&bEnabled))) {
        ruleInfo["Enabled"] = bEnabled;
    }

    if (SUCCEEDED(FwRule->get_Grouping(&bstrVal))) {
        ruleInfo["Grouping"] = bStrToQString(bstrVal);
    }

    if (SUCCEEDED(FwRule->get_EdgeTraversal(&bEnabled))) {
        ruleInfo["EdgeTraversal"] = bEnabled;
    }

    return ruleInfo;
}

QString FirewallTool::bStrToQString(BSTR bstr)
{
    char *text = _com_util::ConvertBSTRToString(bstr);
    QString str = QString(text);
    delete[] text;

    return str;
}
