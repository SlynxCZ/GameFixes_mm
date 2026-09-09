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

#include "team_limit.h"

#include <eiface.h>
#include <entitysystem.h>
#include <interfaces/interfaces.h>
#include <tier1/strtools.h>

bool CTeamLimitFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    // Nothing to resolve; the listener goes in once the game event manager exists.
    return true;
}

void CTeamLimitFix::Unload()
{
    if (m_pGameEventManager)
        m_pGameEventManager->RemoveListener(this);

    m_pGameEventManager = nullptr;
    m_pGameRules = nullptr;
}

void CTeamLimitFix::OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName)
{
    GF_TRACE(3);

    // New map, new cs_gamerules.
    m_pGameRules = nullptr;
}

void CTeamLimitFix::OnGameEventManagerReady(IGameEventManager2* pManager)
{
    GF_TRACE(3);

    m_pGameEventManager = pManager;
    m_pGameEventManager->AddListener(this, "round_start", true);

    Log("listening for round_start");
}

void CTeamLimitFix::FireGameEvent(IGameEvent* pEvent)
{
    GF_TRACE(3);

    CCSGameRules* pGameRules = GetGameRules();
    CGlobalVars* pGlobals = g_pEngineServer->GetServerGlobals();
    if (!pGameRules || !pGlobals)
        return;

    const int iMaxPlayers = pGlobals->maxClients;

    pGameRules->m_iNumSpawnableTerrorist() = iMaxPlayers;
    pGameRules->m_iNumSpawnableTerrorist.NetworkStateChanged();
    pGameRules->m_iMaxNumTerrorists() = iMaxPlayers;
    pGameRules->m_iMaxNumTerrorists.NetworkStateChanged();

    pGameRules->m_iNumSpawnableCT() = iMaxPlayers;
    pGameRules->m_iNumSpawnableCT.NetworkStateChanged();
    pGameRules->m_iMaxNumCTs() = iMaxPlayers;
    pGameRules->m_iMaxNumCTs.NetworkStateChanged();
}

CCSGameRules* CTeamLimitFix::GetGameRules()
{
    if (m_pGameRules)
        return m_pGameRules;

    CGameEntitySystem* pEntitySystem = GameEntitySystem();
    if (!pEntitySystem)
        return nullptr;

    // The rules object hangs off the one cs_gamerules entity; walk the active entity list for it.
    for (CEntityIdentity* pIdentity = pEntitySystem->m_EntityList.m_pFirstActiveEntity; pIdentity; pIdentity = pIdentity->m_pNext)
    {
        if (!pIdentity->m_pInstance || V_strcmp(pIdentity->GetClassname(), "cs_gamerules") != 0)
            continue;

        m_pGameRules = static_cast<CCSGameRulesProxy*>(pIdentity->m_pInstance)->m_pGameRules();
        break;
    }

    return m_pGameRules;
}
