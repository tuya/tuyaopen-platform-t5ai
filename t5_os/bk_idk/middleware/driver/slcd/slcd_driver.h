// Copyright 2023-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <components/log.h>
#include <driver/hal/hal_gpio_types.h>


#define SLCD_TAG "slcd"

#define SLCD_LOGI(...) BK_LOGI(SLCD_TAG, ##__VA_ARGS__)
#define SLCD_LOGW(...) BK_LOGW(SLCD_TAG, ##__VA_ARGS__)
#define SLCD_LOGE(...) BK_LOGE(SLCD_TAG, ##__VA_ARGS__)
#define SLCD_LOGD(...) BK_LOGD(SLCD_TAG, ##__VA_ARGS__)


#define SLCD_COM_NUM    4

#if SLCD_COM_NUM == 4
#define SLCD_GPIO_MAP \
{\
    {GPIO_47, GPIO_DEV_SLCD_COM0},\
    {GPIO_46, GPIO_DEV_SLCD_COM1},\
    {GPIO_45, GPIO_DEV_SLCD_COM2},\
    {GPIO_44, GPIO_DEV_SLCD_COM3},\
    {GPIO_43, GPIO_DEV_SLCD_SEG0},\
    {GPIO_42, GPIO_DEV_SLCD_SEG1},\
    {GPIO_41, GPIO_DEV_SLCD_SEG2},\
    {GPIO_40, GPIO_DEV_SLCD_SEG3},\
    {GPIO_26, GPIO_DEV_SLCD_SEG4},\
    {GPIO_25, GPIO_DEV_SLCD_SEG5},\
    {GPIO_24, GPIO_DEV_SLCD_SEG6},\
    {GPIO_23, GPIO_DEV_SLCD_SEG7},\
    {GPIO_22, GPIO_DEV_SLCD_SEG8},\
    {GPIO_21, GPIO_DEV_SLCD_SEG9},\
    {GPIO_20, GPIO_DEV_SLCD_SEG10},\
    {GPIO_19, GPIO_DEV_SLCD_SEG11},\
    {GPIO_18, GPIO_DEV_SLCD_SEG12},\
    {GPIO_17, GPIO_DEV_SLCD_SEG13},\
    {GPIO_16, GPIO_DEV_SLCD_SEG14},\
    {GPIO_15, GPIO_DEV_SLCD_SEG15},\
    {GPIO_14, GPIO_DEV_SLCD_SEG16},\
    {GPIO_27, GPIO_DEV_SLCD_SEG17},\
    {GPIO_28, GPIO_DEV_SLCD_SEG18},\
    {GPIO_29, GPIO_DEV_SLCD_SEG19},\
    {GPIO_30, GPIO_DEV_SLCD_SEG20},\
    {GPIO_31, GPIO_DEV_SLCD_SEG21},\
    {GPIO_32, GPIO_DEV_SLCD_SEG22},\
    {GPIO_33, GPIO_DEV_SLCD_SEG23},\
    {GPIO_34, GPIO_DEV_SLCD_SEG24},\
    {GPIO_35, GPIO_DEV_SLCD_SEG25},\
    {GPIO_36, GPIO_DEV_SLCD_SEG26},\
    {GPIO_37, GPIO_DEV_SLCD_SEG27},\
    {GPIO_38, GPIO_DEV_SLCD_SEG28},\
    {GPIO_39, GPIO_DEV_SLCD_SEG29},\
    {GPIO_4, GPIO_DEV_SLCD_SEG30},\
    {GPIO_5, GPIO_DEV_SLCD_SEG31},\
}
#elif SLCD_COM_NUM == 8
#define SLCD_GPIO_MAP \
{\
    {GPIO_47, GPIO_DEV_SLCD_COM0},\
    {GPIO_46, GPIO_DEV_SLCD_COM1},\
    {GPIO_45, GPIO_DEV_SLCD_COM2},\
    {GPIO_44, GPIO_DEV_SLCD_COM3},\
    {GPIO_38, GPIO_DEV_SLCD_COM4},\
    {GPIO_39, GPIO_DEV_SLCD_COM5},\
    {GPIO_4, GPIO_DEV_SLCD_COM6},\
    {GPIO_5, GPIO_DEV_SLCD_COM7},\
    {GPIO_43, GPIO_DEV_SLCD_SEG0},\
    {GPIO_42, GPIO_DEV_SLCD_SEG1},\
    {GPIO_41, GPIO_DEV_SLCD_SEG2},\
    {GPIO_40, GPIO_DEV_SLCD_SEG3},\
    {GPIO_26, GPIO_DEV_SLCD_SEG4},\
    {GPIO_25, GPIO_DEV_SLCD_SEG5},\
    {GPIO_24, GPIO_DEV_SLCD_SEG6},\
    {GPIO_23, GPIO_DEV_SLCD_SEG7},\
    {GPIO_22, GPIO_DEV_SLCD_SEG8},\
    {GPIO_21, GPIO_DEV_SLCD_SEG9},\
    {GPIO_20, GPIO_DEV_SLCD_SEG10},\
    {GPIO_19, GPIO_DEV_SLCD_SEG11},\
    {GPIO_18, GPIO_DEV_SLCD_SEG12},\
    {GPIO_17, GPIO_DEV_SLCD_SEG13},\
    {GPIO_16, GPIO_DEV_SLCD_SEG14},\
    {GPIO_15, GPIO_DEV_SLCD_SEG15},\
    {GPIO_14, GPIO_DEV_SLCD_SEG16},\
    {GPIO_27, GPIO_DEV_SLCD_SEG17},\
    {GPIO_28, GPIO_DEV_SLCD_SEG18},\
    {GPIO_29, GPIO_DEV_SLCD_SEG19},\
    {GPIO_30, GPIO_DEV_SLCD_SEG20},\
    {GPIO_31, GPIO_DEV_SLCD_SEG21},\
    {GPIO_32, GPIO_DEV_SLCD_SEG22},\
    {GPIO_33, GPIO_DEV_SLCD_SEG23},\
    {GPIO_34, GPIO_DEV_SLCD_SEG24},\
    {GPIO_35, GPIO_DEV_SLCD_SEG25},\
    {GPIO_36, GPIO_DEV_SLCD_SEG26},\
    {GPIO_37, GPIO_DEV_SLCD_SEG27},\
}
#endif


#ifdef __cplusplus
}
#endif

