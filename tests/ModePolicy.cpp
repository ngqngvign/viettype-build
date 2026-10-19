// SPDX-License-Identifier: GPL-3.0-or-later
#include "ModePolicy.h"
#include <utility>

namespace vtclean {

ModePolicy::ModePolicy(InputMode defaultMode) : defaultMode_(defaultMode) {}

void ModePolicy::SetRule(std::string appId, AppRule rule) {
    rules_[std::move(appId)] = rule;
}

void ModePolicy::SetManualMode(const std::string& appId, InputMode mode) {
    manualModes_[appId] = mode;
}

void ModePolicy::ToggleManualMode(const std::string& appId) {
    SetManualMode(appId, BaseMode(appId) == InputMode::Vietnamese
                             ? InputMode::English : InputMode::Vietnamese);
}

void ModePolicy::ClearManualMode(const std::string& appId) {
    manualModes_.erase(appId);
}

InputMode ModePolicy::BaseMode(const std::string& appId) const {
    const auto manual = manualModes_.find(appId);
    if (manual != manualModes_.end()) return manual->second;
    const auto it = rules_.find(appId);
    if (it != rules_.end()) {
        if (it->second.initial == InitialMode::English) return InputMode::English;
        if (it->second.initial == InitialMode::Vietnamese) return InputMode::Vietnamese;
    }
    return defaultMode_;
}

Decision ModePolicy::Evaluate(const std::string& appId, FocusKind focus) const {
    const InputMode mode = BaseMode(appId);
    bool isText = (focus == FocusKind::EditableText);
    if (focus == FocusKind::Unknown) {
        const auto it = rules_.find(appId);
        // Unknown MUST NOT silently mutate V/E mode.
        isText = (it == rules_.end() ||
                  it->second.unknown == UnknownPolicy::TryVietnamese);
    }
    // Password and non-text always bypass Vietnamese transformation.
    const bool transform = isText && mode == InputMode::Vietnamese;
    return Decision{mode, transform, !transform};
}

}  // namespace vtclean
