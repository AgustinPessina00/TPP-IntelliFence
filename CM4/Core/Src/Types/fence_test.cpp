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
        {-34.603722, -58.381592},  // Esquina 1
        {-34.603722, -58.380592},  // Esquina 2
        {-34.602722, -58.380592},  // Esquina 3
        {-34.602722, -58.381592}   // Esquina 4
    };
    
    if (fence.saveVertices(rectangulo, 4)) {
        printf("[TEST 1] ✓ Saved 4 vertices\n");
    } else {
        printf("[TEST 1] ✗ Failed to save vertices\n");
        return;
    }
    
    // Test 2: Crear límites
    printf("\n[TEST 2] Creating fence limits...\n");
    fence.createLimits();
    
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
    
    // Test 5: Verificar límites del array (overflow protection)
    printf("\n[TEST 5] Testing array overflow protection...\n");
    Vertex tooMany[MAX_VERTICES + 5];
    for (uint8_t i = 0; i < MAX_VERTICES + 5; i++) {
        tooMany[i] = {-34.0 + i*0.001, -58.0 + i*0.001};
    }
    
    if (!fence.saveVertices(tooMany, MAX_VERTICES + 5)) {
        printf("[TEST 5] ✓ Correctly rejected %d vertices (max is %d)\n", 
               MAX_VERTICES + 5, MAX_VERTICES);
    } else {
        printf("[TEST 5] ✗ Should have rejected oversized array\n");
    }
    
    // Test 6: Limpiar y verificar
    printf("\n[TEST 6] Testing clearVertices()...\n");
    fence.clearVertices();
    printf("[TEST 6] Vertex count after clear: %d (should be 0)\n", fence.getVertexCount());
    printf("[TEST 6] Limit count after clear: %d (should be 0)\n", fence.getLimitCount());
    
    printf("\n========================================\n");
    printf("  ALL TESTS COMPLETED\n");
    printf("========================================\n\n");
}
