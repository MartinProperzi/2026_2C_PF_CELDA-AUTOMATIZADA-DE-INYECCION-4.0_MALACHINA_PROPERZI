#include "MachineData.h"

MachineState PLC = { 0 };
SemaphoreHandle_t plc_mutex = NULL;
