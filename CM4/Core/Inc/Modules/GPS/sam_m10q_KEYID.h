#ifndef MODULES_GPS_SAM_M10Q_KEYID_H_
#define MODULES_GPS_SAM_M10Q_KEYID_H_

#include <cstdint>
#include <cstddef>

// ========================
// Constantes y contadores
// ========================
constexpr size_t M10Q_NUM_RATE_OPTIONS   = 7; 
constexpr size_t M10Q_NUM_INIT_PSM_DATA_ELEMENTS  = 40; //Cantidad de elementos en m10q_data para configuración inicial (Sin PSMOO)
constexpr size_t M10Q_NUM_DATA_ELEMENTS  = 48;  // Cantidad de elementos en m10q_data

// ======================================================
// Estructura para empaqueta puntero + tamaño (ahorro de memoria vs arrays paralelos)
// ======================================================
struct M10QPayload {
    const uint8_t* data;
    uint8_t size;  // uint8_t es suficiente (max payload ~100 bytes)
};

// ======================================================
// EXTERN DECLARATIONS - All definitions in sam_m10q_KEYID.cpp
// This prevents multiple definition errors and saves ROM/RAM
// ======================================================

// RATES: m10q_new_acq_time (payloads)
extern const M10QPayload m10q_new_acq_time_green_zone[M10Q_NUM_RATE_OPTIONS];
extern const M10QPayload m10q_new_acq_time_near_limit[M10Q_NUM_RATE_OPTIONS];
extern const M10QPayload m10q_new_acq_time_continuous[1];

// DATA: m10q_data_payloads (configuration payloads con tamaño incluido)
extern const M10QPayload m10q_data_payloads[M10Q_NUM_DATA_ELEMENTS];

#endif /* MODULES_GPS_SAM_M10Q_KEYID_H_ */
