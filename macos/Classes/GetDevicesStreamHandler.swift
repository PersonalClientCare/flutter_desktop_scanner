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
        deviceBrowser?.browsedDeviceTypeMask = ICDeviceTypeMask.scanner
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
    
    func device(_ device: ICDevice, didCloseSessionWithError error: Error?) {
        if let error = error {
            print("Device closed with error: \(error.localizedDescription)")
        } else {
            print("Device closed successfully")
        }
    }

    func device(_ device: ICDevice, didOpenSessionWithError error: Error?) {
        if let error = error {
            print("Device opened with error: \(error.localizedDescription)")
        } else {
            print("Device opened successfully")
        }
    }
    
    func deviceDidBecomeReady(_ device: ICDevice) {
        print("Device is ready: \(device.name ?? "Unknown")")
    }
    
    // ICScannerDeviceDelegate methods
    func scannerDeviceDidBecomeAvailable(_ scanner: ICScannerDevice) {
        print("Scanner device did become available: \(scanner.name ?? "Unknown")")
    }

    func didRemove(_ device: ICDevice) {
        print("Did remove for: \(device.name ?? "Unknown")")
    }
    
}

class GetDevicesStreamHandler: NSObject, FlutterStreamHandler {
    private var eventSink: FlutterEventSink?
    private let scannerManager: ScannerManager = ScannerManager()

    func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
        eventSink = events
        return nil
    }

    func onCancel(withArguments arguments: Any?) -> FlutterError? {
        eventSink = nil
        return nil
    }

    func getDevices() {
        print("Starting to get devices")
        let rawScanners = scannerManager.listScanners()
        print("got scanners")
        var scannerResult = [[String: String?]]()
        for scanner in rawScanners {
            let result = ["name": scanner.name,
                "model": scanner.persistentIDString,
                "vendor": "macOS",
                "type": scanner.productKind
            ]
            print("Got device \(result)")
            scannerResult.append(result)
        }
        eventSink?(scannerResult)
        scannerManager.stopScanningForDevices()
    }
}