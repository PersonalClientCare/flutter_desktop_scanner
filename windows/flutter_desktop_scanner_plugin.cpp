#include "flutter_desktop_scanner_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/event_channel.h>
#include <flutter/event_sink.h>
#include <flutter/encodable_value.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <sstream>
#include <algorithm>

#include <wia.h>

#pragma comment(lib, "wiaguid.lib")

namespace flutter_desktop_scanner {

// static
std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> FlutterDesktopScannerPlugin::_event_sink = nullptr;

void FlutterDesktopScannerPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {  
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "personalclientcare/flutter_desktop_scanner",
          &flutter::StandardMethodCodec::GetInstance());

  flutter::EventChannel<> get_devices_event_channel(
    registrar->messenger(), "personalclientcare/flutter_desktop_scanner_get_devices",
          &flutter::StandardMethodCodec::GetInstance());

  flutter::EventChannel<> scan_event_channel(
    registrar->messenger(), "personalclientcare/flutter_desktop_scanner_scan",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<FlutterDesktopScannerPlugin>();

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  get_devices_event_channel.SetStreamHandler(
    std::make_unique<flutter::StreamHandlerFunctions<>>(
          [](auto arguments, auto events) {
            FlutterDesktopScannerPlugin::OnGetDevicesStreamListen(std::move(events));
            return nullptr;
          },
          [](auto arguments) {
            FlutterDesktopScannerPlugin::OnGetDevicesStreamCancel();
            return nullptr;
          }));

  scan_event_channel.SetStreamHandler(
    std::make_unique<flutter::StreamHandlerFunctions<>>(
          [](auto arguments, auto events) {
            FlutterDesktopScannerPlugin::OnScanStreamListen(std::move(events));
            return nullptr;
          },
          [](auto arguments) {
            FlutterDesktopScannerPlugin::OnScanStreamCancel();
            return nullptr;
          }));

  registrar->AddPlugin(std::move(plugin));
}

FlutterDesktopScannerPlugin::FlutterDesktopScannerPlugin() {}

FlutterDesktopScannerPlugin::~FlutterDesktopScannerPlugin() {}


DWORD WINAPI FlutterDesktopScannerPlugin::HandleGetDevices(LPVOID lpParam) {
    HRESULT hr;
    IWiaDevMgr2 *dev_manager;
    hr = CoCreateInstance(CLSID_WiaDevMgr2, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr2, (void**)&dev_manager);
    if (FAILED(hr)) {
        _event_sink->Error("WIA_ERROR", "Failed to create WIA device manager");
        return 1;
    }

    IEnumWIA_DEV_INFO *dev_info_enum;
    hr = dev_manager->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL, &dev_info_enum);
    if (FAILED(hr)) {
        _event_sink->Error("WIA_ERROR", "Failed to enumerate WIA devices");
        return 1;
    }

    IWiaPropertyStorage *dev_info;
    ULONG fetched;

    auto devices = flutter::EncodableList();

    while (true) {
        hr = dev_info_enum->Next(1, &dev_info, &fetched);
        if (hr == S_FALSE) {
            break;
        }
        if (FAILED(hr)) {
            _event_sink->Error("WIA_ERROR", "Failed to get WIA device info");
            return 1;
        }

        // get device name
        PROPSPEC dev_name_spec;
        dev_name_spec.ulKind = PRSPEC_PROPID;
        dev_name_spec.propid = WIA_DIP_DEV_NAME;

        PROPVARIANT dev_name;
        hr = dev_info->ReadMultiple(1, &dev_name_spec, &dev_name);
        if (FAILED(hr)) {
            _event_sink->Error("WIA_ERROR", "Failed to get WIA device name");
            return 1;
        }

        auto wide_name = std::wstring(dev_name.bstrVal);

        std::string name(wide_name.length(), 0);
        std::transform(wide_name.begin(), wide_name.end(), name.begin(), [] (wchar_t c) {
            return (char)c;
        });

        // get device model
        PROPSPEC psDeviceID;
        psDeviceID.ulKind = PRSPEC_PROPID;
        psDeviceID.propid = WIA_DIP_DEV_ID;

        PROPVARIANT dev_model;
        hr = dev_info->ReadMultiple(1, &psDeviceID, &dev_model);
        if (FAILED(hr)) {
            _event_sink->Error("WIA_ERROR", "Failed to get WIA model");
            return 1;
        }

        auto wide_model = std::wstring(dev_model.bstrVal);

        std::string model(wide_model.length(), 0);
        std::transform(wide_model.begin(), wide_model.end(), model.begin(), [] (wchar_t c) {
            return (char)c;
        });

        // get device vendor
        PROPSPEC psDeviceVendor;
        psDeviceVendor.ulKind = PRSPEC_PROPID;
        psDeviceVendor.propid = WIA_DIP_VEND_DESC;

        PROPVARIANT dev_vendor;
        hr = dev_info->ReadMultiple(1, &psDeviceVendor, &dev_vendor);
        if (FAILED(hr)) {
            _event_sink->Error("WIA_ERROR", "Failed to get WIA vendor");
            return 1;
        }

        auto wide_vendor = std::wstring(dev_vendor.bstrVal);

        std::string vendor(wide_vendor.length(), 0);
        std::transform(wide_vendor.begin(), wide_vendor.end(), vendor.begin(), [] (wchar_t c) {
            return (char)c;
        });

        // get device type
        PROPSPEC psDeviceType;
        psDeviceType.ulKind = PRSPEC_PROPID;
        psDeviceType.propid = WIA_DIP_DEV_TYPE;

        PROPVARIANT dev_type;
        hr = dev_info->ReadMultiple(1, &psDeviceType, &dev_type);
        if (FAILED(hr)) {
            _event_sink->Error("WIA_ERROR", "Failed to get WIA type");
            return 1;
        }

        auto wide_type = std::wstring(dev_type.bstrVal);

        std::string type(wide_type.length(), 0);
        std::transform(wide_type.begin(), wide_type.end(), type.begin(), [] (wchar_t c) {
            return (char)c;
        });

        flutter::EncodableMap device = {
          {"name", name},
          {"model", model},
          {"vendor", vendor},
          {"type", type}
        };

        devices.push_back(device);
    }

    _event_sink->Success(devices);

    dev_info_enum->Release();
    dev_manager->Release();

    return 0;
}

void FlutterDesktopScannerPlugin::OnGetDevicesStreamListen(std::unique_ptr<flutter::EventSink<>>&& events) {
  _event_sink = std::move(events);
}

void FlutterDesktopScannerPlugin::OnGetDevicesStreamCancel() { _event_sink = nullptr; }

void FlutterDesktopScannerPlugin::OnScanStreamListen(std::unique_ptr<flutter::EventSink<>>&& events) {
  _event_sink = std::move(events);
}

void FlutterDesktopScannerPlugin::OnScanStreamCancel() { _event_sink = nullptr; }


void FlutterDesktopScannerPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare("getPlatformVersion") == 0) {
    std::ostringstream version_stream;
    version_stream << "Windows ";
    if (IsWindows10OrGreater()) {
      version_stream << "10+";
    } else if (IsWindows8OrGreater()) {
      version_stream << "8";
    } else if (IsWindows7OrGreater()) {
      version_stream << "7";
    }
    result->Success(flutter::EncodableValue(version_stream.str()));
   } else if (method_call.method_name().compare("getScanners") == 0) {
    CreateThread(NULL, 0, HandleGetDevices, NULL, 0, NULL);
    result->Success(flutter::EncodableValue(true));
   } else {
    result->NotImplemented();
  }
}

}  // namespace flutter_desktop_scanner
