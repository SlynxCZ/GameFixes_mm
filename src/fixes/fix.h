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

// What every fix looks like to the plugin.
#pragma once

#include <cstddef>

// "debug" "1" at the top of game_fixes.ini: every plugin step, every hook install
// with the slot or address it lands on, and the first calls of every handler
// go to the log -- enough to see which hook a crash follows.
namespace gfdebug
{
    extern bool g_bEnabled;

    // "[debug] <message>" through metamod's logger; nothing when debug is off.
    void Log(const char* pszFormat, ...);
}

// The first N calls of the enclosing function, when debug logging is on.
#define GF_TRACE(N) \
    do \
    { \
        if (gfdebug::g_bEnabled) \
        { \
            static unsigned s_nCalls = 0; \
            if (s_nCalls < (N)) \
            { \
                ++s_nCalls; \
                gfdebug::Log("%s: call %u", __func__, s_nCalls); \
            } \
        } \
    } while (0)

class KeyValues;
class GameSessionConfiguration_t;
class IGameEventManager2;

namespace DynLibUtils { class CModule; }

// The game modules a fix may need to scan, resolved once by the plugin.
struct FixModules
{
    DynLibUtils::CModule& server;
    DynLibUtils::CModule& engine;
};

class CFix
{
public:
    virtual ~CFix() = default;

    // Key of this fix's block in game_fixes.ini; also its prefix in the log.
    virtual const char* GetName() const = 0;

    // Reads the fix's own options from its ini block. Only called when the
    // block has "enable" "1", right before Load().
    virtual void ReadConfig(KeyValues* pConfig) {}

    // Installs the fix. On failure, writes why into error and returns false;
    // the plugin logs it, calls Unload() and leaves just this fix off, the
    // others still load. KHook hooks only come down in their destructor, so
    // a fix owns them through plain pointers and deletes them in Unload().
    virtual bool Load(const FixModules& modules, char* error, size_t maxlen) = 0;
    virtual void Unload() {}

    // Forwarded from the plugin's own engine hooks (one shared hook each).
    // StartupServer is Pre, so config is still editable.
    virtual void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) {}
    virtual void OnGameFrame(bool simulating) {}

    // Once, when the game's event manager shows up (its first
    // LoadEventsFromFile, so the game's own events are registered by then) --
    // the place to AddListener().
    virtual void OnGameEventManagerReady(IGameEventManager2* pManager) {}

protected:
    // "<name>: <message>" through metamod's logger.
    void Log(const char* pszFormat, ...) const;

    // Same, prefixed [debug] and only when debug logging is on.
    void LogDebug(const char* pszFormat, ...) const;
};
