#pragma once
#include <lvgl.h>

void UIManager_Init();
void UIManager_Update();
void limpiar_workspace();

extern lv_obj_t * workspace;
extern lv_obj_t * lbl_status_general;