#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 -m unittest discover -s tests -p 'test_apply_patch.py' -v
g++ -std=c++17 -Wall -Wextra -Werror -I tests tests/ModePolicy.cpp tests/policy_tests.cpp -o /tmp/viettype-policy-tests
/tmp/viettype-policy-tests
g++ -std=c++20 -Wall -Wextra -Werror -I tests/windows_mock -I patch tests/test_windows_app_mode.cpp -o /tmp/viettype-mode-registry-tests
/tmp/viettype-mode-registry-tests
