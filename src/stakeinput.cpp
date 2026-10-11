// Copyright (c) 2017 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "main.h"
#include "stakeinput.h"
#include "wallet/wallet.h"

//!PRCY Stake
bool CPrcyStake::SetInput(CTransaction txPrev, unsigned int n)
{
    this->txFrom = txPrev;
    this->fHaveTxFrom = true;
    this->hashFrom = txPrev.GetHash();
    this->nPosition = n;
    return true;
}

bool CPrcyStake::SetPrevout(const COutPoint& prevout, const CTxOut& out, bool fCoinBaseOrStake)
{
    this->fHaveTxFrom = false;
    this->hashFrom = prevout.hash;
    this->nPosition = prevout.n;
    this->outFrom = out;
    this->fFromCoinBaseOrStake = fCoinBaseOrStake;
    return true;
}

bool CPrcyStake::GetTxFrom(CTransaction& tx)
{
    if (!fHaveTxFrom)
        return false;
    tx = txFrom;
    return true;
}

bool CPrcyStake::CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut)
{
    CPubKey sharedSec;
    txIn = CTxIn(txFrom.GetHash(), nPosition);

    const CWalletTx* pWalletTx = pwalletMain->GetWalletTx(txFrom.GetHash());
    pwalletMain->computeSharedSec(txFrom, pWalletTx->vout[nPosition], sharedSec);

    //copy encryption key so that full nodes can decode the amount in the txin
    std::copy(sharedSec.begin(), sharedSec.begin() + 33, std::back_inserter(txIn.encryptionKey));

    CScript scriptPubKeyKernel = txFrom.vout[nPosition].scriptPubKey;

    if (!pwalletMain->generateKeyImage(scriptPubKeyKernel, txIn.keyImage)) {
        LogPrintf("CreateCoinStake : cannot generate key image\n");
        return false;
    }
    return true;
}

CAmount CPrcyStake::GetValue()
{
    if (fHaveTxFrom ? (txFrom.IsCoinBase() || txFrom.IsCoinStake()) : fFromCoinBaseOrStake) {
        return fHaveTxFrom ? txFrom.vout[nPosition].nValue : outFrom.nValue;
    }

    if (!pwalletMain) // a coinstake that fails its check on a node without a wallet
        return 0;
    const CWalletTx* pWalletTx = pwalletMain->GetWalletTx(hashFrom);
    if (!pWalletTx) {
        return 0;
    }

    return pwalletMain->getCTxOutValue(*pWalletTx, pWalletTx->vout[nPosition]);
}

bool CPrcyStake::CreateTxOuts(CWallet* pwallet, std::vector<CTxOut>& vout, CAmount nTotal)
{
    std::vector<valtype> vSolutions;
    txnouttype whichType;
    CScript scriptPubKeyKernel = txFrom.vout[nPosition].scriptPubKey;

    CKey view, spend;
    pwalletMain->myViewPrivateKey(view);
    pwalletMain->mySpendPrivateKey(spend);
    CPubKey viewPub = view.GetPubKey();
    CPubKey spendPub = spend.GetPubKey();

    if (!Solver(scriptPubKeyKernel, whichType, vSolutions)) {
        LogPrintf("CreateCoinStake : failed to parse kernel\n");
        return false;
    }

    if (whichType != TX_PUBKEY && whichType != TX_PUBKEYHASH)
        return false; // only support pay to public key and pay to address

    CScript scriptPubKey;
    if (whichType == TX_PUBKEYHASH) // pay to address type
    {
        //convert to pay to public key type
        CKey key;
        if (!pwallet->GetKey(uint160(vSolutions[0]), key))
            return false;

        scriptPubKey << key.GetPubKey() << OP_CHECKSIG;
    } else
        scriptPubKey = scriptPubKeyKernel;

    //first UTXO for the staked amount
    //create a new pubkey
    CKey myTxPriv;
    myTxPriv.MakeNewKey(true);
    CPubKey txPub = myTxPriv.GetPubKey();
    CPubKey newPub;
    CWallet::ComputeStealthDestination(myTxPriv, viewPub, spendPub, newPub);
    scriptPubKey = GetScriptForDestination(newPub);
    CTxOut out(0, scriptPubKey);
    std::copy(txPub.begin(), txPub.end(), std::back_inserter(out.txPub));
    vout.emplace_back(out);

    //second UTXO for staking reward
    //create a new pubkey
    CKey myTxPrivStaking;
    myTxPrivStaking.MakeNewKey(true);
    CPubKey txPubStaking = myTxPrivStaking.GetPubKey();
    CPubKey newPubStaking;
    CWallet::ComputeStealthDestination(myTxPrivStaking, viewPub, spendPub, newPubStaking);
    CScript scriptPubKeyOutStaking = GetScriptForDestination(newPubStaking);
    CTxOut outStaking(0, scriptPubKeyOutStaking);
    std::copy(txPubStaking.begin(), txPubStaking.end(), std::back_inserter(outStaking.txPub));
    vout.emplace_back(outStaking);

    // Calculate if we need to split the output
    /*if (nTotal / 2 > (CAmount)(pwallet->nStakeSplitThreshold * COIN))
        vout.emplace_back(CTxOut(0, scriptPubKey));*/

    return true;
}

bool CPrcyStake::GetModifier(uint64_t& nStakeModifier)
{
    int nStakeModifierHeight = 0;
    int64_t nStakeModifierTime = 0;
    GetIndexFrom();
    if (!pindexFrom)
        return error("%s: failed to get index from", __func__);

    if (!GetKernelStakeModifier(pindexFrom->GetBlockHash(), nStakeModifier, nStakeModifierHeight, nStakeModifierTime, false))
        return error("CheckStakeKernelHash(): failed to get kernel stake modifier \n");

    return true;
}

CDataStream CPrcyStake::GetUniqueness()
{
    //The unique identifier for a PRCY stake is the outpoint
    CDataStream ss(SER_NETWORK, 0);
    ss << nPosition << hashFrom;
    return ss;
}

//The block that the UTXO was added to the chain
CBlockIndex* CPrcyStake::GetIndexFrom()
{
    uint256 hashBlock = 0;
    CTxOut out;
    bool fTxFound = false;
    GetOutput(COutPoint(hashFrom, nPosition), out, hashBlock, nullptr, nullptr, &fTxFound); // the transaction has to exist, as before
    if (fTxFound) {
        // If the index is in the chain, then set it as the "index from"
        if (mapBlockIndex.count(hashBlock)) {
            CBlockIndex* pindex = mapBlockIndex.at(hashBlock);
            if (chainActive.Contains(pindex))
                pindexFrom = pindex;
        }
    } else {
        LogPrintf("%s : failed to find tx %s\n", __func__, hashFrom.GetHex());
    }

    return pindexFrom;
}
