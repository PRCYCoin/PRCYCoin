// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "data/tx_invalid.json.h"
#include "data/tx_valid.json.h"

#include "clientversion.h"
#include "key.h"
#include "keystore.h"
#include "main.h"
#include "script/script.h"
#include "script/script_error.h"
#include "core_io.h"

#include <map>
#include <string>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>

#include <univalue.h>


// In script_tests.cpp
extern UniValue read_json(const std::string& jsondata);

static std::map<string, unsigned int> mapFlagNames = boost::assign::map_list_of
        (string("NONE"), (unsigned int)SCRIPT_VERIFY_NONE)
        (string("P2SH"), (unsigned int)SCRIPT_VERIFY_P2SH)
        (string("STRICTENC"), (unsigned int)SCRIPT_VERIFY_STRICTENC)
        (string("DERSIG"), (unsigned int)SCRIPT_VERIFY_DERSIG)
        (string("LOW_S"), (unsigned int)SCRIPT_VERIFY_LOW_S)
        (string("SIGPUSHONLY"), (unsigned int)SCRIPT_VERIFY_SIGPUSHONLY)
        (string("MINIMALDATA"), (unsigned int)SCRIPT_VERIFY_MINIMALDATA)
        (string("NULLDUMMY"), (unsigned int)SCRIPT_VERIFY_NULLDUMMY)
        (string("DISCOURAGE_UPGRADABLE_NOPS"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS)
        (string("CLEANSTACK"), (unsigned int)SCRIPT_VERIFY_CLEANSTACK)
        (string("CHECKLOCKTIMEVERIFY"), (unsigned int)SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY);
	
unsigned int ParseScriptFlags(string strFlags)
{
    if (strFlags.empty()) {
        return 0;
    }
    unsigned int flags = 0;
    vector<string> words;
    split(words, strFlags, is_any_of(","));

    for (string word : words)
    {
        if (!mapFlagNames.count(word))
            BOOST_ERROR("Bad test: unknown verification flag '" << word << "'");
        flags |= mapFlagNames[word];
    }

    return flags;
}

string FormatScriptFlags(unsigned int flags)
{
    if (flags == 0) {
        return "";
    }
    string ret;
    std::map<string, unsigned int>::const_iterator it = mapFlagNames.begin();
    while (it != mapFlagNames.end()) {
        if (flags & it->second) {
            ret += it->first + ",";
        }
        it++;
    }
    return ret.substr(0, ret.size() - 1);
}

#ifdef DISABLE_FAILED_TEST
BOOST_AUTO_TEST_SUITE(transaction_tests)

BOOST_AUTO_TEST_CASE(tx_valid)
        {
                // Read tests from test/data/tx_valid.json
                // Format is an array of arrays
                // Inner arrays are either [ "comment" ]
                // or [[[prevout hash, prevout index, prevout scriptPubKey], [input 2], ...],"], serializedTransaction, verifyFlags
                // ... where all scripts are stringified scripts.
                //
                // verifyFlags is a comma separated list of script verification flags to apply, or "NONE"
                UniValue tests = read_json(std::string(json_tests::tx_valid, json_tests::tx_valid + sizeof(json_tests::tx_valid)));

                ScriptError err;
                for (unsigned int idx = 0; idx < tests.size(); idx++) {
            UniValue test = tests[idx];
            string strTest = test.write();
            if (test[0].isArray())
            {
                if (test.size() != 3 || !test[1].isStr() || !test[2].isStr())
                {
                    BOOST_ERROR("Bad test: " << strTest);
                    continue;
                }

                map<COutPoint, CScript> mapprevOutScriptPubKeys;
                UniValue inputs = test[0].get_array();
                bool fValid = true;
                for (unsigned int inpIdx = 0; inpIdx < inputs.size(); inpIdx++) {
                    const UniValue& input = inputs[inpIdx];
                    if (!input.isArray())
                    {
                        fValid = false;
                        break;
                    }
                    UniValue vinput = input.get_array();
                    if (vinput.size() != 3)
                    {
                        fValid = false;
                        break;
                    }

                    mapprevOutScriptPubKeys[COutPoint(uint256(vinput[0].get_str()), vinput[1].get_int())] = ParseScript(vinput[2].get_str());
                }
                if (!fValid)
                {
                    BOOST_ERROR("Bad test: " << strTest);
                    continue;
                }

                string transaction = test[1].get_str();
                CDataStream stream(ParseHex(transaction), SER_NETWORK, PROTOCOL_VERSION);
                CTransaction tx;
                stream >> tx;

                CValidationState state;
                BOOST_CHECK_MESSAGE(CheckTransaction(tx, false, state), strTest);
                BOOST_CHECK(state.IsValid());

                for (unsigned int i = 0; i < tx.vin.size(); i++)
                {
                    if (!mapprevOutScriptPubKeys.count(tx.vin[i].prevout))
                    {
                        BOOST_ERROR("Bad test: " << strTest);
                        break;
                    }

                    unsigned int verify_flags = ParseScriptFlags(test[2].get_str());
                    BOOST_CHECK_MESSAGE(VerifyScript(tx.vin[i].scriptSig, mapprevOutScriptPubKeys[tx.vin[i].prevout],
                                                     verify_flags, TransactionSignatureChecker(&tx, i), &err),
                                        strTest);
                    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
                }
            }
        }
        }

BOOST_AUTO_TEST_CASE(tx_invalid)
        {
                // Read tests from test/data/tx_invalid.json
                // Format is an array of arrays
                // Inner arrays are either [ "comment" ]
                // or [[[prevout hash, prevout index, prevout scriptPubKey], [input 2], ...],"], serializedTransaction, verifyFlags
                // ... where all scripts are stringified scripts.
                //
                // verifyFlags is a comma separated list of script verification flags to apply, or "NONE"
                UniValue tests = read_json(std::string(json_tests::tx_invalid, json_tests::tx_invalid + sizeof(json_tests::tx_invalid)));

                ScriptError err;
                for (unsigned int idx = 0; idx < tests.size(); idx++) {
            UniValue test = tests[idx];
            string strTest = test.write();
            if (test[0].isArray())
            {
                if (test.size() != 3 || !test[1].isStr() || !test[2].isStr())
                {
                    BOOST_ERROR("Bad test: " << strTest);
                    continue;
                }

                map<COutPoint, CScript> mapprevOutScriptPubKeys;
                UniValue inputs = test[0].get_array();
                bool fValid = true;
                for (unsigned int inpIdx = 0; inpIdx < inputs.size(); inpIdx++) {
                    const UniValue& input = inputs[inpIdx];
                    if (!input.isArray())
                    {
                        fValid = false;
                        break;
                    }
                    UniValue vinput = input.get_array();
                    if (vinput.size() != 3)
                    {
                        fValid = false;
                        break;
                    }

                    mapprevOutScriptPubKeys[COutPoint(uint256(vinput[0].get_str()), vinput[1].get_int())] = ParseScript(vinput[2].get_str());
                }
                if (!fValid)
                {
                    BOOST_ERROR("Bad test: " << strTest);
                    continue;
                }

                string transaction = test[1].get_str();
                CDataStream stream(ParseHex(transaction), SER_NETWORK, PROTOCOL_VERSION);
                CTransaction tx;
                stream >> tx;

                CValidationState state;
                fValid = CheckTransaction(tx, false, state) && state.IsValid();

                for (unsigned int i = 0; i < tx.vin.size() && fValid; i++)
                {
                    if (!mapprevOutScriptPubKeys.count(tx.vin[i].prevout))
                    {
                        BOOST_ERROR("Bad test: " << strTest);
                        break;
                    }

                    unsigned int verify_flags = ParseScriptFlags(test[2].get_str());
                    fValid = VerifyScript(tx.vin[i].scriptSig, mapprevOutScriptPubKeys[tx.vin[i].prevout],
                                          verify_flags, TransactionSignatureChecker(&tx, i), &err);
                }
                BOOST_CHECK_MESSAGE(!fValid, strTest);
                BOOST_CHECK_MESSAGE(err != SCRIPT_ERR_OK, ScriptErrorString(err));
            }
        }
        }

BOOST_AUTO_TEST_CASE(basic_transaction_tests)
{
    // Random real transaction (ef59a7f1c8c1c75faabbcab542f81f5406f5b4b3d68ca150d665c260e064db54)
    unsigned char ch[] = {0x01,0x00,0x00,0x00,0x01,0xd8,0x73,0xc0,0x6a,0xca,0x7a,0x5e,0x5a,0x2f,0xab,0x6f,0x73,0xe9,0x09,0x1b,0x38,0x82,0x2c,0x02,0xd5,0x58,0x05,0x5b,0x96,0x69,0x09,0xf3,0x03,0x43,0x89,0x3b,0xc4,0x01,0x00,0x00,0x00,0x48,0x47,0x30,0x44,0x02,0x20,0x5b,0x35,0x3e,0xda,0x4d,0x08,0x2f,0x5b,0x18,0x16,0xca,0x5a,0x7b,0x05,0x65,0xe7,0x98,0x02,0x3a,0x2c,0xaf,0x38,0xb6,0xbc,0x23,0x12,0x81,0x35,0x93,0x6e,0xa6,0x85,0x02,0x20,0x15,0xe8,0x01,0x3b,0x7f,0xea,0xaf,0xef,0x72,0xe0,0xf5,0xad,0xb5,0x53,0x22,0xfa,0x82,0xae,0x88,0xdb,0x7f,0x83,0x35,0xb2,0x8e,0x9a,0xeb,0x94,0x93,0xb0,0x82,0x82,0x01,0xff,0xff,0xff,0xff,0x00,0x21,0x02,0x2f,0xaa,0xa4,0x51,0x15,0x06,0x6a,0x38,0x41,0xd0,0xee,0x97,0x70,0xa5,0x5b,0xc0,0xb6,0x6d,0xfb,0x2c,0x75,0xbe,0x9c,0x23,0x18,0xd2,0x81,0xbd,0xa5,0x1f,0x13,0x47,0x05,0x17,0x2e,0x55,0xe3,0xac,0x18,0xfc,0xd3,0x3f,0x68,0x45,0xa6,0xee,0xbb,0xb7,0xd9,0x95,0xf3,0x92,0xca,0x09,0x8e,0x25,0x90,0xfb,0xf4,0xbb,0xa5,0xf7,0xc4,0xd3,0x4f,0x01,0x00,0x00,0x00,0x17,0x2e,0x55,0xe3,0xac,0x18,0xfc,0xd3,0x3f,0x68,0x45,0xa6,0xee,0xbb,0xb7,0xd9,0x95,0xf3,0x92,0xca,0x09,0x8e,0x25,0x90,0xfb,0xf4,0xbb,0xa5,0xf7,0xc4,0xd3,0x4f,0x02,0x00,0x00,0x00,0x17,0x2e,0x55,0xe3,0xac,0x18,0xfc,0xd3,0x3f,0x68,0x45,0xa6,0xee,0xbb,0xb7,0xd9,0x95,0xf3,0x92,0xca,0x09,0x8e,0x25,0x90,0xfb,0xf4,0xbb,0xa5,0xf7,0xc4,0xd3,0x4f,0x03,0x00,0x00,0x00,0xc2,0x58,0xfd,0xbe,0x8d,0x9c,0x7b,0x80,0x7e,0x8c,0x33,0xee,0xb8,0x40,0x8a,0xb6,0x1d,0x98,0x0a,0x0c,0x1c,0xf1,0x7e,0x50,0x58,0x31,0x32,0x21,0x3b,0xf8,0x7f,0xe4,0x01,0x00,0x00,0x00,0xc2,0x58,0xfd,0xbe,0x8d,0x9c,0x7b,0x80,0x7e,0x8c,0x33,0xee,0xb8,0x40,0x8a,0xb6,0x1d,0x98,0x0a,0x0c,0x1c,0xf1,0x7e,0x50,0x58,0x31,0x32,0x21,0x3b,0xf8,0x7f,0xe4,0x02,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x23,0x21,0x03,0x24,0xde,0xca,0x39,0x9c,0xa2,0xfd,0x68,0x12,0xfe,0xcf,0x2e,0x6e,0x73,0x6c,0x2e,0xe6,0x63,0x17,0x8f,0x48,0xec,0x4c,0x21,0xcb,0xc9,0x48,0xca,0xf7,0xee,0xa8,0xa2,0xac,0x6e,0xdf,0xfc,0x3e,0xdb,0x8b,0x0c,0x32,0x3a,0x28,0x32,0xa8,0xa5,0x8b,0x85,0xe8,0x56,0xfc,0x7c,0x5b,0x6d,0xab,0x0d,0xa2,0x2a,0xae,0xee,0x20,0xd9,0xac,0xd7,0xc3,0xb5,0xeb,0xd9,0xb1,0x93,0xe8,0xd2,0x5a,0xb2,0x97,0x14,0xd9,0x12,0xff,0x4f,0x98,0x46,0xe7,0x34,0x2c,0x58,0xcb,0xf2,0x96,0x10,0x51,0xf7,0x68,0x63,0xd6,0x6c,0xe2,0xf6,0x75,0xa7,0xb4,0x6a,0x47,0x7b,0xf0,0x1d,0x33,0x55,0x17,0x25,0x4f,0x3c,0x4e,0xad,0x71,0xd5,0xf5,0x6e,0x11,0x49,0x80,0x21,0x30,0xa4,0x26,0xb7,0x30,0x9d,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x23,0x21,0x02,0xed,0x68,0x9c,0x2b,0x17,0xa4,0x47,0xe1,0x71,0x4a,0x0e,0xec,0xcb,0x9d,0x12,0xc0,0x66,0x26,0xe8,0x7a,0x05,0xfb,0xf6,0xdf,0x4d,0x27,0x49,0xe7,0x18,0xd2,0x34,0x03,0xac,0x83,0xce,0xfc,0xd5,0xe0,0x4c,0xb6,0x0d,0x0a,0x9e,0x83,0x1e,0x96,0x0c,0xef,0x39,0x74,0xb6,0xf7,0x30,0x8f,0xf5,0x79,0x5e,0x18,0x5a,0xa1,0x40,0xf4,0xa6,0xc2,0xe1,0x5b,0x3a,0xda,0x38,0xc5,0x21,0x92,0x96,0xfe,0x7c,0x2f,0xc3,0x97,0x57,0x0a,0xc0,0x45,0x90,0x9c,0x84,0x1b,0x5c,0xbd,0x1b,0x41,0xef,0xf4,0x6c,0xe9,0x19,0xf3,0x6e,0x32,0x8f,0x8e,0xff,0x2a,0xa3,0x41,0xba,0x31,0x88,0x46,0xe9,0x37,0xdd,0x12,0xe2,0x86,0x14,0xdf,0xdb,0xab,0x56,0xe6,0x0d,0xc0,0x64,0x4d,0x64,0xfd,0x3d,0xca,0x2a,0x00,0x00,0x00,0x00,0x00,0x00,0x21,0x03,0xa7,0x38,0x11,0x9f,0xdf,0x41,0x90,0x4a,0x31,0x72,0x46,0x3f,0xae,0x0d,0x51,0xf3,0x2c,0x67,0xcd,0x08,0xf7,0xac,0xfb,0x52,0x33,0x2c,0xbd,0x17,0x49,0x2e,0x19,0xf2,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    vector<unsigned char> vch(ch, ch + sizeof(ch));
    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    CMutableTransaction tx;
    stream >> tx;
    CValidationState state;
    BOOST_CHECK_MESSAGE(CheckTransaction(tx, false, state) && state.IsValid(), "Simple deserialized transaction should be valid.");

    // Check that duplicate txins fail
    tx.vin.push_back(tx.vin[0]);
    BOOST_CHECK_MESSAGE(!CheckTransaction(tx, false, state) || !state.IsValid(), "Transaction with duplicate txins should be invalid.");
}

//
// Helper: create two dummy transactions, each with
// two outputs.  The first has 11 and 50 CENT outputs
// paid to a TX_PUBKEY, the second 21 and 22 CENT outputs
// paid to a TX_PUBKEYHASH.
//
static std::vector<CMutableTransaction>
SetupDummyInputs(CBasicKeyStore& keystoreRet, CCoinsViewCache& coinsRet)
{
    std::vector<CMutableTransaction> dummyTransactions;
    dummyTransactions.resize(2);

    // Add some keys to the keystore:
    CKey key[4];
    for (int i = 0; i < 4; i++)
    {
        key[i].MakeNewKey(i % 2);
        keystoreRet.AddKey(key[i]);
    }

    // Create some dummy input transactions
    dummyTransactions[0].vout.resize(2);
    dummyTransactions[0].vout[0].nValue = 11*CENT;
    dummyTransactions[0].vout[0].scriptPubKey << ToByteVector(key[0].GetPubKey()) << OP_CHECKSIG;
    dummyTransactions[0].vout[1].nValue = 50*CENT;
    dummyTransactions[0].vout[1].scriptPubKey << ToByteVector(key[1].GetPubKey()) << OP_CHECKSIG;
    coinsRet.ModifyCoins(dummyTransactions[0].GetHash())->FromTx(dummyTransactions[0], 0);

    dummyTransactions[1].vout.resize(2);
    dummyTransactions[1].vout[0].nValue = 21*CENT;
    dummyTransactions[1].vout[0].scriptPubKey = GetScriptForDestination(key[2].GetPubKey().GetID());
    dummyTransactions[1].vout[1].nValue = 22*CENT;
    dummyTransactions[1].vout[1].scriptPubKey = GetScriptForDestination(key[3].GetPubKey().GetID());
    coinsRet.ModifyCoins(dummyTransactions[1].GetHash())->FromTx(dummyTransactions[1], 0);

    return dummyTransactions;
}

BOOST_AUTO_TEST_CASE(test_Get)
        {
                CBasicKeyStore keystore;
                CCoinsView coinsDummy;
                CCoinsViewCache coins(&coinsDummy);
                std::vector<CMutableTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

                CMutableTransaction t1;
                t1.vin.resize(3);
                t1.vin[0].prevout.hash = dummyTransactions[0].GetHash();
                t1.vin[0].prevout.n = 1;
                t1.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
                t1.vin[1].prevout.hash = dummyTransactions[1].GetHash();
                t1.vin[1].prevout.n = 0;
                t1.vin[1].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
                t1.vin[2].prevout.hash = dummyTransactions[1].GetHash();
                t1.vin[2].prevout.n = 1;
                t1.vin[2].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
                t1.vout.resize(2);
                t1.vout[0].nValue = 90*CENT;
                t1.vout[0].scriptPubKey << OP_1;

                BOOST_CHECK(AreInputsStandard(t1, coins));
                BOOST_CHECK_EQUAL(coins.GetValueIn(t1), (50+21+22)*CENT);

                // Adding extra junk to the scriptSig should make it non-standard:
                t1.vin[0].scriptSig << OP_11;
                BOOST_CHECK(!AreInputsStandard(t1, coins));

                // ... as should not having enough:
                t1.vin[0].scriptSig = CScript();
                BOOST_CHECK(!AreInputsStandard(t1, coins));
        }

BOOST_AUTO_TEST_CASE(test_IsStandard)
        {
                LOCK(cs_main);
                CBasicKeyStore keystore;
                CCoinsView coinsDummy;
                CCoinsViewCache coins(&coinsDummy);
                std::vector<CMutableTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

                CMutableTransaction t;
                t.vin.resize(1);
                t.vin[0].prevout.hash = dummyTransactions[0].GetHash();
                t.vin[0].prevout.n = 1;
                t.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
                t.vout.resize(1);
                t.vout[0].nValue = 90*CENT;
                CKey key;
                key.MakeNewKey(true);
                t.vout[0].scriptPubKey = GetScriptForDestination(key.GetPubKey().GetID());

                string reason;
                BOOST_CHECK(IsStandardTx(t, reason));

                t.vout[0].nValue = 5011; // dust
                BOOST_CHECK(!IsStandardTx(t, reason));

                t.vout[0].nValue = 6011; // not dust
                BOOST_CHECK(IsStandardTx(t, reason));

                t.vout[0].scriptPubKey = CScript() << OP_1;
                BOOST_CHECK(!IsStandardTx(t, reason));

                // MAX_OP_RETURN_RELAY-byte TX_NULL_DATA (standard)
                t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
                BOOST_CHECK_EQUAL(MAX_OP_RETURN_RELAY, t.vout[0].scriptPubKey.size());
                BOOST_CHECK(IsStandardTx(t, reason));

                // MAX_OP_RETURN_RELAY+1-byte TX_NULL_DATA (non-standard)
                t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3800");
                BOOST_CHECK_EQUAL(MAX_OP_RETURN_RELAY + 1, t.vout[0].scriptPubKey.size());
                BOOST_CHECK(!IsStandardTx(t, reason));

                // Data payload can be encoded in any way...
                t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("");
                BOOST_CHECK(IsStandardTx(t, reason));
                t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("00") << ParseHex("01");
                BOOST_CHECK(IsStandardTx(t, reason));
                // OP_RESERVED *is* considered to be a PUSHDATA type opcode by IsPushOnly()!
                t.vout[0].scriptPubKey = CScript() << OP_RETURN << OP_RESERVED << -1 << 0 << ParseHex("01") << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16;
                BOOST_CHECK(IsStandardTx(t, reason));
                t.vout[0].scriptPubKey = CScript() << OP_RETURN << 0 << ParseHex("01") << 2 << ParseHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
                BOOST_CHECK(IsStandardTx(t, reason));

                // ...so long as it only contains PUSHDATA's
                t.vout[0].scriptPubKey = CScript() << OP_RETURN << OP_RETURN;
                BOOST_CHECK(!IsStandardTx(t, reason));

                // TX_NULL_DATA w/o PUSHDATA
                t.vout.resize(1);
                t.vout[0].scriptPubKey = CScript() << OP_RETURN;
                BOOST_CHECK(IsStandardTx(t, reason));

                // Only one TX_NULL_DATA permitted in all cases
                t.vout.resize(2);
                t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
                t.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
                BOOST_CHECK(!IsStandardTx(t, reason));

                t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
                t.vout[1].scriptPubKey = CScript() << OP_RETURN;
                BOOST_CHECK(!IsStandardTx(t, reason));

                t.vout[0].scriptPubKey = CScript() << OP_RETURN;
                t.vout[1].scriptPubKey = CScript() << OP_RETURN;
                BOOST_CHECK(!IsStandardTx(t, reason));
        }
BOOST_AUTO_TEST_SUITE_END()
#endif

