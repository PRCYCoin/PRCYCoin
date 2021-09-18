// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2018 The PIVX developers
// Copyright (c) 2018-2020 The DAPS Project developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sendcoinsentry.h"
#include "ui_sendcoinsentry.h"

#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"

#include <QApplication>
#include <QClipboard>
#include <QDoubleValidator>

SendCoinsEntry::SendCoinsEntry(QWidget* parent) : QStackedWidget(parent),
                                                  ui(new Ui::SendCoinsEntry),
                                                  model(0)
{
    ui->setupUi(this);

    setCurrentWidget(ui->SendCoins);


#ifdef Q_OS_MAC
    ui->payToLayout_s->setSpacing(4);
#endif

    // normal prcycoin address field
    GUIUtil::setupAddressWidget(ui->payTo, this);

    // Connect signals
    connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_is, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_s, SIGNAL(clicked()), this, SLOT(deleteClicked()));

    // Hide unused UI items
    ui->deleteButton->setVisible(false);
    ui->addressBookButton->setVisible(false);
    ui->addAsLabel->setVisible(false);
    ui->labelLabel->setVisible(false);

    QLocale lo(QLocale::C);
    lo.setNumberOptions(QLocale::RejectGroupSeparator);
    QDoubleValidator *dblVal = new QDoubleValidator(0, Params().MAX_MONEY, 8, ui->payAmount);
    dblVal->setNotation(QDoubleValidator::StandardNotation);
    dblVal->setLocale(lo);
    ui->payAmount->setValidator(dblVal);
}

SendCoinsEntry::~SendCoinsEntry()
{
    delete ui;
}

/*void SendCoinsEntry::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->payTo->setText(QApplication::clipboard()->text());
}*/

void SendCoinsEntry::on_addressBookButton_clicked()
{
    if (!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if (dlg.exec()) {
        ui->payTo->setText(dlg.getReturnValue());
        ui->payAmount->setFocus();
    }
}

/*void SendCoinsEntry::on_clearAllButton_clicked()
{
    ui->payTo->clear();
    ui->addAsLabel->clear();
    ui->payAmount->clear();
    ui->payTo->setStyleSheet(GUIUtil::loadStyleSheet());
    ui->payAmount->setStyleSheet(GUIUtil::loadStyleSheet());
}*/

void SendCoinsEntry::on_payTo_textChanged(const QString& address)
{
    updateLabel(address);
}

void SendCoinsEntry::setModel(WalletModel* model)
{
    this->model = model;

    if (model && model->getOptionsModel())
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    clear();
}

void SendCoinsEntry::clear()
{
    // clear UI elements for normal payment
    ui->payTo->clear();
    //ui->addAsLabel->clear();
    ui->payAmount->clear();
    // clear UI elements for insecure payment request
    ui->payTo_is->clear();
    ui->memoTextLabel_is->clear();
    ui->payAmount_is->clear();
    // clear UI elements for secure payment request
    ui->payTo_s->clear();
    ui->memoTextLabel_s->clear();
    ui->payAmount_s->clear();

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void SendCoinsEntry::deleteClicked()
{
    Q_EMIT removeEntry(this);
}

static inline int64_t roundint64(double d)
{
    return (int64_t)(d > 0 ? d + 0.5 : d - 0.5);
}

CAmount SendCoinsEntry::getValidatedAmount() {
    double dAmount = ui->payAmount->text().toDouble();
    if (dAmount < 0.0 || dAmount > Params().MAX_MONEY) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Invalid Amount");
        msgBox.setText("Invalid amount entered. Please enter an amount less than 2.1B PRCY.");
        msgBox.setStyleSheet(GUIUtil::loadStyleSheet());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }
    CAmount nAmount = roundint64(dAmount * COIN);
    return nAmount;
}

SendCoinsRecipient SendCoinsEntry::getValue()
{
    // Payment request
    if (recipient.paymentRequest.IsInitialized())
        return recipient;

    // Normal payment
    recipient.address = ui->payTo->text();
    //recipient.label = ui->addAsLabel->text();
    recipient.amount = getValidatedAmount();

    return recipient;
}

QWidget* SendCoinsEntry::setupTabChain(QWidget* prev)
{
    QWidget::setTabOrder(prev, ui->payTo);
    //QWidget::setTabOrder(ui->payTo, ui->addAsLabel);

    return ui->deleteButton;
}

void SendCoinsEntry::setValue(const SendCoinsRecipient& value)
{
    recipient = value;

    if (recipient.paymentRequest.IsInitialized()) // payment request
    {
        if (recipient.authenticatedMerchant.isEmpty()) // insecure
        {
            ui->payTo_is->setText(recipient.address);
            ui->memoTextLabel_is->setText(recipient.message);
            ui->payAmount_is->setValue(recipient.amount);
            ui->payAmount_is->setReadOnly(true);
            setCurrentWidget(ui->SendCoins_InsecurePaymentRequest);
        } else // secure
        {
            ui->payTo_s->setText(recipient.authenticatedMerchant);
            ui->memoTextLabel_s->setText(recipient.message);
            ui->payAmount_s->setValue(recipient.amount);
            ui->payAmount_s->setReadOnly(true);
            setCurrentWidget(ui->SendCoins_SecurePaymentRequest);
        }
    } else // normal payment
    {
        //ui->addAsLabel->clear();
        ui->payTo->setText(recipient.address); // this may set a label from addressbook
        //if (!recipient.label.isEmpty())        // if a label had been set from the addressbook, dont overwrite with an empty label
            //ui->addAsLabel->setText(recipient.label);
        ui->payAmount->setText(QString::number((double)(recipient.amount) / (double)COIN, 'f', 3));
    }
}

void SendCoinsEntry::setAddress(const QString& address)
{
    ui->payTo->setText(address);
    ui->payAmount->setFocus();
}

bool SendCoinsEntry::isClear()
{
    return ui->payTo->text().isEmpty() && ui->payTo_is->text().isEmpty() && ui->payTo_s->text().isEmpty();
}

void SendCoinsEntry::setFocus()
{
    ui->payTo->setFocus();
}

void SendCoinsEntry::updateDisplayUnit()
{
    if (model && model->getOptionsModel()) {
        // Update payAmount with the current unit
        ui->payAmount_is->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_s->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }
}

bool SendCoinsEntry::updateLabel(const QString& address)
{
    if (!model)
        return false;

    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if (!associatedLabel.isEmpty()) {
        //ui->addAsLabel->setText(associatedLabel);
        return true;
    }

    return false;
}

void SendCoinsEntry::errorAddress(bool valid){
    if (valid)
        ui->payTo->setStyleSheet(GUIUtil::loadStyleSheet());
    else ui->payTo->setStyleSheet("border-color: red;");
}

void SendCoinsEntry::errorAmount(bool valid){
    if (valid)
        ui->payAmount->setStyleSheet(GUIUtil::loadStyleSheet());
    else ui->payAmount->setStyleSheet("border-color: red;");
}
