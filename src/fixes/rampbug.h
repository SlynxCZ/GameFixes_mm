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

// Surf ramps: a movement trace that stops a hair short of a ramp, or lands on
// its edge, hands TryPlayerMove a plane the player never really hit, and the
// player is flung off -- a rampbug. Every trace of the pass is redone from a
// small offset along the last plane the player was known to be on, and a
// result the game then mangles is put back where it belongs; CategorizePosition
// gets the same treatment on the way down a ramp. CS2KZ's fix, as carried by
// CS2Fixes-RampbugFix (github.com/Interesting-exe/CS2Fixes-RampbugFix). It
// lessens rampbugs, it does not end them.
#pragma once

#include "fix.h"
#include "plugin.h"
#include "utils.hpp"

#include "sdk/CMoveData.h"

#include <gametrace.h>
#include <mathlib/vector.h>

class CBasePlayerPawn;
class CCSPlayer_MovementServices;
struct bbox_t;

class CRampbugFix final : public CFix
{
public:
    CRampbugFix();

    const char* GetName() const override { return "rampbug"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) override;

public: // Hooks
    KHook::Return<void> CCSPlayer_MovementServices_ProcessMovement(CCSPlayer_MovementServices* pThis, void* pMove);
    KHook::Return<void> CCSPlayer_MovementServices_ProcessMovementPost(CCSPlayer_MovementServices* pThis, void* pMove);
    KHook::Return<void> CCSPlayer_MovementServices_TryPlayerMove(CCSPlayer_MovementServices* pThis, CMoveData* pMove, Vector* pFirstDest, trace_t* pFirstTrace, bool* pIsSurfing);
    KHook::Return<void> CCSPlayer_MovementServices_TryPlayerMovePost(CCSPlayer_MovementServices* pThis, CMoveData* pMove, Vector* pFirstDest, trace_t* pFirstTrace, bool* pIsSurfing);
    KHook::Return<void> CCSPlayer_MovementServices_CategorizePosition(CCSPlayer_MovementServices* pThis, CMoveData* pMove, bool bStayOnGround);

    KHook::Member<CCSPlayer_MovementServices, void, void*>* m_hProcessMovement = nullptr;
    KHook::Member<CCSPlayer_MovementServices, void, CMoveData*, Vector*, trace_t*, bool*>* m_hTryPlayerMove = nullptr;
    KHook::Member<CCSPlayer_MovementServices, void, CMoveData*, bool>* m_hCategorizePosition = nullptr;

private:
    // What one player's movement pass left behind, from one call to the next.
    struct PlayerState
    {
        CMoveData* pMoveData = nullptr;       // the pass in flight, null outside ProcessMovement
        bool bProcessingMovement = false;
        bool bPreviousOnGround = false;
        bool bDidTPM = false;                 // TryPlayerMove ran during this pass
        bool bOverrideTPM = false;            // our own trace result should replace the game's
        Vector vecTPMVelocity;                // vec3_invalid until TryPlayerMove ran
        Vector vecTPMOrigin;
        Vector vecLastValidPlane;             // vec3_origin when there is none
        Vector vecTakeoffVelocity;
        Vector vecLandingOrigin;
        Vector vecLandingVelocity;

        void Reset();
    };

    // The state of the pawn a movement service belongs to; null for anything that is not a player.
    PlayerState* GetState(CCSPlayer_MovementServices* pServices, CBasePlayerPawn** ppPawn);

    // The player's origin and velocity as the pass sees them: the move data while a
    // pass is in flight, the pawn otherwise.
    static void GetOrigin(const PlayerState& state, CBasePlayerPawn* pPawn, Vector& vecOut);
    static void GetVelocity(const PlayerState& state, CBasePlayerPawn* pPawn, Vector& vecOut);
    static void SetOrigin(PlayerState& state, CBasePlayerPawn* pPawn, const Vector& vecOrigin);
    static void SetVelocity(PlayerState& state, CBasePlayerPawn* pPawn, const Vector& vecVelocity);
    static void GetBounds(CCSPlayer_MovementServices* pServices, bbox_t& bounds);

    void RegisterLanding(PlayerState& state, CBasePlayerPawn* pPawn, const Vector& vecVelocity);
    void ApplySlopeFix(PlayerState& state, CBasePlayerPawn* pPawn);
    bool IsValidMovementTrace(trace_t& trace, const bbox_t& bounds, CTraceFilter* pFilter);

    // void TracePlayerBBox(const Vector& start, const Vector& end, const bbox_t& bounds, CTraceFilter* filter, trace_t& pm)
    void (*m_pfnTracePlayerBBox)(const Vector&, const Vector&, const bbox_t&, CTraceFilter*, trace_t&) = nullptr;

    PlayerState m_states[64];
};
