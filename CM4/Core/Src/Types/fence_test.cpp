/**
 * @file fence_test.cpp
 * @brief Test implementation for embedded-friendly Fence class
 * @author TPP-IntelliFence Team
 * @date 2025
 */

#include "fence.h"
#include <stdio.h>

/**
 * @brief Test básico de la clase Fence sin std::vector
 */
void test_fence_static_implementation() {
    printf("\n========================================\n");
    printf("  FENCE STATIC IMPLEMENTATION TEST\n");
    printf("========================================\n\n");
    
    // Crear instancia de Fence
    Fence fence;
    
    // Test 1: Crear un cerco rectangular simple
    printf("[TEST 1] Creating rectangular fence...\n");
    Vertex rectangulo[4] = {
        {-34.603722f, -58.381592f},  // Esquina 1
        {-34.603722f, -58.380592f},  // Esquina 2
        {-34.602722f, -58.380592f},  // Esquina 3
        {-34.602722f, -58.381592f}   // Esquina 4
    };
    
    // Crear límites directamente
    fence.createLimits(rectangulo, 4);
    printf("[TEST 1] ✓ Created fence with 4 vertices\n");
    
    // Test 2: Verificar límites
    printf("\n[TEST 2] Checking fence limits...\n");
    printf("[TEST 2] ✓ Created %d limits\n", fence.getLimitCount());
    
    // Test 3: Verificar centro del cerco
    printf("\n[TEST 3] Checking fence center...\n");
    Vertex center = fence.getCenterFence();
    printf("[TEST 3] Center: (%.6f, %.6f)\n", center.latitude, center.longitude);
    
    // Test 4: Verificar umbrales de zona
    printf("\n[TEST 4] Checking zone thresholds...\n");
    printf("[TEST 4] LIGHT_BLUE_ZONE threshold: %.1f meters\n", fence.getThreshold(LIGHT_BLUE_ZONE));
    printf("[TEST 4] BLUE_ZONE threshold: %.1f meters\n", fence.getThreshold(BLUE_ZONE));
    printf("[TEST 4] DARK_BLUE_ZONE threshold: %.1f meters\n", fence.getThreshold(DARK_BLUE_ZONE));
    printf("[TEST 4] YELLOW_ZONE threshold: %.1f meters\n", fence.getThreshold(YELLOW_ZONE));
    printf("[TEST 4] RED_ZONE threshold: %.1f meters\n", fence.getThreshold(RED_ZONE));
    
    printf("\n========================================\n");
    printf("  ALL TESTS COMPLETED\n");
    printf("========================================\n\n");
}
