// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2015-2018 The PIVX developers
// Copyright (c) 2018-2020 The DAPS Project developers
// Copyright (c) 2020-2021 The PRCY developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "poa.h"

#include "chain.h"
#include "chainparams.h"
#include "main.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

#include <math.h>

unsigned int N_BITS = 0x1e050000;
unsigned int N_BITS_SF = 0x1e127ff8; // Params().SoftFork()
unsigned int N_BITS_PD = 0x1e02b2dc; // Params().PoANewDiff()

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock)
{
    if (N_BITS != 0 && pblock->IsPoABlockByVersion()) {
        if (pindexLast->nHeight < Params().SoftFork()) {
            LogPrint("poa", "%s: returning N_BITS\n", __func__);
            return N_BITS;
        }
        if (pindexLast->nHeight < Params().PoANewDiff()) {
            LogPrint("poa", "%s: returning N_BITS_SF\n", __func__);
            return N_BITS_SF;
        }
        LogPrint("poa", "%s: returning N_BITS_PD\n", __func__);
        return N_BITS_PD;
    }
    /* current difficulty formula, prcycoin - DarkGravity v3, written by Evan Duffield - evan@dashpay.io */
    const CBlockIndex* BlockLastSolved = pindexLast;
    const CBlockIndex* BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 24;
    int64_t PastBlocksMax = 24;
    int64_t CountBlocks = 0;
    uint256 PastDifficultyAverage;
    uint256 PastDifficultyAveragePrev;

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) {
        return Params().ProofOfWorkLimit().GetCompact();
    }

    if (pindexLast->nHeight > Params().LAST_POW_BLOCK()) {
        uint256 bnTargetLimit = (~uint256(0) >> 24);
        int64_t nTargetSpacing = 60;
        int64_t nTargetTimespan = 60 * 40;

        //finding last PoS block
        CBlockIndex* pLastPoS = pindexLast->pprev;
        while (!pLastPoS->IsProofOfStake() && pLastPoS->nHeight > Params().LAST_POW_BLOCK()) {
            pLastPoS = pLastPoS->pprev;
        }
        int64_t nActualSpacing = 0;
        //ig
        if (pindexLast->nHeight != 0)
            nActualSpacing = pindexLast->GetBlockTime() - pLastPoS->GetBlockTime();

        if (nActualSpacing < 0)
            nActualSpacing = 1;

        // ppcoin: target change every block
        // ppcoin: retarget with exponential moving toward target spacing
        uint256 bnNew;
        if (pindexLast->nHeight < Params().SoftFork()) {
            bnNew.SetCompact(pindexLast->nBits);
        } else {
            if (pindexLast->IsProofOfStake()) {
                bnNew.SetCompact(pindexLast->nBits);
            } else {
                bnNew.SetCompact(pLastPoS->nBits);
            }
        }

        int64_t nInterval = nTargetTimespan / nTargetSpacing;
        bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
        bnNew /= ((nInterval + 1) * nTargetSpacing);

        if (bnNew <= 0 || bnNew > bnTargetLimit)
            bnNew = bnTargetLimit;

        return bnNew.GetCompact();
    }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) {
            break;
        }
        CountBlocks++;

        if (CountBlocks <= PastBlocksMin) {
            if (CountBlocks == 1) {
                PastDifficultyAverage.SetCompact(BlockReading->nBits);
            } else {
                PastDifficultyAverage = ((PastDifficultyAveragePrev * CountBlocks) + (uint256().SetCompact(BlockReading->nBits))) / (CountBlocks + 1);
            }
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if (LastBlockTime > 0) {
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            nActualTimespan += Diff;
        }
        LastBlockTime = BlockReading->GetBlockTime();

        if (BlockReading->pprev == NULL) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    uint256 bnNew(PastDifficultyAverage);

    int64_t _nTargetTimespan = CountBlocks * Params().TargetSpacing();

    if (nActualTimespan < _nTargetTimespan / 3)
        nActualTimespan = _nTargetTimespan / 3;
    if (nActualTimespan > _nTargetTimespan * 3)
        nActualTimespan = _nTargetTimespan * 3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= _nTargetTimespan;

    if (bnNew > Params().ProofOfWorkLimit()) {
        bnNew = Params().ProofOfWorkLimit();
    }

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    uint256 bnTarget;

    if (Params().SkipProofOfWorkCheck())
        return true;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > Params().ProofOfWorkLimit())
        return error("CheckProofOfWork(): nBits below minimum work");

    // Check proof of work matches claimed amount
    if (hash > bnTarget)
        return error("CheckProofOfWork(): hash doesn't match nBits");

    return true;
}

uint256 GetBlockProof(const CBlockIndex& block)
{
    uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}

CBlockIndex* FindPrevPoSBlock(CBlockIndex* p) {
    if (!p || !p->pprev) return NULL;
    return p->pprev->IsProofOfStake()?p->pprev:FindPrevPoSBlock(p->pprev);
}

//If blockheight = -1, the to-be-checked block is not included yet in the chain, otherwise, that is the height of the poa block
bool CheckPoAContainRecentHash(const CBlock& block)
{
    int nHeight;
    //block.Merkle
    CBlockIndex* currentTip = mapBlockIndex[block.hashPrevBlock];
    if (!currentTip) {
        return error("CheckPoAContainRecentHash(): Previous block not found");
    }
    //Find the previous PoA block
    CBlockIndex* pindex = currentTip;
    nHeight = pindex->nHeight;
    while (pindex->nHeight >= Params().START_POA_BLOCK()) {
        if (pindex->GetBlockHeader().IsPoABlockByVersion()) {
            break;
        }
        pindex = pindex->pprev;
    }
    bool ret = true;
    if (pindex->nHeight <= Params().START_POA_BLOCK()) {
        //this is the first PoA block ==> check all PoS blocks from LAST_POW_BLOCK up to currentHeight - POA_BLOCK_PERIOD - 1 inclusive
        int index = 0;
        for (size_t i = Params().LAST_POW_BLOCK() + 1; i <= Params().LAST_POW_BLOCK() + block.posBlocksAudited.size(); i++) {
            PoSBlockSummary pos = block.posBlocksAudited.at(index);
            CBlockIndex* pidxInChain = mapBlockIndex[pos.hash];
            if (!pidxInChain) {
                return error("CheckPoAContainRecentHash(): Audited blocks not found");
            }
            if (pos.hash != pidxInChain->GetBlockHash() || pos.nTime != pidxInChain->nTime || pos.height != (uint32_t)pidxInChain->nHeight) {
                ret = false;
                break;
            }
            CBlockIndex* p = mapBlockIndex[pos.hash];
            bool auditResult = ReVerifyPoSBlock(p);
            if (!auditResult) {
                if (pos.nTime) {
                    ret = false;
                    break;
                }
            }
            index++;
        }
    } else {
        if (pindex->nHeight >= Params().START_POA_BLOCK()) {
            // Bypass bad block         
            if (pindex->nHeight == 17077 || pindex->nHeight == 17154 || pindex->nHeight == 135887 || pindex->nHeight == 311272) {
                return true;
            }
            CBlock prevPoablock;
            CBlockIndex* pblockindex = pindex;
            if (!ReadBlockFromDisk(prevPoablock, pblockindex))
                throw runtime_error("Can't read block from disk");
            PoSBlockSummary lastAuditedPoSBlockInfo = prevPoablock.posBlocksAudited.back();
            uint256 lastAuditedPoSHash = lastAuditedPoSBlockInfo.hash;
            if (mapBlockIndex.count(lastAuditedPoSHash) < 1 && !IsWrongAudit(lastAuditedPoSHash.GetHex(), nHeight)) {
                return error("CheckPoAContainRecentHash(): Audited blocks not found");
            }

            uint256 currentFirstPoSAuditedHash = block.posBlocksAudited[0].hash;
            uint256 currentLastPoSAuditedHash = block.posBlocksAudited.back().hash;
            if (mapBlockIndex.count(currentFirstPoSAuditedHash) < 1 || mapBlockIndex.count(currentLastPoSAuditedHash) < 1) {
                return error("CheckPoAContainRecentHash(): Being audited blocks not found");
            }
            CBlockIndex* pCurrentFirstPoSAuditedIndex = mapBlockIndex[currentFirstPoSAuditedHash];
            CBlockIndex* pCurrentLastPoSAuditedIndex = mapBlockIndex[currentLastPoSAuditedHash];
            uint256 fixedPoSAuditedHash = pCurrentFirstPoSAuditedIndex->GetAncestor(lastAuditedPoSBlockInfo.height)->GetBlockHash();
            //check lastAuditedPoSHash and currentFirstPoSAuditedHash must be on the same fork
            //that lastAuditedPoSHash must be parent block of currentFirstPoSAuditedHash
            if (pCurrentFirstPoSAuditedIndex->GetAncestor(lastAuditedPoSBlockInfo.height)->GetBlockHash() != lastAuditedPoSHash && !IsFixedAudit(fixedPoSAuditedHash.GetHex(), nHeight)) {
                return error("CheckPoAContainRecentHash(): PoA block is not on the same fork with the previous poa block");
            }

            //check there is no pos block between lastAuditedPoSHash and currentFirstPoSAuditedHash
            CBlockIndex* pIndexLoop = pCurrentFirstPoSAuditedIndex->pprev;
            while(pIndexLoop && !pIndexLoop->IsProofOfStake()) {
                pIndexLoop = pIndexLoop->pprev;
            }
            if (!pIndexLoop || (pIndexLoop->GetBlockHash() != lastAuditedPoSHash && !IsFixedAudit(fixedPoSAuditedHash.GetHex(), nHeight))) {
                return error("CheckPoAContainRecentHash(): Some PoS block between %s and %s is not audited\n", lastAuditedPoSHash.GetHex(), currentFirstPoSAuditedHash.GetHex());
            }

            //alright, check all pos blocks audited in the block is conseutive in the chain
            for(size_t i = block.posBlocksAudited.size() - 1; i > 0; i--) {
                uint256 thisPoSAduditedHash = block.posBlocksAudited[i].hash;
                if (mapBlockIndex.count(thisPoSAduditedHash) < 1) {
                    return error("CheckPoAContainRecentHash(): PoS block %s not found\n", thisPoSAduditedHash.GetHex());
                }
                CBlockIndex* thisPoSAuditedIndex = mapBlockIndex[thisPoSAduditedHash];
                CBlockIndex* previousPoSIndex = FindPrevPoSBlock(thisPoSAuditedIndex);
                if (!previousPoSIndex) {
                    return error("CheckPoAContainRecentHash(): Failed to find previous PoS block for block %s\n", thisPoSAduditedHash.GetHex());
                }
                PoSBlockSummary previousSummary = block.posBlocksAudited[i - 1];
                if (previousPoSIndex->GetBlockHash() != previousSummary.hash ||
                    (uint32_t)previousPoSIndex->nHeight != previousSummary.height ||
                    previousPoSIndex->GetBlockTime() != previousSummary.nTime) {
                    return error("CheckPoAContainRecentHash(): PoS block info not matched for %s\n", thisPoSAduditedHash.GetHex());
                }
                bool auditResult = ReVerifyPoSBlock(thisPoSAuditedIndex);
                if (!auditResult) {
                    if (previousSummary.nTime) {
                        ret = false;
                        LogPrintf("%s: Failed to reverify block %s\n", __func__, previousSummary.hash.GetHex());
                        break;
                    }
                }
            }
            if (ret) {
                bool auditResult = ReVerifyPoSBlock(pCurrentFirstPoSAuditedIndex);
                if (!auditResult) {
                    if (block.posBlocksAudited[0].nTime) {
                        ret = false;
                        LogPrintf("%s: Failed to reverify block %s\n", __func__, block.posBlocksAudited[0].hash.GetHex());
                    }
                }
            }
        } else {
            ret = block.hashPrevPoABlock.IsNull();
        }
    }
    return ret;
}

bool CheckNumberOfAuditedPoSBlocks(const CBlock& block, const CBlockIndex* pindex)
{
    bool ret = true;

    if (pindex->nHeight > Params().PoAPaddingBlock()){
        if (block.posBlocksAudited.size() < (size_t)Params().MIN_NUM_POS_BLOCKS_AUDITED() || block.posBlocksAudited.size() > (size_t)Params().MAX_NUM_POS_BLOCKS_AUDITED()) {
            ret = false;
        }
    } else {
        if (block.posBlocksAudited.size() < (size_t)Params().MIN_NUM_POS_BLOCKS_AUDITED() || block.posBlocksAudited.size() > 120 ) {
            ret = false;
        }
    }
    return ret;
}

//Check whether the block is successfully mined and the mined hash satisfy the difficulty
bool CheckPoABlockMinedHash(const CBlockHeader& block)
{
    const uint256 minedHash = block.minedHash; //block.ComputeMinedHash();
    if (minedHash == block.minedHash) {
        //Check minedHash satisfy difficulty based on nbits
        bool fNegative;
        bool fOverflow;
        uint256 bnTarget;

        //As of now, there is no PoA miner, this will let all emulated PoA blocks bypass the check
        if (Params().SkipProofOfWorkCheck() || Params().NetworkID() == CBaseChainParams::TESTNET)
            return true;

        bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
        LogPrintf("Target: %s, minedHash: %s\n", bnTarget.GetHex(), minedHash.GetHex());

        // Check proof of work matches claimed amount
        if (minedHash > bnTarget) {
            LogPrintf("Block mined hash not satisfied\n");
            return error("CheckPoABlockMinedHash(): hash doesn't match nBits");
        }

        return true;
    }
    return false;
}

//A PoA block should contains previous PoA block hash
bool CheckPrevPoABlockHash(const CBlockHeader& block)
{
    CBlockIndex* currentTip = mapBlockIndex[block.hashPrevBlock];
    if (!currentTip) {
        return error("CheckPrevPoABlockHash(): Previous block not found");
    }
    //Find the previous PoA block
    CBlockIndex* pindex = currentTip;
    while (pindex->nHeight > Params().START_POA_BLOCK()) {
        if (pindex->GetBlockHeader().IsPoABlockByVersion()) {
            break;
        }
        pindex = pindex->pprev;
    }
    bool ret = false;

    if (pindex->nHeight > Params().START_POA_BLOCK()) {
        CBlockHeader header = pindex->GetBlockHeader();
        uint256 poaBlockHash = header.GetHash();
        if (poaBlockHash == block.hashPrevPoABlock) {
            ret = true;
        }
    } else {
        //This is the first poa block ==> previous poa hash = 0
        ret = block.hashPrevPoABlock.IsNull();
    }

    return ret;
}

//Check whether the poa merkle root is correctly computed
bool CheckPoAMerkleRoot(const CBlock& block, bool* fMutate)
{
    uint256 expected = block.BuildPoAMerkleTree(fMutate);
    if (expected == block.hashPoAMerkleRoot) {
        return true;
    }
    return false;
}

//A PoA block cannot contain information of any PoA block information (hash, height, timestamp)
bool CheckPoABlockNotContainingPoABlockInfo(const CBlock& block, const CBlockIndex* pindex)
{
    // Bypass bad block
    if (pindex->nHeight == 17154 || pindex->nHeight == 135948 || pindex->nHeight == 311332) {
        return true;
    } 
    uint32_t numOfPoSBlocks = block.posBlocksAudited.size();
    for (uint32_t i = 0; i < numOfPoSBlocks; i++) {
        PoSBlockSummary pos = block.posBlocksAudited.at(i);
        uint256 hash = pos.hash;
        if (mapBlockIndex.count(hash) == 0) {
            return false;
        }
        CBlockIndex* pblockindex = mapBlockIndex[hash];
        CBlockHeader header = pblockindex->GetBlockHeader();
        if (header.IsPoABlockByVersion()) {
            return false;
        }
    }
    return true;
}

bool CheckPoAblockTime(const CBlock& block)
{
    bool ret = false;

    if (block.hashPrevPoABlock.IsNull()) {
        ret = true;
    } else {
        LogPrint("poa", "%s: Previous PoA block hash %s\n", __func__, block.hashPrevPoABlock.GetHex());
        if (mapBlockIndex.count(block.hashPrevPoABlock) != 0) {
            CBlockIndex* pindex = mapBlockIndex[block.hashPrevPoABlock];
            uint32_t prevPoATime = pindex->nTime;
            if (block.nTime > prevPoATime && (block.nTime - pindex->nTime >= (uint32_t)Params().POA_BLOCK_TIME())) {
                ret = true;
            }
            LogPrint("poa", "%s: PoA Block time: %d, Previous: %d, Current: %d, Distance: %d\n", __func__,
                Params().POA_BLOCK_TIME(), prevPoATime, block.nTime, block.nTime - pindex->nTime);
        } else {
            LogPrint("poa", "%s: Cannot find block hash %s\n", __func__, block.hashPrevPoABlock.GetHex());
        }
    }
    return ret;
}

bool CheckPoABlockNotAuditingOverlap(const CBlock& block)
{
    bool ret = false;

    if (block.hashPrevPoABlock.IsNull()) {
        //First PoA block
        LogPrint("poa", "%s: First PoA Block Hash: %s\n", __func__, block.GetHash().GetHex());
        ret = true;
    } else {
        if (mapBlockIndex.count(block.hashPrevPoABlock) != 0) {
            CBlockIndex* pPrevPoAIndex = mapBlockIndex[block.hashPrevPoABlock];
            CBlock prevPoablock;
            if (!ReadBlockFromDisk(prevPoablock, pPrevPoAIndex))
                throw runtime_error("Can't read block from disk");
            ret = true;
            for (size_t i = 0; i < block.posBlocksAudited.size(); i++) {
                bool isAlreadyAudited = false;
                for (size_t j = 0; j < prevPoablock.posBlocksAudited.size(); j++) {
                    if (prevPoablock.posBlocksAudited[j].hash == block.posBlocksAudited[i].hash && prevPoablock.posBlocksAudited[j].nTime == block.posBlocksAudited[i].nTime && prevPoablock.posBlocksAudited[j].height == block.posBlocksAudited[i].height) {
                        isAlreadyAudited = true;
                        LogPrint("poa", "%s: PoA Block Hash: %s, is already auditted by Block %s\n", __func__,
                            block.posBlocksAudited[i].hash.GetHex(),
                            prevPoablock.GetHash().GetHex());
                        break;
                    }
                }

                if (isAlreadyAudited) {
                    ret = false;
                    break;
                }
            }
        }
    }

    return ret;
}

bool CheckPoABlockRewardAmount(const CBlock& block, const CBlockIndex* pindex)
{
    bool ret = false;
    CAmount nReward;
    if (pindex->nHeight >= Params().HardFork()) {
        nReward = 0.25 * COIN;
    } else {
        nReward = 0.5 * COIN;
    }
    ret = block.vtx.size() == 1;
    ret = ret && block.vtx[0].vout.size() == 1;
    ret = ret && block.vtx[0].vout[0].nValue == block.posBlocksAudited.size() * nReward;
    ret = ret && VerifyZeroBlindCommitment(block.vtx[0].vout[0]);

    return ret;
}

bool CheckPoABlockPaddingAmount(const CBlock& block, const CBlockIndex* pindex)
{
    bool ret = true;

    int nHeight = pindex->nHeight;
    int prevPoAHeight = 0;
    int lastPoSHeight = 0;
    int padding = 0;

    if (nHeight >= Params().HardFork()) {
        ret = false;
        if (mapBlockIndex.count(block.hashPrevPoABlock) != 0) {
            CBlockIndex* pPrevPoAIndex = mapBlockIndex[block.hashPrevPoABlock];
            CBlock prevPoablock;
            if (!ReadBlockFromDisk(prevPoablock, pPrevPoAIndex))
                throw runtime_error("Can't read block from disk");
            prevPoAHeight = pPrevPoAIndex->nHeight;
            for (size_t i = 0; i < block.posBlocksAudited.size(); i++) {
                lastPoSHeight = block.posBlocksAudited[i].height;
            }
        }
        padding = (nHeight - lastPoSHeight);
        if (padding >= Params().PoAPadding()){
            ret = true;
        }
        LogPrint("poa", "%s: nHeight: %d, prevPoAHeight: %d, lastPoSHeight: %d, padding: %d\n", __func__, nHeight, prevPoAHeight, lastPoSHeight, padding);
    }
    return ret;
}

// The functions below are workarounds for incorrectly audited blocks.
// Without them, PoA mining can not continue as these values are expected.
// To determine them, check the last 1-5 audited blocks of the raw data of
// the PoA block where the issue occurred. Compare to the real blocks/txids.
bool IsFixedAudit(std::string txid, int nHeight) {
    LogPrint("poa", "%s: block %d passed in as nHeight\n", __func__, nHeight);
    // Correct TXIDs for Block 17152, Block 135946, Block 311330 and Block 311331
    return (txid == "9965850037f14dcb4abf1168016e9f96f53692322714e7fac92a2b8838544135" || txid == "dd3d1dccf8f39a220e3a83cfabaf1b567b6696af877073ec580d09af6198f098" || txid =="e8aafd0513a8b2da536d55d9efd788956d03c6a0baa8acc4251f8dc0f3f03e87" || txid == "2666169b99521f12b6c69454f66e23af465c63e4a4807a5a8ed45467846ebe93");
}

bool IsWrongAudit(std::string txid, int nHeight) {
    LogPrint("poa", "%s: block %d passed in as nHeight\n", __func__, nHeight);
    // Orphan TXIDs for Block 135946, Block 311330 and Block 311331
    return (txid == "ef99f7882a681a075ebd51fa83be01685257ca66ccb736950fefc037f00e1538" || txid == "6514be1fad4d956a059924d5185a6f9db20a62f2f99e3e9b79257d6d3ca36065" || txid == "fd5a19a7a7df25774a6a030295f01bae6395be4229ebe2caf4974d536432e0dd");
}
