#include "UIManager.h"
#include "HMIData.h"
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>

// Colores locales de la vista de secuencia
#define COLOR_FONDO_GRIS    lv_color_hex(0xE0E0E0)
#define COLOR_INACTIVO      lv_color_hex(0xF5F5F5)
#define COLOR_ACTIVO        lv_color_hex(0x00FF00)
#define COLOR_BORDE_AZUL    lv_color_hex(0x0055AA)
#define COLOR_FONDO_ACTIVO  lv_color_hex(0x87CEEB)
#define COLOR_BORDE_ROJO    lv_color_hex(0xAA0000)
#define COLOR_GRIS_CAJA     lv_color_hex(0xB0B0B0)
#define COLOR_NEGRO_TEXTO   lv_color_hex(0x000000)
#define COLOR_ROJO_TEXTO    lv_color_hex(0xAA0000)
#define COLOR_VERDE_TEXTO   lv_color_hex(0x00AA00)

// Punteros locales
static lv_obj_t * ind_cerrar;
static lv_obj_t * ind_inyectar;
static lv_obj_t * ind_enfriar;
static lv_obj_t * ind_abrir;
static lv_obj_t * ind_rb_entra;
static lv_obj_t * ind_expulsor;
static lv_obj_t * ind_agarrar;
static lv_obj_t * ind_rb_sale;

static lv_obj_t * box_zona1;
static lv_obj_t * box_zona2;
static lv_obj_t * box_zona3;
static lv_obj_t * lbl_z1_val;
static lv_obj_t * lbl_z1_sp;
static lv_obj_t * lbl_z2_val;
static lv_obj_t * lbl_z2_sp;
static lv_obj_t * lbl_z3_val;
static lv_obj_t * lbl_z3_sp;

static lv_obj_t * btn_automatico;
static lv_obj_t * btn_manual;

// Funciones externas definidas en UIManager.cpp
void limpiar_workspace();

static void btn_auto_event_cb(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        bool auto_actual = (HMI.plc_regs[MB_HMI_BOTONES] & (1 << 2)) != 0;
        if (!auto_actual) {
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 2, 1);
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 1, 0);
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 0, 0);
        } else {
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 2, 0);
        }
    }
}

static void btn_manual_event_cb(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        bool manual_actual = (HMI.plc_regs[MB_HMI_BOTONES] & (1 << 0)) != 0;
        if (!manual_actual) {
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 0, 1);
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 1, 0);
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 2, 0);
        } else {
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 0, 0);
        }
    }
}

void dibujar_vista_secuencia() {
    limpiar_workspace();

    static int32_t col_dsc[] = {190, 190, 190, 190, 190, 190, LV_GRID_TEMPLATE_LAST};
    static int32_t row_dsc[] = {160, 160, 160, LV_GRID_TEMPLATE_LAST};

    lv_obj_t * grid = lv_obj_create(workspace);
    lv_obj_set_size(grid, lv_pct(98), lv_pct(98));
    lv_obj_center(grid);
    lv_obj_set_style_bg_color(grid, COLOR_FONDO_GRIS, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    auto crear_indicador = [](lv_obj_t * p, int c, int f, const char * sim, const char * nom) -> lv_obj_t * {
        lv_obj_t * panel = lv_obj_create(p);
        lv_obj_set_size(panel, 175, 145);
        lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(panel, COLOR_INACTIVO, 0);
        lv_obj_set_style_border_color(panel, COLOR_BORDE_AZUL, 0);
        lv_obj_set_style_border_width(panel, 5, 0);
        
        lv_obj_t * lbl_sim = lv_label_create(panel);
        lv_label_set_text(lbl_sim, sim);
        lv_obj_set_style_text_color(lbl_sim, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_font(lbl_sim, &lv_font_montserrat_32, 0); 
        lv_obj_align(lbl_sim, LV_ALIGN_CENTER, 0, -20);

        lv_obj_t * lbl_nom = lv_label_create(panel);
        lv_label_set_text(lbl_nom, nom);
        lv_obj_set_style_text_color(lbl_nom, lv_color_hex(0x555555), 0);
        lv_obj_set_style_text_font(lbl_nom, &lv_font_montserrat_24, 0); 
        lv_obj_align(lbl_nom, LV_ALIGN_BOTTOM_MID, 0, -5);

        lv_obj_set_grid_cell(panel, LV_GRID_ALIGN_CENTER, c, 1, LV_GRID_ALIGN_CENTER, f, 1);
        return panel;
    };

    ind_cerrar     = crear_indicador(grid, 0, 0, LV_SYMBOL_RIGHT, "CIERRE");
    ind_inyectar   = crear_indicador(grid, 1, 0, LV_SYMBOL_BATTERY_2, "INYECCION");
    ind_enfriar    = crear_indicador(grid, 2, 0, LV_SYMBOL_PAUSE, "MOLDEO");
    ind_abrir      = crear_indicador(grid, 3, 0, LV_SYMBOL_LEFT, "APERTURA");

    // Botón Automático
    btn_automatico = lv_button_create(grid);
    lv_obj_set_size(btn_automatico, 370, 145); 
    lv_obj_remove_flag(btn_automatico, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn_automatico, COLOR_INACTIVO, 0);
    lv_obj_set_style_border_color(btn_automatico, COLOR_BORDE_ROJO, 0);
    lv_obj_set_style_border_width(btn_automatico, 5, 0);
    
    lv_obj_t * lbl_auto_sim = lv_label_create(btn_automatico);
    lv_label_set_text(lbl_auto_sim, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(lbl_auto_sim, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(lbl_auto_sim, &lv_font_montserrat_32, 0);
    lv_obj_align(lbl_auto_sim, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t * lbl_auto_nom = lv_label_create(btn_automatico);
    lv_label_set_text(lbl_auto_nom, "AUTOMATICO");
    lv_obj_set_style_text_color(lbl_auto_nom, lv_color_hex(0x555555), 0);
    lv_obj_set_style_text_font(lbl_auto_nom, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_auto_nom, LV_ALIGN_BOTTOM_MID, 0, -5);

    lv_obj_set_grid_cell(btn_automatico, LV_GRID_ALIGN_CENTER, 4, 2, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(btn_automatico, btn_auto_event_cb, LV_EVENT_CLICKED, NULL);

    // Caja Zona 1
    box_zona1 = lv_obj_create(grid);
    lv_obj_set_size(box_zona1, 175, 325);
    lv_obj_remove_flag(box_zona1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(box_zona1, COLOR_GRIS_CAJA, 0);
    lv_obj_set_style_border_color(box_zona1, COLOR_GRIS_CAJA, 0);
    lv_obj_set_style_border_width(box_zona1, 5, 0);

    lv_obj_t * l1 = lv_label_create(box_zona1);
    lv_label_set_text(l1, "ZONA 1");
    lv_obj_set_style_text_color(l1, COLOR_NEGRO_TEXTO, 0);
    lv_obj_set_style_text_font(l1, &lv_font_montserrat_24, 0);
    lv_obj_align(l1, LV_ALIGN_TOP_MID, 0, 10);

    lbl_z1_val = lv_label_create(box_zona1);
    lv_label_set_text(lbl_z1_val, "PV: 0 °C");
    lv_obj_set_style_text_color(lbl_z1_val, COLOR_ROJO_TEXTO, 0);
    lv_obj_set_style_text_font(lbl_z1_val, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_z1_val, LV_ALIGN_CENTER, 0, -10);

    lbl_z1_sp = lv_label_create(box_zona1);
    lv_label_set_text(lbl_z1_sp, "SV: 0 °C");
    lv_obj_set_style_text_color(lbl_z1_sp, COLOR_VERDE_TEXTO, 0);
    lv_obj_set_style_text_font(lbl_z1_sp, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_z1_sp, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_grid_cell(box_zona1, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 2);

    // Caja Zona 2
    box_zona2 = lv_obj_create(grid);
    lv_obj_set_size(box_zona2, 175, 325);
    lv_obj_remove_flag(box_zona2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(box_zona2, COLOR_GRIS_CAJA, 0);
    lv_obj_set_style_border_color(box_zona2, COLOR_GRIS_CAJA, 0);
    lv_obj_set_style_border_width(box_zona2, 5, 0);

    lv_obj_t * l2 = lv_label_create(box_zona2);
    lv_label_set_text(l2, "ZONA 2");
    lv_obj_set_style_text_color(l2, COLOR_NEGRO_TEXTO, 0);
    lv_obj_set_style_text_font(l2, &lv_font_montserrat_24, 0);
    lv_obj_align(l2, LV_ALIGN_TOP_MID, 0, 10);

    lbl_z2_val = lv_label_create(box_zona2);
    lv_label_set_text(lbl_z2_val, "PV: 0 °C");
    lv_obj_set_style_text_color(lbl_z2_val, COLOR_ROJO_TEXTO, 0);
    lv_obj_set_style_text_font(lbl_z2_val, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_z2_val, LV_ALIGN_CENTER, 0, -10);

    lbl_z2_sp = lv_label_create(box_zona2);
    lv_label_set_text(lbl_z2_sp, "SV: 0 °C");
    lv_obj_set_style_text_color(lbl_z2_sp, COLOR_VERDE_TEXTO, 0);
    lv_obj_set_style_text_font(lbl_z2_sp, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_z2_sp, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_grid_cell(box_zona2, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 1, 2);

    // Caja Zona 3
    box_zona3 = lv_obj_create(grid);
    lv_obj_set_size(box_zona3, 175, 325);
    lv_obj_remove_flag(box_zona3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(box_zona3, COLOR_GRIS_CAJA, 0);
    lv_obj_set_style_border_color(box_zona3, COLOR_GRIS_CAJA, 0);
    lv_obj_set_style_border_width(box_zona3, 5, 0);

    lv_obj_t * l3 = lv_label_create(box_zona3);
    lv_label_set_text(l3, "ZONA 3");
    lv_obj_set_style_text_color(l3, COLOR_NEGRO_TEXTO, 0);
    lv_obj_set_style_text_font(l3, &lv_font_montserrat_24, 0);
    lv_obj_align(l3, LV_ALIGN_TOP_MID, 0, 10);

    lbl_z3_val = lv_label_create(box_zona3);
    lv_label_set_text(lbl_z3_val, "PV: 0 °C");
    lv_obj_set_style_text_color(lbl_z3_val, COLOR_ROJO_TEXTO, 0);
    lv_obj_set_style_text_font(lbl_z3_val, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_z3_val, LV_ALIGN_CENTER, 0, -10);

    lbl_z3_sp = lv_label_create(box_zona3);
    lv_label_set_text(lbl_z3_sp, "SV: 0 °C");
    lv_obj_set_style_text_color(lbl_z3_sp, COLOR_VERDE_TEXTO, 0);
    lv_obj_set_style_text_font(lbl_z3_sp, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_z3_sp, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_grid_cell(box_zona3, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 1, 2);

    // Botón Manual
    btn_manual = lv_button_create(grid);
    lv_obj_set_size(btn_manual, 370, 145);
    lv_obj_remove_flag(btn_manual, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn_manual, COLOR_INACTIVO, 0);
    lv_obj_set_style_border_color(btn_manual, COLOR_BORDE_ROJO, 0);
    lv_obj_set_style_border_width(btn_manual, 5, 0);

    lv_obj_t * lbl_man_sim = lv_label_create(btn_manual);
    lv_label_set_text(lbl_man_sim, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(lbl_man_sim, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(lbl_man_sim, &lv_font_montserrat_32, 0);
    lv_obj_align(lbl_man_sim, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t * lbl_man_nom = lv_label_create(btn_manual);
    lv_label_set_text(lbl_man_nom, "MANUAL");
    lv_obj_set_style_text_color(lbl_man_nom, lv_color_hex(0x555555), 0);
    lv_obj_set_style_text_font(lbl_man_nom, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_man_nom, LV_ALIGN_BOTTOM_MID, 0, -5);

    lv_obj_set_grid_cell(btn_manual, LV_GRID_ALIGN_CENTER, 4, 2, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_add_event_cb(btn_manual, btn_manual_event_cb, LV_EVENT_CLICKED, NULL);

    // Indicadores restantes de la secuencia
    ind_rb_entra   = crear_indicador(grid, 3, 1, LV_SYMBOL_DOWN, "Rb IN");
    ind_expulsor   = crear_indicador(grid, 3, 2, LV_SYMBOL_RIGHT, "EXPULSION");
    ind_agarrar    = crear_indicador(grid, 4, 2, LV_SYMBOL_SHUFFLE, "GRIPPER");
    ind_rb_sale    = crear_indicador(grid, 5, 2, LV_SYMBOL_UP, "Rb OUT");
}


void VistaSecuencia_Update() {
    char buf_z1_v[24], buf_z1_s[24];
    char buf_z2_v[24], buf_z2_s[24];
    char buf_z3_v[24], buf_z3_s[24];

    // Snapshot atómico de los registros bajo mutex
    uint16_t z1_pv, z1_sv, z2_pv, z2_sv, z3_pv, z3_sv, botones, paso;
    HMI_Lock();
    z1_pv = HMI.plc_regs[MB_TEMPZ1];
    z1_sv = HMI.plc_regs[MB_SETPOINTZ1];
    z2_pv = HMI.plc_regs[MB_TEMPZ2];
    z2_sv = HMI.plc_regs[MB_SETPOINTZ2];
    z3_pv = HMI.plc_regs[MB_TEMPZ3];
    z3_sv = HMI.plc_regs[MB_SETPOINTZ3];
    botones = HMI.plc_regs[MB_HMI_BOTONES];
    paso = HMI.plc_regs[MB_PASO_ACTUAL];
    HMI_Unlock();

    // Actualización de Temperaturas Z1, Z2, Z3 conectadas a los nuevos registros del enum
    snprintf(buf_z1_v, sizeof(buf_z1_v), "PV: %d °C", z1_pv);
    if (lbl_z1_val && lv_obj_is_valid(lbl_z1_val)) lv_label_set_text(lbl_z1_val, buf_z1_v);
    snprintf(buf_z1_s, sizeof(buf_z1_s), "SV: %d °C", z1_sv);
    if (lbl_z1_sp && lv_obj_is_valid(lbl_z1_sp)) lv_label_set_text(lbl_z1_sp, buf_z1_s);

    snprintf(buf_z2_v, sizeof(buf_z2_v), "PV: %d °C", z2_pv);
    if (lbl_z2_val && lv_obj_is_valid(lbl_z2_val)) lv_label_set_text(lbl_z2_val, buf_z2_v);
    snprintf(buf_z2_s, sizeof(buf_z2_s), "SV: %d °C", z2_sv);
    if (lbl_z2_sp && lv_obj_is_valid(lbl_z2_sp)) lv_label_set_text(lbl_z2_sp, buf_z2_s);

    snprintf(buf_z3_v, sizeof(buf_z3_v), "PV: %d °C", z3_pv);
    if (lbl_z3_val && lv_obj_is_valid(lbl_z3_val)) lv_label_set_text(lbl_z3_val, buf_z3_v);
    snprintf(buf_z3_s, sizeof(buf_z3_s), "SV: %d °C", z3_sv);
    if (lbl_z3_sp && lv_obj_is_valid(lbl_z3_sp)) lv_label_set_text(lbl_z3_sp, buf_z3_s);

    // Solo actualizamos si los objetos de la vista de secuencia son válidos
    if (ind_cerrar != NULL && lv_obj_is_valid(ind_cerrar)) {
        // Reseteamos colores de secuencia
        lv_obj_set_style_bg_color(ind_cerrar,   COLOR_INACTIVO, 0);
        lv_obj_set_style_bg_color(ind_inyectar, COLOR_INACTIVO, 0);
        lv_obj_set_style_bg_color(ind_enfriar,  COLOR_INACTIVO, 0);
        lv_obj_set_style_bg_color(ind_abrir,    COLOR_INACTIVO, 0);
        lv_obj_set_style_bg_color(ind_expulsor, COLOR_INACTIVO, 0);
        lv_obj_set_style_bg_color(ind_rb_entra, COLOR_INACTIVO, 0);
        lv_obj_set_style_bg_color(ind_agarrar,  COLOR_INACTIVO, 0);
        lv_obj_set_style_bg_color(ind_rb_sale,  COLOR_INACTIVO, 0);

        // Estado del Botón Automático (Bit 2)
        if (btn_automatico && lv_obj_is_valid(btn_automatico)) {
            bool auto_activo = (botones & (1 << 2)) != 0;
            lv_obj_set_style_bg_color(btn_automatico, auto_activo ? COLOR_ACTIVO : COLOR_INACTIVO, 0);
            lv_obj_set_style_border_color(btn_automatico, auto_activo ? COLOR_BORDE_AZUL : COLOR_BORDE_ROJO, 0);
        }

        // Estado del Botón Manual (Bit 0)
        if (btn_manual && lv_obj_is_valid(btn_manual)) {
            bool manual_activo = (botones & (1 << 0)) != 0;
            lv_obj_set_style_bg_color(btn_manual, manual_activo ? COLOR_ACTIVO : COLOR_INACTIVO, 0);
            lv_obj_set_style_border_color(btn_manual, manual_activo ? COLOR_BORDE_AZUL : COLOR_BORDE_ROJO, 0);
        }

        // Pasos del ciclo en color celeste
        if (paso == 1 || paso == 2) {
            lv_obj_set_style_bg_color(ind_cerrar, COLOR_FONDO_ACTIVO, 0);
        } else if (paso >= 4 && paso <= 6) {
            lv_obj_set_style_bg_color(ind_inyectar, COLOR_FONDO_ACTIVO, 0);
        } else if (paso == 7) {
            lv_obj_set_style_bg_color(ind_enfriar, COLOR_FONDO_ACTIVO, 0);
        } else if (paso == 8) {
            lv_obj_set_style_bg_color(ind_abrir, COLOR_FONDO_ACTIVO, 0);
        } else if (paso >= 9 && paso <= 12) {
            lv_obj_set_style_bg_color(ind_expulsor, COLOR_FONDO_ACTIVO, 0);
        }
    }
}