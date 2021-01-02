#include "customaddresslistwidget.h"

CustomAddressListWidget::CustomAddressListWidget(QListWidget *listWidget, QLabel *selectCountLabel, bool customContextMenu, QObject *parent): QObject(parent)
{
    this->listWidget = listWidget;
    this->selectCountLabel = selectCountLabel;

    selectCountLabel->setText("");

    if (customContextMenu) {
        listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(listWidget, &QListWidget::customContextMenuRequested, this, &CustomAddressListWidget::onCustomContextMenuRequested);
    }

    connect(listWidget, &QListWidget::itemSelectionChanged, this, &CustomAddressListWidget::onListSelectionChanged);
}

void CustomAddressListWidget::onListSelectionChanged()
{
    int count = listWidget->selectedItems().count();
    if (count == 0) {
        selectCountLabel->clear();
        return;
    }

    selectCountLabel->setText(QString("%1 selected").arg(count));
}

void CustomAddressListWidget::onCustomContextMenuRequested(const QPoint &pos)
{
    if (listWidget->selectedItems().isEmpty()) {
        return;
    }

    QPoint globalPos = listWidget->mapToGlobal(pos);

    QMenu menu;
    menu.addAction("Remove Selected", this, &CustomAddressListWidget::removeSelection);
    menu.exec(globalPos);
}

void CustomAddressListWidget::removeSelection()
{
    QMap<QString, QVariant> removedItems;

    QList<QListWidgetItem *> items = listWidget->selectedItems();
    for (int i = 0; i < items.count(); i += 1) {
        QListWidgetItem *item = items[i];

        removedItems[item->text()] = item->data(Qt::UserRole);

        listWidget->removeItemWidget(item);
        delete item;
    }

    emit selectionRemoved(removedItems);
}

int CustomAddressListWidget::addAddressToList(QString address)
{
    QHostAddress hostAddress = IPTool::getQHostAddress(address);
    if (hostAddress.isNull()) {
        return -1;
    }

    int row = 0;
    for (int i = 0; i < listWidget->count(); i += 1) {
        QHostAddress hostAddress2 = QHostAddress(listWidget->item(i)->text());
        if (!hostAddress2.isNull() && hostAddress.toIPv4Address() < hostAddress2.toIPv4Address()) {
            break;
        }

        row += 1;
    }

    QListWidgetItem *item = new QListWidgetItem(address);
    item->setData(Qt::UserRole, address);
    listWidget->insertItem(row, item);

    return row;
}

QStringList CustomAddressListWidget::getAddresses()
{
    QStringList addresses;
    for (int i = 0; i < listWidget->count(); i += 1) {
        QListWidgetItem *item = listWidget->item(i);
        QVariant data = item->data(Qt::UserRole);
        if (!data.isNull()) {
            addresses.append(data.toString());
        }
    }

    return addresses;
}

QStringList CustomAddressListWidget::getSelectedAddresses()
{
    QList<QListWidgetItem *> selectedItems = listWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return QStringList();
    }

    QStringList addresses;
    for (int i = 0; i < selectedItems.count(); i += 1) {
        QListWidgetItem *item = selectedItems[i];
        QVariant data = item->data(Qt::UserRole);
        if (!data.isNull()) {
            addresses.append(data.toString());
        }
    }

    return addresses;
}
