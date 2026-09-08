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
// and kicks every connected account it finds there. Hooked around its virtual
// caller, CCSGameRules::Think: Pre strips the whitelisted accounts out of the
// map, Post clears whatever the pass left in it, so a stale entry can't
// spread to the next player to join.
#pragma once

#include "fix.h"
#include "plugin.h"
#include "utils.hpp"

#include "sdk/CGcBanInformation.h"

#include "dynlibutils/virtual.hpp"

#include <tier1/utlmap.h>

#include <unordered_set>

class CCSGameRules;

class CSteamBanFix final : public CFix
{
public:
    CSteamBanFix();

    const char* GetName() const override { return "steam_ban"; }

    void ReadConfig(KeyValues* pConfig) override;
    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

public: // Hooks
    KHook::Return<void> CCSGameRules_Think(CCSGameRules* pThis);
    KHook::Return<void> CCSGameRules_ThinkPost(CCSGameRules* pThis);

    // void CCSGameRules::Think() -- the Linux index is read off libserver.so's
    // _ZTV12CCSGameRules; Windows is the usual one-less (a single destructor
    // slot) and has not been verified against a Windows build.
    KHook::Virtual<CCSGameRules, void>* m_hThink = nullptr;

private:
    // CCSGameRules' vtable (from libserver); doubles as the hook's stand-in
    // object, see AsHookTarget().
    DynLibUtils::VirtualTable m_VTable;

    // Steam32 account ids ("whitelist", given as SteamID64s) the pass must
    // never see.
    std::unordered_set<uint32> m_whitelist;

    // CCSGameRules::sm_mapGcBanInformation, keyed by account id. The third
    // template argument is deliberately uint32, not a real comparator, to
    // match the engine's layout -- so only structural, index-based access
    // (FOR_EACH_MAP / Key / RemoveAt / RemoveAll) is valid on it, never
    // Find / Insert.
    CUtlOrderedMap<uint32, CGcBanInformation_t, uint32>* m_pBanMap = nullptr;
};
