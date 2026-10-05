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

#include "vprof.hpp"

#include "dynlibutils/module.hpp"

#include <algorithm>
#include <limits>

using namespace DynLibUtils;

// One message: voice_data, the serialized message, and its cost (the
// serialized size plus 4 bytes per packet) are each held to 16 KiB, and it
// may carry at most 64 packets.
static constexpr size_t kMaxVoiceDataBytes = 16384;
static constexpr size_t kMaxMessageBytes = 16384;
static constexpr uint32_t kMaxPacketsPerMessage = 64;
static constexpr size_t kBytesPerPacket = 4;

// The rolling budget, per client, over the last second.
static constexpr auto kBudgetWindow = std::chrono::seconds(1);
static constexpr size_t kMaxMessagesPerWindow = 192;
static constexpr size_t kMaxCostPerWindow = 256 * 1024;
static constexpr uint32_t kMaxPacketsPerWindow = 1024;

CVoiceFloodFix::CVoiceFloodFix() :
    KHOOK_NEW(m_hProcessVoiceData, &CServerSideClientBase::ProcessVoiceData, this, &CVoiceFloodFix::CServerSideClient_ProcessVoiceData, nullptr)
{
}

bool CVoiceFloodFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    // An engine class with no interface to fetch and no instance at load time; the vtable is resolved by name and the hook covers every client sharing it.
    CMemory pVTable = modules.engine.GetVirtualTableByName("CServerSideClient");

    m_VTable.m_pVTFs = pVTable.RCast<void**>();
    m_hProcessVoiceData->AddGlobal(AsHookTarget<CServerSideClientBase>(m_VTable));

    Log("hooked CServerSideClient::ProcessVoiceData on vtable %p", pVTable.GetPtr());
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

void CVoiceFloodFix::OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName)
{
    for (Budget& budget : m_Budgets)
        budget = {};

    if (m_nDropped)
        Log("%llu voice message(s) dropped on the last map", m_nDropped);

    m_nDropped = 0;
    m_bLoggedFirstDrop = false;
}

KHook::Return<bool> CVoiceFloodFix::CServerSideClient_ProcessVoiceData(CServerSideClientBase* pThis, const CCLCMsg_VoiceData_t& msg)
{
    GF_VPROF("GameFixes::voice_flood::ProcessVoiceData");

    if (!ShouldDrop(pThis, msg))
        return { KHook::Action::Ignore, true };

    m_nDropped++;
    if (!m_bLoggedFirstDrop)
    {
        m_bLoggedFirstDrop = true;
        Log("dropped a voice message from slot %d", pThis->GetPlayerSlot().Get());
    }

    // Answered true, so the engine doesn't count the client as broken; the
    // message just never reaches the voice system.
    return { KHook::Action::Supersede, true };
}

bool CVoiceFloodFix::ShouldDrop(CServerSideClientBase* pClient, const CCLCMsg_VoiceData& msg)
{
    // Bots and clients still connecting are left to the engine.
    if (!pClient || pClient->IsFakeClient() || !pClient->IsInGame())
        return false;

    const int nSlot = pClient->GetPlayerSlot().Get();
    if (nSlot < 0 || nSlot >= ABSOLUTE_PLAYER_LIMIT)
        return true;

    // Malformed: no audio, no sender, or a sender that isn't this client.
    if (!msg.has_audio() || !msg.has_xuid())
        return true;
    if (msg.xuid() == 0 || msg.xuid() != pClient->GetClientSteamID().ConvertToUint64())
        return true;

    const CMsgVoiceAudio& audio = msg.audio();
    if (!VoiceDataFormat_t_IsValid(audio.format()))
        return true;

    const uint32_t nOffsets = static_cast<uint32_t>(audio.packet_offsets_size());
    const uint32_t nPackets = audio.num_packets();
    const size_t nVoiceBytes = audio.voice_data().size();

    // Packets announced with no data to go with them.
    if ((nOffsets != 0 || nPackets != 0) && nVoiceBytes == 0)
        return true;

    if (nOffsets > kMaxPacketsPerMessage || nPackets > kMaxPacketsPerMessage || nVoiceBytes > kMaxVoiceDataBytes)
        return true;

    const size_t nMessageBytes = msg.ByteSizeLong();
    if (nMessageBytes > kMaxMessageBytes)
        return true;

    const uint32_t packets = std::max(nOffsets, nPackets);
    const size_t cost = nMessageBytes + kBytesPerPacket * packets;
    if (cost > kMaxMessageBytes)
        return true;

    // Forget what fell out of the window, then take this message only if it
    // still fits.
    Budget& budget = m_Budgets[nSlot];
    const Clock::time_point now = Clock::now();
    while (!budget.recent.empty() && now - budget.recent.front().time >= kBudgetWindow)
    {
        budget.cost -= budget.recent.front().cost;
        budget.packets -= budget.recent.front().packets;
        budget.recent.pop_front();
    }

    if (budget.recent.size() >= kMaxMessagesPerWindow)
        return true;
    if (budget.cost + cost > kMaxCostPerWindow)
        return true;
    if (budget.packets + packets > kMaxPacketsPerWindow)
        return true;

    budget.recent.push_back({ now, cost, packets });
    budget.cost += cost;
    budget.packets += packets;
    return false;
}
