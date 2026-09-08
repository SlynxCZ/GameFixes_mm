//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef CBEAM_H
#define CBEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "CBaseModelEntity.h"

#include <mathlib/vectorws.h>

class CBeam : public CBaseModelEntity
{
public:
    SCHEMA_FIELD(VectorWS, CBeam, m_vecEndPos);
};

#endif // CBEAM_H
