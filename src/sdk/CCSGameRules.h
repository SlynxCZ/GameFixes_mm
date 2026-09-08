//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef CCSGAMERULES_H
#define CCSGAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "CBaseEntity.h"

class CCSGameRules
{
public:
    SCHEMA_FIELD(int32_t, CCSGameRules, m_iNumSpawnableTerrorist);
    SCHEMA_FIELD(int32_t, CCSGameRules, m_iNumSpawnableCT);
    SCHEMA_FIELD(int32_t, CCSGameRules, m_iMaxNumTerrorists);
    SCHEMA_FIELD(int32_t, CCSGameRules, m_iMaxNumCTs);
};

class CCSGameRulesProxy : public CBaseEntity
{
public:
    SCHEMA_FIELD(CCSGameRules*, CCSGameRulesProxy, m_pGameRules);
};

#endif // CCSGAMERULES_H
