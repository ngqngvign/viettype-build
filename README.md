# VietType Improved — SmartSwitch v0.1 (source patch + Windows builder)

**Status:** A real patch targeting the official VietType TSF code was authored and tested against guarded source fixtures, and its registry helper was compiled/tested with a mocked Windows API. **It has NOT been built into a real Windows DLL/MSI, nor run on real Windows.** The Linux environment did not have Visual Studio/ATL or access to clone the repository. Do not confuse this source bundle with a verified installer.

## What changes

- Preserves **dinhngtu/VietType** original Telex core. References VKey's *per-app mode memory* idea; **no VKey source is copied**.
- Hooks exactly two places in `VietTypeATL/ContextManager.cpp`:
  1. In `ToggleUserEnabled`, remember the V/E mode after the original TSF compartment update succeeds.
  2. In `OnSetThreadFocus`, restore an explicitly remembered per-executable mode; otherwise keep the original VietType default and existing TSF behavior.
- New header-only `VietTypeATL/AppModeMemory.h` reads/writes `HKCU\Software\VietType\SmartSwitch` only, best effort. No global hooks, helper processes, auto-start, watchdog, admin rights, startup entries, or COM registrations in this patch.
- The memory feature defaults to **enabled**, and can be disabled without affecting VietType itself. Switching between Chrome, Everything, Notepad, etc. will remember the last explicitly toggled mode for that `exe` on return focus when TSF works as designed.
- **Game behavior limitation**: This is *not* game chat detection. VietType's existing TSF focus blocking behavior is retained. Games that do not expose chat as TSF editable context cannot be made to type Vietnamese via this patch. For `left4dead2.exe` you can explicitly prefer English when launching and toggle Vietnamese in a TSF chat field if supported.

## Before you use it

The upstream project at https://github.com/dinhngtu/VietType is archived, GPL-3.0. Keep upstream license and changes if redistributing. VKey: https://github.com/phatMT97/VKey, used as feature-design inspiration. It is **not** true portable software; Windows TSF IMEs require Windows registration through an appropriate installer. Avoid trying to manually replace an in-use system DLL.

## Path A: Windows developer machine (local build)

Requirements: Windows 10/11 x64, Git for Windows, Python 3 (or Python launcher `py`), Visual Studio with **C++ Desktop, ATL, Windows SDK, MSBuild** (VS version supporting `.slnx`). Internet access to GitHub. **No Administrator required for compiling.**

1. Unzip the full directory, open PowerShell in it.
2. Execute:

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Build-Modified-VietType.ps1 -Platform x64
   ```

3. On success check `output\x64\VietTypeATL.dll` and SHA256 file. The script records `logs\UPSTREAM_COMMIT.txt` and build/restore logs. Nothing is registered, installed or launched. **A DLL by itself is not a usable install package.**
4. For an **experimental MSI build**, the optional `-BuildSetup` flag attempts to compile the setup project using upstream's project and tools. **Don't install the resulting MSI on your primary PC until it has passed a virtual-machine installation/uninstallation test.**

The script refuses to overwrite an existing checkout. For retry after a failed build, run with `-UseExistingCheckout` only if `tools/apply_patch.py` has **not** already patched that checkout; otherwise compile with MSBuild directly inside the checkout. The patcher detects duplicate applications.

## Path B: GitHub Actions Windows build (no Visual Studio locally)

Create your own GitHub repository with the files from this ZIP, including `.github/workflows/build-modified-viettype.yml`. Then in the repository's **Actions** tab, run **Build modified VietType (no installation)** manually. It clones upstream, applies guarded edits, and builds with the Windows runner. **This job has not been executed**; it is a reproducible plan, not proof of an MSI build.

## Per-application configuration

Use PowerShell in the root directory:

```powershell
.\Set-AppMode.ps1 -ExeName chrome.exe -Mode Show
.\Set-AppMode.ps1 -ExeName left4dead2.exe -Mode English
.\Set-AppMode.ps1 -ExeName chrome.exe -Mode Vietnamese
.\Set-AppMode.ps1 -ExeName chrome.exe -Mode Forget
.\Set-AppMode.ps1 -Memory Disable
.\Set-AppMode.ps1 -Memory Enable
.\Set-AppMode.ps1 -Memory Show
```

When enabled, the **last explicit toggle** in an app overwrites its preference on next focus. This is expected. The setting resides only in HKCU and can be deleted manually. The helper never manipulates COM/TSF registry entries. Two programs with the same executable *basename* share a preference; per-window and per-Chrome-tab preferences aren't supported. No keystrokes, text content, or full executable paths are stored.

## Recovery / uninstall

- Before installing **any** compiled VietType variant, test using a Windows Sandbox or disposable VM. Take a snapshot. Use Windows installed-apps uninstaller for the actual MSI and verify the input method disappears.
- To switch off **this improvement only**, execute `.\Set-AppMode.ps1 -Memory Disable`. This does not unregister VietType itself; the original TSF remains.
- To revert **source edits** safely, restore `VietTypeATL/ContextManager.cpp.smart-switch-original` from the checkout and delete `VietTypeATL/AppModeMemory.h`, after making sure it was created by this patch. Nothing in this patch touches source of Telex.
- To remove **saved modes**, delete only `HKCU\Software\VietType\SmartSwitch` in Registry Editor *after backing up the key*. Avoid touching input processor registrations by hand.
- Please **do not install a modified MSI over an existing VietType MSI** until product/upgrade codes and rollback behaviors have been verified. This source patch reuses the original application identity by design, and untested MSI upgrade behavior could interfere with the existing installation.

## Test evidence and outstanding work

Verified here on Linux:
- 17 policy assertions from the earlier standalone model (`tests/ModePolicy.*`, `tests/policy_tests.cpp`)
- `tests/test_apply_patch.py` tests anchor matching, duplicate protection and fail-closed behavior.
- `patch/AppModeMemory.h` compiled as C++20 using **mock Win32 registry APIs**, tests pass for default/remember/restore, isolation, feature disable, malformed registry values. This is not a real Windows API or IME test.

Not verified:
- Real Windows C++ ATL/TSF DLL compilation, MSI compilation, app behavior, password fields, Chrome, Everything, Left 4 Dead 2, installer cleanup.
- App preferences changed by external Windows switching rather than VietType's built-in toggle are not guaranteed to be remembered.
- Cross-process simultaneous mode changes are last-write-wins; TSF focus restores the saved value at focus time.

Next integration milestones: real Windows build and smoke tests -> verify exact TSF life-cycle -> implement genuine game context classifier only after reproducible test cases -> verify installer uninstall. Keep the original Telex engine untouched until those pass.
