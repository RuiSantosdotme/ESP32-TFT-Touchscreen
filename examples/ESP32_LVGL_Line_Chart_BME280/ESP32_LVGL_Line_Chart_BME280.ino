/*  Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/esp32-cyd-lvgl-line-chart/
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

// Install Adafruit Unified Sensor and Adafruit BME280 Library
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define I2C_SDA 27
#define I2C_SCL 22
TwoWire I2CBME = TwoWire(0);
Adafruit_BME280 bme;

// SET VARIABLE TO 0 FOR TEMPERATURE IN FAHRENHEIT DEGREES
#define TEMP_CELSIUS 1
#define BME_NUM_READINGS 20
float bme_last_readings[BME_NUM_READINGS] = {-20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0, -20.0};
float scale_min_temp;
float scale_max_temp;

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

// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0; // will store last time the chart was updated
// Interval at which the chart will be updated (milliseconds) 10000 milliseconds = 10 seconds
const long interval = 10000;

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

// Draw a label on that chart with the value of the pressed point
static void chart_draw_label_cb(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * chart = (lv_obj_t*) lv_event_get_target(e);

  if(code == LV_EVENT_VALUE_CHANGED) {
    lv_obj_invalidate(chart);
  }
  if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
    int32_t * s = (int32_t*)lv_event_get_param(e);
    *s = LV_MAX(*s, 20);
  }
  // Draw the label on the chart based on the pressed point
  else if(code == LV_EVENT_DRAW_POST_END) {
    int32_t id = lv_chart_get_pressed_point(chart);
    if(id == LV_CHART_POINT_NONE) return;

    LV_LOG_USER("Selected point %d", (int)id);

    lv_chart_series_t * ser = lv_chart_get_series_next(chart, NULL);
    while(ser) {
      lv_point_t p;
      lv_chart_get_point_pos_by_id(chart, ser, id, &p);

      int32_t * y_array = lv_chart_get_y_array(chart, ser);
      int32_t value = y_array[id];
      char buf[16];
      #if TEMP_CELSIUS
        const char degree_symbol[] = "\u00B0C";
      #else
        const char degree_symbol[] = "\u00B0F";
      #endif

      // Preparing the label text for the selected data point
      lv_snprintf(buf, sizeof(buf), LV_SYMBOL_DUMMY " %3.2f %s ", float(bme_last_readings[id]), degree_symbol);

      // Draw the rectangular label that will display the temperature value
      lv_draw_rect_dsc_t draw_rect_dsc;
      lv_draw_rect_dsc_init(&draw_rect_dsc);
      draw_rect_dsc.bg_color = lv_color_black();
      draw_rect_dsc.bg_opa = LV_OPA_60;
      draw_rect_dsc.radius = 2;
      draw_rect_dsc.bg_image_src = buf;
      draw_rect_dsc.bg_image_recolor = lv_color_white();
      // Rectangular label size
      lv_area_t a;
      a.x1 = chart->coords.x1 + p.x - 35;
      a.x2 = chart->coords.x1 + p.x + 35;
      a.y1 = chart->coords.y1 + p.y - 30;
      a.y2 = chart->coords.y1 + p.y - 10;
      lv_layer_t * layer = lv_event_get_layer(e);
      lv_draw_rect(layer, &draw_rect_dsc, &a);
      ser = lv_chart_get_series_next(chart, ser);
    }
  }
  else if(code == LV_EVENT_RELEASED) {
    lv_obj_invalidate(chart);
  }
}

// Draw chart
void lv_draw_chart(void) {
  // Clear screen
  lv_obj_clean(lv_scr_act());

  // Create a a text label aligned on top
  lv_obj_t * label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "BME280 Temperature Readings");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

  // Create a container to display the chart and scale
  lv_obj_t * container_row = lv_obj_create(lv_screen_active());
  lv_obj_set_size(container_row, SCREEN_HEIGHT-20,  SCREEN_WIDTH-40);
  lv_obj_align(container_row, LV_ALIGN_BOTTOM_MID, 0, -10);
  // Set the container in a flexbox row layout aligned center
  lv_obj_set_flex_flow(container_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(container_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Create a chart
  lv_obj_t * chart = lv_chart_create(container_row);
  lv_obj_set_size(chart, SCREEN_HEIGHT-90, SCREEN_WIDTH-70);
  lv_chart_set_point_count(chart, BME_NUM_READINGS);
  lv_obj_add_event_cb(chart, chart_draw_label_cb, LV_EVENT_ALL, NULL);
  lv_obj_refresh_ext_draw_size(chart);

  // Add a data series
  lv_chart_series_t * chart_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);

  for(int i = 0; i < BME_NUM_READINGS; i++) {
    if(float(bme_last_readings[i]) != -20.0) { // Ignores default array values
      // Set points in the chart and scale them with an *100 multiplier to remove the 2 floating-point numbers
      chart_series->y_points[i] = float(bme_last_readings[i]) * 100;
    }
  }
  // Set the chart range and also scale it with an *100 multiplier to remove the 2 floating-point numbers
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, int(scale_min_temp-1)*100, int(scale_max_temp+1)*100);
  lv_chart_refresh(chart); // Required to update the chart with the new values

  // Create a scale (y axis for the temperature) aligned vertically on the right
  lv_obj_t * scale = lv_scale_create(container_row);
  lv_obj_set_size(scale, 15, SCREEN_WIDTH-90);
  lv_scale_set_mode(scale, LV_SCALE_MODE_VERTICAL_RIGHT);
  lv_scale_set_label_show(scale, true);
  // Set the scale ticks count 
  lv_scale_set_total_tick_count(scale, int(scale_max_temp+2) - int(scale_min_temp-1));
  if((int(scale_max_temp+2) - int(scale_min_temp-1)) < 10) {
    lv_scale_set_major_tick_every(scale, 1); // set y axis to have 1 tick every 1 degree
  }
  else {
    lv_scale_set_major_tick_every(scale, 10); // set y axis to have 1 tick every 10 degrees
  }
  // Set the scale style and range
  lv_obj_set_style_length(scale, 5, LV_PART_ITEMS);
  lv_obj_set_style_length(scale, 10, LV_PART_INDICATOR);
  lv_scale_set_range(scale, int(scale_min_temp-1), int(scale_max_temp+1));
}

// Get the latest BME readings
void get_bme_readings(void) {
  #if TEMP_CELSIUS
    float bme_temp = bme.readTemperature();
  #else
    float bme_temp = 1.8 * bme.readTemperature() + 32;  
  #endif
  
  // Reset scale range (chart y axis) variables
  scale_min_temp = 120.0;
  scale_max_temp = -20.0;

  // Shift values to the left of the array and inserts the latest reading at the end
  for (int i = 0; i < BME_NUM_READINGS; i++) {
    if(i == (BME_NUM_READINGS-1) && float(bme_temp) < 120.0) {
      bme_last_readings[i] = float(bme_temp);  // Inserts the new reading at the end
    }
    else {
      bme_last_readings[i] = float(bme_last_readings[i + 1]);  // Shift values to the left of the array
    }
    // Get the min/max value in the array to set the scale range (chart y axis)
    if((float(bme_last_readings[i]) < scale_min_temp) && (float(bme_last_readings[i]) != -20.0 )) {
      scale_min_temp = bme_last_readings[i];
    }
    if((float(bme_last_readings[i]) > scale_max_temp) && (float(bme_last_readings[i]) != -20.0 )) {
      scale_max_temp = bme_last_readings[i];
    }
  }
  Serial.print("Min temp: ");
  Serial.println(float(scale_min_temp));
  Serial.print("Max temp: ");
  Serial.println(float(scale_max_temp));
  Serial.print("BME last reading: ");
  Serial.println(float(bme_last_readings[BME_NUM_READINGS-1]));
  lv_draw_chart();
}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

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

  get_bme_readings();
}

void loop() {
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time that chart was updated
    previousMillis = currentMillis;
    get_bme_readings();    
  }
}
