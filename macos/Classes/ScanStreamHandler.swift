import Cocoa
import Foundation
import ImageCaptureCore
import FlutterMacOS
import CoreGraphics

extension CGImage {
    /**
            Converts the given CGImage int a PNG Data? object
     */
    var png: Data? {
        guard let mutableData = CFDataCreateMutable(nil, 0),
              let destination = CGImageDestinationCreateWithData(mutableData, "public.png" as CFString, 1, nil) else { return nil }
        CGImageDestinationAddImage(destination, self, nil)
        guard CGImageDestinationFinalize(destination) else { return nil }
        return mutableData as Data
    }
}

class ScannerHandler: NSObject, ICScannerDeviceDelegate {
    private var eventSink: FlutterEventSink?
    private var scanner: ICScannerDevice?
    
    init(scanner: ICScannerDevice, eventSink: FlutterEventSink?) {
        super.init()
        self.scanner = scanner
        self.scanner?.delegate = self
        self.eventSink = eventSink
    }
    
    func startScan() {
        // Access the functional unit
        let functionalUnit = scanner!.selectedFunctionalUnit
        
        // Configure the functional unit as needed, e.g., set resolution, color mode, etc.
        functionalUnit.resolution = 300
        functionalUnit.bitDepth = ICScannerBitDepth.depth8Bits
        
        scanner?.requestOpenSession()
    }
    
    func scannerDevice(_ scanner: ICScannerDevice, didScanTo url: URL) {        
        // Read the scanned data from the URL
        do {
            let scannedData = try Data(contentsOf: url)
            eventSink?([UInt8](scannedData))
        } catch {
            print("Error reading scanned data: \(error)")
            eventSink?(nil)
        }
    }
    
    func scannerDevice(_ scanner: ICScannerDevice, didScanTo data: ICScannerBandData) {
        print("Got memory based data \(data.dataBuffer!)")
        eventSink?([UInt8](data.dataBuffer!))
    }
    
    func scannerDevice(_ scanner: ICScannerDevice, didCompleteOverviewScanWithError error: Error?) {
        if let error = error {
            print("Overview scan completed with error: \(error.localizedDescription)")
            eventSink?(nil)
        } else {
            print("Overview scan completed successfully")
            let funcUnit = scanner.selectedFunctionalUnit
            guard let image = funcUnit.overviewImage else {
                print("Overview image is not available.")
                return
            }
            
            eventSink?([UInt8](image.png!))
            
            scanner.requestCloseSession()
        }
    }
    
    func scannerDevice(_ scanner: ICScannerDevice, didCompleteScanWithError error: Error?) {
        if let error = error {
            print("Scan completed with error: \(error.localizedDescription)")
            eventSink?(nil)
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
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            self.scanner?.requestOverviewScan()
        }
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
    private var scannerHandler: ScannerHandler?

    func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
        eventSink = events
        return nil
    }

    func onCancel(withArguments arguments: Any?) -> FlutterError? {
        eventSink = nil
        return nil
    }

    func initScan(scannerId: String) {
        // add artificial delay to ensure onListen was called and eventSink != nil
        DispatchQueue.main.asyncAfter(deadline: .now() + 2.5) {
            self.scannerManager.getScannerById(scannerId) { scanner in
                if let scanner = scanner {
                    self.scannerHandler = ScannerHandler(scanner: scanner, eventSink: self.eventSink)
                    self.scannerHandler!.startScan()
                } else {
                    print("Scanner with ID \(scannerId) not found")
                }
            }
        }
    }
}
