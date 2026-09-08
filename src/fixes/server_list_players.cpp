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

#include "server_list_players.h"
#include "scheduler.h"

#include "sdk/CBasePlayerController.h"

#include <eiface.h>
#include <entitysystem.h>
#include <interfaces/interfaces.h>
#include <tier1/KeyValues.h>

CServerListPlayersFix::CServerListPlayersFix() :
    m_hSteamAPIActivated(new KHook::Virtual(&ISource2Server::GameServerSteamAPIActivated, this, nullptr, &CServerListPlayersFix::CSource2Server_GameServerSteamAPIActivated)),
    m_hSteamAPIDeactivated(new KHook::Virtual(&ISource2Server::GameServerSteamAPIDeactivated, this, nullptr, &CServerListPlayersFix::CSource2Server_GameServerSteamAPIDeactivated))
{
}

void CServerListPlayersFix::ReadConfig(KeyValues* pConfig)
{
    m_flUpdateInterval = pConfig->GetFloat("update_interval", 5.0f);

    if (m_flUpdateInterval < 1.0f)
        m_flUpdateInterval = 1.0f;
}

bool CServerListPlayersFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    m_hSteamAPIActivated->Add(g_pSource2Server);
    m_hSteamAPIDeactivated->Add(g_pSource2Server);

    // Loaded after the API already came up (late load): there won't be another activation, so try it right away.
    m_bSteamAPIReady = m_steamAPI.Init();

    m_pUpdateTimer = scheduler::AddTimer(m_flUpdateInterval, [this]() { UpdatePlayers(); }, TIMER_FLAG_REPEAT);

    Log("pushing player data to Steam every %.0f s (Steam API %s)", m_flUpdateInterval, m_bSteamAPIReady ? "ready" : "not up yet");
    return true;
}

void CServerListPlayersFix::Unload()
{
    m_hSteamAPIActivated->Remove(g_pSource2Server);
    m_hSteamAPIDeactivated->Remove(g_pSource2Server);

    delete m_hSteamAPIActivated;
    delete m_hSteamAPIDeactivated;
    m_hSteamAPIActivated = nullptr;
    m_hSteamAPIDeactivated = nullptr;

    scheduler::KillTimer(m_pUpdateTimer);
    m_pUpdateTimer = nullptr;

    m_steamAPI.Clear();
    m_bSteamAPIReady = false;
}

KHook::Return<void> CServerListPlayersFix::CSource2Server_GameServerSteamAPIActivated(ISource2Server* pThis)
{
    m_bSteamAPIReady = m_steamAPI.Init();

    return { KHook::Action::Ignore };
}

KHook::Return<void> CServerListPlayersFix::CSource2Server_GameServerSteamAPIDeactivated(ISource2Server* pThis)
{
    m_steamAPI.Clear();
    m_bSteamAPIReady = false;

    return { KHook::Action::Ignore };
}

void CServerListPlayersFix::UpdatePlayers()
{
    if (!m_bSteamAPIReady)
        return;

    ISteamGameServer* pGameServer = m_steamAPI.SteamGameServer();
    CGameEntitySystem* pEntitySystem = GameEntitySystem();
    CGlobalVars* pGlobals = g_pEngineServer->GetServerGlobals();
    if (!pGameServer || !pEntitySystem || !pGlobals)
        return;

    for (int i = 0; i < pGlobals->maxClients; i++)
    {
        const CSteamID* pSteamId = g_pEngineServer->GetClientSteamID(CPlayerSlot(i));
        if (!pSteamId)
            continue;

        auto* pController = static_cast<CBasePlayerController*>(pEntitySystem->GetEntityInstance(CEntityIndex(i + 1)));
        if (!pController)
            continue;

        pGameServer->BUpdateUserData(*pSteamId, pController->GetPlayerName(), g_pSource2GameClients->GetPlayerScore(CPlayerSlot(i)));
    }
}
