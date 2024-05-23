#ifndef FLUTTER_PLUGIN_FLUTTER_DESKTOP_SCANNER_PLUGIN_H_
#define FLUTTER_PLUGIN_FLUTTER_DESKTOP_SCANNER_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace flutter_desktop_scanner {

class FlutterDesktopScannerPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  FlutterDesktopScannerPlugin();

  virtual ~FlutterDesktopScannerPlugin();

  // Disallow copy and assign.
  FlutterDesktopScannerPlugin(const FlutterDesktopScannerPlugin&) = delete;
  FlutterDesktopScannerPlugin& operator=(const FlutterDesktopScannerPlugin&) = delete;

  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace flutter_desktop_scanner

#endif  // FLUTTER_PLUGIN_FLUTTER_DESKTOP_SCANNER_PLUGIN_H_
