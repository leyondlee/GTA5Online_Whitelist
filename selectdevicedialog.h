#include <QDialog>
#include <QTableWidget>

#include "sniffer.h"

#ifndef SELECTDEVICEDIALOG_H
#define SELECTDEVICEDIALOG_H

namespace Ui {
class SelectDeviceDialog;
}

class SelectDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectDeviceDialog(Sniffer *sniffer, QWidget *parent = nullptr);
    ~SelectDeviceDialog();
    QString getSelectedDevice();

private:
    Ui::SelectDeviceDialog *ui;
    QTableWidget *deviceTableWidget;
};

#endif // SELECTDEVICEDIALOG_H
