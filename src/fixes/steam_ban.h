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

// The engine's per-frame GC ban / competitive cooldown kick pass
// (GameSystem_Think_CheckSteamBan) walks CCSGameRules::sm_mapGcBanInformation
// and kicks every connected account it finds there. Hooked by signature, the
// way CS2Fixes does it: Pre strips the whitelisted accounts out of the map and,
// below sv_kick_players_with_cooldown 2, the competitive-cooldown entries the
// pass would still act on; Post clears whatever the pass left in it, so a
// stale entry can't spread to the next player to join.
#pragma once

#include "fix.h"
#include "plugin.h"
#include "utils.hpp"

#include "sdk/CGcBanInformation.h"

#include <tier1/utlmap.h>

#include <unordered_set>

class CSteamBanFix final : public CFix
{
public:
    CSteamBanFix();

    const char* GetName() const override { return "steam_ban"; }

    void ReadConfig(KeyValues* pConfig) override;
    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

public: // Hooks
    KHook::Return<void> GameSystem_Think_CheckSteamBan();
    KHook::Return<void> GameSystem_Think_CheckSteamBanPost();

    // void GameSystem_Think_CheckSteamBan(), found by signature.
    KHook::Function<void>* m_hCheckSteamBan = nullptr;

private:
    // Steam32 account ids ("whitelist", given as SteamID64s) the pass must
    // never see.
    std::unordered_set<uint32> m_whitelist;

    // "clear_after_pass": empty the map once the pass ran, so an entry the
    // engine already acted on never reaches the next player to join.
    bool m_bClearAfterPass = true;

    // CCSGameRules::sm_mapGcBanInformation, keyed by account id. The third
    // template argument is deliberately uint32, not a real comparator, to
    // match the engine's layout -- so only structural, index-based access
    // (FOR_EACH_MAP / Key / Element / RemoveAt / RemoveAll) is valid on it,
    // never Find / Insert.
    CUtlOrderedMap<uint32, CGcBanInformation_t, uint32>* m_pBanMap = nullptr;
};
