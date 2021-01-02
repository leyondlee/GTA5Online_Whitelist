#include <QObject>
#include <QListWidget>
#include <QLabel>
#include <QMenu>
#include <QHostAddress>

#include "iptool.h"

#ifndef CUSTOMADDRESSLISTWIDGET_H
#define CUSTOMADDRESSLISTWIDGET_H

class CustomAddressListWidget : public QObject
{
    Q_OBJECT

public:
    CustomAddressListWidget(QListWidget *listWidget, QLabel *selectCountLabel, bool customContextMenu = false, QObject *parent = nullptr);
    int addAddressToList(QString address);
    QStringList getAddresses();
    QStringList getSelectedAddresses();

private:
    QListWidget *listWidget;
    QLabel *selectCountLabel;

    void onListSelectionChanged();
    void onCustomContextMenuRequested(const QPoint &pos);
    void removeSelection();

signals:
    void selectionRemoved(QMap<QString, QVariant> itemsRemoved);
};

#endif // CUSTOMADDRESSLISTWIDGET_H
