// SPDX-License-Identifier: GPL-3.0-or-later
// Independent reference implementation; not yet integrated with VietType TSF.
#pragma once
#include <string>
#include <unordered_map>

namespace vtclean {

enum class InputMode { English, Vietnamese };
enum class FocusKind { EditableText, NonText, Password, Unknown };
enum class InitialMode { Inherit, English, Vietnamese };
enum class UnknownPolicy { TryVietnamese, PassThrough };

struct AppRule {
    InitialMode initial = InitialMode::Inherit;
    UnknownPolicy unknown = UnknownPolicy::TryVietnamese;
};

struct Decision {
    InputMode selectedMode;  // Manual/user-facing V/E mode is preserved.
    bool applyVietnamese;    // Whether TSF should process this *text* keystroke.
    bool passThrough;        // For events not handled by the Vietnamese engine.
};

// Pure policy only: no hooks, TSF registration, IPC, registry or Win32 calls.
// appId is a stable, normalized executable identity supplied by a TSF adapter.
class ModePolicy {
public:
    explicit ModePolicy(InputMode defaultMode = InputMode::Vietnamese);
    void SetRule(std::string appId, AppRule rule);
    void SetManualMode(const std::string& appId, InputMode mode);
    void ToggleManualMode(const std::string& appId);
    void ClearManualMode(const std::string& appId);
    Decision Evaluate(const std::string& appId, FocusKind focus) const;

private:
    InputMode BaseMode(const std::string& appId) const;
    InputMode defaultMode_;
    std::unordered_map<std::string, AppRule> rules_;
    std::unordered_map<std::string, InputMode> manualModes_;
};

}  // namespace vtclean
