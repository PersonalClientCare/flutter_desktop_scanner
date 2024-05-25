import 'dart:isolate';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_desktop_scanner/classes.dart';
import 'package:image/image.dart';

import 'flutter_desktop_scanner_platform_interface.dart';

class _IsolateData {
  final RootIsolateToken token;
  final SendPort answerPort;

  _IsolateData({
    required this.token,
    required this.answerPort,
  });
}

/// An implementation of [FlutterDesktopScannerPlatform] that uses method channels.
class MethodChannelFlutterDesktopScanner extends FlutterDesktopScannerPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel =
      const MethodChannel("personalclientcare/flutter_desktop_scanner");

  @override
  Future<String?> getPlatformVersion() async {
    final version =
        await methodChannel.invokeMethod<String>("getPlatformVersion");
    return version;
  }

  @override
  Future<List<Scanner>> getScanners() async {
    final rawScanners = await methodChannel
        .invokeListMethod<Map<Object?, Object?>>("getScanners");
    if (rawScanners == null) return [];
    return [
      for (final scanner in rawScanners)
        Scanner(scanner["name"].toString(), scanner["vendor"].toString(),
            scanner["model"].toString(), scanner["type"].toString())
    ];
  }

  @override
  Future<Uint8List> getRawPNMBytes(String scannerName) async {
    final bytes = await methodChannel
        .invokeMethod<Uint8List>("initiateScan", {"scannerName": scannerName});
    if (bytes == null) return Uint8List(0);
    return bytes;
  }

  @override
  Future<Image?> getImageRepr(String scannerName) async {
    final bytes = await methodChannel
        .invokeMethod<Uint8List>("initiateScan", {"scannerName": scannerName});
    if (bytes == null) return Image.empty();
    return decodePnm(bytes);
  }
}
