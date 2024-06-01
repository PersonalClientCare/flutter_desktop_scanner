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
#include <vector>
#include <thread>

#include <wia.h>
#include <comdef.h>

#pragma comment(lib, "wiaguid.lib")

namespace flutter_desktop_scanner {

// helper classes
class MemoryStream : public IStream {
    std::vector<BYTE> m_data;
    ULONG m_refCount;
    ULONGLONG m_position;
public:
    MemoryStream() : m_refCount(1), m_position(0) {}

    STDMETHODIMP_(ULONG) AddRef() override {
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) Release() override {
        ULONG cRef = InterlockedDecrement(&m_refCount);
        if (cRef == 0) {
            delete this;
        }
        return cRef;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == IID_IUnknown || riid == IID_IStream) {
            *ppvObject = static_cast<IStream*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead) override {
        if (m_position >= m_data.size()) {
            if (pcbRead) *pcbRead = 0;
            return S_FALSE;
        }

        ULONG bytesToRead = static_cast<ULONG>(min(cb, m_data.size() - m_position));
        memcpy(pv, &m_data[m_position], bytesToRead);
        m_position += bytesToRead;

        if (pcbRead) *pcbRead = bytesToRead;
        return S_OK;
    }

    STDMETHODIMP Write(const void* pv, ULONG cb, ULONG* pcbWritten) override {
        if (m_position + cb > m_data.size()) {
            m_data.resize(m_position + cb);
        }

        memcpy(&m_data[m_position], pv, cb);
        m_position += cb;

        if (pcbWritten) *pcbWritten = cb;
        return S_OK;
    }

    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) override {
        LONGLONG newPosition = 0;

        switch (dwOrigin) {
        case STREAM_SEEK_SET:
            newPosition = dlibMove.QuadPart;
            break;
        case STREAM_SEEK_CUR:
            newPosition = m_position + dlibMove.QuadPart;
            break;
        case STREAM_SEEK_END:
            newPosition = m_data.size() + dlibMove.QuadPart;
            break;
        default:
            return STG_E_INVALIDFUNCTION;
        }

        if (newPosition < 0) return STG_E_SEEKERROR;

        m_position = static_cast<ULONGLONG>(newPosition);
        if (plibNewPosition) plibNewPosition->QuadPart = m_position;
        return S_OK;
    }

    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize) override {
        m_data.resize(libNewSize.QuadPart);
        return S_OK;
    }

    STDMETHODIMP CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) override {
        ULONG bytesToCopy = static_cast<ULONG>(min(cb.QuadPart, m_data.size() - m_position));
        ULONG bytesRead = 0;
        ULONG bytesWritten = 0;

        HRESULT hr = pstm->Write(&m_data[m_position], bytesToCopy, &bytesWritten);
        if (SUCCEEDED(hr)) {
            bytesRead = bytesToCopy;
            m_position += bytesToCopy;
        }

        if (pcbRead) pcbRead->QuadPart = bytesRead;
        if (pcbWritten) pcbWritten->QuadPart = bytesWritten;
        return hr;
    }

    STDMETHODIMP Commit(DWORD grfCommitFlags) override {
        return S_OK;
    }

    STDMETHODIMP Revert() override {
        return S_OK;
    }

    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override {
        return STG_E_INVALIDFUNCTION;
    }

    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override {
        return STG_E_INVALIDFUNCTION;
    }

    STDMETHODIMP Stat(STATSTG* pstatstg, DWORD grfStatFlag) override {
        if (!pstatstg) return STG_E_INVALIDPOINTER;

        ZeroMemory(pstatstg, sizeof(STATSTG));
        pstatstg->type = STGTY_STREAM;
        pstatstg->cbSize.QuadPart = m_data.size();
        return S_OK;
    }

    STDMETHODIMP Clone(IStream** ppstm) override {
        return E_NOTIMPL;
    }

    std::vector<BYTE>& GetData() {
        return m_data;
    }
};

class WiaTransferCallback : public IWiaTransferCallback {
    ULONG m_cRef;
    IStream* m_pStream;

public:
    WiaTransferCallback(IStream* pStream) : m_cRef(1), m_pStream(pStream) {}

    STDMETHODIMP_(ULONG) AddRef() override {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG) Release() override {
        ULONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0) {
            delete this;
        }
        return cRef;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == IID_IUnknown || riid == IID_IWiaTransferCallback) {
            *ppvObject = static_cast<IWiaTransferCallback*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP TransferCallback(LONG lFlags, WiaTransferParams* pWiaTransferParams) override {
        if (pWiaTransferParams->lMessage == WIA_TRANSFER_MSG_STATUS) {
            std::cout << "Transfer status: " << pWiaTransferParams->ulTransferredBytes << " bytes transferred.\n";
        }
        return S_OK;
    }

    STDMETHODIMP GetNextStream(LONG lFlags, BSTR bstrItemName, BSTR bstrFullItemName, IStream** ppDestination) override {
        *ppDestination = m_pStream;
        if (m_pStream) m_pStream->AddRef();
        return S_OK;
    }
};

std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> FlutterDesktopScannerPlugin::_devices_event_sink = nullptr;
std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> FlutterDesktopScannerPlugin::_scan_event_sink = nullptr;

// static

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

void FlutterDesktopScannerPlugin::HandleGetDevices() {
  HRESULT hr = CoInitialize(nullptr);
  if (FAILED(hr)) {
    _devices_event_sink->Error("WIA_ERROR", "Failed to initialize COM interface");
    return;
  }

  IWiaDevMgr2 *dev_manager;
  hr = CoCreateInstance(CLSID_WiaDevMgr2, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr2, (void**)&dev_manager);
  if (FAILED(hr)) {
      CoUninitialize();
      _devices_event_sink->Error("WIA_ERROR", "Failed to create WIA device manager");
      return;
  }

  IEnumWIA_DEV_INFO *dev_info_enum;
  hr = dev_manager->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL, &dev_info_enum);
  if (FAILED(hr)) {
      CoUninitialize();
      _devices_event_sink->Error("WIA_ERROR", "Failed to enumerate WIA devices");
      return;
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
      CoUninitialize();
      _devices_event_sink->Error("WIA_ERROR", "Failed to get WIA device info");
      return;
    }

    // get device name
    PROPSPEC dev_name_spec;
    dev_name_spec.ulKind = PRSPEC_PROPID;
    dev_name_spec.propid = WIA_DIP_DEV_NAME;

    PROPVARIANT dev_name;
    hr = dev_info->ReadMultiple(1, &dev_name_spec, &dev_name);
    if (FAILED(hr)) {
      CoUninitialize();
      _devices_event_sink->Error("WIA_ERROR", "Failed to get WIA device name");
      return;
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
      CoUninitialize();
      _devices_event_sink->Error("WIA_ERROR", "Failed to get WIA model");
      return;
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
      CoUninitialize();
      _devices_event_sink->Error("WIA_ERROR", "Failed to get WIA vendor");
      return;
    }

    auto wide_vendor = std::wstring(dev_vendor.bstrVal);

    std::string vendor(wide_vendor.length(), 0);
    std::transform(wide_vendor.begin(), wide_vendor.end(), vendor.begin(), [] (wchar_t c) {
      return (char)c;
    });

    flutter::EncodableMap device = {
      {"name", name},
      {"model", model},
      {"vendor", vendor},
      {"type", "Windows Scanner Device"} // TODO: find a way to get real scanner type WIA_DIP_DEV_TYPE
    };

    devices.push_back(device);
  }

  _devices_event_sink->Success(devices);

  dev_info_enum->Release();
  dev_manager->Release();
}

std::wstring ConvertAnsiToWide(const std::string& str)
{
    int count = MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.length()), NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.length()), &wstr[0], count);
    return wstr;
}

void FlutterDesktopScannerPlugin::HandleScan(std::string deviceId) {
  std::vector<BYTE> scannedData;

  std::wstring wide_id = ConvertAnsiToWide(deviceId);

  // Initialize COM library
  HRESULT hr = CoInitialize(nullptr);
  if (FAILED(hr)) {
    _devices_event_sink->Error("WIA_ERROR", "Failed to initialize COM interface");
    return;
  }

  // Create WIA device manager
  IWiaDevMgr2 *pWiaDevMgr;
  hr = CoCreateInstance(CLSID_WiaDevMgr2, nullptr, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr2, (void**)&pWiaDevMgr);
  if (FAILED(hr)) {
    CoUninitialize();
    _devices_event_sink->Error("WIA_ERROR", "Failed to create WIA device manager");
    return;
  }

  // Create WIA device using the device ID
  IWiaItem2 *pWiaRootItem;
  hr = pWiaDevMgr->CreateDevice(0, SysAllocStringLen(wide_id.data(), static_cast<unsigned int>(wide_id.size())), &pWiaRootItem);
  if (FAILED(hr)) {
    CoUninitialize();
    _devices_event_sink->Error("WIA_ERROR", "Failed to create device");
    return;
  }

  // Enumerate child items to find the scanner items
  IEnumWiaItem2 *pEnumItem;
  hr = pWiaRootItem->EnumChildItems(nullptr, &pEnumItem);
  if (FAILED(hr)) {
    CoUninitialize();
    _devices_event_sink->Error("WIA_ERROR", "Failed to enumerate items");
    return;
  }

  // Find the scanner item
  IWiaItem2 *pWiaScanItem;
  ULONG ulFetched = 0;
  while (pEnumItem->Next(1, &pWiaScanItem, &ulFetched) == S_OK) {
      LONG itemType = 0;
      pWiaScanItem->GetItemType(&itemType);

      if (itemType & WiaItemTypeImage) {
          break; // Found the image item
      }

      pWiaScanItem->Release(); // Release the item if it is not the desired type
  }

  if (!pWiaScanItem) {
    CoUninitialize();
    _devices_event_sink->Error("WIA_ERROR", "Failed to find scan item");
    return;
  }

  // Create a memory stream to receive the scanned data
  IStream *pMemoryStream = new MemoryStream();

  // Set up the transfer callback
  IWiaTransfer *pWiaTransfer;
  hr = pWiaScanItem->QueryInterface(IID_IWiaTransfer, (void**)&pWiaTransfer);
  if (FAILED(hr)) {
    CoUninitialize();
    _devices_event_sink->Error("WIA_ERROR", "Failed to initiate wia transfer");
    return;
  }

  WiaTransferCallback* pWiaTransferCallback = new WiaTransferCallback(pMemoryStream);
  hr = pWiaTransfer->Download(0, pWiaTransferCallback);
  if (FAILED(hr)) {
    CoUninitialize();
    _devices_event_sink->Error("WIA_ERROR", "Failed to download transfer data");
    return;
  }
  
  MemoryStream* pMemStream = static_cast<MemoryStream*>(pMemoryStream);
  scannedData = pMemStream->GetData();

  _scan_event_sink->Success(flutter::EncodableValue(scannedData));

  // Clean up
  pWiaTransferCallback->Release();
  CoUninitialize();
}

void FlutterDesktopScannerPlugin::OnGetDevicesStreamListen(std::unique_ptr<flutter::EventSink<>>&& events) {
  _devices_event_sink = std::move(events);
}

void FlutterDesktopScannerPlugin::OnGetDevicesStreamCancel() { _devices_event_sink = nullptr; }

void FlutterDesktopScannerPlugin::OnScanStreamListen(std::unique_ptr<flutter::EventSink<>>&& events) {
  _scan_event_sink = std::move(events);
}

void FlutterDesktopScannerPlugin::OnScanStreamCancel() { _scan_event_sink = nullptr; }


void FlutterDesktopScannerPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

  const auto *args = std::get_if<flutter::EncodableMap>(method_call.arguments());

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
    // again sending responses to event sinks from threads other than the main thread is discouraged by flutter but we must not block the main thread
    std::thread get_devices_thread(HandleGetDevices);
    get_devices_thread.detach();
    result->Success(flutter::EncodableValue(true));
   } else if (method_call.method_name().compare("initiateScan") == 0) {
    auto scannerName = std::get_if<std::string>(&(args->find(flutter::EncodableValue("scannerName")))->second);
    // again sending responses to event sinks from threads other than the main thread is discouraged by flutter but we must not block the main thread
    std::thread scan_thread(HandleScan, *scannerName);
    scan_thread.detach();
    result->Success(flutter::EncodableValue(true));
   } else {
    result->NotImplemented();
  }
}

}  // namespace flutter_desktop_scanner
