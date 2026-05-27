#include "lvgl.h"

#define TX_PIN PC6  // Physical pin D1 (TX to Radar's RX)
#define RX_PIN PC7  // Physical pin D0 (RX to Radar's TX)

static void event_handler(lv_event_t * e);
void testLvgl();
void myTask(void *pvParameters);

#ifdef ARDUINO
#include "lvglDrivers.h"
#include <Arduino.h>

// Radar parsing buffer
static uint8_t radar_buffer[512];
static uint16_t buffer_len = 0;
static lv_obj_t *uart_display_label;

HardwareSerial Serial6(USART6); 

void mySetup()
{
  // Initialise LVGL and UI
  testLvgl();

  // Initialise USART6 for LD2450 radar (115200 baud)
Serial6.setTx(TX_PIN);
  Serial6.setRx(RX_PIN);
  Serial6.begin(115200);
  Serial6.setTimeout(10);  // non-blocking reads

  // Create the radar reading task (higher priority than default)
  xTaskCreate(myTask, "RadarTask", 2048, NULL, 2, NULL);
}

void loop()
{
  // Empty – FreeRTOS handles everything
  vTaskDelay(pdMS_TO_TICKS(1000));
}

#else
// Simulator part (unchanged)
#include "lvgl.h"
#include "app_hal.h"
#include <cstdio>

int main(void)
{
  printf("LVGL Simulator\n");
  fflush(stdout);
  lv_init();
  hal_setup();
  testLvgl();
  hal_loop();
  return 0;
}
#endif

// Parse radar data exactly as your PC code does
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

      // Update LVGL label (must be called from the same task that owns LVGL)
      static char display_str[64];
      snprintf(display_str, sizeof(display_str), "x:%5d y:%5d", x, y);
      lv_label_set_text(uart_display_label, display_str);

      i += 7; // skip parsed frame
    }
  }
}

void myTask(void *pvParameters)
{
  // Allow some time for hardware to settle
  vTaskDelay(pdMS_TO_TICKS(100));

  while (1)
  {
    // Read all available bytes from Serial6
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

void testLvgl()
{
  // Your existing buttons
  lv_obj_t * label;
  lv_obj_t * btn1 = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
  lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);
  label = lv_label_create(btn1);
  lv_label_set_text(label, "Button");
  lv_obj_center(label);

  lv_obj_t * btn2 = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
  lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_height(btn2, LV_SIZE_CONTENT);
  label = lv_label_create(btn2);
  lv_label_set_text(label, "Toggle");
  lv_obj_center(label);

  // Radar display label
  uart_display_label = lv_label_create(lv_screen_active());
  lv_label_set_text(uart_display_label, "Waiting for radar...");
  lv_obj_align(uart_display_label, LV_ALIGN_TOP_MID, 0, 20);
}