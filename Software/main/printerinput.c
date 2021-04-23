#include "printerinput.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define EN_A 4
#define EN_B 13
#define EN_BTN 23

void printerInputInit() {
	gpio_reset_pin(EN_A);
	gpio_set_direction(EN_A, GPIO_MODE_OUTPUT);
	gpio_set_level(EN_A, 0);
	gpio_pullup_dis(EN_A);
	gpio_reset_pin(EN_B);
	gpio_set_direction(EN_B, GPIO_MODE_OUTPUT);
	gpio_set_level(EN_B, 0);
	gpio_pullup_dis(EN_B);
	gpio_reset_pin(EN_BTN);
	gpio_set_direction(EN_BTN, GPIO_MODE_OUTPUT);
	gpio_set_level(EN_BTN, 0);
	gpio_pullup_dis(EN_BTN);
}

void printerInputLeft() {
	gpio_set_level(EN_A, 1);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_B, 1);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_A, 0);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_B, 0);
}
void printerInputRight() {
	gpio_set_level(EN_B, 1);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_A, 1);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_B, 0);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_A, 0);
}
void printerInputClick() {
	gpio_set_level(EN_BTN, 1);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_BTN, 0);
}
