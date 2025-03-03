// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_WALLETFRAME_H
#define BITCOIN_QT_WALLETFRAME_H

#include "askpassphrasedialog.h"

#include <QFrame>
#include <QMap>
#include <QSettings>

class BitcoinGUI;
class ClientModel;
class SendCoinsRecipient;
class WalletModel;
class WalletView;
class BlockExplorer;

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE

class WalletFrame : public QFrame
{
    Q_OBJECT

public:
    explicit WalletFrame(BitcoinGUI* _gui = 0);
    ~WalletFrame();

    void setClientModel(ClientModel* clientModel);
    bool addWallet(const QString& name, WalletModel* walletModel);
    bool setCurrentWallet(const QString& name);
    bool removeWallet(const QString& name);
    void removeAllWallets();
    void showSyncStatus(bool fShow);

private:
    QStackedWidget* walletStack;
    BitcoinGUI* gui;
    ClientModel* clientModel;
    QMap<QString, WalletView*> mapWalletViews;
    bool bOutOfSync;
    WalletView* currentWalletView();

public Q_SLOTS:
    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to masternode page */
    void gotoMasternodePage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to options page */
    void gotoOptionsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "");
    /** Switch to explorer page */
    void gotoBlockExplorerPage();
    /** Show MultiSend Dialog **/
    void gotoMultiSendDialog();
    /** Encrypt the wallet */
    void encryptWallet(bool status);
    /** Backup the wallet */
    void backupWallet();
    /** Show 24 word seed phrase */
    void showSeedPhrase();
    /** Change encrypted wallet passphrase */
    void changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */
    void unlockWallet(AskPassphraseDialog::Context context);
    void unlockWallet(bool setContext);
    /** Lock wallet */
    void lockWallet();
    /** Toggle Wallet Lock State */
    void toggleLockWallet();

    /** Show used sending addresses */
    void usedSendingAddresses();
    /** Show used receiving addresses */
    void usedReceivingAddresses();
};

#endif // BITCOIN_QT_WALLETFRAME_H
