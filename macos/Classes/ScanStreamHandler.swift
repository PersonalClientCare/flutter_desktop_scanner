import Cocoa
import Foundation
import ImageCaptureCore
import FlutterMacOS

class ScannerHandler: NSObject, ICScannerDeviceDelegate {
    private var scanner: ICScannerDevice?
    private var scanCompletion: ((Data?) -> Void)?
    
    init(scanner: ICScannerDevice) {
        super.init()
        self.scanner = scanner
        self.scanner?.delegate = self
    }
    
    func startScan(completion: @escaping (Data?) -> Void) {
        guard let scanner = scanner else {
            print("No scanner selected")
            return
        }
        
        // Access the functional unit and cast it to the correct type
        let functionalUnit = scanner.selectedFunctionalUnit
        // Store the completion handler
        scanCompletion = completion
            
        // Configure the functional unit as needed, e.g., set resolution, color mode, etc.
        functionalUnit.resolution = 300

        scanner.requestScan()
    }
    
    func scannerDevice(_ scanner: ICScannerDevice, didScanTo url: URL) {        
        // Read the scanned data from the URL
        do {
            let scannedData = try Data(contentsOf: url)
            scanCompletion?(scannedData)
        } catch {
            print("Error reading scanned data: \(error)")
            scanCompletion?(nil)
        }
    }

    func scannerDevice(_ scanner: ICScannerDevice, didScanToBandData data: Data) {
        // Call the completion handler with the scanned data
        scanCompletion?(data)
    }
    
    func scannerDevice(_ scanner: ICScannerDevice, didCompleteOverviewScanWithError error: Error?) {
        if let error = error {
            print("Overview scan completed with error: \(error.localizedDescription)")
            scanCompletion?(nil)
        } else {
            print("Overview scan completed successfully")
        }
    }
    
    func scannerDevice(_ scanner: ICScannerDevice, didCompleteScanWithError error: Error?) {
        if let error = error {
            print("Scan completed with error: \(error.localizedDescription)")
            scanCompletion?(nil)
        } else {
            print("Scan completed successfully")
        }
    }
    
    func scannerDevice(_ scanner: ICScannerDevice, didReceive statusInformation: [String : Any]) {
        print("Received status information: \(statusInformation)")
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

    func device(_ device: ICDevice, didEncounterError error: Error?) {
        if let error = error {
            print("Device encountered error: \(error.localizedDescription)")
        }
    }
    
    func deviceDidBecomeReady(_ device: ICDevice) {
        print("Device is ready: \(device.name ?? "Unknown")")
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
        scannerManager.getScannerById(scannerId) { scanner in
            if let scanner = scanner {
                let scannerHandler = ScannerHandler(scanner: scanner)
                scannerHandler.startScan { data in
                    if let imageData = data {
                        // Handle the scanned image data
                        let byteBuffer = [UInt8](imageData)
                        self.eventSink?(byteBuffer)
                    } else {
                        print("Failed to scan image")
                    }
                }
            } else {
                print("Scanner with ID \(scannerId) not found")
            }
        }
    }
}
