#include "ModbusManager.h"
#include "MachineData.h"
#include "Config.h"
#include <ModbusRTU.h>

#if ARDUINO_USB_CDC_ON_BOOT
  #define S3_PRINTF(...) do { Serial.printf(__VA_ARGS__); Serial0.printf(__VA_ARGS__); } while(0)
  #define S3_PRINTLN(...) do { Serial.println(__VA_ARGS__); Serial0.println(__VA_ARGS__); } while(0)
#else
  #define S3_PRINTF(...) Serial.printf(__VA_ARGS__)
  #define S3_PRINTLN(...) Serial.println(__VA_ARGS__)
#endif

ModbusRTU mb;

// Rastreo de cambios para diagnóstico por Serial USB CDC
static uint16_t prev_hmi_botones = 0;

static const char* nombreBoton(uint8_t bit) {
  switch (bit) {
    case 0:  return "MODO MANUAL";
    case 1:  return "MODO SEMIAUTOMATICO";
    case 2:  return "MODO AUTOMATICO";
    case 3:  return "MOTOR PRINCIPAL ON";
    case 4:  return "MOTOR PRINCIPAL OFF";
    case 5:  return "INICIO CICLO / START";
    case 6:  return "CARGA / DOSIFICACION";
    case 7:  return "INYECCION";
    case 8:  return "CIERRE PRENSA";
    case 9:  return "APERTURA PRENSA";
    case 10: return "UNIDAD ATRAS";
    case 11: return "UNIDAD ADELANTE";
    case 12: return "EXPULSOR ADELANTE";
    case 13: return "EXPULSOR ATRAS";
    case 14: return "RESET CONTADOR PIEZAS";
    default: return "BOTON DESCONOCIDO";
  }
}

void ModbusManager_Init() {
  pinMode(RS485_RX_PIN, INPUT_PULLUP);
  Serial2.setRxBufferSize(512);
  Serial2.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);

  mb.begin(&Serial2);
  mb.slave(MODBUS_SLAVE_ID);
  mb.setBaudrate(RS485_BAUD);

  for (int i = 0; i < MB_REGS; i++) {
    mb.addHreg(i, 0);
  }

  // Setpoints iniciales coherentes
  PLC_Lock();
  PLC.regs[MB_SETPOINTZ1]     = 180;
  PLC.regs[MB_SETPOINTZ2]     = 190;
  PLC.regs[MB_SETPOINTZ3]     = 200;
  PLC.regs[MB_TEMPZ1]         = 180;
  PLC.regs[MB_TEMPZ2]         = 190;
  PLC.regs[MB_TEMPZ3]         = 200;
  PLC.regs[MB_TINYECCION]     = 5;
  PLC.regs[MB_TSOSTENIMIENTO] = 3;
  PLC.regs[MB_TENFRIAMIENTO]  = 12;
  PLC.regs[MB_TEXP_AVANCE]    = 2;
  PLC.regs[MB_TEXP_RETROCESO] = 2;
  PLC.regs[MB_HISTERESIS]     = 2;
  PLC.regs[MB_TOTAL_PIEZAS]   = 0;
  PLC.regs[MB_TIEMPO_CICLO]   = 0;
  PLC.regs[MB_PASO_ACTUAL]    = 0;

  // Estado seguro de entradas por defecto (Seta emerg. NC=1, puertas NC=1, presion=1)
  bitWrite(PLC.regs[MB_SENSORES], 9, 1);
  bitWrite(PLC.regs[MB_SENSORES], 6, 1);
  bitWrite(PLC.regs[MB_SENSORES], 5, 1);
  bitWrite(PLC.regs[MB_SENSORES], 4, 1);

  for (int i = 0; i < MB_REGS; i++) {
    mb.Hreg(i, PLC.regs[i]);
  }
  PLC_Unlock();

  // Callbacks de diagnóstico por consola USB CDC (idénticos al monitor que funcionó)
  mb.onRequest([](Modbus::FunctionCode fc, const Modbus::RequestData data) -> Modbus::ResultCode {
    if (fc == Modbus::FC_READ_REGS) {
      static unsigned long t_s3_read_log = 0;
      if (millis() - t_s3_read_log > 2000) {
        t_s3_read_log = millis();
        S3_PRINTF("[PLC-S3 RX] Peticion LECTURA de Pantalla: Reg %d (Cantidad: %d)\n", data.reg.address, data.regCount);
      }
    } else if (fc == Modbus::FC_WRITE_REG) {
      S3_PRINTF("[PLC-S3 RX] Escritura desde HMI: Reg %d = %d (0x%04X)\n", 
                data.reg.address, data.regCount, data.regCount);
    }
    return Modbus::EX_SUCCESS;
  });

  mb.onRequestSuccess([](Modbus::FunctionCode fc, const Modbus::RequestData data) -> Modbus::ResultCode {
    if (fc == Modbus::FC_READ_REGS) {
      static unsigned long t_s3_resp_log = 0;
      if (millis() - t_s3_resp_log > 2000) {
        t_s3_resp_log = millis();
        S3_PRINTF("[PLC-S3 TX] RESPUESTA ENVIADA A PANTALLA: %d registros devueltos.\n", data.regCount);
      }
    } else if (fc == Modbus::FC_WRITE_REG) {
      S3_PRINTF("[PLC-S3 TX] CONFIRMACION ENVIADA: Reg %d respondido a la pantalla.\n", data.reg.address);
    }
    return Modbus::EX_SUCCESS;
  });

  S3_PRINTLN("[MODBUS] Esclavo RS-485 listo en Serial2 (RX:16, TX:17 @ 9600 baud, ID:1)");
}

void ModbusManager_Update() {
  // 1. Sincronizar hacia los holding registers Modbus los datos generados por el PLC
  PLC_Lock();
  mb.Hreg(MB_SENSORES,        PLC.regs[MB_SENSORES]);
  mb.Hreg(MB_SDIGITALES,      PLC.regs[MB_SDIGITALES]);
  mb.Hreg(MB_CONTROL,         PLC.regs[MB_CONTROL]);
  mb.Hreg(MB_TESPERAI,        PLC.regs[MB_TESPERAI]);
  mb.Hreg(MB_TINYECCIONI,     PLC.regs[MB_TINYECCIONI]);
  mb.Hreg(MB_TSOSTENIMIENTOI, PLC.regs[MB_TSOSTENIMIENTOI]);
  mb.Hreg(MB_TENFRIAMIENTOI,  PLC.regs[MB_TENFRIAMIENTOI]);
  mb.Hreg(MB_TEXP_AVANCEI,    PLC.regs[MB_TEXP_AVANCEI]);
  mb.Hreg(MB_TEXP_RETROCESOI, PLC.regs[MB_TEXP_RETROCESOI]);
  mb.Hreg(MB_TEMPZ1,          PLC.regs[MB_TEMPZ1]);
  mb.Hreg(MB_TEMPZ2,          PLC.regs[MB_TEMPZ2]);
  mb.Hreg(MB_TEMPZ3,          PLC.regs[MB_TEMPZ3]);
  mb.Hreg(MB_TOTAL_PIEZAS,    PLC.regs[MB_TOTAL_PIEZAS]);
  mb.Hreg(MB_TIEMPO_CICLO,    PLC.regs[MB_TIEMPO_CICLO]);
  mb.Hreg(MB_PASO_ACTUAL,     PLC.regs[MB_PASO_ACTUAL]);
  PLC_Unlock();

  // 2. Procesar tramas del protocolo Modbus RTU
  mb.task();

  // 3. Tomar comandos y setpoints actualizados por la pantalla
  PLC_Lock();
  PLC.regs[MB_HMI_BOTONES]    = mb.Hreg(MB_HMI_BOTONES);
  PLC.regs[MB_SETPOINTZ1]     = mb.Hreg(MB_SETPOINTZ1);
  PLC.regs[MB_SETPOINTZ2]     = mb.Hreg(MB_SETPOINTZ2);
  PLC.regs[MB_SETPOINTZ3]     = mb.Hreg(MB_SETPOINTZ3);
  PLC.regs[MB_TINYECCION]     = mb.Hreg(MB_TINYECCION);
  PLC.regs[MB_TSOSTENIMIENTO] = mb.Hreg(MB_TSOSTENIMIENTO);
  PLC.regs[MB_TENFRIAMIENTO]  = mb.Hreg(MB_TENFRIAMIENTO);
  PLC.regs[MB_TEXP_AVANCE]    = mb.Hreg(MB_TEXP_AVANCE);
  PLC.regs[MB_TEXP_RETROCESO] = mb.Hreg(MB_TEXP_RETROCESO);
  PLC.regs[MB_TESPERA]        = mb.Hreg(MB_TESPERA);
  PLC.regs[MB_HISTERESIS]     = mb.Hreg(MB_HISTERESIS);

  // Sincronizacion de Handshake con Robot (transmitido por la HMI en MB_CONTROL)
  // Nota: Las alarmas (bits 8 y 9) se reafirman y limpian de forma segura en AlarmManager al conmutar a Modo Manual.
  uint16_t hmi_control = mb.Hreg(MB_CONTROL);

  // Sincronizacion de Handshake con Robot (transmitido por la HMI)
  if (bitRead(hmi_control, 4) == 1) {
    bitWrite(PLC.regs[MB_CONTROL], 4, 1);
    bitWrite(hmi_control, 4, 0);
    mb.Hreg(MB_CONTROL, hmi_control);
    S3_PRINTLN("[PLC-S3] Handshake: Permiso de Cierre recibido del Robot!");
  }
  if (bitRead(hmi_control, 10) == 1) {
    bitWrite(PLC.regs[MB_CONTROL], 10, 1);
    bitWrite(hmi_control, 10, 0);
    mb.Hreg(MB_CONTROL, hmi_control);
    S3_PRINTLN("[PLC-S3] Handshake: Robot solicita Expulsor Adelante!");
  }
  if (bitRead(hmi_control, 11) == 1) {
    bitWrite(PLC.regs[MB_CONTROL], 11, 1);
    bitWrite(hmi_control, 11, 0);
    mb.Hreg(MB_CONTROL, hmi_control);
    S3_PRINTLN("[PLC-S3] Handshake: Robot solicita Expulsor Atras!");
  }

  uint16_t cur_botones = PLC.regs[MB_HMI_BOTONES];
  PLC_Unlock();

  // 4. Log detallado de cambios en botones por el puerto Serial USB
  if (cur_botones != prev_hmi_botones) {
    for (uint8_t bit = 0; bit < 15; bit++) {
      bool act = bitRead(cur_botones, bit);
      bool ant = bitRead(prev_hmi_botones, bit);
      if (act != ant) {
        if (act) {
          Serial.printf("[PLC-S3] BOTON ON  ===> %s (Bit %d)\n", nombreBoton(bit), bit);
        } else {
          Serial.printf("[PLC-S3] BOTON OFF ---  %s (Bit %d)\n", nombreBoton(bit), bit);
        }
      }
    }
    prev_hmi_botones = cur_botones;
  }
}
