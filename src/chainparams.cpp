// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2018 The PIVX developers
// Copyright (c) 2018-2020 The DAPS Project developers
// Copyright (c) 2020-2021 The PRCY developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "chainparamsseeds.h"
#include "util.h"
#include "utilstrencodings.h"

#include <boost/assign/list_of.hpp>

#include <assert.h>


std::string CDNSSeedData::getHost(uint64_t requiredServiceBits) const {
    //use default host for non-filter-capable seeds or if we use the default service bits (NODE_NETWORK)
    if (!supportsServiceBitsFiltering || requiredServiceBits == NODE_NETWORK)
        return host;

    return strprintf("x%x.%s", requiredServiceBits, host);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */
static Checkpoints::MapCheckpoints mapCheckpoints =
    boost::assign::map_list_of
    (0, uint256("000006957e238ff4e6bcf00c8a7d1b3e7249c0a2109b0391d8740821a40c1d8c"))
    (500, uint256("00214da9b906c7c14558395b9bc88293301e6e5f87a714194079dd651293fadb")) // Final POW Block
    (561, uint256("54872c72e81b34117bc5a6095d6f1b8d85746992d2513d7adc90a2aceed1651e")) // First PoA Block
    (562, uint256("0f00d3a6636c8a265724764da082fdef8106fce7057dfdda94ab6537f7211b4f")) // First Block after PoA
    (14905, uint256("6389ecdb851500d9467b41a54d02c58b1542bfc2d5c99339821c89d25135a4b0")) // Chain split
    (17128, uint256("98d76615ef96b3ced1d9902715ba432393335b791b2256936883c323f0bb91f4"))
    (17133, uint256("d9dcec83e8a675db0f7b6d28fde591f494a1b0766f7cb56ea8d8cb95348f835b"))
    (17150, uint256("bad4dccf8fd86f00d6c215802d39342e4de64e21155b76b38f0182ba7d96edd2"))
    (17153, uint256("4e63d92ac5209f0a342c2e74098778bbe36324de66734ff41132c7f3db3ad628"))
    (18375, uint256("c291cf0a7bcd73a91c6f68d28dc6ce9e1acee973fd134f4250c1ee80bf3d0c03"))
    (18813, uint256("ccf401e6fb24f34c105b2f693c2eb75c2ba7a2a4e3ffbbfe152e98ae9806d418"))
    (18814, uint256("f8092b5c474cc260edb7c378c4f7e2123b007a55018d32f97b9b729f729067ae"))
    (19317, uint256("0bd530827eff9bdc79893739c02c14c02bb35a39b943eaeeb72e04767e0597a5"))    
    (19400, uint256("cdeebfe4fdeda461ab2025d08248e89b13a1e970c76b91ac2788837c147e6f33"))
    (77852, uint256("e6ec3ddccfb6bd378ca3cf5cc292c011ec235536a9edbb8c3fdbe17abd921d44"))
    (98715, uint256("04b3cfde139af89ddacf3cb790daf734e4953119c4a58da9b8f07be888271670"))
    (98768, uint256("43f42aaba4a1e8f7a4981b7d8620e5967974b0bbdee4ae22e42f14585be3a52f"))
    (105629, uint256("82ae47c00a0338f33a41b8505162cabaa4740f2187feb07f4ee4bc6138461acb"))
    (129267, uint256("773de63a6ef8cd4d56769a0efc8625fa617bbac34802019b3a211bbe390f34a6"))
    (132020, uint256("d93d3f4b85dd3f3995d010966f8e0163f324bfe3e748507fadf472a14c76ce36"))
    (133535, uint256("33d415384dc2d181f9cd0208d6c8664dfdbfb95a061a639220b4ea253df7788c"))
    (140352, uint256("cdef2002ee6d10a0a8e85ba47329455773245e6008aa691416b63d7ec3aef78d"))
    (155115, uint256("ea78ac399244d06b407ff349ba71747d8c672ccd54216317abc28dbca04c71e5")) // A PoA block was rejected here, avoid it
    (155116, uint256("929d16db920af3df60cf2e869ee08d174f7d476d65e53cbf07d54b7d1cca2380")) // First PoA Block after fix/difficulty bump
    (193949, uint256("98ed9238e67297071a13b6e62fa17c5c992998a295ea7535cdcd4c3dda8aeab3")) // First PoA Block after fix/difficulty bump
    (260162, uint256("97d593c9ebbcc219eeed822f05ba4291e7dcc4c3667836dbfe29667fe31808a7"))
    (369757, uint256("445cc1e7abeca5bd13669704241efb9c045bf414d80c5173c7f80018381ba5a3"))
    (370034, uint256("2c98b9b6fc800b0ba9836669c1474a95d1e4afd2a91edbec25d3bc05636deab3"))
    ;
static const Checkpoints::CCheckpointData data = {
    &mapCheckpoints,
    1629658989,          // * UNIX timestamp of last checkpoint block
    779662,          // * total number of transactions between genesis and last checkpoint
                //   (the tx=... number in the SetBestChain debug.log lines)
    1440        // * estimated number of transactions per day after checkpoint
};

static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
    boost::assign::map_list_of(0, uint256("000001488be8bb442cd72cb737ade49a31de90dbbe5dce36f7d7e07f5dde2b77"));
static const Checkpoints::CCheckpointData dataTestnet = {
    &mapCheckpointsTestnet,
    0,
    0,
    0};

static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
    boost::assign::map_list_of(0, uint256("690cbb5c7ae999de1de49948a3c109d3b15fe4de4297980de8ff0cbfe3c7823a"));
static const Checkpoints::CCheckpointData dataRegtest = {
    &mapCheckpointsRegtest,
    0,
    0,
    0};

class CMainParams : public CChainParams
{
public:
    CMainParams()
    {
        networkID = CBaseChainParams::MAIN;
        strNetworkID = "main";
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */
        pchMessageStart[0] = 0xf8;
        pchMessageStart[1] = 0x8e;
        pchMessageStart[2] = 0xa6;
        pchMessageStart[3] = 0x90;
        nDefaultPort = 59682;
        bnProofOfWorkLimit = ~uint256(0) >> 1; // PRCYcoin starting difficulty is 1 / 2^12
        nSubsidyHalvingInterval = 210000;
        nMaxReorganizationDepth = 100;
        nEnforceBlockUpgradeMajority = 8100; // 75%
        nRejectBlockOutdatedMajority = 10260; // 95%
        nToCheckBlockUpgradeMajority = 10800; // Approximate expected amount of blocks in 7 days (1440*7.5)
        nMinerThreads = 0;
        nTargetTimespan = 1 * 60; // PRCYcoin: 1 day
        nTargetSpacing = 1 * 60;  // PRCYcoin: 1 minute
        nMaturity = 100;
        nMasternodeCountDrift = 20;
        MAX_MONEY = 70000000.0;
        nMaxMoneyOut = MAX_MONEY * COIN;
        nMNCollateralAmt = 5000 * COIN;
        nMinimumStakeAmount = 2500 * COIN;

        /** Height or Time Based Activations **/
        nLastPOWBlock = 500;
        nStartPOABlock = 560;
        nModifierUpdateBlock = 615800;
        nPoABlockTime = 60 * 60; //a PoA block every 60 minutes
        nMinNumPoSBlocks = 59;
        nMaxNumPoSBlocks = 65;
        nSoftForkBlock = 120000; // Soft fork block for difficulty change
        nPoANewDiff = 150000; // New PoA difficulty
        nBIP65ActivationHeight = 125000; // Last v3 block was 124712, leave a bit of padding
        nPoAFixTime = 1616716800; // Fork time for PoA fix - Friday, March 26, 2021 12:00:00 AM (GMT)
        nPoAPaddingBlock = 169869; // Last block with 120 PoS blocks in a PoA Audit
        nPoAPadding = 10; // Current PoA Padding
        nHardForkBlock = 375000; // Add hard fork block for Consensus/PoA Padding

        /**
         * Build the genesis block. Note that the output of the genesis coinbase cannot
         * be spent as it did not originally exist in the database.
         *
         * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
         *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
         *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
         *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
         *   vMerkleTree: e0028e
         */
        const char* pszTimestamp = "12 January 2021 PRCY Coin Development Team - Privacy is Your Right";
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 0 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("04b78f63269234b741668d85b57ba11edec2ee20f15719db180d5d6a37c4e9db0c494390fb54925934bc7b29f148a372c00273bbd5c939830d7d2941de6ce44b8b") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime = 1610409600; // 1/12/2021 @ 12:00am (GMT)
        genesis.nBits = 0x1e0ffff0;
        genesis.nNonce = 28141687;

        //change blockhash from 0x00000e9468bba3df97334bfb5015543488a0fb38193c82b101472937590e3037 because of transaction structure change
        if(genesis.GetHash()!=uint256("000006957e238ff4e6bcf00c8a7d1b3e7249c0a2109b0391d8740821a40c1d8c"))
        {
            printf("Searchingforgenesisblock...\n");
            uint256 hashTarget=uint256().SetCompact(genesis.nBits);

            printf("hashTarget:%s\n",hashTarget.ToString().c_str());

            while(uint256(genesis.GetHash())>hashTarget)
            {
                printf("loop:%s\n",genesis.GetHash().ToString().c_str());
                ++genesis.nNonce;
                if(genesis.nNonce==0)
                {
                    printf("NONCEWRAPPED,incrementingtime");
                    std::cout<<std::string("NONCEWRAPPED,incrementingtime:\n");
                    ++genesis.nTime;
                }
                if(genesis.nNonce%10000==0)
                {
                    printf("Mainnet:nonce%08u:hash=%s\n",genesis.nNonce,genesis.GetHash().ToString().c_str());
                }
            }
            printf("block.nTime=%u\n",genesis.nTime);
            printf("block.nNonce=%u\n",genesis.nNonce);
            printf("block.GetHash=%s\n",genesis.GetHash().ToString().c_str());
            printf("hashMerkleRoot=%s\n",genesis.hashMerkleRoot.ToString().c_str());

        }



        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("000006957e238ff4e6bcf00c8a7d1b3e7249c0a2109b0391d8740821a40c1d8c"));
        assert(genesis.hashMerkleRoot == uint256("cd01f1ca20c22b336f1ee83af9fd8b7facbf42083bf3bed49af045f5cadc9cd4"));

        // nodes with support for servicebits filtering should be at the top
        vSeeds.push_back(CDNSSeedData("seed.prcycoin.com", "seed.prcycoin.com"));          // Single node address
        vSeeds.push_back(CDNSSeedData("seed1.prcycoin.com", "seed1.prcycoin.com"));        // Single node address
        vSeeds.push_back(CDNSSeedData("seed2.prcycoin.com", "seed2.prcycoin.com"));        // Single node address
        vSeeds.push_back(CDNSSeedData("seed3.prcycoin.com", "seed3.prcycoin.com"));        // Single node address
        vSeeds.push_back(CDNSSeedData("seed4.prcycoin.com", "seed4.prcycoin.com"));        // Single node address
        vSeeds.push_back(CDNSSeedData("seed5.prcycoin.com", "seed5.prcycoin.com"));        // Single node address
        vSeeds.push_back(CDNSSeedData("seed6.prcycoin.com", "seed6.prcycoin.com"));        // Single node address
        vSeeds.push_back(CDNSSeedData("seed7.prcycoin.com", "seed7.prcycoin.com"));        // Single node address
        vSeeds.push_back(CDNSSeedData("seed8.prcycoin.com", "seed8.prcycoin.com"));        // Single node address
        vSeeds.push_back(CDNSSeedData("seed9.prcycoin.com", "seed9.prcycoin.com"));        // Single node address
        vSeeds.push_back(CDNSSeedData("vps.prcycoin.com", "vps.prcycoin.com"));            // Single node address
        vSeeds.push_back(CDNSSeedData("vps1.prcycoin.com", "vps1.prcycoin.com"));          // Single node address
        vSeeds.push_back(CDNSSeedData("vps2.prcycoin.com", "vps2.prcycoin.com"));          // Single node address
        vSeeds.push_back(CDNSSeedData("vps3.prcycoin.com", "vps3.prcycoin.com"));          // Single node address
        vSeeds.push_back(CDNSSeedData("vps4.prcycoin.com", "vps4.prcycoin.com"));          // Single node address
        vSeeds.push_back(CDNSSeedData("vps5.prcycoin.com", "vps5.prcycoin.com"));          // Single node address
        vSeeds.push_back(CDNSSeedData("vps6.prcycoin.com", "vps6.prcycoin.com"));          // Single node address
        vSeeds.push_back(CDNSSeedData("vps7.prcycoin.com", "vps7.prcycoin.com"));          // Single node address
        vSeeds.push_back(CDNSSeedData("vps8.prcycoin.com", "vps8.prcycoin.com"));          // Single node address
        vSeeds.push_back(CDNSSeedData("vps9.prcycoin.com", "vps9.prcycoin.com"));          // Single node address

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 55);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 61);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 28);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x57)(0x4D)(0x12)(0x23).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x57)(0x41)(0x71)(0x65).convert_to_container<std::vector<unsigned char> >();
        //  BIP44 coin type is from https://github.com/satoshilabs/slips/blob/master/slip-0044.md
        nExtCoinType = 0x80000355;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fSkipProofOfWorkCheck = false;
        fTestnetToBeDeprecatedFieldRPC = false;
        fHeadersFirstSyncingActive = false;

        nPoolMaxTransactions = 3;
        strObfuscationPoolDummyAddress = "D87q2gC9j6nNrnzCsg4aY6bHMLsT9nUhEw";
        nStartMasternodePayments = 1546809115; //Wed, 25 Jun 2014 20:36:16 GMT
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return data;
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams
{
public:
    CTestNetParams()
    {
        networkID = CBaseChainParams::TESTNET;
        strNetworkID = "test";
        pchMessageStart[0] = 0xc1;
        pchMessageStart[1] = 0xaa;
        pchMessageStart[2] = 0xb2;
        pchMessageStart[3] = 0xe9;
        nDefaultPort = 59684;
        nEnforceBlockUpgradeMajority = 4320; // 75%
        nRejectBlockOutdatedMajority = 5472; // 95%
        nToCheckBlockUpgradeMajority = 5760; // 4 days
        nMinerThreads = 0;
        nTargetTimespan = 1 * 60; // PRCYcoin: 1 day
        nTargetSpacing = 1 * 60;  // PRCYcoin: 1 minute
        nLastPOWBlock = 500;
        nStartPOABlock = 560;
        nPoABlockTime = 30 * 60;  //1 PoA block every 30 minutes
        nMinNumPoSBlocks = 29;
        nMaxNumPoSBlocks = 33;
        nMaturity = 15;
        nMasternodeCountDrift = 4;
        nModifierUpdateBlock = 51197; //approx Mon, 17 Apr 2017 04:00:00 GMT
        MAX_MONEY = 70000000.0;
        nMaxMoneyOut = MAX_MONEY * COIN;
        nMNCollateralAmt = 5000 * COIN;
        nSoftForkBlock = 600; // Soft fork block for difficulty change - testnet started with it
        nPoANewDiff = 650;
        nBIP65ActivationHeight = 0;
        nPoAFixTime = 1616277580; // Fork time for PoA fix - Saturday, March 20, 2021 22:00:00 AM (GMT)
        nPoAPaddingBlock = 0;
        nPoAPadding = 5; // Current PoA Padding
        nHardForkBlock = 700; // Add hard fork block for Consensus/PoA Padding

        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1608422400;
        genesis.nNonce = 23323155;

        if(genesis.GetHash()!=uint256("000001488be8bb442cd72cb737ade49a31de90dbbe5dce36f7d7e07f5dde2b77"))
        {
            printf("Searchingforgenesisblock...\n");
            uint256 hashTarget=uint256().SetCompact(genesis.nBits);

            printf("hashTarget:%s\n",hashTarget.ToString().c_str());

            while(uint256(genesis.GetHash())>hashTarget)
            {
                printf("loop:%s\n",genesis.GetHash().ToString().c_str());
                ++genesis.nNonce;
                if(genesis.nNonce==0)
                {
                    printf("NONCEWRAPPED,incrementingtime");
                    std::cout<<std::string("NONCEWRAPPED,incrementingtime:\n");
                    ++genesis.nTime;
                }
                if(genesis.nNonce%10000==0)
                {
                    printf("Mainnet:nonce%08u:hash=%s\n",genesis.nNonce,genesis.GetHash().ToString().c_str());
                }
            }
            printf("block.nTime=%u\n",genesis.nTime);
            printf("block.nNonce=%u\n",genesis.nNonce);
            printf("block.GetHash=%s\n",genesis.GetHash().ToString().c_str());
            printf("hashMerkleRoot=%s\n",genesis.hashMerkleRoot.ToString().c_str());

        }

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("000001488be8bb442cd72cb737ade49a31de90dbbe5dce36f7d7e07f5dde2b77"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.push_back(CDNSSeedData("testnet.prcycoin-seeds.com", "testnet.prcycoin-seeds.com"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 139); // Testnet prcycoin addresses start with 'x' or 'y'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 19);  // Testnet prcycoin script addresses start with '8' or '9'
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);     // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        // Testnet prcycoin BIP32 pubkeys start with 'DRKV'
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x3a)(0x80)(0x61)(0xa0).convert_to_container<std::vector<unsigned char> >();
        // Testnet prcycoin BIP32 prvkeys start with 'DRKP'
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x3a)(0x80)(0x58)(0x37).convert_to_container<std::vector<unsigned char> >();
        // Testnet prcycoin BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 0x80000001;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 2;
        strObfuscationPoolDummyAddress = "y57cqfGRkekRyDRNeJiLtYVEbvhXrNbmox";
        nStartMasternodePayments = 1420837558; //Fri, 09 Jan 2015 21:05:58 GMT
        nBudget_Fee_Confirmations = 3; // Number of confirmations for the finalization fee. We have to make this very short
                                       // here because we only have a 8 block finalization window on testnet
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataTestnet;
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CTestNetParams
{
public:
    CRegTestParams()
    {
        networkID = CBaseChainParams::REGTEST;
        strNetworkID = "regtest";
        strNetworkID = "regtest";
        pchMessageStart[0] = 0xc6;
        pchMessageStart[1] = 0xb3;
        pchMessageStart[2] = 0x97;
        pchMessageStart[3] = 0xd1;
        nSubsidyHalvingInterval = 150;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 1;
        nTargetTimespan = 24 * 60 * 60; // Prcycoin: 1 day
        nTargetSpacing = 1 * 60;        // Prcycoin: 1 minutes
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        genesis.nTime = 1608422399;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 12361;

        if(genesis.GetHash()!=uint256("690cbb5c7ae999de1de49948a3c109d3b15fe4de4297980de8ff0cbfe3c7823a"))
        {
            printf("Searchingforgenesisblock...\n");
            uint256 hashTarget=uint256().SetCompact(genesis.nBits);

            printf("hashTarget:%s\n",hashTarget.ToString().c_str());

            while(uint256(genesis.GetHash())>hashTarget)
            {
                printf("loop:%s\n",genesis.GetHash().ToString().c_str());
                ++genesis.nNonce;
                if(genesis.nNonce==0)
                {
                    printf("NONCEWRAPPED,incrementingtime");
                    std::cout<<std::string("NONCEWRAPPED,incrementingtime:\n");
                    ++genesis.nTime;
                }
                if(genesis.nNonce%10000==0)
                {
                    printf("Mainnet:nonce%08u:hash=%s\n",genesis.nNonce,genesis.GetHash().ToString().c_str());
                }
            }
            printf("block.nTime=%u\n",genesis.nTime);
            printf("block.nNonce=%u\n",genesis.nNonce);
            printf("block.GetHash=%s\n",genesis.GetHash().ToString().c_str());
            printf("hashMerkleRoot=%s\n",genesis.hashMerkleRoot.ToString().c_str());

        }

        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 51476;

        assert(hashGenesisBlock == uint256("690cbb5c7ae999de1de49948a3c109d3b15fe4de4297980de8ff0cbfe3c7823a"));

        vFixedSeeds.clear(); //! Testnet mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Testnet mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataRegtest;
    }
};
static CRegTestParams regTestParams;

static CChainParams* pCurrentParams = 0;

const CChainParams& Params()
{
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(CBaseChainParams::Network network)
{
    switch (network) {
    case CBaseChainParams::MAIN:
        return mainParams;
    case CBaseChainParams::TESTNET:
        return testNetParams;
    case CBaseChainParams::REGTEST:
        return regTestParams;
    default:
        assert(false && "Unimplemented network");
        return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);
    return true;
}
