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

// CBaseFilter::InputTestActivator dereferences the input's activator without
// checking it, so a TestActivator input fired with no activator (a map's
// output wired straight to a filter, for one) takes the server down. Calls
// without an activator are dropped before the game sees them.
#pragma once

#include "fix.h"
#include "plugin.h"

#include "sdk/InputData.h"

class CBaseFilter;

class CInputActivatorCrashFix final : public CFix
{
public:
    CInputActivatorCrashFix();

    const char* GetName() const override { return "input_activator_crash"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

public: // Hooks
    KHook::Return<void> CBaseFilter_InputTestActivator(CBaseFilter* pThis, InputData_t* pInput);

    KHook::Member<CBaseFilter, void, InputData_t*>* m_hInputTestActivator = nullptr;
};
