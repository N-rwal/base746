#include "lvgl.h"
#include "lvglDrivers.h"
#include <Arduino.h>
#include "LD2450.h"

#define TX_PIN PC6
#define RX_PIN PC7

static lv_obj_t *uart_display_label;

HardwareSerial Serial6(USART6); 

static lv_obj_t *target1_label;
static lv_obj_t *target2_label;
static lv_obj_t *target3_label;

// Create radar object
LD2450 radar(&Serial6);
LD2450_Target targets[3];

void myTask(void *pvParameters);
static void event_handler(lv_event_t * e);
void uiTask();

void mySetup()
{
  uiTask();

  Serial6.setTx(TX_PIN);
  Serial6.setRx(RX_PIN);
  Serial6.begin(115200);
  Serial6.setTimeout(10);

  xTaskCreate(myTask, "RadarTask", 2048, NULL, 2, NULL);
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}

void myTask(void *pvParameters)
{
  vTaskDelay(pdMS_TO_TICKS(100));

  while (1)
  {
    while (Serial6.available())
    {
      uint8_t c = Serial6.read();
      radar.processByte(c);
    }

    // Get all targets from radar
    radar.getTargets(targets);
    
    // Update UI with all 3 targets
    lv_lock();
    
    char display_str[80];
    
    // Target 1
    if (targets[0].x != 0 || targets[0].y != 0) {
      snprintf(display_str, sizeof(display_str),
               "Target 1: x=%5d mm, y=%5d mm, sp=%3d cm/s",
               targets[0].x, targets[0].y, targets[0].speed);
    } else {
      snprintf(display_str, sizeof(display_str), "Target 1: ---");
    }
    lv_label_set_text(target1_label, display_str);
    
    // Target 2
    if (targets[1].x != 0 || targets[1].y != 0) {
      snprintf(display_str, sizeof(display_str),
               "Target 2: x=%5d mm, y=%5d mm, sp=%3d cm/s",
               targets[1].x, targets[1].y, targets[1].speed);
    } else {
      snprintf(display_str, sizeof(display_str), "Target 2: ---");
    }
    lv_label_set_text(target2_label, display_str);
    
    // Target 3
    if (targets[2].x != 0 || targets[2].y != 0) {
      snprintf(display_str, sizeof(display_str),
               "Target 3: x=%5d mm, y=%5d mm, sp=%3d cm/s",
               targets[2].x, targets[2].y, targets[2].speed);
    } else {
      snprintf(display_str, sizeof(display_str), "Target 3: ---");
    }
    lv_label_set_text(target3_label, display_str);
    
    lv_unlock();

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

  // Create 3 labels for radar targets (rows)
  lv_obj_t * radar_container = lv_obj_create(lv_screen_active());
  lv_obj_set_size(radar_container, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_align(radar_container, LV_ALIGN_TOP_MID, 0, 60);
  lv_obj_set_style_border_width(radar_container, 0, 0);
  lv_obj_set_style_pad_all(radar_container, 10, 0);
  
  // Target 1 label (red theme)
  target1_label = lv_label_create(radar_container);
  lv_label_set_text(target1_label, "Target 1: ---");
  lv_obj_set_style_text_color(target1_label, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_align(target1_label, LV_ALIGN_TOP_LEFT, 0, 0);
  
  // Target 2 label (green theme)
  target2_label = lv_label_create(radar_container);
  lv_label_set_text(target2_label, "Target 2: ---");
  lv_obj_set_style_text_color(target2_label, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_align(target2_label, LV_ALIGN_TOP_LEFT, 0, 30);
  
  // Target 3 label (blue theme)
  target3_label = lv_label_create(radar_container);
  lv_label_set_text(target3_label, "Target 3: ---");
  lv_obj_set_style_text_color(target3_label, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_align(target3_label, LV_ALIGN_TOP_LEFT, 0, 60);
}