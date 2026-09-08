//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef CBASEENTITY_H
#define CBASEENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "schemasystem_helper.h"

#include <const.h>
#include <entityinstance.h>
#include <mathlib/vector.h>

class CGameSceneNode
{
public:
    SCHEMA_FIELD(CGameSceneNode*, CGameSceneNode, m_pParent);
    SCHEMA_FIELD(Vector, CGameSceneNode, m_vecAbsOrigin);
};

class CBodyComponent
{
public:
    SCHEMA_FIELD(CGameSceneNode*, CBodyComponent, m_pSceneNode);
};

class CBaseEntity : public CEntityInstance
{
public:
    SCHEMA_FIELD(CBodyComponent*, CBaseEntity, m_CBodyComponent);
    SCHEMA_FIELD(uint32_t, CBaseEntity, m_fFlags);
    SCHEMA_FIELD(MoveType_t, CBaseEntity, m_MoveType);
    SCHEMA_FIELD(MoveType_t, CBaseEntity, m_nActualMoveType);
};

#endif // CBASEENTITY_H
