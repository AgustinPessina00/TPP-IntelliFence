#ifndef MODULES_GPS_SAM_M10Q_KEYID_H_
#define MODULES_GPS_SAM_M10Q_KEYID_H_

#include <cstdint>
#include <cstddef>

// ========================
// Constantes y contadores
// ========================
constexpr size_t M10Q_NUM_RATE_OPTIONS   = 4;   // STOP, SLOW, MEDIUM, FAST
constexpr size_t M10Q_NUM_DATA_ELEMENTS  = 45;  // Cantidad de elementos en m10q_data
constexpr size_t M10Q_NUM_CK_PAIRS       = 88;  // Un par de bytes por cada data element

// ======================================================
// RATES: m10q_new_acq_time (payloads)  [4 elementos]
//        m10q_new_adq_time_checksum (tal cual tu header)
// ======================================================
// Payloads (5 bytes c/u)
static const uint8_t m10q_new_acq_time_0[] = {0x1F, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_new_acq_time_1[] = {0x01, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_new_acq_time_2[] = {0x20, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_new_acq_time_3[] = {0x05, 0x00, 0x31, 0x10, 0x01};

static const uint8_t* const m10q_new_acq_time[M10Q_NUM_RATE_OPTIONS] = {
    m10q_new_acq_time_0,
    m10q_new_acq_time_1,
    m10q_new_acq_time_2,
    m10q_new_acq_time_3
};
static const uint8_t m10q_new_acq_time_len[M10Q_NUM_RATE_OPTIONS] = {5, 5, 5, 5};

// “Checksums” tal como figura en tu archivo (mismos 5 bytes que payloads)
static const uint8_t m10q_new_acq_ck_0[] = {0x1F, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_new_acq_ck_1[] = {0x01, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_new_acq_ck_2[] = {0x20, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_new_acq_ck_3[] = {0x05, 0x00, 0x31, 0x10, 0x01};

static const uint8_t* const m10q_new_acq_ck[M10Q_NUM_RATE_OPTIONS] = {
    m10q_new_acq_ck_0,
    m10q_new_acq_ck_1,
    m10q_new_acq_ck_2,
    m10q_new_acq_ck_3
};
static const uint8_t m10q_new_acq_ck_len[M10Q_NUM_RATE_OPTIONS] = {5, 5, 5, 5};

// ======================================================
// m10q_data (43 elementos) → payloads de longitud variable
// m10q_checksum (43 pares de 2 bytes) → checksums precalculados
// ======================================================
// --- Payloads ---
static const uint8_t m10q_data_00[] = {0x1F, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-GPS_ENA
static const uint8_t m10q_data_01[] = {0x01, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-GPS_L1CA_ENA
static const uint8_t m10q_data_02[] = {0x20, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-SBAS_ENA
static const uint8_t m10q_data_03[] = {0x05, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-SBAS_L1CA_ENA
static const uint8_t m10q_data_04[] = {0x21, 0x00, 0x31, 0x10, 0x00}; // SIGNAL-GAL_ENA
static const uint8_t m10q_data_05[] = {0x07, 0x00, 0x31, 0x10, 0x00}; // SIGNAL-GAL_E1_ENA
static const uint8_t m10q_data_06[] = {0x22, 0x00, 0x31, 0x10, 0x00}; // SIGNAL-BDS_ENA
static const uint8_t m10q_data_07[] = {0x0D, 0x00, 0x31, 0x10, 0x00}; // SIGNAL-BDS_B1_ENA
static const uint8_t m10q_data_08[] = {0x0F, 0x00, 0x31, 0x10, 0x00}; // SIGNAL-BDS_B1C_ENA
static const uint8_t m10q_data_09[] = {0x24, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-QZSS_ENA
static const uint8_t m10q_data_10[] = {0x12, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-QZSS_L1CA_ENA
static const uint8_t m10q_data_11[] = {0x14, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-QZSS_L1S_ENA
static const uint8_t m10q_data_12[] = {0x25, 0x00, 0x31, 0x10, 0x00}; // SIGNAL-GLO_ENA
static const uint8_t m10q_data_13[] = {0x18, 0x00, 0x31, 0x10, 0x00}; // SIGNAL-GLO_L1_ENA
static const uint8_t m10q_data_14[] = {0x21, 0x00, 0x11, 0x20, 0x03}; // NAVSPG-DYNMODEL
static const uint8_t m10q_data_15[] = {0x05, 0x00, 0x22, 0x20, 0x00}; // ODO-PROFILE
static const uint8_t m10q_data_16[] = {0xB3, 0x00, 0x11, 0x30, 0x14, 0x00}; // NAVSPG-OUTFIL_PACC
static const uint8_t m10q_data_17[] = {0xBA, 0x00, 0x91, 0x20, 0x01}; // MSGOUT-NMEA_ID_GGA_I2C
static const uint8_t m10q_data_18[] = {0xBE, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GGA_SPI
static const uint8_t m10q_data_19[] = {0xBB, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GGA_UART1
static const uint8_t m10q_data_20[] = {0xAB, 0x00, 0x91, 0x20, 0x01}; // MSGOUT-NMEA_ID_RMC_I2C
static const uint8_t m10q_data_21[] = {0xAF, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_RMC_SPI
static const uint8_t m10q_data_22[] = {0xAC, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_RMC_UART1
static const uint8_t m10q_data_23[] = {0xC9, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GLL_I2C
static const uint8_t m10q_data_24[] = {0xCD, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GLL_SPI
static const uint8_t m10q_data_25[] = {0xCA, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GLL_UART1
static const uint8_t m10q_data_26[] = {0xBF, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSA_I2C
static const uint8_t m10q_data_27[] = {0xC3, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSA_SPI
static const uint8_t m10q_data_28[] = {0xC0, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSA_UART1
static const uint8_t m10q_data_29[] = {0xC4, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSV_I2C
static const uint8_t m10q_data_30[] = {0xC8, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSV_SPI
static const uint8_t m10q_data_31[] = {0xC5, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSV_UART1
static const uint8_t m10q_data_32[] = {0xB0, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_VTG_I2C
static const uint8_t m10q_data_33[] = {0xB4, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_VTG_SPI
static const uint8_t m10q_data_34[] = {0xB1, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_VTG_UART1
static const uint8_t m10q_data_35[] = {0x02, 0x00, 0xD0, 0x40, 0x3C, 0x00, 0x00, 0x00}; // PM-POSUPDATEPERIOD
static const uint8_t m10q_data_36[] = {0x03, 0x00, 0xD0, 0x40, 0x0A, 0x00, 0x00, 0x00}; // PM-ACQPERIOD
static const uint8_t m10q_data_37[] = {0x05, 0x00, 0xD0, 0x30, 0x05, 0x00}; // PM-ONTIME
static const uint8_t m10q_data_38[] = {0x06, 0x00, 0xD0, 0x20, 0x02}; // PM-MINACQTIME
static const uint8_t m10q_data_39[] = {0x07, 0x00, 0xD0, 0x20, 0x0A}; // PM-MAXACQTIME
static const uint8_t m10q_data_40[] = {0x09, 0x00, 0xD0, 0x10, 0x01}; // PM-WAITTIMEFIX
static const uint8_t m10q_data_41[] = {0x0C, 0x00, 0xD0, 0x10, 0x01}; // PM-EXTINTWAKE
static const uint8_t m10q_data_42[] = {0x01, 0x00, 0xD0, 0x20, 0x01}; // PM-OPERATEMODE
static const uint8_t m10q_data_43[] = {0x03, 0x00, 0x51, 0x10, 0x01}; // CFG-I2C-ENABLED - RAM
static const uint8_t m10q_data_44[] = {0x02, 0x00, 0x74, 0x10, 0x00}; // CFG-I2C-ENABLED - RAM
static const uint8_t m10q_data_45[] = {0x02, 0x00, 0x74, 0x10, 0x01}; // CFG-I2C-ENABLED - RAM

// Tabla de punteros a payloads
static const uint8_t* const m10q_data_payloads[M10Q_NUM_DATA_ELEMENTS] = {
    m10q_data_43,m10q_data_00, m10q_data_01, m10q_data_02, m10q_data_03, m10q_data_04,
    m10q_data_05, m10q_data_06, m10q_data_07, m10q_data_08, m10q_data_09,
    m10q_data_10, m10q_data_11, m10q_data_12, m10q_data_13, m10q_data_14,
    m10q_data_15, m10q_data_16, m10q_data_17, m10q_data_18, m10q_data_19,
    m10q_data_20, m10q_data_21, m10q_data_22, m10q_data_23, m10q_data_24,
    m10q_data_25, m10q_data_26, m10q_data_27, m10q_data_28, m10q_data_29,
    m10q_data_30, m10q_data_31, m10q_data_32, m10q_data_33, m10q_data_34,
    m10q_data_35, m10q_data_36, m10q_data_37, m10q_data_38, m10q_data_39,
    m10q_data_40, m10q_data_41, m10q_data_42
};

#endif /* MODULES_GPS_SAM_M10Q_KEYID_H_ */
