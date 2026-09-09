//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef CCOLLISIONPROPERTY_H
#define CCOLLISIONPROPERTY_H
#ifdef _WIN32
#pragma once
#endif

#include "schemasystem_helper.h"

struct VPhysicsCollisionAttribute_t
{
    SCHEMA_FIELD(uint8, VPhysicsCollisionAttribute_t, m_nCollisionGroup);
    SCHEMA_FIELD(uint64, VPhysicsCollisionAttribute_t, m_nInteractsAs);
    SCHEMA_FIELD(uint64, VPhysicsCollisionAttribute_t, m_nInteractsWith);
    SCHEMA_FIELD(uint64, VPhysicsCollisionAttribute_t, m_nInteractsExclude);
    SCHEMA_FIELD(uint16, VPhysicsCollisionAttribute_t, m_nHierarchyId);
};

class CCollisionProperty
{
public:
    SCHEMA_FIELD(VPhysicsCollisionAttribute_t, CCollisionProperty, m_collisionAttribute);
    SCHEMA_FIELD(uint8, CCollisionProperty, m_CollisionGroup);
};

#endif // CCOLLISIONPROPERTY_H
