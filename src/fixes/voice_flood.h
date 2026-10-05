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

// A client can send clc_VoiceData that is malformed (no audio, a sender that
// isn't them, an unknown format, packets announced with no data) or simply
// huge, and the engine does the work of handling every one of them. Those
// messages are dropped in CServerSideClient::ProcessVoiceData before the
// engine sees them, and every client is held to a rolling one-second budget
// of voice work. Nobody is kicked; a dropped message is just never played.
#pragma once

#include "fix.h"
#include "plugin.h"
#include "utils.hpp"

#include "sdk/CServerSideClient.h"
#include "sdk/netmessages.h"

#include "dynlibutils/virtual.hpp"

#include <const.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>

class CVoiceFloodFix final : public CFix
{
public:
    CVoiceFloodFix();

    const char* GetName() const override { return "voice_flood"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) override;

public: // Hooks
    KHook::Return<bool> CServerSideClient_ProcessVoiceData(CServerSideClientBase* pThis, const CCLCMsg_VoiceData_t& msg);

    KHook::Virtual<CServerSideClientBase, bool, const CCLCMsg_VoiceData_t&>* m_hProcessVoiceData = nullptr;

private:
    using Clock = std::chrono::steady_clock;

    struct Work
    {
        Clock::time_point time{};
        size_t cost = 0;
        uint32_t packets = 0;
    };

    // What one client got through in the last second. A slot taken over by a
    // new client inherits at most one second of someone else's budget, so a
    // disconnect needs no reset of its own.
    struct Budget
    {
        std::deque<Work> recent;
        size_t cost = 0;
        uint32_t packets = 0;
    };

    bool ShouldDrop(CServerSideClientBase* pClient, const CCLCMsg_VoiceData& msg);

    // CServerSideClient's vtable (from libengine2); doubles as the hook's
    // stand-in object, see AsHookTarget().
    DynLibUtils::VirtualTable m_VTable;

    Budget m_Budgets[ABSOLUTE_PLAYER_LIMIT];
    uint64_t m_nDropped = 0;
    bool m_bLoggedFirstDrop = false;
};
