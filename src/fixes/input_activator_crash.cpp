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
#include "vprof.hpp"

#include "dynlibutils/module.hpp"

#include <cstdio>
#include <cstring>

using namespace DynLibUtils;

CInputActivatorCrashFix::CInputActivatorCrashFix() :
#ifdef _WIN32
    KHOOK_NEW(m_hInputTestActivator, this, &CInputActivatorCrashFix::CBaseFilter_API_TestActivator, nullptr)
#else
    KHOOK_NEW(m_hInputTestActivator, this, &CInputActivatorCrashFix::CBaseFilter_InputTestActivator, nullptr)
#endif
{
}

#ifdef _WIN32
// The dispatcher is generated binding code whose prologue is shared by every
// other _API dispatcher, so it is found by its body (the PassesFilter vcall and
// the m_bNegated check) and then walked back to its start: MSVC aligns
// functions to 16 and pads between them with int3.
static void* FindTestActivatorDispatcher(const CModule& server)
{
    CMemory pBody = server.FindPattern(ParseStringPattern("FF 52 08 48 8B 0F 4C 8B C6 48 8B D0 4C 8B 89 ? ? ? ? 48 8B CF 41 FF D1 80 BF ? ? ? ? 00"));
    if (!pBody)
        return nullptr;

    static constexpr uint8_t PROLOGUE[] = { 0x48, 0x89, 0x5C, 0x24 }; // mov [rsp+..], rbx
    const auto body = reinterpret_cast<uintptr_t>(pBody.GetPtr());
    for (uintptr_t p = body & ~uintptr_t(0xF); p + 0x400 > body; p -= 0x10)
    {
        const auto* pFunc = reinterpret_cast<const uint8_t*>(p);
        if (pFunc[-1] == 0xCC && std::memcmp(pFunc, PROLOGUE, sizeof(PROLOGUE)) == 0)
            return reinterpret_cast<void*>(p);
    }
    return nullptr;
}
#endif

bool CInputActivatorCrashFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
#ifdef _WIN32
    // int64 CBaseFilter_API::TestActivator dispatcher -- CBaseFilter::InputTestActivator is inlined into it on Windows.
    void* pTarget = FindTestActivatorDispatcher(modules.server);
    if (!pTarget)
    {
        std::snprintf(error, maxlen, "CBaseFilter_API::TestActivator dispatcher not found");
        return false;
    }

    m_hInputTestActivator->Configure(reinterpret_cast<int64_t (*)(void*, void*, void*, void*, void*)>(pTarget));

    Log("hooked CBaseFilter_API::TestActivator (%p)", pTarget);
#else
    // void CBaseFilter::InputTestActivator(InputData_t* pInput) -- since the 2026-09-23 update the datamap no longer
    // points at it; the "TestActivator" input dispatcher calls it directly with { activator, caller }.
    CMemory pInputTestActivator = modules.server.FindPattern(ParseStringPattern("55 48 89 E5 41 55 41 54 49 89 F4 53 48 89 FB 48 81 EC ? ? ? ? 48 8B 07 48 8B 16"));
    if (!pInputTestActivator)
    {
        std::snprintf(error, maxlen, "CBaseFilter::InputTestActivator not found");
        return false;
    }

    m_hInputTestActivator->Configure(pInputTestActivator.GetPtr());

    Log("hooked CBaseFilter::InputTestActivator (%p)", pInputTestActivator.GetPtr());
#endif
    return true;
}

void CInputActivatorCrashFix::Unload()
{
    delete m_hInputTestActivator;
    m_hInputTestActivator = nullptr;
}

#ifdef _WIN32
KHook::Return<int64_t> CInputActivatorCrashFix::CBaseFilter_API_TestActivator(void* pBinding, void* a2, void* a3, void* pContext, void* pArgs)
{
    GF_VPROF("GameFixes::input_activator_crash::InputTestActivator");

    // The dispatcher asks the object at pContext + 16 for the activator
    // (vtable slot 0) and hands it to PassesFilter unchecked.
    void* pSource = pContext ? *reinterpret_cast<void**>(static_cast<uint8_t*>(pContext) + 16) : nullptr;
    using GetActivatorFn = void* (*)(void*);
    if (!pSource || !(*reinterpret_cast<GetActivatorFn**>(pSource))[0](pSource))
        return { KHook::Action::Supersede, 0 };

    return { KHook::Action::Ignore, 0 };
}
#else
KHook::Return<void> CInputActivatorCrashFix::CBaseFilter_InputTestActivator(CBaseFilter* pThis, InputData_t* pInput)
{
    GF_VPROF("GameFixes::input_activator_crash::InputTestActivator");

    if (!pInput || !pInput->pActivator)
        return { KHook::Action::Supersede };

    return { KHook::Action::Ignore };
}
#endif
