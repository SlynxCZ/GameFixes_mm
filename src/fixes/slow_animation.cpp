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

#include "slow_animation.h"
#include "scheduler.h"

#include "sdk/CBasePlayerController.h"
#include "sdk/GameSessionConfiguration.h"

#include <eiface.h>
#include <entitysystem.h>
#include <interfaces/interfaces.h>
#include <tier1/KeyValues.h>
#include <tier1/strtools.h>

void CSlowAnimationFix::ReadConfig(KeyValues* pConfig)
{
    m_flReloadInterval = pConfig->GetFloat("reload_interval", 1800.0f);

    // Anything shorter would just keep flipping the map around.
    if (m_flReloadInterval < 60.0f)
        m_flReloadInterval = 60.0f;
}

bool CSlowAnimationFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    m_timelimit = std::make_unique<CConVarRef<float>>("mp_timelimit");
    m_pReloadTimer = scheduler::AddTimer(m_flReloadInterval, [this]() { OnReloadTimer(); }, TIMER_FLAG_REPEAT);

    Log("checking for an empty server every %.0f s", m_flReloadInterval);
    return true;
}

void CSlowAnimationFix::Unload()
{
    scheduler::KillTimer(m_pReloadTimer);
    m_pReloadTimer = nullptr;
    m_timelimit.reset();
}

void CSlowAnimationFix::OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName)
{
    V_snprintf(m_szMap, sizeof(m_szMap), "%s", (pszMapName && pszMapName[0]) ? pszMapName : "unknown");
    m_dMapStartTime = g_dUniversalTime;

    Log("map snapshot '%s'", m_szMap);

    if (m_flPendingTimelimit < 0.0f)
        return;

    const float flRemaining = m_flPendingTimelimit;
    m_flPendingTimelimit = -1.0f;

    // The map's own cfg runs after StartupServer and would overwrite the value; one frame later is past that.
    scheduler::NextFrame([this, flRemaining]()
    {
        if (!m_timelimit || !m_timelimit->IsValidRef())
            return;

        const float flValue = flRemaining > 0.0f ? flRemaining : 0.1f;
        Log("restoring the remaining timelimit -> mp_timelimit %.1f", flValue);
        m_timelimit->Set(flValue);
    });
}

void CSlowAnimationFix::OnReloadTimer()
{
    if (!m_szMap[0])
    {
        Log("reload skipped: no map snapshot yet (loaded mid-map, waiting for the next StartupServer)");
        return;
    }

    const int iPlayers = CountHumanPlayers();
    if (iPlayers > 0)
    {
        Log("reload skipped: %d human player(s) on '%s', next check in %.0f s", iPlayers, m_szMap, m_flReloadInterval);
        return;
    }

    const float flElapsedMinutes = static_cast<float>(g_dUniversalTime - m_dMapStartTime) / 60.0f;
    const float flTimelimit = (m_timelimit && m_timelimit->IsValidRef()) ? m_timelimit->Get() : 0.0f;

    if (flTimelimit > 0.0f)
    {
        m_flPendingTimelimit = flTimelimit - flElapsedMinutes;
        Log("timelimit %.1f min, elapsed %.1f min -> restoring %.1f min after the reload", flTimelimit, flElapsedMinutes, m_flPendingTimelimit);
    }

    if (g_pEngineServer->IsMapValid(m_szMap))
    {
        Log("server empty -> ChangeLevel('%s')", m_szMap);
        g_pEngineServer->ChangeLevel(m_szMap, nullptr);
    }
    else
    {
        // Not a stock map -> a workshop one, which only changelevels by id.
        char szCommand[320];
        V_snprintf(szCommand, sizeof(szCommand), "ds_workshop_changelevel %s", m_szMap);
        Log("server empty -> '%s'", szCommand);
        g_pEngineServer->ServerCommand(szCommand);
    }
}

int CSlowAnimationFix::CountHumanPlayers() const
{
    CGameEntitySystem* pEntitySystem = GameEntitySystem();
    CGlobalVars* pGlobals = g_pEngineServer->GetServerGlobals();
    if (!pEntitySystem || !pGlobals)
        return 0;

    int iPlayers = 0;
    for (int i = 0; i < pGlobals->maxClients; i++)
    {
        auto* pController = static_cast<CBasePlayerController*>(pEntitySystem->GetEntityInstance(CEntityIndex(i + 1)));
        if (pController && pController->IsConnected() && !pController->IsBot() && !pController->IsHLTV())
            iPlayers++;
    }

    return iPlayers;
}
