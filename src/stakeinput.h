// Copyright (c) 2017 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRCY_STAKEINPUT_H
#define PRCY_STAKEINPUT_H

#include "chain.h"
#include "streams.h"
#include "uint256.h"

class CKeyStore;
class CWallet;
class CWalletTx;

class CStakeInput
{
protected:
    CBlockIndex* pindexFrom;

public:
    virtual ~CStakeInput(){};
    virtual CBlockIndex* GetIndexFrom() = 0;
    virtual bool CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut = 0) = 0;
    virtual bool GetTxFrom(CTransaction& tx) = 0;
    virtual CAmount GetValue() = 0;
    virtual bool CreateTxOuts(CWallet* pwallet, std::vector<CTxOut>& vout, CAmount nTotal) = 0;
    virtual bool GetModifier(uint64_t& nStakeModifier) = 0;
    virtual CDataStream GetUniqueness() = 0;
};

class CPrcyStake : public CStakeInput
{
private:
    CTransaction txFrom; // the whole transaction, when set by SetInput() (staking)
    bool fHaveTxFrom;
    uint256 hashFrom;
    unsigned int nPosition;
    CTxOut outFrom; // the staked output alone, when set by SetPrevout() (validation)
    bool fFromCoinBaseOrStake;
public:
    CPrcyStake() : fHaveTxFrom(false), nPosition(0), fFromCoinBaseOrStake(false)
    {
        this->pindexFrom = nullptr;
    }

    bool SetInput(CTransaction txPrev, unsigned int n);
    /** For checking a coinstake: the staked output as GetOutput() returns it, not its whole transaction. */
    bool SetPrevout(const COutPoint& prevout, const CTxOut& out, bool fCoinBaseOrStake);

    CBlockIndex* GetIndexFrom() override;
    bool GetTxFrom(CTransaction& tx) override;
    CAmount GetValue() override;
    bool GetModifier(uint64_t& nStakeModifier) override;
    CDataStream GetUniqueness() override;
    bool CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut = 0) override;
    bool CreateTxOuts(CWallet* pwallet, std::vector<CTxOut>& vout, CAmount nTotal) override;
};


#endif //PRCY_STAKEINPUT_H
