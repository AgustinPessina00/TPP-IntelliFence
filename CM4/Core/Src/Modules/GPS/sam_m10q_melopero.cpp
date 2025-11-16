/**
 * @file sam_m10q_melopero.cpp
 * @brief GPS SAM-M10Q implementation based on Melopero approach
 * @details Implementation of UBX protocol GPS communication following
 *          the Melopero SAM-M8Q library methodology
 */

#include "sam_m10q_melopero.h"
#include "I2CManager.h"
#include <string.h>
#include <stdio.h>

// Static instance for C interface
static SamM10qMelopero* g_gpsInstance = nullptr;

/**
 * @brief Constructor
 */
SamM10qMelopero::SamM10qMelopero(I2CBus* i2cBus, uint8_t address) 
    : i2cBus(i2cBus), i2cAddress(address), isInitialized(false) {
    
    // Initialize navigation data to zero
    memset(&navData, 0, sizeof(navData));
    
    // Clear buffers
    memset(rxBuffer, 0, sizeof(rxBuffer));
    memset(txBuffer, 0, sizeof(txBuffer));
}

/**
 * @brief Destructor
 */
SamM10qMelopero::~SamM10qMelopero() {
    isInitialized = false;
}

/**
 * @brief Calculate UBX checksum
 */
void SamM10qMelopero::calculateChecksum(const uint8_t* message, uint16_t length, uint8_t& ckA, uint8_t& ckB) {
    ckA = 0;
    ckB = 0;
    
    for (uint16_t i = 0; i < length; i++) {
        ckA += message[i];
        ckB += ckA;
    }
}

/**
 * @brief Compose UBX message with header and checksum
 */
uint16_t SamM10qMelopero::composeMessage(uint8_t messageClass, uint8_t messageId, 
                                        const uint8_t* payload, uint16_t payloadLength, 
                                        uint8_t* outputBuffer) {
    
    if (payloadLength > (UBX::MAX_MESSAGE_SIZE - UBX::HEADER_SIZE - UBX::CHECKSUM_SIZE)) {
        return 0; // Payload too large
    }
    
    uint16_t index = 0;
    
    // Sync characters
    outputBuffer[index++] = UBX::SYNC_CHAR_1;
    outputBuffer[index++] = UBX::SYNC_CHAR_2;
    
    // Class and ID
    outputBuffer[index++] = messageClass;
    outputBuffer[index++] = messageId;
    
    // Length (little endian)
    outputBuffer[index++] = (uint8_t)(payloadLength & 0xFF);
    outputBuffer[index++] = (uint8_t)((payloadLength >> 8) & 0xFF);
    
    // Payload
    if (payload && payloadLength > 0) {
        memcpy(&outputBuffer[index], payload, payloadLength);
        index += payloadLength;
    }
    
    // Calculate checksum (class + id + length + payload)
    uint8_t ckA, ckB;
    calculateChecksum(&outputBuffer[2], index - 2, ckA, ckB);
    
    // Add checksum
    outputBuffer[index++] = ckA;
    outputBuffer[index++] = ckB;
    
    return index;
}

/**
 * @brief Get number of bytes available to read
 */
uint16_t SamM10qMelopero::getAvailableBytes() {
    uint8_t msb, lsb;
    
    I2CResult result1 = i2cBus->memRead(i2cAddress, UBX::BYTES_AVAILABLE_MSB, 1, &msb, 1, 100);
    I2CResult result2 = i2cBus->memRead(i2cAddress, UBX::BYTES_AVAILABLE_LSB, 1, &lsb, 1, 100);
    
    if (result1 != I2C_OK || result2 != I2C_OK) {
        return 0;
    }
    
    return (uint16_t)((msb << 8) | lsb);
}

/**
 * @brief Write UBX message to GPS
 */
bool SamM10qMelopero::writeMessage(const uint8_t* message, uint16_t length) {
    if (!message || length == 0 || length > UBX::MAX_MESSAGE_SIZE) {
        return false;
    }
    
    I2CResult result = i2cBus->transmit(i2cAddress, message, length, 1000);
    return (result == I2C_OK);
}

/**
 * @brief Read message from GPS
 */
bool SamM10qMelopero::readMessage(uint8_t* buffer, uint16_t maxLength, uint16_t& actualLength) {
    actualLength = 0;
    
    uint16_t availableBytes = getAvailableBytes();
    if (availableBytes == 0) {
        return false;
    }
    
    // Limit read to buffer size
    uint16_t bytesToRead = (availableBytes > maxLength) ? maxLength : availableBytes;
    
    // Read data from data stream register
    for (uint16_t i = 0; i < bytesToRead; i++) {
        I2CResult result = i2cBus->memRead(i2cAddress, UBX::DATA_STREAM_REGISTER, 1, &buffer[i], 1, 100);
        if (result != I2C_OK) {
            return false;
        }
    }
    
    actualLength = bytesToRead;
    return true;
}

/**
 * @brief Wait for specific UBX message
 */
bool SamM10qMelopero::waitForMessage(uint8_t messageClass, uint8_t messageId, uint32_t timeoutMs) {
    uint32_t startTime = HAL_GetTick();
    
    while ((HAL_GetTick() - startTime) < timeoutMs) {
        uint16_t messageLength;
        if (readMessage(rxBuffer, sizeof(rxBuffer), messageLength)) {
            
            // Check for valid UBX message
            if (messageLength >= UBX::HEADER_SIZE + UBX::CHECKSUM_SIZE &&
                rxBuffer[0] == UBX::SYNC_CHAR_1 && 
                rxBuffer[1] == UBX::SYNC_CHAR_2) {
                
                // Check if this is the message we're looking for
                if (rxBuffer[2] == messageClass && rxBuffer[3] == messageId) {
                    return true;
                }
            }
        }
        
        HAL_Delay(10); // Small delay between reads
    }
    
    return false;
}

/**
 * @brief Wait for ACK/NAK message
 */
bool SamM10qMelopero::waitForAcknowledge(uint8_t messageClass, uint8_t messageId, uint32_t timeoutMs) {
    uint32_t startTime = HAL_GetTick();
    
    while ((HAL_GetTick() - startTime) < timeoutMs) {
        uint16_t messageLength;
        if (readMessage(rxBuffer, sizeof(rxBuffer), messageLength)) {
            
            // Check for valid UBX ACK message
            if (messageLength >= 10 && // ACK message is 10 bytes total
                rxBuffer[0] == UBX::SYNC_CHAR_1 && 
                rxBuffer[1] == UBX::SYNC_CHAR_2 &&
                rxBuffer[2] == UBX::ACK_CLASS) {
                
                // Check if ACK/NAK is for our message
                if (rxBuffer[6] == messageClass && rxBuffer[7] == messageId) {
                    return (rxBuffer[3] == UBX::ACK_ACK); // Return true only for ACK, false for NAK
                }
            }
        }
        
        HAL_Delay(10); // Small delay between reads
    }
    
    return false;
}

/**
 * @brief Configure GPS to use UBX protocol only
 */
bool SamM10qMelopero::configureUbxOnly() {
    // UBX CFG-PRT payload for I2C port, UBX only input/output
    uint8_t payload[] = {
        0x00, 0x00, 0x00, 0x00, // Port ID (I2C = 0)
        0x84, 0x00, 0x00, 0x00, // Mode (I2C address 0x42)
        0x00, 0x00, 0x00, 0x00, // Reserved
        0x01, 0x00, 0x01, 0x00, // Input protocols: UBX only
        0x00, 0x00, 0x00, 0x00  // Output protocols: UBX only
    };
    
    uint16_t messageLength = composeMessage(UBX::CFG_CLASS, UBX::CFG_PRT, payload, sizeof(payload), txBuffer);
    
    if (messageLength == 0) {
        return false;
    }
    
    if (!writeMessage(txBuffer, messageLength)) {
        return false;
    }
    
    return waitForAcknowledge(UBX::CFG_CLASS, UBX::CFG_PRT, 1000);
}

/**
 * @brief Set message frequency for specific UBX message
 */
bool SamM10qMelopero::setMessageFrequency(uint8_t messageClass, uint8_t messageId, uint8_t frequency) {
    uint8_t payload[] = {
        messageClass, messageId, frequency, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    uint16_t messageLength = composeMessage(UBX::CFG_CLASS, UBX::CFG_MSG, payload, sizeof(payload), txBuffer);
    
    if (messageLength == 0) {
        return false;
    }
    
    if (!writeMessage(txBuffer, messageLength)) {
        return false;
    }
    
    return waitForAcknowledge(UBX::CFG_CLASS, UBX::CFG_MSG, 1000);
}

/**
 * @brief Set measurement rate
 */
bool SamM10qMelopero::setMeasurementRate(uint16_t measurementPeriodMs, uint16_t navigationRate) {
    uint8_t payload[] = {
        (uint8_t)(measurementPeriodMs & 0xFF),       // Measurement period LSB
        (uint8_t)((measurementPeriodMs >> 8) & 0xFF), // Measurement period MSB
        (uint8_t)(navigationRate & 0xFF),            // Navigation rate LSB
        (uint8_t)((navigationRate >> 8) & 0xFF),     // Navigation rate MSB
        0x00, 0x00                                   // Time reference (UTC)
    };
    
    uint16_t messageLength = composeMessage(UBX::CFG_CLASS, UBX::CFG_RATE, payload, sizeof(payload), txBuffer);
    
    if (messageLength == 0) {
        return false;
    }
    
    if (!writeMessage(txBuffer, messageLength)) {
        return false;
    }
    
    return waitForAcknowledge(UBX::CFG_CLASS, UBX::CFG_RATE, 1000);
}

/**
 * @brief Configure GPS for indoor/weak signal conditions
 */
bool SamM10qMelopero::configureIndoorMode() {
    printf("[GPS] Configurando modo interior/senal debil...\n");
    
    // 1. Set dynamic model to stationary (mejor para uso indoor estatico)
    printf("[GPS] - Configurando modelo dinamico estatico...\n");
    uint8_t navConfig[] = {
        0xFF, 0xFF, // mask (update dynamic model only)
        0x02,       // dynModel: 2 = stationary
        0x03,       // fixMode: 3 = auto 2D/3D
        0x00, 0x00, 0x00, 0x00, // fixedAlt (not used)
        0x10, 0x27, 0x00, 0x00, // fixedAltVar (10000)
        0x05,       // minElev: 5 degrees
        0x00,       // drLimit (dead reckoning limit)
        0xFA, 0x00, // pDop: 25.0
        0xFA, 0x00, // tDop: 25.0
        0x64, 0x00, // pAcc: 100m
        0x2C, 0x01, // tAcc: 300m
        0x00,       // staticHoldThresh
        0x3C,       // dgnssTimeout: 60s
        0x00,       // cnoThreshNumSVs
        0x00,       // cnoThresh
        0x00, 0x00, // reserved
        0x00, 0x00, // staticHoldMaxDist
        0x00,       // utcStandard
        0x00, 0x00, 0x00, 0x00, 0x00 // reserved
    };
    
    uint16_t messageLength = composeMessage(UBX::CFG_CLASS, 0x24, navConfig, sizeof(navConfig), txBuffer); // CFG-NAV5
    if (messageLength > 0 && writeMessage(txBuffer, messageLength)) {
        if (waitForAcknowledge(UBX::CFG_CLASS, 0x24, 2000)) {
            printf("[GPS] OK - Modelo dinamico estatico configurado\n");
        } else {
            printf("[GPS] WARN - Timeout configurando modelo dinamico\n");
        }
    }
    
    HAL_Delay(200);
    
    // 2. Configure GNSS systems (habilitar multiples constelaciones para mejor cobertura)
    printf("[GPS] - Habilitando multiples constelaciones GNSS...\n");
    uint8_t gnssConfig[] = {
        0x00,                    // msgVer
        0x20,                    // numTrkChHw (32 channels)
        0x20,                    // numTrkChUse (use all)
        0x04,                    // numConfigBlocks
        // GPS - 8 channels
        0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
        // SBAS - 1 channel
        0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01,
        // Galileo - 4 channels  
        0x02, 0x04, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01,
        // GLONASS - 8 channels
        0x06, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x01
    };
    
    messageLength = composeMessage(UBX::CFG_CLASS, 0x3E, gnssConfig, sizeof(gnssConfig), txBuffer); // CFG-GNSS
    if (messageLength > 0 && writeMessage(txBuffer, messageLength)) {
        if (waitForAcknowledge(UBX::CFG_CLASS, 0x3E, 3000)) {
            printf("[GPS] OK - GPS+SBAS+Galileo+GLONASS habilitados\n");
        } else {
            printf("[GPS] WARN - Timeout configurando GNSS\n");
        }
    }
    
    HAL_Delay(500);
    
    // 3. Enable SBAS for improved accuracy
    printf("[GPS] - Habilitando SBAS para mejor precision...\n");
    uint8_t sbasConfig[] = {
        0x01,       // mode: enabled
        0x07,       // usage: range + diffcorr + integrity
        0x03,       // maxSBAS: 3 channels
        0x00,       // scanmode2: continue searching
        0x00, 0x00, 0x00, 0x00  // scanmode1: auto-scan
    };
    
    messageLength = composeMessage(UBX::CFG_CLASS, 0x16, sbasConfig, sizeof(sbasConfig), txBuffer); // CFG-SBAS
    if (messageLength > 0 && writeMessage(txBuffer, messageLength)) {
        if (waitForAcknowledge(UBX::CFG_CLASS, 0x16, 2000)) {
            printf("[GPS] OK - SBAS habilitado\n");
        } else {
            printf("[GPS] WARN - Timeout configurando SBAS\n");
        }
    }
    
    HAL_Delay(200);
    
    printf("[GPS] Configuracion interior completada\n");
    return true;
}

/**
 * @brief Parse NAV-PVT message payload
 */
bool SamM10qMelopero::parseNavPvtMessage(const uint8_t* payload, uint16_t length) {
    if (!payload || length < 92) { // NAV-PVT message is 92 bytes
        return false;
    }
    
    // Parse time information
    navData.iTOW = *((uint32_t*)&payload[0]);
    navData.year = *((uint16_t*)&payload[4]);
    navData.month = payload[6];
    navData.day = payload[7];
    navData.hour = payload[8];
    navData.minute = payload[9];
    navData.second = payload[10];
    
    // Parse validity flags
    uint8_t validFlags = payload[11];
    navData.validDate = (validFlags & 0x01) != 0;
    navData.validTime = (validFlags & 0x02) != 0;
    navData.fullyResolved = (validFlags & 0x04) != 0;
    navData.validMag = (validFlags & 0x08) != 0;
    
    // Parse fix information
    navData.fixType = (GpsFixType)payload[20];
    navData.numSV = payload[23];
    
    // Parse position (scaled integers)
    navData.longitude = *((int32_t*)&payload[24]);
    navData.latitude = *((int32_t*)&payload[28]);
    navData.height = *((int32_t*)&payload[32]);
    navData.hMSL = *((int32_t*)&payload[36]);
    
    // Parse accuracy
    navData.hAcc = *((uint32_t*)&payload[40]);
    navData.vAcc = *((uint32_t*)&payload[44]);
    
    // Parse velocity
    navData.velN = *((int32_t*)&payload[48]);
    navData.velE = *((int32_t*)&payload[52]);
    navData.velD = *((int32_t*)&payload[56]);
    navData.gSpeed = *((int32_t*)&payload[60]);
    
    return true;
}

/**
 * @brief Initialize GPS module
 */
bool SamM10qMelopero::initialize() {
    if (!i2cBus) {
        return false;
    }
    
    // Test basic connectivity first
    if (!testConnectivity()) {
        return false;
    }
    
    // Small delay after connectivity test
    HAL_Delay(100);
    
    isInitialized = true;
    return true;
}

/**
 * @brief Configure GPS with Melopero approach
 */
bool SamM10qMelopero::configure(uint16_t measurementRateMs) {
    if (!isInitialized) {
        return false;
    }
    
    // Step 1: Configure UBX-only protocol
    printf("[GPS] Configurando protocolo UBX...\n");
    if (!configureUbxOnly()) {
        printf("[GPS] ERROR - Fallo configuracion UBX\n");
        return false;
    }
    printf("[GPS] OK - Protocolo UBX configurado\n");
    
    HAL_Delay(100);
    
    // Step 2: Configure indoor mode for better signal acquisition
    printf("[GPS] === CONFIGURACION PARA INTERIORES ===\n");
    if (!configureIndoorMode()) {
        printf("[GPS] WARN - Configuracion interior parcial\n");
    }
    
    // Step 3: Enable NAV-PVT messages
    printf("[GPS] Habilitando mensajes NAV-PVT...\n");
    if (!setMessageFrequency(UBX::NAV_CLASS, UBX::NAV_PVT, 1)) {
        printf("[GPS] ERROR - Fallo configuracion NAV-PVT\n");
        return false;
    }
    printf("[GPS] OK - Mensajes NAV-PVT habilitados\n");
    
    HAL_Delay(100);
    
    // Step 4: Set measurement rate (slower for indoor use)
    uint16_t indoorRate = (measurementRateMs < 5000) ? 5000 : measurementRateMs; // Min 5 seconds for indoor
    printf("[GPS] Configurando frecuencia optimizada para interiores (%dms)...\n", indoorRate);
    if (!setMeasurementRate(indoorRate, 1)) {
        printf("[GPS] ERROR - Fallo configuracion de frecuencia\n");
        return false;
    }
    printf("[GPS] OK - Frecuencia configurada para interiores\n");
    
    HAL_Delay(1000); // Allow GPS to settle with new configuration
    
    printf("[GPS] === CONFIGURACION COMPLETADA ===\n");
    printf("[GPS] INFO - GPS configurado para uso en interiores\n");
    printf("[GPS] INFO - Cold start puede tomar 10-15 minutos\n");
    printf("[GPS] INFO - Coloque antena cerca de ventana para mejor recepcion\n");
    
    return true;
}

/**
 * @brief Get navigation data
 */
bool SamM10qMelopero::getNavigationData(bool polling, uint32_t timeoutMs) {
    if (!isInitialized) {
        return false;
    }
    
    if (polling) {
        // Send NAV-PVT poll message
        uint16_t messageLength = composeMessage(UBX::NAV_CLASS, UBX::NAV_PVT, nullptr, 0, txBuffer);
        if (messageLength == 0 || !writeMessage(txBuffer, messageLength)) {
            return false;
        }
    }
    
    // Wait for NAV-PVT response
    uint32_t startTime = HAL_GetTick();
    
    while ((HAL_GetTick() - startTime) < timeoutMs) {
        uint16_t messageLength;
        if (readMessage(rxBuffer, sizeof(rxBuffer), messageLength)) {
            
            // Check for valid NAV-PVT message
            if (messageLength >= 98 && // NAV-PVT with header and checksum is 98 bytes
                rxBuffer[0] == UBX::SYNC_CHAR_1 && 
                rxBuffer[1] == UBX::SYNC_CHAR_2 &&
                rxBuffer[2] == UBX::NAV_CLASS &&
                rxBuffer[3] == UBX::NAV_PVT) {
                
                // Verify checksum
                uint8_t ckA, ckB;
                calculateChecksum(&rxBuffer[2], messageLength - 4, ckA, ckB);
                
                if (rxBuffer[messageLength-2] == ckA && rxBuffer[messageLength-1] == ckB) {
                    // Parse the payload (skip header)
                    return parseNavPvtMessage(&rxBuffer[6], messageLength - 8);
                }
            }
        }
        
        HAL_Delay(50); // Polling interval
    }
    
    return false;
}

/**
 * @brief Get fix type as string
 */
const char* SamM10qMelopero::getFixTypeString() const {
    switch (navData.fixType) {
        case GpsFixType::NO_FIX: return "No Fix";
        case GpsFixType::DEAD_RECKONING: return "Dead Reckoning";
        case GpsFixType::FIX_2D: return "2D Fix";
        case GpsFixType::FIX_3D: return "3D Fix";
        case GpsFixType::GNSS_DEAD_RECKONING: return "GNSS+DR";
        case GpsFixType::TIME_ONLY: return "Time Only";
        default: return "Unknown";
    }
}

/**
 * @brief Test GPS connectivity
 */
bool SamM10qMelopero::testConnectivity() {
    uint8_t dummy;
    I2CResult result = i2cBus->memRead(i2cAddress, UBX::DATA_STREAM_REGISTER, 1, &dummy, 1, 100);
    return (result == I2C_OK || result == I2C_NACK); // NACK is acceptable for empty buffer
}

/**
 * @brief Send custom UBX message
 */
bool SamM10qMelopero::sendUbxMessage(uint8_t messageClass, uint8_t messageId, 
                                    const uint8_t* payload, uint16_t payloadLength) {
    uint16_t messageLength = composeMessage(messageClass, messageId, payload, payloadLength, txBuffer);
    
    if (messageLength == 0) {
        return false;
    }
    
    return writeMessage(txBuffer, messageLength);
}

/**
 * @brief Poll for specific UBX message
 */
bool SamM10qMelopero::pollUbxMessage(uint8_t messageClass, uint8_t messageId,
                                    uint8_t* responseBuffer, uint16_t maxResponseSize,
                                    uint16_t& actualResponseSize, uint32_t timeoutMs) {
    
    // Send poll message
    if (!sendUbxMessage(messageClass, messageId)) {
        return false;
    }
    
    // Wait for response
    uint32_t startTime = HAL_GetTick();
    
    while ((HAL_GetTick() - startTime) < timeoutMs) {
        uint16_t messageLength;
        if (readMessage(rxBuffer, sizeof(rxBuffer), messageLength)) {
            
            // Check for requested message
            if (messageLength >= UBX::HEADER_SIZE + UBX::CHECKSUM_SIZE &&
                rxBuffer[0] == UBX::SYNC_CHAR_1 && 
                rxBuffer[1] == UBX::SYNC_CHAR_2 &&
                rxBuffer[2] == messageClass &&
                rxBuffer[3] == messageId) {
                
                // Verify checksum
                uint8_t ckA, ckB;
                calculateChecksum(&rxBuffer[2], messageLength - 4, ckA, ckB);
                
                if (rxBuffer[messageLength-2] == ckA && rxBuffer[messageLength-1] == ckB) {
                    // Copy response to user buffer
                    uint16_t copyLength = (messageLength > maxResponseSize) ? maxResponseSize : messageLength;
                    memcpy(responseBuffer, rxBuffer, copyLength);
                    actualResponseSize = copyLength;
                    return true;
                }
            }
        }
        
        HAL_Delay(10);
    }
    
    actualResponseSize = 0;
    return false;
}

/**
 * @brief Print satellite signal quality information
 */
void SamM10qMelopero::printSatelliteInfo(uint8_t maxSatellites) {
    printf("[GPS] === INFORMACION DE SATELITES ===\n");
    
    // Poll NAV-SAT message for satellite info
    uint8_t responseBuffer[256];
    uint16_t responseSize = 0;
    
    if (pollUbxMessage(UBX::NAV_CLASS, 0x35, responseBuffer, sizeof(responseBuffer), responseSize, 3000)) {
        if (responseSize >= 12) {
            uint8_t numSvs = responseBuffer[11]; // Number of satellites at offset 11
            printf("[GPS] Satelites visibles: %d\n", numSvs);
            
            uint8_t satellitesToShow = (numSvs > maxSatellites) ? maxSatellites : numSvs;
            
            printf("[GPS] | ID | GNSS | CNO | Elev | Azim | Estado |\n");
            printf("[GPS] |----|------|-----|------|------|---------|\n");
            
            // Parse satellite info (each satellite takes 12 bytes starting at offset 12)
            for (uint8_t i = 0; i < satellitesToShow; i++) {
                if (12 + (i + 1) * 12 <= responseSize) {
                    uint8_t* svInfo = &responseBuffer[12 + i * 12];
                    uint8_t gnssId = svInfo[0];   // GNSS identifier
                    uint8_t svId = svInfo[1];     // Satellite identifier
                    uint8_t cno = svInfo[2];      // Signal strength (C/N0)
                    int8_t elev = (int8_t)svInfo[3];    // Elevation
                    int16_t azim = (int16_t)((svInfo[5] << 8) | svInfo[4]); // Azimuth
                    uint32_t flags = (uint32_t)((svInfo[11] << 24) | (svInfo[10] << 16) | (svInfo[9] << 8) | svInfo[8]);
                    
                    const char* gnssName = "UNK";
                    switch(gnssId) {
                        case 0: gnssName = "GPS"; break;
                        case 1: gnssName = "SBAS"; break;
                        case 2: gnssName = "GAL"; break;  // Galileo
                        case 3: gnssName = "BDS"; break;  // BeiDou
                        case 5: gnssName = "QZSS"; break;
                        case 6: gnssName = "GLO"; break; // GLONASS
                        default: gnssName = "UNK"; break;
                    }
                    
                    bool healthy = (flags & 0x10) != 0;
                    bool used = (flags & 0x08) != 0;
                    
                    char status[10];
                    if (used && healthy) {
                        strcpy(status, "USADO");
                    } else if (healthy) {
                        strcpy(status, "VISIBLE");
                    } else {
                        strcpy(status, "NO_OK");
                    }
                    
                    printf("[GPS] | %2d | %4s | %2d  | %3d  | %3d  | %7s |\n", 
                           svId, gnssName, cno, elev, azim, status);
                }
            }
            
            // Count satellites being used
            uint8_t usedCount = 0;
            for (uint8_t i = 0; i < numSvs && i < 32; i++) {
                if (12 + (i + 1) * 12 <= responseSize) {
                    uint8_t* svInfo = &responseBuffer[12 + i * 12];
                    uint32_t flags = (uint32_t)((svInfo[11] << 24) | (svInfo[10] << 16) | (svInfo[9] << 8) | svInfo[8]);
                    if (flags & 0x08) usedCount++;
                }
            }
            
            printf("[GPS] Resumen: %d visibles, %d en uso\n", numSvs, usedCount);
            
            if (usedCount >= 4) {
                printf("[GPS] SUCCESS - Suficientes satelites para 3D fix\n");
            } else if (usedCount >= 3) {
                printf("[GPS] INFO - Suficientes satelites para 2D fix\n");
            } else {
                printf("[GPS] WARN - Pocos satelites, sin fix posible\n");
            }
        } else {
            printf("[GPS] WARN - Respuesta NAV-SAT muy corta\n");
        }
    } else {
        printf("[GPS] ERROR - No se pudo obtener informacion de satelites\n");
        printf("[GPS] Esto es normal si el GPS aun esta inicializando\n");
    }
    
    printf("[GPS] =====================================\n");
}

// ========== C Interface Implementation ==========

extern "C" {

bool gps_melopero_init(void) {
    if (g_gpsInstance) {
        delete g_gpsInstance;
    }
    
    // Initialize I2CManager if not already done
    if (!I2CManager::isInitialized()) {
        if (!I2CManager::initializeAll()) {
            return false;
        }
    }
    
    // Create GPS instance
    g_gpsInstance = new SamM10qMelopero(&I2CManager::getBus2(), 0x42);
    
    return g_gpsInstance->initialize();
}

bool gps_melopero_get_data(void) {
    if (!g_gpsInstance) {
        return false;
    }
    
    return g_gpsInstance->getNavigationData(true, 2000);
}

double gps_melopero_get_latitude(void) {
    if (!g_gpsInstance) {
        return 0.0;
    }
    
    return g_gpsInstance->getNavData().getLatitudeDegrees();
}

double gps_melopero_get_longitude(void) {
    if (!g_gpsInstance) {
        return 0.0;
    }
    
    return g_gpsInstance->getNavData().getLongitudeDegrees();
}

bool gps_melopero_has_fix(void) {
    if (!g_gpsInstance) {
        return false;
    }
    
    return g_gpsInstance->hasValidFix();
}

void gps_melopero_test(void) {
    printf("[GPS-MELOPERO] ========== INICIO TEST MELOPERO ==========\n");
    
    // Initialize
    if (!gps_melopero_init()) {
        printf("[GPS-MELOPERO] ERROR - Fallo inicializacion\n");
        return;
    }
    printf("[GPS-MELOPERO] OK - GPS inicializado\n");
    
    // Configure
    if (!g_gpsInstance->configure(1000)) { // 1Hz rate
        printf("[GPS-MELOPERO] ERROR - Fallo configuracion\n");
        return;
    }
    printf("[GPS-MELOPERO] OK - GPS configurado (1Hz)\n");
    
    // Test data acquisition
    printf("[GPS-MELOPERO] Probando adquisicion de datos...\n");
    
    for (int i = 0; i < 10; i++) {
        printf("[GPS-MELOPERO] === Lectura #%d ===\n", i + 1);
        
        if (gps_melopero_get_data()) {
            const GpsNavData& data = g_gpsInstance->getNavData();
            
            printf("[GPS-MELOPERO] Tiempo: %04d/%02d/%02d %02d:%02d:%02d\n",
                   data.year, data.month, data.day, data.hour, data.minute, data.second);
            printf("[GPS-MELOPERO] Fix: %s, Satelites: %d\n", 
                   g_gpsInstance->getFixTypeString(), data.numSV);
            printf("[GPS-MELOPERO] Coordenadas: %.8f, %.8f\n",
                   data.getLatitudeDegrees(), data.getLongitudeDegrees());
            printf("[GPS-MELOPERO] Precision H: %.3fm, V: %.3fm\n",
                   data.getHAccMeters(), data.getVAccMeters());
            
            if (g_gpsInstance->hasValidFix()) {
                printf("[GPS-MELOPERO] SUCCESS - Fix valido obtenido!\n");
            } else {
                printf("[GPS-MELOPERO] INFO - Sin fix satelital\n");
            }
        } else {
            printf("[GPS-MELOPERO] WARN - No se pudieron obtener datos\n");
        }
        
        HAL_Delay(2000); // 2 second interval
    }
    
    printf("[GPS-MELOPERO] ========== FIN TEST MELOPERO ==========\n");
}

} // extern "C"