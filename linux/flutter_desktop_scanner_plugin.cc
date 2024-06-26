#include "include/flutter_desktop_scanner/flutter_desktop_scanner_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <sys/utsname.h>
#include <sane/sane.h>
#include <iostream>
#include <memory>
#include <thread>

#include <cstring>

#include "flutter_desktop_scanner_plugin_private.h"

#define FLUTTER_DESKTOP_SCANNER_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), flutter_desktop_scanner_plugin_get_type(), \
                              FlutterDesktopScannerPlugin))

struct _FlutterDesktopScannerPlugin {
  GObject parent_instance;
};

static gboolean send_get_devices_events = FALSE;
static FlEventChannel *get_devices_event_channel = NULL;

static gboolean send_scan_events = FALSE;
static FlEventChannel *scan_event_channel = NULL;

G_DEFINE_TYPE(FlutterDesktopScannerPlugin, flutter_desktop_scanner_plugin, g_object_get_type())

// Called when a method call is received from Flutter.
static void flutter_desktop_scanner_plugin_handle_method_call(
    FlutterDesktopScannerPlugin* self,
    FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;

  const gchar* method = fl_method_call_get_name(method_call);
  FlValue* args = fl_method_call_get_args(method_call);

  if (strcmp(method, "getPlatformVersion") == 0) {
    response = get_platform_version();
  } else if (strcmp(method, "getScanners") == 0) {
    response = get_scanners();
  } else if (strcmp(method, "initiateScan") == 0) {
    const char* scanner_name = fl_value_get_string(fl_value_lookup_string(args, "scannerName"));
    response = initiate_scan(scanner_name);
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

FlMethodResponse* get_platform_version() {
  struct utsname uname_data = {};
  uname(&uname_data);
  g_autofree gchar *version = g_strdup_printf("Linux %s", uname_data.version);
  g_autoptr(FlValue) result = fl_value_new_string(version);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

void get_scanners_thread() {
  const SANE_Device **device_list;

  SANE_Status st = sane_get_devices(&device_list, SANE_FALSE);

  // get devices
  if (st != SANE_STATUS_GOOD) {
    if (send_get_devices_events) {
      g_autoptr(GError) error = NULL;
      if (!fl_event_channel_send(get_devices_event_channel, fl_value_new_null(), NULL, &error)) {
        g_warning("Failed to send event: %s", error->message);
      }
    }
    return;
  }

  // Check if any devices are available
  if (device_list == nullptr) {
    if (send_get_devices_events) {
      g_autoptr(GError) error = NULL;
      if (!fl_event_channel_send(get_devices_event_channel, fl_value_new_null(), NULL, &error)) {
        g_warning("Failed to send event: %s", error->message);
      }
    }
    return;
  }

  g_autoptr(FlValue) result = fl_value_new_list();

  // compile a device list
  for (int i = 0; device_list[i] != nullptr; i++) {
    const SANE_Device *dev = device_list[i];
    g_autoptr(FlValue) entry = fl_value_new_map();
    fl_value_set(entry, fl_value_new_string("name"), fl_value_new_string(dev->name));
    fl_value_set(entry, fl_value_new_string("vendor"), fl_value_new_string(dev->vendor));
    fl_value_set(entry, fl_value_new_string("model"), fl_value_new_string(dev->model));
    fl_value_set(entry, fl_value_new_string("type"), fl_value_new_string(dev->type));
    fl_value_append(result, entry);
  }

  if (send_get_devices_events) {
    g_autoptr(GError) error = NULL;
    // According to flutter this is unsafe and event channel messages should be sent in the main thread. 
    // But we need to thread here so sane_get_devices does not block the UI thread.
    // @liquidiert maybe investigate a better approach
    if (!fl_event_channel_send(get_devices_event_channel, result, NULL, &error)) {
      g_warning("Failed to send event: %s", error->message);
    }
  }
}

FlMethodResponse* get_scanners() {
  
  std::thread get_devices_thread(get_scanners_thread);
  get_devices_thread.detach();

  g_autoptr(FlValue) result = fl_value_new_bool(TRUE);

  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error("Error during string formatting"); }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

static std::string write_pnm_header (SANE_Frame sane_format, int width, int height, int depth) {
  /* The netpbm-package does not define raw image data with maxval > 255. */
  /* But writing maxval 65535 for 16bit data gives at least a chance */
  /* to read the image. */
  switch (sane_format)
    {
    case SANE_FRAME_RED:
    case SANE_FRAME_GREEN:
    case SANE_FRAME_BLUE:
    case SANE_FRAME_RGB:
      return string_format("P6\n# SANE data follows\n%d %d\n%d\n", width, height, (depth <= 8) ? 255 : 65535);
    default:
      if (depth == 1)
        return string_format("P4\n# SANE data follows\n%d %d\n", width, height);
      else
        return string_format("P5\n# SANE data follows\n%d %d\n%d\n", width, height, (depth <= 8) ? 255 : 65535);
    }
}

void initiate_scan_thread(SANE_Handle handle) {

  // find resolution option
  const SANE_Option_Descriptor* descriptor = sane_get_option_descriptor(handle, 0);

  int nb_opts;
  SANE_Status st = sane_control_option(handle, 0, SANE_ACTION_GET_VALUE, &nb_opts, nullptr);
  if (st != SANE_STATUS_GOOD) {
    sane_close(handle);
    if (send_scan_events) {
      g_autoptr(GError) error = NULL;
      if (!fl_event_channel_send(scan_event_channel, fl_value_new_null(), NULL, &error)) {
        g_warning("Failed to send event: %s", error->message);
      }
    }
    return;
  }

  int resolution_indx = 0;

  for (int i = 0; i < nb_opts; i++) {
    if (strcmp(descriptor[i].name, "resolution")) {
      resolution_indx = i;
    }
  }

  int resolution = 300;
  if (resolution_indx != 0) {
    sane_control_option(handle, resolution_indx, SANE_ACTION_SET_VALUE, &resolution, nullptr);
  }

  SANE_Parameters parameters;
  if (sane_get_parameters(handle, &parameters) < 0) {
    sane_close(handle);
    if (send_scan_events) {
      g_autoptr(GError) error = NULL;
      if (!fl_event_channel_send(scan_event_channel, fl_value_new_null(), NULL, &error)) {
        g_warning("Failed to send event: %s", error->message);
      }
    }
    return;
  }

  if (sane_start(handle) < 0) {
      sane_close(handle);
      if (send_scan_events) {
      g_autoptr(GError) error = NULL;
      if (!fl_event_channel_send(scan_event_channel, fl_value_new_null(), NULL, &error)) {
        g_warning("Failed to send event: %s", error->message);
      }
    }
    return;
  }

  const int max_buffer_size = parameters.bytes_per_line * parameters.lines;
  uint8_t *image_data = new uint8_t[max_buffer_size];
  SANE_Word read_bytes;

  if (sane_read(handle, image_data, max_buffer_size, &read_bytes) < 0) {
    delete[] image_data;
    sane_cancel(handle);
    sane_close(handle);
    if (send_scan_events) {
      g_autoptr(GError) error = NULL;
      if (!fl_event_channel_send(scan_event_channel, fl_value_new_null(), NULL, &error)) {
        g_warning("Failed to send event: %s", error->message);
      }
    }
    return;
  };

  sane_close(handle);

  auto header_str = write_pnm_header(parameters.format, parameters.pixels_per_line, parameters.lines, parameters.depth);

  int header_len = header_str.size();
  int result_size = header_len + max_buffer_size;
  uint8_t *result_buffer = new uint8_t[result_size];

  memcpy(result_buffer, reinterpret_cast<const uint8_t*>(header_str.c_str()), header_len);
  memcpy(result_buffer + header_len, image_data, max_buffer_size);

  g_autoptr(FlValue) result = fl_value_new_uint8_list(result_buffer, result_size);

  if (send_scan_events) {
    g_autoptr(GError) error = NULL;
    if (!fl_event_channel_send(scan_event_channel, result, NULL, &error)) {
      g_warning("Failed to send event: %s", error->message);
    }
  }
}

FlMethodResponse* initiate_scan(const char* scanner_name) {
  SANE_Handle handle;
  // sane open has to be called on main thread, don't ask me why
  SANE_Status st = sane_open(scanner_name, &handle);

  if (st != SANE_STATUS_GOOD) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new("E001", "Failed to open device", nullptr));
  }

  std::thread scan_thread(initiate_scan_thread, handle);
  scan_thread.detach();

  g_autoptr(FlValue) result = fl_value_new_bool(TRUE);

  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

static void flutter_desktop_scanner_plugin_dispose(GObject* object) {
  // close sane connection when plugin gets disposed
  sane_exit();
  G_OBJECT_CLASS(flutter_desktop_scanner_plugin_parent_class)->dispose(object);
}

static void flutter_desktop_scanner_plugin_class_init(FlutterDesktopScannerPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = flutter_desktop_scanner_plugin_dispose;
}

static void flutter_desktop_scanner_plugin_init(FlutterDesktopScannerPlugin* self) {
  // initialize sane when plugin gets registered
  sane_init(nullptr, nullptr);
}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  FlutterDesktopScannerPlugin* plugin = FLUTTER_DESKTOP_SCANNER_PLUGIN(user_data);
  flutter_desktop_scanner_plugin_handle_method_call(plugin, method_call);
}

static FlMethodErrorResponse* on_listen_get_devices_callback(FlEventChannel* channel,
                                         FlValue *args,
                                         gpointer user_data) {
  send_get_devices_events = TRUE;
  return NULL;
}

// TODO: find a way to use this
/* static FlMethodErrorResponse* on_cancel_get_devices_callback(GObject *object,
                                         FlValue *args,
                                         gpointer user_data) {
  send_get_devices_events = FALSE;
  return NULL;
} */

static FlMethodErrorResponse* on_listen_scan_callback(FlEventChannel* channel,
                                         FlValue *args,
                                         gpointer user_data) {
  send_scan_events = TRUE;
  return NULL;
}

// TODO: find a way to use this
/* static FlMethodErrorResponse* on_cancel_scan_callback(GObject *object,
                                         FlValue *args,
                                         gpointer user_data) {
  send_scan_events = FALSE;
  return NULL;
} */

void flutter_desktop_scanner_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  FlutterDesktopScannerPlugin* plugin = FLUTTER_DESKTOP_SCANNER_PLUGIN(
      g_object_new(flutter_desktop_scanner_plugin_get_type(), nullptr));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "personalclientcare/flutter_desktop_scanner",
                            FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(channel, method_call_cb,
                                            g_object_ref(plugin),
                                            g_object_unref);

  get_devices_event_channel =
      fl_event_channel_new(
                        fl_plugin_registrar_get_messenger(registrar),
                        "personalclientcare/flutter_desktop_scanner_get_devices",
                        FL_METHOD_CODEC(codec)
                      );

  fl_event_channel_set_stream_handlers(get_devices_event_channel, on_listen_get_devices_callback, nullptr,
                                         g_object_ref(plugin), g_object_unref);

  scan_event_channel =
      fl_event_channel_new(
                        fl_plugin_registrar_get_messenger(registrar),
                        "personalclientcare/flutter_desktop_scanner_scan",
                        FL_METHOD_CODEC(codec)
                      );

  fl_event_channel_set_stream_handlers(scan_event_channel, on_listen_scan_callback, nullptr,
                                         g_object_ref(plugin), g_object_unref);

  g_object_unref(plugin);
}
