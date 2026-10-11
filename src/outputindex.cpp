// Copyright (c) 2026 The PRCY developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "outputindex.h"

#include "primitives/block.h"
#include "util.h"

static const char DB_OUTPUT_TX = 'T';
static const char DB_BEST_BLOCK = 'B';
static const char DB_VERSION = 'V';

COutputIndexTx::COutputIndexTx(const CTransaction& tx, int nHeightIn, const uint256& hashBlockIn) : nHeight(nHeightIn), hashBlock(hashBlockIn), fCoinBase(tx.IsCoinBase()), fCoinStake(tx.IsCoinStake())
{
    vout.reserve(tx.vout.size());
    for (const CTxOut& out : tx.vout)
        vout.emplace_back(out);
}

COutputIndexDB::COutputIndexDB(size_t nCacheSizeIn, bool fMemoryIn, bool fWipe) : CDBWrapper(GetDataDir() / "outputindex", nCacheSizeIn, fMemoryIn, fWipe), nCacheSize(nCacheSizeIn), fMemory(fMemoryIn)
{
}

bool COutputIndexDB::ReadTx(const uint256& txid, COutputIndexTx& rec) const
{
    return Read(std::make_pair(DB_OUTPUT_TX, txid), rec);
}

bool COutputIndexDB::ReadBestBlock(CBlockLocator& locator) const
{
    return Read(DB_BEST_BLOCK, locator);
}

bool COutputIndexDB::ReadVersion(int& nVersion) const
{
    return Read(DB_VERSION, nVersion);
}

void COutputIndexDB::AddBlock(CDBBatch& batch, const CBlock& block, int nHeight, const uint256& hashBlock)
{
    for (const CTransaction& tx : block.vtx)
        batch.Write(std::make_pair(DB_OUTPUT_TX, tx.GetHash()), COutputIndexTx(tx, nHeight, hashBlock));
}

void COutputIndexDB::RemoveBlock(CDBBatch& batch, const CBlock& block)
{
    for (const CTransaction& tx : block.vtx)
        batch.Erase(std::make_pair(DB_OUTPUT_TX, tx.GetHash()));
}

void COutputIndexDB::SetBestBlock(CDBBatch& batch, const CBlockLocator& locator)
{
    batch.Write(DB_BEST_BLOCK, locator);
}

void COutputIndexDB::SetVersion(CDBBatch& batch)
{
    const int nVersion = CURRENT_VERSION; // a copy: Write() takes a reference
    batch.Write(DB_VERSION, nVersion);
}
