/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_port_v9.h"

static const char *TAG = "lv_minimal";

// Callback de volcado normal para LVGL
static void simple_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    lv_disp_flush_ready(disp);
}

static void tick_increment(void *arg)
{
    lv_tick_inc(2);
}

esp_err_t lvgl_port_init(esp_lcd_panel_handle_t lcd_handle, esp_lcd_touch_handle_t tp_handle, lvgl_port_interface_t interface)
{
    assert(lcd_handle);

    lv_init();

    const esp_timer_create_args_t tick_timer_args = {
        .callback = &tick_increment,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 2000));

    void *fb1 = NULL;
    void *fb2 = NULL;
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(lcd_handle, 2, &fb1, &fb2));
    assert(fb1);

    int total_pixels = LVGL_PORT_H_RES * LVGL_PORT_V_RES;

    lv_display_t *display = lv_display_create(LVGL_PORT_H_RES, LVGL_PORT_V_RES);
    assert(display);

    lv_display_set_buffers(display, fb1, fb2, total_pixels * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(display, simple_flush_cb);
    lv_display_set_user_data(display, lcd_handle);

    ESP_LOGI(TAG, "LVGL inicializado correctamente para pantalla completa");
    return ESP_OK;
}

bool lvgl_port_lock(int timeout_ms) { return true; }
void lvgl_port_unlock(void) {}
bool lvgl_port_notify_lcd_vsync(void) { return false; }