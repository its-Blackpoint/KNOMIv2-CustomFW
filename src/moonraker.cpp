#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "moonraker.h"
#include "knomi.h"

// #define MOONRAKER_DEBUG

void lv_popup_warning(const char * warning, bool clickable);

String MOONRAKER::send_request(const char * type, String path) {
    String ip = knomi_config.moonraker_ip;
    String port = knomi_config.moonraker_port;
    String url = "http://" + ip + ":" + port + path;
    String response = "";
    HTTPClient client;
    // replace all " " space to "%20" for http
    url.replace(" ", "%20");
    client.begin(url);
    // set timeout to 60 seconds since some gcode like G28 need long time to feedback
    client.setTimeout(60000);
    int code = client.sendRequest(type, "");
    // http request success
    if (code > 0) {
        unconnected = false;
        response = client.getString();
        if (code == 400) {
            if (!response.isEmpty()) {
                // Serial.println(response.c_str());
                DynamicJsonDocument json_parse(response.length() * 2);
                deserializeJson(json_parse, response);
                String msg = json_parse["error"]["message"].as<String>();
#ifdef MOONRAKER_DEBUG
                Serial.println(msg.c_str());
#endif
                msg.remove(0, 41); //  remove header {'error': 'WebRequestError', 'message':
                msg.remove(msg.length() - 2, 2); // remove tail }
                msg.replace("\\n", "\n");
                lv_popup_warning(msg.c_str(), true);
            }
        }
    } else {
        /*
         * since some gcode need long time cause code=-11 error
         * so don't set status when POST gcode
         * only set when GET
         */
        if (strcmp(type, "GET") == 0)
            unconnected = true;
        Serial.printf("moonraker http %s error.\r\n", type);
    }
    client.end(); //Free the resources

#ifdef MOONRAKER_DEBUG
    Serial.printf("\r\n\r\n %s code:%d************ %s *******************\r\n\r\n", type, code, url.c_str());
    Serial.println(response.c_str());
    Serial.println("\r\n*******************************\r\n\r\n");
#endif

    return response;
}

void MOONRAKER::http_post_loop(void) {
    if (post_queue.count == 0) return;
    send_request("POST", post_queue.queue[post_queue.index_r]);
    post_queue.count--;
    post_queue.index_r = (post_queue.index_r + 1) % QUEUE_LEN;
}

bool MOONRAKER::post_to_queue(String path) {
    if (post_queue.count >= QUEUE_LEN) {
        Serial.println("moonraker post queue overflow!");
        return false;
    }
    post_queue.queue[post_queue.index_w] = path;
    post_queue.index_w = (post_queue.index_w + 1) % QUEUE_LEN;
    post_queue.count++;
#ifdef MOONRAKER_DEBUG
    Serial.printf("\r\n\r\n ************ post queue *******************\r\n\r\n");
    Serial.print("count: ");   Serial.println(post_queue.count);
    Serial.print("index_w: "); Serial.println(post_queue.index_w);
    Serial.print("queue: ");   Serial.println(path);
    Serial.println("\r\n*******************************\r\n\r\n");
#endif
    return true;
}

bool MOONRAKER::post_gcode_to_queue(String gcode) {
    String path = "/printer/gcode/script?script=" + gcode;
    return post_to_queue(path);
}

void MOONRAKER::get_printer_ready(void) {
    String webhooks = send_request("GET", "/printer/objects/query?webhooks");
    if (!webhooks.isEmpty()) {
        DynamicJsonDocument json_parse(webhooks.length() * 2);
        deserializeJson(json_parse, webhooks);
        String state = json_parse["result"]["status"]["webhooks"]["state"].as<String>();
        unready = (state == "ready") ? false : true;
#ifdef MOONRAKER_DEBUG
        Serial.print("unready: ");
        Serial.println(unready);
#endif
    } else {
        unready = true;
        Serial.println("Empty: moonraker: get_printer_ready");
    }
}

void MOONRAKER::get_printer_info(void) {
    String printer_info = send_request("GET", "/api/printer");
    if (!printer_info.isEmpty()) {
        DynamicJsonDocument json_parse(printer_info.length() * 2);
        deserializeJson(json_parse, printer_info);
        data.pause = json_parse["state"]["flags"]["pausing"].as<bool>(); // pausing
        data.pause |= json_parse["state"]["flags"]["paused"].as<bool>(); // paused
        data.printing = json_parse["state"]["flags"]["printing"].as<bool>(); // printing
        data.printing |= json_parse["state"]["flags"]["cancelling"].as<bool>(); // cancelling
        data.printing |= data.pause; // pause
        data.bed_actual = int16_t(json_parse["temperature"]["bed"]["actual"].as<double>() + 0.5f);
        data.bed_target = int16_t(json_parse["temperature"]["bed"]["target"].as<double>() + 0.5f);
        data.nozzle_actual = int16_t(json_parse["temperature"][knomi_config.moonraker_tool]["actual"].as<double>() + 0.5f);
        data.nozzle_target = int16_t(json_parse["temperature"][knomi_config.moonraker_tool]["target"].as<double>() + 0.5f);
#ifdef MOONRAKER_DEBUG
        Serial.print("unoperational: ");
        Serial.println(unoperational);
        Serial.print("printing: ");
        Serial.println(data.printing);
        Serial.print("bed_actual: ");
        Serial.println(data.bed_actual);
        Serial.print("bed_target: ");
        Serial.println(data.bed_target);
        Serial.print("nozzle_actual: ");
        Serial.println(data.nozzle_actual);
        Serial.print("nozzle_target: ");
        Serial.println(data.nozzle_target);
#endif
    } else {
        Serial.println("Empty: moonraker: get_printer_info");
    }
}

// only return gcode file name except path
// for example:"SD:/test/123.gcode"
// only return "123.gcode"
const char * path_only_gcode(const char * path)
{
  char * name = strrchr(path, '/');

  if (name != NULL)
    return (name + 1);
  else
    return path;
}

// percent-encode a file path for a URL query (keeps '/')
static String url_encode_path(const String &s) {
    const char * hex = "0123456789ABCDEF";
    String out;
    out.reserve(s.length() * 3);
    for (size_t i = 0; i < s.length(); i++) {
        uint8_t c = (uint8_t)s[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
            out += (char)c;
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 0x0F];
        }
    }
    return out;
}

// GET /server/files/metadata?filename=<path relative to gcodes root>
// returns gcode_start_byte / gcode_end_byte (Moonraker file manager API)
bool MOONRAKER::get_gcode_metadata(const String &filename) {
    String response = send_request("GET", "/server/files/metadata?filename=" + url_encode_path(filename));
    if (response.isEmpty()) return false;

    StaticJsonDocument<96> filter;
    filter["result"]["gcode_start_byte"] = true;
    filter["result"]["gcode_end_byte"] = true;
    StaticJsonDocument<256> json_parse;
    if (deserializeJson(json_parse, response, DeserializationOption::Filter(filter))) return false;

    JsonVariant start = json_parse["result"]["gcode_start_byte"];
    JsonVariant end = json_parse["result"]["gcode_end_byte"];
    if (!start.is<uint32_t>() || !end.is<uint32_t>()) return false;

    gcode_start_byte = start.as<uint32_t>();
    gcode_end_byte = end.as<uint32_t>();
    return gcode_end_byte > gcode_start_byte;
}

void MOONRAKER::get_progress(void) {
    String display_status = send_request("GET", "/printer/objects/query?virtual_sdcard&print_stats=filename");
    if (!display_status.isEmpty()) {
        DynamicJsonDocument json_parse(display_status.length() * 2);
        deserializeJson(json_parse, display_status);
        JsonVariant vsd = json_parse["result"]["status"]["virtual_sdcard"];
        double progress = vsd["progress"].as<double>(); // fallback: absolute file position / file size
        uint32_t file_position = vsd["file_position"].as<uint32_t>();
        String filename = json_parse["result"]["status"]["print_stats"]["filename"].as<String>();

        // new file: fetch gcode byte range once (retry every 10s if it failed)
        if (filename != meta_filename) {
            meta_filename = filename;
            meta_valid = false;
            meta_retry_ms = 0;
        }
        if (!meta_valid && !meta_filename.isEmpty() && (int32_t)(millis() - meta_retry_ms) >= 0) {
            meta_valid = get_gcode_metadata(meta_filename);
            meta_retry_ms = millis() + 10000;
        }

        // same calculation as Mainsail "file-relative"
        if (meta_valid) {
            if (file_position <= gcode_start_byte) {
                progress = 0.0;
            } else if (file_position >= gcode_end_byte) {
                progress = 1.0;
            } else {
                progress = (double)(file_position - gcode_start_byte) / (double)(gcode_end_byte - gcode_start_byte);
            }
        }
        // round down like Mainsail (Math.floor)
        int32_t percent = (int32_t)(progress * 100.0);
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        data.progress = (uint8_t)percent;

        String path = vsd["file_path"].as<String>();
        strlcpy(data.file_path, path_only_gcode(path.c_str()), sizeof(data.file_path) - 1);
        data.file_path[sizeof(data.file_path) - 1] = 0;
#ifdef MOONRAKER_DEBUG
        Serial.print("progress: ");
        Serial.println(data.progress);
        Serial.print("path: ");
        Serial.println(data.file_path);
#endif
    } else {
        Serial.println("Empty: moonraker: get_progress");
    }
}

void MOONRAKER::get_knomi_status(void) {
    String knomi_status = send_request("GET", "/printer/objects/query?gcode_macro%20_KNOMI_STATUS");
    if (!knomi_status.isEmpty()) {
        DynamicJsonDocument json_parse(knomi_status.length() * 2);
        deserializeJson(json_parse, knomi_status);
        data.homing = json_parse["result"]["status"]["gcode_macro _KNOMI_STATUS"]["homing"].as<bool>();
        data.probing = json_parse["result"]["status"]["gcode_macro _KNOMI_STATUS"]["probing"].as<bool>();
        data.qgling = json_parse["result"]["status"]["gcode_macro _KNOMI_STATUS"]["qgling"].as<bool>();
        data.heating_nozzle = json_parse["result"]["status"]["gcode_macro _KNOMI_STATUS"]["heating_nozzle"].as<bool>();
        data.heating_bed = json_parse["result"]["status"]["gcode_macro _KNOMI_STATUS"]["heating_bed"].as<bool>();
#ifdef MOONRAKER_DEBUG
        Serial.print("homing: ");
        Serial.println(data.homing);
        Serial.print("probing: ");
        Serial.println(data.probing);
        Serial.print("qgling: ");
        Serial.println(data.qgling);
        Serial.print("heating_nozzle: ");
        Serial.println(data.heating_nozzle);
        Serial.print("heating_bed: ");
        Serial.println(data.heating_bed);
#endif
    } else {
        Serial.println("Empty: moonraker: get_knomi_status");
    }
}

void MOONRAKER::http_get_loop(void) {
    data_unlock = false;
    get_printer_ready();
    if (!unready) {
        // get_knomi_status() must before get_printer_info()
        // avoid homing, qgling, etc action flag = 1
        // but printing flag has not refresh
        get_knomi_status();
        get_printer_info();
        if (data.printing) {
            get_progress();
        }
    }
    data_unlock = true;
}

MOONRAKER moonraker;

void moonraker_post_task(void * parameter) {
    for(;;) {
        moonraker.http_post_loop();
        delay(500);
    }
}

void moonraker_task(void * parameter) {

    xTaskCreate(moonraker_post_task, "moonraker post",
        4096,  // Stack size (bytes)
        NULL,  // Parameter to pass
        8,     // Task priority
        NULL   // Task handle
        );

    for(;;) {
        if (wifi_get_connect_status() == WIFI_STATUS_CONNECTED) {
            moonraker.http_get_loop();
        }
        delay(200);
    }
}

// Klipper Control: Restart Firmware Restart. /printer/restart, /printer/firmware_restart
// Service Control: stop start restart. POST /machine/services/stop|restart|start?service={name}
// Host Control: Reboot, Shutdown. POST /machine/shutdown, POST /machine/reboot
