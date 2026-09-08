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

// module.hpp first: vthook.hpp uses DYNLIB_FORCE_INLINE, which only module.hpp
// defines, and does not include it itself.
#include "dynlibutils/module.hpp"
#include "dynlibutils/memaddr.hpp"
#include "dynlibutils/virtual.hpp"
#include "dynlibutils/vthook.hpp"

#include <cstddef>
#include <cstring>
#include <type_traits>

#ifdef _WIN32
#define WIN_LINUX(win, linux) win
#else
#define WIN_LINUX(win, linux) linux
#endif

// Overwrites len bytes of code at addr.
inline void WriteCode(DynLibUtils::CMemory addr, const void* pBytes, std::size_t len)
{
#ifdef _WIN32
    // VirtualProtect hands the previous flags back, so DynLibUtils' RAII
    // helper restores the page exactly as it was.
    DynLibUtils::VirtualUnprotector unprotect(addr.GetPtr(), len);
    std::memcpy(addr.GetPtr(), pBytes, len);
#else
    // Linux has no mprotect() query, and VirtualUnprotector assumes the page
    // was PROT_READ -- fine for a vtable, but it would strip the execute bit
    // off .text. Code goes through its own RWX -> RX window instead.
    const auto pageSize = static_cast<std::uintptr_t>(sysconf(_SC_PAGESIZE));
    const std::uintptr_t pageStart = addr.GetAddr() & ~(pageSize - 1);
    const std::uintptr_t pageEnd = (addr.GetAddr() + len + pageSize - 1) & ~(pageSize - 1);
    void* pPage = reinterpret_cast<void*>(pageStart);

    mprotect(pPage, pageEnd - pageStart, PROT_READ | PROT_WRITE | PROT_EXEC);
    std::memcpy(addr.GetPtr(), pBytes, len);
    mprotect(pPage, pageEnd - pageStart, PROT_READ | PROT_EXEC);
#endif
}

// KHook's Virtual::AddGlobal / RemoveGlobal want an object to read the vtable
// off of, and only ever look at its first pointer. DynLibUtils::VirtualTable
// is exactly that -- one pointer, the vtable -- so it doubles as the stand-in
// when there is no real instance to hand over (engine classes resolved by
// RTTI name): the hook then covers every object sharing that vtable.
template<typename T>
inline T* AsHookTarget(DynLibUtils::VirtualTable& vtable)
{
    return reinterpret_cast<T*>(&vtable);
}

// Mem-initializer for a hook held through a plain pointer: the hook's type is
// already spelled out on the member's declaration, so it is taken from there
// instead of being repeated -- or deduced, which MSVC fails at for KHook's
// manual-index and some member-function-pointer constructors.
//   CFoo::CFoo() : KHOOK_NEW(m_hThink, 52u, this, &CFoo::Pre, &CFoo::Post) {}
#define KHOOK_NEW(member, ...) member(new std::remove_pointer_t<decltype(member)>(__VA_ARGS__))
