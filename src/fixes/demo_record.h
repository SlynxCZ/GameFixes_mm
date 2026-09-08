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

// With tv_enable 1 the GOTV client is never let go: its
// CServerSideClient::Disconnect is superseded and the session's maxplayers is
// raised by one at StartupServer (Pre, so the config is still editable), so
// the slot is its own and the demo keeps recording across match restarts on
// the same map.
#pragma once

#include "fix.h"
#include "plugin.h"
#include "utils.hpp"

#include "sdk/CServerSideClient.h"

#include "dynlibutils/virtual.hpp"

#include <tier1/convar.h>

#include <memory>

class CDemoRecordFix final : public CFix
{
public:
    CDemoRecordFix();

    const char* GetName() const override { return "demo_record"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) override;

public: // Hooks
    KHook::Return<void> CServerSideClient_Disconnect(CServerSideClientBase* pThis, ENetworkDisconnectionReason reason, const char* pszInternalReason);

    KHook::Virtual<CServerSideClientBase, void, ENetworkDisconnectionReason, const char*>* m_hDisconnect = nullptr;

private:
    bool IsTvEnabled() const;

    // CServerSideClient's vtable (from libengine2); doubles as the hook's
    // stand-in object, see AsHookTarget().
    DynLibUtils::VirtualTable m_VTable;

    // tv_enable; both the disconnect block and the extra slot are no-ops
    // while it's off.
    std::unique_ptr<CConVarRef<bool>> m_tvEnable;
};
