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

// CBeam::SetBeamOrigin / CBeam::SetBeamEndPos loop forever on a beam without
// a parent -- an easy way for a map to crash the server. Pre supersedes that
// path, Post does the one assignment the original would have made; parented
// beams go through the game code untouched.
#pragma once

#include "fix.h"
#include "plugin.h"

#include "sdk/CBeam.h"

class CBeamCrashFix final : public CFix
{
public:
    CBeamCrashFix();

    const char* GetName() const override { return "beam_crash"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

public: // Hooks
    KHook::Return<void> CBeam_SetBeamOrigin(CBeam* pThis, const Vector* pVecPosition);
    KHook::Return<void> CBeam_SetBeamOriginPost(CBeam* pThis, const Vector* pVecPosition);
    KHook::Return<void> CBeam_SetBeamEndPos(CBeam* pThis, const Vector* pVecPosition);
    KHook::Return<void> CBeam_SetBeamEndPosPost(CBeam* pThis, const Vector* pVecPosition);

    KHook::Member<CBeam, void, const Vector*>* m_hSetBeamOrigin = nullptr;
    KHook::Member<CBeam, void, const Vector*>* m_hSetBeamEndPos = nullptr;
};
