/*  Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/esp32-cyd-esp-now-control-multiple-boards/
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

// YOU MUST REPLACE WITH YOUR RECEIVER'S MAC Address
uint8_t board1Address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t board2Address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    int gpio;
    bool state;
} struct_message;

// Create a struct_message for each board
struct_message board1Data;
struct_message board2Data;

esp_now_peer_info_t peerInfo;

// User data structure for switches
typedef struct {
    int gpio;
    struct_message* data;
    uint8_t* targetAddress;
} SwitchUserData;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Function to send messages via ESP-NOW
void sendEspNowMessage(const uint8_t *boardMacAddress, const struct_message &message) {
  size_t messageSize = sizeof(message);
  
  esp_err_t result = esp_now_send(boardMacAddress, (const uint8_t *) &message, messageSize);
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } 
  else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
    Serial.println("Error: ESP-NOW not initialized");
  } 
  else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Error: Peer not found");
  } 
  else {
    Serial.printf("Error sending the data: %d\n", result);
  }
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

// Callback that is triggered when any toggle switch changes state
static void toggle_switch_event_handler(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    lv_obj_t * toggle_switch = (lv_obj_t*) lv_event_get_target(e);
    
    void* user_data = lv_obj_get_user_data(toggle_switch);
    if (user_data == NULL) return;
    
    SwitchUserData* sw_data = (SwitchUserData*)user_data;
    
    bool is_on = lv_obj_has_state(toggle_switch, LV_STATE_CHECKED);
    
    LV_LOG_USER("GPIO %d State: %s", sw_data->gpio, is_on ? "On" : "Off");
    
    sw_data->data->gpio = sw_data->gpio;
    sw_data->data->state = is_on;
    
    sendEspNowMessage(sw_data->targetAddress, *(sw_data->data));
  }
}

void lv_create_main_gui(void) {
  // Create a Tab view object
  lv_obj_t * tabview = lv_tabview_create(lv_screen_active());
  lv_tabview_set_tab_bar_size(tabview, 40);

  // Add 2 tabs
  lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "Board #1");
  lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "Board #2");

  // TAB 1 - Board #1 - Add 2 Toggle Switches
  lv_obj_t * switch_label1 = lv_label_create(tab1);
  lv_label_set_text(switch_label1, "GPIO 2");
  lv_obj_align(switch_label1, LV_ALIGN_CENTER, 0, -80);

  lv_obj_t * switch1 = lv_switch_create(tab1);
  lv_obj_align(switch1, LV_ALIGN_CENTER, 0, -40);
  
  static SwitchUserData switch1_data = {2, &board1Data, board1Address};
  lv_obj_set_user_data(switch1, &switch1_data);
  lv_obj_add_event_cb(switch1, toggle_switch_event_handler, LV_EVENT_ALL, NULL);

  lv_obj_t * switch_label2 = lv_label_create(tab1);
  lv_label_set_text(switch_label2, "GPIO 4");
  lv_obj_align(switch_label2, LV_ALIGN_CENTER, 0, 10);

  lv_obj_t * switch2 = lv_switch_create(tab1);
  lv_obj_align(switch2, LV_ALIGN_CENTER, 0, 50);
  
  static SwitchUserData switch2_data = {4, &board1Data, board1Address};
  lv_obj_set_user_data(switch2, &switch2_data);
  lv_obj_add_event_cb(switch2, toggle_switch_event_handler, LV_EVENT_ALL, NULL);

  // TAB 2 - Board #2 - Add 2 Toggle Switches
  lv_obj_t * switch_label3 = lv_label_create(tab2);
  lv_label_set_text(switch_label3, "GPIO 20");
  lv_obj_align(switch_label3, LV_ALIGN_CENTER, 0, -80);

  lv_obj_t * switch3 = lv_switch_create(tab2);
  lv_obj_align(switch3, LV_ALIGN_CENTER, 0, -40);
  
  static SwitchUserData switch3_data = {20, &board2Data, board2Address};
  lv_obj_set_user_data(switch3, &switch3_data);
  lv_obj_add_event_cb(switch3, toggle_switch_event_handler, LV_EVENT_ALL, NULL);

  lv_obj_t * switch_label4 = lv_label_create(tab2);
  lv_label_set_text(switch_label4, "GPIO 21");
  lv_obj_align(switch_label4, LV_ALIGN_CENTER, 0, 10);

  lv_obj_t * switch4 = lv_switch_create(tab2);
  lv_obj_align(switch4, LV_ALIGN_CENTER, 0, 50);
  
  static SwitchUserData switch4_data = {21, &board2Data, board2Address};
  lv_obj_set_user_data(switch4, &switch4_data);
  lv_obj_add_event_cb(switch4, toggle_switch_event_handler, LV_EVENT_ALL, NULL);
}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
    
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  // Register peers
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Register first peer  
  memcpy(peerInfo.peer_addr, board1Address, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register second peer  
  memcpy(peerInfo.peer_addr, board2Address, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
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
}

void loop() {
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
}
