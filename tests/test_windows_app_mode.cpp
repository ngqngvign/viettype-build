#include "AppModeMemory.h"
#include <cassert>
#include <iostream>

using VietType::SmartSwitch::TryLoad;
using VietType::SmartSwitch::Remember;

int main() {
    bool isV = false;
    // No stored preference -> VietType default remains intact.
    assert(!TryLoad(&isV));
    assert(!isV);
    assert(MockReg::registry.empty());
    // Explicit toggle stores only this app name and mode, no full paths.
    Remember(true);
    assert(TryLoad(&isV) && isV);
    assert(MockReg::registry.find(L"Software\\VietType\\SmartSwitch\\chrome.exe") != MockReg::registry.end());
    assert(MockReg::registry.find(L"Software\\VietType\\SmartSwitch\\C:\\Programs\\Chrome.EXE") == MockReg::registry.end());
    // Saved preferences are isolated across executable basenames.
    MockReg::exe = L"C:\\GAMES\\left4dead2.exe";
    assert(!TryLoad(&isV));
    Remember(false);
    assert(TryLoad(&isV) && !isV);
    MockReg::exe = L"C:\\Programs\\Chrome.EXE";
    assert(TryLoad(&isV) && isV);
    // Disable switch -> no reading nor writing, keeps original state.
    MockReg::registry[L"Software\\VietType\\SmartSwitch"][L"MemoryEnabled"] = {REG_DWORD,0};
    assert(!TryLoad(&isV));
    Remember(false);
    MockReg::registry[L"Software\\VietType\\SmartSwitch"][L"MemoryEnabled"] = {REG_DWORD,1};
    assert(TryLoad(&isV) && isV);
    // Bad type must fail safely back to enabled; corrupted mode never applied.
    MockReg::registry[L"Software\\VietType\\SmartSwitch"][L"MemoryEnabled"] = {5,0};
    assert(TryLoad(&isV) && isV);
    MockReg::registry[L"Software\\VietType\\SmartSwitch\\chrome.exe"][L"Mode"] = {REG_DWORD,9};
    assert(!TryLoad(&isV));
    // Null pointer cannot cause mutation.
    assert(!TryLoad(nullptr));
    std::cout << "PASS: Windows registry behavior mocked and verified\n";
}
