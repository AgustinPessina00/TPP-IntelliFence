/**
 * @file cow_fence_test.h
 * @brief Header for Cow and Fence test suite
 * @author TPP-IntelliFence Team
 * @date 2025-11-22
 */

#ifndef COW_FENCE_TEST_H
#define COW_FENCE_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run comprehensive test suite for Cow and Fence classes
 * @details Tests include:
 *   - Cow creation, state management, position/acceleration updates
 *   - Fence geometry (rectangle, polygon), overflow protection
 *   - Cow-Fence integration scenarios (zones, distances)
 * @note Call from main() before RTOS startup or from a test task
 */
void run_cow_fence_tests(void);

#ifdef __cplusplus
}
#endif

#endif // COW_FENCE_TEST_H
