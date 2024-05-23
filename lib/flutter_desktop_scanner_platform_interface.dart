import 'package:flutter_desktop_scanner/classes.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'flutter_desktop_scanner_method_channel.dart';

abstract class FlutterDesktopScannerPlatform extends PlatformInterface {
  /// Constructs a FlutterDesktopScannerPlatform.
  FlutterDesktopScannerPlatform() : super(token: _token);

  static final Object _token = Object();

  static FlutterDesktopScannerPlatform _instance =
      MethodChannelFlutterDesktopScanner();

  /// The default instance of [FlutterDesktopScannerPlatform] to use.
  ///
  /// Defaults to [MethodChannelFlutterDesktopScanner].
  static FlutterDesktopScannerPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [FlutterDesktopScannerPlatform] when
  /// they register themselves.
  static set instance(FlutterDesktopScannerPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<String?> getPlatformVersion() {
    throw UnimplementedError("platformVersion() has not been implemented.");
  }

  /// Returns a list of scanners found by
  Future<List<Scanner>> getScanners() {
    throw UnimplementedError("getScanners() has not been implemented.");
  }
}
