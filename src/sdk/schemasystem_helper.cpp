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

#include "schemasystem_helper.h"
#include "utils.hpp"

#include <cstring>
#include <entity2/entitysystem.h>
#include <entity2/entityclass.h>
#include <networksystem/inetworkserializer.h>
#include <schemasystem/schemasystem.h>

static SchemaClassInfoData_t* FindDeclaredServerClass(const char* pszClassName)
{
    static CSchemaSystemTypeScope* pType = g_pSchemaSystem->FindTypeScopeForModule(WIN_LINUX("server.dll", "libserver.so"));

    return pType->FindDeclaredClass(pszClassName).Get();
}

// Whether the engine actually replicates pszPropName over the network - plenty of
// schema fields (caches, bookkeeping, ...) never leave the server, and pushing those
// through NetworkStateChanged() would just be wasted work (or worse, a bad edict flag).
static bool IsFieldNetworked(const char* pszClassName, const char* pszPropName)
{
    if (!GameEntitySystem())
        return false;

    // Any networked entity class works here, we just need access to the shared serializer database.
    CEntityClass* pAnyClass = GameEntitySystem()->FindClassByName("CBaseEntity");
    if (!pAnyClass || !pAnyClass->m_NetworkSerializerInfo)
        return false;

    CNetworkSerializerCodeGenDatabase* pDatabase = pAnyClass->m_NetworkSerializerInfo->m_pDatabase;

    int index = pDatabase->m_ClassInfos.Find(pszClassName);
    if (index == pDatabase->m_ClassInfos.InvalidIndex())
        return false;

    return pDatabase->m_ClassInfos[index]->FindField(pszPropName) != nullptr;
}

SchemaFieldInfo_t GetServerPropInfo(const char* pszClassName, const char* pszPropName)
{
    SchemaClassInfoData_t* pClassInfo = FindDeclaredServerClass(pszClassName);
    if (!pClassInfo)
    {
        Error("GetServerPropInfo: '%s' was not found!\n", pszClassName);
        return {};
    }

    for (uint16 i = 0; i < pClassInfo->m_nFieldCount; ++i)
    {
        SchemaClassFieldData_t& fieldData = pClassInfo->m_pFields[i];

        if (std::strcmp(fieldData.m_pszName, pszPropName) == 0)
            return {fieldData.m_nSingleInheritanceOffset, IsFieldNetworked(pszClassName, pszPropName)};
    }

    Error("GetServerPropInfo: '%s::%s' was not found!\n", pszClassName, pszPropName);
    return {};
}

int32_t GetServerChainOffset(const char* pszClassName)
{
    SchemaClassInfoData_t* pClassInfo = FindDeclaredServerClass(pszClassName);

    // Recursively look for __m_pChainEntity in base classes, e.g.
    // CCSGameRules -> CTeamplayRules -> CMultiplayRules -> CGameRules, where it's declared on CGameRules
    while (pClassInfo)
    {
        for (uint16 i = 0; i < pClassInfo->m_nFieldCount; ++i)
        {
            SchemaClassFieldData_t& fieldData = pClassInfo->m_pFields[i];

            if (std::strcmp(fieldData.m_pszName, "__m_pChainEntity") == 0)
                return fieldData.m_nSingleInheritanceOffset;
        }

        pClassInfo = pClassInfo->m_nBaseClassCount ? pClassInfo->m_pBaseClasses[0].m_pClass : nullptr;
    }

    return 0;
}

void EntityNetworkStateChanged(uintptr_t pEntity, int32_t nOffset)
{
    reinterpret_cast<CEntityInstance*>(pEntity)->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(nOffset)));
}

void ChainNetworkStateChanged(uintptr_t pNetworkVarChainer, int32_t nOffset)
{
    CNetworkVarChainer* pChainer = reinterpret_cast<CNetworkVarChainer*>(pNetworkVarChainer);

    if (pChainer->m_pEntity)
        pChainer->m_pEntity->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(nOffset), -1, pChainer->m_PathIndex));
}
