/*  Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/esp32-cyd-lvgl-display-bme280-data-table/   |   https://RandomNerdTutorials.com/esp32-tft-lvgl-display-bme280-data-table/
    THIS EXAMPLE WAS TESTED WITH THE FOLLOWING HARDWARE:
    1) ESP32-2432S028R 2.8 inch 240×320 also known as the Cheap Yellow Display (CYD): https://makeradvisor.com/tools/cyd-cheap-yellow-display-esp32-2432s028r/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/cyd-lvgl/
    2) REGULAR ESP32 Dev Board + 2.8 inch 240x320 TFT Display: https://makeradvisor.com/tools/2-8-inch-ili9341-tft-240x320/ and https://makeradvisor.com/tools/esp32-dev-board-wi-fi-bluetooth/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/esp32-tft-lvgl/
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

/*  Install the "lvgl" library version 9.X by kisvegabor to interface with the TFT Display - https://lvgl.io/
    *** IMPORTANT: lv_conf.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE lv_conf.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <lvgl.h>

/*  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI
    *** IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE User_Setup.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <TFT_eSPI.h>

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen - Note: this library doesn't require further configuration
#include <XPT2046_Touchscreen.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Specify the timezone you want to get the time for
const char* timezone = "Europe/Lisbon";

// Store date and time
String current_date;
String current_time;

// Install Adafruit Unified Sensor and Adafruit BME280 Library
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define I2C_SDA 27
#define I2C_SCL 22
#define SEALEVELPRESSURE_HPA (1013.25)
TwoWire I2CBME = TwoWire(0);
Adafruit_BME280 bme;

// SET VARIABLE TO 0 FOR TEMPERATURE IN FAHRENHEIT DEGREES
#define TEMP_CELSIUS 1    

#define LDR_PIN 34

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

// Get the Touchscreen data
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();

    // Advanced Touchscreen calibration, LEARN MORE » https://RandomNerdTutorials.com/touchscreen-calibration/
    float alpha_x, beta_x, alpha_y, beta_y, delta_x, delta_y;

    // REPLACE WITH YOUR OWN CALIBRATION VALUES » https://RandomNerdTutorials.com/touchscreen-calibration/
    alpha_x = -0.000;
    beta_x = 0.090;
    delta_x = -33.771;
    alpha_y = 0.066;
    beta_y = 0.000;
    delta_y = -14.632;

    x = alpha_y * p.x + beta_y * p.y + delta_y;
    // clamp x between 0 and SCREEN_WIDTH - 1
    x = max(0, x);
    x = min(SCREEN_WIDTH - 1, x);

    y = alpha_x * p.x + beta_x * p.y + delta_x;
    // clamp y between 0 and SCREEN_HEIGHT - 1
    y = max(0, y);
    y = min(SCREEN_HEIGHT - 1, y);

    // Basic Touchscreen calibration points with map function to the correct width and height
    //x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    //y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);

    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    // Set the coordinates
    data->point.x = x;
    data->point.y = y;

    // Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
    Serial.print("X = ");
    Serial.print(x);
    Serial.print(" | Y = ");
    Serial.print(y);
    Serial.print(" | Pressure = ");
    Serial.print(z);
    Serial.println();
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

static void float_button_event_cb(lv_event_t * e) {
  update_table_values();
}

static lv_obj_t * table;
static void update_table_values(void) {
  // Get the latest temperature reading in Celsius or Fahrenheit
  #if TEMP_CELSIUS
    float bme_temp = bme.readTemperature();
    const char degree_symbol[] = "\u00B0C";
  #else
    float bme_temp = 1.8 * bme.readTemperature() + 32;
    const char degree_symbol[] = "\u00B0F";
  #endif
  
  String bme_temp_value = String(bme_temp) + degree_symbol;
  String bme_humi_value = String(bme.readHumidity()) + "%";
  String bme_press_value = String(bme.readPressure() / 100.0F) + " hPa";
  String ldr_value = String(analogRead(LDR_PIN));
  
  // Get the time from WorldTimeAPI
  get_date_and_time();
  //Serial.println("Current Date: " + current_date);
  //Serial.println("Current Time: " + current_time);

  // Fill the first column
  lv_table_set_cell_value(table, 0, 0, "Data");
  lv_table_set_cell_value(table, 1, 0, "Temperature");
  lv_table_set_cell_value(table, 2, 0, "Humidity");
  lv_table_set_cell_value(table, 3, 0, "Pressure");
  lv_table_set_cell_value(table, 4, 0, "Luminosity");
  lv_table_set_cell_value(table, 5, 0, "Date");
  lv_table_set_cell_value(table, 6, 0, "Time");
  lv_table_set_cell_value(table, 7, 0, "IP Address");

  // Fill the second column
  lv_table_set_cell_value(table, 0, 1, "Value");
  lv_table_set_cell_value(table, 1, 1, bme_temp_value.c_str());
  lv_table_set_cell_value(table, 2, 1, bme_humi_value.c_str());
  lv_table_set_cell_value(table, 3, 1, bme_press_value.c_str());
  lv_table_set_cell_value(table, 4, 1, ldr_value.c_str());
  lv_table_set_cell_value(table, 5, 1, current_date.c_str());
  lv_table_set_cell_value(table, 6, 1, current_time.c_str());
  lv_table_set_cell_value(table, 7, 1, WiFi.localIP().toString().c_str());
}

static void draw_event_cb(lv_event_t * e) {
  lv_draw_task_t * draw_task = lv_event_get_draw_task(e);
  lv_draw_dsc_base_t * base_dsc = (lv_draw_dsc_base_t*) draw_task->draw_dsc;
  // If the cells are drawn
  if(base_dsc->part == LV_PART_ITEMS) {
    uint32_t row = base_dsc->id1;
    uint32_t col = base_dsc->id2;

    // Make the texts in the first cell center aligned
    if(row == 0) {
      lv_draw_label_dsc_t * label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
      if(label_draw_dsc) {
        label_draw_dsc->align = LV_TEXT_ALIGN_CENTER;
      }
      lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
      if(fill_draw_dsc) {
        fill_draw_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_BLUE), fill_draw_dsc->color, LV_OPA_20);
        fill_draw_dsc->opa = LV_OPA_COVER;
      }
    }
    // In the first column align the texts to the right
    else if(col == 0) {
      lv_draw_label_dsc_t * label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
      if(label_draw_dsc) {
        label_draw_dsc->align = LV_TEXT_ALIGN_RIGHT;
      }
    }

    // Make every 2nd row gray color
    if((row != 0 && row % 2) == 0) {
      lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
      if(fill_draw_dsc) {
        fill_draw_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_GREY), fill_draw_dsc->color, LV_OPA_10);
        fill_draw_dsc->opa = LV_OPA_COVER;
      }
    }
  }
}

void lv_create_main_gui(void) {
  table = lv_table_create(lv_screen_active());

  // Inserts or updates all table values
  update_table_values();

  // Set a smaller height to the table. It will make it scrollable
  lv_obj_set_height(table, 200);
  lv_obj_center(table);

  // Add an event callback to apply some custom drawing
  lv_obj_add_event_cb(table, draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
  lv_obj_add_flag(table, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

  // Create floating button
  lv_obj_t * float_button = lv_button_create(lv_screen_active());
  lv_obj_set_size(float_button, 50, 50);
  lv_obj_add_flag(float_button, LV_OBJ_FLAG_FLOATING);
  lv_obj_align(float_button, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
  lv_obj_add_event_cb(float_button, float_button_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_set_style_radius(float_button, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_image_src(float_button, LV_SYMBOL_REFRESH, 0);
  lv_obj_set_style_text_font(float_button, lv_theme_get_font_large(float_button), 0);
  lv_obj_set_style_bg_color(float_button, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
}

void get_date_and_time() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Construct the API endpoint
    String url = String("http://worldtimeapi.org/api/timezone/") + timezone;

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
          const char* datetime = doc["datetime"];
          //Serial.println("Datetime: " + String(datetime));
          
          // Split the datetime into date and time
          String datetimeStr = String(datetime);
          int splitIndex = datetimeStr.indexOf('T');
          current_date = datetimeStr.substring(0, splitIndex);
          current_time = datetimeStr.substring(splitIndex + 1, splitIndex + 9); // Extract time portion

        } else {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
        }
      }
    } else {
      Serial.printf("GET request failed, error: %s\n", http.errorToString(httpCode).c_str());
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

  // Set analog read resolution
  analogReadResolution(12);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConnected to Wi-Fi network with IP Address: ");
  Serial.println(WiFi.localIP());

  I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
  bool status;
  // Passing a &Wire2 to set custom I2C ports
  status = bme.begin(0x76, &I2CBME);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  
  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
  touchscreen.setRotation(2);

  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
  
  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  // Set the callback function to read Touchscreen input
  lv_indev_set_read_cb(indev, touchscreen_read);

  // Function to draw the GUI
  lv_create_main_gui();
}

void loop() {
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
}
