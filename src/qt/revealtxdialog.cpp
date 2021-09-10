#include "revealtxdialog.h"
#include "ui_revealtxdialog.h"
#include "bitcoinunits.h"

#include <QClipboard>

RevealTxDialog::RevealTxDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new Ui::RevealTxDialog)
{
    ui->setupUi(this);

    ui->pushButtonCopyID->setStyleSheet("background:transparent;");
    ui->pushButtonCopyID->setIcon(QIcon(":/icons/editcopy"));
    connect(ui->pushButtonCopyID, SIGNAL(clicked()), this, SLOT(copyID()));

    ui->pushButtonCopyAddr->setStyleSheet("background:transparent;");
    ui->pushButtonCopyAddr->setIcon(QIcon(":/icons/editcopy"));
    connect(ui->pushButtonCopyAddr, SIGNAL(clicked()), this, SLOT(copyAddress()));

    ui->pushButtonCopyPrivKey->setStyleSheet("background:transparent;");
    ui->pushButtonCopyPrivKey->setIcon(QIcon(":/icons/editcopy"));
    connect(ui->pushButtonCopyPrivKey, SIGNAL(clicked()), this, SLOT(copyPrivateKey()));

    ui->pushButtonCopyTxAmount->setStyleSheet("background:transparent;");
    ui->pushButtonCopyTxAmount->setIcon(QIcon(":/icons/editcopy"));
    connect(ui->pushButtonCopyTxAmount, SIGNAL(clicked()), this, SLOT(copyTxAmount()));

    ui->pushButtonCopyTxFee->setStyleSheet("background:transparent;");
    ui->pushButtonCopyTxFee->setIcon(QIcon(":/icons/editcopy"));
    connect(ui->pushButtonCopyTxFee, SIGNAL(clicked()), this, SLOT(copyTxFee()));
}

RevealTxDialog::~RevealTxDialog()
{
    delete ui;
}

void RevealTxDialog::setTxID(QString strId) 
{
    ui->lblTxID->setText(strId);
}

void RevealTxDialog::setTxAddress(QString strAddr) 
{
    ui->lblAddress->setText(strAddr);
}

void RevealTxDialog::setTxPrivKey(QString strPrivKey) 
{
    ui->lblPrivateKey->setText(strPrivKey);
}

void RevealTxDialog::setTxAmount(QString amount)
{
    int nDisplayUnit;
    ui->lblTxAmount->setText(amount.append(" PRCY"));
}

void RevealTxDialog::setTxFee(CAmount fee)
{
    int nDisplayUnit;
    ui->lblTxFee->setText(BitcoinUnits::formatHtmlWithUnit(0, fee, false, BitcoinUnits::separatorAlways));
}

void RevealTxDialog::on_buttonBox_accepted()
{

}

void RevealTxDialog::copyID(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->lblTxID->text());
}

void RevealTxDialog::copyAddress(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->lblAddress->text());
}

void RevealTxDialog::copyPrivateKey(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->lblPrivateKey->text());
}

void RevealTxDialog::copyTxAmount(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->lblTxAmount->text());
}

void RevealTxDialog::copyTxFee(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->lblTxFee->text());
}
