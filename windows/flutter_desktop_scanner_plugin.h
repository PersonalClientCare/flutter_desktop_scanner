#ifndef FLUTTER_PLUGIN_FLUTTER_DESKTOP_SCANNER_PLUGIN_H_
#define FLUTTER_PLUGIN_FLUTTER_DESKTOP_SCANNER_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/event_channel.h>
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

  static DWORD WINAPI HandleGetDevices(LPVOID);

  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  private:
    static void OnGetDevicesStreamListen(std::unique_ptr<flutter::EventSink<>>&& events);
    static void OnGetDevicesStreamCancel();

    static void OnScanStreamListen(std::unique_ptr<flutter::EventSink<>>&& events);
    static void OnScanStreamCancel();

    static std::unique_ptr<flutter::EventSink<>> _event_sink;
};

}  // namespace flutter_desktop_scanner

#endif  // FLUTTER_PLUGIN_FLUTTER_DESKTOP_SCANNER_PLUGIN_H_
