import 'package:flutter_desktop_scanner/classes.dart';

import 'flutter_desktop_scanner_platform_interface.dart';

export 'classes.dart';

class FlutterDesktopScanner {
  Future<String?> getPlatformVersion() {
    return FlutterDesktopScannerPlatform.instance.getPlatformVersion();
  }

  Future<List<Scanner>> getScanners() {
    return FlutterDesktopScannerPlatform.instance.getScanners();
  }
}
