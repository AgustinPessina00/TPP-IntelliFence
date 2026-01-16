/* Includes ------------------------------------------------------------------*/

#include "../../Modules/GPS/sam_m10q.h"
#include "stm32wlxx_hal.h"
#include "I2CManager.h"
#include "../../Modules/UART/UARTManager.h"
#include <stdio.h>
#include <cstring>

/* Private includes ----------------------------------------------------------*/
void BusyDelayMs(uint32_t ms);

//static uint8_t buffer[NMEA_BUFFER_SIZE];

// Trivial constructor - does NOT access hardware, allocate memory, or block
SamM10q::SamM10q() {
	this->i2cAddr = 0;
    this->i2cBus = nullptr;
    this->uartBus = nullptr;
	this->version = VALSET_VERSION;
    this->reserved = RESERVED;
    this->initialized = false;
}

/* Must be called after HAL_Init() and MX_I2C_Init() */
bool SamM10q::init(uint8_t i2cAddr) {
    if (initialized) {
        return true; // Already initialized
    }
    
    this->i2cAddr = i2cAddr;
    
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    // Initialize thread-safe I2C bus
    i2cBus = &I2CManager::getBus2();
    if (!i2cBus) {
        return false;
    }
    
    // Initialize thread-safe UART bus
    uartBus = &UARTManager::getUart1();
    if (!uartBus) {
        return false;
    }
    
    configure_gps();
    
    initialized = true;
    return true;
}

void SamM10q::testGPS() {
    //printf("[TEST GPS] Iniciando test de GPS...\n");

    if (!this->update_location_and_time()) {
        //printf("[TEST GPS] Fallo al leer NMEA\n");
        return;
    } else {
		//printf("[TEST GPS] Posición válida: %.6f, %.6f\n", this->latitude, this->longitude);
    }
}

HAL_StatusTypeDef SamM10q::read_gps_position() {
    // Intentar obtener datos PVT del GPS
    if (update_location_and_time()) {
        return HAL_OK;
    } else {
        return HAL_ERROR;
    }
}

/**
 * @brief Actualiza los atributos de ubicación y tiempo de la clase
 * @details Obtiene datos PVT del GPS y actualiza latitude, longitude, fechaUTC y horaUTC
 * @return true si se obtuvieron datos válidos, false en caso contrario
 */
bool SamM10q::update_location_and_time() {
    UBX_NAV_PVT_data_t pvtData;
    
    // Obtener datos PVT del GPS con timeout de 2 segundos
    if (getPVT(&pvtData, 2000)) {
        // Actualizar latitud y longitud (convertir de deg*1e-7 a grados decimales)
        latitude = pvtData.lat * 1e-7;
        longitude = pvtData.lon * 1e-7;
        
        // Actualizar fecha UTC en formato YYMMDD
        fechaUTC = (pvtData.year % 100) * 10000 + 
                   pvtData.month * 100 + 
                   pvtData.day;
        
        // Actualizar hora UTC en formato HHMMSS
        horaUTC = pvtData.hour * 10000 + 
                  pvtData.min * 100 + 
                  pvtData.sec;
        
        return true;
    }
    
    // Si falla, mantener valores anteriores (no los sobrescribimos con 0)
    // Esto evita perder la última posición válida en caso de pérdida temporal de señal
    return false;
}

bool SamM10q::set_new_acq_time(gpsRateSpeed gpsRate) {
    if (gpsRate > gpsRateSpeed::FAST) {
        return false;
    }

    switch (gpsRate)
    {
    case gpsRateSpeed::STOP:
        configure_all_registers(m10q_new_acq_time_stop, M10Q_NUM_RATE_OPTIONS);
        break;
    case gpsRateSpeed::SLOW:
        configure_all_registers(m10q_new_acq_time_slow, M10Q_NUM_RATE_OPTIONS);
        break;
    case gpsRateSpeed::MEDIUM:
        configure_all_registers(m10q_new_acq_time_medium, M10Q_NUM_RATE_OPTIONS);
        break;
    case gpsRateSpeed::FAST:
        configure_all_registers(m10q_new_acq_time_fast, M10Q_NUM_RATE_OPTIONS);
        break;
    default:
        break;
    }

    return true;
}

/* =========================================================== */
/* ========== CONFIGURACION INICIAL DEL GPS VIA I2C ========== */
/* =========================================================== */

void SamM10q::configure_all_registers(const M10QPayload configPayloads[], size_t numPayloads) {
    const uint8_t* payload;
    size_t payload_len;
    for(size_t i = 0; i < numPayloads; i++) {
        payload = configPayloads[i].data;
        payload_len = configPayloads[i].size;
    
        // Escribir en RAM
        if (!write_register(payload, payload_len, RAM)) {
            // Si falla la escritura en RAM, reintentar
            i--;
            continue;
        }
        // Escribir en BBR (persistente)
        if (!write_register(payload, payload_len, BBR)) {
            // Si falla la escritura en BBR, reintentar
            i--;
            continue;
        }
    }
}

void SamM10q::configure_gps() {
    //write_register_uart(m10q_data_payloads[49], sizeof(m10q_data_payloads[49]), RAM); // Deshabilito TRAMAS NMEA UART via UART RAM
    //write_register_uart(m10q_data_payloads[49].data, m10q_data_payloads[49].size, RAM); // Deshabilito TRAMAS NMEA UART via UART RAM

    //write_register(m10q_data_payloads[50], sizeof(m10q_data_payloads[50]), RAM); // Habilito TRAMAS NMEA UART via I2C RAM

    
    //write_register_uart(m10q_data_payloads[54].data, m10q_data_payloads[54].size, RAM); // Habilito TRAMAS UBX UART via UART RAM
    
    write_register_uart(m10q_data_payloads[48].data, m10q_data_payloads[48].size, RAM); // Habilito I2C via UART RAM
    write_register_uart(m10q_data_payloads[48].data, m10q_data_payloads[48].size, BBR); // Habilito I2C via UART BBR

    write_register_uart(m10q_data_payloads[52].data, m10q_data_payloads[52].size, RAM); // Habilita CFG-I2COUTPROT-UBX via uart
    write_register_uart(m10q_data_payloads[52].data, m10q_data_payloads[52].size, BBR);
    
    write_register_uart(m10q_data_payloads[48].data, m10q_data_payloads[48].size, RAM); // Habilita UBX_NAV_PVT_I2C via uart
    write_register_uart(m10q_data_payloads[48].data, m10q_data_payloads[48].size, BBR);

    write_register_uart(m10q_data_payloads[51].data, m10q_data_payloads[51].size, RAM); // Desabilita CFG-I2COUTPROT-NMEA via uart
    write_register_uart(m10q_data_payloads[51].data, m10q_data_payloads[51].size, BBR);

    write_register_uart(m10q_data_payloads[53].data, m10q_data_payloads[53].size, RAM); // Habilita CFG-MSGOUT-UBX_NAV_PVT_UART via uart
    write_register_uart(m10q_data_payloads[53].data, m10q_data_payloads[53].size, BBR);

    // Reemplazo todo lo de abajo con esta función:
    // configure_all_registers(m10q_data_payloads, M10Q_NUM_DATA_ELEMENTS);

    // for(size_t i = 0; i < M10Q_NUM_DATA_ELEMENTS; i++) {
    //     const uint8_t* payload = m10q_data_payloads[i].data;
    //     size_t payload_len = m10q_data_payloads[i].size;
        
    //     // Escribir en RAM
    //     if (!write_register(payload, payload_len, RAM)) {
    //         // Si falla la escritura en RAM, reintentar
    //         i--;
    //         continue;
    //     }
        
    //     // Escribir en BBR (persistente)
    //     if (!write_register(payload, payload_len, BBR)) {
    //         // Si falla la escritura en BBR, reintentar
    //         i--;
    //         continue;
    //     }
    // }
}

/* ============================================================ */
/* ========== CONFIGURACION INICIAL DEL GPS VIA UART ========== */
/* ============================================================ */
void SamM10q::configure_gps_uart() {    
    for(size_t i = 0; i < M10Q_NUM_DATA_ELEMENTS; i++){
        const uint8_t* payload = m10q_data_payloads[i].data;
        size_t payloadlen = m10q_data_payloads[i].size;
        
        // Escribir en RAM via UART
        if (!write_register_uart(payload, payloadlen, RAM)) {
            // Si falla la escritura en RAM, reintentar
            i--;
            continue;
        }
        
        // Escribir en BBR (persistente) via UART
        if (!write_register_uart(payload, payloadlen, BBR)) {
            // Si falla la escritura en BBR, reintentar
            i--;
            continue;
        }
    }
}

/* ================================================================================= */
/* ========== ARMA EL FRAME UBX-VALSET / UBX-VALGET USANDO ARRAY ESTÁTICO ========== */
/* ================================================================================= */
uint16_t SamM10q::build_ubx_message(uint8_t msgClass, uint8_t msgID, uint8_t layer, const uint8_t* payload_data, size_t payload_len, uint8_t* buffer, uint16_t buffer_size) {
    if (!buffer || buffer_size < UBX_MAX_MESSAGE_SIZE) {
        return 0;  // Buffer inválido
    }

    uint16_t idx = 0;

	// 1. Sync chars
    buffer[idx++] = UBX_HEADER1;	// 0xB5
    buffer[idx++] = UBX_HEADER2;	// 0x62

	// 2. Class & ID
    buffer[idx++] = msgClass;
    buffer[idx++] = msgID;

    // 3. Payload length = 4 (version, layer, reserved) + payload size
    uint16_t payloadLength = static_cast<uint16_t>(4 + payload_len);
    buffer[idx++] = static_cast<uint8_t>(payloadLength & 0xFF);			// Little endian LSB
    buffer[idx++] = static_cast<uint8_t>((payloadLength >> 8) & 0xFF);	// Little endian MSB

    // 4. Payload header
    buffer[idx++] = version; // 0x00
    buffer[idx++] = layer;   // RAM o BBR

    // 5. Reserved (2 bytes)
    buffer[idx++] = static_cast<uint8_t>(reserved & 0xFF);			// LSB
    buffer[idx++] = static_cast<uint8_t>((reserved >> 8) & 0xFF);	// MSB

    // 6. Append the actual Key Id (first 4 bytes)
    for (size_t i = 0; i < UBX_KEYID_SIZE; i++) {
        buffer[idx++] = payload_data[i];
    }

    // 7. Append the actual Value if I use VALSET (remaining bytes)
    if(msgID == VALSET_ID) {
        for(size_t i = UBX_KEYID_SIZE; i < payload_len; i++)
        buffer[idx++] = payload_data[i];
    }

    // 8. Calcular el checksum UBX
    uint8_t ck_a = 0, ck_b = 0;
    ubx_calculate_checksum(buffer, idx, &ck_a, &ck_b);
    buffer[idx++] = ck_a;
    buffer[idx++] = ck_b;

	return idx;  // Retorna la longitud total del mensaje
}

void SamM10q::ubx_calculate_checksum(const uint8_t* msg, uint16_t length, uint8_t* ck_a, uint8_t* ck_b) {
    if (!msg || length < 6 || !ck_a || !ck_b) {  // Mínimo: sync(2) + class(1) + id(1) + len(2)
        return;
    }
    
    *ck_a = 0;
    *ck_b = 0;
    
    // Calcular checksum UBX Fletcher sobre Class, ID, Length y Payload
    // Excluir los 2 bytes de sync chars (0xB5 0x62) al inicio
    
    // Class (índice 2)
    *ck_a += msg[2];
    *ck_b += *ck_a;
    
    // ID (índice 3)
    *ck_a += msg[3];
    *ck_b += *ck_a;
    
    // Length LSB (índice 4)
    *ck_a += msg[4];
    *ck_b += *ck_a;
    
    // Length MSB (índice 5)
    *ck_a += msg[5];
    *ck_b += *ck_a;
    
    // Payload (desde índice 6 hasta length-2 para excluir checksum final)
    for (uint16_t i = 6; i < length; i++) {
        *ck_a += msg[i];
        *ck_b += *ck_a;
    }
}

void BusyDelayMs(uint32_t ms) {
    // Usar HAL_Delay que ahora funciona correctamente con TIM2 timebase
    HAL_Delay(ms);
}

/* =================================================================================== */
/* ========== FUNCIONES I2C PARA CONFIGURACIÓN INICIAL LUEGO DE HABILITARLO ========== */
/* =================================================================================== */

/* Escritura de registro usando UBX-CFG-VALSET mediante I2C */
bool SamM10q::write_register(const uint8_t* payload_data, size_t payload_len, uint8_t layer) {
    if (!payload_data || payload_len < UBX_KEYID_SIZE) {
        return false;
    }

    uint8_t message[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALSET con Checksum incluido..
    uint16_t msg_len = build_ubx_message(VALSET_CLASS, VALSET_ID, layer, payload_data, payload_len, message, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }

    // Enviar mensaje
    return send_message(message, msg_len, 15) == HAL_OK;
}

/* Lectura de configuración de KeyID usando UBX-CFG-VALGET mediante I2C*/
bool SamM10q::read_configuration(const uint8_t* payload_data, size_t payload_len, uint8_t* response_buffer, uint16_t buffer_size, uint8_t layer) {
    if (!payload_data || payload_len == 0 || (*response_buffer == 0xFF) || buffer_size == 0) {
        return false;
    }

    uint8_t buffer[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALGET incluido checksum
    uint16_t msg_len = build_ubx_message(VALGET_CLASS, VALGET_ID, layer, payload_data, payload_len, buffer, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }

    // Enviar mensaje de petición
    if (send_message(buffer, msg_len, 15) != HAL_OK) {
        return false;
    }

    HAL_Delay(1000);

    // Leer respuesta del módulo GPS
    if (!i2cBus) {
        return false;
    }

    // Usar bus I2C thread-safe para leer la respuesta
    I2CResult result = i2cBus->memRead(i2cAddr, 0xFD, 1, response_buffer, 1, 100);

    //TODO PESSI: Acá faltaría leer el 0xFE y luego sabiendo la cantidad de bytes para leer, leer el resto de la respuesta desde 0xFF en adelante.

    return (result == I2C_OK);
}

HAL_StatusTypeDef SamM10q::send_message(const uint8_t* message, uint16_t message_length, uint32_t delay_ms) {
    if (!i2cBus || !message || message_length == 0) {
        return HAL_ERROR;
    }

    // Usar método transmit thread-safe del I2CBus para envío directo sin registros
    I2CResult result = i2cBus->memWrite(i2cAddr, 0xFF, 1, message, message_length, 100);
    
    // Delay para que el módulo procese
    BusyDelayMs(delay_ms);

    return (result == I2C_OK) ? HAL_OK : HAL_ERROR;
}

/* =============================================================== */
/* ========== FUNCIONES UART PARA CONFIGURACIÓN INICIAL ========== */
/* =============================================================== */

/* Escritura de registro usando UBX-CFG-VALSET via UART */
bool SamM10q::write_register_uart(const uint8_t* payload_data, size_t payload_len, uint8_t layer) {
    if (!payload_data || payload_len == 0) {
        return false;
    }

    uint8_t buffer[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALSET con checksum incluido.
    uint16_t msg_len = build_ubx_message(VALSET_CLASS, VALSET_ID, layer, payload_data, payload_len, buffer, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }
    
    // Enviar mensaje via UART
    return send_message_uart(buffer, msg_len, 15) == HAL_OK;
}

/* Lectura de configuración de un KeyID usando UBX-CFG-VALGET via UART */
bool SamM10q::read_register_uart(const uint8_t* payload_data, size_t payload_len, uint8_t* response_buffer, uint16_t buffer_size, uint8_t layer) {
    if (!payload_data || payload_len == 0 || !response_buffer || buffer_size == 0) {
        return false;
    }
    uint8_t buffer[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALGET sin checksum (las keys son el payload)
    uint16_t msg_len = build_ubx_message(VALGET_CLASS, VALGET_ID, layer, payload_data, payload_len, buffer, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }

    // Enviar mensaje de petición via UART
    if (send_message_uart(buffer, msg_len, 1000) != HAL_OK) {
        return false;
    }

    // Leer respuesta del módulo GPS via UART
    if (!uartBus) {
        return false;
    }

    // Usar bus UART thread-safe para leer la respuesta disponible
    // El GPS puede enviar mensajes de longitud variable, por lo que usamos receiveAvailable
    uint16_t bytesReceived = 0;
    UARTResult result = uartBus->receiveAvailable(response_buffer, buffer_size, &bytesReceived, 500);
    
    // Opcional: Log de diagnóstico
    printf("[GPS] Recibidos %u bytes, resultado: %d\n", bytesReceived, result);
    
    return (result == UART_OK && bytesReceived > 0);
}

HAL_StatusTypeDef SamM10q::send_message_uart(const uint8_t* message, uint16_t message_length, uint32_t delay_ms) {
    if (!uartBus || !message || message_length == 0) {
        return HAL_ERROR;
    }

    // Usar método transmit thread-safe del UARTBus
    UARTResult result = uartBus->transmit(message, message_length, 100);
    
    // Delay para que el módulo procese
    BusyDelayMs(delay_ms);

    return (result == UART_OK) ? HAL_OK : HAL_ERROR;
}

/* =========================================================== */
/* ============== FUNCIONES PARA OBTENER PVT ================= */
/* =========================================================== */

/**
 * @brief Función principal para obtener datos PVT (Position, Velocity, Time)
 * @param pvtData Puntero a estructura donde se almacenarán los datos
 * @param maxWaitMs Tiempo máximo de espera en milisegundos (default: 1000ms)
 * @return true si se obtuvieron datos válidos, false en caso contrario
 */
bool SamM10q::getPVT(UBX_NAV_PVT_data_t* pvtData, uint32_t maxWaitMs) {
    if (pvtData == nullptr) {
        return false;
    }

    // Limpiar la estructura antes de usarla
    memset(pvtData, 0, sizeof(UBX_NAV_PVT_data_t));

    // 1. Solicitar mensaje PVT al GPS
    if (!requestPVT()) {
        return false;
    }

    //HAL_Delay(1500);

    // 2. Esperar y recibir la respuesta
    if (!receivePVT(pvtData, maxWaitMs)) {
        return false;
    }

    // 3. Verificar que tenemos un fix válido
    // fixType: 0=no fix, 2=2D fix, 3=3D fix
    if (pvtData->fixType < 2) {
        return false; // No hay fix válido
    }

    return true;
}

/**
 * @brief Solicita datos PVT al módulo GPS (polling message)
 * @return true si el mensaje se envió correctamente
 */
bool SamM10q::requestPVT() {
    uint8_t message[8]; // Header(2) + Class(1) + ID(1) + Length(2) + Checksum(2)
    
    // Construir mensaje UBX-NAV-PVT (sin payload, es un polling message)
    message[0] = UBX_HEADER1;  // 0xB5
    message[1] = UBX_HEADER2;  // 0x62
    message[2] = NAV_CLASS;    // 0x01
    message[3] = PVT_ID;       // 0x07
    message[4] = 0x00;         // Length LSB (0 bytes de payload)
    message[5] = 0x00;         // Length MSB
    
    // Calcular checksum sobre el mensaje completo (sin checksum final)
    uint8_t ck_a, ck_b;
    ubx_calculate_checksum(message, 6, &ck_a, &ck_b);
    message[6] = ck_a;
    message[7] = ck_b;
    
    // Enviar mensaje por I2C
    HAL_StatusTypeDef status = send_message(message, 8, 10);
    
    return (status == HAL_OK);
}

/**
 * @brief Recibe y parsea la respuesta PVT del GPS
 * @param pvtData Puntero a estructura donde se almacenarán los datos
 * @param maxWaitMs Tiempo máximo de espera en milisegundos
 * @return true si se recibió y parseó correctamente
 */
bool SamM10q::receivePVT(UBX_NAV_PVT_data_t* pvtData, uint32_t maxWaitMs) {
    // El mensaje UBX-NAV-PVT completo tiene:
    // Header(2) + Class(1) + ID(1) + Length(2) + Payload(92) + Checksum(2) = 100 bytes
    const uint16_t PVT_MESSAGE_SIZE = 100;
    uint8_t buffer[PVT_MESSAGE_SIZE];
    
    uint32_t startTime = HAL_GetTick();
    uint16_t bytesRead = 0;
    
    // Polling: intentar leer hasta que lleguen datos o timeout
    while ((HAL_GetTick() - startTime) < maxWaitMs) {
        // Leer 2 bytes para verificar disponibilidad de datos
        uint8_t bytesAvailable[2] = {10, 10};
        I2CResult result = i2cBus->memRead(i2cAddr, 0xFD, I2C_MEMADD_SIZE_8BIT, bytesAvailable, 2, 100);
        
        if (result != I2C_OK) {
            BusyDelayMs(10);
            continue;
        }
        
        // Los bytes disponibles están en formato little-endian
        uint16_t available = (bytesAvailable[1] << 8) | bytesAvailable[0];
        available &= 0x7FFF; // Limpiar bit 15 (bug conocido del firmware GPS)
        
        // Si hay suficientes bytes disponibles, leer el mensaje completo en chunks de 64 Bytes.
        if (available >= PVT_MESSAGE_SIZE) {
            const uint8_t CHUNK_SIZE = 128;
            uint16_t totalRead = 0;
            bool readSuccess = true;
            
            while (totalRead < PVT_MESSAGE_SIZE) {
                uint8_t toRead = (PVT_MESSAGE_SIZE - totalRead) > CHUNK_SIZE ? CHUNK_SIZE : (PVT_MESSAGE_SIZE - totalRead);
                result = i2cBus->memRead(i2cAddr, 0xFF, I2C_MEMADD_SIZE_8BIT, &buffer[totalRead], toRead, 100);
                
                if (result != I2C_OK) {
                    readSuccess = false;
                    break;
                }
                totalRead += toRead;
            }
            
            if (readSuccess) {
                bytesRead = PVT_MESSAGE_SIZE;
                break;
            }
        }
        
        BusyDelayMs(1); // Esperar 1ms antes de reintentar (optimizado)
    }
    
    if (bytesRead == 0) {
        return false; // Timeout sin recibir datos
    }
    
    // Parsear el mensaje recibido
    return parseUBXMessage(buffer, bytesRead, pvtData);
}

/**
 * @brief Recibe y parsea la respuesta PVT del GPS usando máquina de estados
 * @param pvtData Puntero a estructura donde se almacenarán los datos
 * @param maxWaitMs Tiempo máximo de espera en milisegundos
 * @return true si se recibió y parseó correctamente
 */
bool SamM10q::receivePVTValidateOption(UBX_NAV_PVT_data_t* pvtData, uint32_t maxWaitMs) {
    // Estados de la máquina de estados para parseo UBX
    enum ParseState {
        WAITING_SYNC1,      // Esperando 0xB5
        WAITING_SYNC2,      // Esperando 0x62
        READING_CLASS,      // Leyendo Class
        READING_ID,         // Leyendo ID
        READING_LENGTH_LSB, // Leyendo Length LSB
        READING_LENGTH_MSB, // Leyendo Length MSB
        READING_PAYLOAD,    // Leyendo Payload
        READING_CHECKSUM_A, // Leyendo CK_A
        READING_CHECKSUM_B  // Leyendo CK_B
    };
    
    const uint16_t PVT_MESSAGE_SIZE = 100;
    uint8_t buffer[PVT_MESSAGE_SIZE];
    
    ParseState state = WAITING_SYNC1;
    uint16_t bufferIndex = 0;
    uint16_t payloadLength = 0;
    uint16_t payloadBytesRead = 0;
    
    uint32_t startTime = HAL_GetTick();
    uint32_t lastCheck = HAL_GetTick();
    const uint32_t pollingWait = 100; // 100ms entre checks (apropiado para GPS @ 1Hz)
    
    // Polling con máquina de estados: procesar byte por byte
    while ((HAL_GetTick() - startTime) < maxWaitMs) {
        // Limitar frecuencia de polling según configuración GPS
        if ((HAL_GetTick() - lastCheck) < pollingWait) {
            BusyDelayMs(10); // Pequeño delay para no saturar el bus
            continue;
        }
        lastCheck = HAL_GetTick();
        
        // Verificar disponibilidad de datos
        uint8_t bytesAvailable[2];
        I2CResult result = i2cBus->memRead(i2cAddr, 0xFD, I2C_MEMADD_SIZE_8BIT, bytesAvailable, 2, 100);
        
        if (result != I2C_OK) {
            continue;
        }
        
        uint16_t available = (bytesAvailable[1] << 8) | bytesAvailable[0];
        available &= 0x7FFF; // Limpiar bit 15 (bug conocido del firmware GPS)
        
        if (available == 0) {
            continue;
        }
        
        // Leer bytes disponibles (máximo 64 por iteración según límite del buffer I2C)
        uint8_t readSize = (available > 64) ? 64 : available;
        uint8_t tempBuffer[64];
        result = i2cBus->memRead(i2cAddr, 0xFF, I2C_MEMADD_SIZE_8BIT, tempBuffer, readSize, 100);
        
        if (result != I2C_OK) {
            continue;
        }
        
        // Procesar cada byte con la máquina de estados
        for (uint16_t i = 0; i < readSize; i++) {
            uint8_t byte = tempBuffer[i];
            
            switch (state) {
                case WAITING_SYNC1:
                    if (byte == UBX_HEADER1) {
                        buffer[0] = byte;
                        bufferIndex = 1;
                        state = WAITING_SYNC2;
                    }
                    break;
                    
                case WAITING_SYNC2:
                    if (byte == UBX_HEADER2) {
                        buffer[bufferIndex++] = byte;
                        state = READING_CLASS;
                    } else {
                        state = WAITING_SYNC1; // Reset si no es 0x62
                    }
                    break;
                    
                case READING_CLASS:
                    buffer[bufferIndex++] = byte;
                    state = READING_ID;
                    break;
                    
                case READING_ID:
                    buffer[bufferIndex++] = byte;
                    state = READING_LENGTH_LSB;
                    break;
                    
                case READING_LENGTH_LSB:
                    buffer[bufferIndex++] = byte;
                    payloadLength = byte;
                    state = READING_LENGTH_MSB;
                    break;
                    
                case READING_LENGTH_MSB:
                    buffer[bufferIndex++] = byte;
                    payloadLength |= (byte << 8);
                    
                    // Verificar que el payload es del tamaño esperado (92 bytes para PVT)
                    if (payloadLength == 92 && bufferIndex + payloadLength + 2 <= PVT_MESSAGE_SIZE) {
                        payloadBytesRead = 0;
                        state = READING_PAYLOAD;
                    } else {
                        // Payload inválido, reiniciar
                        state = WAITING_SYNC1;
                        bufferIndex = 0;
                    }
                    break;
                    
                case READING_PAYLOAD:
                    buffer[bufferIndex++] = byte;
                    payloadBytesRead++;
                    
                    if (payloadBytesRead >= payloadLength) {
                        state = READING_CHECKSUM_A;
                    }
                    break;
                    
                case READING_CHECKSUM_A:
                    buffer[bufferIndex++] = byte;
                    state = READING_CHECKSUM_B;
                    break;
                    
                case READING_CHECKSUM_B:
                    buffer[bufferIndex++] = byte;
                    
                    // Mensaje completo recibido, validar y parsear
                    if (buffer[2] == NAV_CLASS && buffer[3] == PVT_ID) {
                        return parseUBXMessage(buffer, bufferIndex, pvtData);
                    } else {
                        // No es PVT, reiniciar y buscar siguiente mensaje
                        state = WAITING_SYNC1;
                        bufferIndex = 0;
                    }
                    break;
            }
            
            // Protección contra overflow del buffer
            if (bufferIndex >= PVT_MESSAGE_SIZE) {
                state = WAITING_SYNC1;
                bufferIndex = 0;
            }
        }
    }
    
    return false; // Timeout sin recibir mensaje válido
}

/**
 * @brief Parsea un mensaje UBX y extrae los datos PVT
 * @param buffer Buffer con el mensaje UBX completo
 * @param bufferLen Longitud del buffer
 * @param pvtData Puntero a estructura donde se almacenarán los datos
 * @return true si el mensaje es válido y se parseó correctamente
 */
bool SamM10q::parseUBXMessage(const uint8_t* buffer, uint16_t bufferLen, UBX_NAV_PVT_data_t* pvtData) {
    // Verificar que el buffer tiene el tamaño mínimo
    if (bufferLen < 100) {
        return false;
    }
    
    // Verificar header UBX
    if (buffer[0] != UBX_HEADER1 || buffer[1] != UBX_HEADER2) {
        return false;
    }
    
    // Verificar que es un mensaje NAV-PVT
    if (buffer[2] != NAV_CLASS || buffer[3] != PVT_ID) {
        return false;
    }
    
    // Verificar longitud del payload (debe ser 92 bytes)
    uint16_t payloadLen = buffer[4] | (buffer[5] << 8);
    if (payloadLen != 92) {
        return false;
    }
    
    // Verificar checksum
    if (!verifyUBXChecksum(buffer, bufferLen)) {
        return false;
    }
    
    // Parsear payload (comienza en buffer[6])
    const uint8_t* payload = &buffer[6];
    uint16_t offset = 0;
    
    // Extraer datos según la especificación UBX-NAV-PVT
    // Nota: Los datos están en formato little-endian
    
    pvtData->iTOW = payload[offset] | (payload[offset+1] << 8) | 
                    (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->year = payload[offset] | (payload[offset+1] << 8);
    offset += 2;
    
    pvtData->month = payload[offset++];
    pvtData->day = payload[offset++];
    pvtData->hour = payload[offset++];
    pvtData->min = payload[offset++];
    pvtData->sec = payload[offset++];
    pvtData->valid = payload[offset++];
    
    pvtData->tAcc = payload[offset] | (payload[offset+1] << 8) | 
                    (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->nano = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                              (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->fixType = payload[offset++];
    pvtData->flags = payload[offset++];
    pvtData->flags2 = payload[offset++];
    pvtData->numSV = payload[offset++];
    
    pvtData->lon = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                             (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->lat = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                             (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->height = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                                (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->hMSL = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                              (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->hAcc = payload[offset] | (payload[offset+1] << 8) | 
                    (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->vAcc = payload[offset] | (payload[offset+1] << 8) | 
                    (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->velN = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                              (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->velE = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                              (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->velD = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                              (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->gSpeed = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                                (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->headMot = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                                 (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->sAcc = payload[offset] | (payload[offset+1] << 8) | 
                    (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->headAcc = payload[offset] | (payload[offset+1] << 8) | 
                       (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->pDOP = payload[offset] | (payload[offset+1] << 8);
    offset += 2;
    
    pvtData->flags3 = payload[offset] | (payload[offset+1] << 8);
    offset += 2;
    
    // Saltar reserved[4]
    offset += 4;
    
    pvtData->headVeh = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                                 (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->magDec = (int16_t)(payload[offset] | (payload[offset+1] << 8));
    offset += 2;
    
    pvtData->magAcc = payload[offset] | (payload[offset+1] << 8);
    
    return true;
}

/**
 * @brief Verifica el checksum de un mensaje UBX
 * @param buffer Buffer con el mensaje UBX completo
 * @param msgLen Longitud del mensaje
 * @return true si el checksum es correcto
 */
bool SamM10q::verifyUBXChecksum(const uint8_t* buffer, uint16_t msgLen) {
    if (msgLen < 8) {
        return false; // Mensaje muy corto
    }
    
    // Calcular checksum sobre Class + ID + Length + Payload
    uint8_t ck_a_calc, ck_b_calc;
    ubx_calculate_checksum(buffer, msgLen - 2, &ck_a_calc, &ck_b_calc);
    
    // Comparar con el checksum recibido (últimos 2 bytes)
    uint8_t ck_a_recv = buffer[msgLen - 2];
    uint8_t ck_b_recv = buffer[msgLen - 1];
    
    return (ck_a_calc == ck_a_recv) && (ck_b_calc == ck_b_recv);
}
