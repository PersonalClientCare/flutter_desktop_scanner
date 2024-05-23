#include "include/flutter_desktop_scanner/flutter_desktop_scanner_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "flutter_desktop_scanner_plugin.h"

void FlutterDesktopScannerPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  flutter_desktop_scanner::FlutterDesktopScannerPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
