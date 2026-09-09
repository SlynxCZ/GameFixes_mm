//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef CCSPLAYER_MOVEMENTSERVICES_H
#define CCSPLAYER_MOVEMENTSERVICES_H
#ifdef _WIN32
#pragma once
#endif

#include "schemasystem_helper.h"
#include "CBasePlayerPawn.h"

class CPlayerPawnComponent
{
public:
    SCHEMA_FIELD(CBasePlayerPawn*, CPlayerPawnComponent, __m_pChainEntity);

    CBasePlayerPawn* GetPawn() { return __m_pChainEntity(); }
};

class CPlayer_MovementServices : public CPlayerPawnComponent
{
};

class CPlayer_MovementServices_Humanoid : public CPlayer_MovementServices
{
};

class CCSPlayer_MovementServices : public CPlayer_MovementServices_Humanoid
{
public:
    SCHEMA_FIELD(bool, CCSPlayer_MovementServices, m_bDucked);
};

#endif // CCSPLAYER_MOVEMENTSERVICES_H
