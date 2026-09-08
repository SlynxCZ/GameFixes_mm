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

#ifndef SCHEMASYSTEM_HELPER_H
#define SCHEMASYSTEM_HELPER_H
#ifdef _WIN32
#pragma once
#endif

#include <cstdint>
#include <type_traits>
#include <entity2/entityinstance.h>

// Describes a single schema field: where it lives, and whether the engine actually
// replicates it over the network at all (a lot of schema fields never leave the server).
struct SchemaFieldInfo_t
{
    int32_t offset = -1;
    bool networked = false;
};

SchemaFieldInfo_t GetServerPropInfo(const char* pszClassName, const char* pszPropName);

// Offset of the class' __m_pChainEntity (CNetworkVarChainer), walking up base classes
// if it's not declared directly on pszClassName (e.g. CCSGameRules -> ... -> CGameRules).
// Returns 0 if the class doesn't have one.
int32_t GetServerChainOffset(const char* pszClassName);

// CNetworkVarChainer is opaque to the SDK, so this only mirrors the layout of the
// members we actually need (same approach as CS2Fixes' cs2_sdk/schema.h).
class CNetworkVarChainer
{
public:
    CEntityInstance* m_pEntity;

private:
    uint8_t pad_0000[24];

public:
    ChangeAccessorFieldPathIndex_t m_PathIndex;

private:
    uint8_t pad_0024[4];
};

// Notifies the entity directly - use when the field lives straight on a CEntityInstance.
void EntityNetworkStateChanged(uintptr_t pEntity, int32_t nOffset);
// Notifies through a CNetworkVarChainer - use when the field lives on an embedded/chained
// schema class (e.g. CGameRules), so the change gets attributed to the owning entity.
void ChainNetworkStateChanged(uintptr_t pNetworkVarChainer, int32_t nOffset);

// Shared by SCHEMA_FIELD/SCHEMA_FIELD_POINTER: NetworkStateChanged() looks up the field's
// offset + networked status once, then routes the notification through the class' chain
// entity (CNetworkVarChainer) if it has one, straight to the entity otherwise. No-op for
// fields the engine doesn't actually replicate.
#define _SCHEMA_FIELD_NETWORK_STATE_CHANGED(className, propName)                        \
    void NetworkStateChanged()                                                          \
    {                                                                                   \
        static const SchemaFieldInfo_t info = GetServerPropInfo(#className, #propName); \
        static const int32_t chainOffset = GetServerChainOffset(#className);            \
        if (!info.networked)                                                            \
            return;                                                                     \
                                                                                          \
        const uintptr_t pThis = GetOuterThis();                                         \
        if (chainOffset)                                                                \
            ChainNetworkStateChanged(pThis + chainOffset, info.offset);                 \
        else                                                                             \
            EntityNetworkStateChanged(pThis, info.offset);                              \
    }                                                                                    \
                                                                                          \
private:                                                                                 \
    /* this points at the propName sub-object; walk back to the owning className */      \
    uintptr_t GetOuterThis() { return reinterpret_cast<uintptr_t>(this) - offsetof(className, propName); } \
                                                                                          \
    /* Prevent accidentally copying this wrapper class instead of the underlying field */ \
    propName##_prop(const propName##_prop&) = delete;                                   \
                                                                                          \
public:

// Accessor: pInstance->propName() gets you a reference to the field's current value.
// NetVar update: pInstance->propName.NetworkStateChanged() tells the engine to replicate
// it (through the chain entity when there is one) - call it after you write through propName().
#define SCHEMA_FIELD(type, className, propName)                                                          \
    class propName##_prop                                                                                \
    {                                                                                                     \
    public:                                                                                               \
        std::add_lvalue_reference_t<type> operator()()                                                    \
        {                                                                                                  \
            static const int32_t offset = GetServerPropInfo(#className, #propName).offset;                \
            return *reinterpret_cast<std::add_pointer_t<type>>(GetOuterThis() + offset);                  \
        }                                                                                                  \
                                                                                                             \
        _SCHEMA_FIELD_NETWORK_STATE_CHANGED(className, propName)                                          \
    } propName;

// Same as SCHEMA_FIELD, but propName() gets you a pointer to the field instead of a reference.
#define SCHEMA_FIELD_POINTER(type, className, propName)                                                   \
    class propName##_prop                                                                                 \
    {                                                                                                      \
    public:                                                                                                \
        type* operator()()                                                                                 \
        {                                                                                                   \
            static const int32_t offset = GetServerPropInfo(#className, #propName).offset;                 \
            return reinterpret_cast<type*>(GetOuterThis() + offset);                                       \
        }                                                                                                   \
                                                                                                              \
        _SCHEMA_FIELD_NETWORK_STATE_CHANGED(className, propName)                                           \
    } propName;

#endif //SCHEMASYSTEM_HELPER_H
