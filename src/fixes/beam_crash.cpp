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

#include "beam_crash.h"
#include "utils.hpp"

#include "dynlibutils/module.hpp"

#include <cstdio>

using namespace DynLibUtils;

CBeamCrashFix::CBeamCrashFix() :
    m_hSetBeamOrigin(new KHook::Member(this, &CBeamCrashFix::CBeam_SetBeamOrigin, &CBeamCrashFix::CBeam_SetBeamOriginPost)),
    m_hSetBeamEndPos(new KHook::Member(this, &CBeamCrashFix::CBeam_SetBeamEndPos, &CBeamCrashFix::CBeam_SetBeamEndPosPost))
{
}

bool CBeamCrashFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    // https://github.com/Source2ZE/CS2Fixes/commit/0686a807d790ef22407aa83e33b40bbed6531b51

    // void CBeam::SetBeamOrigin(const Vector* pVecPosition)
    CMemory pSetBeamOrigin = modules.server.FindPattern(ParseStringPattern(WIN_LINUX("48 89 5C 24 ? 57 48 81 EC ? ? ? ? 48 8B FA 48 8B D9 E8 ? ? ? ? 48 8B CB", "55 48 89 E5 41 54 49 89 F4 53 48 89 FB 48 83 EC ? 0F 1F 80")));
    if (!pSetBeamOrigin)
    {
        std::snprintf(error, maxlen, "CBeam::SetBeamOrigin not found");
        return false;
    }

    // void CBeam::SetBeamEndPos(const Vector* pVecPosition)
    CMemory pSetBeamEndPos = modules.server.FindPattern(ParseStringPattern(WIN_LINUX("48 8B C4 48 89 58 ? 57 48 81 EC ? ? ? ? 0F 29 70 ? 48 8B FA 0F 29 78 ? 48 8B D9", "55 48 89 E5 41 57 41 56 41 55 41 54 49 89 F4 53 48 89 FB 48 81 EC ? ? ? ? 66 0F 1F 44 00")));
    if (!pSetBeamEndPos)
    {
        std::snprintf(error, maxlen, "CBeam::SetBeamEndPos not found");
        return false;
    }

    m_hSetBeamOrigin->Configure(pSetBeamOrigin.GetPtr());
    m_hSetBeamEndPos->Configure(pSetBeamEndPos.GetPtr());

    Log("hooked CBeam::SetBeamOrigin (%p) and CBeam::SetBeamEndPos (%p)", pSetBeamOrigin.GetPtr(), pSetBeamEndPos.GetPtr());
    return true;
}

void CBeamCrashFix::Unload()
{
    delete m_hSetBeamOrigin;
    delete m_hSetBeamEndPos;
    m_hSetBeamOrigin = nullptr;
    m_hSetBeamEndPos = nullptr;
}

KHook::Return<void> CBeamCrashFix::CBeam_SetBeamOrigin(CBeam* pThis, const Vector* pVecPosition)
{
    // Game code still works for parented beams; without a parent it would loop forever, so it's skipped and finished in Post.
    if (pThis->m_CBodyComponent()->m_pSceneNode()->m_pParent())
        return { KHook::Action::Ignore };

    return { KHook::Action::Supersede };
}

KHook::Return<void> CBeamCrashFix::CBeam_SetBeamOriginPost(CBeam* pThis, const Vector* pVecPosition)
{
    pThis->m_CBodyComponent()->m_pSceneNode()->m_vecAbsOrigin() = *pVecPosition;

    return { KHook::Action::Ignore };
}

KHook::Return<void> CBeamCrashFix::CBeam_SetBeamEndPos(CBeam* pThis, const Vector* pVecPosition)
{
    // Same as CBeam_SetBeamOrigin.
    if (pThis->m_CBodyComponent()->m_pSceneNode()->m_pParent())
        return { KHook::Action::Ignore };

    return { KHook::Action::Supersede };
}

KHook::Return<void> CBeamCrashFix::CBeam_SetBeamEndPosPost(CBeam* pThis, const Vector* pVecPosition)
{
    pThis->m_vecEndPos() = *reinterpret_cast<const VectorWS*>(pVecPosition);

    return { KHook::Action::Ignore };
}
