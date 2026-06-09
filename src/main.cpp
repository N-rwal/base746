#include "lvgl.h"
#include "lvglDrivers.h"
#include <Arduino.h>


#define TX_PIN PC6
#define RX_PIN PC7

typedef struct {
    int16_t x;          // mm
    int16_t y;          // mm
    int16_t speed;      // cm/s (positive = moving away? check sign convention)
    uint16_t dist_res;  // mm (distance resolution, optional)
} radar_target_t;

static uint8_t radar_buffer[512];
static uint16_t buffer_len = 0;
static lv_obj_t *uart_display_label;

HardwareSerial Serial6(USART6); 

static int16_t decode_ld2450_value(uint8_t low, uint8_t high);
void parse_radar_data(uint8_t *buf, int len);
void myTask(void *pvParameters);
static void event_handler(lv_event_t * e);
void uiTask();


void mySetup()
{
  uiTask();

  Serial6.setTx(TX_PIN);
  Serial6.setRx(RX_PIN);
  Serial6.begin(115200);
  Serial6.setTimeout(10);  // non-blocking reads

  xTaskCreate(myTask, "RadarTask", 2048, NULL, 2, NULL); //here
}

void loop()
{
  // Empty – FreeRTOS handles everything
  vTaskDelay(pdMS_TO_TICKS(1000));
}
static int16_t decode_ld2450_value(uint8_t low, uint8_t high) {
    uint16_t raw = low | (high << 8);
    if (high & 0x80) {
        // Positive: clear the sign bit and return as positive int
        return (int16_t)(raw & 0x7FFF);
    } else {
        // Negative: clear the sign bit, then negate
        return -(int16_t)(raw & 0x7FFF);
    }
}

void parse_radar_data(uint8_t *buf, int len) {
    for (int i = 0; i <= len - 30; i++) {  // minimal frame length = 30 bytes
        // Check full 4-byte header
        if (buf[i] == 0xAA && buf[i+1] == 0xFF && buf[i+2] == 0x03 && buf[i+3] == 0x00) {
            radar_target_t targets[3];
            
            // Parse each of the 3 targets
            for (int t = 0; t < 3; t++) {
                int offset = i + 4 + t * 8;   // each target uses 8 bytes
                uint8_t xl = buf[offset];
                uint8_t xh = buf[offset+1];
                uint8_t yl = buf[offset+2];
                uint8_t yh = buf[offset+3];
                uint8_t sl = buf[offset+4];
                uint8_t sh = buf[offset+5];
                uint8_t rl = buf[offset+6];
                uint8_t rh = buf[offset+7];
                
                targets[t].x = decode_ld2450_value(xl, xh);
                targets[t].y = decode_ld2450_value(yl, yh);
                targets[t].speed = decode_ld2450_value(sl, sh);
                targets[t].dist_res = (rl | (rh << 8));
            }
            
            // Optional: verify end marker (55 CC) – not mandatory but good for sanity
            // if (buf[i+28] == 0x55 && buf[i+29] == 0xCC) { ... }
            
            // --- Use the data (example: display first non-zero target) ---
            for (int t = 0; t < 3; t++) {
                if (targets[t].x != 0 || targets[t].y != 0) { // target exists
                    char display_str[80];
                    snprintf(display_str, sizeof(display_str),
                             "T%d: x=%5d mm, y=%5d mm, sp=%3d cm/s",
                             t+1, targets[t].x, targets[t].y, targets[t].speed);
                    // Update your UI (assuming LVGL)
                    lv_lock();
                    lv_label_set_text(uart_display_label, display_str);
                    lv_unlock();
                    break; // show only the first valid target, or loop to show all
                }
            }
            
            // Skip the whole frame (30 bytes) to continue searching
            i += 29; // loop will increment again, so +29 ends at next byte after frame
        }
    }
}

void myTask(void *pvParameters)
{
  vTaskDelay(pdMS_TO_TICKS(100));

  while (1)
  {
    while (Serial6.available())
    {
      uint8_t c = Serial6.read();
      if (buffer_len < sizeof(radar_buffer))
        radar_buffer[buffer_len++] = c;
      else
        buffer_len = 0; // overflow – reset

      // Try to parse whenever we have at least 8 bytes
      if (buffer_len >= 8)
        parse_radar_data(radar_buffer, buffer_len);
    }

    // Small delay to prevent task from hogging CPU
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

static void event_handler(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
    LV_LOG_USER("Clicked");
  else if (code == LV_EVENT_VALUE_CHANGED)
    LV_LOG_USER("Toggled");
}

void uiTask()
{
  /*Create a Tab view object*/
    lv_obj_t * tabview;
    tabview = lv_tabview_create(lv_screen_active());
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_RIGHT);
    lv_tabview_set_tab_bar_size(tabview, 80);

    lv_obj_set_style_bg_color(tabview, lv_palette_lighten(LV_PALETTE_GREEN, 2), 0);

    lv_obj_t * tab_buttons = lv_tabview_get_tab_bar(tabview);
    lv_obj_set_style_bg_color(tab_buttons, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_color(tab_buttons, lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
    lv_obj_set_style_border_side(tab_buttons, LV_BORDER_SIDE_RIGHT, LV_PART_ITEMS | LV_STATE_CHECKED);

    lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "Mode 1");
    lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "Mode 2");
    lv_obj_t * tab3 = lv_tabview_add_tab(tabview, "Mode 3");

    lv_obj_set_style_bg_color(tab2, lv_palette_lighten(LV_PALETTE_TEAL, 3), 0);
    lv_obj_set_style_bg_opa(tab2, LV_OPA_COVER, 0);

    /*Add content to the tabs*/
    lv_obj_t * label = lv_label_create(tab1);
    lv_label_set_text(label, "First tab");

    label = lv_label_create(tab2);
    lv_label_set_text(label, "Second tab");

    lv_obj_t * btn = lv_button_create(label);
    lv_obj_set_size(btn, 100, 50);
    lv_obj_center(btn);


    label = lv_label_create(tab3);
    lv_label_set_text(label, "Third tab");

    lv_obj_remove_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);

  // Radar display label
  uart_display_label = lv_label_create(lv_screen_active());
  lv_label_set_text(uart_display_label, "Waiting for radar...");
  lv_obj_align(uart_display_label, LV_ALIGN_TOP_MID, 0, 20);
}