import Cocoa
import FlutterMacOS

public class FlutterDesktopScannerPlugin: NSObject, FlutterPlugin {
  private static let devicesHandler = GetDevicesStreamHandler()

  public static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(name: "personalclientcare/flutter_desktop_scanner", binaryMessenger: registrar.messenger)
    let getDevicesChannel = FlutterEventChannel(name: "personalclientcare/flutter_desktop_scanner_get_devices", binaryMessenger: registrar.messenger)
    
    let instance = FlutterDesktopScannerPlugin()
    getDevicesChannel.setStreamHandler(devicesHandler)
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
      default:
        result(FlutterMethodNotImplemented)
    }
  }
}
