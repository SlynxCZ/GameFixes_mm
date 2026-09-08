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

// SetSchemaHammerUniqueId only writes m_sUniqueHammerID behind a jnz, so most
// entities never get one. The jnz is patched to a jmp and put back on unload.
#pragma once

#include "fix.h"

#include "dynlibutils/memaddr.hpp"

#include <cstdint>

class CHammerIdFix final : public CFix
{
public:
    const char* GetName() const override { return "hammer_id"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

private:
    // The patched byte (the jnz opcode) and what it was before.
    DynLibUtils::CMemory m_pJnz;
    uint8_t m_uOriginalByte = 0;
};
