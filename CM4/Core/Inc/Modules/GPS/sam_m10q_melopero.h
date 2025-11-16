/**
 * @file sam_m10q_melopero.h
 * @brief GPS SAM-M10Q implementation based on Melopero approach
 * @details This implementation follows the Melopero SAM-M8Q library approach
 *          using UBX protocol with proper configuration and acknowledgments
 * 
 * Key features:
 * - UBX-only protocol configuration
 * - NAV-PVT message parsing
 * - Proper ACK/NAK handling
 * - Thread-safe I2C communication
 * - Embedded-friendly (no dynamic allocation)
 */

#ifndef SAM_M10Q_MELOPERO_H
#define SAM_M10Q_MELOPERO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

#ifdef __cplusplus
}
#include "II2C.h"
#include "I2CBus.h"

/**
 * @brief UBX Protocol Constants (based on Melopero implementation)
 */
namespace UBX {
    // Sync characters
    static const uint8_t SYNC_CHAR_1 = 0xB5;
    static const uint8_t SYNC_CHAR_2 = 0x62;
    
    // Message Classes
    static const uint8_t NAV_CLASS = 0x01;  // Navigation messages
    static const uint8_t ACK_CLASS = 0x05;  // Acknowledge messages
    static const uint8_t CFG_CLASS = 0x06;  // Configuration messages
    
    // Navigation Messages
    static const uint8_t NAV_PVT = 0x07;    // Position Velocity Time solution
    
    // Configuration Messages
    static const uint8_t CFG_PRT = 0x00;    // Port configuration
    static const uint8_t CFG_MSG = 0x01;    // Message configuration
    static const uint8_t CFG_RATE = 0x08;   // Rate configuration
    
    // Acknowledge Messages
    static const uint8_t ACK_ACK = 0x01;    // Message acknowledged
    static const uint8_t ACK_NAK = 0x00;    // Message not acknowledged
    
    // Buffer sizes
    static const uint16_t MAX_MESSAGE_SIZE = 256;
    static const uint16_t HEADER_SIZE = 6;  // Sync + Class + ID + Length
    static const uint16_t CHECKSUM_SIZE = 2;
    static const uint16_t DATA_STREAM_REGISTER = 0xFF;
    static const uint16_t BYTES_AVAILABLE_MSB = 0xFD;
    static const uint16_t BYTES_AVAILABLE_LSB = 0xFE;
}

/**
 * @brief GPS Fix Types (from UBX NAV-PVT)
 */
enum class GpsFixType : uint8_t {
    NO_FIX = 0,
    DEAD_RECKONING = 1,
    FIX_2D = 2,
    FIX_3D = 3,
    GNSS_DEAD_RECKONING = 4,
    TIME_ONLY = 5
};

/**
 * @brief GPS Navigation Data Structure (NAV-PVT)
 */
struct GpsNavData {
    // Time information
    uint32_t iTOW;        // GPS time of week (ms)
    uint16_t year;        // Year (UTC)
    uint8_t month;        // Month (UTC) 1-12
    uint8_t day;          // Day (UTC) 1-31
    uint8_t hour;         // Hour (UTC) 0-23
    uint8_t minute;       // Minute (UTC) 0-59
    uint8_t second;       // Second (UTC) 0-60
    
    // Position information (scaled)
    int32_t longitude;    // Longitude (deg * 1e-7)
    int32_t latitude;     // Latitude (deg * 1e-7)
    int32_t height;       // Height above ellipsoid (mm)
    int32_t hMSL;         // Height above mean sea level (mm)
    
    // Accuracy information
    uint32_t hAcc;        // Horizontal accuracy estimate (mm)
    uint32_t vAcc;        // Vertical accuracy estimate (mm)
    
    // Velocity information
    int32_t velN;         // NED north velocity (mm/s)
    int32_t velE;         // NED east velocity (mm/s)
    int32_t velD;         // NED down velocity (mm/s)
    int32_t gSpeed;       // Ground speed (mm/s)
    
    // Status information
    GpsFixType fixType;   // GNSS fix type
    uint8_t numSV;        // Number of satellites used
    
    // Validity flags
    bool validDate;       // Valid UTC date
    bool validTime;       // Valid UTC time
    bool fullyResolved;   // UTC time fully resolved
    bool validMag;        // Valid magnetic declination
    
    // Convenience methods for scaled values
    double getLatitudeDegrees() const { return (double)latitude * 1e-7; }
    double getLongitudeDegrees() const { return (double)longitude * 1e-7; }
    double getHeightMeters() const { return (double)height * 1e-3; }
    double getHMSLMeters() const { return (double)hMSL * 1e-3; }
    double getHAccMeters() const { return (double)hAcc * 1e-3; }
    double getVAccMeters() const { return (double)vAcc * 1e-3; }
};

/**
 * @brief UBX Message structure for embedded handling
 */
struct UbxMessage {
    uint8_t messageClass;
    uint8_t messageId;
    uint16_t length;
    uint8_t payload[UBX::MAX_MESSAGE_SIZE - UBX::HEADER_SIZE - UBX::CHECKSUM_SIZE];
    uint8_t checksumA;
    uint8_t checksumB;
    
    bool isValid() const {
        return (length <= (UBX::MAX_MESSAGE_SIZE - UBX::HEADER_SIZE - UBX::CHECKSUM_SIZE));
    }
};

/**
 * @brief SAM-M10Q GPS class using Melopero approach
 */
class SamM10qMelopero {
private:
    I2CBus* i2cBus;         // Thread-safe I2C bus with transmit support
    uint8_t i2cAddress;     // GPS I2C address (7-bit)
    GpsNavData navData;     // Current navigation data
    bool isInitialized;     // Initialization flag
    
    // Internal buffers (embedded-friendly)
    uint8_t rxBuffer[UBX::MAX_MESSAGE_SIZE];
    uint8_t txBuffer[UBX::MAX_MESSAGE_SIZE];
    
    // Private methods
    bool writeMessage(const uint8_t* message, uint16_t length);
    bool readMessage(uint8_t* buffer, uint16_t maxLength, uint16_t& actualLength);
    uint16_t getAvailableBytes();
    bool waitForMessage(uint8_t messageClass, uint8_t messageId, uint32_t timeoutMs = 1000);
    bool waitForAcknowledge(uint8_t messageClass, uint8_t messageId, uint32_t timeoutMs = 1000);
    
    // UBX protocol helpers
    void calculateChecksum(const uint8_t* message, uint16_t length, uint8_t& ckA, uint8_t& ckB);
    uint16_t composeMessage(uint8_t messageClass, uint8_t messageId, 
                           const uint8_t* payload, uint16_t payloadLength, 
                           uint8_t* outputBuffer);
    bool parseNavPvtMessage(const uint8_t* payload, uint16_t length);
    
    // Configuration helpers
    bool configureUbxOnly();
    bool configureIndoorMode();
    bool setMessageFrequency(uint8_t messageClass, uint8_t messageId, uint8_t frequency);
    bool setMeasurementRate(uint16_t measurementPeriodMs, uint16_t navigationRate);

public:
    /**
     * @brief Constructor
     * @param i2cBus Pointer to thread-safe I2C bus
     * @param address GPS I2C address (7-bit, default 0x42)
     */
    SamM10qMelopero(I2CBus* i2cBus, uint8_t address = 0x42);
    
    /**
     * @brief Destructor
     */
    ~SamM10qMelopero();
    
    /**
     * @brief Initialize the GPS module with Melopero configuration
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Check if GPS module is properly initialized
     * @return true if initialized
     */
    bool isReady() const { return isInitialized; }
    
    /**
     * @brief Configure GPS with optimal settings (following Melopero approach)
     * @param measurementRateMs Measurement period in milliseconds (default 1000ms = 1Hz)
     * @return true if configuration successful
     */
    bool configure(uint16_t measurementRateMs = 1000);
    
    /**
     * @brief Get navigation data (NAV-PVT)
     * @param polling If true, poll for new data; if false, wait for automatic message
     * @param timeoutMs Timeout for data reception
     * @return true if data received and parsed successfully
     */
    bool getNavigationData(bool polling = true, uint32_t timeoutMs = 2000);
    
    /**
     * @brief Get current navigation data structure
     * @return Reference to current navigation data
     */
    const GpsNavData& getNavData() const { return navData; }
    
    /**
     * @brief Check if GPS has a valid fix
     * @return true if GPS has 2D or 3D fix
     */
    bool hasValidFix() const {
        return (navData.fixType == GpsFixType::FIX_2D || 
                navData.fixType == GpsFixType::FIX_3D ||
                navData.fixType == GpsFixType::GNSS_DEAD_RECKONING);
    }
    
    /**
     * @brief Get fix type as string
     * @return String representation of fix type
     */
    const char* getFixTypeString() const;
    
    /**
     * @brief Test GPS connectivity
     * @return true if GPS responds to I2C communication
     */
    bool testConnectivity();
    
    /**
     * @brief Get GPS module version/info (if available)
     * @param buffer Buffer to store version string
     * @param bufferSize Size of buffer
     * @return true if version retrieved successfully
     */
    bool getModuleInfo(char* buffer, uint16_t bufferSize);
    
    /**
     * @brief Send custom UBX message
     * @param messageClass UBX message class
     * @param messageId UBX message ID
     * @param payload Payload data (can be nullptr for polling)
     * @param payloadLength Length of payload
     * @return true if message sent successfully
     */
    bool sendUbxMessage(uint8_t messageClass, uint8_t messageId, 
                       const uint8_t* payload = nullptr, uint16_t payloadLength = 0);
    
    /**
     * @brief Print satellite signal quality information
     * @param maxSatellites Maximum number of satellites to show (default 12)
     */
    void printSatelliteInfo(uint8_t maxSatellites = 12);
    
    /**
     * @brief Poll for specific UBX message
     * @param messageClass UBX message class to poll
     * @param messageId UBX message ID to poll
     * @param responseBuffer Buffer to store response
     * @param maxResponseSize Maximum response buffer size
     * @param actualResponseSize Actual response size received
     * @param timeoutMs Timeout for response
     * @return true if message received successfully
     */
    bool pollUbxMessage(uint8_t messageClass, uint8_t messageId,
                       uint8_t* responseBuffer, uint16_t maxResponseSize,
                       uint16_t& actualResponseSize, uint32_t timeoutMs = 1000);
};

// C interface functions for integration with existing code
extern "C" {
    /**
     * @brief C wrapper for GPS initialization
     * @return true if successful
     */
    bool gps_melopero_init(void);
    
    /**
     * @brief C wrapper for GPS navigation data retrieval
     * @return true if data retrieved successfully
     */
    bool gps_melopero_get_data(void);
    
    /**
     * @brief C wrapper to get latitude in degrees
     * @return Latitude in degrees
     */
    double gps_melopero_get_latitude(void);
    
    /**
     * @brief C wrapper to get longitude in degrees
     * @return Longitude in degrees
     */
    double gps_melopero_get_longitude(void);
    
    /**
     * @brief C wrapper to check if GPS has valid fix
     * @return true if GPS has valid fix
     */
    bool gps_melopero_has_fix(void);
    
    /**
     * @brief C wrapper for comprehensive GPS test
     */
    void gps_melopero_test(void);
}

#endif

#endif // SAM_M10Q_MELOPERO_H