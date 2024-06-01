import 'dart:async';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_desktop_scanner/classes.dart';
import 'package:image/image.dart' as img;

import 'flutter_desktop_scanner_platform_interface.dart';

/// An implementation of [FlutterDesktopScannerPlatform] that uses method channels.
class MethodChannelFlutterDesktopScanner extends FlutterDesktopScannerPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel =
      const MethodChannel("personalclientcare/flutter_desktop_scanner");

  @visibleForTesting
  final getDevicesEventChannel = const EventChannel(
      "personalclientcare/flutter_desktop_scanner_get_devices");

  @visibleForTesting
  final scanEventChannel =
      const EventChannel("personalclientcare/flutter_desktop_scanner_scan");

  @override
  Future<String?> getPlatformVersion() async {
    final version =
        await methodChannel.invokeMethod<String>("getPlatformVersion");
    return version;
  }

  @override
  Future<bool> initGetDevices() async {
    final response = await methodChannel.invokeMethod<bool>("getScanners");
    if (response == null) return false;
    return response;
  }

  @override
  Stream<List<Scanner>> getDevicesStream() async* {
    final stream = getDevicesEventChannel.receiveBroadcastStream();
    await for (final rawScanners in stream) {
      yield [
        for (final scanner in rawScanners ?? [])
          Scanner(scanner["name"].toString(), scanner["vendor"].toString(),
              scanner["model"].toString(), scanner["type"].toString())
      ];
    }
  }

  @override
  Future<bool> initScan(Scanner scanner) async {
    var identifier = scanner.name;
    if (Platform.isWindows) {
      identifier = scanner.model;
    }
    final response = await methodChannel
        .invokeMethod<bool>("initiateScan", {"scannerName": identifier});
    if (response == null) return false;
    return response;
  }

  @override
  Stream<Uint8List?> rawBytesStream() async* {
    final stream = scanEventChannel.receiveBroadcastStream();
    await for (final bytes in stream) {
      yield bytes;
    }
  }

  @override
  Stream<img.Image?> imageReprStream() async* {
    final stream = scanEventChannel.receiveBroadcastStream();
    await for (final bytes in stream) {
      if (bytes != null) {
        if (Platform.isWindows) {
          var decodedImg = img.decodeJpg(bytes);
          // try out some other formats before returning null eventually
          decodedImg ??= img.decodePng(bytes);
          decodedImg ??= img.decodeBmp(bytes);
          decodedImg ??= img.decodeTiff(bytes);
          yield decodedImg;
        } else if (Platform.isLinux) {
          yield img.decodePnm(bytes);
        }
      } else {
        yield null;
      }
    }
  }
}
