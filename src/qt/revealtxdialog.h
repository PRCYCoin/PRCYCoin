#ifndef REVEALTXDIALOG_H
#define REVEALTXDIALOG_H

#include <QDialog>
#include "amount.h"

namespace Ui {
class RevealTxDialog;
}

class RevealTxDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RevealTxDialog(QWidget *parent = 0);
    ~RevealTxDialog();

    void setTxID(QString strId);
    void setTxAddress(QString strAddr);
    void setTxPrivKey(QString strPrivKey);
    void setTxAmount(QString amount);
    void setTxFee(CAmount fee);

private Q_SLOTS:
    void on_buttonBox_accepted();
    void copyID();
    void copyAddress();
    void copyPrivateKey();
    void copyTxAmount();
    void copyTxFee();

private:
    Ui::RevealTxDialog *ui;
};

#endif // REVEALTXDIALOG_H
