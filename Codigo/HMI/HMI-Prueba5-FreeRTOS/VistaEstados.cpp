#include "VistaEstados.h"
#include "UIManager.h"
#include "HMIData.h"
#include "SharedModbusMap.h"
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>

// =========================================================================
// COLORES INDUSTRIALES - VISTA ESTADOS Y DIAGNOSTICO
// =========================================================================
#define COLOR_FONDO_GRIS        lv_color_hex(0xE0E0E0)
#define COLOR_PANEL_BG          lv_color_hex(0xF4F4F4)
#define COLOR_CARD_BORDER       lv_color_hex(0x0055AA)

#define COLOR_VERDE_ON          lv_color_hex(0x008822)
#define COLOR_VERDE_BRIGHT      lv_color_hex(0x00C833)
#define COLOR_GRIS_OFF          lv_color_hex(0x555555)
#define COLOR_ROJO_ALERTA       lv_color_hex(0xD02020)
#define COLOR_AMBAR_AVISO       lv_color_hex(0xE67E00)
#define COLOR_AZUL_INDUSTRIAL   lv_color_hex(0x0055AA)
#define COLOR_AZUL_OSCURO       lv_color_hex(0x002B4D)
#define COLOR_TEXTO_TITULO      lv_color_hex(0x111111)

// =========================================================================
// ESTRUCTURAS DE WIDGETS
// =========================================================================
struct IndicatorBoxUI {
    lv_obj_t * box;
    lv_obj_t * led;
    lv_obj_t * lbl_nombre;
    lv_obj_t * lbl_valor;
};

#define CANT_DI 10
#define CANT_DO 11

static IndicatorBoxUI di_boxes[CANT_DI];
static IndicatorBoxUI do_boxes[CANT_DO + 1]; // 11 DOs + 1 handshake robot

// Elementos superiores
static lv_obj_t * lbl_est_modo = NULL;
static lv_obj_t * lbl_est_ciclo = NULL;
static lv_obj_t * lbl_est_plc = NULL;
static lv_obj_t * lbl_est_robot = NULL;

// Elemento paso de inyeccion
static lv_obj_t * lbl_est_paso = NULL;

// =========================================================================
// TEXTOS DE DIAGNOSTICO DE PASO SEGUN DiagnosticsManager.cpp DE MMI_S3
// =========================================================================
static const char * obtener_diagnostico_paso_estados(int paso, bool ciclo_activo) {
    if (!ciclo_activo && paso == 0) return "CICLO DETENIDO - Maquina en espera de inicio";
    switch (paso) {
        case 0:  return "[Paso 0] Esperando permiso de cierre (Handshake Robot)";
        case 1:  return "[Paso 1] Iniciando cierre de matriz";
        case 2:  return "[Paso 2] Matriz cerrada. Esperando presion final";
        case 3:  return "[Paso 3] Bloqueo completado. Tiempo de espera en curso";
        case 4:  return "[Paso 4] Avanzando unidad inyeccion (Carro adelante)";
        case 5:  return "[Paso 5] Inyectando material en cavidad...";
        case 6:  return "[Paso 6] Fase de Sostenimiento iniciada";
        case 7:  return "[Paso 7] Carga de material nuevo y tiempo de enfriamiento";
        case 8:  return "[Paso 8] Iniciando apertura de matriz";
        case 9:  return "[Paso 9] Robot ordena: Expulsor Adelante";
        case 10: return "[Paso 10] Expulsando pieza (Temporizado)...";
        case 11: return "[Paso 11] Robot ordena: Expulsor Atras";
        case 12: return "[Paso 12] Retrayendo expulsor. Finalizando ciclo";
        default: return "Estado de paso desconocido";
    }
}

// Nombres descriptivos de las 10 Entradas Digitales segun Config.h / InputsManager.cpp
static const char * nombres_di[CANT_DI] = {
    "DI 0: Lim. Apertura",
    "DI 1: Lim. Cierre",
    "DI 2: Carro Atras",
    "DI 3: Carro Adelante",
    "DI 4: Lim. Husillo",
    "DI 5: Guarda 1 (NC)",
    "DI 6: Guarda 2 (NC)",
    "DI 7: Marcha Motor",
    "DI 8: Parada Motor",
    "DI 9: E-STOP (NC)"
};

// Nombres descriptivos de las 11 Salidas Digitales segun Config.h / OutputsManager.cpp
static const char * nombres_do[CANT_DO + 1] = {
    "DO 0: Apertura Prensa",
    "DO 1: Cierre Prensa",
    "DO 2: Carro Atras",
    "DO 3: Carro Adelante",
    "DO 4: Inyeccion Husillo",
    "DO 5: Motor Carga Mat.",
    "DO 6: Presion General",
    "DO 7: Motor Principal",
    "DO 8: Rele Sirena/Baliza",
    "DO 9: Expulsor Adelante",
    "DO 10: Expulsor Atras",
    "ROBOT: Permiso Cierre"
};

void dibujar_vista_estados() {
    limpiar_workspace();

    if (workspace == NULL || !lv_obj_is_valid(workspace)) return;

    // =========================================================================
    // 1. BARRA SUPERIOR DE ESTADO GENERAL Y COMUNICACIONES (Y: 10, H: 45, W: 1200)
    // =========================================================================
    lv_obj_t * pnl_top = lv_obj_create(workspace);
    lv_obj_set_size(pnl_top, 1200, 45);
    lv_obj_set_pos(pnl_top, 30, 1);
    lv_obj_remove_flag(pnl_top, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(pnl_top, 6, 0);
    lv_obj_set_style_bg_color(pnl_top, lv_color_hex(0x222222), 0);
    lv_obj_set_style_border_width(pnl_top, 0, 0);
    lv_obj_set_style_pad_hor(pnl_top, 25, 0);
    lv_obj_set_style_pad_ver(pnl_top, 0, 0);
    lv_obj_set_flex_flow(pnl_top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pnl_top, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lbl_est_modo = lv_label_create(pnl_top);
    lv_label_set_text(lbl_est_modo, "MODO: AUTOMATICO");
    lv_obj_set_style_text_color(lbl_est_modo, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_est_modo, &lv_font_montserrat_18, 0);

    lbl_est_ciclo = lv_label_create(pnl_top);
    lv_label_set_text(lbl_est_ciclo, "CICLO: DETENIDO");
    lv_obj_set_style_text_color(lbl_est_ciclo, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(lbl_est_ciclo, &lv_font_montserrat_18, 0);

    lbl_est_plc = lv_label_create(pnl_top);
    lv_label_set_text(lbl_est_plc, "PLC S3: ONLINE [OK]");
    lv_obj_set_style_text_color(lbl_est_plc, COLOR_VERDE_BRIGHT, 0);
    lv_obj_set_style_text_font(lbl_est_plc, &lv_font_montserrat_18, 0);

    lbl_est_robot = lv_label_create(pnl_top);
    lv_label_set_text(lbl_est_robot, "ROBOT EXTRACTOR: LISTO");
    lv_obj_set_style_text_color(lbl_est_robot, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(lbl_est_robot, &lv_font_montserrat_18, 0);

    // =========================================================================
    // 2. PASO ACTUAL DEL CICLO DE INYECCION (Y: 62, H: 60, W: 1200)
    // =========================================================================
    lv_obj_t * box_paso = lv_obj_create(workspace);
    lv_obj_set_size(box_paso, 1200, 60);
    lv_obj_set_pos(box_paso, 30, 55);
    lv_obj_remove_flag(box_paso, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(box_paso, 6, 0);
    lv_obj_set_style_bg_color(box_paso, COLOR_AZUL_OSCURO, 0);
    lv_obj_set_style_border_color(box_paso, COLOR_AZUL_INDUSTRIAL, 0);
    lv_obj_set_style_border_width(box_paso, 2, 0);

    lbl_est_paso = lv_label_create(box_paso);
    lv_label_set_text(lbl_est_paso, "[Paso 0] Esperando permiso de cierre (Handshake Robot)");
    lv_obj_set_style_text_font(lbl_est_paso, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl_est_paso, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_est_paso);

    // =========================================================================
    // 3. PANEL ENTRADAS DIGITALES - SENSORES (Y: 130, H: 195, W: 1200)
    // =========================================================================
    lv_obj_t * pnl_di = lv_obj_create(workspace);
    lv_obj_set_size(pnl_di, 1200, 195);
    lv_obj_set_pos(pnl_di, 30, 125);
    lv_obj_remove_flag(pnl_di, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(pnl_di, 8, 0);
    lv_obj_set_style_bg_color(pnl_di, COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_color(pnl_di, COLOR_AZUL_INDUSTRIAL, 0);
    lv_obj_set_style_border_width(pnl_di, 2, 0);

    lv_obj_t * lbl_tit_di = lv_label_create(pnl_di);
    lv_label_set_text(lbl_tit_di, "ENTRADAS DIGITALES (SENSORES Y FINES DE CARRERA - MB_SENSORES)");
    lv_obj_set_style_text_font(lbl_tit_di, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_tit_di, COLOR_AZUL_INDUSTRIAL, 0);
    lv_obj_set_pos(lbl_tit_di, 12, 1);

    // 10 cajas: 2 filas x 5 columnas
    const int x_di_cols[5] = {12, 246, 480, 714, 948};
    const int y_di_rows[2] = {36, 108};
    const int w_di = 224;
    const int h_di = 60;

    for (int i = 0; i < CANT_DI; i++) {
        int c = i % 5;
        int r = i / 5;

        di_boxes[i].box = lv_obj_create(pnl_di);
        lv_obj_set_size(di_boxes[i].box, w_di, h_di);
        lv_obj_set_pos(di_boxes[i].box, x_di_cols[c], y_di_rows[r]);
        lv_obj_remove_flag(di_boxes[i].box, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(di_boxes[i].box, 5, 0);
        lv_obj_set_style_bg_color(di_boxes[i].box, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_color(di_boxes[i].box, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_border_width(di_boxes[i].box, 2, 0);
        lv_obj_set_style_pad_all(di_boxes[i].box, 0, 0);

        // LED indicador
        di_boxes[i].led = lv_obj_create(di_boxes[i].box);
        lv_obj_set_size(di_boxes[i].led, 28, 28);
        lv_obj_align(di_boxes[i].led, LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_remove_flag(di_boxes[i].led, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(di_boxes[i].led, 14, 0); // Redondo
        lv_obj_set_style_bg_color(di_boxes[i].led, COLOR_GRIS_OFF, 0);
        lv_obj_set_style_border_width(di_boxes[i].led, 0, 0);

        // Nombre de la entrada
        di_boxes[i].lbl_nombre = lv_label_create(di_boxes[i].box);
        lv_label_set_text(di_boxes[i].lbl_nombre, nombres_di[i]);
        lv_obj_set_style_text_color(di_boxes[i].lbl_nombre, COLOR_TEXTO_TITULO, 0);
        lv_obj_set_style_text_font(di_boxes[i].lbl_nombre, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(di_boxes[i].lbl_nombre, 40, 8);

        // Valor / Estado
        di_boxes[i].lbl_valor = lv_label_create(di_boxes[i].box);
        lv_label_set_text(di_boxes[i].lbl_valor, "INACTIVO");
        lv_obj_set_style_text_color(di_boxes[i].lbl_valor, COLOR_GRIS_OFF, 0);
        lv_obj_set_style_text_font(di_boxes[i].lbl_valor, &lv_font_montserrat_16, 0);
        lv_obj_set_pos(di_boxes[i].lbl_valor, 40, 30);
    }

    // =========================================================================
    // 4. PANEL SALIDAS DIGITALES - ACTUADORES (Y: 335, H: 250, W: 1200)
    // =========================================================================
    lv_obj_t * pnl_do = lv_obj_create(workspace);
    lv_obj_set_size(pnl_do, 1200, 250);
    lv_obj_set_pos(pnl_do, 30, 330);
    lv_obj_remove_flag(pnl_do, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(pnl_do, 8, 0);
    lv_obj_set_style_bg_color(pnl_do, COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_color(pnl_do, COLOR_AZUL_INDUSTRIAL, 0);
    lv_obj_set_style_border_width(pnl_do, 2, 0);

    lv_obj_t * lbl_tit_do = lv_label_create(pnl_do);
    lv_label_set_text(lbl_tit_do, "SALIDAS DIGITALES (ACTUADORES Y SOLENOIDES - MB_SDIGITALES)");
    lv_obj_set_style_text_font(lbl_tit_do, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_tit_do, COLOR_AZUL_INDUSTRIAL, 0);
    lv_obj_set_pos(lbl_tit_do, 12, 1);

    // 12 cajas: 3 filas x 4 columnas
    const int x_do_cols[4] = {12, 304, 596, 888};
    const int y_do_rows[3] = {34, 98, 162};
    const int w_do = 280;
    const int h_do = 54;

    for (int i = 0; i < CANT_DO + 1; i++) {
        int c = i % 4;
        int r = i / 4;

        do_boxes[i].box = lv_obj_create(pnl_do);
        lv_obj_set_size(do_boxes[i].box, w_do, h_do);
        lv_obj_set_pos(do_boxes[i].box, x_do_cols[c], y_do_rows[r]);
        lv_obj_remove_flag(do_boxes[i].box, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(do_boxes[i].box, 5, 0);
        lv_obj_set_style_bg_color(do_boxes[i].box, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_color(do_boxes[i].box, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_border_width(do_boxes[i].box, 2, 0);
        lv_obj_set_style_pad_all(do_boxes[i].box, 0, 0);

        // LED indicador
        do_boxes[i].led = lv_obj_create(do_boxes[i].box);
        lv_obj_set_size(do_boxes[i].led, 26, 26);
        lv_obj_align(do_boxes[i].led, LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_remove_flag(do_boxes[i].led, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(do_boxes[i].led, 4, 0);
        lv_obj_set_style_bg_color(do_boxes[i].led, COLOR_GRIS_OFF, 0);
        lv_obj_set_style_border_width(do_boxes[i].led, 0, 0);

        // Nombre de la salida
        do_boxes[i].lbl_nombre = lv_label_create(do_boxes[i].box);
        lv_label_set_text(do_boxes[i].lbl_nombre, nombres_do[i]);
        lv_obj_set_style_text_color(do_boxes[i].lbl_nombre, COLOR_TEXTO_TITULO, 0);
        lv_obj_set_style_text_font(do_boxes[i].lbl_nombre, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(do_boxes[i].lbl_nombre, 38, 6);

        // Valor / Estado
        do_boxes[i].lbl_valor = lv_label_create(do_boxes[i].box);
        lv_label_set_text(do_boxes[i].lbl_valor, "OFF");
        lv_obj_set_style_text_color(do_boxes[i].lbl_valor, COLOR_GRIS_OFF, 0);
        lv_obj_set_style_text_font(do_boxes[i].lbl_valor, &lv_font_montserrat_16, 0);
        lv_obj_set_pos(do_boxes[i].lbl_valor, 38, 26);
    }

    VistaEstados_Update();
}

void VistaEstados_Update() {
    if (workspace == NULL || !lv_obj_is_valid(workspace)) return;
    if (di_boxes[0].box == NULL || !lv_obj_is_valid(di_boxes[0].box)) return;

    // Snapshot atomico de datos
    HMI_Lock();
    uint16_t sensores   = HMI.plc_regs[MB_SENSORES];
    uint16_t sdigitales = HMI.plc_regs[MB_SDIGITALES];
    uint16_t control    = HMI.plc_regs[MB_CONTROL];
    uint16_t botones    = HMI.plc_regs[MB_HMI_BOTONES];
    uint16_t paso_act   = HMI.plc_regs[MB_PASO_ACTUAL];
    bool plc_online     = HMI.plc_conectado;
    bool robot_online   = HMI.robot_conectado;
    HMI_Unlock();

    // 1. Modos y Estados
    bool modo_manual   = bitRead(botones, 0);
    bool modo_semiauto = bitRead(botones, 1);
    bool modo_auto     = bitRead(botones, 2);
    bool ciclo_en_marcha = (paso_act > 0 && paso_act <= 12);

    if (lbl_est_modo && lv_obj_is_valid(lbl_est_modo)) {
        const char * str_modo = modo_auto ? "MODO: AUTOMATICO" : (modo_semiauto ? "MODO: SEMIAUTO" : (modo_manual ? "MODO: MANUAL" : "MODO: REPOSO"));
        lv_label_set_text(lbl_est_modo, str_modo);
    }

    if (lbl_est_ciclo && lv_obj_is_valid(lbl_est_ciclo)) {
        if (ciclo_en_marcha) {
            lv_label_set_text(lbl_est_ciclo, "CICLO: EN EJECUCION");
            lv_obj_set_style_text_color(lbl_est_ciclo, COLOR_VERDE_BRIGHT, 0);
        } else {
            lv_label_set_text(lbl_est_ciclo, "CICLO: DETENIDO");
            lv_obj_set_style_text_color(lbl_est_ciclo, lv_color_hex(0xAAAAAA), 0);
        }
    }

    if (lbl_est_plc && lv_obj_is_valid(lbl_est_plc)) {
        if (plc_online) {
            lv_label_set_text(lbl_est_plc, "PLC S3: ONLINE [OK]");
            lv_obj_set_style_text_color(lbl_est_plc, COLOR_VERDE_BRIGHT, 0);
        } else {
            lv_label_set_text(lbl_est_plc, "PLC S3: DESCONECTADO");
            lv_obj_set_style_text_color(lbl_est_plc, COLOR_ROJO_ALERTA, 0);
        }
    }

    if (lbl_est_robot && lv_obj_is_valid(lbl_est_robot)) {
        if (robot_online) {
            uint16_t st = HMI.robot_regs[MB_ROBOT_ESTADO];
            const char* st_nom = "LISTO EN REPOSO";
            switch(st) {
                case ROBOT_HOMING_EN_CURSO:    st_nom = "HOMING EN CURSO"; break;
                case ROBOT_LISTO_EN_HOME:      st_nom = "LISTO EN REPOSO"; break;
                case ROBOT_ENTRANDO_PRENSA:    st_nom = "ENTRANDO A PRENSA"; break;
                case ROBOT_EN_POSICION_AGARRE: st_nom = "POSICION AGARRE"; break;
                case ROBOT_SUJETANDO_PIEZA:    st_nom = "SUJETANDO PIEZA"; break;
                case ROBOT_SALIENDO_PRENSA:    st_nom = "SALIENDO DE PRENSA"; break;
                case ROBOT_ZONA_SEGURA:        st_nom = "EN ZONA SEGURA"; break;
                case ROBOT_DESCARGANDO:        st_nom = "DESCARGANDO PIEZA"; break;
                case ROBOT_ALARMA_ERROR:       st_nom = "!ALARMA ROBOT!"; break;
                default:                       st_nom = "CONECTADO"; break;
            }
            char buf_rb[64];
            snprintf(buf_rb, sizeof(buf_rb), "ROBOT EXTRACTOR: %s (Pz: %d)", st_nom, HMI.robot_regs[MB_ROBOT_TOTAL_CICLOS]);
            lv_label_set_text(lbl_est_robot, buf_rb);
            lv_obj_set_style_text_color(lbl_est_robot, (st == ROBOT_ALARMA_ERROR) ? COLOR_ROJO_ALERTA : COLOR_VERDE_BRIGHT, 0);
        } else {
            lv_label_set_text(lbl_est_robot, "ROBOT EXTRACTOR: DESCONECTADO");
            lv_obj_set_style_text_color(lbl_est_robot, lv_color_hex(0xCCCCCC), 0);
        }
    }

    // 2. Paso actual del proceso de inyeccion
    if (lbl_est_paso && lv_obj_is_valid(lbl_est_paso)) {
        lv_label_set_text(lbl_est_paso, obtener_diagnostico_paso_estados(paso_act, ciclo_en_marcha));
    }

    // 3. Entradas Digitales (10)
    for (int i = 0; i < CANT_DI; i++) {
        bool bit_val = (bitRead(sensores, i) == 1);
        
        if (di_boxes[i].box && lv_obj_is_valid(di_boxes[i].box)) {
            // Fines de carrera NC de seguridad: Guardas (5, 6) y E-Stop (9)
            // Bit 1 = Circuito cerrado / Seguro (Verde OK)
            // Bit 0 = Circuito abierto / Alerta (Rojo Alarma)
            if (i == 5 || i == 6 || i == 9) {
                if (bit_val) {
                    lv_obj_set_style_bg_color(di_boxes[i].led, COLOR_VERDE_ON, 0);
                    lv_obj_set_style_border_color(di_boxes[i].box, COLOR_VERDE_ON, 0);
                    lv_label_set_text(di_boxes[i].lbl_valor, "CERRADO [OK]");
                    lv_obj_set_style_text_color(di_boxes[i].lbl_valor, COLOR_VERDE_ON, 0);
                } else {
                    lv_obj_set_style_bg_color(di_boxes[i].led, COLOR_ROJO_ALERTA, 0);
                    lv_obj_set_style_border_color(di_boxes[i].box, COLOR_ROJO_ALERTA, 0);
                    lv_label_set_text(di_boxes[i].lbl_valor, "ABIERTO [ALERTA]");
                    lv_obj_set_style_text_color(di_boxes[i].lbl_valor, COLOR_ROJO_ALERTA, 0);
                }
            } else {
                // Sensores y pulsadores estandard (NO)
                if (bit_val) {
                    lv_obj_set_style_bg_color(di_boxes[i].led, COLOR_VERDE_ON, 0);
                    lv_obj_set_style_border_color(di_boxes[i].box, COLOR_VERDE_ON, 0);
                    lv_label_set_text(di_boxes[i].lbl_valor, "ACTIVO (ON)");
                    lv_obj_set_style_text_color(di_boxes[i].lbl_valor, COLOR_VERDE_ON, 0);
                } else {
                    lv_obj_set_style_bg_color(di_boxes[i].led, COLOR_GRIS_OFF, 0);
                    lv_obj_set_style_border_color(di_boxes[i].box, lv_color_hex(0xCCCCCC), 0);
                    lv_label_set_text(di_boxes[i].lbl_valor, "INACTIVO (OFF)");
                    lv_obj_set_style_text_color(di_boxes[i].lbl_valor, COLOR_GRIS_OFF, 0);
                }
            }
        }
    }

    // 4. Salidas Digitales (11 + 1)
    for (int i = 0; i < CANT_DO; i++) {
        bool out_val = (bitRead(sdigitales, i) == 1);

        if (do_boxes[i].box && lv_obj_is_valid(do_boxes[i].box)) {
            if (out_val) {
                // Alarma / Sirena en bit 8 se destaca en ambar
                lv_color_t color_act = (i == 8) ? COLOR_AMBAR_AVISO : COLOR_VERDE_ON;
                lv_obj_set_style_bg_color(do_boxes[i].led, color_act, 0);
                lv_obj_set_style_border_color(do_boxes[i].box, color_act, 0);
                lv_label_set_text(do_boxes[i].lbl_valor, "ACTIVA [ON]");
                lv_obj_set_style_text_color(do_boxes[i].lbl_valor, color_act, 0);
            } else {
                lv_obj_set_style_bg_color(do_boxes[i].led, COLOR_GRIS_OFF, 0);
                lv_obj_set_style_border_color(do_boxes[i].box, lv_color_hex(0xCCCCCC), 0);
                lv_label_set_text(do_boxes[i].lbl_valor, "APAGADA [OFF]");
                lv_obj_set_style_text_color(do_boxes[i].lbl_valor, COLOR_GRIS_OFF, 0);
            }
        }
    }

    // Elemento 12: Handshake Robot (Bit 4 de MB_CONTROL)
    if (do_boxes[CANT_DO].box && lv_obj_is_valid(do_boxes[CANT_DO].box)) {
        bool robot_permite = (bitRead(HMI.robot_regs[MB_ROBOT_HANDSHAKE], 3) == 1) || (bitRead(control, 4) == 1) || (paso_act > 0);
        if (robot_permite) {
            lv_obj_set_style_bg_color(do_boxes[CANT_DO].led, COLOR_VERDE_ON, 0);
            lv_obj_set_style_border_color(do_boxes[CANT_DO].box, COLOR_VERDE_ON, 0);
            lv_label_set_text(do_boxes[CANT_DO].lbl_valor, "HABILITADO");
            lv_obj_set_style_text_color(do_boxes[CANT_DO].lbl_valor, COLOR_VERDE_ON, 0);
        } else {
            lv_obj_set_style_bg_color(do_boxes[CANT_DO].led, COLOR_GRIS_OFF, 0);
            lv_obj_set_style_border_color(do_boxes[CANT_DO].box, lv_color_hex(0xCCCCCC), 0);
            lv_label_set_text(do_boxes[CANT_DO].lbl_valor, "ESPERA");
            lv_obj_set_style_text_color(do_boxes[CANT_DO].lbl_valor, COLOR_GRIS_OFF, 0);
        }
    }
}
