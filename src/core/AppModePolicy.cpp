#include "core/AppModePolicy.h"

namespace AppModePolicy {

bool runsSpectrumScan(AppMode mode, bool loggingEnabled) {
    switch (mode) {
        case APP_MODE_ANALYZER_SPECTRUM:
        case APP_MODE_WATERFALL:
        case APP_MODE_SURVEY:
        case APP_MODE_EVENTS:
            return true;
        case APP_MODE_LOGGING:
            return loggingEnabled;
        default:
            return false;
    }
}

}  // namespace AppModePolicy
