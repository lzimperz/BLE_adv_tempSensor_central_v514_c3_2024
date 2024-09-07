#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_sleep.h"
#include <string.h>

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sensor_hub.h"

#define SENSOR_HUB_LOG_TAG "SENSOR_HUB"

tempSensor_t tempSensor[MAX_SENSORS_QUANTITY];
uint8_t sensorCount;

/************************************************************************/
/* El dispositivo cuenta las veces que se reinicia por software.        */
/*                                                                      */
/* Esto permite realizar inicializaciones solo al momento en que se     */
/* energiza, y no repetirlas en los reinicios por software.             */
/*                                                                      */
/* En esta aplicación no es necesario pero queda a modo de ejemplo      */
/************************************************************************/
static uint32_t RTC_DATA_ATTR restart_counter = 0;

/************************************************************************/
/* Agrega un dispositivo "tempSensor" al array de dispositivos del "hub"*/
/*                                                                      */
/* Cuando lo agrega, lo marca como "dispositivo registrado" para saber  */
/* que lugares de la estructura están ocupados por dispositivos en ser- */
/* vicio.                                                               */
/************************************************************************/
int8_t add_sensor(uint8_t *sensorBleAddr)
{

    uint8_t sensorIndex = 0;
    int8_t registeredIndex = -1;

    while (registeredIndex == -1 && sensorIndex < MAX_SENSORS_QUANTITY)
    {
        if (tempSensor[sensorIndex].registered == false)
        {
            strncpy((char *)(tempSensor[sensorIndex].ble_addr), (char *)sensorBleAddr, 6);
            tempSensor[sensorIndex].registered = true;
            tempSensor[sensorIndex].temp = 0;
            registeredIndex = sensorIndex;
            sensorCount++;
        }
        else
            sensorIndex++;
    }
    return registeredIndex;
}

/************************************************************************/
/* Inicializa el "objeto".                                              */
/* Lo implementado dentro del IF, se reinicia unicamente cuando el      */
/* dispositivo es energizado, luego, restart_counter será mayor a 0.    */
/* Esto es útil cuando tenemos que inicializar algo en el dispositivo   */
/* por única vez cuando enciende.                                       */
/*                                                                      */
/* En este caso aproveche para agregar los dos dispositivos definidos   */
/* en el .h con sus MACs, usando add_sensor(). Esto en teoría lo usa-   */
/* ríamos en nuestra aplicación                                         */
/************************************************************************/
static void initialize(void)
{
    ESP_LOGI(SENSOR_HUB_LOG_TAG, "Ingresa a initialize().");
    if (restart_counter == 0)
    {
        ESP_LOGI(SENSOR_HUB_LOG_TAG, "Inicialización en encendido (no en reinicio).");
    }
    ESP_LOGI(SENSOR_HUB_LOG_TAG, "Reinicio numero: %lu", restart_counter);
    restart_counter++;

    sensorCount = 0;

    uint8_t sensor1_addr[] = TEMP_SENSOR_1_BLE_ADDR;
    add_sensor(sensor1_addr);

    uint8_t sensor2_addr[] = TEMP_SENSOR_2_BLE_ADDR;
    add_sensor(sensor2_addr);
}

/************************************************************************/
/* Registra las MACs de nuestros sensores en el stack BLE.              */
/* Para esto usa la funcion esp_ble_gap_update_whitelist(...) de la API */
/* del SDK.                                                             */
/* En el caso de este ejemplo, llamo a esta funcion cuando se genera el */
/* EVENTO de "configuracion completa de parametros BLE", en su corres-  */
/* pondiente manejador, dentro del archivo gattc_demo.c                 */
/************************************************************************/
static void register_white_list(void)
{

    for (int i = 0; i < MAX_SENSORS_QUANTITY; i++)
    {
        if (tempSensor[i].registered == true)
        {
            ESP_LOGI(SENSOR_HUB_LOG_TAG, "Adding to BLE Whitelist:");
            esp_log_buffer_hex(SENSOR_HUB_LOG_TAG, tempSensor[i].ble_addr, 6);
            ESP_ERROR_CHECK(esp_ble_gap_update_whitelist(ESP_BLE_WHITELIST_ADD, tempSensor[i].ble_addr, BLE_WL_ADDR_TYPE_PUBLIC));
            ESP_LOGI(SENSOR_HUB_LOG_TAG, "-- -- -- -- -- --\n");
        }
    }
}

/************************************************************************/
/* Esta funcion, en este ejemplo, la disparo con el evento que se gene- */
/* ra cuando un dispositivo BLE es escaneado y admitido por el filtro   */
/* (con filtro, me refiero a la WHITELIST.                              */
/*                                                                      */
/* Esta funcion recibe la MAC del dispositivo escaneado y la compara    */
/* con las MACs de todos nuestros sensores registrados, para validar    */
/* si existe.                                                           */
/*                                                                      */
/* En caso de existir, toma los datos del paquete de advertising los    */
/* parsea, analiza y copia a las variables de destino, dentro del "hub" */
/*                                                                      */
/* Muestra en el monitor, cuando la temperatura difiere de la anterior. */
/* Se puede descomentar algunas lineas para debug o ver el orden de     */
/* ejecucion                                                            */
/************************************************************************/
static void update_sensor_data(uint8_t *bleAddr, uint8_t *data, uint8_t data_len)
{

    // ESP_LOGI(SENSOR_HUB_LOG_TAG, "Ingresa a funcion sensorHub.update_sensor_data(). ");
    for (int i = 0; i < MAX_SENSORS_QUANTITY; i++)
    {
        if (tempSensor[i].registered == true)
        {

            if (strncmp((char *)tempSensor[i].ble_addr, (char *)bleAddr, 6) == 0)
            {
                // ESP_LOGI(SENSOR_HUB_LOG_TAG, "Llego info sobre sensor registrado. ");

                if (strncmp((char *)data, "TMP", 3) == 0)
                {
                    // ESP_LOGI(SENSOR_HUB_LOG_TAG, "Contiene dato de temperatura. ");

                    char temp_string[5];
                    data += 3;
                    strncpy(temp_string, (char *)data, 5);

                    float received_temp = atof(temp_string);
                    if (tempSensor[i].temp != received_temp)
                    {
                        ESP_LOGI("", "\n");
                        ESP_LOGI(SENSOR_HUB_LOG_TAG, "Nueva temperatura en sensor %d: %s", i, temp_string);
                        esp_log_buffer_hex(SENSOR_HUB_LOG_TAG, tempSensor[i].ble_addr, 6);
                        tempSensor[i].temp = received_temp;
                    }
                }
            }
        }
    }
}

/*****************************************************
 *   Driver Instance Declaration(s) API(s)
 ******************************************************/
const sensorHub_t sensorHub = {
    .tempSensor = tempSensor,
    .sensorCount = &sensorCount,
    .initialize = initialize,
    .add_sensor = add_sensor,
    .register_white_list = register_white_list,
    .update_sensor_data = update_sensor_data};
