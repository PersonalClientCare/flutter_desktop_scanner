import 'dart:typed_data';

import 'package:flutter_desktop_scanner/classes.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_desktop_scanner/flutter_desktop_scanner.dart';
import 'package:flutter_desktop_scanner/flutter_desktop_scanner_platform_interface.dart';
import 'package:flutter_desktop_scanner/flutter_desktop_scanner_method_channel.dart';
import 'package:image/image.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockFlutterDesktopScannerPlatform
    with MockPlatformInterfaceMixin
    implements FlutterDesktopScannerPlatform {
  @override
  Future<String?> getPlatformVersion() => Future.value('42');

  @override
  Future<List<Scanner>> getScanners() => Future.value([]);

  @override
  Future<Uint8List> getRawPNMBytes(String scannerName) =>
      Future.value(Uint8List.fromList([]));

  @override
  Future<Image> getImageRepr(String scannerName) => Future.value(Image.empty());
}

void main() {
  final FlutterDesktopScannerPlatform initialPlatform =
      FlutterDesktopScannerPlatform.instance;

  test('$MethodChannelFlutterDesktopScanner is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelFlutterDesktopScanner>());
  });

  test('getPlatformVersion', () async {
    FlutterDesktopScanner flutterDesktopScannerPlugin = FlutterDesktopScanner();
    MockFlutterDesktopScannerPlatform fakePlatform =
        MockFlutterDesktopScannerPlatform();
    FlutterDesktopScannerPlatform.instance = fakePlatform;

    expect(await flutterDesktopScannerPlugin.getPlatformVersion(), '42');
  });
}
