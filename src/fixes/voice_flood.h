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

// The "server lagger": a client floods clc_VoiceData, hundreds of messages a
// tick, each with an empty voice_data and thousands of zero packet_offsets
// (1475 or 16320 of them). Every one the engine accepts is copied into an
// svc_VoiceData for each listener, so the server spends the frame building
// voice nobody can play. CServerSideClient::ProcessVoiceData is where the
// engine takes the message; malformed ones and anything past a per-second
// budget are swallowed there, and the sender is kicked.
#pragma once

#include "fix.h"
#include "plugin.h"
#include "utils.hpp"

#include "sdk/CServerSideClient.h"

#include "dynlibutils/virtual.hpp"

#include <const.h>

class CVoiceFloodFix final : public CFix
{
public:
    CVoiceFloodFix();

    const char* GetName() const override { return "voice_flood"; }

    void ReadConfig(KeyValues* pConfig) override;
    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    // Why a voice message is refused, or nullptr if it may go through. Every
    // entry in packet_offsets marks an Opus packet inside voice_data, and a
    // packet is at least a byte: more offsets than bytes is not voice.
    static const char* CheckVoiceData(const CCLCMsg_VoiceData& msg, int nMaxPackets)
    {
        if (!msg.has_audio())
            return nullptr;

        const CMsgVoiceAudio& audio = msg.audio();
        const int nPackets = audio.packet_offsets_size();

        if (nPackets > nMaxPackets)
            return "too many packet_offsets";

        if (static_cast<size_t>(nPackets) > audio.voice_data().size())
            return "more packet_offsets than voice_data bytes";

        return nullptr;
    }

public: // Hooks
    KHook::Return<bool> CServerSideClient_ProcessVoiceData(CServerSideClientBase* pThis, const CCLCMsg_VoiceData_t& msg);

    KHook::Virtual<CServerSideClientBase, bool, const CCLCMsg_VoiceData_t&>* m_hProcessVoiceData = nullptr;

private:
    struct ClientState
    {
        // Whose state this is: a new client in the slot has another userid.
        int m_nUserId = -1;
        double m_flWindowStart = 0.0;
        int m_nInWindow = 0;
        bool m_bCaught = false;
    };

    void Refuse(CServerSideClientBase* pClient, ClientState& state, const char* pszReason);

    DynLibUtils::VirtualTable m_VTable;

    ClientState m_Clients[ABSOLUTE_PLAYER_LIMIT];

    int m_nMaxPackets = 64;
    int m_nMaxPerSecond = 128;
    bool m_bKick = true;
};
