import Cocoa
import Foundation
import ImageCaptureCore
import FlutterMacOS

class ScannerHandler: NSObject, ICScannerDeviceDelegate {
    private var eventSink: FlutterEventSink?
    private var scanner: ICScannerDevice?
    
    init(scanner: ICScannerDevice, eventSink: FlutterEventSink) {
        super.init()
        self.scanner = scanner
        self.scanner?.delegate = self
        self.eventSink = eventSink
    }
    
    func startScan() {
        guard let scanner = scanner else {
            print("No scanner selected")
            return
        }
        
        guard let functionalUnit = scanner.selectedFunctionalUnit else {
            print("No functional unit selected")
            return
        }
        
        // Configure the functional unit as needed, e.g., set resolution, color mode, etc.
        functionalUnit.scanInProgress = true
        scanner.requestScan()
    }
    
    func scannerDevice(_ scanner: ICScannerDevice, didScanTo url: URL, data: Data?) -> [UInt8] {        
        // Save the scanned data to a byte buffer
        if let imageData = data {
            eventSink?([UInt8](data))
        }
    }
    
    func scannerDevice(_ scanner: ICScannerDevice, didCompleteOverviewScanWithError error: Error?) {
        if let error = error {
            print("Overview scan completed with error: \(error.localizedDescription)")
        } else {
            print("Overview scan completed successfully")
        }
    }
    
    func scannerDevice(_ scanner: ICScannerDevice, didCompleteScanWithError error: Error?) {
        if let error = error {
            print("Scan completed with error: \(error.localizedDescription)")
        } else {
            print("Scan completed successfully")
        }
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
    
    func scannerDevice(_ scanner: ICScannerDevice, didReceive statusInformation: [String : Any]) {
        print("Received status information: \(statusInformation)")
    }

    func scannerDeviceDidBecomeAvailable(_ scanner: ICScannerDevice) {
        print("Scanner device did become available: \(scanner.name ?? "Unknown")")
    }

    func didRemove(_ device: ICDevice) {
        print("Did remove for: \(device.name ?? "Unknown")")
    }
}

class ScanStreamHandler: NSObject, FlutterStreamHandler {
    private var eventSink: FlutterEventSink?
    private let scannerManager: ScannerManager = ScannerManager.shared

    func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
        eventSink = events
        return nil
    }

    func onCancel(withArguments arguments: Any?) -> FlutterError? {
        eventSink = nil
        return nil
    }

    func initScan(scannerId: String) {
        let scanner = scannerManager.getScannerById(scannerId)
        let scannerHandler = ScannerHandler(scanner, eventSink)
        scannerHandler.startScan()
    }
}