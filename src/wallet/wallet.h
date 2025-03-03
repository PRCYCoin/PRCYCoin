// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2018 The PIVX developers
// Copyright (c) 2018-2020 The DAPS Project developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_H
#define BITCOIN_WALLET_H

#include "amount.h"
#include "base58.h"
#include "consensus/tx_verify.h"
#include "crypter.h"
#include "kernel.h"
#include "key.h"
#include "keystore.h"
#include "main.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "guiinterface.h"
#include "util.h"
#include "validationinterface.h"
#include "wallet/wallet_ismine.h"
#include "wallet/walletdb.h"

#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>

extern CWallet* pwalletMain;

/**
 * Settings
 */
extern CFeeRate payTxFee;
extern CAmount maxTxFee;
extern unsigned int nTxConfirmTarget;
extern bool bSpendZeroConfChange;
extern bool bdisableSystemnotifications;
extern bool fSendFreeTransactions;
extern bool fPayAtLeastCustomFee;
extern int64_t nReserveBalance;
extern bool fTxDeleteEnabled;
extern bool fTxConflictDeleteEnabled;
extern int fDeleteInterval;
extern unsigned int fDeleteTransactionsAfterNBlocks;
extern unsigned int fKeepLastNTransactions;

//! -paytxfee default
static const CAmount DEFAULT_TRANSACTION_FEE = 0.1 * COIN;//
//! -paytxfee will warn if called with a higher fee than this amount (in satoshis) per KB
static const CAmount nHighTransactionFeeWarning = 0.1 * COIN;
//! -maxtxfee default
static const CAmount DEFAULT_TRANSACTION_MAXFEE = 1 * COIN;
//! -maxtxfee will warn if called with a higher fee than this amount (in satoshis)
static const CAmount nHighTransactionMaxFeeWarning = 100 * nHighTransactionFeeWarning;
//! Largest (in bytes) free transaction we're willing to create
static const unsigned int MAX_FREE_TRANSACTION_CREATE_SIZE = 1000;
//! -custombackupthreshold default
static const int DEFAULT_CUSTOMBACKUPTHRESHOLD = 1;

//Default Transaction Retention N-BLOCKS
static const int DEFAULT_TX_DELETE_INTERVAL = 10000;

//Default Transaction Retention N-BLOCKS
static const unsigned int DEFAULT_TX_RETENTION_BLOCKS = 10000;

//Default Retenion Last N-Transactions
static const unsigned int DEFAULT_TX_RETENTION_LASTTX = 200;

//Amount of transactions to delete per run while syncing
static const int MAX_DELETE_TX_SIZE = 50000;

// 6666 = 1*5000 + 1*1000 + 1*500 + 1*100 + 1*50 + 1*10 + 1*5 + 1
static const int ZQ_6666 = 6666;

class CAccountingEntry;
class CCoinControl;
class COutput;
class CReserveKey;
class CScript;
class CWalletTx;

/** (client) version numbers for particular wallet features */
enum WalletFeature {
    FEATURE_BASE = 10500, // the earliest version new wallets supports (only useful for getinfo's clientversion output)

    FEATURE_WALLETCRYPT = 40000, // wallet encryption
    FEATURE_COMPRPUBKEY = 60000, // compressed public keys

    FEATURE_LATEST = 61000
};

enum AvailableCoinsType {
    ALL_COINS = 1,
    ONLY_5000 = 5,                        // find masternode outputs including locked ones (use with caution)
    STAKABLE_COINS = 6                          // UTXO's that are valid for staking
};

struct CompactTallyItem {
    CBitcoinAddress address;
    CAmount nAmount;
    std::vector<CTxIn> vecTxIn;
    CompactTallyItem()
    {
        nAmount = 0;
    }
};

/** A key pool entry */
class CKeyPool
{
public:
    int64_t nTime;
    CPubKey vchPubKey;

    CKeyPool();
    CKeyPool(const CPubKey& vchPubKeyIn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(nTime);
        READWRITE(vchPubKey);
    }
};

/** Address book data */
class CAddressBookData
{
public:
    std::string name;
    std::string purpose;

    CAddressBookData()
    {
        purpose = "unknown";
    }

    typedef std::map<std::string, std::string> StringMap;
    StringMap destdata;
};

/**
 * Account information.
 * Stored in wallet with key "acc"+string account name.
 */
class CAccount
{
public:
    CPubKey vchPubKey;
    uint32_t nAccountIndex;

    CAccount()
    {
        SetNull();
    }

    void SetNull()
    {
        vchPubKey = CPubKey();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vchPubKey);
        READWRITE(nAccountIndex);
    }
};

class CStealthAccount
{
public:
    CAccount spendAccount;
    CAccount viewAccount;

    CStealthAccount()
    {
        SetNull();
    }

    void SetNull()
    {
        spendAccount.SetNull();
        viewAccount.SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(spendAccount);
        READWRITE(viewAccount);
    }
};

//in any case consolidation needed, call estimateConsolidationFees function to estimate fees
enum StakingStatusError
{
    STAKING_OK, //use case B, C, D, no consolidation needed, 
    UNSTAKABLE_BALANCE_TOO_LOW, //coin is not mature yet (balance > 2.5k)
    UNSTAKABLE_BALANCE_TOO_LOW_CONSOLIDATION_FAILED, //coin is not mature yet (balance > 2.5k)
    UNSTAKABLE_BALANCE_RESERVE_TOO_HIGH,
    UNSTAKABLE_BALANCE_RESERVE_TOO_HIGH_CONSOLIDATION_FAILED, //even consolidation does not help
    STAKABLE_NEED_CONSOLIDATION,   //stable and consolidation, needs to estimate fees
    STAKABLE_NEED_CONSOLIDATION_WITH_RESERVE_BALANCE  //stable and consolidation, needs to estimate fees
};

enum CombineMode {
    OFF,
    ON,
};

/**
 * A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
 * and provides the ability to create new transactions.
 */
class CWallet : public CCryptoKeyStore, public CValidationInterface
{
private:
    bool SelectCoins(bool needFee, CAmount& estimatedFee, int ringSize, int numOut, const CAmount& nTargetValue, std::set<std::pair<const CWalletTx*, unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl* coinControl = NULL, AvailableCoinsType coin_type = ALL_COINS, bool useIX = true) ;
    //it was public bool SelectCoins(int64_t nTargetValue, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet, const CCoinControl *coinControl = NULL, AvailableCoinsType coin_type=ALL_COINS, bool useIX = true) const;

    CWalletDB* pwalletdbEncryption;
    //! keeps track of whether Unlock has run a thorough check before
    bool fDecryptionThoroughlyChecked{false};

    //! the current wallet version: clients below this version are not able to load the wallet
    int nWalletVersion;

    //! the maximum wallet format version: memory-only variable that specifies to what version this wallet may be upgraded
    int nWalletMaxVersion;

    int64_t nNextResend;
    int64_t nLastResend;

    /**
     * Used to keep track of spent outpoints, and
     * detect and report conflicts (double-spends or
     * mutated transactions where the mutant gets mined).
     */
    typedef std::multimap<COutPoint, uint256> TxSpends;
    TxSpends mapTxSpends;
    void AddToSpends(const COutPoint& outpoint, const uint256& wtxid);
    void AddToSpends(const uint256& wtxid);

    void SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator>);

public:
    static const int32_t MAX_DECOY_POOL = 500;
    static const int32_t PROBABILITY_NEW_COIN_SELECTED = 70;
    bool RescanAfterUnlock(int fromHeight);
    bool MintableCoins();
    StakingStatusError StakingCoinStatus(CAmount& minFee, CAmount& maxFee);
    bool SelectStakeCoins(std::list<std::unique_ptr<CStakeInput> >& listInputs, CAmount nTargetAmount);
    bool IsMasternodeController();
    bool checkPassPhraseRule(const char *pass);
    COutPoint findMyOutPoint(const CTxIn& txin) const;
    static int ComputeTxSize(size_t numIn, size_t numOut, size_t ringSize);
    void resetPendingOutPoints();
    bool estimateStakingConsolidationFees(CAmount& min, CAmount& max);
    static int MaxTxSizePerTx();
    std::string GetTransactionType(const CTransaction& tx);
    bool WriteAutoConsolidateSettingTime(uint32_t settingTime);
    uint32_t ReadAutoConsolidateSettingTime();
    bool IsAutoConsolidateOn();
    std::string GetUniqueWalletBackupName() const;
    /*
     * Main wallet lock.
     * This lock protects all the fields added by CWallet
     *   except for:
     *      fFileBacked (immutable after instantiation)
     *      strWalletFile (immutable after instantiation)
     */
    mutable RecursiveMutex cs_wallet;

    bool fFileBacked;
    bool fWalletUnlockStakingOnly;
    std::string strWalletFile;

    std::set<int64_t> setKeyPool;
    std::map<CKeyID, CKeyMetadata> mapKeyMetadata;

    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID;

    // Stake Settings
    unsigned int nHashDrift;
    unsigned int nHashInterval;
    uint64_t nStakeSplitThreshold;
    int nStakeSetUpdateTime;
    int walletUnlockCountStatus = 0;

    //MultiSend
    std::vector<std::pair<std::string, int> > vMultiSend;
    bool fMultiSendStake;
    bool fMultiSendMasternodeReward;
    bool fMultiSendNotify;
    std::string strMultiSendChangeAddress;
    int nLastMultiSendHeight;
    std::vector<std::string> vDisabledAddresses;

    //Auto Combine Inputs
    bool fCombineDust;
    CAmount nAutoCombineThreshold;
    CAmount nAutoCombineTarget;
    bool CreateSweepingTransaction(CAmount target, CAmount threshold, uint32_t nTimeBefore);
    bool SendAll(std::string des, CWalletTx& wtxNew, bool inclLocked);

    CWallet();
    CWallet(std::string strWalletFileIn);
    ~CWallet();
    void SetNull();
    bool isMultiSendEnabled();
    void setMultiSendDisabled();

    mutable std::map<uint256, CWalletTx> mapWallet;
    std::list<CAccountingEntry> laccentries;

    typedef std::pair<CWalletTx*, CAccountingEntry*> TxPair;
    typedef std::multimap<int64_t, TxPair > TxItems;
    TxItems wtxOrdered;

    int64_t nOrderPosNext;
    std::map<uint256, int> mapRequestCount;

    std::map<CTxDestination, CAddressBookData> mapAddressBook;
    std::map<std::string, std::string> addrToTxHashMap;
    std::vector<CKey> txPrivKeys;	//only for temporary storing tx private keys for user transactions, dont care staking transactions

    CPubKey vchDefaultKey;

    std::set<COutPoint> setLockedCoins;
    std::map<CKeyID, CHDPubKey> mapHdPubKeys; //<! memory map of HD extended pubkeys

    int64_t nTimeFirstKey;

    CombineMode combineMode = OFF;
    int64_t DecoyConfirmationMinimum = 15;

    mutable std::map<std::string, CKeyImage> outpointToKeyImages;
    std::map<std::string, bool> keyImagesSpends;
    std::map<std::string, std::string> keyImageMap;//mapping from: txhashHex-n to key image str, n = index
    std::list<std::string> pendingKeyImages;
    std::map<COutPoint, bool> inSpendQueueOutpoints;
    std::vector<COutPoint> inSpendQueueOutpointsPerSession;
    mutable std::map<CScript, CAmount> amountMap;
    mutable std::map<CScript, CKey> blindMap;
    mutable std::map<COutPoint, uint256> userDecoysPool;	//used in transaction spending user transaction
    mutable std::map<COutPoint, uint256> coinbaseDecoysPool; //used in transction spending coinbase

    CAmount dirtyCachedBalance = 0;

    const CWalletTx* GetWalletTx(const uint256& hash) const;

    std::vector<CWalletTx> getWalletTxs();

    //! check whether we are allowed to upgrade (or already support) to the named feature
    bool CanSupportFeature(enum WalletFeature wf);

    bool generateKeyImage(const CPubKey& pub, CKeyImage& img) const;
    bool generateKeyImage(const CScript& scriptKey, CKeyImage& img) const;

    bool AvailableCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed = true, const CCoinControl* coinControl = NULL, bool fIncludeZeroValue = false, AvailableCoinsType nCoinType = ALL_COINS, bool fUseIX = false);
    std::map<CBitcoinAddress, std::vector<COutput> > AvailableCoinsByAddress(bool fConfirmed = true, CAmount maxCoinValue = 0);
    bool SelectCoinsMinConf(bool needFee, CAmount& estimatedFee, int ringSize, int numOut, const CAmount& nTargetValue, int nConfMine, int nConfTheirs, std::vector<COutput> vCoins, std::set<std::pair<const CWalletTx*, unsigned int> >& setCoinsRet, CAmount& nValueRet);

    /// Get 5000 PRCY output and keys which can be used for the Masternode
    bool GetMasternodeVinAndKeys(CTxIn& txinRet, CPubKey& pubKeyRet,
            CKey& keyRet, std::string strTxHash, std::string strOutputIndex, std::string& strError);
    /// Extract txin information and keys from output
    bool GetVinAndKeysFromOutput(COutput out, CTxIn& txinRet, CPubKey& pubKeyRet, CKey& keyRet);

    bool IsSpent(const uint256& hash, unsigned int n);
    unsigned int GetSpendDepth(const uint256& hash, unsigned int n) const;

    bool IsLockedCoin(uint256 hash, unsigned int n) const;
    void LockCoin(COutPoint& output);
    void UnlockCoin(COutPoint& output);
    void UnlockAllCoins();
    void ListLockedCoins(std::vector<COutPoint>& vOutpts);

    //  keystore implementation
    // Generate a new key
    CPubKey GenerateNewKey();
    void DeriveNewChildKey(uint32_t nAccountIndex, CKey& secretRet);
    void GenerateNewHDChain(std::string* phrase = NULL);
     /* Set the HD chain model (chain child index counters) */
    bool SetHDChain(const CHDChain& chain, bool memonly);
    bool SetCryptedHDChain(const CHDChain& chain, bool memonly);
    bool GetDecryptedHDChain(CHDChain& hdChainRet);
    bool IsHDEnabled();
    bool GetSeedPhrase(std::string &phrase);
    bool HaveKey(const CKeyID &address) const;
    //! GetPubKey implementation that also checks the mapHdPubKeys
    bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const;
    //! GetKey implementation that can derive a HD private key on the fly
    bool GetKey(const CKeyID &address, CKey& keyOut) const;
    //! Adds a HDPubKey into the wallet(database)
    bool AddHDPubKey(const CExtPubKey &extPubKey, bool fInternal, uint32_t nAccountIndex);
    //! loads a HDPubKey into the wallets memory
    bool LoadHDPubKey(const CHDPubKey &hdPubKey);
    //! Adds a key to the store, and saves it to disk.
    bool AddKeyPubKey(const CKey& key, const CPubKey& pubkey);
    //! Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKey(const CKey& key, const CPubKey& pubkey) { return CCryptoKeyStore::AddKeyPubKey(key, pubkey); }
    //! Load metadata (used by LoadWallet)
    bool LoadKeyMetadata(const CPubKey& pubkey, const CKeyMetadata& metadata);

    bool LoadMinVersion(int nVersion);

    //! Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret);
    //! Adds an encrypted key to the store, without saving it to disk (used by LoadWallet)
    bool LoadCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret);
    bool AddCScript(const CScript& redeemScript);
    bool LoadCScript(const CScript& redeemScript);

    //! Adds a destination data tuple to the store, and saves it to disk
    bool AddDestData(const CTxDestination& dest, const std::string& key, const std::string& value);
    //! Erases a destination data tuple in the store and on disk
    bool EraseDestData(const CTxDestination& dest, const std::string& key);
    //! Adds a destination data tuple to the store, without saving it to disk
    bool LoadDestData(const CTxDestination& dest, const std::string& key, const std::string& value);

    //! Adds a watch-only address to the store, and saves it to disk.
    bool AddWatchOnly(const CScript& dest);
    bool RemoveWatchOnly(const CScript& dest);
    //! Adds a watch-only address to the store, without saving it to disk (used by LoadWallet)
    bool LoadWatchOnly(const CScript& dest);

    //! Lock Wallet
    bool Lock();
    bool Unlock(const SecureString& strWalletPassphrase, bool anonimizeOnly = false);
    bool Unlock(const CKeyingMaterial& vMasterKeyIn);
    bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);
    bool EncryptWallet(const SecureString& strWalletPassphrase);

    void GetKeyBirthTimes(std::map<CKeyID, int64_t>& mapKeyBirth) const;
    unsigned int ComputeTimeSmart(const CWalletTx& wtx) const;

    /**
     * Increment the next transaction order id
     * @return next transaction order id
     */
    int64_t IncOrderPosNext(CWalletDB* pwalletdb = NULL);


    void MarkDirty();
    bool AddToWallet(const CWalletTx& wtxIn, bool fFromLoadWallet, CWalletDB* pwalletdb);
    void SyncTransaction(const CTransaction& tx, const CBlock* pblock);
    bool AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlock* pblock, bool fUpdate);
    void EraseFromWallet(const uint256& hash);
    void ReorderWalletTransactions(std::map<std::pair<int,int>, CWalletTx*> &mapSorted, int64_t &maxOrderPos);
    void UpdateWalletTransactionOrder(std::map<std::pair<int,int>, CWalletTx*> &mapSorted, bool resetOrder);
    void DeleteTransactions(std::vector<uint256> &removeTxs);
    void DeleteWalletTransactions(const CBlockIndex* pindex);
    int ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate = false, bool fromStartup = false, int height = -1);
    void ReacceptWalletTransactions();
    void ResendWalletTransactions();
    CAmount GetBalance();
    CAmount GetSpendableBalance();
    CAmount GetLockedCoins() const;
    CAmount GetUnlockedCoins() const;
    CAmount GetUnconfirmedBalance() const;
    CAmount GetImmatureBalance() const;
    CAmount GetWatchOnlyBalance() const;
    CAmount GetUnconfirmedWatchOnlyBalance() const;
    CAmount GetImmatureWatchOnlyBalance() const;
    bool CreateTransaction(CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet, std::string& strFailReason, const CCoinControl* coinControl);
    bool CreateTransaction(const std::vector<std::pair<CScript, CAmount> >& vecSend,
                           CWalletTx& wtxNew,
                           CReserveKey& reservekey,
                           CAmount& nFeeRet,
                           std::string& strFailReason,
                           const CCoinControl* coinControl = NULL,
                           AvailableCoinsType coin_type = ALL_COINS,
                           bool useIX = false,
                           CAmount nFeePay = 0);
    bool CreateTransactionBulletProof(const CKey& txPrivDes,
                           const CPubKey& recipientViewKey,
                           const std::vector<std::pair<CScript, CAmount> >& vecSend,
                           CWalletTx& wtxNew,
                           CReserveKey& reservekey,
                           CAmount& nFeeRet,
                           std::string& strFailReason,
                           const CCoinControl* coinControl = NULL,
                           AvailableCoinsType coin_type = ALL_COINS,
                           bool useIX = false,
                           CAmount nFeePay = 0, int ringSize = 6, bool tomyself = false);

    bool CreateTransactionBulletProof(const CKey& txPrivDes, const CPubKey &recipientViewKey, CScript scriptPubKey, const CAmount &nValue,
                                      CWalletTx &wtxNew, CReserveKey &reservekey, CAmount &nFeeRet,
                                      std::string &strFailReason, const CCoinControl *coinControl = NULL,
                                      AvailableCoinsType coin_type = ALL_COINS, bool useIX = false,
                                      CAmount nFeePay = 0, int ringSize = 6, bool tomyself = false);

    int ComputeFee(size_t numIn, size_t numOut, size_t ringSize);
    CAmount ComputeReserveUTXOAmount();
    bool CreateTransaction(CScript scriptPubKey, const CAmount &nValue, CWalletTx &wtxNew, CReserveKey &reservekey,
                           CAmount &nFeeRet, std::string &strFailReason, const CCoinControl *coinControl = NULL,
                           AvailableCoinsType coin_type = ALL_COINS, bool useIX = false, CAmount nFeePay = 0);
    bool CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey, std::string strCommand = NetMsgType::TX);
    bool AddAccountingEntry(const CAccountingEntry&, CWalletDB & pwalletdb);
    bool CreateCoinStake(const CKeyStore& keystore, unsigned int nBits, int64_t nSearchInterval, CMutableTransaction& txNew, unsigned int& nTxNewTime);
    bool MultiSend();
    void AutoCombineDust();

    static CFeeRate minTxFee;
    static CAmount GetMinimumFee(unsigned int nTxBytes, unsigned int nConfirmTarget, const CTxMemPool& pool);

    bool NewKeyPool();
    bool TopUpKeyPool(unsigned int kpSize = 0);
    void ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool);
    void KeepKey(int64_t nIndex);
    void ReturnKey(int64_t nIndex);
    bool GetKeyFromPool(CPubKey& key);
    int64_t GetOldestKeyPoolTime();
    void GetAllReserveKeys(std::set<CKeyID>& setAddress) const;

    std::set<std::set<CTxDestination> > GetAddressGroupings();
    std::map<CTxDestination, CAmount> GetAddressBalances();

    std::set<CTxDestination> GetAccountAddresses(std::string strAccount) const;

    bool GetBudgetSystemCollateralTX(CTransaction& tx, uint256 hash, bool useIX);
    bool GetBudgetSystemCollateralTX(CWalletTx& tx, uint256 hash, bool useIX);

    isminetype IsMine(const CTxIn& txin) const;
    CAmount GetDebit(const CTxIn& txin, const isminefilter& filter) const;
    isminetype IsMine(const CTxOut& txout) const;
    CAmount GetCredit(const CTransaction& tx, const CTxOut& txout, const isminefilter& filter) const;
    bool IsChange(const CTxOut& txout) const;
    CAmount GetChange(const CTransaction& tx, const CTxOut& txout) const;
    bool IsMine(const CTransaction& tx) const;
    /** should probably be renamed to IsRelevantToMe */
    bool IsFromMe(const CTransaction& tx) const;
    CAmount GetDebit(const CTransaction& tx, const isminefilter& filter) const;
    CAmount GetCredit(const CTransaction& tx, const isminefilter& filter) const;
    CAmount GetChange(const CTransaction& tx) const;
    void SetBestChain(const CBlockLocator& loc);

    DBErrors LoadWallet(bool& fFirstRunRet);
    DBErrors ZapWalletTx(std::vector<CWalletTx>& vWtx);

    bool SetAddressBook(const CTxDestination& address, const std::string& strName, const std::string& purpose);

    bool DelAddressBook(const CTxDestination& address);

    bool UpdatedTransaction(const uint256& hashTx);

    void Inventory(const uint256& hash);

    unsigned int GetKeyPoolSize();

    bool SetDefaultKey(const CPubKey& vchPubKey);

    //! signify that a particular wallet feature is now used. this may change nWalletVersion and nWalletMaxVersion if those are lower
    bool SetMinVersion(enum WalletFeature, CWalletDB* pwalletdbIn = NULL, bool fExplicit = false);

    //! change which version we're allowed to upgrade to (note that this does not immediately imply upgrading to that format)
    bool SetMaxVersion(int nVersion);

    //! get the current wallet format (the oldest client version guaranteed to understand this wallet)
    int GetVersion();

    //! Get wallet transactions that conflict with given transaction (spend same outputs)
    std::set<uint256> GetConflicts(const uint256& txid) const;

    /**
     * Address book entry changed.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void(CWallet* wallet, const CTxDestination& address, const std::string& label, bool isMine, const std::string& purpose, ChangeType status)> NotifyAddressBookChanged;

    /**
     * Wallet transaction added, removed or updated.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void(CWallet* wallet, const uint256& hashTx, ChangeType status)> NotifyTransactionChanged;

    /** Show progress e.g. for rescan */
    boost::signals2::signal<void(const std::string& title, int nProgress)> ShowProgress;

    /** Watch-only address added */
    boost::signals2::signal<void(bool fHaveWatchOnly)> NotifyWatchonlyChanged;

    /** notify wallet file backed up */
    boost::signals2::signal<void (const bool& fSuccess, const std::string& filename)> NotifyWalletBacked;

    bool ComputeStealthPublicAddress(const std::string& accountName, std::string& pubAddress);
    bool ComputeIntegratedPublicAddress(const uint64_t paymentID, const std::string& accountName, std::string& pubAddress);
    bool EncodeStealthPublicAddress(const std::vector<unsigned char>& pubViewKey, const std::vector<unsigned char>& pubSpendKey, std::string& pubAddr);
    bool EncodeStealthPublicAddress(const CPubKey& pubViewKey, const CPubKey& pubSpendKey, std::string& pubAddr);
    static bool DecodeStealthAddress(const std::string& stealth, CPubKey& pubViewKey, CPubKey& pubSpendKey, bool& hasPaymentID, uint64_t& paymentID);
    static bool ComputeStealthDestination(const CKey& secret, const CPubKey& pubViewKey, const CPubKey& pubSpendKey, CPubKey& des);
    bool SendToStealthAddress(const std::string& stealthAddr, CAmount nValue, CWalletTx& wtxNew, bool fUseIX = false, int ringSize = 5);
    bool GenerateAddress(CPubKey& pub, CPubKey& txPub, CKey& txPriv) const;
    bool IsTransactionForMe(const CTransaction& tx);
    bool ReadAccountList(std::string& accountList);
    bool ReadStealthAccount(const std::string& strAccount, CStealthAccount& account);
    bool EncodeIntegratedAddress(const CPubKey& pubViewKey, const CPubKey& pubSpendKey, uint64_t paymentID, std::string& pubAddr);
    bool EncodeIntegratedAddress(const std::vector<unsigned char>& pubViewKey, const std::vector<unsigned char>& pubSpendKey, uint64_t paymentID, std::string& pubAddrb58);
    bool GenerateIntegratedAddress(const std::string& accountName, std::string& pubAddr);
    std::string GenerateIntegratedAddressWithRandomPaymentID(std::string accountName, uint64_t& paymentID);
    std::string GenerateIntegratedAddressWithProvidedPaymentID(std::string accountName, uint64_t paymentID);
    bool GenerateIntegratedAddress(const CPubKey& pubViewKey, const CPubKey& pubSpendKey, std::string& pubAddr);
    bool AllMyPublicAddresses(std::vector<std::string>& addresses, std::vector<std::string>& accountNames);
    bool RevealTxOutAmount(const CTransaction &tx, const CTxOut &out, CAmount &amount, CKey&) const;
    bool EncodeTxOutAmount(CTxOut& out, const CAmount& amount, const unsigned char * sharedSec, bool isCoinstake = false);
    CAmount getCOutPutValue(const COutput& output) const;
    CAmount getCTxOutValue(const CTransaction &tx, const CTxOut &out) const;
    bool findCorrespondingPrivateKey(const CTxOut &txout, CKey &key) const;
    void CreatePrivacyAccount(bool force = false);
    bool mySpendPrivateKey(CKey& spend) const;
    bool myViewPrivateKey(CKey& view) const;
    static bool CreateCommitment(const CAmount val, CKey& blind, std::vector<unsigned char>& commitment);
    static bool CreateCommitment(const unsigned char* blind, CAmount val, std::vector<unsigned char>& commitment);
    static bool CreateCommitmentWithZeroBlind(const CAmount val, unsigned char* pBlind, std::vector<unsigned char>& commitment);
    bool WriteStakingStatus(bool status);
    bool ReadStakingStatus();
    bool Write2FA(bool status);
    bool Read2FA();
    bool Write2FASecret(std::string secret);
    std::string Read2FASecret();
    bool Write2FAPeriod(int period);
    int Read2FAPeriod();
    bool Write2FALastTime(uint64_t lastTime);
    uint64_t Read2FALastTime();
    bool MakeShnorrSignature(CTransaction&);
    bool MakeShnorrSignatureTxIn(CTxIn& txin, uint256);
    bool computeSharedSec(const CTransaction& tx, const CTxOut& out, CPubKey& sharedSec) const;
    void AddComputedPrivateKey(const CTxOut& out);
    bool IsCollateralized(const COutPoint& outpoint);

    /* Wallets parameter interaction */
    static bool ParameterInteraction();
private:
    bool encodeStealthBase58(const std::vector<unsigned char>& raw, std::string& stealth);
    bool allMyPrivateKeys(std::vector<CKey>& spends, std::vector<CKey>& views);
    void createMasterKey() const;
    bool generateBulletProofAggregate(CTransaction& tx);
    bool selectDecoysAndRealIndex(CTransaction& tx, int& myIndex, int ringSize);
    bool makeRingCT(CTransaction& wtxNew, int ringSize, std::string& strFailReason);
    int walletIdxCache = 0;
    bool isMatchMyKeyImage(const CKeyImage& ki, const COutPoint& out);
    void ScanWalletKeyImages();
};


/** A key allocated from the key pool. */
class CReserveKey
{
protected:
    CWallet* pwallet;
    int64_t nIndex;
    CPubKey vchPubKey;

public:
    CReserveKey(CWallet* pwalletIn)
    {
        nIndex = -1;
        pwallet = pwalletIn;
    }

    ~CReserveKey()
    {
        ReturnKey();
    }

    void ReturnKey();
    bool GetReservedKey(CPubKey& pubkey);
    void KeepKey();
};


typedef std::map<std::string, std::string> mapValue_t;


static void ReadOrderPos(int64_t& nOrderPos, mapValue_t& mapValue)
{
    if (!mapValue.count("n")) {
        nOrderPos = -1; // TODO: calculate elsewhere
        return;
    }
    nOrderPos = atoi64(mapValue["n"].c_str());
}


static void WriteOrderPos(const int64_t& nOrderPos, mapValue_t& mapValue)
{
    if (nOrderPos == -1)
        return;
    mapValue["n"] = i64tostr(nOrderPos);
}

struct COutputEntry {
    CTxDestination destination;
    CAmount amount;
    int vout;
};

/** A transaction with a merkle branch linking it to the block chain. */
class CMerkleTx : public CTransaction
{
private:
    int GetDepthInMainChainINTERNAL(const CBlockIndex*& pindexRet) const;

public:
    uint256 hashBlock;
    int nIndex;

    CMerkleTx()
    {
        Init();
    }

    CMerkleTx(const CTransaction& txIn) : CTransaction(txIn)
    {
        Init();
    }

    void Init()
    {
        hashBlock = UINT256_ZERO;
        nIndex = -1;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        std::vector<uint256> vMerkleBranch; // For compatibility with older versions.
        READWRITE(*(CTransaction*)this);
        nVersion = this->nVersion;
        READWRITE(hashBlock);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
    }

    int SetMerkleBranch(const CBlock& block);


    /**
     * Return depth of transaction in blockchain:
     * -1  : not in blockchain, and not in memory pool (conflicted transaction)
     *  0  : in memory pool, waiting to be included in a block
     * >=1 : this many blocks deep in the main chain
     */
    int GetDepthInMainChain(const CBlockIndex*& pindexRet, bool enableIX = true) const;
    int GetDepthInMainChain(bool enableIX = true) const;
    bool IsInMainChain() const;
    bool IsInMainChainImmature() const;
    int GetBlocksToMaturity() const;
    bool AcceptToMemoryPool(bool fLimitFree = true, bool fRejectInsaneFee = true, bool ignoreFees = false);
    int GetTransactionLockSignatures() const;
    bool IsTransactionLockTimedOut() const;
};

/**
 * A transaction with a bunch of additional info that only the owner cares about.
 * It includes any unrecorded transactions needed to link it back to the block chain.
 */
class CWalletTx : public CMerkleTx
{
private:
    CWallet* pwallet;

public:
    mapValue_t mapValue;
    std::vector<std::pair<std::string, std::string> > vOrderForm;
    unsigned int fTimeReceivedIsTxTime;
    unsigned int nTimeReceived; //! time received by this node
    unsigned int nTimeSmart;
    char fFromMe;
    std::string strFromAccount;
    int64_t nOrderPos; //! position in ordered transaction list
    char fSpent = false;

    // memory only
    mutable bool fDebitCached;
    mutable bool fCreditCached;
    mutable bool fImmatureCreditCached;
    mutable bool fAvailableCreditCached;
    mutable bool fWatchDebitCached;
    mutable bool fWatchCreditCached;
    mutable bool fImmatureWatchCreditCached;
    mutable bool fAvailableWatchCreditCached;
    mutable bool fChangeCached;

    mutable CAmount nDebitCached;
    mutable CAmount nCreditCached;
    mutable CAmount nImmatureCreditCached;
    mutable CAmount nAvailableCreditCached;
    mutable CAmount nWatchDebitCached;
    mutable CAmount nWatchCreditCached;
    mutable CAmount nImmatureWatchCreditCached;
    mutable CAmount nAvailableWatchCreditCached;
    mutable CAmount nChangeCached;


    CWalletTx();
    CWalletTx(CWallet* pwalletIn);
    CWalletTx(CWallet* pwalletIn, const CMerkleTx& txIn);
    CWalletTx(CWallet* pwalletIn, const CTransaction& txIn);
    void Init(CWallet* pwalletIn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        if (ser_action.ForRead())
            Init(NULL);
        if (!ser_action.ForRead()) {
            mapValue["fromaccount"] = strFromAccount;

            WriteOrderPos(nOrderPos, mapValue);

            if (nTimeSmart)
                mapValue["timesmart"] = strprintf("%u", nTimeSmart);
        }

        READWRITE(*(CMerkleTx*)this);
        std::vector<CMerkleTx> vUnused; //! Used to be vtxPrev
        READWRITE(vUnused);
        READWRITE(mapValue);
        READWRITE(vOrderForm);
        READWRITE(fTimeReceivedIsTxTime);
        READWRITE(nTimeReceived);
        READWRITE(fFromMe);
        READWRITE(fSpent);

        if (ser_action.ForRead()) {
            strFromAccount = mapValue["fromaccount"];

            ReadOrderPos(nOrderPos, mapValue);

            nTimeSmart = mapValue.count("timesmart") ? (unsigned int)atoi64(mapValue["timesmart"]) : 0;
        }

        mapValue.erase("fromaccount");
        mapValue.erase("version");
        mapValue.erase("spent");
        mapValue.erase("n");
        mapValue.erase("timesmart");
    }

    //! make sure balances are recalculated
    void MarkDirty();

    void BindWallet(CWallet* pwalletIn);

    //! filter decides which addresses will count towards the debit
    CAmount GetUnlockedCredit() const;
    CAmount GetDebit(const isminefilter& filter) const;
    CAmount GetCredit(const isminefilter& filter) const;
    CAmount GetImmatureCredit(bool fUseCache = true) const;
    CAmount GetAvailableCredit(bool fUseCache = true) const;
    // Return sum of unlocked coins
    CAmount GetLockedCredit() const;
    CAmount GetImmatureWatchOnlyCredit(const bool& fUseCache = true) const;
    CAmount GetAvailableWatchOnlyCredit(const bool& fUseCache = true) const;

    CAmount GetChange() const;

    void GetAmounts(std::list<COutputEntry>& listReceived,
                    std::list<COutputEntry>& listSent,
                    CAmount& nFee,
                    std::string& strSentAccount,
                    const isminefilter& filter) const;

    void GetAccountAmounts(const std::string& strAccount, CAmount& nReceived, CAmount& nSent, CAmount& nFee, const isminefilter& filter) const;

    bool IsFromMe(const isminefilter& filter) const;

    bool InMempool() const;

    bool IsTrusted() const;

    int GetBlockHeight() const;

    bool WriteToDisk(CWalletDB *pwalletdb);

    int64_t GetTxTime() const;
    int64_t GetComputedTxTime() const;
    int GetRequestCount() const;
    void RelayWalletTransaction(std::string strCommand = NetMsgType::TX);

    std::set<uint256> GetConflicts() const;
};


class COutput
{
public:
    const CWalletTx* tx;
    int i;
    int nDepth;
    bool fSpendable;

    COutput(const CWalletTx* txIn, int iIn, int nDepthIn, bool fSpendableIn)
    {
        tx = txIn;
        i = iIn;
        nDepth = nDepthIn;
        fSpendable = fSpendableIn;
    }

    CAmount Value() const
    {
        return tx->vout[i].nValue;
    }

    std::string ToString() const;
};


/** Private key that includes an expiration date in case it never gets used. */
class CWalletKey
{
public:
    CPrivKey vchPrivKey;
    int64_t nTimeCreated;
    int64_t nTimeExpires;
    std::string strComment;
    //! todo: add something to note what created it (user, getnewaddress, change)
    //!   maybe should have a map<string, string> property map

    CWalletKey(int64_t nExpires = 0);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vchPrivKey);
        READWRITE(nTimeCreated);
        READWRITE(nTimeExpires);
        READWRITE(LIMITED_STRING(strComment, 65536));
    }
};





/**
 * Internal transfers.
 * Database key is acentry<account><counter>.
 */
class CAccountingEntry
{
public:
    std::string strAccount;
    CAmount nCreditDebit;
    int64_t nTime;
    std::string strOtherAccount;
    std::string strComment;
    mapValue_t mapValue;
    int64_t nOrderPos; //! position in ordered transaction list
    uint64_t nEntryNo;

    CAccountingEntry()
    {
        SetNull();
    }

    void SetNull()
    {
        nCreditDebit = 0;
        nTime = 0;
        strAccount.clear();
        strOtherAccount.clear();
        strComment.clear();
        nOrderPos = -1;
        nEntryNo = 0;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        //! Note: strAccount is serialized as part of the key, not here.
        READWRITE(nCreditDebit);
        READWRITE(nTime);
        READWRITE(LIMITED_STRING(strOtherAccount, 65536));

        if (!ser_action.ForRead()) {
            WriteOrderPos(nOrderPos, mapValue);

            if (!(mapValue.empty() && _ssExtra.empty())) {
                CDataStream ss(nType, nVersion);
                ss.insert(ss.begin(), '\0');
                ss << mapValue;
                ss.insert(ss.end(), _ssExtra.begin(), _ssExtra.end());
                strComment.append(ss.str());
            }
        }

        READWRITE(LIMITED_STRING(strComment, 65536));

        size_t nSepPos = strComment.find("\0", 0, 1);
        if (ser_action.ForRead()) {
            mapValue.clear();
            if (std::string::npos != nSepPos) {
                CDataStream ss(std::vector<char>(strComment.begin() + nSepPos + 1, strComment.end()), nType, nVersion);
                ss >> mapValue;
                _ssExtra = std::vector<char>(ss.begin(), ss.end());
            }
            ReadOrderPos(nOrderPos, mapValue);
        }
        if (std::string::npos != nSepPos)
            strComment.erase(nSepPos);

        mapValue.erase("n");
    }

private:
    std::vector<char> _ssExtra;
};



#endif // BITCOIN_WALLET_H
