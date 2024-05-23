import 'dart:isolate';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_desktop_scanner/classes.dart';

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

  _getScannersIsolate(_IsolateData data) async {
    BackgroundIsolateBinaryMessenger.ensureInitialized(data.token);
    final rawScanners = await methodChannel
        .invokeListMethod<Map<Object?, Object?>>("getScanners");
    if (rawScanners == null) {
      data.answerPort.send([]);
    } else {
      data.answerPort.send([
        for (final scanner in rawScanners)
          Scanner(scanner["name"].toString(), scanner["vendor"].toString(),
              scanner["model"].toString(), scanner["type"].toString())
      ]);
    }
  }

  @override
  Future<List<Scanner>> getScanners() async {
    final p = ReceivePort();
    final rootToken = RootIsolateToken.instance!;

    await Isolate.spawn<_IsolateData>(_getScannersIsolate,
        _IsolateData(token: rootToken, answerPort: p.sendPort));

    return await p.first;
  }
}
