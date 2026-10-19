// SPDX-License-Identifier: GPL-3.0-only
// Optional per-EXE VietType mode memory. Does not change the Telex engine,
// register a TSF service, install a hook, start a helper, or require admin.
#pragma once

#include <Windows.h>
#include <string>
#include <cwchar>

namespace VietType::SmartSwitch {
namespace detail {

// Intentionally scoped to the executable basename (e.g. chrome.exe).
// This is easy to inspect/reset; two different executables with the same
// basename will share a preference. No keystrokes or file paths are stored.
inline std::wstring AppName() {
    wchar_t path[32768] = {};
    const DWORD cap = static_cast<DWORD>(sizeof(path) / sizeof(path[0]));
    const DWORD length = ::GetModuleFileNameW(nullptr, path, cap);
    if (!length || length >= cap) return {};
    const wchar_t* slash = std::wcsrchr(path, L'\\');
    const wchar_t* alt = std::wcsrchr(path, L'/');
    if (alt && (!slash || alt > slash)) slash = alt;
    std::wstring basename(slash ? slash + 1 : path);
    if (basename.empty() || basename.size() > 180) return {};
    ::CharLowerBuffW(basename.data(), static_cast<DWORD>(basename.size()));
    // Ensure a malformed executable name is not used as a registry path.
    if (basename.find_first_of(L"\\/") != std::wstring::npos) return {};
    return basename;
}

inline const wchar_t* Root() noexcept {
    return L"Software\\VietType\\SmartSwitch";
}

inline bool MemoryEnabled() {
    HKEY key = nullptr;
    if (::RegOpenKeyExW(HKEY_CURRENT_USER, Root(), 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return true;  // enabled by default, no registry writes on load
    DWORD value = 1;
    DWORD type = 0;
    DWORD bytes = sizeof(value);
    const LSTATUS status = ::RegQueryValueExW(key, L"MemoryEnabled", nullptr,
                                             &type, reinterpret_cast<BYTE*>(&value), &bytes);
    ::RegCloseKey(key);
    if (status != ERROR_SUCCESS || type != REG_DWORD || bytes != sizeof(value)) return true;
    return value != 0;
}

inline std::wstring AppSubkey() {
    const std::wstring name = AppName();
    return name.empty() ? std::wstring{} : std::wstring(Root()) + L"\\" + name;
}
} // namespace detail

// True only if this application has a saved explicit V/E choice.
// A failed registry read never changes the existing VietType/TSF state.
inline bool TryLoad(bool* vietnamese) {
    if (!vietnamese || !detail::MemoryEnabled()) return false;
    const std::wstring subkey = detail::AppSubkey();
    if (subkey.empty()) return false;
    HKEY key = nullptr;
    if (::RegOpenKeyExW(HKEY_CURRENT_USER, subkey.c_str(), 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    DWORD mode = 0;
    DWORD type = 0;
    DWORD bytes = sizeof(mode);
    const LSTATUS status = ::RegQueryValueExW(key, L"Mode", nullptr, &type,
                                             reinterpret_cast<BYTE*>(&mode), &bytes);
    ::RegCloseKey(key);
    if (status != ERROR_SUCCESS || type != REG_DWORD || bytes != sizeof(mode) || mode > 1)
        return false;
    *vietnamese = mode == 1;
    return true;
}

// Best effort: HKCU only. Failing to write MUST NOT prevent typing or
// interfere with the normal VietType toggle operation.
inline void Remember(bool vietnamese) {
    if (!detail::MemoryEnabled()) return;
    const std::wstring subkey = detail::AppSubkey();
    if (subkey.empty()) return;
    HKEY key = nullptr;
    if (::RegCreateKeyExW(HKEY_CURRENT_USER, subkey.c_str(), 0, nullptr,
                          REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &key,
                          nullptr) != ERROR_SUCCESS) return;
    const DWORD mode = vietnamese ? 1u : 0u;
    (void)::RegSetValueExW(key, L"Mode", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&mode), sizeof(mode));
    ::RegCloseKey(key);
}
} // namespace VietType::SmartSwitch
