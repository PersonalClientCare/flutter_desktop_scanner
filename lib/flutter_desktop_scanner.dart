import 'dart:typed_data';

import 'package:flutter_desktop_scanner/classes.dart';
import 'package:image/image.dart' as img;
import 'flutter_desktop_scanner_platform_interface.dart';

export 'classes.dart';

class FlutterDesktopScanner {
  Future<String?> getPlatformVersion() {
    return FlutterDesktopScannerPlatform.instance.getPlatformVersion();
  }

  Future<bool> initGetDevices() {
    return FlutterDesktopScannerPlatform.instance.initGetDevices();
  }

  Stream<List<Scanner>> getDevicesStream() {
    return FlutterDesktopScannerPlatform.instance.getDevicesStream();
  }

  Future<bool> initScan(String scannerName) {
    return FlutterDesktopScannerPlatform.instance.initScan(scannerName);
  }

  Stream<Uint8List?> rawPNMStream() {
    return FlutterDesktopScannerPlatform.instance.rawPNMStream();
  }

  Stream<img.Image?> imageReprStream() {
    return FlutterDesktopScannerPlatform.instance.imageReprStream();
  }
}
