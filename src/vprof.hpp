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

// What the plugin costs, as seen by the engine's profiler (vprof_on, then
// vprof_generate_report / vprof_generate_report_budget).
#pragma once

#include "tier0/vprof.h"

// Budget group every scope of the plugin is filed under: one "GameFixes" line
// in the budget report, next to the game's own groups.
#define GF_VPROF_BUDGETGROUP "GameFixes"

// Scope covering the rest of the enclosing block; goes first in a hook handler
// or a timer callback. The name is the node in the report, written as
// "GameFixes::<fix>::<hooked function>[Post]" and kept to 52 characters: the
// report cuts the Scope column there, and a cut name would merge a Pre with
// its Post. A Pre and a Post handler are two scopes: the game's original call
// between them is not counted.
// VPROF_BUDGET and not VPROF: that one is detail level 1, which the SDK's
// VPROF_LEVEL 0 compiles out. While vprof is off, or off the main thread,
// tier0 returns right at the top of the enter call.
#define GF_VPROF(name) VPROF_BUDGET(name, GF_VPROF_BUDGETGROUP)
