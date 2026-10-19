#!/usr/bin/env python3
"""Conservative, atomic per-file VietType patcher; never installs TSF."""
import argparse
from pathlib import Path
import re
import shutil
import sys

MARKER = 'VietType SmartSwitch addition'


def patch(source: str) -> str:
    if MARKER in source:
        raise ValueError('Patch already applied to ContextManager.cpp (refusing duplicate)')
    # Anchor exact methods rather than general-purpose regex substitutions.
    focus = re.compile(
        r'(STDMETHODIMP\s+ContextManager::OnSetThreadFocus\s*\(\s*\)\s*\{'  # signature
        r'(?:(?!\n\}).)*?'  # through the method body only
        r'hr\s*=\s*OnSetFocus\s*\(\s*docMgr\s*,\s*nullptr\s*\)\s*;'  # focus
        r'\s*DBG_HRESULT_CHECK\s*\(\s*hr\s*,\s*L"OnSetFocus failed"\s*\)\s*;)',
        re.S,
    )
    toggle = re.compile(
        r'(HRESULT\s+ContextManager::ToggleUserEnabled\s*\(\s*\)\s*\{'  # signature
        r'(?:(?!\n\}).)*?'  # through method
        r'hr\s*=\s*_enabled->SetValue\s*\(\s*enabled\s*\)\s*;'
        r'\s*HRESULT_CHECK_RETURN\s*\(\s*hr\s*,\s*L"_enabled->GetValue failed"\s*\)\s*;)',
        re.S,
    )
    # Upstream has SetValue failed diagnostic in some revisions; accept either.
    toggle = re.compile(
        r'(HRESULT\s+ContextManager::ToggleUserEnabled\s*\(\s*\)\s*\{'  # signature
        r'(?:(?!\n\}).)*?'
        r'hr\s*=\s*_enabled->SetValue\s*\(\s*enabled\s*\)\s*;'
        r'\s*HRESULT_CHECK_RETURN\s*\(\s*hr\s*,\s*L"_enabled->(?:GetValue|SetValue) failed"\s*\)\s*;)',
        re.S,
    )
    if len(focus.findall(source)) != 1 or len(toggle.findall(source)) != 1:
        raise ValueError('Unexpected upstream source: cannot find unique toggle/focus anchors; NO FILES CHANGED')
    if '#include "ContextManager.h"' not in source:
        raise ValueError('Missing ContextManager.h include; NO FILES CHANGED')
    # Focus restore never creates entries or overwrites the selected mode
    # when this app has no explicit saved preference.
    source = focus.sub(
        lambda m: m.group(1) + '\n\n    // ' + MARKER + ': restore only an explicitly saved preference.\n'
        '    // Password/non-editable context handling remains in VietType itself.\n'
        '    bool savedVietnamese = false;\n'
        '    if (SmartSwitch::TryLoad(&savedVietnamese)) {\n'
        '        long oldValue = 0;\n'
        '        if (SUCCEEDED(_enabled->GetValue(&oldValue, _defaultEnabled)) &&\n'
        '            (!!oldValue != savedVietnamese)) {\n'
        '            const HRESULT modeHr = _enabled->SetValue(savedVietnamese ? 1L : 0L);\n'
        '            if (SUCCEEDED(modeHr)) {\n'
        '                (void)_openclose->SetValue(savedVietnamese ? 1L : 0L);\n'
        '            }\n'
        '        }\n'
        '    }\n', source, count=1)
    source = toggle.sub(
        lambda m: m.group(1) + '\n\n    // ' + MARKER + ': save only explicit user toggle.\n'
        '    SmartSwitch::Remember(!!enabled);\n', source, count=1)
    source = source.replace('#include "ContextManager.h"',
                            '#include "ContextManager.h"\n#include "AppModeMemory.h"', 1)
    return source


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True,
                    help='Clean VietType Git clone directory')
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()
    source = args.repo / 'VietTypeATL' / 'ContextManager.cpp'
    dest = args.repo / 'VietTypeATL' / 'AppModeMemory.h'
    overlay = Path(__file__).resolve().parent.parent / 'patch' / 'AppModeMemory.h'
    if not source.is_file() or not (args.repo / 'VietType.slnx').is_file():
        ap.error('The --repo folder is not a valid VietType source checkout')
    if dest.exists():
        ap.error(f'{dest} already exists; refusing to overwrite')
    raw = source.read_bytes()
    bom = raw.startswith(b'\xef\xbb\xbf')
    try:
        original = raw.decode('utf-8-sig')
    except UnicodeDecodeError:
        ap.error('ContextManager.cpp is not UTF-8, no files changed')
    patched = patch(original)
    if args.dry_run:
        print('PASS: patch anchors verified; no files modified')
        return
    backup = source.with_suffix('.cpp.smart-switch-original')
    if backup.exists():
        ap.error(f'{backup} already exists; refusing to overwrite')
    # Write/prepare payloads first; then replace target files in one checked step.
    backup.write_bytes(raw)
    try:
        source.write_bytes((b'\xef\xbb\xbf' if bom else b'') + patched.encode('utf-8'))
        shutil.copyfile(overlay, dest)
    except Exception:
        source.write_bytes(raw)
        dest.unlink(missing_ok=True)
        backup.unlink(missing_ok=True)
        raise
    print('Applied SmartSwitch patch to ' + str(source))
    print('Original backed up at ' + str(backup))
    print('No installations, COM/TSF registrations or system changes performed.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError) as exc:
        print('ERROR: ' + str(exc), file=sys.stderr)
        raise SystemExit(1)
