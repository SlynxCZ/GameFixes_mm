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

#pragma once

#include <functional>

#define TIMER_FLAG_REPEAT       (1 << 0)
#define TIMER_FLAG_NO_MAPCHANGE (1 << 1)

struct Timer
{
    float Interval;
    double ExecTime;
    std::function<void()> Callback;
    int Flags;
    bool KillMe = false;
    bool InExec = false;

    Timer(float interval, double execTime, std::function<void()> cb, int flags)
        : Interval(interval), ExecTime(execTime), Callback(std::move(cb)), Flags(flags)
    {
    }
};

extern double g_dUniversalTime;

namespace scheduler
{
    void Init();
    void Shutdown();
    void Tick(bool simulating = true);
    void RemoveMapChangeTimers();

    Timer* AddTimer(float interval, std::function<void()> callback, int flags = 0);
    void KillTimer(Timer* timer);
    void NextFrame(std::function<void()> task);
}
