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
    this->nPosition = n;
    return true;
}

bool CPrcyStake::GetTxFrom(CTransaction& tx)
{
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

CAmount CPrcyStake::GetValue(const unsigned char* encryptionKey)
{
    /*// If it's from CoinBase or CoinStake, value is visible
    if (txFrom.IsCoinBase() || txFrom.IsCoinStake()) {
        return txFrom.vout[nPosition].nValue;
    }*/

    // If it's from our wallet, we already have the value
    const CWalletTx* pWalletTx = pwalletMain->GetWalletTx(txFrom.GetHash());
    if (pWalletTx && pwalletMain->IsMine(pWalletTx->vout[nPosition])) {
        return pwalletMain->getCTxOutValue(*pWalletTx, pWalletTx->vout[nPosition]);
    }

    // Neither case, decode it
    CAmount nValueIn = 0;
    uint256 val = txFrom.vout[nPosition].maskValue.amount;
    uint256 mask = txFrom.vout[nPosition].maskValue.mask;
    CKey decodedMask;
    CPubKey sharedSec;
    sharedSec.Set(encryptionKey, encryptionKey + 33);
    ECDHInfo::Decode(mask.begin(), val.begin(), sharedSec, decodedMask, nValueIn);
    return nValueIn;
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

    for (int i = 1; i <= 2; ++i) {
        CKey myTxPriv;
        myTxPriv.MakeNewKey(true);
        CPubKey txPub = myTxPriv.GetPubKey();
        CPubKey newPub;
        CWallet::ComputeStealthDestination(myTxPriv, viewPub, spendPub, newPub);
        CScript scriptPubKeyOut = GetScriptForDestination(newPub);
        CTxOut out(0, scriptPubKeyOut);
        std::copy(txPub.begin(), txPub.end(), std::back_inserter(out.txPub));
        vout.emplace_back(out);
    }

    // Calculate if we need to split the output
    /*if (nTotal / 2 > (CAmount)(pwallet->nStakeSplitThreshold * COIN))
        vout.emplace_back(CTxOut(0, scriptPubKey));*/

    return true;
}

bool CPrcyStake::GetModifier(uint64_t& nStakeModifier)
{
    if (this->nStakeModifier == 0) {
        // look for the modifier
        GetIndexFrom();
        if (!pindexFrom)
            return error("%s: failed to get index from", __func__);
        // TODO: This method must be removed from here in the short terms.. it's a call to an static method in kernel.cpp when this class method is only called from kernel.cpp, no comments..
        if (!GetKernelStakeModifier(pindexFrom->GetBlockHash(), this->nStakeModifier, this->nStakeModifierHeight, this->nStakeModifierTime, false))
            return error("CheckStakeKernelHash(): failed to get kernel stake modifier");
    }
    nStakeModifier = this->nStakeModifier;
    return true;
}

CDataStream CPrcyStake::GetUniqueness()
{
    //The unique identifier for a PRCY stake is the outpoint
    CDataStream ss(SER_NETWORK, 0);
    ss << nPosition << txFrom.GetHash();
    return ss;
}

//The block that the UTXO was added to the chain
CBlockIndex* CPrcyStake::GetIndexFrom()
{
    if (pindexFrom)
        return pindexFrom;
    uint256 hashBlock = 0;
    CTransaction tx;
    if (GetTransaction(txFrom.GetHash(), tx, hashBlock, true)) {
        // If the index is in the chain, then set it as the "index from"
        if (mapBlockIndex.count(hashBlock)) {
            CBlockIndex* pindex = mapBlockIndex.at(hashBlock);
            if (chainActive.Contains(pindex))
                pindexFrom = pindex;
        }
    } else {
        LogPrintf("%s : failed to find tx %s\n", __func__, txFrom.GetHash().GetHex());
    }

    return pindexFrom;
}
