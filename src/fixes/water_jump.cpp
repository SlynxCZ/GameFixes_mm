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

#include "water_jump.h"

#include "sdk/CBasePlayerPawn.h"
#include "sdk/CCSPlayer_MovementServices.h"

#include "dynlibutils/module.hpp"

#include <cstdio>
#include <cstring>

using namespace DynLibUtils;

CWaterJumpFix::CWaterJumpFix() :
    KHOOK_NEW(m_hWaterMove, this, &CWaterJumpFix::CCSPlayer_MovementServices_WaterMove, nullptr),
    KHOOK_NEW(m_hCategorizePosition, this, &CWaterJumpFix::CCSPlayer_MovementServices_CategorizePosition, nullptr)
{
}

bool CWaterJumpFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    // void CCSPlayer_MovementServices::WaterMove(CMoveData* mv)
    CMemory pWaterMove = modules.server.FindPattern(ParseStringPattern(WIN_LINUX(
        "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 4C 89 60 ? 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 ? 48 8B F9 0F 29 78 ? 48 8D 4D ? 44 0F 29 40",
        "48 B8 ? ? ? ? ? ? ? ? 55 66 0F EF C0 48 89 E5 41 57 41 56 41 55 4C 8D AD ? ? ? ? 41 54 49 89 FC 4C 89 EF 53 48 89 F3 48 81 EC ? ? ? ? 48 89 85 ? ? ? ? 48 8B 05 ? ? ? ? 0F 29 85 ? ? ? ? 48 C7 85 ? ? ? ? ? ? ? ? 48 C7 45 ? ? ? ? ? 48 89 85 ? ? ? ? 48 B8 ? ? ? ? ? ? ? ? 48 89 45 ? E8 ? ? ? ? 48 8D 8D")));
    if (!pWaterMove)
    {
        std::snprintf(error, maxlen, "CCSPlayer_MovementServices::WaterMove not found");
        return false;
    }

    // void CCSPlayer_MovementServices::CategorizePosition(CMoveData* mv, bool bStayOnGround)
    CMemory pCategorizePosition = modules.server.FindPattern(ParseStringPattern(WIN_LINUX("40 55 56 57 41 54 41 55 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B F9", "48 B8 ? ? ? ? ? ? ? ? 55 66 0F EF C0 48 89 E5 41 57 41 56 41 55 41 89 D5")));
    if (!pCategorizePosition)
    {
        std::snprintf(error, maxlen, "CCSPlayer_MovementServices::CategorizePosition not found");
        return false;
    }

    m_hWaterMove->Configure(pWaterMove.GetPtr());
    m_hCategorizePosition->Configure(pCategorizePosition.GetPtr());

    Log("hooked WaterMove (%p) and CategorizePosition (%p)", pWaterMove.GetPtr(), pCategorizePosition.GetPtr());
    return true;
}

void CWaterJumpFix::Unload()
{
    delete m_hWaterMove;
    delete m_hCategorizePosition;
    m_hWaterMove = nullptr;
    m_hCategorizePosition = nullptr;
}

void CWaterJumpFix::OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName)
{
    std::memset(m_bSkipNextCategorize, 0, sizeof(m_bSkipNextCategorize));
}

int CWaterJumpFix::GetSlot(CCSPlayer_MovementServices* pServices)
{
    CBasePlayerPawn* pPawn = pServices ? pServices->GetPawn() : nullptr;
    CBasePlayerController* pController = pPawn ? pPawn->GetController() : nullptr;
    if (!pController)
        return -1;

    // Controllers sit at entity index 1..64, one per slot.
    const int nSlot = pController->GetEntityIndex().Get() - 1;
    return (nSlot >= 0 && nSlot < 64) ? nSlot : -1;
}

KHook::Return<void> CWaterJumpFix::CCSPlayer_MovementServices_WaterMove(CCSPlayer_MovementServices* pThis, CMoveData* pMove)
{
    const int nSlot = GetSlot(pThis);
    if (nSlot >= 0)
        m_bSkipNextCategorize[nSlot] = true;

    return { KHook::Action::Ignore };
}

KHook::Return<void> CWaterJumpFix::CCSPlayer_MovementServices_CategorizePosition(CCSPlayer_MovementServices* pThis, CMoveData* pMove, bool bStayOnGround)
{
    const int nSlot = GetSlot(pThis);
    if (nSlot < 0 || !m_bSkipNextCategorize[nSlot])
        return { KHook::Action::Ignore };

    m_bSkipNextCategorize[nSlot] = false;
    return { KHook::Action::Supersede };
}
