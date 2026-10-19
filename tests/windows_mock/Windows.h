// TEST MOCK ONLY - NOT FOR WINDOWS BUILD.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <cwctype>
#include <map>
#include <string>
#include <cstring>
using DWORD = std::uint32_t;
using BYTE = std::uint8_t;
using LSTATUS = long;
using HKEY = std::wstring*;
constexpr LSTATUS ERROR_SUCCESS = 0;
constexpr LSTATUS ERROR_FILE_NOT_FOUND = 2;
constexpr DWORD KEY_QUERY_VALUE = 0x1;
constexpr DWORD KEY_SET_VALUE = 0x2;
constexpr DWORD REG_DWORD = 4;
constexpr DWORD REG_OPTION_NON_VOLATILE = 0;
namespace MockReg {
    struct Value { DWORD type=REG_DWORD; DWORD dword=0; };
    inline std::map<std::wstring, std::map<std::wstring, Value>> registry;
    inline std::wstring exe = L"C:\\Programs\\Chrome.EXE";
    inline std::wstring rootHive = L"HKEY_CURRENT_USER";
}
inline HKEY HKEY_CURRENT_USER = &MockReg::rootHive;
inline DWORD GetModuleFileNameW(void*, wchar_t* out, DWORD max) {
    if (MockReg::exe.size() >= max) return max;
    std::wcscpy(out, MockReg::exe.c_str());
    return static_cast<DWORD>(MockReg::exe.size());
}
inline DWORD CharLowerBuffW(wchar_t* text, DWORD len) {
    for (DWORD i=0; i<len; ++i) text[i] = std::towlower(text[i]);
    return len;
}
inline LSTATUS RegOpenKeyExW(HKEY, const wchar_t* path, DWORD, DWORD, HKEY* out) {
    auto it=MockReg::registry.find(path);
    if (it==MockReg::registry.end()) return ERROR_FILE_NOT_FOUND;
    *out=const_cast<std::wstring*>(&it->first);
    return ERROR_SUCCESS;
}
inline LSTATUS RegCreateKeyExW(HKEY, const wchar_t* path, DWORD, void*, DWORD, DWORD, void*, HKEY* out, DWORD*) {
    auto it=MockReg::registry.try_emplace(path).first;
    *out=const_cast<std::wstring*>(&it->first);
    return ERROR_SUCCESS;
}
inline LSTATUS RegQueryValueExW(HKEY key, const wchar_t* name, DWORD*, DWORD* type, BYTE* buf, DWORD* bytes) {
    auto kt=MockReg::registry.find(*key);
    if (kt==MockReg::registry.end()) return ERROR_FILE_NOT_FOUND;
    auto v=kt->second.find(name);
    if (v==kt->second.end()) return ERROR_FILE_NOT_FOUND;
    if (*bytes < sizeof(DWORD)) return ERROR_FILE_NOT_FOUND;
    *type=v->second.type;
    *bytes=sizeof(DWORD);
    std::memcpy(buf, &v->second.dword, sizeof(DWORD));
    return ERROR_SUCCESS;
}
inline LSTATUS RegSetValueExW(HKEY key, const wchar_t* name, DWORD, DWORD type, const BYTE* value, DWORD bytes) {
    if (bytes != sizeof(DWORD)) return ERROR_FILE_NOT_FOUND;
    DWORD data;
    std::memcpy(&data,value,sizeof(DWORD));
    MockReg::registry[*key][name] = {type, data};
    return ERROR_SUCCESS;
}
inline LSTATUS RegCloseKey(HKEY) { return ERROR_SUCCESS; }
