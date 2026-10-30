// Copyright (c) 2026 The PRCY developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "outputindex.h"
#include "primitives/block.h"
#include "script/script.h"
#include "streams.h"
#include "test/test_prcycoin.h"

#include <limits>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(outputindex_tests, TestingSetup)

static CTxOut HiddenOutput(unsigned char seed)
{
    CScript script;
    script << std::vector<unsigned char>(33, seed) << OP_CHECKSIG;
    CTxOut out(0, script);
    out.commitment.assign(33, seed ^ 0x5a);
    out.maskValue.amount = uint256(std::vector<unsigned char>(32, seed + 1));
    out.maskValue.mask = uint256(std::vector<unsigned char>(32, seed + 2));
    out.txPub.assign(33, 0x02); // not kept by the index
    return out;
}

static COutputIndexEntry RoundTrip(const COutputIndexEntry& entry)
{
    CDataStream ss(SER_DISK, CLIENT_VERSION);
    ss << entry;
    COutputIndexEntry read;
    ss >> read;
    BOOST_CHECK(ss.empty());
    return read;
}

static void CheckSame(const CTxOut& a, const CTxOut& b)
{
    BOOST_CHECK_EQUAL(a.nValue, b.nValue);
    BOOST_CHECK(a.scriptPubKey == b.scriptPubKey);
    BOOST_CHECK(a.commitment == b.commitment);
    BOOST_CHECK(a.maskValue.amount == b.maskValue.amount);
    BOOST_CHECK(a.maskValue.mask == b.maskValue.mask);
}

BOOST_AUTO_TEST_CASE(entry_round_trip)
{
    // A hidden-amount output: everything validation reads survives; txPub is dropped.
    const CTxOut hidden = HiddenOutput(7);
    const CTxOut read = RoundTrip(COutputIndexEntry(hidden)).ToTxOut();
    CheckSame(hidden, read);
    BOOST_CHECK(read.txPub.empty());

    // A public amount with no encrypted amount, and the empty first output of a coinstake.
    CTxOut reward(5 * COIN, HiddenOutput(9).scriptPubKey);
    CheckSame(reward, RoundTrip(COutputIndexEntry(reward)).ToTxOut());
    CTxOut empty;
    empty.SetEmpty();
    CheckSame(empty, RoundTrip(COutputIndexEntry(empty)).ToTxOut());

    // Values that are never valid amounts still come back exactly.
    for (const CAmount nValue : {(CAmount)-1, (CAmount)1, (CAmount)70000000 * COIN, std::numeric_limits<CAmount>::max()}) {
        CTxOut out = HiddenOutput(3);
        out.nValue = nValue;
        CheckSame(out, RoundTrip(COutputIndexEntry(out)).ToTxOut());
    }

    // Scripts are stored as they are, including one over MAX_SCRIPT_SIZE.
    CTxOut big(0, CScript() << OP_RETURN << std::vector<unsigned char>(MAX_SCRIPT_SIZE + 10, 0x42));
    CheckSame(big, RoundTrip(COutputIndexEntry(big)).ToTxOut());
}

BOOST_AUTO_TEST_CASE(db_add_read_remove)
{
    COutputIndexDB db(1 << 20, true);

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vin[0].scriptSig = CScript() << 1 << OP_0;
    coinbase.vout.push_back(CTxOut(2 * COIN, HiddenOutput(1).scriptPubKey));

    CMutableTransaction spend;
    spend.vin.resize(1);
    spend.vin[0].prevout = COutPoint(uint256(std::vector<unsigned char>(32, 0x11)), 0);
    spend.vout.push_back(HiddenOutput(2));
    spend.vout.push_back(HiddenOutput(3));

    CBlock block;
    block.vtx.push_back(CTransaction(coinbase));
    block.vtx.push_back(CTransaction(spend));
    const uint256 hashBlock = uint256(std::vector<unsigned char>(32, 0xab));

    CBlockLocator locator(std::vector<uint256>(1, hashBlock));
    CDBBatch batch;
    COutputIndexDB::AddBlock(batch, block, 42, hashBlock);
    COutputIndexDB::SetBestBlock(batch, locator);
    BOOST_CHECK(db.WriteBatch(batch));

    CBlockLocator best;
    BOOST_CHECK(db.ReadBestBlock(best));
    BOOST_CHECK(best.vHave == locator.vHave);

    COutputIndexTx rec;
    BOOST_CHECK(db.ReadTx(block.vtx[0].GetHash(), rec));
    BOOST_CHECK_EQUAL(rec.nHeight, 42);
    BOOST_CHECK(rec.fCoinBase);
    BOOST_CHECK(!rec.fCoinStake);
    BOOST_CHECK(rec.hashBlock == hashBlock);
    BOOST_CHECK_EQUAL(rec.vout.size(), 1U);
    CheckSame(block.vtx[0].vout[0], rec.vout[0].ToTxOut());

    BOOST_CHECK(db.ReadTx(block.vtx[1].GetHash(), rec));
    BOOST_CHECK(!rec.fCoinBase);
    BOOST_CHECK_EQUAL(rec.vout.size(), 2U);
    CheckSame(block.vtx[1].vout[1], rec.vout[1].ToTxOut());

    CDBBatch erase;
    COutputIndexDB::RemoveBlock(erase, block);
    BOOST_CHECK(db.WriteBatch(erase));
    BOOST_CHECK(!db.ReadTx(block.vtx[0].GetHash(), rec));
    BOOST_CHECK(!db.ReadTx(block.vtx[1].GetHash(), rec));
}

BOOST_AUTO_TEST_SUITE_END()
