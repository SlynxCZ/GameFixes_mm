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

// The server browser lists a server's players from what the game server
// reported to the Steam API, and CS2 reports nothing. Every update_interval
// seconds, every connected player's SteamID, name and score is pushed through
// ISteamGameServer::BUpdateUserData (Source2ZE/ServerListPlayersFix).
#pragma once

#include "fix.h"
#include "plugin.h"

#include <steam/steam_gameserver.h>

struct Timer;

class CServerListPlayersFix final : public CFix
{
public:
    CServerListPlayersFix();

    const char* GetName() const override { return "server_list_players"; }

    void ReadConfig(KeyValues* pConfig) override;
    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

public: // Hooks
    KHook::Return<void> CSource2Server_GameServerSteamAPIActivated(ISource2Server* pThis);
    KHook::Return<void> CSource2Server_GameServerSteamAPIDeactivated(ISource2Server* pThis);

    KHook::Virtual<ISource2Server, void>* m_hSteamAPIActivated = nullptr;
    KHook::Virtual<ISource2Server, void>* m_hSteamAPIDeactivated = nullptr;

private:
    void UpdatePlayers();

    // Seconds between pushes ("update_interval").
    float m_flUpdateInterval = 5.0f;
    Timer* m_pUpdateTimer = nullptr;

    // The game server's Steam API, usable from GameServerSteamAPIActivated on.
    CSteamGameServerAPIContext m_steamAPI;
    bool m_bSteamAPIReady = false;
};
