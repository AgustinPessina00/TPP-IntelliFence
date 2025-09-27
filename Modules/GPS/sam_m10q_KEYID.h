#ifndef MODULES_GPS_SAM_M10Q_KEYID_H_
#define MODULES_GPS_SAM_M10Q_KEYID_H_

#include <cstdint>
#include <cstddef>

// ========================
// Constantes y contadores
// ========================
constexpr size_t M10Q_NUM_RATE_OPTIONS   = 4;   // STOP, SLOW, MEDIUM, FAST
constexpr size_t M10Q_NUM_DATA_ELEMENTS  = 43;  // Cantidad de elementos en m10q_data
constexpr size_t M10Q_NUM_CK_PAIRS       = 43;  // Un par de bytes por cada data element

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
static const uint8_t m10q_data_00[] = {0x1F, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_data_01[] = {0x01, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_data_02[] = {0x20, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_data_03[] = {0x05, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_data_04[] = {0x21, 0x00, 0x31, 0x10, 0x00};
static const uint8_t m10q_data_05[] = {0x07, 0x00, 0x31, 0x10, 0x00};
static const uint8_t m10q_data_06[] = {0x22, 0x00, 0x31, 0x10, 0x00};
static const uint8_t m10q_data_07[] = {0x0D, 0x00, 0x31, 0x10, 0x00};
static const uint8_t m10q_data_08[] = {0x0F, 0x00, 0x31, 0x10, 0x00};
static const uint8_t m10q_data_09[] = {0x24, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_data_10[] = {0x12, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_data_11[] = {0x14, 0x00, 0x31, 0x10, 0x01};
static const uint8_t m10q_data_12[] = {0x25, 0x00, 0x31, 0x10, 0x00};
static const uint8_t m10q_data_13[] = {0x18, 0x00, 0x31, 0x10, 0x00};
static const uint8_t m10q_data_14[] = {0x21, 0x00, 0x11, 0x20, 0x03};
static const uint8_t m10q_data_15[] = {0x05, 0x00, 0x22, 0x20, 0x00};
static const uint8_t m10q_data_16[] = {0xB3, 0x00, 0x11, 0x30, 0x14, 0x00};
static const uint8_t m10q_data_17[] = {0xBA, 0x00, 0x91, 0x20, 0x01};
static const uint8_t m10q_data_18[] = {0xBE, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_19[] = {0xBB, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_20[] = {0xAB, 0x00, 0x91, 0x20, 0x01};
static const uint8_t m10q_data_21[] = {0xAF, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_22[] = {0xAC, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_23[] = {0xC9, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_24[] = {0xCD, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_25[] = {0xCA, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_26[] = {0xBF, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_27[] = {0xC3, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_28[] = {0xC0, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_29[] = {0xC4, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_30[] = {0xC8, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_31[] = {0xC5, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_32[] = {0xB0, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_33[] = {0xB4, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_34[] = {0xB1, 0x00, 0x91, 0x20, 0x00};
static const uint8_t m10q_data_35[] = {0x02, 0x00, 0xD0, 0x40, 0x3C, 0x00, 0x00, 0x00};
static const uint8_t m10q_data_36[] = {0x03, 0x00, 0xD0, 0x40, 0x0A, 0x00, 0x00, 0x00};
static const uint8_t m10q_data_37[] = {0x05, 0x00, 0xD0, 0x30, 0x05, 0x00};
static const uint8_t m10q_data_38[] = {0x06, 0x00, 0xD0, 0x20, 0x02};
static const uint8_t m10q_data_39[] = {0x07, 0x00, 0xD0, 0x20, 0x0A};
static const uint8_t m10q_data_40[] = {0x09, 0x00, 0xD0, 0x10, 0x01};
static const uint8_t m10q_data_41[] = {0x0C, 0x00, 0xD0, 0x10, 0x01};
static const uint8_t m10q_data_42[] = {0x01, 0x00, 0xD0, 0x20, 0x01};

// Tabla de punteros a payloads
static const uint8_t* const m10q_data_payloads[M10Q_NUM_DATA_ELEMENTS] = {
    m10q_data_00, m10q_data_01, m10q_data_02, m10q_data_03, m10q_data_04,
    m10q_data_05, m10q_data_06, m10q_data_07, m10q_data_08, m10q_data_09,
    m10q_data_10, m10q_data_11, m10q_data_12, m10q_data_13, m10q_data_14,
    m10q_data_15, m10q_data_16, m10q_data_17, m10q_data_18, m10q_data_19,
    m10q_data_20, m10q_data_21, m10q_data_22, m10q_data_23, m10q_data_24,
    m10q_data_25, m10q_data_26, m10q_data_27, m10q_data_28, m10q_data_29,
    m10q_data_30, m10q_data_31, m10q_data_32, m10q_data_33, m10q_data_34,
    m10q_data_35, m10q_data_36, m10q_data_37, m10q_data_38, m10q_data_39,
    m10q_data_40, m10q_data_41, m10q_data_42
};

// Longitudes de cada payload
static const uint8_t m10q_data_len[M10Q_NUM_DATA_ELEMENTS] = {
    5, 5, 5, 5, 5,
    5, 5, 5, 5, 5,
    5, 5, 5, 5, 5,
    5, 6, 5, 5, 5,
    5, 5, 5, 5, 5,
    5, 5, 5, 5, 5,
    5, 5, 5, 5, 5,
    8, 8, 6, 5, 5,
    5, 5, 5
};

// --- Checksums (pares de 2 bytes) ---
static const uint8_t m10q_ck_00[] = {0xFC, 0x89};
static const uint8_t m10q_ck_01[] = {0xFD, 0x91};
static const uint8_t m10q_ck_02[] = {0xDE, 0xF3};
static const uint8_t m10q_ck_03[] = {0xDF, 0xFB};
static const uint8_t m10q_ck_04[] = {0xFD, 0x8E};
static const uint8_t m10q_ck_05[] = {0xFE, 0x96};
static const uint8_t m10q_ck_06[] = {0xE2, 0x07};
static const uint8_t m10q_ck_07[] = {0xE3, 0x0F};
static const uint8_t m10q_ck_08[] = {0xFD, 0x92};
static const uint8_t m10q_ck_09[] = {0xFE, 0x9A};
static const uint8_t m10q_ck_10[] = {0xE3, 0x10};
static const uint8_t m10q_ck_11[] = {0xE4, 0x18};
static const uint8_t m10q_ck_12[] = {0xFE, 0x97};
static const uint8_t m10q_ck_13[] = {0xFF, 0x9F};
static const uint8_t m10q_ck_14[] = {0xE9, 0x2E};
static const uint8_t m10q_ck_15[] = {0xEA, 0x36};
static const uint8_t m10q_ck_16[] = {0xEB, 0x38};
static const uint8_t m10q_ck_17[] = {0xEC, 0x40};
static const uint8_t m10q_ck_18[] = {0x01, 0xA2};
static const uint8_t m10q_ck_19[] = {0x02, 0xAA};
static const uint8_t m10q_ck_20[] = {0xEF, 0x48};
static const uint8_t m10q_ck_21[] = {0xF0, 0x50};
static const uint8_t m10q_ck_22[] = {0xF1, 0x52};
static const uint8_t m10q_ck_23[] = {0xF2, 0x5A};
static const uint8_t m10q_ck_24[] = {0x01, 0xA6};
static const uint8_t m10q_ck_25[] = {0x02, 0xAE};
static const uint8_t m10q_ck_26[] = {0xF4, 0x65};
static const uint8_t m10q_ck_27[] = {0xF5, 0x6D};
static const uint8_t m10q_ck_28[] = {0xF0, 0x55};
static const uint8_t m10q_ck_29[] = {0xF1, 0x5D};
static const uint8_t m10q_ck_30[] = {0xE2, 0xF9};
static const uint8_t m10q_ck_31[] = {0xE3, 0x01};
static const uint8_t m10q_ck_32[] = {0xA4, 0x0F};
static const uint8_t m10q_ck_33[] = {0xA5, 0x18};
static const uint8_t m10q_ck_34[] = {0x07, 0xD0};
static const uint8_t m10q_ck_35[] = {0x08, 0xD8};
static const uint8_t m10q_ck_36[] = {0x0A, 0xE3};
static const uint8_t m10q_ck_37[] = {0x0B, 0xEB};
static const uint8_t m10q_ck_38[] = {0x07, 0xD4};
static const uint8_t m10q_ck_39[] = {0x08, 0xDC};
static const uint8_t m10q_ck_40[] = {0xF8, 0x85};
static const uint8_t m10q_ck_41[] = {0xF9, 0x8D};
static const uint8_t m10q_ck_42[] = {0xFB, 0x98};

// Tabla de punteros a checksums (2 bytes c/u)
static const uint8_t* const m10q_checksum_vals[M10Q_NUM_CK_PAIRS] = {
    m10q_ck_00, m10q_ck_01, m10q_ck_02, m10q_ck_03, m10q_ck_04,
    m10q_ck_05, m10q_ck_06, m10q_ck_07, m10q_ck_08, m10q_ck_09,
    m10q_ck_10, m10q_ck_11, m10q_ck_12, m10q_ck_13, m10q_ck_14,
    m10q_ck_15, m10q_ck_16, m10q_ck_17, m10q_ck_18, m10q_ck_19,
    m10q_ck_20, m10q_ck_21, m10q_ck_22, m10q_ck_23, m10q_ck_24,
    m10q_ck_25, m10q_ck_26, m10q_ck_27, m10q_ck_28, m10q_ck_29,
    m10q_ck_30, m10q_ck_31, m10q_ck_32, m10q_ck_33, m10q_ck_34,
    m10q_ck_35, m10q_ck_36, m10q_ck_37, m10q_ck_38, m10q_ck_39,
    m10q_ck_40, m10q_ck_41, m10q_ck_42
};
// Longitudes (todas 2)
static const uint8_t m10q_checksum_len[M10Q_NUM_CK_PAIRS] = {
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2
};

#endif /* MODULES_GPS_SAM_M10Q_KEYID_H_ */
