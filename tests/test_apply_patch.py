from pathlib import Path
import tempfile
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from apply_patch import patch, MARKER

# Fixture matches the method signatures and call sequences checked on GitHub.
BASE = '''#include "ContextManager.h"
namespace VietType {
STDMETHODIMP ContextManager::OnSetThreadFocus() {
    HRESULT hr;
    CComPtr<ITfDocumentMgr> docMgr;
    hr = _threadMgr->GetFocus(&docMgr);
    hr = OnSetFocus(docMgr, nullptr);
    DBG_HRESULT_CHECK(hr, L"OnSetFocus failed");
    hr = UpdateStatus(true);
    return S_OK;
}
HRESULT ContextManager::ToggleUserEnabled() {
    long enabled;
    HRESULT hr;
    if (!_focus || _focus->IsBlocked()) return S_OK;
    hr = _enabled->GetValue(&enabled, !_defaultEnabled);
    enabled = !enabled;
    hr = _enabled->SetValue(enabled);
    HRESULT_CHECK_RETURN(hr, L"_enabled->SetValue failed");
    hr = _openclose->SetValue(!!enabled);
    hr = UpdateStatus(true);
    return S_OK;
}
}'''

class ApplyTests(unittest.TestCase):
    def test_inserts_to_two_expected_methods(self):
        r = patch(BASE)
        self.assertEqual(r.count(MARKER), 2)
        self.assertIn('SmartSwitch::Remember(!!enabled)', r)
        self.assertIn('SmartSwitch::TryLoad(&savedVietnamese)', r)
        self.assertIn('#include "AppModeMemory.h"', r)
        self.assertIn('return S_OK;', r)

    def test_idempotence_guard(self):
        with self.assertRaisesRegex(ValueError, 'already applied'):
            patch(patch(BASE))

    def test_fail_closed_changed_upstream(self):
        with self.assertRaisesRegex(ValueError, 'unique toggle/focus anchors'):
            patch(BASE.replace('OnSetFocus(docMgr, nullptr)', 'OnSetFocus(docMgr, &newFocus)'))

    def test_fail_closed_without_include(self):
        with self.assertRaisesRegex(ValueError, 'Missing ContextManager.h'):
            patch(BASE.replace('#include "ContextManager.h"', ''))

if __name__ == '__main__':
    unittest.main()
