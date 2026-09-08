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

// Clients key voice playback off the xuid in svc_VoiceData; on workshop maps
// it doesn't identify the speaker uniquely, streams collide and players stop
// hearing each other. CServerSideClient::SendNetMessage runs once per
// recipient, so the hook knows both the speaker (the message) and the
// listener (the client it was called on) and rewrites the xuid in place to
// one that is unique per speaker for that listener. Seeds reset on every map
// change.
#pragma once

#include "fix.h"
#include "plugin.h"
#include "utils.hpp"

#include "sdk/CServerSideClient.h"

#include "dynlibutils/virtual.hpp"

#include <const.h>

#include <cstdint>

class CWorkshopVoiceFix final : public CFix
{
public:
    CWorkshopVoiceFix();

    const char* GetName() const override { return "workshop_voice"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) override;

public: // Hooks
    KHook::Return<bool> CServerSideClient_SendNetMessage(CServerSideClientBase* pThis, const CNetMessage* pData, NetChannelBufType_t bufType);

    KHook::Virtual<CServerSideClientBase, bool, const CNetMessage*, NetChannelBufType_t>* m_hSendNetMessage = nullptr;

private:
    uint64 SeedForRecipient(int nSlot);

    // CServerSideClient's vtable (from libengine2); doubles as the hook's
    // stand-in object, see AsHookTarget().
    DynLibUtils::VirtualTable m_VTable;

    // Keyed by the slot of the client the voice packet is being sent *to*;
    // the speaker's entity index is added on top. Seeds are handed out 66
    // apart -- wider than any entity index -- so two listeners' xuid windows
    // can never overlap. Keying on the listener is what makes slot reuse
    // harmless: a new client in an old slot has nothing cached, so the
    // mapping it inherits is as good as a fresh one.
    uint64 m_PlayerSeeds[ABSOLUTE_PLAYER_LIMIT] = {};
    uint64 m_nSeedCursor = 0;
    bool m_bLoggedFirstRewrite = false;
};
