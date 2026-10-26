#include "VistaAlarmas.h"
#include "UIManager.h"
#include "HMIData.h"
#include "SharedModbusMap.h"
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>

// =========================================================================
// COLORES INDUSTRIALES - VISTA ALARMAS
// =========================================================================
#define COLOR_FONDO_GRIS lv_color_hex(0xE0E0E0)
#define COLOR_CARD_OK lv_color_hex(0xFFFFFF)
#define COLOR_CARD_BORDER_OK lv_color_hex(0x88AA88)
#define COLOR_CARD_ALARM lv_color_hex(0xFFEAEA)
#define COLOR_CARD_BORDER_ALARM lv_color_hex(0xD02020)

#define COLOR_VERDE_OK lv_color_hex(0x008822)
#define COLOR_ROJO_ALERTA lv_color_hex(0xD02020)
#define COLOR_AMBAR_AVISO lv_color_hex(0xE67E00)
#define COLOR_AZUL_INDUSTRIAL lv_color_hex(0x0055AA)
#define COLOR_AZUL_OSCURO lv_color_hex(0x002B4D)
#define COLOR_GRIS_OSCURO lv_color_hex(0x333333)
#define COLOR_GRIS_MEDIO lv_color_hex(0x555555)
#define COLOR_TEXTO_TITULO lv_color_hex(0x111111)

// =========================================================================
// ESTRUCTURAS DE LAS 4 ALARMAS CRITICAS DE MMI_S3
// =========================================================================
struct AlarmCardUI {
  lv_obj_t* card;
  lv_obj_t* box_icon;
  lv_obj_t* lbl_icon;
  lv_obj_t* lbl_titulo;
  lv_obj_t* lbl_desc;
  lv_obj_t* badge_estado;
  lv_obj_t* lbl_badge;
};

#define CANT_ALARMAS_S3 4
static AlarmCardUI cards[CANT_ALARMAS_S3];

// Banner superior de estado general
static lv_obj_t* banner_alarmas = NULL;
static lv_obj_t* lbl_banner = NULL;

// Barra Inferior de Control
static lv_obj_t* box_sirena = NULL;
static lv_obj_t* lbl_sirena = NULL;
static lv_obj_t* btn_rearmar = NULL;
static lv_obj_t* box_enlaces = NULL;
static lv_obj_t* lbl_enlace_plc = NULL;
static lv_obj_t* lbl_enlace_robot = NULL;

// Callback para silenciar sirena y rearmar alarmas segun AlarmManager.cpp
static void btn_rearmar_event_cb(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    HMI_Lock();
    bitWrite(HMI.plc_regs[MB_SDIGITALES], 8, 0);  // Apagar salida sirena
    bitWrite(HMI.plc_regs[MB_CONTROL], 8, 0);     // Limpiar falla carga material
    bitWrite(HMI.plc_regs[MB_CONTROL], 9, 0);     // Limpiar falla guarda abierta
    // Conmutación exclusiva a MODO MANUAL (apagar modos semi y auto)
    bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 0, 1);  // Modo manual activo
    bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 1, 0);  // Modo semiauto apagado
    bitWrite(HMI.plc_regs[MB_HMI_BOTONES], 2, 0);  // Modo auto apagado
    HMI_Unlock();

    VistaAlarmas_Update();
  }
}

void dibujar_vista_alarmas() {
  limpiar_workspace();

  if (workspace == NULL || !lv_obj_is_valid(workspace)) return;

  // =========================================================================
  // 1. BANNER SUPERIOR DE ESTADO GENERAL (Y: 15, H: 65, W: 1200)
  // =========================================================================
  banner_alarmas = lv_obj_create(workspace);
  lv_obj_set_size(banner_alarmas, 1200, 65);
  lv_obj_set_pos(banner_alarmas, 40, 15);
  lv_obj_remove_flag(banner_alarmas, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(banner_alarmas, 8, 0);
  lv_obj_set_style_border_width(banner_alarmas, 0, 0);
  lv_obj_set_style_bg_color(banner_alarmas, COLOR_VERDE_OK, 0);

  lbl_banner = lv_label_create(banner_alarmas);
  lv_label_set_text(lbl_banner, LV_SYMBOL_OK "  SISTEMA NORMAL - SIN ALARMAS ACTIVAS");
  lv_obj_set_style_text_color(lbl_banner, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(lbl_banner, &lv_font_montserrat_24, 0);
  lv_obj_center(lbl_banner);

  // =========================================================================
  // 2. LAS 4 ALARMAS CRITICAS (2 Columnas x 2 Filas, Sin Superposicion)
  // =========================================================================
  static const char* tit_alarmas[CANT_ALARMAS_S3] = {
    "PARADA DE EMERGENCIA",
    "GUARDAS DE SEGURIDAD",
    "CARGA DE MATERIAL",
    "COMUNICACION PLC S3"
  };

  const int x_cols[2] = { 40, 655 };
  const int w_card = 585;
  const int h_card = 135;
  const int y_rows[2] = { 95, 245 };

  for (int i = 0; i < CANT_ALARMAS_S3; i++) {
    int col = i % 2;
    int row = i / 2;

    cards[i].card = lv_obj_create(workspace);
    lv_obj_set_size(cards[i].card, w_card, h_card);
    lv_obj_set_pos(cards[i].card, x_cols[col], y_rows[row]);
    lv_obj_remove_flag(cards[i].card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(cards[i].card, 8, 0);
    lv_obj_set_style_bg_color(cards[i].card, COLOR_CARD_OK, 0);
    lv_obj_set_style_border_color(cards[i].card, COLOR_CARD_BORDER_OK, 0);
    lv_obj_set_style_border_width(cards[i].card, 2, 0);
    lv_obj_set_style_pad_all(cards[i].card, 0, 0);  // Evitar margenes ocultos

    // Cuadro para icono a la izquierda (12 a 80 px)
    cards[i].box_icon = lv_obj_create(cards[i].card);
    lv_obj_set_size(cards[i].box_icon, 68, 68);
    lv_obj_align(cards[i].box_icon, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_remove_flag(cards[i].box_icon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(cards[i].box_icon, 6, 0);
    lv_obj_set_style_border_width(cards[i].box_icon, 0, 0);
    lv_obj_set_style_bg_color(cards[i].box_icon, COLOR_VERDE_OK, 0);

    cards[i].lbl_icon = lv_label_create(cards[i].box_icon);
    lv_label_set_text(cards[i].lbl_icon, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(cards[i].lbl_icon, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(cards[i].lbl_icon, &lv_font_montserrat_32, 0);
    lv_obj_center(cards[i].lbl_icon);

    // Titulo de la alarma (Con ancho maximo estricto para no tocar el badge)
    cards[i].lbl_titulo = lv_label_create(cards[i].card);
    lv_label_set_text(cards[i].lbl_titulo, tit_alarmas[i]);
    lv_obj_set_style_text_color(cards[i].lbl_titulo, COLOR_TEXTO_TITULO, 0);
    lv_obj_set_style_text_font(cards[i].lbl_titulo, &lv_font_montserrat_20, 0);
    lv_obj_set_size(cards[i].lbl_titulo, 330, LV_SIZE_CONTENT);
    lv_label_set_long_mode(cards[i].lbl_titulo, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(cards[i].lbl_titulo, 92, 22);

    // Subtexto / Descripcion detallada (Con ancho maximo estricto de 330 px)
    cards[i].lbl_desc = lv_label_create(cards[i].card);
    lv_label_set_text(cards[i].lbl_desc, "Normal");
    lv_obj_set_style_text_color(cards[i].lbl_desc, COLOR_GRIS_MEDIO, 0);
    lv_obj_set_style_text_font(cards[i].lbl_desc, &lv_font_montserrat_16, 0);
    lv_obj_set_size(cards[i].lbl_desc, 330, LV_SIZE_CONTENT);
    lv_label_set_long_mode(cards[i].lbl_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(cards[i].lbl_desc, 92, 58);

    // Badge de estado a la derecha (448 a 573 px)
    cards[i].badge_estado = lv_obj_create(cards[i].card);
    lv_obj_set_size(cards[i].badge_estado, 125, 46);
    lv_obj_align(cards[i].badge_estado, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_remove_flag(cards[i].badge_estado, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(cards[i].badge_estado, 6, 0);
    lv_obj_set_style_border_width(cards[i].badge_estado, 0, 0);
    lv_obj_set_style_bg_color(cards[i].badge_estado, COLOR_VERDE_OK, 0);

    cards[i].lbl_badge = lv_label_create(cards[i].badge_estado);
    lv_label_set_text(cards[i].lbl_badge, "NORMAL");
    lv_obj_set_style_text_color(cards[i].lbl_badge, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(cards[i].lbl_badge, &lv_font_montserrat_20, 0);
    lv_obj_center(cards[i].lbl_badge);
  }

  // =========================================================================
  // 3. BARRA DE CONTROL Y ACCION (Y: 400, H: 90, W: 1200)
  // =========================================================================
  // A) Indicador de Baliza / Sirena
  box_sirena = lv_obj_create(workspace);
  lv_obj_set_size(box_sirena, 330, 90);
  lv_obj_set_pos(box_sirena, 40, 400);
  lv_obj_remove_flag(box_sirena, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(box_sirena, 8, 0);
  lv_obj_set_style_border_width(box_sirena, 2, 0);
  lv_obj_set_style_bg_color(box_sirena, COLOR_GRIS_OSCURO, 0);
  lv_obj_set_style_border_color(box_sirena, lv_color_hex(0x666666), 0);
  lv_obj_set_style_pad_all(box_sirena, 0, 0);

  lbl_sirena = lv_label_create(box_sirena);
  lv_label_set_text(lbl_sirena, LV_SYMBOL_BELL "  SIRENA: SILENCIADA");
  lv_obj_set_style_text_color(lbl_sirena, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(lbl_sirena, &lv_font_montserrat_20, 0);
  lv_obj_center(lbl_sirena);

  // B) Boton Grande de Rearmar / Silenciar Alarmas
  btn_rearmar = lv_button_create(workspace);
  lv_obj_set_size(btn_rearmar, 490, 90);
  lv_obj_set_pos(btn_rearmar, 395, 400);
  lv_obj_remove_flag(btn_rearmar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(btn_rearmar, 8, 0);
  lv_obj_set_style_bg_color(btn_rearmar, COLOR_AZUL_INDUSTRIAL, 0);
  lv_obj_set_style_border_color(btn_rearmar, lv_color_hex(0x003388), 0);
  lv_obj_set_style_border_width(btn_rearmar, 3, 0);
  lv_obj_set_style_pad_all(btn_rearmar, 0, 0);
  lv_obj_add_event_cb(btn_rearmar, btn_rearmar_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t* lbl_btn = lv_label_create(btn_rearmar);
  lv_label_set_text(lbl_btn, LV_SYMBOL_REFRESH "  REARMAR / SILENCIAR");
  lv_obj_set_style_text_color(lbl_btn, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(lbl_btn, &lv_font_montserrat_24, 0);
  lv_obj_center(lbl_btn);

  // C) Cuadro de Enlace PLC / Robot (Usa flex column para cero superposicion)
  box_enlaces = lv_obj_create(workspace);
  lv_obj_set_size(box_enlaces, 330, 90);
  lv_obj_set_pos(box_enlaces, 910, 400);
  lv_obj_remove_flag(box_enlaces, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(box_enlaces, 8, 0);
  lv_obj_set_style_border_width(box_enlaces, 2, 0);
  lv_obj_set_style_bg_color(box_enlaces, COLOR_GRIS_OSCURO, 0);
  lv_obj_set_style_border_color(box_enlaces, lv_color_hex(0x666666), 0);
  lv_obj_set_style_pad_all(box_enlaces, 6, 0);
  lv_obj_set_flex_flow(box_enlaces, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(box_enlaces, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(box_enlaces, 8, 0);

  lbl_enlace_plc = lv_label_create(box_enlaces);
  lv_label_set_text(lbl_enlace_plc, "PLC S3: ONLINE [OK]");
  lv_obj_set_style_text_color(lbl_enlace_plc, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_text_font(lbl_enlace_plc, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_align(lbl_enlace_plc, LV_TEXT_ALIGN_CENTER, 0);

  lbl_enlace_robot = lv_label_create(box_enlaces);
  lv_label_set_text(lbl_enlace_robot, "ROBOT EXTRACTOR: LISTO");
  lv_obj_set_style_text_color(lbl_enlace_robot, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_text_font(lbl_enlace_robot, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_align(lbl_enlace_robot, LV_TEXT_ALIGN_CENTER, 0);

  // =========================================================================
  // 4. PANEL INFORMATIVO DE SEGURIDAD (Y: 510, H: 65, W: 1200)
  // =========================================================================
  lv_obj_t* pnl_info = lv_obj_create(workspace);
  lv_obj_set_size(pnl_info, 1200, 65);
  lv_obj_set_pos(pnl_info, 40, 510);
  lv_obj_remove_flag(pnl_info, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(pnl_info, 8, 0);
  lv_obj_set_style_border_width(pnl_info, 1, 0);
  lv_obj_set_style_bg_color(pnl_info, lv_color_hex(0xF8F8F8), 0);
  lv_obj_set_style_border_color(pnl_info, lv_color_hex(0xBBBBBB), 0);
  lv_obj_set_style_pad_all(pnl_info, 8, 0);

  lv_obj_t* lbl_info = lv_label_create(pnl_info);
  lv_label_set_text(lbl_info, LV_SYMBOL_SETTINGS "  PROTOCOLO: El rearme y silenciado de sirena opera en MODO MANUAL con guardas y circuito restablecidos.");
  lv_obj_set_style_text_color(lbl_info, COLOR_GRIS_MEDIO, 0);
  lv_obj_set_style_text_font(lbl_info, &lv_font_montserrat_16, 0);
  lv_obj_set_width(lbl_info, 1160);
  lv_label_set_long_mode(lbl_info, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(lbl_info, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(lbl_info);

  VistaAlarmas_Update();
}

void VistaAlarmas_Update() {
  if (workspace == NULL || !lv_obj_is_valid(workspace)) return;
  if (cards[0].card == NULL || !lv_obj_is_valid(cards[0].card)) return;

  // Snapshot atomico de datos Modbus
  HMI_Lock();
  uint16_t sensores = HMI.plc_regs[MB_SENSORES];
  uint16_t sdigitales = HMI.plc_regs[MB_SDIGITALES];
  uint16_t control = HMI.plc_regs[MB_CONTROL];
  bool plc_online = HMI.plc_conectado;
  bool robot_online = HMI.robot_conectado;
  HMI_Unlock();

  // =========================================================================
  // 1. EVALUACION DE LAS ALARMAS DE MMI_S3
  // =========================================================================
  // 1) Parada de Emergencia: NC fisico abierto (bit 9 == 0), virtual (bit 1 == 1) o rele (bit 0 == 1)
  bool al_estop = (bitRead(sensores, 9) == 0) || (bitRead(control, 1) == 1) || (bitRead(sdigitales, 0) == 1);

  // 2) Peligro Guarda de Seguridad: Intento de cierre con puerta abierta (MB_CONTROL bit 9)
  bool al_guarda = (bitRead(control, 9) == 1) || (bitRead(sdigitales, 1) == 1 && (bitRead(sensores, 5) == 0 || bitRead(sensores, 6) == 0));

  // 3) Falla Carga de Material: Tiempo excedido en carga (MB_CONTROL bit 8)
  bool al_carga = (bitRead(control, 8) == 1);

  // 4) Enlace RS-485 Modbus con PLC
  bool al_comms = !plc_online;

  bool estados_alarmas[CANT_ALARMAS_S3] = { al_estop, al_guarda, al_carga, al_comms };

  static const char* desc_alerta[CANT_ALARMAS_S3] = {
    "Pulsador activado o bucle de seguridad abierto",
    "Guarda abierta o intento de cierre inseguro",
    "Tiempo maximo de carga excedido",
    "Sin respuesta Modbus RS-485 del PLC S3"
  };

  static const char* desc_normal[CANT_ALARMAS_S3] = {
    "Circuito de parada restablecido y cerrado",
    "Guardas cerradas y seguras para operar",
    "iempo de dosificacion y carga correcta",
    "Enlace RS-485 online y sincronizado (ID 1)"
  };

  static const char* badge_alerta_txt[CANT_ALARMAS_S3] = {
    "ALARMA", "PELIGRO", "ERROR", "OFFLINE"
  };

  for (int i = 0; i < CANT_ALARMAS_S3; i++) {
    bool activa = estados_alarmas[i];

    if (cards[i].card && lv_obj_is_valid(cards[i].card)) {
      if (activa) {
        lv_obj_set_style_bg_color(cards[i].card, COLOR_CARD_ALARM, 0);
        lv_obj_set_style_border_color(cards[i].card, COLOR_CARD_BORDER_ALARM, 0);
        lv_obj_set_style_border_width(cards[i].card, 3, 0);

        lv_obj_set_style_bg_color(cards[i].box_icon, COLOR_ROJO_ALERTA, 0);
        lv_label_set_text(cards[i].lbl_icon, LV_SYMBOL_WARNING);

        lv_label_set_text(cards[i].lbl_desc, desc_alerta[i]);
        lv_obj_set_style_text_color(cards[i].lbl_desc, COLOR_ROJO_ALERTA, 0);

        lv_obj_set_style_bg_color(cards[i].badge_estado, COLOR_ROJO_ALERTA, 0);
        lv_label_set_text(cards[i].lbl_badge, badge_alerta_txt[i]);
      } else {
        lv_obj_set_style_bg_color(cards[i].card, COLOR_CARD_OK, 0);
        lv_obj_set_style_border_color(cards[i].card, COLOR_CARD_BORDER_OK, 0);
        lv_obj_set_style_border_width(cards[i].card, 2, 0);

        lv_obj_set_style_bg_color(cards[i].box_icon, COLOR_VERDE_OK, 0);
        lv_label_set_text(cards[i].lbl_icon, LV_SYMBOL_OK);

        lv_label_set_text(cards[i].lbl_desc, desc_normal[i]);
        lv_obj_set_style_text_color(cards[i].lbl_desc, COLOR_GRIS_MEDIO, 0);

        lv_obj_set_style_bg_color(cards[i].badge_estado, COLOR_VERDE_OK, 0);
        lv_label_set_text(cards[i].lbl_badge, (i == 3) ? "ONLINE" : "NORMAL");
      }
    }
  }

  // Actualizacion del Banner Superior
  if (banner_alarmas && lv_obj_is_valid(banner_alarmas)) {
    if (al_estop) {
      lv_obj_set_style_bg_color(banner_alarmas, COLOR_ROJO_ALERTA, 0);
      lv_label_set_text(lbl_banner, LV_SYMBOL_WARNING "  ALARMA CRITICA: PARADA DE EMERGENCIA ACTIVADA");
    } else if (al_guarda) {
      lv_obj_set_style_bg_color(banner_alarmas, COLOR_ROJO_ALERTA, 0);
      lv_label_set_text(lbl_banner, LV_SYMBOL_WARNING "  PELIGRO: INTENTO DE CIERRE CON GUARDA ABIERTA");
    } else if (al_carga) {
      lv_obj_set_style_bg_color(banner_alarmas, COLOR_ROJO_ALERTA, 0);
      lv_label_set_text(lbl_banner, LV_SYMBOL_WARNING "  ERROR: TIEMPO EXCEDIDO EN CARGA DE MATERIAL");
    } else if (al_comms) {
      lv_obj_set_style_bg_color(banner_alarmas, COLOR_ROJO_ALERTA, 0);
      lv_label_set_text(lbl_banner, LV_SYMBOL_WARNING "  ERROR: PERDIDA DE COMUNICACION CON PLC S3");
    } else {
      lv_obj_set_style_bg_color(banner_alarmas, COLOR_VERDE_OK, 0);
      lv_label_set_text(lbl_banner, LV_SYMBOL_OK "  SISTEMA NORMAL - SIN ALARMAS ACTIVAS");
    }
  }

  // Actualizacion de Baliza / Sirena
  bool sirena_activa = (bitRead(sdigitales, 8) == 1);
  if (box_sirena && lv_obj_is_valid(box_sirena)) {
    if (sirena_activa) {
      lv_obj_set_style_bg_color(box_sirena, COLOR_AMBAR_AVISO, 0);
      lv_obj_set_style_border_color(box_sirena, lv_color_hex(0xFFAA00), 0);
      lv_label_set_text(lbl_sirena, LV_SYMBOL_BELL "  SIRENA: ACTIVADA");
      lv_obj_set_style_text_color(lbl_sirena, lv_color_hex(0xFFFFFF), 0);
    } else {
      lv_obj_set_style_bg_color(box_sirena, COLOR_GRIS_OSCURO, 0);
      lv_obj_set_style_border_color(box_sirena, lv_color_hex(0x666666), 0);
      lv_label_set_text(lbl_sirena, LV_SYMBOL_BELL "  SIRENA: SILENCIADA");
      lv_obj_set_style_text_color(lbl_sirena, lv_color_hex(0xCCCCCC), 0);
    }
  }

  // Actualizacion de Enlaces
  if (lbl_enlace_plc && lv_obj_is_valid(lbl_enlace_plc)) {
    if (plc_online) {
      lv_label_set_text(lbl_enlace_plc, "PLC S3: ONLINE [OK]");
      lv_obj_set_style_text_color(lbl_enlace_plc, lv_color_hex(0x00FF00), 0);
    } else {
      lv_label_set_text(lbl_enlace_plc, "PLC S3: DESCONECTADO");
      lv_obj_set_style_text_color(lbl_enlace_plc, lv_color_hex(0xFF4444), 0);
    }
  }

  if (lbl_enlace_robot && lv_obj_is_valid(lbl_enlace_robot)) {
    if (robot_online) {
      lv_label_set_text(lbl_enlace_robot, "ROBOT EXTRACTOR: CONECTADO");
      lv_obj_set_style_text_color(lbl_enlace_robot, lv_color_hex(0x00FF00), 0);
    } else {
      lv_label_set_text(lbl_enlace_robot, "ROBOT EXTRACTOR: LISTO");
      lv_obj_set_style_text_color(lbl_enlace_robot, lv_color_hex(0xAAAAAA), 0);
    }
  }
}
