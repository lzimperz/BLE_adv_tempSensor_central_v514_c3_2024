#ifndef SENSOR_HUB_H_
#define SENSOR_HUB_H_

#define HIDE_BLE_SCAN_RESULTS   // Si se comenta esta linea, envia al monitor todos los datos completos del escaneo.
#define USE_WHITELIST_FILTER    // Si se comenta esta linea, no usa whitelist y escanea a cualquier dispositivo.


/************************************************************************/
/* Aquí se deben configurar las direcciones fisicas del periferico BLE  */
/* de nuestros sensores.                                                */
/* Mirando el monitor del sensor, se puede obtener la MAC con este      */
/* formato: fc f5 c4 0e cc 62.                                          */
/* Debe transformarse para que quede como se ve a continuacion,         */
/*  agregando "0x" y ",", asi:  { 0xfc, 0xf5, 0xc4, 0x0e, 0xcc, 0x62 }  */
/************************************************************************/
#define TEMP_SENSOR_1_BLE_ADDR      { 0xa4, 0xcf, 0x12, 0x1b, 0xd4, 0x46 }
#define TEMP_SENSOR_2_BLE_ADDR      { 0xfc, 0xf5, 0xc4, 0x0e, 0xcc, 0x62 }

#define MAX_SENSORS_QUANTITY        10


/************************************************************************/
/* Esta structura, utilizada de este modo, solamente define un tipo de  */
/* datos, no simula un objeto.                                          */
/* Simplemente organiza datos.                                          */
/************************************************************************/
typedef struct 
{
    float temp;
    uint8_t ble_addr[8];
    uint8_t registered;
} tempSensor_t;


/************************************************************************/
/* La siguiente estructura simula un objeto en C                        */
/*                                                                      */
/* En este caso representa un hub que recibe temperaturas de sensores   */
/* Las propiedades estan implementadas comp punteros a variables del .c */
/* Los metodos son punteros a funciones definidas dentro del .c         */
/************************************************************************/
typedef struct
{
    // Hub props
    tempSensor_t *tempSensor;
    uint8_t *sensorCount;

    // Hub Functions
    void (*initialize)(void);
    int8_t (*add_sensor)(uint8_t *sensorBleAddr);
    void (*update_sensor_data)(uint8_t *bleAddr, uint8_t *data, uint8_t data_len);
    void (*register_white_list)(void);
} sensorHub_t;

/************************************************************************/
/* La declaración de esta variable se realiza dentro del .c             */
/*                                                                      */
/* Agregar este "extern" en el archivo de cabecera, permite que         */
/* cualquier .c que lo incluya con #include, tenga acceso al "objeto".  */
/************************************************************************/
extern const sensorHub_t sensorHub;


#endif /* SENSOR_HUB_H_ */