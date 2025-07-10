/*  Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/esp32-cyd-esp-now-receive-data/
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

#include <esp_now.h>
#include <WiFi.h>
#include <freertos/queue.h>

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

// SET VARIABLE TO 0 FOR THE TEMPERATURE DEGREES SYMBOL IN FAHRENHEIT
#define TEMP_CELSIUS 1  

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// Define a queue handle
QueueHandle_t esp_now_queue;

// Structure to receive data, must match the sender structure
typedef struct {
    int id;
    float temp;
    float hum;
    int readingId;
} struct_message;

static lv_obj_t * table1;
static lv_obj_t * table2;

// Callback function executed when data is received
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  // Copy incoming data to myData structure
  struct_message myData;
  memcpy(&myData, incomingData, sizeof(myData));
  xQueueSendFromISR(esp_now_queue, &myData, NULL); // Send to queue from ISR

  // Get sender's MAC address from recv_info
  const uint8_t *mac_addr = recv_info->src_addr;
  
  // Convert sender's MAC address to a string for display
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  
  // Print received data to Serial Monitor
  Serial.println("Received data:");
  Serial.print("  Sender MAC: ");
  Serial.println(macStr);
  Serial.print("  Board ID: ");
  Serial.println(myData.id);
  Serial.print("  Temperature: ");
  Serial.print(myData.temp);
  Serial.println(" °C");
  Serial.print("  Humidity: ");
  Serial.print(myData.hum);
  Serial.println(" %");
  Serial.print("  Reading ID: ");
  Serial.println(myData.readingId);
  Serial.println("-------------------");
}

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

void lv_create_main_gui(void) {
  // Create a Tab view object
  lv_obj_t * tabview;
  tabview = lv_tabview_create(lv_screen_active());
  lv_tabview_set_tab_bar_size(tabview, 40);
  
  // Add 3 tabs (the tabs are page (lv_page) and can be scrolled
  lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "BOARD #1");
  lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "BOARD #2");

  table1 = lv_table_create(tab1);
  lv_table_set_cell_value(table1, 0, 0, "Temperature");
  lv_table_set_cell_value(table1, 1, 0, "Humidity");
  lv_table_set_cell_value(table1, 2, 0, "Reading ID");
  lv_table_set_cell_value(table1, 0, 1, "--");
  lv_table_set_cell_value(table1, 1, 1, "--");
  lv_table_set_cell_value(table1, 2, 1, "--");

  table2 = lv_table_create(tab2);
  lv_table_set_cell_value(table2, 0, 0, "Temperature");
  lv_table_set_cell_value(table2, 1, 0, "Humidity");
  lv_table_set_cell_value(table2, 2, 0, "Reading ID");
  lv_table_set_cell_value(table2, 0, 1, "--");
  lv_table_set_cell_value(table2, 1, 1, "--");
  lv_table_set_cell_value(table2, 2, 1, "--");

  // Center the tables
  lv_obj_center(table1);
  lv_obj_center(table2);
}

void update_table_values(struct_message *myData) {
  #if TEMP_CELSIUS
    const char degree_symbol[] = "\u00B0C";
  #else
    const char degree_symbol[] = "\u00B0F";
  #endif

  String temp_value = String(myData->temp) + degree_symbol;
  String humi_value = String(myData->hum) + "%";

  if(myData->id == 1) {
    lv_table_set_cell_value(table1, 0, 1, temp_value.c_str());
    lv_table_set_cell_value(table1, 1, 1, humi_value.c_str());
    lv_table_set_cell_value(table1, 2, 1, String(myData->readingId).c_str());
  }
  else if(myData->id == 2) {
    lv_table_set_cell_value(table2, 0, 1, temp_value.c_str());
    lv_table_set_cell_value(table2, 1, 1, humi_value.c_str());
    lv_table_set_cell_value(table2, 2, 1, String(myData->readingId).c_str());
  }
}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
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

  // Register callback function to handle received data
  esp_now_register_recv_cb(OnDataRecv);
  
  esp_now_queue = xQueueCreate(10, sizeof(struct_message)); // Queue for 10 messages
  if (esp_now_queue == NULL) {
    Serial.println("Error creating queue");
    return;
  }
}

void loop() {
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
  
  struct_message myData;
  if (xQueueReceive(esp_now_queue, &myData, 0) == pdTRUE) {
    // Process and display the data
    update_table_values(&myData);
  }
}
