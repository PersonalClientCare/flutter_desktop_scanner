import 'dart:typed_data';

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
  Stream<List<Scanner>> getDevicesStream() {
    // TODO: implement getDevicesStream
    throw UnimplementedError();
  }

  @override
  Stream<Image?> imageReprStream() {
    // TODO: implement imageReprStream
    throw UnimplementedError();
  }

  @override
  Future<bool> initGetDevices() {
    // TODO: implement initGetDevices
    throw UnimplementedError();
  }

  @override
  Future<bool> initScan(Scanner scanner) {
    // TODO: implement initScan
    throw UnimplementedError();
  }

  @override
  Stream<Uint8List> rawBytesStream() {
    // TODO: implement rawPNMStream
    throw UnimplementedError();
  }
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
