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

#include "voice_flood.h"

#include "scheduler.h"
#include "utils.hpp"
#include "vprof.hpp"

#include "dynlibutils/module.hpp"

#include "tier0/platform.h"
#include "tier1/KeyValues.h"

#include <cstdio>

using namespace DynLibUtils;

CVoiceFloodFix::CVoiceFloodFix() :
    KHOOK_NEW(m_hProcessVoiceData, &CServerSideClientBase::ProcessVoiceData, this, &CVoiceFloodFix::CServerSideClient_ProcessVoiceData, nullptr)
{
}

void CVoiceFloodFix::ReadConfig(KeyValues* pConfig)
{
    m_nMaxPackets = pConfig->GetInt("max_packets", 64);
    m_nMaxPerSecond = pConfig->GetInt("max_per_second", 128);
    m_bKick = pConfig->GetBool("kick", true);

    // A client talking sends one Opus packet per 20 ms, so 50 a second; below
    // these, real voice would start getting cut.
    if (m_nMaxPackets < 16)
        m_nMaxPackets = 16;
    if (m_nMaxPerSecond < 64)
        m_nMaxPerSecond = 64;
}

bool CVoiceFloodFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    CMemory pVTable = modules.engine.GetVirtualTableByName("CServerSideClient");
    if (!pVTable)
    {
        std::snprintf(error, maxlen, "CServerSideClient vtable not found");
        return false;
    }

    m_VTable.m_pVTFs = pVTable.RCast<void**>();
    m_hProcessVoiceData->AddGlobal(AsHookTarget<CServerSideClientBase>(m_VTable));

    Log("hooked CServerSideClient::ProcessVoiceData on vtable %p (max_packets %d, max_per_second %d, kick %d)", pVTable.GetPtr(), m_nMaxPackets, m_nMaxPerSecond, m_bKick);
    return true;
}

void CVoiceFloodFix::Unload()
{
    if (m_VTable.m_pVTFs)
        m_hProcessVoiceData->RemoveGlobal(AsHookTarget<CServerSideClientBase>(m_VTable));

    delete m_hProcessVoiceData;
    m_hProcessVoiceData = nullptr;
    m_VTable.m_pVTFs = nullptr;
}

KHook::Return<bool> CVoiceFloodFix::CServerSideClient_ProcessVoiceData(CServerSideClientBase* pThis, const CCLCMsg_VoiceData_t& msg)
{
    GF_VPROF("GameFixes::voice_flood::ProcessVoiceData");

    const int nSlot = pThis->GetPlayerSlot().Get();
    if (nSlot < 0 || nSlot >= ABSOLUTE_PLAYER_LIMIT)
        return { KHook::Action::Ignore, true };

    ClientState& state = m_Clients[nSlot];

    const int nUserId = pThis->GetUserID().Get();
    if (state.m_nUserId != nUserId)
    {
        state = ClientState();
        state.m_nUserId = nUserId;
    }

    const char* pszReason = CheckVoiceData(msg, m_nMaxPackets);

    if (!pszReason)
    {
        const double flNow = Plat_FloatTime();
        if (flNow - state.m_flWindowStart >= 1.0)
        {
            state.m_flWindowStart = flNow;
            state.m_nInWindow = 0;
        }

        if (++state.m_nInWindow > m_nMaxPerSecond)
            pszReason = "voice messages over max_per_second";
    }

    // Once caught, nothing more from this client is let through, even the
    // well-formed messages it mixes in, until it is gone.
    if (!pszReason && !state.m_bCaught)
        return { KHook::Action::Ignore, true };

    if (pszReason && !state.m_bCaught)
        Refuse(pThis, state, pszReason);

    // Swallowed, not rejected: returning false is what the engine counts
    // against a client as malformed traffic, on its own terms.
    return { KHook::Action::Supersede, true };
}

void CVoiceFloodFix::Refuse(CServerSideClientBase* pClient, ClientState& state, const char* pszReason)
{
    state.m_bCaught = true;

    const int nSlot = pClient->GetPlayerSlot().Get();
    const int nUserId = state.m_nUserId;

    Log("%s (slot %d, %llu): %s%s", pClient->GetClientName(), nSlot, pClient->GetClientSteamID().ConvertToUint64(), pszReason, m_bKick ? ", kicking" : ", dropping their voice");

    if (!m_bKick)
        return;

    // Not from inside the client's own message processing: next frame, and
    // only if the slot still holds the same client.
    scheduler::NextFrame([nSlot, nUserId]()
    {
        if (g_pEngineServer->GetPlayerUserId(CPlayerSlot(nSlot)).Get() != nUserId)
            return;

        g_pEngineServer->DisconnectClient(CPlayerSlot(nSlot), NETWORK_DISCONNECT_KICKED, "voice flood");
    });
}
