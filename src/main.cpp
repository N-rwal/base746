#include "lvgl.h"
#include "lvglDrivers.h"
#include <Arduino.h>
#include "LD2450.h"

#define TX_PIN PC6
#define RX_PIN PC7

static lv_obj_t *radar_viz_container;
static lv_obj_t *uart_display_label;

HardwareSerial Serial6(USART6); 

static lv_obj_t *target1_label;
static lv_obj_t *target2_label;
static lv_obj_t *target3_label;

static lv_obj_t *target_dots[3];

// Create radar object
LD2450 radar(&Serial6);
LD2450_Target targets[3];

void myTask(void *pvParameters);
static void event_handler(lv_event_t * e);
void uiTask();

void update_target_dots(LD2450_Target targets[3], lv_obj_t *container) {
  for (int i = 0; i < 3; i++) {
    if (targets[i].x != 0 || targets[i].y != 0) {
      // X maps directly to horizontal position (left/right strafe)
      int px = 200 + (targets[i].x * 200 / 2500);
      
      // Y maps to vertical position (distance: 0 at bottom, 7000 at top)
      int py = 230 - (targets[i].y * 220 / 7000);
      
      // Clamp to visible area
      if (px < 5) px = 5;
      if (px > 395) px = 395;
      if (py < 5) py = 5;
      if (py > 245) py = 245;
      
      lv_obj_set_pos(target_dots[i], px - 5, py - 5);
      lv_obj_clear_flag(target_dots[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(target_dots[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
}

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
    
    // Update Tab 1 text labels
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
    
    // Update Tab 2 dots
    update_target_dots(targets, radar_viz_container);
    
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

  /* Tab 1 content - Radar readings */
  lv_obj_t * radar_container = lv_obj_create(tab1);
  lv_obj_set_width(radar_container, lv_pct(100));
  lv_obj_set_height(radar_container, LV_SIZE_CONTENT);
  lv_obj_align(radar_container, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_set_style_border_width(radar_container, 1, 0);
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

  /* Tab 2 content - Simple radar visualization */
  // Create a simple container with scroll disabled
  radar_viz_container = lv_obj_create(tab2);
  lv_obj_set_size(radar_viz_container, 400, 250);
  lv_obj_center(radar_viz_container);
  lv_obj_set_style_bg_color(radar_viz_container, lv_color_hex(0x1a1a2e), 0);
  lv_obj_set_style_border_width(radar_viz_container, 2, 0);
  lv_obj_set_style_border_color(radar_viz_container, lv_palette_main(LV_PALETTE_GREY), 0);
  
  // Disable scrolling on the container
  lv_obj_remove_flag(radar_viz_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(radar_viz_container, LV_SCROLLBAR_MODE_OFF);
  
  // Add static reference lines for visual reference
  // Center line (X=0)
  lv_obj_t * center_line = lv_obj_create(radar_viz_container);
  lv_obj_set_size(center_line, 1, 220);
  lv_obj_set_pos(center_line, 200, 15);
  lv_obj_set_style_bg_color(center_line, lv_color_hex(0x444466), 0);
  lv_obj_set_style_border_width(center_line, 0, 0);
  lv_obj_remove_flag(center_line, LV_OBJ_FLAG_SCROLLABLE);
  
  // Horizontal reference line at mid-distance (Y=3500mm)
  lv_obj_t * mid_line = lv_obj_create(radar_viz_container);
  lv_obj_set_size(mid_line, 380, 1);
  lv_obj_set_pos(mid_line, 10, 115);
  lv_obj_set_style_bg_color(mid_line, lv_color_hex(0x444466), 0);
  lv_obj_set_style_border_width(mid_line, 0, 0);
  lv_obj_remove_flag(mid_line, LV_OBJ_FLAG_SCROLLABLE);
  
  // Bottom line (radar position)
  lv_obj_t * radar_line = lv_obj_create(radar_viz_container);
  lv_obj_set_size(radar_line, 380, 1);
  lv_obj_set_pos(radar_line, 10, 230);
  lv_obj_set_style_bg_color(radar_line, lv_color_hex(0xff6666), 0);
  lv_obj_set_style_border_width(radar_line, 0, 0);
  lv_obj_remove_flag(radar_line, LV_OBJ_FLAG_SCROLLABLE);
  
  // Add labels for distances
  lv_obj_t * dist_7m = lv_label_create(radar_viz_container);
  lv_label_set_text(dist_7m, "7m");
  lv_obj_set_style_text_color(dist_7m, lv_color_hex(0x888888), 0);
  lv_obj_set_style_text_font(dist_7m, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(dist_7m, 360, 10);
  lv_obj_remove_flag(dist_7m, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t * dist_35m = lv_label_create(radar_viz_container);
  lv_label_set_text(dist_35m, "3.5m");
  lv_obj_set_style_text_color(dist_35m, lv_color_hex(0x888888), 0);
  lv_obj_set_style_text_font(dist_35m, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(dist_35m, 360, 105);
  lv_obj_remove_flag(dist_35m, LV_OBJ_FLAG_SCROLLABLE);
  
  // Create 3 dots for targets (initially hidden)
  target_dots[0] = lv_obj_create(radar_viz_container);
  target_dots[1] = lv_obj_create(radar_viz_container);
  target_dots[2] = lv_obj_create(radar_viz_container);
  
  for (int i = 0; i < 3; i++) {
    lv_obj_set_size(target_dots[i], 10, 10);
    lv_obj_set_style_radius(target_dots[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(target_dots[i], LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(target_dots[i], 1, 0);
    lv_obj_set_style_border_color(target_dots[i], lv_color_hex(0xffffff), 0);
    lv_obj_add_flag(target_dots[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(target_dots[i], LV_OBJ_FLAG_SCROLLABLE);
  }
  
  // Set colors
  lv_obj_set_style_bg_color(target_dots[0], lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_set_style_bg_color(target_dots[1], lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_style_bg_color(target_dots[2], lv_palette_main(LV_PALETTE_BLUE), 0);

  /* Tab 3 content - Empty for now */
  lv_obj_t * label_tab3 = lv_label_create(tab3);
  lv_label_set_text(label_tab3, "Mode 3 - Coming Soon");
  lv_obj_center(label_tab3);

  lv_obj_remove_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);
}