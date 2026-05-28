#include "lvgl.h"
#include "lvglDrivers.h"
#include <Arduino.h>


#define TX_PIN PC6
#define RX_PIN PC7

typedef struct {
    int x;
    int y;
} radar_data_t;

static uint8_t radar_buffer[512];
static uint16_t buffer_len = 0;
static lv_obj_t *uart_display_label;
radar_data_t data;

HardwareSerial Serial6(USART6); 


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

void parse_radar_data(uint8_t *buf, int len)
{
  for (int i = 0; i < len - 7; i++)
  {
    if (buf[i] == 0xAA && buf[i+1] == 0xFF)
    {
      int x = buf[i+4] | (buf[i+5] << 8);
      int y = buf[i+6] | (buf[i+7] << 8);

      if (buf[i+5] & 0x80)
        x -= 0x8000;
        y -= 0x8000;
      static char display_str[64];
      snprintf(display_str, sizeof(display_str), "x:%5d y:%5d", x, y);
      lv_lock();
      lv_label_set_text(uart_display_label, display_str);
      lv_unlock();
      data = { x, y };

      i += 7; // skip parsed frame
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
    lv_obj_t * tab4 = lv_tabview_add_tab(tabview, "Mode 4");

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

    label = lv_label_create(tab4);
    lv_label_set_text(label, "Forth tab");

    lv_obj_remove_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);

  // Radar display label
  uart_display_label = lv_label_create(lv_screen_active());
  lv_label_set_text(uart_display_label, "Waiting for radar...");
  lv_obj_align(uart_display_label, LV_ALIGN_TOP_MID, 0, 20);
}