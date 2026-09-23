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

// The server's CNavPhysicsInterface, reached through its vtable by RTTI name.
// Same declaration order as cs2kz-metamod and source2toolkit use; nothing here
// is instantiated, the vtable pointer is the whole "object".
#ifndef INAVPHYSICSINTERFACE_H
#define INAVPHYSICSINTERFACE_H
#ifdef _WIN32
#pragma once
#endif

#include <gametrace.h>
#include <ray.h>

class CBaseEntity;

class INavPhysicsInterface
{
public:
	virtual ~INavPhysicsInterface() = 0;
	virtual void Nav_TraceLine(const Vector& vStart, const Vector& vEnd, CBaseEntity* pIgnore, uint64 nInteractsWith, uint8 nCollisionGroup, uint8 nObjectSetMask, CGameTrace* trace) = 0;
	virtual void Nav_TraceLine(const Vector& vStart, const Vector& vEnd, CTraceFilter* pFilter, CGameTrace* pTraceOut) = 0;
	virtual void Nav_TraceShape(const Ray_t& ray, const Vector& vStart, const Vector& vEnd, CBaseEntity* pIgnore, uint64 nInteractsWith, uint8 nCollisionGroup, uint8 nObjectSetMask, CGameTrace* trace) = 0;
	virtual void Nav_TraceShape(const Ray_t& ray, const Vector& vStart, const Vector& vEnd, CTraceFilter* pFilter, CGameTrace* trace) = 0;
	virtual uint64 Nav_PointContents(const Vector* const vTestPos, uint64 nContentsMask) = 0;
	virtual bool Nav_CheckAreaOverlappingEntity(const void* const rArea, const CBaseEntity* const rEntity, bool bExtrudeHullHeight) = 0;
	virtual void Nav_GetEntityWorldSpaceAABB(const CBaseEntity* const rEntity, Vector* pMinsOut, Vector* pMaxsOut) = 0;
	virtual void Unk(void*) = 0;
};

#endif // INAVPHYSICSINTERFACE_H
