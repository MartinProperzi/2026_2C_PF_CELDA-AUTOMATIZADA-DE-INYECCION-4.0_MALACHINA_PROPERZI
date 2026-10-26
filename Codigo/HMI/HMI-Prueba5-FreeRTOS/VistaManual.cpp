#include "UIManager.h"
#include "HMIData.h"
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>

// Colores industriales para la interfaz manual
#define COLOR_FONDO_GRIS      lv_color_hex(0xE0E0E0)
#define COLOR_INACTIVO        lv_color_hex(0xD0D0D0)      // Gris claro industrial para los pulsadores inactivos
#define COLOR_VERDE_ACTIVO    lv_color_008000 ? lv_color_hex(0x008000) : lv_color_hex(0x008000) // Verde intenso para el activo

#define COLOR_MODO_INACTIVO   lv_color_hex(0x404040)      // Gris oscuro para modos inactivos
#define COLOR_MOTOR_INACTIVO  lv_color_hex(0x606060)      // Gris un poco más claro para motor inactivo

#define COLOR_BORDE_AZUL      lv_color_hex(0x0055AA)      
#define COLOR_BORDE_GRIS      lv_color_hex(0x888888)

// Punteros globales locales para referencia
static lv_obj_t * btn_modo_manual = NULL;
static lv_obj_t * btn_modo_semi = NULL;
static lv_obj_t * btn_modo_auto = NULL;
static lv_obj_t * btn_motor_on = NULL;
static lv_obj_t * btn_motor_off = NULL;
static lv_obj_t * lbl_motor_on = NULL;
static lv_obj_t * lbl_motor_off = NULL;

// Prototipos internos
void VistaManual_Update();
static void actualizar_boton_estado(lv_obj_t * btn, bool activo, bool es_motor);

// 1. Callback para pulsadores momentáneos (Movimientos hidráulicos)
static void manual_pulsador_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_current_target(e);
    int bit_index = (int)(uintptr_t)lv_event_get_user_data(e);

    if (bit_index >= 0) {
        if (code == LV_EVENT_PRESSED) {
            HMI_Lock();
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], bit_index, 1);
            HMI_Unlock();
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF00), 0);
            Serial.printf("[HMI TOUCH] Pulsador Bit %d -> PRESIONADO\n", bit_index);
        } 
        else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
            HMI_Lock();
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], bit_index, 0);
            HMI_Unlock();
            lv_obj_set_style_bg_color(btn, COLOR_INACTIVO, 0);
            Serial.printf("[HMI TOUCH] Pulsador Bit %d -> LIBERADO\n", bit_index);
        }
    }
}

// 2. Callback para botones enclavados / selectores (Modos de Operación)
// Callback para botones manuales del Robot en la pesta�a Manual
static void robot_manual_cmd_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_current_target(e);
    int bit_cmd = (int)(uintptr_t)lv_event_get_user_data(e);
    if (code == LV_EVENT_PRESSED) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF00), 0);
    } else if (code == LV_EVENT_RELEASED) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x004080), 0);
        HMI_EnviarComandoRobot(1 << bit_cmd);
        Serial.printf("[HMI TOUCH] Boton Robot (Bit %d) pulsado en pesta�a MANUAL\n", bit_cmd);
    } else if (code == LV_EVENT_PRESS_LOST) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x004080), 0);
    }
}

static void manual_enclavado_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    int tipo_selector = (int)(uintptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
        HMI_Lock();
        if (tipo_selector == 0) { // Modo Manual (Bit 0)
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 0, 1);
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 1, 0);
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 2, 0);
        }
        else if (tipo_selector == 1) { // Modo Semiautomático (Bit 1)
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 0, 0);
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 1, 1);
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 2, 0);
        }
        else if (tipo_selector == 2) { // Modo Automático (Bit 2)
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 0, 0);
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 1, 0);
            bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 2, 1);
        }
        uint16_t b_snap = HMI.plc_regs[MB_HMI_BOTONES];
        HMI_Unlock();

        // Refresco visual inmediato de los selectores de modo
        if (btn_modo_manual && btn_modo_semi && btn_modo_auto) {
            actualizar_boton_estado(btn_modo_manual, (b_snap & (1 << 0)) != 0, false);
            actualizar_boton_estado(btn_modo_semi,   (b_snap & (1 << 1)) != 0, false);
            actualizar_boton_estado(btn_modo_auto,   (b_snap & (1 << 2)) != 0, false);
        }
    }
}

void dibujar_vista_manual() {
    limpiar_workspace();

    if (workspace == NULL || !lv_obj_is_valid(workspace)) return;

    static int32_t col_dsc[] = {220, 220, 220, 220, 220, LV_GRID_TEMPLATE_LAST};
    static int32_t row_dsc[] = {160, 160, 160, LV_GRID_TEMPLATE_LAST};

    lv_obj_t * grid = lv_obj_create(workspace);
    lv_obj_set_size(grid, lv_pct(98), lv_pct(98));
    lv_obj_center(grid);
    lv_obj_set_style_bg_color(grid, COLOR_FONDO_GRIS, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    // Función auxiliar para pulsadores de movimiento (con borde más grueso y centrado)
    auto crear_pulsador = [](lv_obj_t * p, int c, int f, const char * sim, const char * nom, int bit_idx) -> lv_obj_t * {
        lv_obj_t * btn = lv_button_create(p);
        lv_obj_set_size(btn, 205, 145);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(btn, COLOR_INACTIVO, 0);
        lv_obj_set_style_border_color(btn, COLOR_BORDE_AZUL, 0);
        lv_obj_set_style_border_width(btn, 6, 0); // Borde engrosado a 6 píxeles

        // Icono / Símbolo centrado
        lv_obj_t * lbl_sim = lv_label_create(btn);
        lv_label_set_text(lbl_sim, sim);
        lv_obj_remove_flag(lbl_sim, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(lbl_sim, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_font(lbl_sim, &lv_font_montserrat_44, 0); 
        lv_obj_align(lbl_sim, LV_ALIGN_CENTER, 0, -18);

        // Texto inferior perfectamente centrado
        lv_obj_t * lbl_nom = lv_label_create(btn);
        lv_label_set_text(lbl_nom, nom);
        lv_obj_remove_flag(lbl_nom, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(lbl_nom, lv_color_hex(0x222222), 0);
        lv_obj_set_style_text_font(lbl_nom, &lv_font_montserrat_20, 0); 
        lv_obj_set_style_text_align(lbl_nom, LV_TEXT_ALIGN_CENTER, 0); // Centrado de texto multilínea
        lv_obj_align(lbl_nom, LV_ALIGN_BOTTOM_MID, 0, -10);

        lv_obj_add_event_cb(btn, manual_pulsador_event_cb, LV_EVENT_ALL, (void *)(uintptr_t)bit_idx);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, c, 1, LV_GRID_ALIGN_CENTER, f, 1);
        return btn;
    };

    auto crear_enclavado = [](lv_obj_t * p, int c, int f, const char * nom, int tipo_sel, bool es_motor) -> lv_obj_t * {
        lv_obj_t * btn = lv_button_create(p);
        lv_obj_set_size(btn, 205, 145);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_color_t color_bg = es_motor ? COLOR_MOTOR_INACTIVO : COLOR_MODO_INACTIVO;
        lv_obj_set_style_bg_color(btn, color_bg, 0);
        lv_obj_set_style_border_color(btn, color_bg, 0);
        lv_obj_set_style_border_width(btn, 3, 0);

        lv_obj_t * lbl_nom = lv_label_create(btn);
        lv_label_set_text(lbl_nom, nom);
        lv_obj_remove_flag(lbl_nom, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(lbl_nom, lv_color_hex(0xFFFFFF), 0); 
        lv_obj_set_style_text_font(lbl_nom, &lv_font_montserrat_26, 0); 
        lv_obj_set_style_text_align(lbl_nom, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl_nom, LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_event_cb(btn, manual_enclavado_event_cb, LV_EVENT_PRESSED, (void *)(uintptr_t)tipo_sel);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, c, 1, LV_GRID_ALIGN_CENTER, f, 1);
        return btn;
    };

    auto crear_indicador_motor = [](lv_obj_t * p, int c, int f, const char * nom, lv_obj_t ** out_lbl) -> lv_obj_t * {
        lv_obj_t * btn = lv_button_create(p);
        lv_obj_set_size(btn, 205, 145);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_CLICKABLE); // Solo indicador visual (no reacciona al touch)
        
        lv_obj_set_style_bg_color(btn, COLOR_MOTOR_INACTIVO, 0);
        lv_obj_set_style_border_color(btn, COLOR_BORDE_GRIS, 0);
        lv_obj_set_style_border_width(btn, 3, 0);

        lv_obj_t * lbl = lv_label_create(btn);
        lv_label_set_text(lbl, nom);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0); 
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0); 
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

        if (out_lbl) *out_lbl = lbl;

        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, c, 1, LV_GRID_ALIGN_CENTER, f, 1);
        return btn;
    };

    auto crear_vacio = [](lv_obj_t * p, int c, int f) {
        lv_obj_t * btn = lv_button_create(p);
        lv_obj_set_size(btn, 205, 145);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xD0D0D0), 0);
        lv_obj_set_style_border_color(btn, COLOR_BORDE_GRIS, 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_add_state(btn, LV_STATE_DISABLED);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, c, 1, LV_GRID_ALIGN_CENTER, f, 1);
    };

    // --- FILA 0 ---
    crear_pulsador(grid, 0, 0, LV_SYMBOL_RIGHT, "CIERRE", 8);
    crear_pulsador(grid, 1, 0, LV_SYMBOL_LEFT, "APERTURA", 9);
    btn_modo_manual = crear_enclavado(grid, 2, 0, "MODO\nMANUAL", 0, false);
    crear_pulsador(grid, 3, 0, LV_SYMBOL_LEFT, "INYECCION", 7);
    crear_pulsador(grid, 4, 0, LV_SYMBOL_RIGHT, "CARGA", 6);

    // --- FILA 1 ---
    crear_pulsador(grid, 0, 1, LV_SYMBOL_RIGHT, "EXPULSOR\nADELANTE", 12); 
    crear_pulsador(grid, 1, 1, LV_SYMBOL_LEFT, "EXPULSOR\nATRAS", 13);
    btn_modo_semi = crear_enclavado(grid, 2, 1, "MODO\nSEMI\nAUTOMATICO", 1, false);
    crear_pulsador(grid, 3, 1, LV_SYMBOL_RIGHT, "UNIDAD\nATRAS", 10);
    crear_pulsador(grid, 4, 1, LV_SYMBOL_LEFT, "UNIDAD\nADELANTE.", 11);

    // --- FILA 2 ---
    btn_motor_on  = crear_indicador_motor(grid, 0, 2, "MOTOR\nON", &lbl_motor_on);
    btn_motor_off = crear_indicador_motor(grid, 1, 2, "MOTOR\nOFF", &lbl_motor_off);
    btn_modo_auto = crear_enclavado(grid, 2, 2, "MODO\nAUTOMATICO", 2, false);
    auto crear_boton_robot_manual = [](lv_obj_t * p, int c, int f, const char * sim, const char * nom, int bit_cmd) -> lv_obj_t * {
        lv_obj_t * btn = lv_button_create(p);
        lv_obj_set_size(btn, 205, 145);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x004080), 0);
        lv_obj_set_style_border_color(btn, COLOR_BORDE_AZUL, 0);
        lv_obj_set_style_border_width(btn, 6, 0);
        lv_obj_t * lbl_sim = lv_label_create(btn);
        lv_label_set_text(lbl_sim, sim);
        lv_obj_remove_flag(lbl_sim, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(lbl_sim, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl_sim, &lv_font_montserrat_44, 0);
        lv_obj_align(lbl_sim, LV_ALIGN_CENTER, 0, -18);
        lv_obj_t * lbl_nom = lv_label_create(btn);
        lv_label_set_text(lbl_nom, nom);
        lv_obj_remove_flag(lbl_nom, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(lbl_nom, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl_nom, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_align(lbl_nom, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl_nom, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_add_event_cb(btn, robot_manual_cmd_event_cb, LV_EVENT_ALL, (void *)(uintptr_t)bit_cmd);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, c, 1, LV_GRID_ALIGN_CENTER, f, 1);
        return btn;
    };
    crear_boton_robot_manual(grid, 3, 2, LV_SYMBOL_HOME, "ROBOT\nHOMING", 0);
    crear_boton_robot_manual(grid, 4, 2, LV_SYMBOL_PLAY, "ROBOT\nEXTRAER", 1);

    VistaManual_Update();
}

static void actualizar_boton_estado(lv_obj_t * btn, bool activo, bool es_motor) {
    if (!btn || !lv_obj_is_valid(btn)) return;
    
    if (activo) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x008000), 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x008000), 0);
    } else {
        lv_color_t color_inactivo = es_motor ? COLOR_MOTOR_INACTIVO : COLOR_MODO_INACTIVO;
        lv_obj_set_style_bg_color(btn, color_inactivo, 0);
        lv_obj_set_style_border_color(btn, color_inactivo, 0);
    }
}

void VistaManual_Update() {
    HMI_Lock();
    uint16_t b_snap = HMI.plc_regs[MB_HMI_BOTONES];
    // El estado del motor se lee directamente de la salida física del PLC (MB_SDIGITALES Bit 7)
    bool motor_activo = (bitRead(HMI.plc_regs[MB_SDIGITALES], 7) == 1);
    HMI_Unlock();

    if (btn_modo_manual && lv_obj_is_valid(btn_modo_manual)) {
        actualizar_boton_estado(btn_modo_manual, (b_snap & (1 << 0)) != 0, false);
    }
    if (btn_modo_semi && lv_obj_is_valid(btn_modo_semi)) {
        actualizar_boton_estado(btn_modo_semi, (b_snap & (1 << 1)) != 0, false);
    }
    if (btn_modo_auto && lv_obj_is_valid(btn_modo_auto)) {
        actualizar_boton_estado(btn_modo_auto, (b_snap & (1 << 2)) != 0, false);
    }

    // Actualización de lámparas indicadoras del estado real del motor
    if (btn_motor_on && lv_obj_is_valid(btn_motor_on)) {
        if (motor_activo) {
            lv_obj_set_style_bg_color(btn_motor_on, lv_color_hex(0x00A000), 0); // Verde en marcha
            lv_obj_set_style_border_color(btn_motor_on, lv_color_hex(0x00FF00), 0);
            lv_obj_set_style_border_width(btn_motor_on, 5, 0);
            if (lbl_motor_on && lv_obj_is_valid(lbl_motor_on)) {
                lv_obj_set_style_text_color(lbl_motor_on, lv_color_hex(0xFFFFFF), 0);
            }
        } else {
            lv_obj_set_style_bg_color(btn_motor_on, COLOR_MOTOR_INACTIVO, 0); // Gris inactivo
            lv_obj_set_style_border_color(btn_motor_on, COLOR_BORDE_GRIS, 0);
            lv_obj_set_style_border_width(btn_motor_on, 3, 0);
            if (lbl_motor_on && lv_obj_is_valid(lbl_motor_on)) {
                lv_obj_set_style_text_color(lbl_motor_on, lv_color_hex(0x888888), 0);
            }
        }
    }
    if (btn_motor_off && lv_obj_is_valid(btn_motor_off)) {
        if (!motor_activo) {
            lv_obj_set_style_bg_color(btn_motor_off, lv_color_hex(0xB02020), 0); // Rojo detenido
            lv_obj_set_style_border_color(btn_motor_off, lv_color_hex(0xFF3030), 0);
            lv_obj_set_style_border_width(btn_motor_off, 5, 0);
            if (lbl_motor_off && lv_obj_is_valid(lbl_motor_off)) {
                lv_obj_set_style_text_color(lbl_motor_off, lv_color_hex(0xFFFFFF), 0);
            }
        } else {
            lv_obj_set_style_bg_color(btn_motor_off, COLOR_MOTOR_INACTIVO, 0); // Gris inactivo
            lv_obj_set_style_border_color(btn_motor_off, COLOR_BORDE_GRIS, 0);
            lv_obj_set_style_border_width(btn_motor_off, 3, 0);
            if (lbl_motor_off && lv_obj_is_valid(lbl_motor_off)) {
                lv_obj_set_style_text_color(lbl_motor_off, lv_color_hex(0x888888), 0);
            }
        }
    }
}
