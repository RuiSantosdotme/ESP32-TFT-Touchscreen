/*  Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/esp32-cyd-lvgl-digital-clock/  |  https://RandomNerdTutorials.com/esp32-tft-lvgl-digital-clock/
    THIS EXAMPLE WAS TESTED WITH THE FOLLOWING HARDWARE:
    1) ESP32-2432S028R 2.8 inch 240×320 also known as the Cheap Yellow Display (CYD): https://makeradvisor.com/tools/cyd-cheap-yellow-display-esp32-2432s028r/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/cyd-lvgl/
    2) REGULAR ESP32 Dev Board + 2.8 inch 240x320 TFT Display: https://makeradvisor.com/tools/2-8-inch-ili9341-tft-240x320/ and https://makeradvisor.com/tools/esp32-dev-board-wi-fi-bluetooth/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/esp32-tft-lvgl/
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

/*  Install the "lvgl" library version 9.4 by kisvegabor to interface with the TFT Display - https://lvgl.io/
    *** IMPORTANT: lv_conf.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE lv_conf.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <lvgl.h>

/*  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI
    *** IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE User_Setup.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <TFT_eSPI.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Specify the timezone you want to get the time for: https://timeapi.io/api/TimeZone/AvailableTimeZones
// Timezone example for Portugal: "Europe/Lisbon"
const char* timezone = "Europe/Lisbon";

// Store date and time
String current_date;
String current_time;

// Store hour, minute, second
static int32_t hour;
static int32_t minute;
static int32_t second;
bool sync_time_date = false;

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

String format_time(int time) {
  return (time < 10) ? "0" + String(time) : String(time);
}

static lv_obj_t * text_label_time;
static lv_obj_t * text_label_date;

static void timer_cb(lv_timer_t * timer){
  LV_UNUSED(timer);
  second++;
  if(second > 59) {
    second = 0;
    minute++;
    if(minute > 59) {
      minute = 0;
      hour++;
      sync_time_date = true;
      Serial.println(sync_time_date);
      Serial.println("\n\n\n\n\n\n\n\n");
      if(hour > 23) {
        hour = 0;
      }
    }
  }

  String hour_time_f = format_time(hour);
  String minute_time_f = format_time(minute);
  String second_time_f = format_time(second);

  String final_time_str = String(hour_time_f) + ":" + String(minute_time_f) + ":"  + String(second_time_f);
  //Serial.println(final_time_str);
  lv_label_set_text(text_label_time, final_time_str.c_str());
  lv_label_set_text(text_label_date, current_date.c_str());
}

void lv_create_main_gui(void) {
  // Get the time and date from TimeAPI.io
  while(hour==0 && minute==0 && second==0) {
    get_date_and_time();
  }
  Serial.println("Current Time: " + current_time);
  Serial.println("Current Date: " + current_date);

  lv_timer_t * timer = lv_timer_create(timer_cb, 1000, NULL);
  lv_timer_ready(timer);

  // Create a text label for the time aligned center
  text_label_time = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_time, "");
  lv_obj_align(text_label_time, LV_ALIGN_CENTER, 0, -30);
  // Set font type and size
  static lv_style_t style_text_label;
  lv_style_init(&style_text_label);
  lv_style_set_text_font(&style_text_label, &lv_font_montserrat_48);
  lv_obj_add_style(text_label_time, &style_text_label, 0);

  // Create a text label for the date aligned center
  text_label_date = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_date, current_date.c_str());
  lv_obj_align(text_label_date, LV_ALIGN_CENTER, 0, 40);
  // Set font type and size
  static lv_style_t style_text_label2;
  lv_style_init(&style_text_label2);
  lv_style_set_text_font(&style_text_label2, &lv_font_montserrat_30);
  lv_obj_add_style(text_label_date, &style_text_label2, 0); 
  lv_obj_set_style_text_color((lv_obj_t*) text_label_date, lv_palette_main(LV_PALETTE_GREY), 0);
}

void get_date_and_time() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Construct the API endpoint
    String url = String("https://timeapi.io/api/Time/current/zone?timeZone=") + timezone;
    http.begin(url);
    int httpCode = http.GET(); // Make the GET request

    if (httpCode > 0) {
      // Check for the response
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        //Serial.println("Time information:");
        //Serial.println(payload);
        // Parse the JSON to extract the time
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          current_date = String(doc["date"]);
          current_time = String(doc["time"]);
          hour = doc["hour"];
          minute = doc["minute"];
          second = doc["seconds"];
        } else {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
        }
      }
    } else {
      Serial.printf("GET request failed, error: %s\n", http.errorToString(httpCode).c_str());
      sync_time_date = true;
    }
    http.end(); // Close connection
  } else {
    Serial.println("Not connected to Wi-Fi");
  }
}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConnected to Wi-Fi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
  
  // Function to draw the GUI
  lv_create_main_gui();
}

void loop() {
  // Get the time and date from TimeAPI.io
  if(sync_time_date) {
    sync_time_date = false;
    get_date_and_time();
    while(hour==0 && minute==0 && second==0) {
      get_date_and_time();
    }
  }
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
}
