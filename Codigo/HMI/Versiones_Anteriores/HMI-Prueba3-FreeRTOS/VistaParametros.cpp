#include "UIManager.h"
#include "HMIData.h"
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>

// Variables locales de la vista de parámetros
static lv_obj_t * kb_numerico_param = NULL;

// Callback seguro para el teclado y los textareas
static void textarea_param_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = (lv_obj_t *)lv_event_get_target(e);
    int reg = (int)(uintptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_FOCUSED) {
        if (kb_numerico_param != NULL && lv_obj_is_valid(kb_numerico_param)) {
            lv_keyboard_set_textarea(kb_numerico_param, ta);
            lv_obj_clear_flag(kb_numerico_param, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(kb_numerico_param); 
        }
    } 
    else if (code == LV_EVENT_READY || code == LV_EVENT_DEFOCUSED) {
        if (kb_numerico_param != NULL && lv_obj_is_valid(kb_numerico_param)) {
            lv_obj_add_flag(kb_numerico_param, LV_OBJ_FLAG_HIDDEN);
        }
        if (ta && lv_obj_is_valid(ta)) {
            uint16_t val = (uint16_t)atoi(lv_textarea_get_text(ta));
            HMI_Lock();
            HMI.plc_regs[reg] = val;
            HMI_Unlock();
            lv_obj_remove_state(ta, LV_STATE_FOCUSED);
        }
    }
}

void dibujar_vista_parametros() {
    limpiar_workspace();

    if (workspace == NULL || !lv_obj_is_valid(workspace)) return;

    // Teclado numérico flotante
    kb_numerico_param = lv_keyboard_create(workspace);
    lv_keyboard_set_mode(kb_numerico_param, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_size(kb_numerico_param, lv_pct(70), lv_pct(45));
    lv_obj_align(kb_numerico_param, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_flag(kb_numerico_param, LV_OBJ_FLAG_HIDDEN); 

    // Función auxiliar para crear paneles simétricos e igualados
    auto crear_panel = [](const char * tit, int w, int h, int x, int y) -> lv_obj_t * {
        lv_obj_t * p = lv_obj_create(workspace);
        lv_obj_set_size(p, w, h);
        lv_obj_set_pos(p, x, y);
        lv_obj_set_style_bg_color(p, lv_color_hex(0xC0C0C0), 0);
        lv_obj_set_style_border_color(p, lv_color_hex(0x0055AA), 0);
        lv_obj_set_style_border_width(p, 2, 0);
        lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t * l = lv_label_create(p);
        lv_label_set_text(l, tit);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_32, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(0x000000), 0);
        lv_obj_align(l, LV_ALIGN_TOP_LEFT, 10, 10);
        return p;
    };

    // Función auxiliar para crear campos con fuente destacada
    auto crear_param = [](lv_obj_t * p, const char * nom, int y, int reg, const char * unidad) {
        lv_obj_t * l = lv_label_create(p);
        lv_label_set_text(l, nom);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_30, 0); // Fuente grande y distinguida
        lv_obj_set_style_text_color(l, lv_color_hex(0x111111), 0);
        lv_obj_align(l, LV_ALIGN_TOP_LEFT, 10, y + 8);
        
        lv_obj_t * ta = lv_textarea_create(p);
        lv_obj_set_size(ta, 110, 48);
        lv_obj_align(ta, LV_ALIGN_TOP_RIGHT, -45, y);
        lv_textarea_set_one_line(ta, true);
        lv_obj_set_style_text_font(ta, &lv_font_montserrat_30, 0); // Fuente destacada en el input
        
        char buf[16];
        HMI_Lock();
        uint16_t cur_val = HMI.plc_regs[reg];
        HMI_Unlock();
        snprintf(buf, sizeof(buf), "%d", cur_val);
        lv_textarea_set_text(ta, buf);
        lv_obj_add_event_cb(ta, textarea_param_event_cb, LV_EVENT_ALL, (void *)(uintptr_t)reg);

        // Etiqueta de unidad fija al lado
        lv_obj_t * lbl_unidad = lv_label_create(p);
        lv_label_set_text(lbl_unidad, unidad);
        lv_obj_set_style_text_font(lbl_unidad, &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(lbl_unidad, lv_color_hex(0x333333), 0);
        lv_obj_align_to(lbl_unidad, ta, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    };

    // Panel 1: Temperaturas (Setpoints con °C) - Ancho 500, Alto 400
    lv_obj_t * p_temp = crear_panel("TEMPERATURAS", 500, 400, 40, 15);
    crear_param(p_temp, "Setpoint ZONA1:", 65, MB_SETPOINTZ1, "°C");
    crear_param(p_temp, "Setpoint ZONA2:", 150, MB_SETPOINTZ2, "°C");
    crear_param(p_temp, "Setpoint ZONA3:", 235, MB_SETPOINTZ3, "°C");
    
    // Panel 2: Tiempos fijos del ciclo (Setpoints)
    lv_obj_t * p_tiempo = crear_panel("TIEMPOS DE CICLO (s)", 620, 400, 560, 15);
    crear_param(p_tiempo, "INYECCION:", 65, MB_TINYECCION, "s");
    crear_param(p_tiempo, "MOLDEO:", 125, MB_TENFRIAMIENTO, "s");
    crear_param(p_tiempo, "SOSTENIMIENTO:", 185, MB_TSOSTENIMIENTO, "s");
    crear_param(p_tiempo, "AVANCE EXPULSOR:", 245, MB_TEXP_AVANCE, "s");
    crear_param(p_tiempo, "RETROCESO EXPULSOR:", 305, MB_TEXP_RETROCESO, "s");
}