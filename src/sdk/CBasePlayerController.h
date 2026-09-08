//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef CBASEPLAYERCONTROLLER_H
#define CBASEPLAYERCONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

#include "CBaseEntity.h"

#include <const.h>
#include <entityhandle.h>
#include <entitysystem.h>

enum class PlayerConnectedState : uint32_t
{
    PlayerNeverConnected = 0xFFFFFFFF,
    PlayerConnected = 0x0,
    PlayerConnecting = 0x1,
    PlayerReconnecting = 0x2,
    PlayerDisconnecting = 0x3,
    PlayerDisconnected = 0x4,
    PlayerReserved = 0x5,
};

class CBasePlayerController : public CBaseEntity
{
public:
    SCHEMA_FIELD(PlayerConnectedState, CBasePlayerController, m_iConnected);
    SCHEMA_FIELD(bool, CBasePlayerController, m_bIsHLTV);
    SCHEMA_FIELD(CEntityHandle, CBasePlayerController, m_hPawn);
    SCHEMA_FIELD_POINTER(char, CBasePlayerController, m_iszPlayerName);

    bool IsBot() { return (m_fFlags() & FL_FAKECLIENT) != 0; }
    bool IsConnected() { return m_iConnected() == PlayerConnectedState::PlayerConnected; }
    bool IsHLTV() { return m_bIsHLTV(); }

    CBaseEntity* GetPawn() { return static_cast<CBaseEntity*>(GameEntitySystem()->GetEntityInstance(m_hPawn())); }
    const char* GetPlayerName() { return m_iszPlayerName(); }
};

#endif // CBASEPLAYERCONTROLLER_H
