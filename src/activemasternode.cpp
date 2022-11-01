// Copyright (c) 2014-2016 The Dash developers
// Copyright (c) 2015-2018 The PIVX developers
// Copyright (c) 2018-2020 The DAPS Project developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"

#include "addrman.h"
#include "masternode-sync.h"
#include "masternode.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "netbase.h"
#include "protocol.h"

//
// Bootup the Masternode, look for a 5000 PRCY input and register on the network
//
void CActiveMasternode::ManageStatus()
{
    std::string errorMessage;

    if (!fMasterNode) return;

    LogPrint(BCLog::MASTERNODE, "CActiveMasternode::ManageStatus() - Begin\n");

    //need correct blocks to send ping
    if (!Params().IsRegTestNet() && !masternodeSync.IsBlockchainSynced()) {
        status = ACTIVE_MASTERNODE_SYNC_IN_PROCESS;
        LogPrint(BCLog::MASTERNODE, "CActiveMasternode::ManageStatus() - %s\n", GetStatus());
        return;
    }

    if (status == ACTIVE_MASTERNODE_SYNC_IN_PROCESS) status = ACTIVE_MASTERNODE_INITIAL;

    if (status == ACTIVE_MASTERNODE_INITIAL) {
        CMasternode* pmn;
        pmn = mnodeman.Find(pubKeyMasternode);
        if (pmn != NULL) {
            pmn->Check();
            if (pmn->IsEnabled() && pmn->protocolVersion == PROTOCOL_VERSION) EnableHotColdMasterNode(pmn->vin, pmn->addr);
        }
    }

    if (status != ACTIVE_MASTERNODE_STARTED) {
        // Set defaults
        status = ACTIVE_MASTERNODE_NOT_CAPABLE;
        notCapableReason = "";

        if (pwalletMain->IsLocked()) {
            notCapableReason = "Wallet is locked.";
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        if (pwalletMain->GetBalance() == 0) {
            notCapableReason = "Hot node, waiting for remote activation.";
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        if (strMasterNodeAddr.empty()) {
            if (!GetLocal(service)) {
                notCapableReason = "Can't detect external address. Please use the masternodeaddr configuration option.";
                LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
                return;
            }
        } else {
            int nPort;
            std::string strHost;
            SplitHostPort(strMasterNodeAddr, nPort, strHost);
            service = LookupNumeric(strHost.c_str(), nPort);
        }

        // The service needs the correct default port to work properly
        if(!CMasternodeBroadcast::CheckDefaultPort(service, errorMessage, "CActiveMasternode::ManageStatus()"))
            return;

        LogPrintf("CActiveMasternode::ManageStatus() - Checking inbound connection to '%s'\n", service.ToString());

        CAddress addr(service, NODE_NETWORK);
        if (!OpenNetworkConnection(addr, true, nullptr)) {
            notCapableReason = "Could not connect to " + service.ToString();
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;

        if (GetMasterNodeVin(vin, pubKeyCollateralAddress, keyCollateralAddress)) {
            if (GetInputAge(vin) < MASTERNODE_MIN_CONFIRMATIONS) {
                status = ACTIVE_MASTERNODE_INPUT_TOO_NEW;
                notCapableReason = strprintf("%s - %d confirmations", GetStatus(), GetInputAge(vin));
                LogPrintf("CActiveMasternode::ManageStatus() - %s\n", notCapableReason);
                return;
            }
            LOCK(pwalletMain->cs_wallet);
            pwalletMain->LockCoin(vin.prevout);

            // send to all nodes
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            if (!CMessageSigner::GetKeysFromSecret(strMasterNodePrivKey, keyMasternode, pubKeyMasternode)) {
                LogPrintf("%s : Invalid masternode key", __func__);
                return;
            }

            CMasternodeBroadcast mnb;
            if (!CreateBroadcast(vin, service, keyCollateralAddress, pubKeyCollateralAddress, keyMasternode, pubKeyMasternode, errorMessage, mnb)) {
                notCapableReason = "Error on Register: " + errorMessage;
                LogPrintf("CActiveMasternode::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            //send to all peers
            LogPrintf("CActiveMasternode::ManageStatus() - Relay broadcast vin = %s\n", vin.ToString());
            mnb.Relay();

            LogPrintf("CActiveMasternode::ManageStatus() - Is capable master node!\n");
            status = ACTIVE_MASTERNODE_STARTED;

            return;
        } else {
            notCapableReason = "Could not find suitable coins!";
            LogPrintf("CActiveMasternode::ManageStatus() - %s\n", notCapableReason);
            return;
        }
    }

    //send to all peers
    if (!SendMasternodePing(errorMessage)) {
        LogPrintf("CActiveMasternode::ManageStatus() - Error on Ping: %s\n", errorMessage);
    }
}

std::string CActiveMasternode::GetStatus()
{
    switch (status) {
    case ACTIVE_MASTERNODE_INITIAL:
        return "Node just started, not yet activated";
    case ACTIVE_MASTERNODE_SYNC_IN_PROCESS:
        return "Sync in progress. Must wait until sync is complete to start Masternode";
    case ACTIVE_MASTERNODE_INPUT_TOO_NEW:
        return strprintf("Masternode input must have at least %d confirmations", MASTERNODE_MIN_CONFIRMATIONS);
    case ACTIVE_MASTERNODE_NOT_CAPABLE:
        return "Not capable masternode: " + notCapableReason;
    case ACTIVE_MASTERNODE_STARTED:
        return "Masternode successfully started";
    default:
        return "unknown";
    }
}

bool CActiveMasternode::SendMasternodePing(std::string& errorMessage)
{
    if (status != ACTIVE_MASTERNODE_STARTED) {
        errorMessage = "Masternode is not in a running status";
        return false;
    }

    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    if (!CMessageSigner::GetKeysFromSecret(strMasterNodePrivKey, keyMasternode, pubKeyMasternode)) {
        errorMessage = "Error upon calling SetKey.\n";
        return false;
    }

    bool fNewSigs = true;
    {
        LOCK(cs_main);
        fNewSigs = chainActive.NewSigsActive();
    }

    std::string stl(vin.masternodeStealthAddress.begin(), vin.masternodeStealthAddress.end());
    CMasternodePing mnp(vin);
    if (!mnp.Sign(keyMasternode, pubKeyMasternode, fNewSigs)) {
        errorMessage = "Couldn't sign Masternode Ping";
        return false;
    }

    // Update lastPing for our masternode in Masternode list
    CMasternode* pmn = mnodeman.Find(vin);
    if (pmn != NULL) {
        if (pmn->IsPingedWithin(MASTERNODE_PING_SECONDS, mnp.sigTime)) {
            errorMessage = "Too early to send Masternode Ping";
            return false;
        }

        pmn->lastPing = mnp;
        mnodeman.mapSeenMasternodePing.insert(std::make_pair(mnp.GetHash(), mnp));

        //mnodeman.mapSeenMasternodeBroadcast.lastPing is probably outdated, so we'll update it
        CMasternodeBroadcast mnb(*pmn);
        uint256 hash = mnb.GetHash();
        if (mnodeman.mapSeenMasternodeBroadcast.count(hash)) mnodeman.mapSeenMasternodeBroadcast[hash].lastPing = mnp;

        mnp.Relay();

        return true;
    } else {
        // Seems like we are trying to send a ping while the Masternode is not registered in the network
        errorMessage = "Masternode List doesn't include our Masternode, shutting down Masternode pinging service! " + vin.ToString();
        status = ACTIVE_MASTERNODE_NOT_CAPABLE;
        notCapableReason = errorMessage;
        return false;
    }
}

bool CActiveMasternode::CreateBroadcast(std::string strService, std::string strKeyMasternode, std::string strTxHash, std::string strOutputIndex, std::string& errorMessage, CMasternodeBroadcast &mnb, bool fOffline)
{
    CTxIn vin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    //need correct blocks to send ping
    if (!fOffline && !masternodeSync.IsBlockchainSynced()) {
        errorMessage = "Sync in progress. Must wait until sync is complete to start Masternode";
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    if (!CMessageSigner::GetKeysFromSecret(strKeyMasternode, keyMasternode, pubKeyMasternode)) {
        errorMessage = strprintf("Can't find keys for masternode %s", strService);
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    if (!GetMasterNodeVin(vin, pubKeyCollateralAddress, keyCollateralAddress, strTxHash, strOutputIndex)) {
        errorMessage = strprintf("Could not allocate vin %s:%s for masternode %s", strTxHash, strOutputIndex, strService);
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    int nPort;
    std::string strHost;
    SplitHostPort(strService, nPort, strHost);
    CService _service(LookupNumeric(strHost.c_str(), nPort));

    // The service needs the correct default port to work properly
    if(!CMasternodeBroadcast::CheckDefaultPort(_service, errorMessage, "CActiveMasternode::CreateBroadcast()"))
        return false;

    return CreateBroadcast(vin, _service, keyCollateralAddress, pubKeyCollateralAddress, keyMasternode, pubKeyMasternode, errorMessage, mnb);
}

bool CActiveMasternode::CreateBroadcast(CTxIn vin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyMasternode, CPubKey pubKeyMasternode, std::string& errorMessage, CMasternodeBroadcast &mnb)
{
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    bool fNewSigs = true;
    {
        LOCK(cs_main);
        fNewSigs = chainActive.NewSigsActive();
    }

    CMasternodePing mnp(vin);
    if (!mnp.Sign(keyMasternode, pubKeyMasternode, fNewSigs)) {
        errorMessage = strprintf("Failed to sign ping, vin: %s", vin.ToString());
        LogPrintf("CActiveMasternode::CreateBroadcast() -  %s\n", errorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    mnb = CMasternodeBroadcast(service, vin, pubKeyCollateralAddress, pubKeyMasternode, PROTOCOL_VERSION);
    mnb.lastPing = mnp;
    if (!mnb.Sign(keyCollateralAddress, pubKeyCollateralAddress, fNewSigs)) {
        errorMessage = strprintf("Failed to sign broadcast, vin: %s", vin.ToString());
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    return true;
}

bool CActiveMasternode::GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey)
{
    return GetMasterNodeVin(vin, pubkey, secretKey, "", "");
}

bool CActiveMasternode::GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex)
{
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    // Find possible candidates
    TRY_LOCK(pwalletMain->cs_wallet, fWallet);
    if (!fWallet) return false;

    std::vector<COutput> possibleCoins = SelectCoinsMasternode();
    COutput* selectedOutput;

    // Find the vin
    if (!strTxHash.empty()) {
        // Let's find it
        uint256 txHash(uint256S(strTxHash));
        int outputIndex;
        try {
            outputIndex = std::stoi(strOutputIndex.c_str());
        } catch (const std::exception& e) {
            LogPrintf("%s: %s on strOutputIndex\n", __func__, e.what());
            return false;
        }

        bool found = false;
        for (COutput& out : possibleCoins) {
            if (out.tx->GetHash() == txHash && out.i == outputIndex) {
                selectedOutput = &out;
                found = true;
                break;
            }
        }
        if (!found) {
            LogPrintf("CActiveMasternode::GetMasterNodeVin - Could not locate valid vin\n");
            return false;
        }
    } else {
        // No output specified,  Select the first one
        if (possibleCoins.size() > 0) {
            selectedOutput = &possibleCoins[0];
        } else {
            LogPrintf("CActiveMasternode::GetMasterNodeVin - Could not locate specified vin from possible list\n");
            return false;
        }
    }

    // At this point we have a selected output, retrieve the associated info
    return GetVinFromOutput(*selectedOutput, vin, pubkey, secretKey);
}


// Extract Masternode vin information from output
bool CActiveMasternode::GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey)
{
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;
    CScript pubScript;

    vin = CTxIn(out.tx->GetHash(), out.i);
    pubScript = out.tx->vout[out.i].scriptPubKey; // the inputs PubKey

    pwalletMain->findCorrespondingPrivateKey(out.tx->vout[out.i], secretKey);
    CTxDestination address1;
    ExtractDestination(pubScript, address1);
    CBitcoinAddress address2(address1);
    CPubKey sharedSec;
    pwalletMain->computeSharedSec(*out.tx, out.tx->vout[out.i], sharedSec);
    vin.encryptionKey.clear();
    std::copy(sharedSec.begin(), sharedSec.end(), std::back_inserter(vin.encryptionKey));

    CKeyID keyID;
    if (!address2.GetKeyID(keyID)) {
        LogPrintf("CActiveMasternode::GetMasterNodeVin - Address does not refer to a key\n");
        return false;
    }

    if (!pwalletMain->GetKey(keyID, secretKey)) {
        LogPrintf("CActiveMasternode::GetMasterNodeVin - Private key for address is not known\n");
        return false;
    }

    pubkey = secretKey.GetPubKey();
    std::string msa;
    pwalletMain->ComputeStealthPublicAddress("masteraccount", msa);
    std::copy(msa.begin(), msa.end(), std::back_inserter(vin.masternodeStealthAddress));
    if (!pwalletMain->generateKeyImage(out.tx->vout[out.i].scriptPubKey, vin.keyImage)) {
        LogPrintf("CActiveMasternode::GetMasterNodeVin - Failed to generate key image\n");
        return false;
    }
    if (!pwalletMain->MakeShnorrSignatureTxIn(vin, GetTxInSignatureHash(vin))) {
        LogPrintf("CActiveMasternode::GetMasterNodeVin - Failed to make Shnorr signature\n");
        return false;
    }


    //test verification masternode broadcast
    if (!VerifyShnorrKeyImageTxIn(vin, GetTxInSignatureHash(vin))) {
        LogPrintf("CActiveMasternode::GetMasterNodeVin - Failed to verify Shnorr signature\n");
        return false;
    }

    //Test the commitment and decoded value, if everything goes right, other nodes can verify it as well
    COutPoint prevout = vin.prevout;
    CTransaction prev;
    uint256 bh;
    if (!GetTransaction(prevout.hash, prev, bh, true)) {
        LogPrint(BCLog::MASTERNODE,"dsee - failed to read transaction hash %s\n", vin.prevout.hash.ToString());
        return false;
    }

    CTxOut txout = prev.vout[prevout.n];
    CPubKey sharedSec1(vin.encryptionKey.begin(), vin.encryptionKey.end());
    CKey mask;
    CAmount amount;
    ECDHInfo::Decode(txout.maskValue.mask.begin(), txout.maskValue.amount.begin(), sharedSec1, mask, amount);

    std::vector<unsigned char> commitment;
    CWallet::CreateCommitment(mask.begin(), amount, commitment);
    if (commitment != txout.commitment) {
        LogPrintf("dsee - decoded masternode collateralization not match %s\n", vin.prevout.hash.ToString());
        return false;
    }

    if (amount != Params().MNCollateralAmt()) {
        LogPrintf("dsee - masternode collateralization not equal to 5K %s\n", vin.prevout.hash.ToString());
        return false;
    }

    return true;
}

// get all possible outputs for running Masternode
std::vector<COutput> CActiveMasternode::SelectCoinsMasternode()
{
    std::vector<COutput> vCoins;
    std::vector<COutput> filteredCoins;
    std::vector<COutPoint> confLockedCoins;

    // Temporary unlock MN coins from masternode.conf
    if (GetBoolArg("-mnconflock", true)) {
        uint256 mnTxHash;
        {
            LOCK2(cs_main, pwalletMain->cs_wallet);
            for (CMasternodeConfig::CMasternodeEntry mne : masternodeConfig.getEntries()) {
                mnTxHash.SetHex(mne.getTxHash());

                int nIndex;
                if(!mne.castOutputIndex(nIndex))
                    continue;

                COutPoint outpoint = COutPoint(mnTxHash, nIndex);
                confLockedCoins.push_back(outpoint);
                pwalletMain->UnlockCoin(outpoint);
            }
        }
    }

    // Retrieve all possible outputs
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);
        pwalletMain->AvailableCoins(vCoins, true, NULL, false, AvailableCoinsType::ONLY_5000);
        // Lock MN coins from masternode.conf back if they where temporary unlocked
        if (!confLockedCoins.empty()) {
            for (COutPoint outpoint : confLockedCoins)
                pwalletMain->LockCoin(outpoint);
        }

        // Filter
        for (const COutput& out : vCoins) {
            if (pwalletMain->getCTxOutValue(*out.tx, out.tx->vout[out.i]) == Params().MNCollateralAmt()) { //exactly
                filteredCoins.push_back(out);
            }
        }
    }
    return filteredCoins;
}

// when starting a Masternode, this can enable to run as a hot wallet with no funds
bool CActiveMasternode::EnableHotColdMasterNode(CTxIn& newVin, CService& newService)
{
    if (!fMasterNode) return false;

    status = ACTIVE_MASTERNODE_STARTED;

    //The values below are needed for signing mnping messages going forward
    vin = newVin;
    service = newService;

    LogPrintf("CActiveMasternode::EnableHotColdMasterNode() - Enabled! You may shut down the cold daemon.\n");

    return true;
}
