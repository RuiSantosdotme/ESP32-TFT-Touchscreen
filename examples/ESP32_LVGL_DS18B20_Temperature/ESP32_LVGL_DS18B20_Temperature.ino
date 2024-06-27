/*  Rui Santos & Sara Santos - Random Nerd Tutorials
    CYD Project Details: https://RandomNerdTutorials.com/esp32-cyd-lvgl-temperature-ds18b20/
    TFT Project Details: https://RandomNerdTutorials.com/esp32-lvgl-ds18b20-tempreature-tft-display/
    
    THIS EXAMPLE WAS TESTED WITH THE FOLLOWING HARDWARE:
    1) ESP32-2432S028R 2.8 inch 240×320 also known as the Cheap Yellow Display (CYD): https://makeradvisor.com/tools/cyd-cheap-yellow-display-esp32-2432s028r/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/cyd-lvgl/
    2) REGULAR ESP32 Dev Board + 2.8 inch 240x320 TFT Display: https://makeradvisor.com/tools/2-8-inch-ili9341-tft-240x320/ and https://makeradvisor.com/tools/esp32-dev-board-wi-fi-bluetooth/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/esp32-tft-lvgl/
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files. The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/
//  *** YOU MUST USE THE lv_conf.h AND User_Setup.h FILES PROVIDED IN THE NEXT LINKS IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/ 
//  Install the "lvgl" library version 9.X by kisvegabor to interface with the TFT Display - https://lvgl.io/ - IMPORTANT: lv_conf.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials
#include <lvgl.h>

//  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI - IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials
#include <TFT_eSPI.h>

// Install OneWire and DallasTemperature libraries
#include <OneWire.h>
#include <DallasTemperature.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = 27;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// SET VARIABLE TO 0 FOR TEMPERATURE IN FAHRENHEIT DEGREES
#define TEMP_CELSIUS 1    

#if TEMP_CELSIUS
  #define TEMP_ARC_MIN -20
  #define TEMP_ARC_MAX 40
#else
  #define TEMP_ARC_MIN -4
  #define TEMP_ARC_MAX 104
#endif

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

lv_obj_t * arc;

// Set the temperature value in the arc and text label
static void set_temp(void * text_label_temp_value, int32_t v) {
  sensors.requestTemperatures();
  // Get the latest temperature reading in Celsius or Fahrenheit
  #if TEMP_CELSIUS
    float ds18b20_temp = sensors.getTempCByIndex(0);
    if(ds18b20_temp <= 10.0) {
      lv_obj_set_style_text_color((lv_obj_t*) text_label_temp_value, lv_palette_main(LV_PALETTE_BLUE), 0);
    }
    else if (ds18b20_temp > 10.0 && ds18b20_temp <= 29.0) {
      lv_obj_set_style_text_color((lv_obj_t*) text_label_temp_value, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
    else {
      lv_obj_set_style_text_color((lv_obj_t*) text_label_temp_value, lv_palette_main(LV_PALETTE_RED), 0);
    }
    const char degree_symbol[] = "\u00B0C";
  #else
    float ds18b20_temp = 1.8 * sensors.getTempFByIndex(0) + 32;
    if(ds18b20_temp <= 50.0) {
      lv_obj_set_style_text_color((lv_obj_t*) text_label_temp_value, lv_palette_main(LV_PALETTE_BLUE), 0);
    }
    else if (ds18b20_temp > 50.0 && ds18b20_temp <= 84.2) {
      lv_obj_set_style_text_color((lv_obj_t*) text_label_temp_value, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
    else {
      lv_obj_set_style_text_color((lv_obj_t*) text_label_temp_value, lv_palette_main(LV_PALETTE_RED), 0);
    }
    const char degree_symbol[] = "\u00B0F";
  #endif

  lv_arc_set_value(arc, map(int(ds18b20_temp), TEMP_ARC_MIN, TEMP_ARC_MAX, 0, 100));

  String ds18b20_temp_text = String(ds18b20_temp) + degree_symbol;
  lv_label_set_text((lv_obj_t*) text_label_temp_value, ds18b20_temp_text.c_str());
  Serial.print("Temperature: ");
  Serial.println(ds18b20_temp_text);
}

void lv_create_main_gui(void) {
  // Create an Arc
  arc = lv_arc_create(lv_screen_active());
  lv_obj_set_size(arc, 210, 210);
  lv_arc_set_rotation(arc, 135);
  lv_arc_set_bg_angles(arc, 0, 270);
  lv_obj_set_style_arc_color(arc, lv_color_hex(0x666666), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(arc, lv_color_hex(0x333333), LV_PART_KNOB);
  lv_obj_align(arc, LV_ALIGN_CENTER, 0, 10);

  // Create a text label in font size 32 to display the latest temperature reading
  lv_obj_t * text_label_temp_value = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_temp_value, "--.--");
  lv_obj_set_style_text_align(text_label_temp_value, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(text_label_temp_value, LV_ALIGN_CENTER, 0, 10);
  static lv_style_t style_temp;
  lv_style_init(&style_temp);
  lv_style_set_text_font(&style_temp, &lv_font_montserrat_32);
  lv_obj_add_style(text_label_temp_value, &style_temp, 0);

  // Create an animation to update with the latest temperature value every 10 seconds
  lv_anim_t a_temp;
  lv_anim_init(&a_temp);
  lv_anim_set_exec_cb(&a_temp, set_temp);
  lv_anim_set_duration(&a_temp, 1000000);
  lv_anim_set_playback_duration(&a_temp, 1000000);
  lv_anim_set_var(&a_temp, text_label_temp_value);
  lv_anim_set_values(&a_temp, 0, 100);
  lv_anim_set_repeat_count(&a_temp, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a_temp);
}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  // Start the DS18B20 sensor
  sensors.begin();
  
  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  
 // Function to draw the GUI
  lv_create_main_gui();
}

void loop() {
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
}
