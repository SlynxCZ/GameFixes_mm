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

//=============================================================================
//
// Purpose: Minimal schema field access for standalone Metamod plugins
//
// Usage:
//   class CBaseEntity : public CEntityInstance
//   {
//   public:
//       SCHEMA_FIELD(int32_t, CBaseEntity, m_iHealth);
//   };
//
//   pEntity->m_iHealth() = 100;               write (no network update by itself)
//   pEntity->m_iHealth.NetworkStateChanged(); replicate it to clients
//
// className must be the schema class that declares the field (it is also the
// C++ class the macro sits in). A class or field missing from the schema is
// reported once and reads/writes scratch memory instead of the object's.
//
//=============================================================================

#ifndef SCHEMASYSTEM_HELPER_H
#define SCHEMASYSTEM_HELPER_H
#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <entity2/entityinstance.h>

namespace schema
{
    // Offset of pszClassName::pszFieldName, or -1 (with a warning) when it does not exist.
    int32_t GetOffset(const char* pszClassName, const char* pszFieldName);

    // The C++ name of T at compile time ("CChicken"); schema class names are the C++ ones.
    template <typename T>
    constexpr std::string_view ClassName()
    {
#ifdef _MSC_VER
        // "... schema::ClassName<class CChicken>(void)"
        std::string_view name = __FUNCSIG__;
        name.remove_prefix(name.find("ClassName<") + sizeof("ClassName<") - 1);
        name = name.substr(0, name.rfind(">(void)"));
        for (std::string_view prefix : {"class ", "struct "})
        {
            if (name.substr(0, prefix.size()) == prefix)
                name.remove_prefix(prefix.size());
        }
#else
        // Clang: "... [T = CChicken]", GCC: "... [with T = CChicken; ...]"
        std::string_view name = __PRETTY_FUNCTION__;
        name.remove_prefix(name.find("T = ") + sizeof("T = ") - 1);
        name = name.substr(0, name.find_first_of(";]"));
#endif
        return name;
    }

    // ClassName<T>() as a null-terminated string, for the functions below.
    template <typename T>
    const char* ClassNameCStr()
    {
        static const std::string s_className(ClassName<T>());
        return s_className.c_str();
    }

    // Each function taking a class name also takes the class as a type or as a
    // pointer (its static type): GetOffset<CChicken>("m_leader"), GetOffset(this, "m_leader").
    template <typename T>
    int32_t GetOffset(const char* pszFieldName) { return GetOffset(ClassNameCStr<T>(), pszFieldName); }
    template <typename T>
    int32_t GetOffset(const T*, const char* pszFieldName) { return GetOffset<T>(pszFieldName); }

    // Zeroed scratch memory a missing field resolves to, so nothing outside it
    // gets read or overwritten. Shared by all missing fields, 4 KiB.
    void* GetMissingFieldStorage();

    // Offset of the class' __m_pChainEntity (CNetworkVarChainer), walking up base
    // classes (e.g. CCSGameRules -> ... -> CGameRules). 0 when there is none.
    int32_t GetChainOffset(const char* pszClassName);
    template <typename T>
    int32_t GetChainOffset() { return GetChainOffset(ClassNameCStr<T>()); }
    template <typename T>
    int32_t GetChainOffset(const T*) { return GetChainOffset<T>(); }

    // How NetworkStateChanged() reaches the engine for one field.
    struct NetworkInfo_t
    {
        bool resolved = false;  // false until the entity system exists; resolve again later
        bool networked = false; // plenty of schema fields never leave the server
        int32_t offset = -1;
        int32_t chainOffset = 0;
    };

    NetworkInfo_t GetNetworkInfo(const char* pszClassName, const char* pszFieldName);
    template <typename T>
    NetworkInfo_t GetNetworkInfo(const char* pszFieldName) { return GetNetworkInfo(ClassNameCStr<T>(), pszFieldName); }
    template <typename T>
    NetworkInfo_t GetNetworkInfo(const T*, const char* pszFieldName) { return GetNetworkInfo<T>(pszFieldName); }

    void EntityNetworkStateChanged(CEntityInstance* pEntity, int32_t nOffset);
    void ChainNetworkStateChanged(void* pNetworkVarChainer, int32_t nOffset);

    // For an object embedded in an entity (CEconItemView, CCollisionProperty, ...):
    // the engine wraps it in a NetworkVar_<field> class whose NetworkStateChanged
    // virtual forwards to the owner. Its vtable slot is found at runtime from the
    // object's own vtable. A plain object that is not embedded has none: no-op.
    void EmbeddedNetworkStateChanged(void* pObject, int32_t nOffset);

    // Routes the notification: chain entity if the class has one, the entity itself
    // for entity classes, otherwise the embedded object's NetworkVar wrapper.
    template <typename Class>
    void NetworkStateChanged(uintptr_t pObject, NetworkInfo_t& info, const char* pszClassName, const char* pszFieldName)
    {
        if (!info.resolved)
            info = GetNetworkInfo(pszClassName, pszFieldName);

        if (!info.resolved || !info.networked)
            return;

        if (info.chainOffset)
            ChainNetworkStateChanged(reinterpret_cast<void*>(pObject + info.chainOffset), info.offset);
        else if constexpr (std::is_base_of_v<CEntityInstance, Class>)
            EntityNetworkStateChanged(reinterpret_cast<CEntityInstance*>(pObject), info.offset);
        else
            EmbeddedNetworkStateChanged(reinterpret_cast<void*>(pObject), info.offset);
    }
}

// CNetworkVarChainer is opaque to the SDK, so this only mirrors the members we need.
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

#define _SCHEMA_FIELD_COMMON(type, className, propName)                                                  \
    /* This points at the propName member; walk back to the owning className */                       \
    uintptr_t GetOuterThis() { return reinterpret_cast<uintptr_t>(this) - offsetof(className, propName); } \
                                                                                                           \
    std::add_pointer_t<type> GetPointer()                                                                  \
    {                                                                                                      \
        static const int32_t s_nOffset = schema::GetOffset(#className, #propName);                         \
        if (s_nOffset < 0)                                                                                 \
            return reinterpret_cast<std::add_pointer_t<type>>(schema::GetMissingFieldStorage());           \
        return reinterpret_cast<std::add_pointer_t<type>>(GetOuterThis() + s_nOffset);                     \
    }                                                                                                      \
                                                                                                           \
public:                                                                                                    \
    propName##_prop() = default;                                                                           \
    /* Prevent accidentally copying this wrapper instead of the underlying field */                        \
    propName##_prop(const propName##_prop&) = delete;                                                      \
    propName##_prop& operator=(const propName##_prop&) = delete;                                           \
                                                                                                           \
    /* Call after writing through the accessor; no-op for fields that are not networked */                 \
    void NetworkStateChanged()                                                                             \
    {                                                                                                      \
        static schema::NetworkInfo_t s_info;                                                               \
        schema::NetworkStateChanged<className>(GetOuterThis(), s_info, #className, #propName);             \
    }

// propName() returns a reference to the field.
#define SCHEMA_FIELD(type, className, propName)                  \
    class propName##_prop                                        \
    {                                                            \
        _SCHEMA_FIELD_COMMON(type, className, propName)          \
                                                                 \
        std::add_lvalue_reference_t<type> operator()()           \
        {                                                        \
            return *GetPointer();                                \
        }                                                        \
    } propName;

// propName() returns a pointer to the field.
#define SCHEMA_FIELD_POINTER(type, className, propName)          \
    class propName##_prop                                        \
    {                                                            \
        _SCHEMA_FIELD_COMMON(type, className, propName)          \
                                                                 \
        std::add_pointer_t<type> operator()()                    \
        {                                                        \
            return GetPointer();                                 \
        }                                                        \
    } propName;

#endif // SCHEMASYSTEM_HELPER_H
