#pragma once

#include "BadUsbPayload.h"

// Use as a payload's entry point while its implementation is excluded.
inline void showDisabledPayload(const BadUsbProgress &progress) {
    if (progress.showDisabled) {
        progress.showDisabled(progress.context);
    }
}
