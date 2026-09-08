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

// The game caps how many players may spawn on each team from the map's spawn
// points, so a join gets refused as "team full" on maps with few spawns. On
// every round_start, CCSGameRules' spawnable/max T and CT counts are raised
// to maxplayers.
#pragma once

#include "fix.h"

#include "sdk/CCSGameRules.h"

#include <igameevents.h>

class CTeamLimitFix final : public CFix, public IGameEventListener2
{
public:
    const char* GetName() const override { return "team_limit"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) override;
    void OnGameEventManagerReady(IGameEventManager2* pManager) override;

public: // IGameEventListener2 (round_start)
    void FireGameEvent(IGameEvent* pEvent) override;

private:
    CCSGameRules* GetGameRules();

    IGameEventManager2* m_pGameEventManager = nullptr;

    // cs_gamerules' rules object, looked up once per map.
    CCSGameRules* m_pGameRules = nullptr;
};
