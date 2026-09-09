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

#include "fix.h"
#include "plugin.h"

#include <tier1/strtools.h>

#include <cstdarg>

void CFix::Log(const char* pszFormat, ...) const
{
    char szMessage[1024];

    va_list args;
    va_start(args, pszFormat);
    V_vsnprintf(szMessage, sizeof(szMessage), pszFormat, args);
    va_end(args);

    META_LOG(&g_Plugin, "%s: %s\n", GetName(), szMessage);
}

void CFix::LogDebug(const char* pszFormat, ...) const
{
    if (!gfdebug::g_bEnabled)
        return;

    char szMessage[1024];

    va_list args;
    va_start(args, pszFormat);
    V_vsnprintf(szMessage, sizeof(szMessage), pszFormat, args);
    va_end(args);

    META_LOG(&g_Plugin, "[debug] %s: %s\n", GetName(), szMessage);
}

namespace gfdebug
{
    bool g_bEnabled = false;

    void Log(const char* pszFormat, ...)
    {
        if (!g_bEnabled)
            return;

        char szMessage[1024];

        va_list args;
        va_start(args, pszFormat);
        V_vsnprintf(szMessage, sizeof(szMessage), pszFormat, args);
        va_end(args);

        META_LOG(&g_Plugin, "[debug] %s\n", szMessage);
    }
}
