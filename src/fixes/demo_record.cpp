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

#include "demo_record.h"

#include "sdk/GameSessionConfiguration.h"

#include "dynlibutils/module.hpp"

#include <cstdio>

using namespace DynLibUtils;

CDemoRecordFix::CDemoRecordFix() :
    m_hDisconnect(new KHook::Virtual(&CServerSideClientBase::Disconnect, this, &CDemoRecordFix::CServerSideClient_Disconnect, nullptr))
{
}

bool CDemoRecordFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    m_tvEnable = std::make_unique<CConVarRef<bool>>("tv_enable");

    CMemory pVTable = modules.engine.GetVirtualTableByName("CServerSideClient");

    m_VTable.m_pVTFs = pVTable.RCast<void**>();
    m_hDisconnect->AddGlobal(AsHookTarget<CServerSideClientBase>(m_VTable));

    Log("hooked CServerSideClient::Disconnect on vtable %p, tv_enable is %s", pVTable.GetPtr(), IsTvEnabled() ? "on" : "off");
    return true;
}

void CDemoRecordFix::Unload()
{
    if (m_VTable.m_pVTFs)
        m_hDisconnect->RemoveGlobal(AsHookTarget<CServerSideClientBase>(m_VTable));

    delete m_hDisconnect;
    m_hDisconnect = nullptr;
    m_VTable.m_pVTFs = nullptr;
    m_tvEnable.reset();
}

bool CDemoRecordFix::IsTvEnabled() const
{
    return m_tvEnable && m_tvEnable->IsValidRef() && m_tvEnable->Get();
}

void CDemoRecordFix::OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName)
{
    if (!IsTvEnabled())
        return;

    // The engine reads maxPlayers after this Pre hook, so the GOTV slot comes on top of the configured player count instead of eating one.
    Log("GOTV is enabled, expanding the session's maxplayers (%i -> %i)", config.maxPlayers, config.maxPlayers + 1);
    const_cast<GameSessionConfiguration_t&>(config).maxPlayers++;
}

KHook::Return<void> CDemoRecordFix::CServerSideClient_Disconnect(CServerSideClientBase* pThis, ENetworkDisconnectionReason reason, const char* pszInternalReason)
{
    if (pThis->IsHLTV() && IsTvEnabled())
    {
        Log("GOTV is enabled, blocking the GOTV client's disconnect");
        return { KHook::Action::Supersede };
    }

    return { KHook::Action::Ignore };
}
