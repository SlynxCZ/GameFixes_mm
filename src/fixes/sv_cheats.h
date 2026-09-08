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

// Turning sv_cheats off leaves anyone who was in noclip flying around with
// it. Watches the cvar through ICvar's global change callback and puts every
// such pawn back to MOVETYPE_WALK the moment it flips to 0.
#pragma once

#include "fix.h"

#include <icvar.h>

class CSvCheatsFix final : public CFix
{
public:
    const char* GetName() const override { return "sv_cheats"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

private:
    // ICvar hands out one plain function pointer per callback, so the one
    // fix instance is reached through s_pInstance.
    static void OnConVarChanged(ConVarRefAbstract* pConVar, CSplitScreenSlot nSlot, const char* pszNewValue, const char* pszOldValue, void* pUnknown);
    void OnSvCheatsDisabled();

    static inline CSvCheatsFix* s_pInstance = nullptr;
    bool m_bCallbackInstalled = false;
};
