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

#include "rampbug.h"

#include "sdk/CBasePlayerPawn.h"
#include "sdk/CCSPlayer_MovementServices.h"

#include "dynlibutils/module.hpp"

#include <const.h>
#include <eiface.h>
#include <globalvars.h>
#include <mathlib/mathlib.h>

#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace DynLibUtils;

namespace
{
    constexpr int MAX_BUMPS = 4;
    constexpr float RAMP_PIERCE_DISTANCE = 0.0625f;
    constexpr float RAMP_BUG_THRESHOLD = 0.98f;
    constexpr float RAMP_BUG_VELOCITY_THRESHOLD = 0.95f;
    constexpr float NEW_RAMP_THRESHOLD = 0.95f;

    // The movement pass's own filter: the player and whatever it owns pass through, everything the player collides with counts.
    struct CTraceFilterPlayerMovementCS : public CTraceFilter
    {
        explicit CTraceFilterPlayerMovementCS(CBasePlayerPawn* pPawn) :
            CTraceFilter(pPawn, GameEntitySystem()->GetEntityInstance(pPawn->m_hOwnerEntity()), pPawn->m_Collision().m_collisionAttribute().m_nHierarchyId(),
                         pPawn->m_pCollision()->m_collisionAttribute().m_nInteractsWith(), COLLISION_GROUP_PLAYER, true)
        {
            EnableInteractsAsLayer(LAYER_INDEX_CONTENTS_PLAYER);
            m_nObjectSetMask = RNQUERY_OBJECTS_ALL;
            m_bHitSolid = true;
            m_bHitSolidRequiresGenerateContacts = true;
            m_bHitTrigger = false;
            m_bShouldIgnoreDisabledPairs = true;
            m_bIgnoreIfBothInteractWithHitboxes = false;
            m_bForceHitEverything = false;
            m_bUnknown = true;
        }
    };

    void ClipVelocity(const Vector& in, const Vector& normal, Vector& out)
    {
        float backoff = -((in.x * normal.x) + ((normal.z * in.z) + (in.y * normal.y)));
        backoff = fmaxf(backoff, 0.0f) + 0.03125f;

        out = normal * backoff + in;
    }
}

void CRampbugFix::PlayerState::Reset()
{
    pMoveData = nullptr;
    bProcessingMovement = false;
    bPreviousOnGround = false;
    bDidTPM = false;
    bOverrideTPM = false;
    vecTPMVelocity = vec3_invalid;
    vecTPMOrigin = vec3_invalid;
    vecLastValidPlane = vec3_origin;
    vecTakeoffVelocity = vec3_origin;
    vecLandingOrigin = vec3_origin;
    vecLandingVelocity = vec3_origin;
}

CRampbugFix::CRampbugFix() :
    KHOOK_NEW(m_hProcessMovement, this, &CRampbugFix::CCSPlayer_MovementServices_ProcessMovement, &CRampbugFix::CCSPlayer_MovementServices_ProcessMovementPost),
    KHOOK_NEW(m_hTryPlayerMove, this, &CRampbugFix::CCSPlayer_MovementServices_TryPlayerMove, &CRampbugFix::CCSPlayer_MovementServices_TryPlayerMovePost),
    KHOOK_NEW(m_hCategorizePosition, this, &CRampbugFix::CCSPlayer_MovementServices_CategorizePosition, nullptr)
{
    for (PlayerState& state : m_states)
        state.Reset();
}

bool CRampbugFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    // void TracePlayerBBox(const Vector& start, const Vector& end, const bbox_t& bounds, CTraceFilter* filter, trace_t& pm)
    CMemory pTracePlayerBBox = modules.server.FindPattern(ParseStringPattern(WIN_LINUX("48 8B C4 4C 89 40 ? 48 89 48 ? 55 53 56 57", "55 48 89 E5 41 57 41 56 49 89 D6 41 55 49 89 CD 41 54 53 48 89 F3")));
    if (!pTracePlayerBBox)
    {
        std::snprintf(error, maxlen, "TracePlayerBBox not found");
        return false;
    }

    // void CCSPlayer_MovementServices::ProcessMovement(void* pMove)
    CMemory pProcessMovement = modules.server.FindPattern(ParseStringPattern(WIN_LINUX("40 57 41 57 48 81 EC ? ? ? ? 48 83 79", "55 48 89 E5 41 57 41 56 41 55 49 89 F5 41 54 53 48 89 FB 48 83 EC ? 48 8B 7F")));
    if (!pProcessMovement)
    {
        std::snprintf(error, maxlen, "CCSPlayer_MovementServices::ProcessMovement not found");
        return false;
    }

    // void CCSPlayer_MovementServices::TryPlayerMove(CMoveData* mv, Vector* pFirstDest, trace_t* pFirstTrace, bool* bIsSurfing)
    CMemory pTryPlayerMove = modules.server.FindPattern(ParseStringPattern(WIN_LINUX("48 8B C4 4C 89 48 ? 4C 89 40 ? 48 89 50 ? 48 89 48 ? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70", "48 B8 ? ? ? ? ? ? ? ? 55 66 0F EF C0 48 89 E5 41 57 41 56 49 89 F6")));
    if (!pTryPlayerMove)
    {
        std::snprintf(error, maxlen, "CCSPlayer_MovementServices::TryPlayerMove not found");
        return false;
    }

    // void CCSPlayer_MovementServices::CategorizePosition(CMoveData* mv, bool bStayOnGround)
    CMemory pCategorizePosition = modules.server.FindPattern(ParseStringPattern(WIN_LINUX("40 55 56 57 41 54 41 55 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B F9", "48 B8 ? ? ? ? ? ? ? ? 55 66 0F EF C0 48 89 E5 41 57 41 56 41 55 41 89 D5")));
    if (!pCategorizePosition)
    {
        std::snprintf(error, maxlen, "CCSPlayer_MovementServices::CategorizePosition not found");
        return false;
    }

    m_pfnTracePlayerBBox = pTracePlayerBBox.RCast<decltype(m_pfnTracePlayerBBox)>();

    m_hProcessMovement->Configure(pProcessMovement.GetPtr());
    m_hTryPlayerMove->Configure(pTryPlayerMove.GetPtr());
    m_hCategorizePosition->Configure(pCategorizePosition.GetPtr());

    Log("hooked ProcessMovement (%p), TryPlayerMove (%p) and CategorizePosition (%p); TracePlayerBBox at %p", pProcessMovement.GetPtr(), pTryPlayerMove.GetPtr(), pCategorizePosition.GetPtr(), pTracePlayerBBox.GetPtr());
    return true;
}

void CRampbugFix::Unload()
{
    delete m_hProcessMovement;
    delete m_hTryPlayerMove;
    delete m_hCategorizePosition;
    m_hProcessMovement = nullptr;
    m_hTryPlayerMove = nullptr;
    m_hCategorizePosition = nullptr;
    m_pfnTracePlayerBBox = nullptr;
}

void CRampbugFix::OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName)
{
    for (PlayerState& state : m_states)
        state.Reset();
}

CRampbugFix::PlayerState* CRampbugFix::GetState(CCSPlayer_MovementServices* pServices, CBasePlayerPawn** ppPawn)
{
    CBasePlayerPawn* pPawn = pServices ? pServices->GetPawn() : nullptr;
    if (!pPawn)
        return nullptr;

    CBasePlayerController* pController = pPawn->GetController();
    if (!pController)
        return nullptr;

    // Controllers sit at entity index 1..64, one per slot.
    const int nSlot = pController->GetEntityIndex().Get() - 1;
    if (nSlot < 0 || nSlot >= 64)
        return nullptr;

    if (ppPawn)
        *ppPawn = pPawn;

    return &m_states[nSlot];
}

void CRampbugFix::GetOrigin(const PlayerState& state, CBasePlayerPawn* pPawn, Vector& vecOut)
{
    vecOut = pPawn->m_CBodyComponent()->m_pSceneNode()->m_vecAbsOrigin();
}

void CRampbugFix::GetVelocity(const PlayerState& state, CBasePlayerPawn* pPawn, Vector& vecOut)
{
    if (state.bProcessingMovement && state.pMoveData)
        vecOut = state.pMoveData->m_vecVelocity;
    else
        vecOut = pPawn->m_vecAbsVelocity();
}

void CRampbugFix::SetOrigin(PlayerState& state, CBasePlayerPawn* pPawn, const Vector& vecOrigin)
{
    // Only ever reached from inside a pass, where the move data is what the game reads back.
    if (state.bProcessingMovement && state.pMoveData)
        state.pMoveData->m_vecAbsOrigin = vecOrigin;
    else
        pPawn->m_CBodyComponent()->m_pSceneNode()->m_vecAbsOrigin() = vecOrigin;
}

void CRampbugFix::SetVelocity(PlayerState& state, CBasePlayerPawn* pPawn, const Vector& vecVelocity)
{
    if (state.bProcessingMovement && state.pMoveData)
        state.pMoveData->m_vecVelocity = vecVelocity;
    else
        pPawn->m_vecAbsVelocity() = vecVelocity;
}

void CRampbugFix::GetBounds(CCSPlayer_MovementServices* pServices, bbox_t& bounds)
{
    bounds.mins = Vector(-16.0f, -16.0f, 0.0f);
    bounds.maxs = Vector(16.0f, 16.0f, pServices->m_bDucked() ? 54.0f : 72.0f);
}

void CRampbugFix::RegisterLanding(PlayerState& state, CBasePlayerPawn* pPawn, const Vector& vecVelocity)
{
    if (state.bProcessingMovement && state.pMoveData)
        state.vecLandingOrigin = state.pMoveData->m_vecAbsOrigin;
    else
        GetOrigin(state, pPawn, state.vecLandingOrigin);

    state.vecLandingVelocity = vecVelocity;
}

// Landing on a slope: the game clips the landing velocity against the slope
// and loses speed the player should keep.
void CRampbugFix::ApplySlopeFix(PlayerState& state, CBasePlayerPawn* pPawn)
{
    if (!state.bProcessingMovement || !state.pMoveData)
        return;

    bbox_t bounds;
    GetBounds(static_cast<CCSPlayer_MovementServices*>(pPawn->m_pMovementServices()), bounds);

    CTraceFilterPlayerMovementCS filter(pPawn);
    Vector ground = state.pMoveData->m_vecAbsOrigin;
    ground.z -= 2.0f;
    trace_t trace;

    m_pfnTracePlayerBBox(state.pMoveData->m_vecAbsOrigin, ground, bounds, &filter, trace);

    if (trace.m_bStartInSolid || trace.m_flFraction == 1.0f || trace.m_vHitNormal.z < 0.7f || trace.m_vHitNormal.z >= 1.0f)
        return;

    Vector newVelocity;
    const float backoff = DotProduct(state.vecLandingVelocity, trace.m_vHitNormal);

    for (int i = 0; i < 3; i++)
    {
        const float change = trace.m_vHitNormal[i] * backoff;
        newVelocity[i] = state.vecLandingVelocity[i] - change;
    }

    const float adjust = DotProduct(newVelocity, trace.m_vHitNormal);
    if (adjust < 0.0f)
        newVelocity -= trace.m_vHitNormal * adjust;

    if (newVelocity.Length2D() >= state.vecLandingVelocity.Length2D())
    {
        state.pMoveData->m_vecVelocity.x = newVelocity.x;
        state.pMoveData->m_vecVelocity.y = newVelocity.y;
        state.vecLandingVelocity.x = newVelocity.x;
        state.vecLandingVelocity.y = newVelocity.y;
    }
}

bool CRampbugFix::IsValidMovementTrace(trace_t& trace, const bbox_t& bounds, CTraceFilter* pFilter)
{
    if (trace.m_bStartInSolid)
        return false;

    // Hit something, but no plane to show for it.
    if (trace.m_flFraction < 1.0f && fabsf(trace.m_vHitNormal.x) < FLT_EPSILON && fabsf(trace.m_vHitNormal.y) < FLT_EPSILON && fabsf(trace.m_vHitNormal.z) < FLT_EPSILON)
        return false;

    // A deformed plane.
    if (fabsf(trace.m_vHitNormal.x) > 1.0f || fabsf(trace.m_vHitNormal.y) > 1.0f || fabsf(trace.m_vHitNormal.z) > 1.0f)
        return false;

    // An unswept trace and a backward one, to be sure.
    trace_t stuck;
    m_pfnTracePlayerBBox(trace.m_vEndPos, trace.m_vEndPos, bounds, pFilter, stuck);
    if (stuck.m_bStartInSolid || stuck.m_flFraction < 1.0f - FLT_EPSILON)
        return false;

    // Since the Call to Arms update a trace can hit in one direction only, so
    // the backward fraction is not checked.
    m_pfnTracePlayerBBox(trace.m_vEndPos, trace.m_vStartPos, bounds, pFilter, stuck);
    if (stuck.m_bStartInSolid)
        return false;

    return true;
}

KHook::Return<void> CRampbugFix::CCSPlayer_MovementServices_ProcessMovement(CCSPlayer_MovementServices* pThis, void* pMove)
{
    CBasePlayerPawn* pPawn = nullptr;
    PlayerState* pState = GetState(pThis, &pPawn);
    if (!pState)
        return { KHook::Action::Ignore };

    pState->pMoveData = static_cast<CMoveData*>(pMove);
    pState->bDidTPM = false;
    pState->bProcessingMovement = true;

    const bool bOnGround = (pPawn->m_fFlags() & FL_ONGROUND) != 0;
    if (!pState->bPreviousOnGround && bOnGround)
    {
        Vector velocity;
        GetVelocity(*pState, pPawn, velocity);
        RegisterLanding(*pState, pPawn, velocity);
        ApplySlopeFix(*pState, pPawn);
    }
    else if (pState->bPreviousOnGround && !bOnGround)
    {
        GetVelocity(*pState, pPawn, pState->vecTakeoffVelocity);
    }

    return { KHook::Action::Ignore };
}

KHook::Return<void> CRampbugFix::CCSPlayer_MovementServices_ProcessMovementPost(CCSPlayer_MovementServices* pThis, void* pMove)
{
    CBasePlayerPawn* pPawn = nullptr;
    PlayerState* pState = GetState(pThis, &pPawn);
    if (!pState)
        return { KHook::Action::Ignore };

    if (!pState->bDidTPM)
        pState->vecLastValidPlane = vec3_origin;

    pState->bProcessingMovement = false;
    pState->pMoveData = nullptr;
    pState->bPreviousOnGround = (pPawn->m_fFlags() & FL_ONGROUND) != 0;

    return { KHook::Action::Ignore };
}

// The pass's own walk along the ramp, redone with every trace nudged off the
// last valid plane, so a trace that stops a hair short of the ramp (or on
// its edge) cannot hand the game a plane the player never hit.
KHook::Return<void> CRampbugFix::CCSPlayer_MovementServices_TryPlayerMove(CCSPlayer_MovementServices* pThis, CMoveData* pMove, Vector* pFirstDest, trace_t* pFirstTrace, bool* pIsSurfing)
{
    CBasePlayerPawn* pPawn = nullptr;
    PlayerState* pState = GetState(pThis, &pPawn);
    if (!pState)
        return { KHook::Action::Ignore };

    PlayerState& state = *pState;
    state.bOverrideTPM = false;
    state.bDidTPM = true;

    const CGlobalVars* pGlobals = g_SMAPI->GetCGlobals();
    if (!pGlobals)
        return { KHook::Action::Ignore };

    float timeLeft = pGlobals->frametime;

    Vector start, velocity, end;
    GetOrigin(state, pPawn, start);
    GetVelocity(state, pPawn, velocity);

    if (velocity.Length() == 0.0f)
        return { KHook::Action::Ignore };

    const Vector primalVelocity = velocity;
    bool validPlane = false;

    float allFraction = 0.0f;
    trace_t pm;
    unsigned bumpCount = 0;
    Vector planes[5];
    unsigned numPlanes = 0;
    trace_t pierce;

    bbox_t bounds;
    GetBounds(pThis, bounds);

    CTraceFilterPlayerMovementCS filter(pPawn);

    bool potentiallyStuck = false;

    for (bumpCount = 0; bumpCount < MAX_BUMPS; bumpCount++)
    {
        // Assume the whole way from the origin to the end point is free.
        VectorMA(start, timeLeft, velocity, end);

        if (pFirstDest && end == *pFirstDest)
        {
            pm = *pFirstTrace;
        }
        else
        {
            m_pfnTracePlayerBBox(start, end, bounds, &filter, pm);
            if (end == start)
                continue;

            if (IsValidMovementTrace(pm, bounds, &filter) && pm.m_flFraction == 1.0f)
                break;  // nothing in the way

            if (state.vecLastValidPlane.Length() > FLT_EPSILON
                && (!IsValidMovementTrace(pm, bounds, &filter) || pm.m_vHitNormal.Dot(state.vecLastValidPlane) < RAMP_BUG_THRESHOLD
                    || (potentiallyStuck && pm.m_flFraction == 0.0f)))
            {
                // A plane that would change the velocity a lot: make sure it is a real one.
                Vector offsetDirection;
                const float offsets[] = {0.0f, -1.0f, 1.0f};
                bool success = false;
                for (unsigned i = 0; i < 3 && !success; i++)
                {
                    for (unsigned j = 0; j < 3 && !success; j++)
                    {
                        for (unsigned k = 0; k < 3 && !success; k++)
                        {
                            if (i == 0 && j == 0 && k == 0)
                            {
                                offsetDirection = state.vecLastValidPlane;
                            }
                            else
                            {
                                offsetDirection = Vector(offsets[i], offsets[j], offsets[k]);
                                if (state.vecLastValidPlane.Dot(offsetDirection) <= 0.0f)
                                    continue;

                                trace_t test;
                                m_pfnTracePlayerBBox(start + offsetDirection * RAMP_PIERCE_DISTANCE, start, bounds, &filter, test);
                                if (!IsValidMovementTrace(test, bounds, &filter))
                                    continue;
                            }

                            bool goodTrace = false;
                            float ratio = 0.0f;
                            bool hitNewPlane = false;
                            for (ratio = 0.25f; ratio <= 1.0f; ratio += 0.25f)
                            {
                                m_pfnTracePlayerBBox(start + offsetDirection * RAMP_PIERCE_DISTANCE * ratio, end + offsetDirection * RAMP_PIERCE_DISTANCE * ratio, bounds, &filter, pierce);
                                if (!IsValidMovementTrace(pierce, bounds, &filter))
                                    continue;

                                // Until a similar plane is hit.
                                validPlane = pierce.m_flFraction < 1.0f && pierce.m_flFraction > 0.1f && pierce.m_vHitNormal.Dot(state.vecLastValidPlane) >= RAMP_BUG_THRESHOLD;
                                hitNewPlane = pm.m_vHitNormal.Dot(pierce.m_vHitNormal) < NEW_RAMP_THRESHOLD && state.vecLastValidPlane.Dot(pierce.m_vHitNormal) > NEW_RAMP_THRESHOLD;
                                goodTrace = CloseEnough(pierce.m_flFraction, 1.0f, FLT_EPSILON) || validPlane;
                                if (goodTrace)
                                    break;
                            }

                            if (goodTrace || hitNewPlane)
                            {
                                // Back to the original end point, for its normal.
                                trace_t test;
                                m_pfnTracePlayerBBox(pierce.m_vEndPos, end, bounds, &filter, test);
                                pm = pierce;
                                pm.m_vStartPos = start;
                                pm.m_flFraction = clamp((pierce.m_vEndPos - pierce.m_vStartPos).Length() / (end - start).Length(), 0.0f, 1.0f);
                                pm.m_vEndPos = test.m_vEndPos;
                                if (pierce.m_vHitNormal.Length() > 0.0f)
                                {
                                    pm.m_vHitNormal = pierce.m_vHitNormal;
                                    state.vecLastValidPlane = pierce.m_vHitNormal;
                                }
                                else
                                {
                                    pm.m_vHitNormal = test.m_vHitNormal;
                                    state.vecLastValidPlane = test.m_vHitNormal;
                                }
                                success = true;
                                state.bOverrideTPM = true;
                            }
                        }
                    }
                }
            }

            if (pm.m_vHitNormal.Length() > 0.99f)
                state.vecLastValidPlane = pm.m_vHitNormal;

            potentiallyStuck = pm.m_flFraction == 0.0f;
        }

        if (pm.m_flFraction * velocity.Length() > 0.03125f || pm.m_flFraction > 0.03125f)
        {
            allFraction += pm.m_flFraction;
            start = pm.m_vEndPos;
            numPlanes = 0;
        }

        if (allFraction == 1.0f)
            break;

        timeLeft -= pGlobals->frametime * pm.m_flFraction;

        if (numPlanes >= 5 || (pm.m_vHitNormal.z >= 0.7f && velocity.Length2D() < 1.0f))
        {
            velocity = vec3_origin;
            break;
        }

        planes[numPlanes] = pm.m_vHitNormal;
        numPlanes++;

        if (numPlanes == 1 && pPawn->m_MoveType() == MOVETYPE_WALK && GameEntitySystem()->GetEntityInstance(pPawn->m_hGroundEntity()) == nullptr)
        {
            ClipVelocity(velocity, planes[0], velocity);
        }
        else
        {
            unsigned i, j;
            for (i = 0; i < numPlanes; i++)
            {
                ClipVelocity(velocity, planes[i], velocity);
                for (j = 0; j < numPlanes; j++)
                {
                    if (j != i && velocity.Dot(planes[j]) < 0)
                        break;  // moving against this plane now
                }

                if (j == numPlanes)
                    break;  // no clip needed
            }

            if (i == numPlanes)
            {
                // Along the crease.
                if (numPlanes != 2)
                {
                    velocity = vec3_origin;
                    break;
                }

                Vector dir;
                CrossProduct(planes[0], planes[1], dir);
                dir.NormalizeInPlace();
                const float d = dir.Dot(velocity);
                VectorScale(dir, d, velocity);

                if (velocity.Dot(primalVelocity) <= 0)
                {
                    velocity = vec3_origin;
                    break;
                }
            }
        }
    }

    state.vecTPMOrigin = pm.m_vEndPos;
    state.vecTPMVelocity = velocity;

    return { KHook::Action::Ignore };
}

KHook::Return<void> CRampbugFix::CCSPlayer_MovementServices_TryPlayerMovePost(CCSPlayer_MovementServices* pThis, CMoveData* pMove, Vector* pFirstDest, trace_t* pFirstTrace, bool* pIsSurfing)
{
    CBasePlayerPawn* pPawn = nullptr;
    PlayerState* pState = GetState(pThis, &pPawn);
    if (!pState)
        return { KHook::Action::Ignore };

    Vector velocity;
    GetVelocity(*pState, pPawn, velocity);

    // The game took the player somewhere our own walk did not: put ours back.
    const bool velocityHeavilyModified = pState->vecTPMVelocity.Normalized().Dot(velocity.Normalized()) < RAMP_BUG_THRESHOLD
        || (pState->vecTPMVelocity.Length() > 50.0f && velocity.Length() / pState->vecTPMVelocity.Length() < RAMP_BUG_VELOCITY_THRESHOLD);

    if (pState->bOverrideTPM && velocityHeavilyModified && pState->vecTPMOrigin != vec3_invalid && pState->vecTPMVelocity != vec3_invalid)
    {
        SetOrigin(*pState, pPawn, pState->vecTPMOrigin);
        SetVelocity(*pState, pPawn, pState->vecTPMVelocity);
    }

    return { KHook::Action::Ignore };
}

// Going down a ramp fast, the ground check can find a standable plane the
// player is not really on; nudge them onto the last valid plane first.
KHook::Return<void> CRampbugFix::CCSPlayer_MovementServices_CategorizePosition(CCSPlayer_MovementServices* pThis, CMoveData* pMove, bool bStayOnGround)
{
    CBasePlayerPawn* pPawn = nullptr;
    PlayerState* pState = GetState(pThis, &pPawn);
    if (!pState)
        return { KHook::Action::Ignore };

    PlayerState& state = *pState;

    // Already colliding with a standable, valid plane: nothing to check.
    if (bStayOnGround || state.vecLastValidPlane.Length() < 0.000001f || state.vecLastValidPlane.z > 0.7f)
        return { KHook::Action::Ignore };

    Vector velocity;
    GetVelocity(state, pPawn, velocity);

    // Only worth it going down at some speed.
    if (velocity.z > -64.0f)
        return { KHook::Action::Ignore };

    bbox_t bounds;
    GetBounds(pThis, bounds);

    CTraceFilterPlayerMovementCS filter(pPawn);
    trace_t trace;

    Vector origin, groundOrigin;
    GetOrigin(state, pPawn, origin);
    groundOrigin = origin;
    groundOrigin.z -= 2.0f;

    m_pfnTracePlayerBBox(origin, groundOrigin, bounds, &filter, trace);

    if (trace.m_flFraction == 1.0f)
        return { KHook::Action::Ignore };

    // Something the player could actually stand on?
    if (trace.m_flFraction < 0.95f && trace.m_vHitNormal.z > 0.7f && state.vecLastValidPlane.Dot(trace.m_vHitNormal) < RAMP_BUG_THRESHOLD)
    {
        origin += state.vecLastValidPlane * 0.0625f;
        groundOrigin = origin;
        groundOrigin.z -= 2.0f;
        m_pfnTracePlayerBBox(origin, groundOrigin, bounds, &filter, trace);

        if (trace.m_bStartInSolid)
            return { KHook::Action::Ignore };

        if (trace.m_flFraction == 1.0f || state.vecLastValidPlane.Dot(trace.m_vHitNormal) >= RAMP_BUG_THRESHOLD)
            SetOrigin(state, pPawn, origin);
    }

    return { KHook::Action::Ignore };
}
