/**
 * vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * GameFixes_mm
 * Copyright (C) 2026 Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   - Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl
 *
 * Project: GameFixes_mm
 */

#include "workshop_voice.h"

#include "utils.hpp"

#include "dynlibutils/module.hpp"

#include <networksystem/inetworkserializer.h>

#include <cstdio>
#include <cstring>
#include <random>

using namespace DynLibUtils;

CWorkshopVoiceFix::CWorkshopVoiceFix() :
    KHOOK_NEW(m_hSendNetMessage, &CServerSideClientBase::SendNetMessage, this, &CWorkshopVoiceFix::CServerSideClient_SendNetMessage, nullptr)
{
}

bool CWorkshopVoiceFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    // An engine class with no interface to fetch and no instance at load time; the vtable is resolved by name and the hook covers every client sharing it.
    CMemory pVTable = modules.engine.GetVirtualTableByName("CServerSideClient");

    m_VTable.m_pVTFs = pVTable.RCast<void**>();
    m_hSendNetMessage->AddGlobal(AsHookTarget<CServerSideClientBase>(m_VTable));

    Log("hooked CServerSideClient::SendNetMessage on vtable %p", pVTable.GetPtr());
    return true;
}

void CWorkshopVoiceFix::Unload()
{
    if (m_VTable.m_pVTFs)
        m_hSendNetMessage->RemoveGlobal(AsHookTarget<CServerSideClientBase>(m_VTable));

    delete m_hSendNetMessage;
    m_hSendNetMessage = nullptr;
    m_VTable.m_pVTFs = nullptr;
}

void CWorkshopVoiceFix::OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName)
{
    GF_TRACE(3);

    std::memset(m_PlayerSeeds, 0, sizeof(m_PlayerSeeds));
    m_bLoggedFirstRewrite = false;
}

KHook::Return<bool> CWorkshopVoiceFix::CServerSideClient_SendNetMessage(CServerSideClientBase* pThis, const CNetMessage* pData, NetChannelBufType_t bufType)
{
    GF_TRACE(3);

    if (!pData)
        return { KHook::Action::Ignore, true };

    INetworkMessageInternal* pNetMsg = pData->GetNetMessage();
    if (!pNetMsg)
        return { KHook::Action::Ignore, true };

    NetMessageInfo_t* pInfo = pNetMsg->GetNetMessageInfo();
    if (!pInfo || pInfo->m_MessageId != SVC_Messages::svc_VoiceData)
        return { KHook::Action::Ignore, true };

    // The reference implementation's "playerid": the client being sent to, not the one talking.
    const int nRecipient = pThis->GetPlayerSlot().Get();

    // Rewritten in place in Pre, so the engine serializes the modified message into this client's channel.
    CSVCMsg_VoiceData* pMsg = const_cast<CSVCMsg_VoiceData*>(static_cast<const CSVCMsg_VoiceData*>(pData->ToPB<CSVCMsg_VoiceData>()));

    // And its "msg.Entity": the speaker.
    const int nSpeaker = pMsg->entity();

    pMsg->set_xuid(SeedForRecipient(nRecipient) + static_cast<uint64>(nSpeaker));

    if (!m_bLoggedFirstRewrite)
    {
        m_bLoggedFirstRewrite = true;
        Log("rewriting svc_VoiceData xuid, first packet was entity %d -> slot %d", nSpeaker, nRecipient);
    }

    return { KHook::Action::Ignore, true };
}

uint64 CWorkshopVoiceFix::SeedForRecipient(int nSlot)
{
    if (nSlot < 0 || nSlot >= ABSOLUTE_PLAYER_LIMIT)
        return 0;

    if (m_PlayerSeeds[nSlot] == 0)
    {
        if (m_nSeedCursor == 0)
        {
            std::random_device rd;
            std::mt19937_64 gen(rd());

            m_nSeedCursor = gen() | 1;
        }

        m_nSeedCursor += 66;
        m_PlayerSeeds[nSlot] = m_nSeedCursor;
    }

    return m_PlayerSeeds[nSlot];
}
