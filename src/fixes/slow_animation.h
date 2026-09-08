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

// curtime is a 32-bit float; after a day or two on one map it has lost enough
// precision for animation, movement and lag compensation to go sluggish, and
// nothing in the game ever resets it. Every reload_interval seconds, while no
// human is connected, the current map is reloaded; the remaining mp_timelimit
// is carried over to the fresh map.
#pragma once

#include "fix.h"

#include <tier1/convar.h>

#include <memory>

struct Timer;

class CSlowAnimationFix final : public CFix
{
public:
    const char* GetName() const override { return "slow_animation"; }

    void ReadConfig(KeyValues* pConfig) override;
    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) override;

private:
    void OnReloadTimer();
    int CountHumanPlayers() const;

    // Seconds between empty-server checks ("reload_interval").
    float m_flReloadInterval = 1800.0f;

    std::unique_ptr<CConVarRef<float>> m_timelimit;
    Timer* m_pReloadTimer = nullptr;

    // Map name and universal time from the last StartupServer.
    char m_szMap[256] = "";
    double m_dMapStartTime = 0.0;

    // Remaining mp_timelimit (minutes) to restore after a reload; < 0 = none.
    float m_flPendingTimelimit = -1.0f;
};
