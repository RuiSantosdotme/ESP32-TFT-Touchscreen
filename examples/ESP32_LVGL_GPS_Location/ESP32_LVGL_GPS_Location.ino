/*  Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/esp32-cyd-lvgl-gps-location/
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

#include <TinyGPS++.h>

#include "gps_image.h"

// Define the RX and TX pins for Serial 2
#define RXD2 22
#define TXD2 27

#define GPS_BAUD 9600

// The TinyGPS++ object
TinyGPSPlus gps;

// Create an instance of the HardwareSerial class for Serial 2
HardwareSerial gpsSerial(2);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

String current_date;
String utc_time;
String latitude;
String longitude;
String altitude;
String speed;
String hdop;
String satellites;

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

String format_time(int time) {
  return (time < 10) ? "0" + String(time) : String(time);
}

static lv_obj_t * text_label_date;
static lv_obj_t * text_label_latitude;
static lv_obj_t * text_label_longitude;
static lv_obj_t * text_label_altitude;
static lv_obj_t * text_label_speed;
static lv_obj_t * text_label_utc_time;
static lv_obj_t * text_label_hdop_satellites;
static lv_obj_t * text_label_gps_data;

static void timer_cb(lv_timer_t * timer){
  LV_UNUSED(timer);
  lv_label_set_text(text_label_date, current_date.c_str());
  lv_label_set_text(text_label_hdop_satellites, String("HDOP " + hdop + "  SAT. " + satellites).c_str());
  lv_label_set_text(text_label_gps_data, String("LAT   " + latitude + "\nLON   " + longitude + "\nALT   " 
                                                + altitude + "m" + "\nSPEED   " + speed + "km/h").c_str());
  lv_label_set_text(text_label_utc_time, String("UTC TIME - " + utc_time).c_str());
}

void lv_create_main_gui(void) {
  LV_IMAGE_DECLARE(image_gpsmap);

  lv_obj_t * img_gpsmap = lv_image_create(lv_screen_active());
  lv_obj_align(img_gpsmap, LV_ALIGN_LEFT_MID, 10, -20);
  lv_image_set_src(img_gpsmap, &image_gpsmap);

  text_label_hdop_satellites = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_hdop_satellites, String("HDOP " + hdop + "  SAT. " + satellites).c_str());
  lv_obj_align(text_label_hdop_satellites, LV_ALIGN_BOTTOM_LEFT, 30, -50);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_hdop_satellites, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color((lv_obj_t*) text_label_hdop_satellites, lv_palette_main(LV_PALETTE_GREY), 0);

  text_label_date = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_date, current_date.c_str());
  lv_obj_align(text_label_date, LV_ALIGN_CENTER, 60, -80);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_date, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color((lv_obj_t*) text_label_date, lv_palette_main(LV_PALETTE_TEAL), 0);

  lv_obj_t * text_label_location = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_location, "LOCATION");
  lv_obj_align(text_label_location, LV_ALIGN_CENTER, 50, -40);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_location, &lv_font_montserrat_20, 0);
  
  text_label_gps_data = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_gps_data, String("LAT   " + latitude + "\nLON   " + longitude + "\nALT   " 
                                                 + altitude + "m" + "\nSPEED   " + speed + "km/h").c_str());
  lv_obj_align(text_label_gps_data, LV_ALIGN_CENTER, 60, 20);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_gps_data, &lv_font_montserrat_14, 0);

  text_label_utc_time = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_utc_time, String("UTC TIME - " + utc_time).c_str());
  lv_obj_align(text_label_utc_time, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_utc_time, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color((lv_obj_t*) text_label_utc_time, lv_palette_main(LV_PALETTE_TEAL), 0);
 
  lv_timer_t * timer = lv_timer_create(timer_cb, 1, NULL);
  lv_timer_ready(timer);
}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  // Start Serial 2 with the defined RX and TX pins and a baud rate of 9600
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial 2 started at 9600 baud rate");

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
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
  
  // This sketch displays information every time a new sentence is correctly encoded.
  unsigned long start = millis();

  while (millis() - start < 1000) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
    if (gps.location.isUpdated()) {
      Serial.print("LAT: ");
      latitude = String(gps.location.lat(), 6);
      Serial.println(latitude);
      Serial.print("LONG: "); 
      longitude = String(gps.location.lng(), 6);
      Serial.println(longitude);
      Serial.print("SPEED (km/h) = "); 
      speed = String(gps.speed.kmph(), 2);
      Serial.println(speed);
      Serial.print("ALT (min)= "); 
      altitude = String(gps.altitude.meters(), 2);
      Serial.println(altitude);
      Serial.print("HDOP = "); 
      hdop = String(gps.hdop.value() / 100.0, 2);
      Serial.println(hdop);
      Serial.print("Satellites = "); 
      satellites = String(gps.satellites.value());
      Serial.println(satellites);
      Serial.print("Time in UTC: ");
      Serial.println(String(gps.date.year()) + "-" + String(gps.date.month()) + "-" + String(gps.date.day()) + "," + String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()));
      current_date = String(gps.date.year()) + "-" + String(gps.date.month()) + "-" + String(gps.date.day());
      Serial.println(current_date);
      utc_time = String(format_time(gps.time.hour())) + ":" + String(format_time(gps.time.minute())) + ":" + String(format_time(gps.time.second()));
      Serial.println(utc_time);
      Serial.println("");
    }
  }
}