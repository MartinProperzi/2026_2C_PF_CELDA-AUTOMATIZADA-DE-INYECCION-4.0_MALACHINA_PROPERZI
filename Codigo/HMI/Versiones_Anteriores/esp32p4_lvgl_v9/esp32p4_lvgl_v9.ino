#include <Arduino.h>
#include "lvgl.h"
#include "lvgl_port_v9.h"

void lvgl_sw_rotation(void);

void setup()
{
  Serial.begin(115200);
  Serial.println("--- INICIO HMI CELDA 4.0 ---");

  // 1. Inicializa el hardware base
  lvgl_sw_rotation();

  Serial.println("--- Pantalla lista ---");

  // 2. Fondo azul oscuro (hex: 0x0B132B)
  lv_obj_t * scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B132B), 0);

  // 3. TÍTULO PRINCIPAL: "HMI CELDA 4.0" (letras blancas)
  lv_obj_t * title = lv_label_create(scr);
  lv_label_set_text(title, "HMI CELDA 4.0");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 50);

  // 4. PANEL CONTENEDOR VERTICAL PARA LOS 3 SUBSISTEMAS
  // Como el panel físico mantiene su matriz vertical nativa, apilamos los elementos 
  // uno debajo del otro de forma limpia y perfectamente centrada.
  lv_obj_t * panel = lv_obj_create(scr);
  lv_obj_set_size(panel, 650, 700);
  lv_obj_align(panel, LV_ALIGN_CENTER, 0, 40);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x1C2541), 0); 
  lv_obj_set_style_border_color(panel, lv_color_hex(0x3A506B), 0);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);          // Alinear elementos en columna vertical
  lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // --- Elemento 1: Inyectora ---
  lv_obj_t * lbl_iny = lv_label_create(panel);
  lv_label_set_text(lbl_iny, "INYECTORA");
  lv_obj_set_style_text_color(lbl_iny, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(lbl_iny, &lv_font_montserrat_14, 0);

  // --- Elemento 2: Robot ---
  lv_obj_t * lbl_rob = lv_label_create(panel);
  lv_label_set_text(lbl_rob, "ROBOT");
  lv_obj_set_style_text_color(lbl_rob, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(lbl_rob, &lv_font_montserrat_14, 0);

  // --- Elemento 3: Cámara ---
  lv_obj_t * lbl_cam = lv_label_create(panel);
  lv_label_set_text(lbl_cam, "CAMARA");
  lv_obj_set_style_text_color(lbl_cam, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(lbl_cam, &lv_font_montserrat_14, 0);

  Serial.println("--- Interfaz Vertical Estable Creada ---");
}

void loop()
{
  lv_timer_handler();
  vTaskDelay(pdMS_TO_TICKS(5));
}