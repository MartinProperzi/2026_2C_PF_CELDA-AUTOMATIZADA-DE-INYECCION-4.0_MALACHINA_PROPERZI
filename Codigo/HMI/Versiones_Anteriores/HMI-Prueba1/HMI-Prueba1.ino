/*******************************************************************************
 * JC8012P4A1C LVGL 9.3 Squareline Launcher
 *
 * For the Guition JC8012P4A1C ESP32-P4 LCD (800x1280 JD9365 MIPI-DSI panel +
 * GSL3680 capacitive touch).  Display rotation is done by the ESP32-P4 PPA
 * in hardware.
 *
 * Demonstrates touchscreen, LVGL 9.3 and a SquareLine Studio UI.
 *
 * Orientation: lv_setup.begin(rotation) accepts 0 = portrait (800x1280, the
 * panel's native orientation), 1 = landscape (1280x800), 2 = portrait flipped,
 * 3 = landscape flipped (degrees 90/180/270 work as aliases for 1/2/3).  This
 * sketch defaults to portrait; change the argument below to rotate.  Display
 * and touch rotate together.  Size your SquareLine project to match the
 * orientation you choose:
 *   portrait  rotation 0 or 2  ->  800x1280
 *   landscape rotation 1 or 3  ->  1280x800
 *
 * Requires the following libraries:
 *   - chipguy_JC8012P4A1C_display  (display + touch driver, this library)
 *   - lvgl 9.3
 *
 * Arduino IDE Board Settings:
 *   - Board: ESP32P4 Dev Module
 *   - PSRAM: Enabled   (REQUIRED — the sketch will not compile without it)
 *   - Flash Size: 16 MegaBytes (128 megabits)
 *   - USB CDC On Boot: Enabled
 *
 ******************************************************************************/

// This is built for LVGL 9.3, which SquareLine can generate UIs for.
// It's important for the version numbers to match, even if it's not newest.
#include "lv_conf.h"
#include "lvgl.h"
#include "lv_setup.hpp"

// Contadores para MANUAL y AUTOMATICO de la Inyectora
static int manual_count = 0;
static int auto_count = 0;
static lv_obj_t * lbl_manual_count = NULL;
static lv_obj_t * lbl_auto_count = NULL;

// Declaración de pantallas (vistas)
static lv_obj_t * screen_main = NULL;
static lv_obj_t * screen_inyectora = NULL;
static lv_obj_t * screen_robot = NULL;
static lv_obj_t * screen_camara = NULL;

// Callbacks de navegación
static void btn_back_event_handler(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_screen_load(screen_main);
    }
}

static void btn_inyectora_click(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_screen_load(screen_inyectora);
    }
}

static void btn_robot_click(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_screen_load(screen_robot);
    }
}

static void btn_camara_click(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_screen_load(screen_camara);
    }
}

// Calladores de contadores para Inyectora
static void btn_manual_click(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        manual_count++;
        char buf[32];
        lv_snprintf(buf, sizeof(buf), "Pulsos: %d", manual_count);
        lv_label_set_text(lbl_manual_count, buf);
    }
}

static void btn_auto_click(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        auto_count++;
        char buf[32];
        lv_snprintf(buf, sizeof(buf), "Pulsos: %d", auto_count);
        lv_label_set_text(lbl_auto_count, buf);
    }
}

// Función auxiliar para crear el botón superior de "Volver"
static void create_back_button(lv_obj_t * parent) {
    lv_obj_t * btn_back = lv_btn_create(parent);
    lv_obj_set_size(btn_back, 120, 50);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x3A506B), 0);
    lv_obj_add_event_cb(btn_back, btn_back_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " VOLVER");
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_back);
}

void setup() {
    Serial0.begin(115200);

    // Inicializamos en modo horizontal (1 = landscape 1280x800)
    lv_setup.begin(1);

    // ==========================================
    // 1. PANTALLA PRINCIPAL (MENU)
    // ==========================================
    screen_main = lv_screen_active();
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0x0B132B), 0);

    lv_obj_t * title = lv_label_create(screen_main);
    lv_label_set_text(title, "HMI CELDA 4.0");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t * panel = lv_obj_create(screen_main);
    lv_obj_set_size(panel, 1150, 480);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x1C2541), 0); 
    lv_obj_set_style_border_color(panel, lv_color_hex(0x3A506B), 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_ROW); 
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // INYECTORA
    lv_obj_t * btn_iny = lv_btn_create(panel);
    lv_obj_set_size(btn_iny, 320, 360);
    lv_obj_set_style_bg_color(btn_iny, lv_color_hex(0x0B132B), 0);
    lv_obj_set_style_border_color(btn_iny, lv_color_hex(0x3A506B), 0);
    lv_obj_set_style_border_width(btn_iny, 2, 0);
    lv_obj_set_flex_flow(btn_iny, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_iny, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_iny, btn_inyectora_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_iny = lv_label_create(btn_iny);
    lv_label_set_text(lbl_iny, "INYECTORA");
    lv_obj_set_style_text_color(lbl_iny, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_iny, &lv_font_montserrat_20, 0);

    lv_obj_t * icon_iny = lv_label_create(btn_iny);
    lv_label_set_text(icon_iny, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(icon_iny, lv_color_hex(0x3A506B), 0);
    lv_obj_set_style_text_font(icon_iny, &lv_font_montserrat_36, 0);

    // ROBOT
    lv_obj_t * btn_rob = lv_btn_create(panel);
    lv_obj_set_size(btn_rob, 320, 360);
    lv_obj_set_style_bg_color(btn_rob, lv_color_hex(0x0B132B), 0);
    lv_obj_set_style_border_color(btn_rob, lv_color_hex(0x3A506B), 0);
    lv_obj_set_style_border_width(btn_rob, 2, 0);
    lv_obj_set_flex_flow(btn_rob, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_rob, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_rob, btn_robot_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_rob = lv_label_create(btn_rob);
    lv_label_set_text(lbl_rob, "ROBOT");
    lv_obj_set_style_text_color(lbl_rob, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_rob, &lv_font_montserrat_20, 0);

    lv_obj_t * icon_rob = lv_label_create(btn_rob);
    lv_label_set_text(icon_rob, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(icon_rob, lv_color_hex(0x3A506B), 0);
    lv_obj_set_style_text_font(icon_rob, &lv_font_montserrat_36, 0);

    // CAMARA
    lv_obj_t * btn_cam = lv_btn_create(panel);
    lv_obj_set_size(btn_cam, 320, 360);
    lv_obj_set_style_bg_color(btn_cam, lv_color_hex(0x0B132B), 0);
    lv_obj_set_style_border_color(btn_cam, lv_color_hex(0x3A506B), 0);
    lv_obj_set_style_border_width(btn_cam, 2, 0);
    lv_obj_set_flex_flow(btn_cam, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_cam, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_cam, btn_camara_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_cam = lv_label_create(btn_cam);
    lv_label_set_text(lbl_cam, "CAMARA");
    lv_obj_set_style_text_color(lbl_cam, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_cam, &lv_font_montserrat_20, 0);

    lv_obj_t * icon_cam = lv_label_create(btn_cam);
    lv_label_set_text(icon_cam, LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_color(icon_cam, lv_color_hex(0x3A506B), 0);
    lv_obj_set_style_text_font(icon_cam, &lv_font_montserrat_36, 0);


    // ==========================================
    // 2. SECCION INYECTORA (MANUAL / AUTOMATICO)
    // ==========================================
    screen_inyectora = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_inyectora, lv_color_hex(0x0B132B), 0);
    create_back_button(screen_inyectora);

    lv_obj_t * title_iny = lv_label_create(screen_inyectora);
    lv_label_set_text(title_iny, "CONTROL INYECTORA");
    lv_obj_set_style_text_color(title_iny, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_iny, &lv_font_montserrat_28, 0);
    lv_obj_align(title_iny, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t * panel_iny = lv_obj_create(screen_inyectora);
    lv_obj_set_size(panel_iny, 1150, 480);
    lv_obj_align(panel_iny, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(panel_iny, lv_color_hex(0x1C2541), 0); 
    lv_obj_set_style_border_color(panel_iny, lv_color_hex(0x3A506B), 0);
    lv_obj_set_flex_flow(panel_iny, LV_FLEX_FLOW_ROW); 
    lv_obj_set_flex_align(panel_iny, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Botón MANUAL
    lv_obj_t * btn_manual = lv_btn_create(panel_iny);
    lv_obj_set_size(btn_manual, 450, 360);
    lv_obj_set_style_bg_color(btn_manual, lv_color_hex(0x0B132B), 0);
    lv_obj_set_style_border_color(btn_manual, lv_color_hex(0x3A506B), 0);
    lv_obj_set_style_border_width(btn_manual, 2, 0);
    lv_obj_set_flex_flow(btn_manual, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_manual, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_manual, btn_manual_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_m_title = lv_label_create(btn_manual);
    lv_label_set_text(lbl_m_title, "MANUAL");
    lv_obj_set_style_text_color(lbl_m_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_m_title, &lv_font_montserrat_24, 0);

    lbl_manual_count = lv_label_create(btn_manual);
    lv_label_set_text(lbl_manual_count, "Pulsos: 0");
    lv_obj_set_style_text_color(lbl_manual_count, lv_color_hex(0x00FFCC), 0);
    lv_obj_set_style_text_font(lbl_manual_count, &lv_font_montserrat_20, 0);

    // Botón AUTOMATICO
    lv_obj_t * btn_auto = lv_btn_create(panel_iny);
    lv_obj_set_size(btn_auto, 450, 360);
    lv_obj_set_style_bg_color(btn_auto, lv_color_hex(0x0B132B), 0);
    lv_obj_set_style_border_color(btn_auto, lv_color_hex(0x3A506B), 0);
    lv_obj_set_style_border_width(btn_auto, 2, 0);
    lv_obj_set_flex_flow(btn_auto, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_auto, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_auto, btn_auto_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_a_title = lv_label_create(btn_auto);
    lv_label_set_text(lbl_a_title, "AUTOMATICO");
    lv_obj_set_style_text_color(lbl_a_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_a_title, &lv_font_montserrat_24, 0);

    lbl_auto_count = lv_label_create(btn_auto);
    lv_label_set_text(lbl_auto_count, "Pulsos: 0");
    lv_obj_set_style_text_color(lbl_auto_count, lv_color_hex(0x00FFCC), 0);
    lv_obj_set_style_text_font(lbl_auto_count, &lv_font_montserrat_20, 0);


    // ==========================================
    // 3. SECCION ROBOT (POSICION X, Y, GRIPPER)
    // ==========================================
    screen_robot = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_robot, lv_color_hex(0x0B132B), 0);
    create_back_button(screen_robot);

    lv_obj_t * title_rob = lv_label_create(screen_robot);
    lv_label_set_text(title_rob, "ESTADO ROBOT");
    lv_obj_set_style_text_color(title_rob, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_rob, &lv_font_montserrat_28, 0);
    lv_obj_align(title_rob, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t * panel_rob = lv_obj_create(screen_robot);
    lv_obj_set_size(panel_rob, 1150, 480);
    lv_obj_align(panel_rob, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(panel_rob, lv_color_hex(0x1C2541), 0); 
    lv_obj_set_style_border_color(panel_rob, lv_color_hex(0x3A506B), 0);
    lv_obj_set_flex_flow(panel_rob, LV_FLEX_FLOW_ROW); 
    lv_obj_set_flex_align(panel_rob, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Recuadro Posición X
    lv_obj_t * box_x = lv_obj_create(panel_rob);
    lv_obj_set_size(box_x, 340, 360);
    lv_obj_set_style_bg_color(box_x, lv_color_hex(0x0B132B), 0);
    lv_obj_set_style_border_color(box_x, lv_color_hex(0x3A506B), 0);
    lv_obj_set_flex_flow(box_x, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box_x, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t * lbl_x_title = lv_label_create(box_x);
    lv_label_set_text(lbl_x_title, "POSICION EN X");
    lv_obj_set_style_text_color(lbl_x_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_x_title, &lv_font_montserrat_20, 0);

    lv_obj_t * lbl_x_val = lv_label_create(box_x);
    lv_label_set_text(lbl_x_val, "0.00 mm");
    lv_obj_set_style_text_color(lbl_x_val, lv_color_hex(0x00FFCC), 0);
    lv_obj_set_style_text_font(lbl_x_val, &lv_font_montserrat_24, 0);

    // Recuadro Posición Y
    lv_obj_t * box_y = lv_obj_create(panel_rob);
    lv_obj_set_size(box_y, 340, 360);
    lv_obj_set_style_bg_color(box_y, lv_color_hex(0x0B132B), 0);
    lv_obj_set_style_border_color(box_y, lv_color_hex(0x3A506B), 0);
    lv_obj_set_flex_flow(box_y, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box_y, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * lbl_y_title = lv_label_create(box_y);
    lv_label_set_text(lbl_y_title, "POSICION EN Y");
    lv_obj_set_style_text_color(lbl_y_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_y_title, &lv_font_montserrat_20, 0);

    lv_obj_t * lbl_y_val = lv_label_create(box_y);
    lv_label_set_text(lbl_y_val, "0.00 mm");
    lv_obj_set_style_text_color(lbl_y_val, lv_color_hex(0x00FFCC), 0);
    lv_obj_set_style_text_font(lbl_y_val, &lv_font_montserrat_24, 0);

    // Recuadro Gripper Abierto
    lv_obj_t * box_grip = lv_obj_create(panel_rob);
    lv_obj_set_size(box_grip, 340, 360);
    lv_obj_set_style_bg_color(box_grip, lv_color_hex(0x0B132B), 0);
    lv_obj_set_style_border_color(box_grip, lv_color_hex(0x3A506B), 0);
    lv_obj_set_flex_flow(box_grip, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box_grip, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * lbl_g_title = lv_label_create(box_grip);
    lv_label_set_text(lbl_g_title, "GRIPPER ABIERTO");
    lv_obj_set_style_text_color(lbl_g_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_g_title, &lv_font_montserrat_20, 0);

    lv_obj_t * lbl_g_val = lv_label_create(box_grip);
    lv_label_set_text(lbl_g_val, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(lbl_g_val, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(lbl_g_val, &lv_font_montserrat_48, 0);


    // ==========================================
    // 4. SECCION CAMARA (PIEZA OK / SCRAP)
    // ==========================================
    screen_camara = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_camara, lv_color_hex(0x0B132B), 0);
    create_back_button(screen_camara);

    lv_obj_t * title_cam = lv_label_create(screen_camara);
    lv_label_set_text(title_cam, "INSPECCION CAMARA");
    lv_obj_set_style_text_color(title_cam, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_cam, &lv_font_montserrat_28, 0);
    lv_obj_align(title_cam, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t * panel_cam = lv_obj_create(screen_camara);
    lv_obj_set_size(panel_cam, 1150, 480);
    lv_obj_align(panel_cam, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(panel_cam, lv_color_hex(0x1C2541), 0); 
    lv_obj_set_style_border_color(panel_cam, lv_color_hex(0x3A506B), 0);
    lv_obj_set_flex_flow(panel_cam, LV_FLEX_FLOW_ROW); 
    lv_obj_set_flex_align(panel_cam, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Recuadro Pieza OK
    lv_obj_t * box_ok = lv_obj_create(panel_cam);
    lv_obj_set_size(box_ok, 500, 360);
    lv_obj_set_style_bg_color(box_ok, lv_color_hex(0x0B132B), 0);
    lv_obj_set_style_border_color(box_ok, lv_color_hex(0x3A506B), 0);
    lv_obj_set_flex_flow(box_ok, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box_ok, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * lbl_ok_title = lv_label_create(box_ok);
    lv_label_set_text(lbl_ok_title, "PIEZA OK");
    lv_obj_set_style_text_color(lbl_ok_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_ok_title, &lv_font_montserrat_24, 0);

    // Tilde verde usando símbolos de LVGL
    lv_obj_t * lbl_ok_icon = lv_label_create(box_ok);
    lv_label_set_text(lbl_ok_icon, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(lbl_ok_icon, lv_color_hex(0x00FF00), 0); // Verde
    lv_obj_set_style_text_font(lbl_ok_icon, &lv_font_montserrat_48, 0);

    // Recuadro Pieza Scrap
    lv_obj_t * box_scrap = lv_obj_create(panel_cam);
    lv_obj_set_size(box_scrap, 500, 360);
    lv_obj_set_style_bg_color(box_scrap, lv_color_hex(0x0B132B), 0);
    lv_obj_set_style_border_color(box_scrap, lv_color_hex(0x3A506B), 0);
    lv_obj_set_flex_flow(box_scrap, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box_scrap, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * lbl_scrap_title = lv_label_create(box_scrap);
    lv_label_set_text(lbl_scrap_title, "PIEZA SCRAP");
    lv_obj_set_style_text_color(lbl_scrap_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_scrap_title, &lv_font_montserrat_24, 0);

    // Equis roja utilizando un símbolo de cierre/cruz (`LV_SYMBOL_CLOSE`)
    lv_obj_t * lbl_scrap_icon = lv_label_create(box_scrap);
    lv_label_set_text(lbl_scrap_icon, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(lbl_scrap_icon, lv_color_hex(0xFF0000), 0); // Rojo
    lv_obj_set_style_text_font(lbl_scrap_icon, &lv_font_montserrat_48, 0);

    Serial0.println("--- Estructura de Secciones e Interfaz Navegable Lista ---");
}

void loop() {
    lv_timer_handler();
    delay(5);
}