/**
 * @file      utilities.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2024-01-22
 *
 */
#pragma once

#define RGB_MAX_PIXEL_CLOCK_HZ (8000000UL)

#define WS_BOARD_TFT_WIDTH (480)
#define WS_BOARD_TFT_HEIGHT (480)

#define WS_BOARD_TFT_BL (6)

#define WS_BOARD_TFT_HSYNC (38)
#define WS_BOARD_TFT_VSYNC (39)
#define WS_BOARD_TFT_DE (40)
#define WS_BOARD_TFT_PCLK (41)

// T-RGB physical connection is RGB666, but ESP only supports RGB565

// RGB data signal(
// DB0:BlUE LSB;DB5:BIUE MSB;
// DB6:GREEN LSB;DB11:GREEN,MSB;
// DB12:RED LSB;DB17:RED MSB)
#define WS_BOARD_TFT_DATA0 (0)  // B0
#define WS_BOARD_TFT_DATA1 (5)  // B1
#define WS_BOARD_TFT_DATA2 (45) // B2
#define WS_BOARD_TFT_DATA3 (48) // B3
#define WS_BOARD_TFT_DATA4 (47) // B4
#define WS_BOARD_TFT_DATA5 (21) // B5

#define WS_BOARD_TFT_DATA6 (14)  // G0
#define WS_BOARD_TFT_DATA7 (13)  // G1
#define WS_BOARD_TFT_DATA8 (12)  // G2
#define WS_BOARD_TFT_DATA9 (11)  // G3
#define WS_BOARD_TFT_DATA10 (10) // G4
#define WS_BOARD_TFT_DATA11 (9)  // G5

#define WS_BOARD_TFT_DATA12 (0)  // R0
#define WS_BOARD_TFT_DATA13 (46) // R1
#define WS_BOARD_TFT_DATA14 (3)  // R2
#define WS_BOARD_TFT_DATA15 (8)  // R3
#define WS_BOARD_TFT_DATA16 (18) // R4
#define WS_BOARD_TFT_DATA17 (17) // R5

#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA0 (5)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA1 (45)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA2 (48)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA3 (47)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA4 (21)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA5 (14)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA6 (13)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA7 (12)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA8 (11)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA9 (10)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA10 (9)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA11 (46)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA12 (3)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA13 (8)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA14 (18)
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA15 (17)

#define WS_BOARD_TFT_RST (0) // Extension IO1
#define WS_BOARD_TFT_CS (0)  // Extension IO3
#define WS_BOARD_TFT_MOSI (1)
#define WS_BOARD_TFT_SCLK (2)

#define WS_BOARD_I2C_SDA (15)
#define WS_BOARD_I2C_SCL (7)

#define WS_BOARD_TOUCH_IRQ (16)
#define WS_BOARD_TOUCH_RST (0) // Extension IO2

#define WS_BOARD_SDMMC_EN (0) // Extension IO4
#define WS_BOARD_SDMMC_SCK (2)
#define WS_BOARD_SDMMC_CMD (1)
#define WS_BOARD_SDMMC_DAT (42)

#define WS_BOARD_ADC_DET (4)
