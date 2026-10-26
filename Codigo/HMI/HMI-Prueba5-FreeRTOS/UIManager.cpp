#include "UIManager.h"
#include "HMIData.h"
#include "VistaAlarmas.h"
#include "VistaEstados.h"
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>

lv_obj_t * workspace;
lv_obj_t * lbl_status_general;
static lv_obj_t * lbl_live_piezas;

// Enumerador para rastrear la vista actual de la pantalla (5 pestañas)
enum VistaActiva { VISTA_SECUENCIA, VISTA_PARAMETROS, VISTA_MANUAL, VISTA_ESTADOS, VISTA_ALARMAS };
static VistaActiva vista_actual = VISTA_SECUENCIA;

// Prototipos de vistas
void dibujar_vista_secuencia();
void dibujar_vista_parametros();
void dibujar_vista_manual();
void VistaSecuencia_Update();
void VistaManual_Update();

void limpiar_workspace() {
    if (workspace != NULL && lv_obj_is_valid(workspace)) {
        lv_obj_clean(workspace);
    }
}

// Callback de navegación entre pestañas (F1 a F5)
static void fkey_event_cb(lv_event_t * e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    
    if (id == 0) { 
        vista_actual = VISTA_SECUENCIA;
        dibujar_vista_secuencia(); 
        lv_label_set_text(lbl_status_general, "CICLO"); 
    }
    else if (id == 1) { 
        vista_actual = VISTA_PARAMETROS;
        dibujar_vista_parametros(); 
        lv_label_set_text(lbl_status_general, "PARAMETROS"); 
    }
    else if (id == 2) { 
        vista_actual = VISTA_MANUAL;
        dibujar_vista_manual(); 
        lv_label_set_text(lbl_status_general, "MANUAL"); 
    }
    else if (id == 3) { 
        vista_actual = VISTA_ESTADOS;
        dibujar_vista_estados(); 
        lv_label_set_text(lbl_status_general, "ESTADOS"); 
    }
    else if (id == 4) { 
        vista_actual = VISTA_ALARMAS;
        dibujar_vista_alarmas(); 
        lv_label_set_text(lbl_status_general, "ALARMAS"); 
    }
}

void UIManager_Init() {
    lv_obj_t * screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xE0E0E0), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    // --- HEADER (Barra Superior Rectangular) ---
    lv_obj_t * header = lv_obj_create(screen);
    lv_obj_set_size(header, lv_pct(100), 75); 
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0); // Estrictamente rectangular
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    // Etiqueta de Estado a la izquierda
    lbl_status_general = lv_label_create(header);
    lv_label_set_text(lbl_status_general, "CICLO");
    lv_obj_set_style_text_color(lbl_status_general, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_status_general, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_status_general, LV_ALIGN_LEFT_MID, 10, 0);

    // Título central: FLUIDMEC H40-60 (Naranja, negrita/destacado, centrado)
    lv_obj_t * lbl_titulo = lv_label_create(header);
    lv_label_set_text(lbl_titulo, "FLUIDMEC HF40-60");
    lv_obj_set_style_text_color(lbl_titulo, lv_color_hex(0xFF6600), 0); // Naranja industrial
    lv_obj_set_style_text_font(lbl_titulo, &lv_font_montserrat_30, 0);
    lv_obj_align(lbl_titulo, LV_ALIGN_CENTER, 0, 0);

    // Etiqueta de Conteo de Piezas a la derecha
    lbl_live_piezas = lv_label_create(header);
    lv_label_set_text(lbl_live_piezas, "PIEZAS: 0");
    lv_obj_set_style_text_color(lbl_live_piezas, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(lbl_live_piezas, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_live_piezas, LV_ALIGN_RIGHT_MID, -10, 0);

    // --- FOOTER (Barra Inferior Rectangular con 5 botones distribuidos) ---
    lv_obj_t * footer = lv_obj_create(screen);
    lv_obj_set_size(footer, lv_pct(100), 105);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_radius(footer, 0, 0); // Estrictamente rectangular
    lv_obj_remove_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const char * fkeys[5] = {"CICLO", "PARAMETROS", "MANUAL", "ESTADOS", "ALARMAS"};
    for (int i = 0; i < 5; i++) {
        lv_obj_t * btn = lv_button_create(footer);
        lv_obj_set_size(btn, 230, 85); // 5 botones de 230px que aprovechan los 1280px con espaciado uniforme
        lv_obj_t * lbl = lv_label_create(btn);
        lv_label_set_text(lbl, fkeys[i]);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, fkey_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }

    // --- WORKSPACE ---
    workspace = lv_obj_create(screen);
    lv_obj_set_size(workspace, lv_pct(100), lv_pct(75));
    lv_obj_set_pos(workspace, 0, 75);
    lv_obj_set_style_bg_color(workspace, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_border_width(workspace, 0, 0);
    lv_obj_remove_flag(workspace, LV_OBJ_FLAG_SCROLLABLE);

    vista_actual = VISTA_SECUENCIA;
    dibujar_vista_secuencia();
}

void UIManager_Update() {
    char buf_p[32];
    HMI_Lock();
    uint16_t piezas = HMI.plc_regs[MB_TOTAL_PIEZAS];
    HMI_Unlock();
    snprintf(buf_p, sizeof(buf_p), "PIEZAS: %d", piezas);
    if (lbl_live_piezas && lv_obj_is_valid(lbl_live_piezas)) {
        lv_label_set_text(lbl_live_piezas, buf_p);
    }

    // Refresco de datos segun la vista activa
    if (vista_actual == VISTA_SECUENCIA) {
        VistaSecuencia_Update();
    } else if (vista_actual == VISTA_MANUAL) {
        VistaManual_Update();
    } else if (vista_actual == VISTA_ESTADOS) {
        VistaEstados_Update();
    } else if (vista_actual == VISTA_ALARMAS) {
        VistaAlarmas_Update();
    }
}