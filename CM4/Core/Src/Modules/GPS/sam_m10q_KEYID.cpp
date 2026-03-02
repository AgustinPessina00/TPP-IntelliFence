/**
 * @file sam_m10q_KEYID.cpp
 * @brief SAM-M10Q GPS configuration data arrays
 * @note All large const arrays moved from header to avoid multiple definitions
 */

#include "sam_m10q_KEYID.h"

/*
 * =======================
 * PSMOO – Power Save Mode (On/Off)
 * =======================
 *
 * En PSMOO el receptor GNSS opera en ciclos ON/OFF para minimizar consumo.
 * El comportamiento general es:
 *
 *   WAKE
 *    └─ Acquisition
 *        ├─ Permanece al menos PM-MINACQTIME
 *        ├─ Intenta conseguir fix hasta PM-MAXACQTIME
 *        └─ Si consigue fix:
 *             └─ (opcional) espera estabilidad si PM-WAITTIMEFIX = 1
 *             └─ Pasa a Tracking
 *                 ├─ Genera soluciones PVT (normalmente a 1 Hz)
 *                 └─ Permanece activo durante PM-ONTIME
 *                     └─ Entra a OFF (PSMOO-OFF)
 *
 * Tras PM-POSUPDATEPERIOD segundos, el ciclo se repite.
 *
 * IMPORTANTE:
 * - Al entrar en PSMOO-OFF se borra la RAM del receptor.
 * - La configuración se recarga desde BBR al siguiente WAKE.
 */

// ======================================================
// RATES: m10q_new_acq_time (payloads)  [4 elementos]
// ======================================================
static const uint8_t m10q_set_continuous_mode[] = {0x01, 0x00, 0xD0, 0x20, 0x00}; // PM-OPERATEMODE -- Continuous
static const uint8_t m10q_set_psmoo_mode[] = {0x01, 0x00, 0xD0, 0x20, 0x01}; // PM-OPERATEMODE -- PSMOO


// ---  Payloads for different acquisition time settings (GREEN ZONE) ---
static const uint8_t m10q_green_zone_0_posupdate[]    = {0x02, 0x00, 0xD0, 0x40, 0x2C, 0x01, 0x00, 0x00}; // POSUPDATEPERIOD = 300 s
static const uint8_t m10q_green_zone_1_acqperiod[]    = {0x03, 0x00, 0xD0, 0x40, 0x64, 0x00, 0x00, 0x00}; // ACQPERIOD       = 100 s (3 intentos de fix)
static const uint8_t m10q_green_zone_2_ontime[]       = {0x05, 0x00, 0xD0, 0x30, 0x0F, 0x00};             // ONTIME          = 15 s
static const uint8_t m10q_green_zone_3_minacq[]       = {0x06, 0x00, 0xD0, 0x20, 0x1E};                   // MINACQTIME      = 30 s
static const uint8_t m10q_green_zone_4_maxacq[]       = {0x07, 0x00, 0xD0, 0x20, 0x1E};                   // MAXACQTIME      = 30 s 
static const uint8_t m10q_green_zone_5_waitfix[]      = {0x09, 0x00, 0xD0, 0x10, 0x01};                   // WAITTIMEFIX     = 1 (primer punto más estable)

const M10QPayload m10q_new_acq_time_green_zone[M10Q_NUM_RATE_OPTIONS] = {
    //{m10q_set_continuous_mode, sizeof(m10q_set_continuous_mode)},
    {m10q_green_zone_0_posupdate, sizeof(m10q_green_zone_0_posupdate)},
    {m10q_green_zone_1_acqperiod, sizeof(m10q_green_zone_1_acqperiod)},
    {m10q_green_zone_2_ontime, sizeof(m10q_green_zone_2_ontime)},
    {m10q_green_zone_3_minacq, sizeof(m10q_green_zone_3_minacq)},
    {m10q_green_zone_4_maxacq, sizeof(m10q_green_zone_4_maxacq)},
    {m10q_green_zone_5_waitfix, sizeof(m10q_green_zone_5_waitfix)},
    {m10q_set_psmoo_mode, sizeof(m10q_set_psmoo_mode)}
    
};

// ---  Payloads for different acquisition time settings (NEAR LIMIT) ---
static const uint8_t m10q_near_limit_0_posupdate[]     = {0x02, 0x00, 0xD0, 0x40, 0x78, 0x00, 0x00, 0x00}; // POSUPDATEPERIOD = 120 s
static const uint8_t m10q_near_limit_1_acqperiod[]     = {0x03, 0x00, 0xD0, 0x40, 0x3C, 0x00, 0x00, 0x00}; // ACQPERIOD       = 60 s (2 intentos de fix)
static const uint8_t m10q_near_limit_2_ontime[]        = {0x05, 0x00, 0xD0, 0x30, 0x0F, 0x00};             // ONTIME          = 15 s
static const uint8_t m10q_near_limit_3_minacq[]        = {0x06, 0x00, 0xD0, 0x20, 0x1E};                   // MINACQTIME      = 30 s
static const uint8_t m10q_near_limit_4_maxacq[]        = {0x07, 0x00, 0xD0, 0x20, 0x1E};                   // MAXACQTIME      = 30 s
static const uint8_t m10q_near_limit_5_waitfix[]       = {0x09, 0x00, 0xD0, 0x10, 0x01};                   // WAITTIMEFIX     = 1

const M10QPayload m10q_new_acq_time_near_limit[M10Q_NUM_RATE_OPTIONS] = {
    //{m10q_set_continuous_mode, sizeof(m10q_set_continuous_mode)},
    {m10q_near_limit_0_posupdate, sizeof(m10q_near_limit_0_posupdate)},
    {m10q_near_limit_1_acqperiod, sizeof(m10q_near_limit_1_acqperiod)},
    {m10q_near_limit_2_ontime, sizeof(m10q_near_limit_2_ontime)},
    {m10q_near_limit_3_minacq, sizeof(m10q_near_limit_3_minacq)},
    {m10q_near_limit_4_maxacq, sizeof(m10q_near_limit_4_maxacq)},
    {m10q_near_limit_5_waitfix, sizeof(m10q_near_limit_5_waitfix)},
    {m10q_set_psmoo_mode, sizeof(m10q_set_psmoo_mode)}
};


// ---  Payloads for different acquisition time settings (CONTINUOUS) ---

const M10QPayload m10q_new_acq_time_continuous[1] = {
    {m10q_set_continuous_mode, sizeof(m10q_set_continuous_mode)}
};

// ======================================================
// m10q_data (45 elementos) → payloads de longitud variable
// m10q_checksum (45 pares de 2 bytes) → checksums precalculados
// ======================================================
// --- Payloads ---
static const uint8_t m10q_data_00[] = {0x03, 0x00, 0x51, 0x10, 0x01}; // CFG-I2C-ENABLED
static const uint8_t m10q_data_01[] = {0x01, 0x00, 0x72, 0x10, 0x01}; // CFG-I2COUTPROT-UBX ENABLED
static const uint8_t m10q_data_02[] = {0x02, 0x00, 0x72, 0x10, 0x00}; // CFG-I2COUTPROT-NMEA DISABLED
static const uint8_t m10q_data_03[] = {0x06, 0x00, 0x91, 0x20, 0x00}; // CFG-MSGOUT-UBX_NAV_PVT_I2C DISABLED -> SE HABILITA PARA PVT SIN REQUEST.
static const uint8_t m10q_data_04[] = {0x07, 0x00, 0x91, 0x20, 0x01}; // CFG-MSGOUT-UBX_NAV_PVT_UART DISABLED -> SE HABILITA PARA DEBUG.
static const uint8_t m10q_data_05[] = {0x1F, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-GPS_ENA
static const uint8_t m10q_data_06[] = {0x01, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-GPS_L1CA_ENA
static const uint8_t m10q_data_07[] = {0x20, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-SBAS_ENA -> LO DESHABILITO, NO APORTA MUCHO EN ARGENTINA.
static const uint8_t m10q_data_08[] = {0x05, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-SBAS_L1CA_ENA -> LO DESHABILITO, NO APORTA MUCHO EN ARGENTINA.
static const uint8_t m10q_data_09[] = {0x21, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-GAL_ENA -> LO HABILITO, GALILEO+GPS EN CAMPO ABIERTO MEJORA LA DISPONIBILIDAD/PRECISION.
static const uint8_t m10q_data_10[] = {0x07, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-GAL_E1_ENA -> LO HABILITO, GALILEO+GPS EN CAMPO ABIERTO MEJORA LA DISPONIBILIDAD/PRECISION.
static const uint8_t m10q_data_11[] = {0x22, 0x00, 0x31, 0x10, 0x00}; // SIGNAL-BDS_ENA
static const uint8_t m10q_data_12[] = {0x0D, 0x00, 0x31, 0x10, 0x00}; // SIGNAL-BDS_B1_ENA -> DESHABILITADO PORQUE NO PUEDE ESTAR AL MISMO TIEMPO QUE B1C (B1C default: 1).
static const uint8_t m10q_data_13[] = {0x0F, 0x00, 0x31, 0x10, 0x00}; // SIGNAL-BDS_B1C_ENA
static const uint8_t m10q_data_14[] = {0x24, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-QZSS_ENA -> LO DESHABILITO. ES REGIONAL ASIA/JAPÓN, NO APORTA NADA EN ARGENTINA.
static const uint8_t m10q_data_15[] = {0x12, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-QZSS_L1CA_ENA -> LO DESHABILITO. ES REGIONAL ASIA/JAPÓN, NO APORTA NADA EN ARGENTINA.
static const uint8_t m10q_data_16[] = {0x14, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-QZSS_L1S_ENA -> LO DESHABILITO. ES REGIONAL ASIA/JAPÓN, NO APORTA NADA EN ARGENTINA.
static const uint8_t m10q_data_17[] = {0x25, 0x00, 0x31, 0x10, 0x00}; // SIGNAL-GLO_ENA -> PODEMOS HABILITARLO EN UN FUTURO PARA VER SI HAY MEJORAS. ES RUSO, NO APORTA MUCHO EN ARGENTINA.
static const uint8_t m10q_data_18[] = {0x18, 0x00, 0x31, 0x10, 0x01}; // SIGNAL-GLO_L1_ENA -> PODEMOS HABILITARLO EN UN FUTURO PARA VER SI HAY MEJORAS. ES RUSO, NO APORTA MUCHO EN ARGENTINA.
static const uint8_t m10q_data_19[] = {0x21, 0x00, 0x11, 0x20, 0x03}; // NAVSPG-DYNMODEL
static const uint8_t m10q_data_20[] = {0x05, 0x00, 0x22, 0x20, 0x00}; // ODO-PROFILE
static const uint8_t m10q_data_21[] = {0xB3, 0x00, 0x11, 0x30, 0x14, 0x00}; // NAVSPG-OUTFIL_PACC -> SI VEMOS QUE FILTRA DEMASIADO Y NO ACTUALIZA LOS VALORES PVT PODEMOS SUBIRLE EL VALOR. AHORA ES 20M.
static const uint8_t m10q_data_22[] = {0xBA, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GGA_I2C -> LO DESHABILITO YA QUE DESHABILITAMOS LAS TRAMAS NMEA POR I2C.
static const uint8_t m10q_data_23[] = {0xBE, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GGA_SPI
static const uint8_t m10q_data_24[] = {0xBB, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GGA_UART1
static const uint8_t m10q_data_25[] = {0xAB, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_RMC_I2C -> LO DESHABILITO YA QUE DESHABILITAMOS LAS TRAMAS NMEA POR I2C.
static const uint8_t m10q_data_26[] = {0xAF, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_RMC_SPI
static const uint8_t m10q_data_27[] = {0xAC, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_RMC_UART1
static const uint8_t m10q_data_28[] = {0xC9, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GLL_I2C
static const uint8_t m10q_data_29[] = {0xCD, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GLL_SPI
static const uint8_t m10q_data_30[] = {0xCA, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GLL_UART1
static const uint8_t m10q_data_31[] = {0xBF, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSA_I2C
static const uint8_t m10q_data_32[] = {0xC3, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSA_SPI
static const uint8_t m10q_data_33[] = {0xC0, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSA_UART1
static const uint8_t m10q_data_34[] = {0xC4, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSV_I2C
static const uint8_t m10q_data_35[] = {0xC8, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSV_SPI
static const uint8_t m10q_data_36[] = {0xC5, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_GSV_UART1
static const uint8_t m10q_data_37[] = {0xB0, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_VTG_I2C
static const uint8_t m10q_data_38[] = {0xB4, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_VTG_SPI
static const uint8_t m10q_data_39[] = {0xB1, 0x00, 0x91, 0x20, 0x00}; // MSGOUT-NMEA_ID_VTG_UART1
static const uint8_t m10q_data_40[] = {0x02, 0x00, 0xD0, 0x40, 0x78, 0x00, 0x00, 0x00}; // PM-POSUPDATEPERIOD -> POSICION CADA 60 SEGUNDOS
static const uint8_t m10q_data_41[] = {0x03, 0x00, 0xD0, 0x40, 0x78, 0x00, 0x00, 0x00}; // PM-ACQPERIOD -> PODRIAMOS LLEVARLO AL MISMO VALOR QUE EL POSUPDATEPERIOD. LO DEJO DE 10S PORQUE ASI EN CAMPO ABIERTO TENGO MAS ACTUALIZACIONES Y NO PERDEMOS EL FIX.
static const uint8_t m10q_data_42[] = {0x05, 0x00, 0xD0, 0x30, 0x1E, 0x00}; // PM-ONTIME -> 30 SEGUNDOS ACTUALMENTE, PODEMOS AUMENTARLO UN POCO AL TIEMPO QUE EL RECEPTOR PERMANECE ENCENDIDO CADA VEZ QUE ENTRA EN UN CICLO DE ADQUISICIÓN PARA NO PERDER MUESTRAS.
static const uint8_t m10q_data_43[] = {0x06, 0x00, 0xD0, 0x20, 0x1E}; // PM-MINACQTIME
static const uint8_t m10q_data_44[] = {0x07, 0x00, 0xD0, 0x20, 0x3C}; // PM-MAXACQTIME -> SI AUMENTAMOS EL PM-ONTIME DEBERIAMOS AUMENTAR EL MAXACQTIME PARA QUE NO CORTARA LA ADQUISICIÓN ANTES DE TIEMPO.
static const uint8_t m10q_data_45[] = {0x09, 0x00, 0xD0, 0x10, 0x01}; // PM-WAITTIMEFIX
static const uint8_t m10q_data_46[] = {0x0C, 0x00, 0xD0, 0x10, 0x01}; // PM-EXTINTWAKE
static const uint8_t m10q_data_47[] = {0x01, 0x00, 0xD0, 0x20, 0x01}; // PM-OPERATEMODE

// static const uint8_t m10q_data_48[] = {0x02, 0x00, 0x74, 0x10, 0x00}; // CFG-UART1OUTPROT-NMEA DISABLED

// static const uint8_t m10q_data_49[] = {0x02, 0x00, 0x74, 0x10, 0x01}; // CFG-UART1OUTPROT-NMEA ENABLED

// Tabla de payloads con tamaño incluido (ahorro de memoria vs arrays paralelos)
const M10QPayload m10q_data_payloads[M10Q_NUM_DATA_ELEMENTS] = {
    {m10q_data_00, sizeof(m10q_data_00)}, {m10q_data_01, sizeof(m10q_data_01)}, {m10q_data_02, sizeof(m10q_data_02)}, 
    {m10q_data_03, sizeof(m10q_data_03)}, {m10q_data_04, sizeof(m10q_data_04)}, {m10q_data_05, sizeof(m10q_data_05)}, 
    {m10q_data_06, sizeof(m10q_data_06)}, {m10q_data_07, sizeof(m10q_data_07)}, {m10q_data_08, sizeof(m10q_data_08)}, 
    {m10q_data_09, sizeof(m10q_data_09)}, {m10q_data_10, sizeof(m10q_data_10)}, {m10q_data_11, sizeof(m10q_data_11)}, 
    {m10q_data_12, sizeof(m10q_data_12)}, {m10q_data_13, sizeof(m10q_data_13)}, {m10q_data_14, sizeof(m10q_data_14)}, 
    {m10q_data_15, sizeof(m10q_data_15)}, {m10q_data_16, sizeof(m10q_data_16)}, {m10q_data_17, sizeof(m10q_data_17)}, 
    {m10q_data_18, sizeof(m10q_data_18)}, {m10q_data_19, sizeof(m10q_data_19)}, {m10q_data_20, sizeof(m10q_data_20)}, 
    {m10q_data_21, sizeof(m10q_data_21)}, {m10q_data_22, sizeof(m10q_data_22)}, {m10q_data_23, sizeof(m10q_data_23)}, 
    {m10q_data_24, sizeof(m10q_data_24)}, {m10q_data_25, sizeof(m10q_data_25)}, {m10q_data_26, sizeof(m10q_data_26)}, 
    {m10q_data_27, sizeof(m10q_data_27)}, {m10q_data_28, sizeof(m10q_data_28)}, {m10q_data_29, sizeof(m10q_data_29)}, 
    {m10q_data_30, sizeof(m10q_data_30)}, {m10q_data_31, sizeof(m10q_data_31)}, {m10q_data_32, sizeof(m10q_data_32)}, 
    {m10q_data_33, sizeof(m10q_data_33)}, {m10q_data_34, sizeof(m10q_data_34)}, {m10q_data_35, sizeof(m10q_data_35)}, 
    {m10q_data_36, sizeof(m10q_data_36)}, {m10q_data_37, sizeof(m10q_data_37)}, {m10q_data_38, sizeof(m10q_data_38)}, 
    {m10q_data_39, sizeof(m10q_data_39)}, {m10q_data_40, sizeof(m10q_data_40)}, {m10q_data_41, sizeof(m10q_data_41)}, 
    {m10q_data_42, sizeof(m10q_data_42)}, {m10q_data_43, sizeof(m10q_data_43)}, {m10q_data_44, sizeof(m10q_data_44)}, 
    {m10q_data_45, sizeof(m10q_data_45)}, {m10q_data_46, sizeof(m10q_data_46)}, {m10q_data_47, sizeof(m10q_data_47)}
};