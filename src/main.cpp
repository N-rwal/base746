#include "lvgl.h"            // LittlevGL (LVGL) graphics library
#include "lvglDrivers.h"    // Platform-specific LVGL drivers (display, input)
#include <Arduino.h>         // Arduino compatibility layer / FreeRTOS helpers
#include "LD2450.h"         // LD2450 radar driver (parses radar frames)

#define TX_PIN PC6  // UART TX pin connected to radar module
#define RX_PIN PC7  // UART RX pin connected to radar module

// LVGL object handles used throughout the UI
static lv_obj_t *radar_viz_container; // container for radar visualization (Tab 2)
static lv_obj_t *uart_display_label; // reserved for UART debug display (unused)

// Serial interface wired to the LD2450 radar module
HardwareSerial Serial6(USART6); 

// Labels that show numeric/readable target info on Tab 1
static lv_obj_t *target1_label;
static lv_obj_t *target2_label;
static lv_obj_t *target3_label;

// Small dot objects used to render targets on the visualization (Tab 2)
static lv_obj_t *target_dots[3];

// Radar parser/manager instance. The LD2450 class reads bytes
// from the UART and exposes parsed target data.
LD2450 radar(&Serial6);

// Pre-allocated array for up to 3 detected targets.
LD2450_Target targets[3];

// Forward declarations for RTOS tasks and LVGL event handler
void myTask(void *pvParameters);
static void event_handler(lv_event_t * e);
void uiTask();

// Map radar coordinates (millimeters) into pixel positions inside
// the visualization container and show/hide the dot objects.
// - targets: array of detected targets (x forward, y lateral, etc.)
// - container: LVGL object that defines the visual area (unused, kept for API clarity)
void update_target_dots(LD2450_Target targets[3], lv_obj_t *container) {
  for (int i = 0; i < 3; i++) {
    // If target coordinate is non-zero, consider it a valid detection
    if (targets[i].x != 0 || targets[i].y != 0) {
      // Convert mm coordinates to pixel positions (ad-hoc scaling)
      int px = 200 + (targets[i].x * 200 / 2500);
      int py = 230 - (targets[i].y * 220 / 7000);

      // Clamp to the visualization bounds so dots remain visible
      if (px < 5) px = 5;
      if (px > 395) px = 395;
      if (py < 5) py = 5;
      if (py > 245) py = 245;

      // Position the dot (subtract half size to center)
      lv_obj_set_pos(target_dots[i], px - 5, py - 5);
      lv_obj_clear_flag(target_dots[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      // No valid target: hide the dot
      lv_obj_add_flag(target_dots[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
}

// Equivalent of Arduino's setup(): initialize UI, UART and start task
void mySetup()
{
  // Build the LVGL UI elements
  uiTask();

  // Configure the hardware UART pins used by the radar
  Serial6.setTx(TX_PIN);
  Serial6.setRx(RX_PIN);
  Serial6.begin(115200);
  Serial6.setTimeout(10);

  // Create a FreeRTOS task that will read/process radar data
  xTaskCreate(myTask, "RadarTask", 2048, NULL, 2, NULL);
}

// Main loop is intentionally empty: the real work happens in RTOS tasks.
void loop()
{
  // Yield for a while to avoid busy-waiting.
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// RTOS task: reads UART bytes, feeds parser and updates LVGL UI
void myTask(void *pvParameters)
{
  // Small initial delay to allow system startup
  vTaskDelay(pdMS_TO_TICKS(100));

  while (1)
  {
    // Drain incoming bytes from the radar UART and feed parser
    while (Serial6.available())
    {
      uint8_t c = Serial6.read();
      radar.processByte(c);
    }

    // Retrieve parsed target list from the radar object
    radar.getTargets(targets);
    
    // LVGL is not thread-safe: lock before modifying UI state
    lv_lock();
    
    char display_str[80];
    
    // Format and update Target 1 label (or show placeholder)
    if (targets[0].x != 0 || targets[0].y != 0) {
      snprintf(display_str, sizeof(display_str),
               "Target 1: x=%5d mm, y=%5d mm, sp=%3d cm/s",
               targets[0].x, targets[0].y, targets[0].speed);
    } else {
      snprintf(display_str, sizeof(display_str), "Target 1: ---");
    }
    lv_label_set_text(target1_label, display_str);
    
    // Format and update Target 2 label
    if (targets[1].x != 0 || targets[1].y != 0) {
      snprintf(display_str, sizeof(display_str),
               "Target 2: x=%5d mm, y=%5d mm, sp=%3d cm/s",
               targets[1].x, targets[1].y, targets[1].speed);
    } else {
      snprintf(display_str, sizeof(display_str), "Target 2: ---");
    }
    lv_label_set_text(target2_label, display_str);
    
    // Format and update Target 3 label
    if (targets[2].x != 0 || targets[2].y != 0) {
      snprintf(display_str, sizeof(display_str),
               "Target 3: x=%5d mm, y=%5d mm, sp=%3d cm/s",
               targets[2].x, targets[2].y, targets[2].speed);
    } else {
      snprintf(display_str, sizeof(display_str), "Target 3: ---");
    }
    lv_label_set_text(target3_label, display_str);
    
    // Update visual dot positions on Tab 2
    update_target_dots(targets, radar_viz_container);
    
    // Release LVGL lock
    lv_unlock();

    // Short delay to yield CPU and control update rate
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Simple LVGL event handler used as an example hook for interactive widgets
static void event_handler(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
    LV_LOG_USER("Clicked");
  else if (code == LV_EVENT_VALUE_CHANGED)
    LV_LOG_USER("Toggled");
}

// Build the user interface (LVGL): a 3-tab view with
// - Tab 1: textual target readouts
// - Tab 2: radar visualization with dots and reference lines
// - Tab 3: placeholder
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

  /* Tab 2 content - radar visualization */
  radar_viz_container = lv_obj_create(tab2);
  lv_obj_set_size(radar_viz_container, 400, 250);
  lv_obj_center(radar_viz_container);
  lv_obj_set_style_bg_color(radar_viz_container, lv_color_hex(0x1a1a2e), 0);
  lv_obj_set_style_border_width(radar_viz_container, 2, 0);
  lv_obj_set_style_border_color(radar_viz_container, lv_palette_main(LV_PALETTE_GREY), 0);
  
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