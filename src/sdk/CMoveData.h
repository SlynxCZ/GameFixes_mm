//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: the movement pass's working set, as CCSPlayer_MovementServices
//          hands it to TryPlayerMove / CategorizePosition. Layout only -- the
//          rampbug fix reads and writes m_vecVelocity and m_vecAbsOrigin.
//
//=============================================================================

#ifndef CMOVEDATA_H
#define CMOVEDATA_H
#ifdef _WIN32
#pragma once
#endif

#include <entityhandle.h>
#include <gametrace.h>
#include <mathlib/vector.h>
#include <tier1/utlvector.h>

struct SubtickMove
{
    float when;
    uint64 button;
    bool pressed;
};

struct MovementVector2D
{
    float x;
    float y;
};

struct touchlist_t
{
    Vector deltavelocity;
    trace_t trace;
};

class CMoveDataBase
{
public:
    bool m_bHasZeroFrametime : 1;
    bool m_bIsLateCommand : 1;
    CEntityHandle m_nPlayerHandle;
    QAngle m_vecAbsViewAngles;
    QAngle m_vecViewAngles;
    Vector m_vecLastMovementImpulses;
    float m_flForwardMove;
    float m_flSideMove;  // flipped against CS:GO: moving right is negative
    float m_flUpMove;
    Vector m_vecVelocity;
    QAngle m_vecAngles;
    Vector m_vecUnknown;
    CUtlVector<SubtickMove> m_SubtickMoves;
    CUtlVector<SubtickMove> m_AttackSubtickMoves;
    bool m_bHasSubtickInputs;
    float unknown;  // 1.0 from SetupMove on, never changes
    CUtlVector<touchlist_t> m_TouchList;
    Vector m_collisionNormal;
    Vector m_groundNormal;
    Vector m_vecAbsOrigin;
    int32 m_nTickCount;
    int32 m_nTargetTick;
    float m_flSubtickStartFraction;
    float m_flSubtickEndFraction;
};

class CMoveData : public CMoveDataBase
{
public:
    Vector m_outWishVel;
    QAngle m_vecOldAngles;
    MovementVector2D m_vecWalkWishVel;
    Vector m_vecContinousAcceleration;  // u/s^2
    Vector m_vecFrameVelocityDelta;     // u/s, the half of air acceleration that skips the per-second path
    float m_flMaxSpeed;
};

#endif // CMOVEDATA_H
