import Cocoa
import FlutterMacOS

public class FlutterDesktopScannerPlugin: NSObject, FlutterPlugin {
  private static let devicesHandler = GetDevicesStreamHandler()
  private static let scanHandler = ScanStreamHandler()

  public static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(name: "personalclientcare/flutter_desktop_scanner", binaryMessenger: registrar.messenger)
    let getDevicesChannel = FlutterEventChannel(name: "personalclientcare/flutter_desktop_scanner_get_devices", binaryMessenger: registrar.messenger)
    let scanChannel = FlutterEventChannel(name: "personalclientcare/flutter_desktop_scanner_scan", binaryMessenger: registrar.messenger)

    let instance = FlutterDesktopScannerPlugin()
    getDevicesChannel.setStreamHandler(devicesHandler)
    scanChannel.setStreamHandler(scanHandler)
    registrar.addMethodCallDelegate(instance, channel: channel)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
      case "getPlatformVersion":
        result("macOS " + ProcessInfo.processInfo.operatingSystemVersionString)
      case "getScanners":
        DispatchQueue.global(qos: .background).async {
          FlutterDesktopScannerPlugin.devicesHandler.getDevices()
        }
        result(true)
      case "initiateScan":
        let args = call.arguments as? Dictionary<String, Any>
        let scannerId = args!["scannerName"] as? String
        DispatchQueue.global(qos: .background).async {
          FlutterDesktopScannerPlugin.scanHandler.initScan(scannerId: scannerId!)
        }
        result(true)
      default:
        result(FlutterMethodNotImplemented)
    }
  }
}
