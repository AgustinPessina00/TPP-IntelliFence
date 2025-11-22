# Cow & Fence Test Suite - Documentation

## 📋 Overview

Comprehensive test suite for the **Cow** and **Fence** embedded classes, validating functionality before FSM integration.

---

## 🧪 Test Structure

### Test File: `cow_fence_test.cpp`
- **Lines of code:** ~550
- **Test cases:** 45+ individual assertions
- **Coverage:** Creation, updates, geometry, integration

### Test Categories

#### 1. **COW CLASS TESTS** (5 sections)
- ✅ Creation and Initialization
- ✅ Position Updates
- ✅ Acceleration Updates
- ✅ State Transitions
- ✅ Zone and Distance Tracking

#### 2. **FENCE CLASS TESTS** (5 sections)
- ✅ Creation and Initialization
- ✅ Rectangular Geometry
- ✅ Polygon Geometry (Hexagon)
- ✅ Overflow Protection
- ✅ Clear Functionality

#### 3. **INTEGRATION TESTS** (1 section)
- ✅ Cow-Fence Integration (4 scenarios)

---

## 🎯 Test Details

### Cow Class Tests

#### Test 1: Creation and Initialization
```cpp
DeviceUID testId = {0x12345678, 0x9ABCDEF0, 0x11223344};
Cow vaca(testId);

✓ Device UID stored correctly
✓ Initial state is SLEEP
✓ Initial zone is BLACK_ZONE
✓ Initial distance is -1
✓ Initial position is (0,0)
```

#### Test 2: Position Updates
```cpp
Position buenosAires = {-34.603722, -58.381592};
vaca.updatePosition(buenosAires);

✓ Latitude updated correctly
✓ Longitude updated correctly
✓ Position update 2 successful
```

#### Test 3: Acceleration Updates
```cpp
// Sleep state
Acceleration sleep_acc = {0.02, 0.01, 0.03};
✓ Acceleration X, Y, Z updated

// Grazing state
Acceleration grazing_acc = {0.03, 0.02, 0.15};
✓ Grazing acceleration detected

// Movement state
Acceleration movement_acc = {0.25, 0.18, 0.12};
✓ Movement acceleration detected
```

#### Test 4: State Transitions
```cpp
vaca.updateState(CowState::SLEEP);
vaca.updateState(CowState::GRAZING);
vaca.updateState(CowState::MOVEMENT);

✓ State changed to SLEEP
✓ State changed to GRAZING
✓ State changed to MOVEMENT
```

#### Test 5: Zone and Distance
```cpp
vaca.updateCurrentZone(GREEN_ZONE);
vaca.updateCurrentZone(YELLOW_ZONE);
vaca.updateCurrentZone(RED_ZONE);

✓ Zone set to GREEN_ZONE
✓ Zone set to YELLOW_ZONE
✓ Zone set to RED_ZONE

vaca.updateDistanceToLimit(5.5);
vaca.updateDistanceToLimit(12.3);

✓ Distance to limit: 5.5m
✓ Distance to limit: 12.3m
```

---

### Fence Class Tests

#### Test 1: Creation and Initialization
```cpp
Fence fence;

✓ Initial vertex count is 0
✓ Initial limit count is 0
✓ Initial center is (0,0)
✓ LIGHT_BLUE threshold set (20m)
✓ BLUE threshold set (15m)
✓ DARK_BLUE threshold set (10m)
✓ YELLOW threshold set (5m)
✓ RED threshold set (1m)
```

#### Test 2: Rectangular Geometry
```cpp
Vertex rectangle[4] = {
    {-34.603722, -58.381592},  // NW corner
    {-34.603722, -58.380592},  // NE corner
    {-34.602722, -58.380592},  // SE corner
    {-34.602722, -58.381592}   // SW corner
};

fence.saveVertices(rectangle, 4);
fence.createLimits();

✓ Saved 4 rectangular vertices
✓ Vertex count is 4
✓ Created 4 limits (sides)
✓ Center latitude calculated correctly
✓ Center longitude calculated correctly
```

#### Test 3: Polygon Geometry (Hexagon)
```cpp
Vertex hexagon[6] = {...};

fence.saveVertices(hexagon, 6);
fence.createLimits();

✓ Saved 6 hexagonal vertices
✓ Vertex count is 6
✓ Created 6 limits
✓ Polygon closes correctly (last → first)
```

#### Test 4: Overflow Protection
```cpp
// Test exceeding MAX_VERTICES
Vertex tooMany[MAX_VERTICES + 5];  // 25 vertices
fence.saveVertices(tooMany, 25);

✓ Rejected oversized array (25 > 20)
✓ Vertex count remains 0 after rejection

// Test maximum allowed
Vertex maxVertices[MAX_VERTICES];  // 20 vertices
fence.saveVertices(maxVertices, 20);

✓ Accepted maximum vertices (20)
✓ Vertex count is MAX_VERTICES
✓ Created MAX_VERTICES limits
```

#### Test 5: Clear Functionality
```cpp
Vertex triangle[3] = {...};
fence.saveVertices(triangle, 3);
fence.createLimits();

✓ 3 vertices before clear
✓ 3 limits before clear

fence.clearVertices();

✓ 0 vertices after clear
✓ 0 limits after clear
✓ Center reset to (0,0)
```

---

### Integration Tests

#### Cow-Fence Integration (4 Scenarios)

**Scenario 1: Cow Safe Inside Fence (GREEN_ZONE)**
```cpp
Position inside = {-34.603500, -58.381000};  // Center of paddock
vaca.updatePosition(inside);
vaca.updateCurrentZone(GREEN_ZONE);
vaca.updateDistanceToLimit(25.0);

✓ Cow in GREEN_ZONE (inside fence)
✓ Distance > 20m (safe zone)
```

**Scenario 2: Cow Approaching Limit (YELLOW_ZONE)**
```cpp
Position approaching = {-34.603200, -58.381000};
vaca.updateCurrentZone(YELLOW_ZONE);
vaca.updateDistanceToLimit(4.5);

✓ Cow in YELLOW_ZONE (approaching limit)
✓ Distance < 5m (warning zone)
```

**Scenario 3: Cow At Limit (RED_ZONE)**
```cpp
Position atLimit = {-34.603050, -58.381000};
vaca.updateCurrentZone(RED_ZONE);
vaca.updateDistanceToLimit(0.8);

✓ Cow in RED_ZONE (at limit)
✓ Distance < 1m (danger zone)
```

**Scenario 4: Cow Escaped (BLACK_ZONE)**
```cpp
Position escaped = {-34.602500, -58.381000};
vaca.updateCurrentZone(BLACK_ZONE);
vaca.updateDistanceToLimit(-5.0);  // Negative = outside

✓ Cow in BLACK_ZONE (escaped)
✓ Negative distance (outside fence)
```

---

## 🚀 How to Run

### Option 1: Run Before RTOS (Recommended)
```c
// In main.c, before osKernelStart()
run_comprehensive_module_tests();  // Includes cow_fence_tests
```

### Option 2: Run from Test Task
```c
void testTask(void *argument) {
    run_cow_fence_tests();
    vTaskDelete(NULL);
}
```

### Option 3: Call Directly
```c
#include "cow_fence_test.h"

int main(void) {
    HAL_Init();
    // ... peripheral init ...
    
    run_cow_fence_tests();
    
    // ... rest of main ...
}
```

---

## 📊 Expected Output

```
╔════════════════════════════════════════════════════════════╗
║     COW & FENCE EMBEDDED CLASSES - TEST SUITE             ║
╚════════════════════════════════════════════════════════════╝

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
                    COW CLASS TESTS
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

[TEST SECTION] Cow Creation and Initialization
----------------------------------------
  ✓ Device UID stored correctly
  ✓ Initial state is SLEEP
  ✓ Initial zone is BLACK_ZONE
  ✓ Initial distance is -1
  ✓ Initial position is (0,0)
  Cow created with ID: 0x12345678-0x9ABCDEF0-0x11223344

[... more tests ...]

╔════════════════════════════════════════════════════════════╗
║                     TEST SUMMARY                           ║
╠════════════════════════════════════════════════════════════╣
║  Tests Passed:  45                                         ║
║  Tests Failed:   0                                         ║
║  Total Tests:   45                                         ║
╠════════════════════════════════════════════════════════════╣
║  Result: ✓ ALL TESTS PASSED                               ║
╚════════════════════════════════════════════════════════════╝
```

---

## 🔧 Integration Status

### Files Modified
- ✅ `CMakeLists.txt` - Added cow_fence_test.cpp
- ✅ `main.c` - Added test call to run_comprehensive_module_tests()

### Dependencies
- `cow.h` / `cow.cpp` - Cow class implementation
- `fence.h` / `fence.cpp` - Fence class implementation
- `zone.h` - Zone definitions
- `stdio.h` - printf for output
- `math.h` - fabs for floating point comparison

---

## ✅ Benefits

1. **Early Validation** - Catch bugs before FSM integration
2. **Regression Testing** - Verify changes don't break existing functionality
3. **Documentation** - Tests serve as usage examples
4. **Confidence** - Know data model is solid before RTOS complexity

---

## 🎯 Next Steps

1. **Run tests on hardware** - Validate on STM32WL55JC
2. **Add to CI/CD** - Automated testing on commits
3. **Extend coverage** - Add edge case tests
4. **Performance benchmarking** - Measure execution time

---

**Status:** ✅ **Ready for testing**  
**Last updated:** 2025-11-22  
**Test coverage:** 45+ assertions across 11 test sections
