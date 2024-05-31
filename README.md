<img src="assets/desktop_scanner_logo.png" width="150" alt="flutter desktop scanner logo" />  

# flutter_desktop_scanner

Making scanning with physical scanners on desktop a breeze!

## Getting Started

Because this plugin depends on some system libraries (SANE, WIA, TWAIN) you have to set up them up for linking in your flutter project.  

### Linux

<details>
<summary>Here's how to setup linking</summary>

In `./linux/CMakeLists.txt` under:

```cmake
apply_standard_settings(${BINARY_NAME})
```

add:

```cmake
set(LINKER_FLAGS "-lsane")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
```

</details>

### Windows

Nothing to do here! WIA is generally available as a header file.

### MacOS

## Using the plugin
Because the scanning process for devices, as well as the literal scanning, takes some time the plugin is based on `EventChannels`. `EventChannels` are like `MethodChannels` but for streams, which allow for getting data from native platforms via a stream. Because of that you always have to get a stream refrence before initiating an action.

Here's how to get devices:

```dart
final _flutterDesktopScanenrPlugin = FlutterDesktopScannerPlugin();

...

// always get the stream first
final scannerStream = _flutterDesktopScannerPlugin.getDevicesStream();

scannerStream.listen((scanners) {
    setState(() {
    _scanners = scanners;
    _loading = false;
    });
});

// then call the init method
await _flutterDesktopScannerPlugin.initGetDevices();
```

And here how you can initiate a scan:

```dart
final _flutterDesktopScanenrPlugin = FlutterDesktopScannerPlugin();

...

// again get the stream first;
// imageReprStream returns a Image class that can be
// encoded into any image format
final stream = _flutterDesktopScannerPlugin.imageReprStream();

stream.listen((image) {
    setState(() {
    if (image != null) {
        _imgBytes = img.encodePng(image);
    }
    _imgLoading = false;
    });
});

// and then init the scan
await _flutterDesktopScannerPlugin.initScan(scanner);
```

## Scan representations

`imageReprStream`: This stream returns an `Image` class that can be encoded into any image format (JPG, PNG, ...) for a full list see [here](https://github.com/brendan-duncan/image/blob/main/doc/formats.md)

`rawBytesStream`: This stream returns a `Uint8List` containing the raw image bytes.  
The format is different on each platform:
- For linux it's PNM.
- For windows it's (most likely) JPEG.
- For MacOS it's TODO.
