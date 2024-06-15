import 'package:flutter/material.dart';
import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter_desktop_scanner/flutter_desktop_scanner.dart';
import 'package:image/image.dart' as img;

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  String _platformVersion = 'Unknown';
  final _flutterDesktopScannerPlugin = FlutterDesktopScanner();
  List<Scanner> _scanners = [];
  bool _loading = false;
  bool _imgLoading = false;
  Uint8List? _imgBytes;

  @override
  void initState() {
    super.initState();
    initPlatformState();
  }

  // Platform messages are asynchronous, so we initialize in an async method.
  Future<void> initPlatformState() async {
    String platformVersion;
    // Platform messages may fail, so we use a try/catch PlatformException.
    // We also handle the message potentially returning null.
    try {
      platformVersion =
          await _flutterDesktopScannerPlugin.getPlatformVersion() ??
              'Unknown platform version';
    } on PlatformException {
      platformVersion = 'Failed to get platform version.';
    }

    // If the widget was removed from the tree while the asynchronous platform
    // message was in flight, we want to discard the reply rather than calling
    // setState to update our non-existent appearance.
    if (!mounted) return;

    setState(() {
      _platformVersion = platformVersion;
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Plugin example app'),
        ),
        body: Center(
          child: Column(
            children: [
              Text('Running on: $_platformVersion\n'),
              ElevatedButton(
                onPressed: _loading ? null : _setScanner,
                child: _loading
                    ? const SizedBox(
                        width: 24.0,
                        height: 24.0,
                        child: CircularProgressIndicator.adaptive())
                    : const Text("Get scanner"),
              ),
              ...List.generate(
                _scanners.length,
                (index) => MouseRegion(
                  cursor: SystemMouseCursors.click,
                  child: GestureDetector(
                    onTap: () => _initiateScan(_scanners[index]),
                    child: Text(
                        "${_scanners[index].name} ${_scanners[index].model}"),
                  ),
                ),
              ),
              if (_imgBytes != null)
                Image.memory(
                  _imgBytes!,
                  width: 250,
                ),
              if (_imgLoading) const CircularProgressIndicator.adaptive(),
            ],
          ),
        ),
      ),
    );
  }

  _setScanner() async {
    setState(() {
      _loading = true;
      _scanners = [];
    });
    final scannerStream = _flutterDesktopScannerPlugin.getDevicesStream();
    scannerStream.listen((scanners) {
      setState(() {
        _scanners = scanners;
        _loading = false;
      });
    });
    await _flutterDesktopScannerPlugin.initGetDevices();
  }

  _initiateScan(Scanner scanner) async {
    setState(() {
      _imgLoading = true;
    });
    final stream = _flutterDesktopScannerPlugin.imageReprStream();
    stream.listen((image) {
      setState(() {
        if (image != null) {
          _imgBytes = img.encodePng(image);
        }
        _imgLoading = false;
      });
    });
    await _flutterDesktopScannerPlugin.initScan(scanner);
  }
}
