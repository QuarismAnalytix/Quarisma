# SMP Module Testing - Quick Reference Guide

## Quick Start

### Run All SMP Tests
```bash
cd build_ninja
./bin/CoreCxxTests --gtest_filter="SMP*:Smp*"
```

**Expected Output:**
```
[==========] Running 71 tests from 6 test suites.
[  PASSED  ] 71 tests.
```

---

## Test Categories

### 1. Core API Tests (18 tests)
```bash
./bin/CoreCxxTests --gtest_filter="SMPComprehensive.*"
```

**What it tests:**
- Backend initialization
- Parallel for loops
- Thread management
- Nested parallelism

### 2. Transform/Fill/Sort Tests (21 tests)
```bash
./bin/CoreCxxTests --gtest_filter="SMPTransformFillSort.*"
```

**What it tests:**
- Transform operations
- Fill operations
- Sort operations
- Edge cases

### 3. Advanced Thread Pool Tests (10 tests)
```bash
./bin/CoreCxxTests --gtest_filter="SmpAdvancedParallelThreadPoolNative.*"
```

**What it tests:**
- Task launching
- Thread pool management
- Concurrent execution

### 4. Thread Naming Tests (9 tests)
```bash
./bin/CoreCxxTests --gtest_filter="SmpAdvancedThreadName.*"
```

**What it tests:**
- Thread name setting/getting
- Special characters
- Multi-threaded naming

### 5. Thread Pool Core Tests (12 tests)
```bash
./bin/CoreCxxTests --gtest_filter="SmpAdvancedThreadPool.*"
```

**What it tests:**
- Pool creation/destruction
- Task execution
- Thread pool state

---

## Running Benchmarks

### Build with Benchmarks
```bash
cd Scripts
python3 setup.py config.build.test.ninja.clang.python -DXSIGMA_ENABLE_BENCHMARK=ON
```

### Run All Benchmarks
```bash
cd ../build_ninja
./bin/CoreCxxBenchmark --benchmark_filter="BM_.*"
```

### Run Specific Benchmarks
```bash
# Parallel For benchmarks
./bin/CoreCxxBenchmark --benchmark_filter="BM_ParallelFor_.*"

# Transform benchmarks
./bin/CoreCxxBenchmark --benchmark_filter="BM_Transform_.*"

# Fill benchmarks
./bin/CoreCxxBenchmark --benchmark_filter="BM_Fill_.*"

# Sort benchmarks
./bin/CoreCxxBenchmark --benchmark_filter="BM_Sort_.*"

# Grain size comparison
./bin/CoreCxxBenchmark --benchmark_filter="BM_ParallelFor_GrainSize"

# Memory vs Compute bound
./bin/CoreCxxBenchmark --benchmark_filter="BM_.*Bound"
```

---

## Test File Locations

| File | Location | Tests |
|------|----------|-------|
| TestSMPComprehensive.cpp | Library/Core/Testing/Cxx/ | 18 |
| TestSMPTransformFillSort.cpp | Library/Core/Testing/Cxx/ | 21 |
| TestSmpAdvancedParallelThreadPoolNative.cpp | Library/Core/Testing/Cxx/ | 10 |
| TestSmpAdvancedThreadName.cpp | Library/Core/Testing/Cxx/ | 9 |
| TestSmpAdvancedThreadPool.cpp | Library/Core/Testing/Cxx/ | 12 |
| BenchmarkSMP.cpp | Library/Core/Testing/Benchmark/ | 13 |

---

## Common Test Patterns

### Testing Parallel For
```cpp
XSIGMATEST(MyTest, parallel_for_test)
{
    std::vector<int> data(100, 0);

    tools::For(0, 100, 10, [&data](int begin, int end) {
        for (int i = begin; i < end; ++i) {
            data[i] = i * 2;
        }
    });

    // Verify results
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(data[i], i * 2);
    }
}
```

### Testing Transform
```cpp
XSIGMATEST(MyTest, transform_test)
{
    std::vector<int> input(100);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(100, 0);

    tools::Transform(input.begin(), input.end(), output.begin(),
                     [](int x) { return x * 2; });

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(output[i], i * 2);
    }
}
```

### Testing Thread Safety
```cpp
XSIGMATEST(MyTest, thread_safety_test)
{
    std::atomic<int> counter{0};

    tools::For(0, 1000, 10, [&counter](int begin, int end) {
        for (int i = begin; i < end; ++i) {
            counter.fetch_add(1, std::memory_order_relaxed);
        }
    });

    EXPECT_EQ(counter.load(), 1000);
}
```

---

## Benchmark Patterns

### Basic Benchmark
```cpp
static void BM_MyOperation(benchmark::State& state)
{
    std::vector<int> data(1000, 0);

    for (auto _ : state) {
        tools::For(0, 1000, 100, [&data](int begin, int end) {
            for (int i = begin; i < end; ++i) {
                data[i] = i;
            }
        });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_MyOperation);
```

### Parameterized Benchmark
```cpp
static void BM_GrainSize(benchmark::State& state)
{
    const int size = 100000;
    const int grain_size = state.range(0);
    std::vector<int> data(size, 0);

    for (auto _ : state) {
        tools::For(0, size, grain_size, [&data](int begin, int end) {
            for (int i = begin; i < end; ++i) {
                data[i] = i;
            }
        });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_GrainSize)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);
```

---

## Troubleshooting

### Tests Fail to Build
```bash
# Clean and rebuild
cd Scripts
python3 setup.py config.build.test.ninja.clang.python
```

### Tests Fail to Run
```bash
# Check test list
cd build_ninja
./bin/CoreCxxTests --gtest_list_tests

# Run with verbose output
./bin/CoreCxxTests --gtest_filter="SMP*" --gtest_color=yes
```

### Benchmarks Not Available
```bash
# Rebuild with benchmarks enabled
cd Scripts
python3 setup.py config.build.test.ninja.clang.python -DXSIGMA_ENABLE_BENCHMARK=ON
```

---

## Coverage Analysis

### Run Tests with Coverage
```bash
# Build with coverage enabled
cd Scripts
python3 setup.py config.build.test.ninja.clang.python -DXSIGMA_ENABLE_COVERAGE=ON

# Run tests
cd ../build_ninja
./bin/CoreCxxTests --gtest_filter="SMP*:Smp*"

# Generate coverage report
llvm-cov show ./bin/CoreCxxTests -instr-profile=default.profdata
```

---

## Performance Tips

### Optimal Grain Sizes

| Workload Size | Recommended Grain Size |
|---------------|------------------------|
| < 100 | Serial execution (no parallelism) |
| 100 - 1,000 | 10 - 50 |
| 1,000 - 10,000 | 50 - 500 |
| 10,000 - 100,000 | 500 - 5,000 |
| > 100,000 | 5,000 - 50,000 |

### Thread Count Guidelines

| CPU Cores | Recommended Threads |
|-----------|---------------------|
| 2 | 2 |
| 4 | 4 |
| 8 | 6-8 |
| 16+ | 12-16 |

**Note:** Leave 1-2 cores for system tasks

---

## Adding New Tests

### 1. Create Test File
```bash
# File: Library/Core/Testing/Cxx/TestMyFeature.cpp
```

### 2. Write Tests
```cpp
#include "Testing/xsigmaTest.h"
#include "smp/tools.h"

namespace xsigma
{

XSIGMATEST(MyFeature, test_name)
{
    // Test implementation
    EXPECT_TRUE(true);
}

}  // namespace xsigma
```

### 3. Build and Run
```bash
cd Scripts
python3 setup.py config.build.test.ninja.clang.python

cd ../build_ninja
./bin/CoreCxxTests --gtest_filter="MyFeature.*"
```

**No CMakeLists.txt modification needed!** Tests are auto-discovered.

---

## CI/CD Integration

### GitHub Actions Example
```yaml
- name: Build and Test SMP Module
  run: |
    cd Scripts
    python3 setup.py config.build.test.ninja.clang.python
    cd ../build_ninja
    ./bin/CoreCxxTests --gtest_filter="SMP*:Smp*" --gtest_output=xml:smp_test_results.xml
```

### Jenkins Example
```groovy
stage('Test SMP Module') {
    steps {
        sh '''
            cd Scripts
            python3 setup.py config.build.test.ninja.clang.python
            cd ../build_ninja
            ./bin/CoreCxxTests --gtest_filter="SMP*:Smp*"
        '''
    }
}
```

---

## Resources

- **Full Documentation:** `Docs/SMP_TESTING_SUMMARY.md`
- **Final Report:** `Docs/SMP_TESTING_FINAL_REPORT.md`
- **Source Code:** `Library/Core/smp/`
- **Tests:** `Library/Core/Testing/Cxx/TestSMP*.cpp`
- **Benchmarks:** `Library/Core/Testing/Benchmark/BenchmarkSMP.cpp`

---

**Last Updated:** 2025-11-01
**Version:** 1.0.0
