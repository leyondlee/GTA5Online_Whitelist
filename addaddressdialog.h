#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QKeyEvent>
#include <QRegularExpression>

#include "sniffer.h"
#include "sessiondialog.h"
#include "selectdevicedialog.h"
#include "iptool.h"
#include "customaddresslistwidget.h"

#ifndef ADDADDRESSDIALOG_H
#define ADDADDRESSDIALOG_H

namespace Ui {
class AddAddressDialog;
}

class AddAddressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddAddressDialog(QWidget *parent = nullptr);
    ~AddAddressDialog();
    QStringList getAddresses();

private:
    Ui::AddAddressDialog *ui;
    QLineEdit *insertLineEdit;
    QPushButton *insertPushButton;
    QPushButton *sessionPushButton;
    QListWidget *addressListWidget;
    QLabel *selectCountLabel;
    CustomAddressListWidget *customAddressListWidget;

    void onInsertButtonClicked(bool checked);
    void onSessionButtonClicked(bool checked);
    void insertAddressToList(QString address);
    void onInsertEditReturnPressed();
    bool isAddressInList(QString address);

protected:
    void keyPressEvent(QKeyEvent *e) override;
};

#endif // ADDADDRESSDIALOG_H
