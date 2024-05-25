#include <flutter_linux/flutter_linux.h>

#include "include/flutter_desktop_scanner/flutter_desktop_scanner_plugin.h"

// This file exposes some plugin internals for unit testing. See
// https://github.com/flutter/flutter/issues/88724 for current limitations
// in the unit-testable API.

// Handles the getPlatformVersion method call.
FlMethodResponse *get_platform_version();

// Handles the getScanners method call.
FlMethodResponse *get_scanners();

// Handles the initiateScan method call.
FlMethodResponse *initiate_scan(const char* scanner_name);
