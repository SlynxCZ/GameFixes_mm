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

#include "hammer_id.h"
#include "utils.hpp"

#include "dynlibutils/module.hpp"

#include <cstdio>

using namespace DynLibUtils;

static constexpr uint8_t OPCODE_JMP_REL8 = 0xEB;

bool CHammerIdFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    // https://github.com/Source2ZE/CS2Fixes/commit/61937f78dd649ed391f6988b0c58ae4a75fd4bc6

    // The pattern starts on the jnz itself (75 ?); one byte turns it into a jmp.
    m_pJnz = modules.server.FindPattern(ParseStringPattern(WIN_LINUX("75 ? 48 8B 03 48 8B CB FF 90 ? ? ? ? 84 C0 74 ? 48 8D 05", "75 ? 48 8B 03 48 89 DF FF 90 ? ? ? ? 84 C0 74 ? 48 8D 05")));
    if (!m_pJnz)
    {
        std::snprintf(error, maxlen, "SetSchemaHammerUniqueId not found");
        return false;
    }

    m_uOriginalByte = *m_pJnz.RCast<const uint8_t*>();
    WriteCode(m_pJnz, &OPCODE_JMP_REL8, sizeof(OPCODE_JMP_REL8));

    Log("patched SetSchemaHammerUniqueId at %p (jnz -> jmp)", m_pJnz.GetPtr());
    return true;
}

void CHammerIdFix::Unload()
{
    if (!m_pJnz)
        return;

    WriteCode(m_pJnz, &m_uOriginalByte, sizeof(m_uOriginalByte));
    m_pJnz = CMemory();
}
