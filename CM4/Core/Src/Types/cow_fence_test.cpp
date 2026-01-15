/**
 * @file cow_fence_test.cpp
 * @brief Comprehensive test suite for Cow and Fence embedded classes
 * @author TPP-IntelliFence Team
 * @date 2025-11-22
 */

#include "cow.h"
#include "fence.h"
#include <stdio.h>
#include <math.h>

// Test result tracking
static uint8_t tests_passed = 0;
static uint8_t tests_failed = 0;

// Helper macros
#define TEST_ASSERT(condition, test_name) \
    do { \
        if (condition) { \
            printf("  [PASS] %s\n", test_name); \
            tests_passed++; \
        } else { \
            printf("  [FAIL] %s FAILED\n", test_name); \
            tests_failed++; \
        } \
    } while(0)

#define TEST_SECTION(section_name) \
    printf("\n[TEST SECTION] %s\n", section_name); \
    printf("----------------------------------------\n")

// ============================================================================
// COW TESTS
// ============================================================================

void test_cow_creation() {
    TEST_SECTION("Cow Creation and Initialization");
    
    // Create a cow with unique device ID
    DeviceUID testId = {0x12345678, 0x9ABCDEF0, 0x11223344};
    Cow vaca(testId);
    
    // Verify ID
    DeviceUID retrievedId = vaca.getId();
    TEST_ASSERT(retrievedId.w0 == testId.w0 && 
                retrievedId.w1 == testId.w1 && 
                retrievedId.w2 == testId.w2, 
                "Device UID stored correctly");
    
    // Verify initial state
    TEST_ASSERT(vaca.getState() == CowState::SLEEP, "Initial state is SLEEP");
    TEST_ASSERT(vaca.getCurrentZone() == BLACK_ZONE, "Initial zone is BLACK_ZONE");
    TEST_ASSERT(vaca.getDistanceToLimit() == -1.0f, "Initial distance is -1");
    
    // Verify initial position (should be 0,0)
    Position pos = vaca.getPosition();
    TEST_ASSERT(pos.latitude == 0.0f && pos.longitude == 0.0f, "Initial position is (0,0)");
    
    printf("  Cow created with ID: 0x%08X-%08X-%08X\n", 
           (unsigned int)testId.w0, (unsigned int)testId.w1, (unsigned int)testId.w2);
}

void test_cow_position_update() {
    TEST_SECTION("Cow Position Updates");
    
    DeviceUID id = {0x00000001, 0x00000002, 0x00000003};
    Cow vaca(id);
    
    // Test position update (Buenos Aires coordinates)
    Position buenosAires = {-34.603722f, -58.381592f};
    vaca.updatePosition(buenosAires);
    
    Position retrieved = vaca.getPosition();
    TEST_ASSERT(fabs(retrieved.latitude - buenosAires.latitude) < 0.000001f, 
                "Latitude updated correctly");
    TEST_ASSERT(fabs(retrieved.longitude - buenosAires.longitude) < 0.000001f, 
                "Longitude updated correctly");
    
    printf("  Position updated to: (%.6f, %.6f)\n", 
           retrieved.latitude, retrieved.longitude);
    
    // Test another position
    Position newPos = {-34.600000f, -58.380000f};
    vaca.updatePosition(newPos);
    retrieved = vaca.getPosition();
    TEST_ASSERT(fabs(retrieved.latitude - newPos.latitude) < 0.000001, 
                "Position update 2 successful");
}

void test_cow_acceleration_update() {
    TEST_SECTION("Cow Acceleration Updates");
    
    DeviceUID id = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC};
    Cow vaca(id);
    
    // Test 1: Sleep state (very low acceleration)
    Acceleration sleep_acc = {0.02, 0.01, 0.03};
    vaca.updateAcceleration(sleep_acc);
    
    Acceleration retrieved = vaca.getAcceleration();
    TEST_ASSERT(fabs(retrieved.ax - sleep_acc.ax) < 0.0001, "Acceleration X updated");
    TEST_ASSERT(fabs(retrieved.ay - sleep_acc.ay) < 0.0001, "Acceleration Y updated");
    TEST_ASSERT(fabs(retrieved.az - sleep_acc.az) < 0.0001, "Acceleration Z updated");
    
    printf("  Sleep acceleration: (%.3f, %.3f, %.3f) g\n", 
           retrieved.ax, retrieved.ay, retrieved.az);
    
    // Test 2: Grazing state (Z-axis movement)
    Acceleration grazing_acc = {0.03, 0.02, 0.15};
    vaca.updateAcceleration(grazing_acc);
    retrieved = vaca.getAcceleration();
    TEST_ASSERT(fabs(retrieved.az - 0.15) < 0.0001, "Grazing acceleration detected");
    
    printf("  Grazing acceleration: (%.3f, %.3f, %.3f) g\n", 
           retrieved.ax, retrieved.ay, retrieved.az);
    
    // Test 3: Movement state (high acceleration)
    Acceleration movement_acc = {0.25, 0.18, 0.12};
    vaca.updateAcceleration(movement_acc);
    retrieved = vaca.getAcceleration();
    TEST_ASSERT(fabs(retrieved.ax - 0.25) < 0.0001, "Movement acceleration detected");
    
    printf("  Movement acceleration: (%.3f, %.3f, %.3f) g\n", 
           retrieved.ax, retrieved.ay, retrieved.az);
}

void test_cow_state_transitions() {
    TEST_SECTION("Cow State Transitions");
    
    DeviceUID id = {0x11111111, 0x22222222, 0x33333333};
    Cow vaca(id);
    
    // Test state transitions
    vaca.updateState(CowState::SLEEP);
    TEST_ASSERT(vaca.getState() == CowState::SLEEP, "State changed to SLEEP");
    
    vaca.updateState(CowState::GRAZING);
    TEST_ASSERT(vaca.getState() == CowState::GRAZING, "State changed to GRAZING");
    
    vaca.updateState(CowState::MOVEMENT);
    TEST_ASSERT(vaca.getState() == CowState::MOVEMENT, "State changed to MOVEMENT");
    
    printf("  State transitions: SLEEP -> GRAZING -> MOVEMENT [OK]\n");
}

void test_cow_zone_and_distance() {
    TEST_SECTION("Cow Zone and Distance Tracking");
    
    DeviceUID id = {0xDEADBEEF, 0xCAFEBABE, 0x12345678};
    Cow vaca(id);
    
    // Test zone updates
    vaca.updateCurrentZone(GREEN_ZONE);
    TEST_ASSERT(vaca.getCurrentZone() == GREEN_ZONE, "Zone set to GREEN_ZONE");
    
    vaca.updateCurrentZone(YELLOW_ZONE);
    TEST_ASSERT(vaca.getCurrentZone() == YELLOW_ZONE, "Zone set to YELLOW_ZONE");
    
    vaca.updateCurrentZone(RED_ZONE);
    TEST_ASSERT(vaca.getCurrentZone() == RED_ZONE, "Zone set to RED_ZONE");
    
    // Test distance updates
    vaca.updateDistanceToLimit(5.5);
    TEST_ASSERT(fabs(vaca.getDistanceToLimit() - 5.5) < 0.001, 
                "Distance to limit: 5.5m");
    
    vaca.updateDistanceToLimit(12.3);
    TEST_ASSERT(fabs(vaca.getDistanceToLimit() - 12.3) < 0.001, 
                "Distance to limit: 12.3m");
    
    printf("  Zone progression: GREEN -> YELLOW -> RED\n");
    printf("  Distance tracking: 5.5m -> 12.3m\n");
}

// ============================================================================
// FENCE TESTS
// ============================================================================

void test_fence_creation() {
    TEST_SECTION("Fence Creation and Initialization");
    
    Fence fence;
    
    // Verify initial state
    TEST_ASSERT(fence.getLimitCount() == 0, "Initial limit count is 0");
    
    Vertex center = fence.getCenterFence();
    TEST_ASSERT(center.latitude == 0.0f && center.longitude == 0.0f, 
                "Initial center is (0,0)");
    
    // Verify thresholds are set
    TEST_ASSERT(fence.getThreshold(LIGHT_BLUE_ZONE) > 0, "LIGHT_BLUE threshold set");
    TEST_ASSERT(fence.getThreshold(BLUE_ZONE) > 0, "BLUE threshold set");
    TEST_ASSERT(fence.getThreshold(DARK_BLUE_ZONE) > 0, "DARK_BLUE threshold set");
    TEST_ASSERT(fence.getThreshold(YELLOW_ZONE) > 0, "YELLOW threshold set");
    TEST_ASSERT(fence.getThreshold(RED_ZONE) > 0, "RED threshold set");
    
    printf("  Thresholds: LIGHT_BLUE=%.1fm, BLUE=%.1fm, DARK_BLUE=%.1fm, YELLOW=%.1fm, RED=%.1fm\n",
           fence.getThreshold(LIGHT_BLUE_ZONE),
           fence.getThreshold(BLUE_ZONE),
           fence.getThreshold(DARK_BLUE_ZONE),
           fence.getThreshold(YELLOW_ZONE),
           fence.getThreshold(RED_ZONE));
}

void test_fence_rectangular() {
    TEST_SECTION("Fence Rectangular Geometry");
    
    Fence fence;
    
    // Create a rectangular fence (approx 100m x 100m in Buenos Aires)
    Vertex rectangle[4] = {
        {-34.603722f, -58.381592f},  // NW corner
        {-34.603722f, -58.380592f},  // NE corner
        {-34.602722f, -58.380592f},  // SE corner
        {-34.602722f, -58.381592f}   // SW corner
    };
    
    // Create limits directly from buffer
    fence.createLimits(rectangle, 4);
    TEST_ASSERT(fence.getLimitCount() == 4, "Created 4 limits (sides)");
    
    // Verify center calculation
    Vertex center = fence.getCenterFence();
    float expectedLat = (-34.603722f - 34.602722f) / 2.0f;
    float expectedLon = (-58.381592f - 58.380592f) / 2.0f;
    
    TEST_ASSERT(fabs(center.latitude - expectedLat) < 0.000001, 
                "Center latitude calculated correctly");
    TEST_ASSERT(fabs(center.longitude - expectedLon) < 0.000001, 
                "Center longitude calculated correctly");
    
    printf("  Rectangle created: 4 vertices, 4 limits\n");
    printf("  Center: (%.6f, %.6f)\n", center.latitude, center.longitude);
}

void test_fence_polygon() {
    TEST_SECTION("Fence Polygon Geometry (Hexagon)");
    
    Fence fence;
    
    // Create a hexagonal fence
    Vertex hexagon[6] = {
        {-34.604000f, -58.381000f},
        {-34.603500f, -58.380500f},
        {-34.603000f, -58.380500f},
        {-34.602500f, -58.381000f},
        {-34.603000f, -58.381500f},
        {-34.603500f, -58.381500f}
    };
    
    fence.createLimits(hexagon, 6);
    TEST_ASSERT(fence.getLimitCount() == 6, "Created 6 limits");
    
    // Verify polygon closure (last limit connects to first vertex)
    const Line* limits = fence.getLimits();
    TEST_ASSERT(fabs(limits[5].end.latitude - hexagon[0].latitude) < 0.000001 &&
                fabs(limits[5].end.longitude - hexagon[0].longitude) < 0.000001,
                "Polygon closes correctly (last -> first)");
    
    printf("  Hexagon created: 6 vertices, 6 limits\n");
    printf("  Polygon closure verified [OK]\n");
}

void test_fence_overflow_protection() {
    TEST_SECTION("Fence Overflow Protection");
    
    Fence fence;
    
    // Test: Add maximum allowed vertices
    Vertex maxVertices[MAX_VERTICES];
    for (uint8_t i = 0; i < MAX_VERTICES; i++) {
        maxVertices[i] = {-34.0f + i * 0.0001f, -58.0f + i * 0.0001f};
    }
    
    fence.createLimits(maxVertices, MAX_VERTICES);
    TEST_ASSERT(fence.getLimitCount() == MAX_VERTICES, 
                "Created MAX_VERTICES limits");
    
    printf("  Overflow protection: [OK] Accepted MAX_VERTICES\n");
}

// ============================================================================
// INTEGRATION TESTS (COW + FENCE)
// ============================================================================

void test_cow_fence_integration() {
    TEST_SECTION("Cow-Fence Integration");
    
    // Create cow and fence
    DeviceUID cowId = {0x00001111, 0x00002222, 0x00003333};
    Cow vaca(cowId);
    Fence cercado;
    
    // Setup fence (rectangular paddock)
    Vertex paddock[4] = {
        {-34.604000f, -58.381500f},
        {-34.604000f, -58.380500f},
        {-34.603000f, -58.380500f},
        {-34.603000f, -58.381500f}
    };
    
    cercado.createLimits(paddock, 4);
    
    // Scenario 1: Cow inside fence (GREEN_ZONE)
    Position inside = {-34.603500f, -58.381000f};  // Center of paddock
    vaca.updatePosition(inside);
    vaca.updateCurrentZone(GREEN_ZONE);
    vaca.updateDistanceToLimit(25.0f);
    
    TEST_ASSERT(vaca.getCurrentZone() == GREEN_ZONE, 
                "Cow in GREEN_ZONE (inside fence)");
    TEST_ASSERT(vaca.getDistanceToLimit() > 20.0, 
                "Distance > 20m (safe zone)");
    
    printf("  Scenario 1: Cow safe inside fence (GREEN, 25m from edge)\n");
    
    // Scenario 2: Cow approaching limit (YELLOW_ZONE)
    Position approaching = {-34.603200f, -58.381000f};
    vaca.updatePosition(approaching);
    vaca.updateCurrentZone(YELLOW_ZONE);
    vaca.updateDistanceToLimit(4.5f);
    
    TEST_ASSERT(vaca.getCurrentZone() == YELLOW_ZONE, 
                "Cow in YELLOW_ZONE (approaching limit)");
    TEST_ASSERT(vaca.getDistanceToLimit() < 5.0f, 
                "Distance < 5m (warning zone)");
    
    printf("  Scenario 2: Cow approaching fence (YELLOW, 4.5m from edge)\n");
    
    // Scenario 3: Cow at limit (RED_ZONE)
    Position atLimit = {-34.603050f, -58.381000f};
    vaca.updatePosition(atLimit);
    vaca.updateCurrentZone(RED_ZONE);
    vaca.updateDistanceToLimit(0.8f);
    
    TEST_ASSERT(vaca.getCurrentZone() == RED_ZONE, 
                "Cow in RED_ZONE (at limit)");
    TEST_ASSERT(vaca.getDistanceToLimit() < 1.0f, 
                "Distance < 1m (danger zone)");
    
    printf("  Scenario 3: Cow at fence limit (RED, 0.8m from edge)\n");
    
    // Scenario 4: Cow escaped (BLACK_ZONE)
    Position escaped = {-34.602500f, -58.381000f};
    vaca.updatePosition(escaped);
    vaca.updateCurrentZone(BLACK_ZONE);
    vaca.updateDistanceToLimit(-5.0);  // Negative = outside
    
    TEST_ASSERT(vaca.getCurrentZone() == BLACK_ZONE, 
                "Cow in BLACK_ZONE (escaped)");
    TEST_ASSERT(vaca.getDistanceToLimit() < 0, 
                "Negative distance (outside fence)");
    
    printf("  Scenario 4: Cow escaped fence (BLACK, -5m outside)\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

extern "C" void run_cow_fence_tests() {
    printf("\n");
    printf("================================================================\n");
    printf("       COW & FENCE EMBEDDED CLASSES - TEST SUITE               \n");
    printf("================================================================\n");
    
    tests_passed = 0;
    tests_failed = 0;
    
    // ===== COW TESTS =====
    printf("\n================================================================\n");
    printf("                    COW CLASS TESTS\n");
    printf("================================================================\n");
    
    test_cow_creation();
    test_cow_position_update();
    test_cow_acceleration_update();
    test_cow_state_transitions();
    test_cow_zone_and_distance();
    
    // ===== FENCE TESTS =====
    printf("\n================================================================\n");
    printf("                   FENCE CLASS TESTS\n");
    printf("================================================================\n");
    
    test_fence_creation();
    test_fence_rectangular();
    test_fence_polygon();
    test_fence_overflow_protection();
    
    // ===== INTEGRATION TESTS =====
    printf("\n================================================================\n");
    printf("                COW & FENCE INTEGRATION\n");
    printf("================================================================\n");
    
    test_cow_fence_integration();
    
    // ===== FINAL REPORT =====
    printf("\n");
    printf("================================================================\n");
    printf("                     TEST SUMMARY                            \n");
    printf("================================================================\n");
    printf("  Tests Passed:  %3d                                        \n", tests_passed);
    printf("  Tests Failed:  %3d                                        \n", tests_failed);
    printf("  Total Tests:   %3d                                        \n", tests_passed + tests_failed);
    printf("================================================================\n");
    
    if (tests_failed == 0) {
        printf("  Result: [ALL TESTS PASSED]                               \n");
        printf("================================================================\n");
    } else {
        printf("  Result: [SOME TESTS FAILED]                              \n");
        printf("================================================================\n");
    }
    
    printf("\n");
}
