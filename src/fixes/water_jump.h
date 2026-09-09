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

// Touching the bottom of a body of water, the CategorizePosition that follows
// WaterMove sees ground under the player and treats them as standing on it,
// and a jump out of the water no longer takes. That one CategorizePosition is
// skipped after every WaterMove, so the player stays in the water state and
// can jump out. Moved here from the FUNPLAY gamemode plugins.
#pragma once

#include "fix.h"
#include "plugin.h"
#include "utils.hpp"

class CCSPlayer_MovementServices;
class CMoveData;

class CWaterJumpFix final : public CFix
{
public:
    CWaterJumpFix();

    const char* GetName() const override { return "water_jump"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) override;

public: // Hooks
    KHook::Return<void> CCSPlayer_MovementServices_WaterMove(CCSPlayer_MovementServices* pThis, CMoveData* pMove);
    KHook::Return<void> CCSPlayer_MovementServices_CategorizePosition(CCSPlayer_MovementServices* pThis, CMoveData* pMove, bool bStayOnGround);

    KHook::Member<CCSPlayer_MovementServices, void, CMoveData*>* m_hWaterMove = nullptr;
    KHook::Member<CCSPlayer_MovementServices, void, CMoveData*, bool>* m_hCategorizePosition = nullptr;

private:
    // Which slot a movement service belongs to; -1 for anything that is not a player.
    static int GetSlot(CCSPlayer_MovementServices* pServices);

    // Per slot: WaterMove ran, the next CategorizePosition is to be skipped.
    bool m_bSkipNextCategorize[64] = {};
};
