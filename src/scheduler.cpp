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

#include "scheduler.h"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <queue>
#include <vector>

double g_dUniversalTime = 0.0;
static double s_dLastTickTime = 0.0;
static double s_dTimerNextThink = 0.0;

static std::vector<Timer*> s_onceTimers;
static std::vector<Timer*> s_repeatTimers;
static std::mutex s_nextFrameMutex;
static std::queue<std::function<void()>> s_nextFrameQueue;

namespace scheduler
{
    void Init()
    {
        g_dUniversalTime = 0.0;
        s_dLastTickTime = 0.0;
        s_dTimerNextThink = 0.0;
    }

    void Shutdown()
    {
        for (auto* t : s_onceTimers) delete t;
        for (auto* t : s_repeatTimers) delete t;
        s_onceTimers.clear();
        s_repeatTimers.clear();

        std::lock_guard lock(s_nextFrameMutex);
        std::queue<std::function<void()>> empty;
        std::swap(s_nextFrameQueue, empty);
    }

    void NextFrame(std::function<void()> task)
    {
        std::lock_guard lock(s_nextFrameMutex);
        s_nextFrameQueue.emplace(std::move(task));
    }

    Timer* AddTimer(float interval, std::function<void()> callback, int flags)
    {
        Timer* timer = new Timer(interval, g_dUniversalTime + interval, std::move(callback), flags);
        if (flags & TIMER_FLAG_REPEAT)
            s_repeatTimers.push_back(timer);
        else
            s_onceTimers.push_back(timer);
        return timer;
    }

    void KillTimer(Timer* timer)
    {
        if (!timer) return;

        if (timer->InExec)
        {
            timer->KillMe = true;
            return;
        }

        auto kill = [](std::vector<Timer*>& list, Timer* target)
        {
            auto it = std::remove(list.begin(), list.end(), target);
            if (it != list.end())
            {
                delete target;
                list.erase(it, list.end());
            }
        };

        if (timer->Flags & TIMER_FLAG_REPEAT)
            kill(s_repeatTimers, timer);
        else
            kill(s_onceTimers, timer);
    }

    void Tick(bool simulating)
    {
        std::queue<std::function<void()>> local;
        {
            std::lock_guard lock(s_nextFrameMutex);
            std::swap(local, s_nextFrameQueue);
        }
        while (!local.empty())
        {
            local.front()();
            local.pop();
        }

        double now = std::chrono::duration_cast<std::chrono::duration<double>>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        if (simulating)
            g_dUniversalTime += now - s_dLastTickTime;
        else
            g_dUniversalTime += 0.015;
        s_dLastTickTime = now;

        if (g_dUniversalTime < s_dTimerNextThink)
            return;

        for (int i = static_cast<int>(s_onceTimers.size()) - 1; i >= 0; --i)
        {
            Timer* t = s_onceTimers[i];
            if (g_dUniversalTime >= t->ExecTime)
            {
                t->InExec = true;
                t->Callback();
                delete t;
                s_onceTimers.erase(s_onceTimers.begin() + i);
            }
        }

        for (int i = static_cast<int>(s_repeatTimers.size()) - 1; i >= 0; --i)
        {
            Timer* t = s_repeatTimers[i];
            if (g_dUniversalTime >= t->ExecTime)
            {
                t->InExec = true;
                t->Callback();
                if (t->KillMe)
                {
                    delete t;
                    s_repeatTimers.erase(s_repeatTimers.begin() + i);
                    continue;
                }
                t->InExec = false;
                t->ExecTime = g_dUniversalTime + t->Interval;
            }
        }

        s_dTimerNextThink = g_dUniversalTime + 0.1;
    }

    void RemoveMapChangeTimers()
    {
        auto remove = [](std::vector<Timer*>& list)
        {
            for (int i = static_cast<int>(list.size()) - 1; i >= 0; --i)
            {
                if (list[i]->Flags & TIMER_FLAG_NO_MAPCHANGE)
                {
                    delete list[i];
                    list.erase(list.begin() + i);
                }
            }
        };
        remove(s_onceTimers);
        remove(s_repeatTimers);
    }
}
