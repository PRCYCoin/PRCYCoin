// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Copyright (c) 2015-2018 The PIVX developers
// Copyright (c) 2018-2020 The DAPS Project developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include "primitives/transaction.h"
#include "keystore.h"
#include "serialize.h"
#include "uint256.h"

/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_BLOCK_SIZE_CURRENT = 2000000;
static const unsigned int MAX_BLOCK_SIZE_LEGACY = 1000000;

class PoSBlockSummary {
public:
    uint256 hash;
    uint32_t nTime;
    uint32_t height;
    
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->hash);
        READWRITE(this->nTime);
        READWRITE(this->height);
    }
    
    friend bool operator==(const PoSBlockSummary& a, const PoSBlockSummary& b)
    {
        return a.hash == b.hash && a.nTime == b.nTime && a.height == b.height;
    }

    friend bool operator!=(const PoSBlockSummary& a, const PoSBlockSummary& b)
    {
        return (a.hash != b.hash) || (a.nTime != b.nTime) || (a.height != b.height);
    }

    uint256 GetHash() const;
};

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // header
    static const int32_t CURRENT_VERSION=5;     // Version 5 supports CLTV activation
    //Efficient and compatible, but not beautiful design: A PoA block version will be always equal or higher this const
    static const int32_t POA_BLOCK_VERSION_LOW_LIMIT = 100;
    int32_t nVersion;
    //hashPrevBlock of PoA blocks is 0x00..00 for differentiating it from other block types
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;

    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;
    uint256 nAccumulatorCheckpoint;

    //PoA block specific
    //hash of previous PoA block, other block types dont need to care this property
    //For the first PoA block, this property should be set as a default value: maybe 0x11 (magic number) 
    //or the hash of the genenis block
    uint256 hashPrevPoABlock;
    //The hash root of all audited PoS block summary
    uint256 hashPoAMerkleRoot;
    //hash of any mined PoA block: minedHash is found when a miner successfully mines a PoA block
    //PoA block hash is hash of combination of previous hash and minedHash, since the previous hash of
    //a PoA block is only known once the miner has mined the PoA block
    uint256 minedHash;

    CBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        if (IsPoABlockByVersion()) {
            //PoA block
            READWRITE(hashPrevPoABlock);
            READWRITE(hashPoAMerkleRoot);
            READWRITE(minedHash);
        }
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    }

    bool IsPoABlockByVersion() const {
        return nVersion >= CBlockHeader::POA_BLOCK_VERSION_LOW_LIMIT;
    }

    void SetVersionPoABlock() {
        nVersion = CBlockHeader::POA_BLOCK_VERSION_LOW_LIMIT;
    }

    void SetNull()
    {
        nVersion = CBlockHeader::CURRENT_VERSION;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        hashPrevPoABlock.SetNull();
        hashPoAMerkleRoot.SetNull();
        minedHash.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
        nAccumulatorCheckpoint = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;
    uint256 ComputeMinedHash() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransaction> vtx;
    //Contain the summary of all audited PoS blocks sorted in an increasing order of block height
    //In between sequential audited PoS blocks, there might be PoA blocks which should not be found here
    std::vector<PoSBlockSummary> posBlocksAudited;

    // ppcoin: block signature - signed by one of the coin base txout[N]'s owner
    std::vector<unsigned char> vchBlockSig;

    // memory only
    mutable CScript payee;
    mutable std::vector<uint256> vMerkleTree;
    mutable std::vector<uint256> poaMerkleTree;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
        if(vtx.size() > 1 && vtx[1].IsCoinStake())
            READWRITE(vchBlockSig);
        if (IsProofOfAudit()) {
            READWRITE(posBlocksAudited);
        }
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        posBlocksAudited.clear();
        vMerkleTree.clear();
        poaMerkleTree.clear();
        payee = CScript();
        vchBlockSig.clear();
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;

        block.hashPrevPoABlock = hashPrevPoABlock;
        block.hashPoAMerkleRoot = hashPoAMerkleRoot;
        block.minedHash = minedHash;

        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        block.nAccumulatorCheckpoint = nAccumulatorCheckpoint;
        return block;
    }

    // ppcoin: two types of block: proof-of-work or proof-of-stake
    bool IsProofOfStake() const
    {
        return (vtx.size() > 1 && vtx[1].IsCoinStake()) && !IsProofOfAudit();
    }

    bool IsProofOfWork() const
    {
        return !IsProofOfStake() && !IsProofOfAudit();
    }

    /**
     * @todo
     * add condition check for Audit mining
     * @return
     */
    bool IsProofOfAudit() const
    {
        return IsPoABlockByVersion();
    }

    std::pair<COutPoint, unsigned int> GetProofOfStake() const
    {
        return IsProofOfStake()? std::make_pair(vtx[1].vin[0].prevout, nTime) : std::make_pair(COutPoint(), (unsigned int)0);
    }

    // Build the in-memory merkle tree for this block and return the merkle root.
    // If non-NULL, *mutated is set to whether mutation was detected in the merkle
    // tree (a duplication of transactions in the block leading to an identical
    // merkle root).
    uint256 BuildMerkleTree(bool* mutated = NULL) const;

    std::vector<uint256> GetMerkleBranch(int nIndex) const;
    static uint256 CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex);
    std::string ToString() const;
    void print() const;
    
    uint256 BuildPoAMerkleTree(bool* mutated = NULL) const;
    std::vector<uint256> GetPoAMerkleBranch(int nIndex) const;
    static uint256 CheckPoAMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex);
};


/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    CBlockLocator(const std::vector<uint256>& vHaveIn)
    {
        vHave = vHaveIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull()
    {
        return vHave.empty();
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
