#pragma once

#include "core/AppTypes.h"

namespace AppModePolicy {

// Centralized runtime behavior for screens. New analyzer screens only need to
// be registered here instead of extending the main loop with another branch.
bool runsSpectrumScan(AppMode mode, bool loggingEnabled);

}  // namespace AppModePolicy
