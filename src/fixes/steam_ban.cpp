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

#include "dynlibutils/module.hpp"

#include <steam/steamclientpublic.h>
#include <tier1/KeyValues.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace DynLibUtils;

CSteamBanFix::CSteamBanFix() :
    m_hThink(new KHook::Virtual(WIN_LINUX(51u, 52u), this, &CSteamBanFix::CCSGameRules_Think, &CSteamBanFix::CCSGameRules_ThinkPost))
{
}

void CSteamBanFix::ReadConfig(KeyValues* pConfig)
{
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

    // The pass itself is non-virtual; its caller Think is a vtable slot, so a Pre/Post pair on the class vtable brackets it without a detour.
    CMemory pVTable = modules.server.GetVirtualTableByName("CCSGameRules");

    m_VTable.m_pVTFs = pVTable.RCast<void**>();
    m_hThink->AddGlobal(AsHookTarget<CCSGameRules>(m_VTable));

    Log("hooked CCSGameRules::Think on vtable %p, %zu whitelisted account(s)", pVTable.GetPtr(), m_whitelist.size());
    return true;
}

void CSteamBanFix::Unload()
{
    if (m_VTable.m_pVTFs)
        m_hThink->RemoveGlobal(AsHookTarget<CCSGameRules>(m_VTable));

    delete m_hThink;
    m_hThink = nullptr;
    m_VTable.m_pVTFs = nullptr;
    m_pBanMap = nullptr;
}

KHook::Return<void> CSteamBanFix::CCSGameRules_Think(CCSGameRules* pThis)
{
    if (!m_pBanMap || m_whitelist.empty() || m_pBanMap->Count() <= 0)
        return { KHook::Action::Ignore };

    // Collect first, remove after -- mutating the tree mid-walk isn't safe.
    std::vector<int> toRemove;
    FOR_EACH_MAP(*m_pBanMap, i)
    {
        if (m_whitelist.contains(m_pBanMap->Key(i)))
            toRemove.push_back(i);
    }

    for (int i : toRemove)
        m_pBanMap->RemoveAt(i);

    return { KHook::Action::Ignore };
}

KHook::Return<void> CSteamBanFix::CCSGameRules_ThinkPost(CCSGameRules* pThis)
{
    // Whoever the pass wanted kicked has been kicked by now; the rest of the map is stale and must not survive to the next frame. (Shared by @aiolos1045.)
    if (m_pBanMap && m_pBanMap->Count() > 0)
        m_pBanMap->RemoveAll();

    return { KHook::Action::Ignore };
}
