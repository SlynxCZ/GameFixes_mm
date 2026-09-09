//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef CBASEPLAYERPAWN_H
#define CBASEPLAYERPAWN_H
#ifdef _WIN32
#pragma once
#endif

#include "CBaseModelEntity.h"
#include "CBasePlayerController.h"

class CPlayer_MovementServices;

class CBasePlayerPawn : public CBaseModelEntity
{
public:
    SCHEMA_FIELD(CPlayer_MovementServices*, CBasePlayerPawn, m_pMovementServices);
    SCHEMA_FIELD(CEntityHandle, CBasePlayerPawn, m_hController);

    CBasePlayerController* GetController() { return static_cast<CBasePlayerController*>(GameEntitySystem()->GetEntityInstance(m_hController())); }
};

#endif // CBASEPLAYERPAWN_H
