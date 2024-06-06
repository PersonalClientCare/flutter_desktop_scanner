import Cocoa
import ImageCaptureCore
import FlutterMacOS

class ScannerManager: NSObject, ICDeviceBrowserDelegate, ICScannerDeviceDelegate {
    private var deviceBrowser: ICDeviceBrowser?
    private var scannerDevices: [ICScannerDevice] = []
    private var deviceReadyHandlers: [String: (ICDevice) -> Void] = [:]

    static let shared = ScannerManager() 
    
    private override init() {
        super.init()
        deviceBrowser = ICDeviceBrowser()
        deviceBrowser?.delegate = self
        deviceBrowser?.browsedDeviceTypeMask = ICDeviceTypeMask(rawValue: ICDeviceTypeMask.scanner.rawValue |
            ICDeviceLocationTypeMask.local.rawValue |
            ICDeviceLocationTypeMask.shared.rawValue |
            ICDeviceLocationTypeMask.bonjour.rawValue |
            ICDeviceLocationTypeMask.bluetooth.rawValue |
            ICDeviceLocationTypeMask.remote.rawValue)!
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

            // If there is a ready handler for this device, call it
            if let handler = deviceReadyHandlers[scannerDevice.uuidString!] {
                handler(scannerDevice)
                deviceReadyHandlers.removeValue(forKey: scannerDevice.uuidString!)
            }
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

    func getScannerById(_ scannerId: String, completion: @escaping (ICScannerDevice?) -> Void) {
        if let scanner = scannerDevices.first(where: { $0.uuidString == scannerId }) {
            completion(scanner)
        } else {
            deviceReadyHandlers[scannerId] = { device in
                completion(device as? ICScannerDevice)
            }
        }
    }
    
}

class GetDevicesStreamHandler: NSObject, FlutterStreamHandler {
    private var eventSink: FlutterEventSink?
    private let scannerManager: ScannerManager = ScannerManager.shared

    func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
        scannerManager.startScanningForDevices()
        eventSink = events
        return nil
    }

    func onCancel(withArguments arguments: Any?) -> FlutterError? {
        scannerManager.stopScanningForDevices()
        eventSink = nil
        return nil
    }

    func getDevices() {
        // add artificial delay to ensure devices are found
        DispatchQueue.main.asyncAfter(deadline: .now() + 2.5) {
            let rawScanners = self.scannerManager.listScanners()
            var scannerResult = [[String: String?]]()
            for scanner in rawScanners {
                let result = ["name": scanner.name,
                    "model": scanner.uuidString,
                    "vendor": "macOS",
                    "type": scanner.productKind
                ]
                scannerResult.append(result)
            }
            self.eventSink?(scannerResult)
        }
    }
}