#include "ExtractionManager.h"
#include "Config.h"
#include "RobotData.h"
#include "SafetyManager.h"
#include "GripperManager.h"
#include "MotionManager.h"

void ExtractionManager_Init() {
  Robot.estado = ROBOT_DESCONOCIDO;
  Robot.homing_completado = false;
}

bool ExtractionManager_Homing() {
  Serial.println("[HOMING] Iniciando rutina de Homing...");
  Robot.estado = ROBOT_HOMING_EN_CURSO;
  Robot.homing_completado = false;
  bitWrite(Robot.handshake, 0, 0); // No esta en Home
  bitWrite(Robot.handshake, 3, 0); // Deshabilita permiso de cierre durante movimiento

  // 1. Homing Eje Y (Hacia arriba primero por seguridad dentro del molde)
  Serial.println("[HOMING] 1. Elevando Eje Y buscando origen superior...");
  long pasosSeguridadY = 0;
  while (!Safety_IsHomeY()) {
    MotionManager_PasoUnico(0, -1, Robot.velocidad_y_us);
    pasosSeguridadY++;
    if (pasosSeguridadY > 6000) {
      Serial.println("[HOMING ALERTA] Limite de pasos alcanzado en Eje Y sin tocar switch!");
      Robot.estado = ROBOT_ALARMA_ERROR;
      return false;
    }
  }
  vTaskDelay(pdMS_TO_TICKS(200));

  // Despegue de 5 mm (25 pasos) hacia abajo para liberar el switch Y
  Serial.println("[HOMING] Despegando switch Y...");
  MotionManager_Mover(0, 25, Robot.velocidad_y_us);
  vTaskDelay(pdMS_TO_TICKS(300));

  // 2. Homing Eje X (Hacia la derecha una vez despejada la altura)
  Serial.println("[HOMING] 2. Trasladando Eje X buscando origen derecho...");
  long pasosSeguridadX = 0;
  while (!Safety_IsHomeX()) {
    MotionManager_PasoUnico(1, 0, Robot.velocidad_x_us);
    pasosSeguridadX++;
    if (pasosSeguridadX > 6000) {
      Serial.println("[HOMING ALERTA] Limite de pasos alcanzado en Eje X sin tocar switch!");
      Robot.estado = ROBOT_ALARMA_ERROR;
      return false;
    }
  }
  vTaskDelay(pdMS_TO_TICKS(200));

  // Despegue de 5 mm (25 pasos) hacia la izquierda para liberar el switch X
  Serial.println("[HOMING] Despegando switch X...");
  MotionManager_Mover(-25, 0, Robot.velocidad_x_us);
  vTaskDelay(pdMS_TO_TICKS(300));

  // 3. Finalizacion exitosa
  Robot.homing_completado = true;
  Robot.estado = ROBOT_LISTO_EN_HOME;
  bitWrite(Robot.handshake, 0, 1); // robot_en_home = 1
  
  // Actualizar interlock de zona segura
  SafetyManager_Update();

  Serial.println("[HOMING] Homing completado con exito. Cero de maquina establecido.\n");
  return true;
}

void ExtractionManager_CicloCompleto() {
  Serial.println("\n>>> [ROBOT: INICIANDO CICLO DE EXTRACCION] <<<");

  // ETAPA 1: ENTRAR A LA PRENSA (Prensa abierta)
  Robot.estado = ROBOT_ENTRANDO_PRENSA;
  bitWrite(Robot.handshake, 0, 0); // Ya no esta en Home
  bitWrite(Robot.handshake, 1, 0); // Limpia pedido previo de avance
  bitWrite(Robot.handshake, 2, 0); // Limpia pedido previo de retroceso
  bitWrite(Robot.handshake, 3, 0); // No permite cierre de prensa (riesgo de colision)

  Serial.printf("1. Traslado horizontal a cavidad: 60 cm Izquierda (-X) [%d us]\n", Robot.velocidad_x_us);
  MotionManager_Mover(-DISTANCIA_60CM, 0, Robot.velocidad_x_us);
  vTaskDelay(pdMS_TO_TICKS(150));

  Serial.printf("2. Descenso RAPIDO al molde: 40 cm Abajo (+Y) [%d us]\n", Robot.velocidad_y_us);
  MotionManager_Mover(0, DISTANCIA_40CM, Robot.velocidad_y_us);
  vTaskDelay(pdMS_TO_TICKS(150));

  // ETAPA 2: POSICIONADO FRENTE A LA PIEZA (PIDE EXPULSOR)
  Robot.estado = ROBOT_EN_POSICION_AGARRE;
  bitWrite(Robot.handshake, 1, 1); // robot_pide_expulsor_adelante = 1
  Serial.println("3. En posicion de agarre. Senal enviada a PLC: Pide Expulsor Adelante.");
  vTaskDelay(pdMS_TO_TICKS(1500)); // Tiempo de avance del expulsor de la inyectora

  // ETAPA 3: SUJECION DE LA PIEZA CON EL GRIPPER
  Robot.estado = ROBOT_SUJETANDO_PIEZA;
  Serial.println("4. Cerrando gripper sobre la pieza...");
  GripperManager_Cerrar();
  vTaskDelay(pdMS_TO_TICKS(1000)); // Tiempo de agarre seguro

  // ETAPA 4: ORDEN DE RETIRO DE EXPULSOR
  bitWrite(Robot.handshake, 1, 0); // Limpia pedido de avance
  bitWrite(Robot.handshake, 2, 1); // robot_pide_expulsor_atras = 1
  Serial.println("5. Pieza asegurada. Senal enviada a PLC: Pide Expulsor Atras.");
  vTaskDelay(pdMS_TO_TICKS(1500)); // Tiempo de retraccion inicial del expulsor
  // Mantenemos bit 2 activo durante la retirada para evitar condiciones de carrera en el bus Modbus

  // ETAPA 5: SALIR DE LA PRENSA HACIA ZONA SEGURA
  Robot.estado = ROBOT_SALIENDO_PRENSA;
  Serial.printf("6. Subida RAPIDA con la pieza: 40 cm Arriba (-Y) [%d us]\n", Robot.velocidad_y_us);
  MotionManager_Mover(0, -DISTANCIA_40CM, Robot.velocidad_y_us);
  vTaskDelay(pdMS_TO_TICKS(150));

  Serial.printf("7. Saliendo de la prensa hacia descarga: 50 cm Derecha (+X) [%d us]\n", Robot.velocidad_x_us);
  MotionManager_Mover(DISTANCIA_50CM, 0, Robot.velocidad_x_us);
  vTaskDelay(pdMS_TO_TICKS(150));

  // ETAPA 6: ZONA SEGURA DESPEJADA (VERIFICADA POR FIN DE CARRERA FISICO)
  Robot.estado = ROBOT_ZONA_SEGURA;
  bitWrite(Robot.handshake, 2, 0); // Molde despejado: limpiar peticion de retroceso expulsor
  SafetyManager_Update();
  if (Safety_IsZonaSegura()) {
    Serial.println("8. >>> ROBOT EN ZONA SEGURA (Fin de carrera activado): Permiso de Cierre concedido al PLC. <<<");
  } else {
    Serial.println("8. [ALERTA DE SEGURIDAD] Robot en cota pero fin de carrera de Zona Segura no accionado.");
  }

  // ETAPA 7: DESCARGA DE PIEZA SOBRE CAJA O CINTA
  Robot.estado = ROBOT_DESCARGANDO;
  Serial.printf("9. Bajada RAPIDA a cota de descarga: 40 cm Abajo (+Y) [%d us]\n", Robot.velocidad_y_us);
  MotionManager_Mover(0, DISTANCIA_40CM, Robot.velocidad_y_us);
  vTaskDelay(pdMS_TO_TICKS(150));

  Serial.println("10. Abriendo gripper (Descarga de pieza)");
  GripperManager_Abrir();
  vTaskDelay(pdMS_TO_TICKS(800));

  // ETAPA 8: RETORNO A REPOSO / HOME
  Serial.printf("11. Subida RAPIDA y retorno a Home (-Y [%d us] y +X [%d us])\n", Robot.velocidad_y_us, Robot.velocidad_x_us);
  MotionManager_Mover(0, -DISTANCIA_40CM, Robot.velocidad_y_us);
  vTaskDelay(pdMS_TO_TICKS(150));
  MotionManager_Mover(DISTANCIA_60CM - DISTANCIA_50CM, 0, Robot.velocidad_x_us); // 10 cm finales a Home

  Robot.total_piezas_extraidas++;
  Robot.estado = ROBOT_LISTO_EN_HOME;
  bitWrite(Robot.handshake, 0, 1); // robot_en_home = 1
  SafetyManager_Update();

  Serial.printf(">>> [CICLO COMPLETADO: Total piezas extraidas por robot: %d] <<<\n\n", Robot.total_piezas_extraidas);
}
