//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef INPUTDATA_H
#define INPUTDATA_H
#ifdef _WIN32
#pragma once
#endif

class CEntityInstance;

struct InputData_t
{
    CEntityInstance* pActivator;
    CEntityInstance* pCaller;
};

#endif // INPUTDATA_H
