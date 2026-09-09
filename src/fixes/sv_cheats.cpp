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

#include "sv_cheats.h"

#include "sdk/CBasePlayerController.h"

#include <eiface.h>
#include <entitysystem.h>
#include <interfaces/interfaces.h>
#include <tier1/convar.h>
#include <tier1/strtools.h>

bool CSvCheatsFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    s_pInstance = this;
    g_pCVar->InstallGlobalChangeCallback(&CSvCheatsFix::OnConVarChanged);
    m_bCallbackInstalled = true;

    Log("watching sv_cheats");
    return true;
}

void CSvCheatsFix::Unload()
{
    if (m_bCallbackInstalled)
        g_pCVar->RemoveGlobalChangeCallback(&CSvCheatsFix::OnConVarChanged);

    m_bCallbackInstalled = false;
    s_pInstance = nullptr;
}

void CSvCheatsFix::OnConVarChanged(ConVarRefAbstract* pConVar, CSplitScreenSlot nSlot, const char* pszNewValue, const char* pszOldValue, void* pUnknown)
{
    if (!s_pInstance || !pConVar || V_strcmp(pConVar->GetName(), "sv_cheats") != 0)
        return;

    if (pszNewValue && !V_atoi(pszNewValue))
        s_pInstance->OnSvCheatsDisabled();
}

void CSvCheatsFix::OnSvCheatsDisabled()
{
    CGameEntitySystem* pEntitySystem = GameEntitySystem();
    CGlobalVars* pGlobals = g_pEngineServer->GetServerGlobals();
    if (!pEntitySystem || !pGlobals)
        return;

    int iFixed = 0;
    for (int i = 0; i < pGlobals->maxClients; i++)
    {
        auto* pController = static_cast<CBasePlayerController*>(pEntitySystem->GetEntityInstance(CEntityIndex(i + 1)));
        if (!pController || !pController->IsConnected())
            continue;

        CBaseEntity* pPawn = pController->GetPawn();
        if (!pPawn || (pPawn->m_MoveType() != MOVETYPE_NOCLIP && pPawn->m_nActualMoveType() != MOVETYPE_NOCLIP))
            continue;

        pPawn->m_MoveType() = MOVETYPE_WALK;
        pPawn->m_nActualMoveType() = MOVETYPE_WALK;
        pPawn->m_MoveType.NetworkStateChanged();
        iFixed++;
    }

    if (iFixed)
        Log("sv_cheats went off, %d pawn(s) taken out of noclip", iFixed);
}
