// Copyright (c) 2026 The PRCY developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRCY_OUTPUTINDEX_H
#define PRCY_OUTPUTINDEX_H

#include "dbwrapper.h"
#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"

#include <vector>

class CBlock;
struct CBlockLocator;

/**
 * The parts of a transaction output that validation reads when the output is spent or used as
 * a ring member: the plain amount, the script, the commitment and the encrypted amount and
 * blinding factor. Everything else in CTxOut (txPub, txPriv, hashOfKey, the masternode stealth
 * address) is only read from the transaction that creates the output, never from a later one.
 */
class COutputIndexEntry
{
public:
    CAmount nValue;
    CScript scriptPubKey;
    std::vector<unsigned char> commitment;
    uint256 amount;
    uint256 mask;

    COutputIndexEntry() : nValue(0) {}
    explicit COutputIndexEntry(const CTxOut& out) : nValue(out.nValue), scriptPubKey(out.scriptPubKey),
        commitment(out.commitment), amount(out.maskValue.amount), mask(out.maskValue.mask) {}

    /** A CTxOut holding these fields; the fields the index does not keep are left empty. */
    CTxOut ToTxOut() const
    {
        CTxOut out(nValue, scriptPubKey);
        out.commitment = commitment;
        out.maskValue.amount = amount;
        out.maskValue.mask = mask;
        return out;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        // nValue as an unsigned VARINT so every int64 value, including -1, round-trips exactly
        // (most outputs hide their amount and store 0 here, which takes one byte).
        uint64_t nValueBits = (uint64_t)nValue;
        READWRITE(VARINT(nValueBits));
        if (ser_action.ForRead())
            nValue = (CAmount)nValueBits;
        READWRITE(*(CScriptBase*)(&scriptPubKey));
        READWRITE(commitment);
        // Outputs with a public amount carry no encrypted amount; skip the 64 zero bytes.
        unsigned char fHasMask = (!amount.IsNull() || !mask.IsNull()) ? 1 : 0;
        READWRITE(fHasMask);
        if (fHasMask) {
            READWRITE(amount);
            READWRITE(mask);
        } else if (ser_action.ForRead()) {
            amount.SetNull();
            mask.SetNull();
        }
    }
};

/** One transaction's outputs, with the block that holds it. */
class COutputIndexTx
{
public:
    int nHeight;
    /**
     * That block's hash. A record is only used when the block at nHeight on the index's chain
     * is this one, so records left behind by blocks the node no longer has (a crash can lose
     * the last blocks from the block index) are never taken for current ones.
     */
    uint256 hashBlock;
    bool fCoinBase;  // also CTransaction::IsCoinAudit(), which is the same test
    bool fCoinStake;
    std::vector<COutputIndexEntry> vout;

    COutputIndexTx() : nHeight(0), fCoinBase(false), fCoinStake(false) {}
    COutputIndexTx(const CTransaction& tx, int nHeightIn, const uint256& hashBlockIn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(VARINT(nHeight));
        READWRITE(hashBlock);
        unsigned char nFlags = (fCoinBase ? 1 : 0) | (fCoinStake ? 2 : 0);
        READWRITE(nFlags);
        if (ser_action.ForRead()) {
            fCoinBase = (nFlags & 1) != 0;
            fCoinStake = (nFlags & 2) != 0;
        }
        READWRITE(vout);
    }
};

/**
 * Access to the output index (outputindex/): for every transaction in a block on the active
 * chain, the outputs validation needs, so that checking a ring member or a stake input does not
 * have to read the whole transaction back out of the block files.
 *
 * It is maintained by ConnectTip()/DisconnectTip() and brought up to date with the chain at
 * startup by SyncOutputIndex(); the best-block record, a locator of the block the index is
 * complete up to, is written in the same batch as that block's records.
 */
class COutputIndexDB : public CDBWrapper
{
public:
    /** Bump to make every node rebuild its index at the next start. */
    static const int CURRENT_VERSION = 1;

    COutputIndexDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool ReadTx(const uint256& txid, COutputIndexTx& rec) const;
    bool ReadBestBlock(CBlockLocator& locator) const;
    bool ReadVersion(int& nVersion) const;

    /** Queue the records for every transaction in block, which is at nHeight with hashBlock. */
    static void AddBlock(CDBBatch& batch, const CBlock& block, int nHeight, const uint256& hashBlock);
    /** Queue the removal of the records for every transaction in block. */
    static void RemoveBlock(CDBBatch& batch, const CBlock& block);
    static void SetBestBlock(CDBBatch& batch, const CBlockLocator& locator);
    static void SetVersion(CDBBatch& batch);

    size_t GetCacheSize() const { return nCacheSize; }
    bool IsMemory() const { return fMemory; }

private:
    size_t nCacheSize;
    bool fMemory;

    COutputIndexDB(const COutputIndexDB&);
    void operator=(const COutputIndexDB&);
};

#endif // PRCY_OUTPUTINDEX_H
