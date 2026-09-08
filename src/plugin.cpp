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

#include "plugin.h"
#include "scheduler.h"
#include "utils.hpp"

#include "fixes/fix.h"
#include "fixes/beam_crash.h"
#include "fixes/demo_record.h"
#include "fixes/hammer_id.h"
#include "fixes/input_activator_crash.h"
#include "fixes/server_list_players.h"
#include "fixes/slow_animation.h"
#include "fixes/steam_ban.h"
#include "fixes/sv_cheats.h"
#include "fixes/team_limit.h"
#include "fixes/workshop_voice.h"

#include "sdk/GameSessionConfiguration.h"

#include "dynlibutils/module.hpp"

#include "entitysystem.h"
#include "interfaces/interfaces.h"
#include "tier1/bufferstring.h"
#include "tier1/KeyValues.h"
#include "tier1/strtools.h"

#include <cstdio>

#define VERSION_STRING SEMVER " @ " GITHUB_SHA
#define BUILD_TIMESTAMP __DATE__ " " __TIME__

using namespace DynLibUtils;

Plugin g_Plugin;
PLUGIN_EXPOSE(Plugin, g_Plugin);

Plugin::Plugin() :
    // GameFrame Post: the fixes only read the frame. StartupServer Pre: demo_record edits the session's maxplayers before the engine reads it.
    m_hGameFrame(new KHook::Virtual(&ISource2Server::GameFrame, this, nullptr, &Plugin::CSource2Server_GameFrame)),
    m_hStartupServer(new KHook::Virtual(&INetworkServerService::StartupServer, this, &Plugin::INetworkServerService_StartupServer, nullptr)),
    m_hLoadEventsFromFile(new KHook::Virtual(&IGameEventManager2::LoadEventsFromFile, this, nullptr, &Plugin::CGameEventManager_LoadEventsFromFile))
{
}

bool Plugin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS();

    GET_V_IFACE_CURRENT(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetServerFactory, g_pSource2GameClients, ISource2GameClients, SOURCE2GAMECLIENTS_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngineServer, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

    CModule libserver(g_pSource2Server);
    CModule libengine(g_pEngineServer);
    const FixModules modules{libserver, libengine};

    if (!LoadFixes(modules, error, maxlen))
        return false;

    scheduler::Init();

    m_hGameFrame->Add(g_pSource2Server);
    m_hStartupServer->Add(g_pNetworkServerService);

    m_GameEventManagerVTable.m_pVTFs = libserver.GetVirtualTableByName("CGameEventManager").RCast<void**>();
    m_hLoadEventsFromFile->AddGlobal(AsHookTarget<IGameEventManager2>(m_GameEventManagerVTable));

    g_SMAPI->AddListener(this, this);

    return true;
}

bool Plugin::Unload(char* error, size_t maxlen)
{
    m_hGameFrame->Remove(g_pSource2Server);
    m_hStartupServer->Remove(g_pNetworkServerService);
    if (m_GameEventManagerVTable.m_pVTFs)
        m_hLoadEventsFromFile->RemoveGlobal(AsHookTarget<IGameEventManager2>(m_GameEventManagerVTable));

    delete m_hGameFrame;
    delete m_hStartupServer;
    delete m_hLoadEventsFromFile;
    m_hGameFrame = nullptr;
    m_hStartupServer = nullptr;
    m_hLoadEventsFromFile = nullptr;
    m_GameEventManagerVTable.m_pVTFs = nullptr;
    m_pGameEventManager = nullptr;

    for (auto& fix : m_fixes)
        fix->Unload();
    m_fixes.clear();

    scheduler::Shutdown();

    return true;
}

KHook::Return<void> Plugin::CSource2Server_GameFrame(ISource2Server* pThis, bool simulating, bool bFirstTick, bool bLastTick)
{
    scheduler::Tick(simulating);

    for (auto& fix : m_fixes)
        fix->OnGameFrame(simulating);

    return { KHook::Action::Ignore };
}

KHook::Return<void> Plugin::INetworkServerService_StartupServer(INetworkServerService* pThis, const GameSessionConfiguration_t& config, ISource2WorldSession* pSession, const char* pszMapName)
{
    scheduler::RemoveMapChangeTimers();

    for (auto& fix : m_fixes)
        fix->OnStartupServer(config, pszMapName);

    return { KHook::Action::Ignore };
}

KHook::Return<int> Plugin::CGameEventManager_LoadEventsFromFile(IGameEventManager2* pThis, const char* pszFilename, bool bSearchAll)
{
    if (!m_pGameEventManager)
    {
        m_pGameEventManager = pThis;

        for (auto& fix : m_fixes)
            fix->OnGameEventManagerReady(pThis);
    }

    return { KHook::Action::Ignore, 0 };
}

std::vector<std::unique_ptr<CFix>> Plugin::CreateFixes()
{
    std::vector<std::unique_ptr<CFix>> fixes;
    fixes.push_back(std::make_unique<CBeamCrashFix>());
    fixes.push_back(std::make_unique<CDemoRecordFix>());
    fixes.push_back(std::make_unique<CHammerIdFix>());
    fixes.push_back(std::make_unique<CSlowAnimationFix>());
    fixes.push_back(std::make_unique<CWorkshopVoiceFix>());
    fixes.push_back(std::make_unique<CSteamBanFix>());
    fixes.push_back(std::make_unique<CTeamLimitFix>());
    fixes.push_back(std::make_unique<CInputActivatorCrashFix>());
    fixes.push_back(std::make_unique<CSvCheatsFix>());
    fixes.push_back(std::make_unique<CServerListPlayersFix>());
    return fixes;
}

bool Plugin::LoadFixes(const FixModules& modules, char* error, size_t maxlen)
{
    CBufferStringGrowable<255> gameDir;
    g_pEngineServer->GetGameDir(gameDir);

    char szConfigPath[512];
    V_snprintf(szConfigPath, sizeof(szConfigPath), "%s/addons/game_fixes/game_fixes.ini", gameDir.Get());

    KeyValues::AutoDelete config("game_fixes");
    if (!config->LoadFromFile(g_pFullFileSystem, szConfigPath))
    {
        std::snprintf(error, maxlen, "Failed to load %s", szConfigPath);
        return false;
    }

    for (auto& fix : CreateFixes())
    {
        KeyValues* pBlock = config->FindKey(fix->GetName());
        if (!pBlock || !pBlock->GetBool("enable"))
        {
            META_LOG(this, "%s: disabled\n", fix->GetName());
            continue;
        }

        fix->ReadConfig(pBlock);

        char szError[256] = "";
        if (!fix->Load(modules, szError, sizeof(szError)))
        {
            META_LOG(this, "%s: NOT loaded -- %s\n", fix->GetName(), szError);
            fix->Unload();
            continue;
        }

        META_LOG(this, "%s: loaded\n", fix->GetName());
        m_fixes.push_back(std::move(fix));
    }

    return true;
}

///////////////////////////////////////

// Declared by the SDK's entitysystem.h, defined by whoever links it in.
CGameEntitySystem* GameEntitySystem()
{
    // CGameResourceService::SetEntityResourceManifest
    // str server_entities
    return *CMemory(g_pGameResourceServiceServer).Offset(WIN_LINUX(0x58, 0x50)).RCast<CGameEntitySystem**>();
}

///////////////////////////////////////

const char* Plugin::GetLicense() { return "GPLv3"; }
const char* Plugin::GetVersion() { return VERSION_STRING; }
const char* Plugin::GetDate() { return BUILD_TIMESTAMP; }
const char* Plugin::GetLogTag() { return "GameFixes"; }
const char* Plugin::GetAuthor() { return "Slynx (˙·٠● S l y n x ●٠·˙)"; }
const char* Plugin::GetDescription() { return "CS2 server fixes, each toggled in game_fixes.ini"; }
const char* Plugin::GetName() { return "Game Fixes"; }
const char* Plugin::GetURL() { return "https://slynxdev.cz"; }
