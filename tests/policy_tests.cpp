#include "ModePolicy.h"
#include <cassert>
#include <iostream>
using namespace vtclean;

int main() {
    ModePolicy p;
    // Browser begins in V and stays in V after a focus transition.
    assert(p.Evaluate("chrome.exe", FocusKind::EditableText).applyVietnamese);
    auto chromeNonText = p.Evaluate("chrome.exe", FocusKind::NonText);
    assert(!chromeNonText.applyVietnamese);
    assert(chromeNonText.selectedMode == InputMode::Vietnamese);
    assert(p.Evaluate("chrome.exe", FocusKind::EditableText).applyVietnamese);
    // Game key capture: unknown/non-text pass through, chat can use Telex.
    p.SetRule("left4dead2.exe", {InitialMode::Inherit, UnknownPolicy::PassThrough});
    const auto game = p.Evaluate("left4dead2.exe", FocusKind::Unknown);
    assert(!game.applyVietnamese && game.passThrough);
    assert(game.selectedMode == InputMode::Vietnamese);
    assert(p.Evaluate("left4dead2.exe", FocusKind::EditableText).applyVietnamese);
    // Explicit E choice remains independent of Chrome's mode.
    p.SetManualMode("left4dead2.exe", InputMode::English);
    assert(!p.Evaluate("left4dead2.exe", FocusKind::EditableText).applyVietnamese);
    assert(p.Evaluate("chrome.exe", FocusKind::EditableText).applyVietnamese);
    // Manual switching takes precedence over per-app startup preference.
    p.SetRule("editor.exe", {InitialMode::English, UnknownPolicy::TryVietnamese});
    assert(!p.Evaluate("editor.exe", FocusKind::EditableText).applyVietnamese);
    p.ToggleManualMode("editor.exe");
    assert(p.Evaluate("editor.exe", FocusKind::EditableText).applyVietnamese);
    assert(!p.Evaluate("editor.exe", FocusKind::Password).applyVietnamese);
    assert(p.Evaluate("editor.exe", FocusKind::Password).selectedMode == InputMode::Vietnamese);
    p.ClearManualMode("editor.exe");
    assert(!p.Evaluate("editor.exe", FocusKind::EditableText).applyVietnamese);
    // Unknown fields in a normal app must not automatically become EN.
    assert(p.Evaluate("everything.exe", FocusKind::Unknown).applyVietnamese);
    // Regression: switching context cannot overwrite a manual selection.
    p.SetManualMode("chrome.exe", InputMode::English);
    (void)p.Evaluate("chrome.exe", FocusKind::NonText);
    assert(!p.Evaluate("chrome.exe", FocusKind::EditableText).applyVietnamese);
    p.ToggleManualMode("chrome.exe");
    assert(p.Evaluate("chrome.exe", FocusKind::EditableText).applyVietnamese);
    std::cout << "PASS: 17 clean policy assertions\n";
}
