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

#include <schemasystem/schemasystem.h>
#include "schemasystem_helper.h"
#include "utils.hpp"
#include <cstring>
#include <unordered_map>
#include <utility>

#include <entity2/entitysystem.h>
#include <entity2/entityclass.h>
#include <networksystem/inetworkserializer.h>
#include <dynlibutils/module.hpp>

// The NetworkVar wrapper's NetworkStateChanged starts by checking
// NetworkStateChangedData::m_nPathIndex against -1: `cmp dword ptr [data+0x38], -1`,
// data being the second argument (rdx on Windows, rsi on Linux). No other slot of
// these vtables does that (checked on all NetworkVar_ vtables, 2026-09-23 binaries).
static constexpr const char* g_pszStateChangedSignature = WIN_LINUX("83 7A 38 FF", "83 7E 38 FF");
static constexpr std::size_t g_nStateChangedSearchBytes = 32;

#define SCHEMA_SERVER_MODULE WIN_LINUX("server.dll", "libserver.so")

static SchemaClassInfoData_t* FindServerClass(const char* pszClassName)
{
    if (!g_pSchemaSystem)
        return nullptr;

    CSchemaSystemTypeScope* pType = g_pSchemaSystem->FindTypeScopeForModule(SCHEMA_SERVER_MODULE);
    return pType ? pType->FindDeclaredClass(pszClassName).Get() : nullptr;
}

// Looks in the class, then up its base classes (single inheritance offsets stay
// valid for the derived class). ppDeclaring receives the class that declares it.
static SchemaClassFieldData_t* FindField(SchemaClassInfoData_t* pClassInfo, const char* pszFieldName,
                                         SchemaClassInfoData_t** ppDeclaring = nullptr)
{
    for (; pClassInfo; pClassInfo = pClassInfo->m_nBaseClassCount ? pClassInfo->m_pBaseClasses[0].m_pClass : nullptr)
    {
        for (uint16 i = 0; i < pClassInfo->m_nFieldCount; ++i)
        {
            if (std::strcmp(pClassInfo->m_pFields[i].m_pszName, pszFieldName) == 0)
            {
                if (ppDeclaring)
                    *ppDeclaring = pClassInfo;
                return &pClassInfo->m_pFields[i];
            }
        }
    }

    return nullptr;
}

// The serializer database knows which fields the engine actually replicates.
// Any entity class reaches it (some schema classes have no entity of their own).
// Null until the entity system exists.
static CNetworkSerializerCodeGenDatabase* GetSerializerDatabase()
{
    if (!GameEntitySystem())
        return nullptr;

    CEntityClass* pClass = GameEntitySystem()->FindClassByName("CBaseEntity");
    if (!pClass || !pClass->m_NetworkSerializerInfo)
        return nullptr;

    return pClass->m_NetworkSerializerInfo->m_pDatabase;
}

int32_t schema::GetOffset(const char* pszClassName, const char* pszFieldName)
{
    SchemaClassInfoData_t* pClassInfo = FindServerClass(pszClassName);
    if (!pClassInfo)
    {
        Warning("schema::GetOffset: class '%s' was not found!\n", pszClassName);
        return -1;
    }

    SchemaClassFieldData_t* pField = FindField(pClassInfo, pszFieldName);
    if (!pField)
    {
        Warning("schema::GetOffset: '%s::%s' was not found!\n", pszClassName, pszFieldName);
        return -1;
    }

    return pField->m_nSingleInheritanceOffset;
}

void* schema::GetMissingFieldStorage()
{
    alignas(64) static unsigned char s_storage[4096] = {};
    return s_storage;
}

int32_t schema::GetChainOffset(const char* pszClassName)
{
    SchemaClassFieldData_t* pField = FindField(FindServerClass(pszClassName), "__m_pChainEntity");
    return pField ? pField->m_nSingleInheritanceOffset : 0;
}

schema::NetworkInfo_t schema::GetNetworkInfo(const char* pszClassName, const char* pszFieldName)
{
    NetworkInfo_t info;

    CNetworkSerializerCodeGenDatabase* pDatabase = GetSerializerDatabase();
    if (!pDatabase)
        return info; // unresolved, asked again on the next NetworkStateChanged()

    info.resolved = true;
    info.offset = GetOffset(pszClassName, pszFieldName);
    if (info.offset < 0)
        return info;

    // Asked on the class that declares the field, which may be a base class.
    SchemaClassInfoData_t* pDeclaring = nullptr;
    FindField(FindServerClass(pszClassName), pszFieldName, &pDeclaring);

    int index = pDatabase->m_ClassInfos.Find(pDeclaring->m_pszName);
    info.networked = index != pDatabase->m_ClassInfos.InvalidIndex()
                     && pDatabase->m_ClassInfos[index]->FindField(pszFieldName) != nullptr;
    info.chainOffset = GetChainOffset(pszClassName);

    return info;
}

void schema::EntityNetworkStateChanged(CEntityInstance* pEntity, int32_t nOffset)
{
    pEntity->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(nOffset)));
}

void schema::ChainNetworkStateChanged(void* pNetworkVarChainer, int32_t nOffset)
{
    CNetworkVarChainer* pChainer = static_cast<CNetworkVarChainer*>(pNetworkVarChainer);

    if (pChainer->m_pEntity)
        pChainer->m_pEntity->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(nOffset), -1, pChainer->m_PathIndex));
}

// Whether p points into the .text section of the module pModuleAddress lives in.
// The range is read once; the CModule is temporary, as it holds a copy of the
// whole section read from disk.
static bool IsInModuleText(const void* p, const void* pModuleAddress)
{
    static const auto s_text = [pModuleAddress] {
        DynLibUtils::CModule module(DynLibUtils::CMemory(const_cast<void*>(pModuleAddress)));
        const DynLibUtils::Section_t* pText = module.GetSectionByName(".text");
        return pText ? std::pair(pText->GetAddr(), pText->GetAddr() + static_cast<std::ptrdiff_t>(pText->m_nSectionSize))
                     : std::pair<std::ptrdiff_t, std::ptrdiff_t>(0, 0);
    }();

    const auto address = reinterpret_cast<std::ptrdiff_t>(p);
    return address >= s_text.first && address < s_text.second;
}

static int FindNetworkStateChangedIndex(void** pVtable)
{
    static const auto s_pattern = DynLibUtils::ParsePattern(g_pszStateChangedSignature);

    // The vtable ends where its slots stop pointing into code (Linux: the next
    // vtable's offset-to-top, Windows: its RTTI locator).
    for (int i = 0; i < 256 && IsInModuleText(pVtable[i], pVtable); ++i)
    {
        const auto* pCode = static_cast<const std::uint8_t*>(pVtable[i]);
        for (std::size_t j = 0; j + s_pattern.m_nSize <= g_nStateChangedSearchBytes; ++j)
        {
            std::size_t k = 0;
            while (k < s_pattern.m_nSize && (s_pattern.m_aMask[k] == '?' || pCode[j + k] == s_pattern.m_aBytes[k]))
                ++k;

            if (k == s_pattern.m_nSize)
                return i;
        }
    }

    return -1;
}

void schema::EmbeddedNetworkStateChanged(void* pObject, int32_t nOffset)
{
    static std::unordered_map<void**, int> s_indices;

    void** pVtable = *static_cast<void***>(pObject);
    auto it = s_indices.find(pVtable);
    if (it == s_indices.end())
        it = s_indices.emplace(pVtable, FindNetworkStateChangedIndex(pVtable)).first;

    if (it->second < 0)
        return; // not embedded in a NetworkVar, nothing to replicate

    using NetworkStateChangedFn = void (*)(void*, NetworkStateChangedData*);

    NetworkStateChangedData data(static_cast<uint32>(nOffset));
    reinterpret_cast<NetworkStateChangedFn>(pVtable[it->second])(pObject, &data);
}
