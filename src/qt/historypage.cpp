// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "historypage.h"
#include "ui_historypage.h"

#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "bitcoingui.h"
#include "bitcoinunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "transactionrecord.h"
#include "walletmodel.h"
#include "revealtxdialog.h"

#include <algorithm>

#include <QAction>
#include <QBrush>
#include <QCalendarWidget>
#include <QCursor>
#include <QItemSelection>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QTextDocument>
#include <QTime>
#include <QDate>
#include <QTextStream>
#include <QProcess>

HistoryPage::HistoryPage(QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
                                            ui(new Ui::HistoryPage),
                                            // m_SizeGrip(this),
                                            model(0)

{
    ui->setupUi(this);

    initWidgets();
    connectWidgets();
    updateTableData(pwalletMain);
    updateAddressBookData(pwalletMain);
}


HistoryPage::~HistoryPage()
{
    delete ui;
}
void HistoryPage::initWidgets()
{
    //set String for all addresses
    allAddressString = "All addresses...";
    //adjust qt paint flags
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableView->setAttribute(Qt::WA_TranslucentBackground, true);
    connect(ui->tableView, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(on_cellClicked(int, int)));

    //set date formats and init date from current timestamp
    ui->dateTimeEditTo->setDisplayFormat("M/d/yy");
    ui->dateTimeEditFrom->setDisplayFormat("M/d/yy");
    ui->dateTimeEditTo->setDateTime(QDateTime::currentDateTime().addDays(1));
    ui->dateTimeEditFrom->setDateTime(QDateTime::currentDateTime().addDays(-7));
    //add QTimeEdit's to QCalendar popups
    timeEditTo = new QTimeEdit(ui->dateTimeEditTo);
    timeEditFrom = new QTimeEdit(ui->dateTimeEditFrom);
    ui->dateTimeEditTo->calendarWidget()->parentWidget()->layout()->addWidget(timeEditTo);
    ui->dateTimeEditFrom->calendarWidget()->parentWidget()->layout()->addWidget(timeEditFrom);
    //color calendarwidgets
    GUIUtil::colorCalendarWidgetWeekends(ui->dateTimeEditTo->calendarWidget(), QColor("gray"));
    GUIUtil::colorCalendarWidgetWeekends(ui->dateTimeEditFrom->calendarWidget(), QColor("gray"));
    ui->horizontalLayout_2->setAlignment(Qt::AlignTop);
}

void HistoryPage::connectWidgets() //add functions to widget signals
{
    connect(ui->dateTimeEditTo, SIGNAL(dateChanged(const QDate&)), this, SLOT(updateFilter()));
    connect(ui->dateTimeEditFrom, SIGNAL(dateChanged(const QDate&)), this, SLOT(updateFilter()));
    connect(ui->comboBoxType, SIGNAL(currentIndexChanged(const int&)), this, SLOT(updateFilter()));
    //
    connect(ui->lineEditDesc, SIGNAL(currentIndexChanged(const int&)), this, SLOT(updateFilter()));
    connect(ui->lineEditDesc->lineEdit(), SIGNAL(textChanged(const QString&)), this, SLOT(updateFilter()));

    //
    connect(ui->lineEditAmount, SIGNAL(textChanged(const QString&)), this, SLOT(updateFilter()));
    //
    connect(timeEditFrom, SIGNAL(timeChanged(const QTime&)), this, SLOT(updateFilter()));
    connect(timeEditTo, SIGNAL(timeChanged(const QTime&)), this, SLOT(updateFilter()));
}

void HistoryPage::on_cellClicked(int row, int column) 
{
    if (pwalletMain->IsLocked()) return;
    //1 is column index for type
    QTableWidgetItem* cell = ui->tableView->item(row, 1);
    QString type = cell->data(0).toString();
    std::string stdType = type.trimmed().toStdString();
    //2 is column index for address
    cell = ui->tableView->item(row, 2);
    QString address = cell->data(0).toString();
    //3 is column index for amount
    cell = ui->tableView->item(row, 3);
    QString amount = cell->data(0).toString();
    std::string stdAddress = address.trimmed().toStdString();
    if (pwalletMain->addrToTxHashMap.count(stdAddress) == 1) {
        RevealTxDialog txdlg;
        txdlg.setStyleSheet(GUIUtil::loadStyleSheet());

        txdlg.setTxID(pwalletMain->addrToTxHashMap[stdAddress].c_str());

        txdlg.setTxAddress(stdAddress.c_str());
        bool privkeyFound = false;
        std::string txHash = pwalletMain->addrToTxHashMap[stdAddress];
        if (IsHex(txHash)) {
            uint256 hash;
            hash.SetHex(txHash);

            if (pwalletMain && pwalletMain->mapWallet.count(hash) == 1) {
                CWalletTx tx = pwalletMain->mapWallet[hash];
                for (size_t i = 0; i < tx.vout.size(); i++) {
                    txnouttype type;
                    std::vector<CTxDestination> addresses;
                    int nRequired;

                    if (ExtractDestinations(tx.vout[i].scriptPubKey, type, addresses, nRequired)) {
                        std::string parseddAddress = CBitcoinAddress(addresses[0]).ToString();
                        if (stdAddress == parseddAddress) {
                            if (tx.IsCoinStake() && !tx.vout[i].txPriv.empty()) {
                                CKey txPriv;
                                txPriv.Set(tx.vout[i].txPriv.begin(), tx.vout[i].txPriv.end(), true);
                                txdlg.setTxPrivKey(CBitcoinSecret(txPriv).ToString().c_str());
                                privkeyFound = true;
                            } else {
                                std::string key = txHash + std::to_string(i);
                                std::string secret;
                                if (CWalletDB(pwalletMain->strWalletFile).ReadTxPrivateKey(key, secret)) {
                                    txdlg.setTxPrivKey(secret.c_str());
                                    privkeyFound = true;
                                }
                            }
                        }
                    }
                }
                txdlg.setTxAmount(amount);
                txdlg.setTxFee(tx.nTxFee);
            }
        }
        std::string txdlgMsg = "Request from Sender (if applicable)";
        if (stdType == "Minted") {
            privkeyFound = false;
            txdlgMsg = "Minted transactions do not have a PrivKey";
        }    
        if (pwalletMain->IsLocked()) {
            privkeyFound = false;
            txdlgMsg = "Wallet must be unlocked to view PrivKey";
        }
        if (!privkeyFound) txdlg.setTxPrivKey(std::string(txdlgMsg).c_str());
        
        txdlg.exec();
    }
}

void HistoryPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    ui->tableView->setColumnWidth(2, this->width() * .65);
    ui->tableView->resizeColumnToContents(QHeaderView::ResizeToContents);
    ui->tableView->resizeColumnsToContents();
}

void HistoryPage::keyPressEvent(QKeyEvent* event)
{
    this->QDialog::keyPressEvent(event);
}

void HistoryPage::updateTableData()
{
    if (pwalletMain) {
        updateTableData(pwalletMain);
    }
}

void HistoryPage::updateTableData(CWallet* wallet)
{
    TRY_LOCK(cs_main, lockMain);
    if (!lockMain)
        return;
    TRY_LOCK(pwalletMain->cs_wallet, lockWallet);
    if (!lockWallet)
        return;
    {
        if (!wallet || wallet->IsLocked()) return;
        ui->tableView->setSortingEnabled(false);
        while (ui->tableView->rowCount() > 0)
        {
            ui->tableView->removeRow(0);
        }
        ui->tableView->setRowCount(0);
        std::vector<std::map<QString, QString> > txs;
        if (wallet->IsLocked()) {
            {
                LOCK(pwalletMain->cs_wallet);
                std::vector<std::map<QString, QString>> txs;// = WalletUtil::getTXs(pwalletMain);

                std::map<uint256, CWalletTx> txMap = pwalletMain->mapWallet;
                std::vector<CWalletTx> latestTxes;
                for (std::map<uint256, CWalletTx>::iterator tx = txMap.begin(); tx != txMap.end(); ++tx) {
                    if (tx->second.GetDepthInMainChain() > 0) {
                        latestTxes.push_back(tx->second);
                    }
                }

                for (int i = 0; i < (int)latestTxes.size(); i++) {
                    txs.push_back(WalletUtil::getTx(pwalletMain, latestTxes[i]));
                }
            }
        } else {
            txs = WalletUtil::getTXs(wallet);
        }
        for (int row = 0; row < (short)txs.size(); row++) {
            ui->tableView->insertRow(row);
            int col = 0;
            for (QString dataName : {"date", "type", "address", "amount", "confirmations"}) {
                QString data = txs[row].at(dataName);
                QDateTime date;
                QTableWidgetItem* cell = new QTableWidgetItem();
                switch (col) {
                case 0: /*date*/
                    date = QDateTime::fromString(data, "MM/dd/yy hh:mm:ss").addYears(100);
                    cell->setData(0, date);
                    break;
                case 3: /*amount*/
                    if (wallet->IsLocked()) {
                        cell->setData(0, QString("Locked; Hidden"));
                    } else {
                        cell->setData(0, data);
                    }
                    break;
                case 4: /*confirmations*/
                    cell->setData(0, data.toInt());
                    break;
                default:
                    cell->setData(0, data);
                    break;
                }
                ui->tableView->setItem(row, col, cell);
                cell->setTextAlignment(Qt::AlignCenter);
                col++;
            }
        }
        ui->tableView->setVisible(ui->tableView->rowCount());
        ui->tableView->sortByColumn(4, Qt::AscendingOrder);
        ui->tableView->setSortingEnabled(true);
    }
}

void HistoryPage::updateAddressBookData(CWallet* wallet)
{
    ui->lineEditDesc->clear();
    ui->lineEditDesc->addItem(allAddressString);
    QList<QString> addresses = WalletUtil::getAddressBookData(wallet);
    for (QString address : addresses)
        ui->lineEditDesc->addItem(address);
    ui->lineEditDesc->lineEdit()->setText(QString(""));
}

void HistoryPage::updateFilter()
{
    syncTime(ui->dateTimeEditFrom, timeEditFrom);
    syncTime(ui->dateTimeEditTo, timeEditTo);
    auto selectedAmount = ui->lineEditAmount->text().toFloat();
    QString selectedType = ui->comboBoxType->currentText();
    QList<QString> selectedAddresses = ui->lineEditDesc->lineEdit()->text().split(" | ");

    for (int row = 0; row < ui->tableView->rowCount(); row++) {
        bool hide = false;
        QDateTime date = QDateTime::fromString(ui->tableView->item(row, 0)->text(), "M/d/yyyy h:m");
        QString type = ui->tableView->item(row, 1)->text();
        QString address = ui->tableView->item(row, 2)->text();
        auto amount = ui->tableView->item(row, 3)->text().toFloat();

        if (
            (ui->dateTimeEditFrom->dateTime() > date) || (ui->dateTimeEditTo->dateTime() < date) || //record is not between selected dates

            (amount < selectedAmount) //record smaller than selected min amount
        )
            hide = true;
        if (selectedType != tr("All Types")) {
            if (selectedType == tr("Received")) {
                hide = !(type == tr("Received") || type == tr("Masternode Reward") || type == tr("Staking Reward") || type == ("PoA Reward"));
            } else
                hide = (selectedType != type) || hide;
        }
        if (ui->lineEditDesc->currentText() != allAddressString) {
            bool found = false;
            for (QString selectedAddress : selectedAddresses)
                if (address.contains(selectedAddress))
                    found = true;
            hide = !found || hide;
        }

        ui->tableView->setRowHidden(row, false);
    }
}

void HistoryPage::syncTime(QDateTimeEdit* calendar, QTimeEdit* clock)
{
    calendar->setTime(clock->time());
}

void HistoryPage::setModel(WalletModel* model)
{
    this->model = model;
    connect(model, SIGNAL(WalletUnlocked()), this,
                                             SLOT(updateTableData()));
}
