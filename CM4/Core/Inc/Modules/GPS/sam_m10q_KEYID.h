#ifndef MODULES_GPS_SAM_M10Q_KEYID_H_
#define MODULES_GPS_SAM_M10Q_KEYID_H_

#include <cstdint>
#include <cstddef>

// ========================
// Constantes y contadores
// ========================
constexpr size_t M10Q_NUM_RATE_OPTIONS   = 4;   // STOP, SLOW, MEDIUM, FAST
constexpr size_t M10Q_NUM_DATA_ELEMENTS  = 50;  // Cantidad de elementos en m10q_data

// ======================================================
// EXTERN DECLARATIONS - All definitions in sam_m10q_KEYID.cpp
// This prevents multiple definition errors and saves ROM/RAM
// ======================================================

// RATES: m10q_new_acq_time (payloads)
extern const uint8_t* const m10q_new_acq_time[M10Q_NUM_RATE_OPTIONS];

// RATES: m10q_new_acq_ck (checksums)
extern const uint8_t* const m10q_new_acq_ck[M10Q_NUM_RATE_OPTIONS];

// DATA: m10q_data_payloads (configuration payloads)
extern const uint8_t* const m10q_data_payloads[M10Q_NUM_DATA_ELEMENTS];

#endif /* MODULES_GPS_SAM_M10Q_KEYID_H_ */
