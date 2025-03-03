// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_SENDCOINSENTRY_H
#define BITCOIN_QT_SENDCOINSENTRY_H

#include "walletmodel.h"

#include <QStackedWidget>
#include <QSpinBox>

class WalletModel;

namespace Ui
{
class SendCoinsEntry;
}

/**
 * A single entry in the dialog for sending bitcoins.
 * Stacked widget, with different UIs for payment requests
 * with a strong payee identity.
 */
class SendCoinsEntry : public QStackedWidget
{
    Q_OBJECT

public:
    explicit SendCoinsEntry(QWidget* parent = 0);
    ~SendCoinsEntry();

    void setModel(WalletModel* model);
    SendCoinsRecipient getValue();

    /** Return whether the entry is still empty and unedited */
    bool isClear();

    void setValue(const SendCoinsRecipient& value);
    void setAddress(const QString& address);
    void errorAddress(bool valid);
    void errorAmount(bool valid);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases
     *  (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget* setupTabChain(QWidget* prev);

    void setFocus();

public Q_SLOTS:
    void clear();

Q_SIGNALS:
    void removeEntry(SendCoinsEntry* entry);
    void payAmountChanged();
    void validateAmount(const QString& textAmount);

private Q_SLOTS:
    void deleteClicked();
    void on_payTo_textChanged(const QString& address);
    void on_addressBookButton_clicked();
    //void on_pasteButton_clicked();
    void on_clearAllButton_clicked();
    void on_useAllSpendableButton_clicked();
    void updateDisplayUnit();

private:
    SendCoinsRecipient recipient;
    Ui::SendCoinsEntry* ui;
    WalletModel* model;

    bool updateLabel(const QString& address);

    CAmount getValidatedAmount();
};

#endif // BITCOIN_QT_SENDCOINSENTRY_H
