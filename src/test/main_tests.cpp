// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2018 The PIVX developers
// Copyright (c) 2018-2020 The DAPS Project developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"
#include "main.h"
#include "test_prcycoin.h"

#include <boost/test/unit_test.hpp>

#ifdef DISABLE_FAILED_TEST
BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

CAmount nMoneySupplyPoWEnd = 43199500 * COIN;

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    CAmount nSum = 0;
    for (int nHeight = 0; nHeight < 1; nHeight += 1) {
        /* premine in block 1 (60,001 PRCY) */
        CAmount nSubsidy = GetBlockValue(nHeight);
        BOOST_CHECK(nSubsidy <= 60001 * COIN);
        nSum += nSubsidy;
    }

    for (int nHeight = 1; nHeight < 86400; nHeight += 1) {
        /* PoW Phase One */
        CAmount nSubsidy = GetBlockValue(nHeight);
        //BOOST_CHECK(nSubsidy <= 250 * COIN);
        nSum += nSubsidy;
    }

    for (int nHeight = 86400; nHeight < 151200; nHeight += 1) {
        /* PoW Phase Two */
        CAmount nSubsidy = GetBlockValue(nHeight);
        //BOOST_CHECK(nSubsidy <= 225 * COIN);
        nSum += nSubsidy;
    }

    for (int nHeight = 151200; nHeight < 259200; nHeight += 1) {
        /* PoW Phase Two */
        CAmount nSubsidy = GetBlockValue(nHeight);
       // BOOST_CHECK(nSubsidy <= 45 * COIN);
        nSum += nSubsidy;
        //BOOST_CHECK(nSum > 0 && nSum <= nMoneySupplyPoWEnd);
    }
    //BOOST_CHECK(nSum == 4109975100000000ULL);
    //
}

bool ReturnFalse() { return false; }
bool ReturnTrue() { return true; }

BOOST_AUTO_TEST_CASE(test_combiner_all)
{
    boost::signals2::signal<bool(), CombinerAll> Test;
    BOOST_CHECK(Test());
    Test.connect(&ReturnFalse);
    BOOST_CHECK(!Test());
    Test.connect(&ReturnTrue);
    BOOST_CHECK(!Test());
    Test.disconnect(&ReturnFalse);
    BOOST_CHECK(Test());
    Test.disconnect(&ReturnTrue);
    BOOST_CHECK(Test());
}

BOOST_AUTO_TEST_SUITE_END()
#endif
