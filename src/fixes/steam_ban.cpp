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

#include "steam_ban.h"

#include "utils.hpp"

#include "dynlibutils/module.hpp"

#include <steam/steamclientpublic.h>
#include <tier1/convar.h>
#include <tier1/KeyValues.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace DynLibUtils;

CSteamBanFix::CSteamBanFix() :
    KHOOK_NEW(m_hCheckSteamBan, this, &CSteamBanFix::GameSystem_Think_CheckSteamBan, &CSteamBanFix::GameSystem_Think_CheckSteamBanPost)
{
}

void CSteamBanFix::ReadConfig(KeyValues* pConfig)
{
    m_bClearAfterPass = pConfig->GetBool("clear_after_pass", true);

    KeyValues* pWhitelist = pConfig->FindKey("whitelist");
    if (!pWhitelist)
        return;

    for (KeyValues* pEntry = pWhitelist->GetFirstSubKey(); pEntry; pEntry = pEntry->GetNextKey())
    {
        const char* pszSteamId64 = pEntry->GetString(nullptr, "");
        const CSteamID steamId((std::strtoull(pszSteamId64, nullptr, 10)));

        if (!steamId.IsValid())
        {
            Log("ignoring whitelist entry \"%s\" \"%s\": not a SteamID64", pEntry->GetName(), pszSteamId64);
            continue;
        }

        m_whitelist.insert(steamId.GetAccountID());
    }
}

bool CSteamBanFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    // After https://github.com/Source2ZE/CS2Fixes/commit/c82d21ae36588520391b301ed02bfb851dff18e1,
    // plus the whitelist.

    // lea reg, [rip + sm_mapGcBanInformation] inside CCSGameRules; the map
    // is the rip-relative operand.
    // Location to CUtlMap unk that is referenced on Windows by function with "Notification about user penalty: %u/%u (%u sec)\n" string
    // On Linux, a qword appears twice in GameSystem_Think_CheckSteamBan, and thrice in a sub-function of the function used for Windows (1 top, 2 bottom), the only other reference to this qword is some convar registration function with two unks above, sm_mapGcBanInformation is the unk further away
    CMemory pMapRef = modules.server.FindPattern(ParseStringPattern(WIN_LINUX("48 8D 0D ? ? ? ? 0F 11 44 24 ? 0F 11 44 24 ? E8", "48 8D 35 ? ? ? ? 89 C2 48 63 46")));
    if (!pMapRef)
    {
        std::snprintf(error, maxlen, "CCSGameRules::sm_mapGcBanInformation not found");
        return false;
    }

    m_pBanMap = pMapRef.ResolveRelativeAddress(3, 7).RCast<decltype(m_pBanMap)>();

    // void GameSystem_Think_CheckSteamBan()
    CMemory pCheckSteamBan = modules.server.FindPattern(ParseStringPattern(WIN_LINUX("41 54 48 81 EC ? ? ? ? BA", "55 48 8D 3D ? ? ? ? BE ? ? ? ? 48 89 E5 41 57 41 56 41 55 41 54 53 48 83 EC")));
    if (!pCheckSteamBan)
    {
        std::snprintf(error, maxlen, "GameSystem_Think_CheckSteamBan not found");
        return false;
    }

    // TODO: later change to pCheckSteamBan.GetPtr() after KHook fix
    m_hCheckSteamBan->Configure(pCheckSteamBan.RCast<void (*)()>());

    Log("hooked GameSystem_Think_CheckSteamBan (%p), ban map at %p, %zu whitelisted account(s), clear after pass %s", pCheckSteamBan.GetPtr(), m_pBanMap, m_whitelist.size(), m_bClearAfterPass ? "on" : "off");
    return true;
}

void CSteamBanFix::Unload()
{
    delete m_hCheckSteamBan;
    m_hCheckSteamBan = nullptr;
    m_pBanMap = nullptr;
}

KHook::Return<void> CSteamBanFix::GameSystem_Think_CheckSteamBan()
{
    if (!m_pBanMap || m_pBanMap->Count() <= 0)
        return { KHook::Action::Ignore };

    // Below sv_kick_players_with_cooldown 2 the pass still acts on competitive
    // cooldowns (reasons 20, 22 and 23); those entries go the same way as the
    // whitelisted accounts, before the pass gets to see them.
    static ConVarRefAbstract sv_kick_players_with_cooldown("sv_kick_players_with_cooldown");
    const bool bDropCooldowns = sv_kick_players_with_cooldown.IsValidRef() && sv_kick_players_with_cooldown.GetInt() < 2;

    // Collect first, remove after -- mutating the tree mid-walk isn't safe.
    std::vector<int> toRemove;
    FOR_EACH_MAP(*m_pBanMap, i)
    {
        const uint32 uReason = m_pBanMap->Element(i).m_uiReason;

        if (m_whitelist.contains(m_pBanMap->Key(i)))
            toRemove.push_back(i);
        else if (bDropCooldowns && (uReason == 20 || uReason == 22 || uReason == 23))
            toRemove.push_back(i);
    }

    for (int i : toRemove)
        m_pBanMap->RemoveAt(i);

    return { KHook::Action::Ignore };
}

KHook::Return<void> CSteamBanFix::GameSystem_Think_CheckSteamBanPost()
{
    // Whoever the pass wanted kicked has been kicked by now; the rest of the map
    // is stale and must not survive to the next frame. (Shared by @aiolos1045.)
    if (m_bClearAfterPass && m_pBanMap && m_pBanMap->Count() > 0)
        m_pBanMap->RemoveAll();

    return { KHook::Action::Ignore };
}
