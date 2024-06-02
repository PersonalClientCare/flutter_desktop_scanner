import Cocoa
import ImageCaptureCore
import FlutterMacOS

class ScannerManager: NSObject, ICDeviceBrowserDelegate, ICScannerDeviceDelegate {
    private var deviceBrowser: ICDeviceBrowser?
    private var scannerDevices: [ICScannerDevice] = []
    
    override init() {
        super.init()
        deviceBrowser = ICDeviceBrowser()
        deviceBrowser?.delegate = self
        deviceBrowser?.browsedDeviceTypeMask = ICDeviceTypeMask.scanner.rawValue
    }
    
    func startScanningForDevices() {
        deviceBrowser?.start()
    }
    
    func stopScanningForDevices() {
        deviceBrowser?.stop()
    }
    
    // ICDeviceBrowserDelegate method
    func deviceBrowser(_ browser: ICDeviceBrowser, didAdd device: ICDevice, moreComing: Bool) {
        if let scannerDevice = device as? ICScannerDevice {
            scannerDevices.append(scannerDevice)
            scannerDevice.delegate = self
        }
    }
    
    func deviceBrowser(_ browser: ICDeviceBrowser, didRemove device: ICDevice, moreGoing: Bool) {
        if let scannerDevice = device as? ICScannerDevice {
            if let index = scannerDevices.firstIndex(of: scannerDevice) {
                scannerDevices.remove(at: index)
            }
        }
    }
    
    func listScanners() -> [ICScannerDevice] {
        return scannerDevices
    }
    
}

class GetDevicesStreamHandler: NSObject, FlutterStreamHandler {
    var eventSink: FlutterEventSink?
    var scannerManager: ScannerManager

    override init() {
        super.init()
        scannerManager = ScannerManager()
    }

    func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
        eventSink = events
        return nil
    }

    func onCancel(withArguments arguments: Any?) -> FlutterError? {
        eventSink = nil
        return nil
    }

    func getDevices() {
        let rawScanners = scannerManager.listScanners()
        let scannerResult = []
        for scanner in rawScanners {
            scannerResult.append({
                "name": scanner.name,
                "model": scanner.persistentIDString,
                "vendor": "macOS",
                "type": scanner.productKind
            })
        }
        eventSink?(scanners)
        scannerManager.stopScanningForDevices()
    }
}