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

#include "input_activator_crash.h"
#include "utils.hpp"

#include "dynlibutils/module.hpp"

#include <cstdio>

using namespace DynLibUtils;

CInputActivatorCrashFix::CInputActivatorCrashFix() :
    KHOOK_NEW(m_hInputTestActivator, this, &CInputActivatorCrashFix::CBaseFilter_InputTestActivator, nullptr)
{
}

bool CInputActivatorCrashFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    // void CBaseFilter::InputTestActivator(InputData_t* pInput) -- only ever referenced right next to the string "TestActivator".
    CMemory pInputTestActivator = modules.server.FindPattern(ParseStringPattern(WIN_LINUX("48 89 5C 24 ? 57 48 83 EC ? 4C 8B 02", "55 48 89 E5 41 54 49 89 F4 53 48 89 FB 48 83 EC ? 48 8B 07 48 8B 16")));
    if (!pInputTestActivator)
    {
        std::snprintf(error, maxlen, "CBaseFilter::InputTestActivator not found");
        return false;
    }

    m_hInputTestActivator->Configure(pInputTestActivator.GetPtr());

    Log("hooked CBaseFilter::InputTestActivator (%p)", pInputTestActivator.GetPtr());
    return true;
}

void CInputActivatorCrashFix::Unload()
{
    delete m_hInputTestActivator;
    m_hInputTestActivator = nullptr;
}

KHook::Return<void> CInputActivatorCrashFix::CBaseFilter_InputTestActivator(CBaseFilter* pThis, InputData_t* pInput)
{
    GF_TRACE(3);

    if (!pInput || !pInput->pActivator)
        return { KHook::Action::Supersede };

    return { KHook::Action::Ignore };
}
