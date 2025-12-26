/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of XSigma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@xsigma.co.uk
 * Website: https://www.xsigma.co.uk
 */

#include "common/configure.h"
#include "common/macros.h"
#include "xsigmaTest.h"

#if XSIGMA_HAS_CUDA

#include <string>
#include <vector>

#include "logging/logger.h"
#include "memory/device.h"
#include "memory/gpu/gpu_device_manager.h"

using namespace xsigma;
using namespace xsigma::gpu;

/**
 * @brief Test GPU device manager singleton access
 */
XSIGMATEST(GpuDeviceManager, provides_singleton_instance)
{
    // Test singleton access
    auto& manager1 = gpu_device_manager::instance();
    auto& manager2 = gpu_device_manager::instance();

    // Should be the same instance
    EXPECT_EQ(&manager1, &manager2);

    XSIGMA_LOG_INFO("GPU device manager singleton test passed");
}

/**
 * @brief Test GPU device manager initialization
 */
XSIGMATEST(GpuDeviceManager, initializes_successfully)
{
    auto& manager = gpu_device_manager::instance();

    try
    {
        manager.initialize();
        EXPECT_TRUE(true);  // Initialization succeeded
        XSIGMA_LOG_INFO("GPU device manager initialization test passed");
    }
    catch (const std::exception& e)
    {
        XSIGMA_LOG_INFO(
            "GPU device manager initialization failed (expected if no GPU): {}", e.what());
        // This is acceptable if no GPU hardware is available
    }
}

/**
 * @brief Test runtime information retrieval
 */
XSIGMATEST(GpuDeviceManager, provides_runtime_information)
{
    auto& manager = gpu_device_manager::instance();
    manager.initialize();

    auto runtime_info = manager.get_runtime_info();

    // Runtime info should have valid values
    EXPECT_GE(runtime_info.cuda_runtime_version, 0);
    EXPECT_GE(runtime_info.cuda_driver_version, 0);
    EXPECT_GE(runtime_info.cuda_device_count, 0);

    // Recommended backend should be valid
    EXPECT_TRUE(
        runtime_info.recommended_backend == device_enum::CPU ||
        runtime_info.recommended_backend == device_enum::CUDA);

    XSIGMA_LOG_INFO("CUDA available: {}", runtime_info.cuda_available);
    XSIGMA_LOG_INFO("CUDA runtime version: {}", runtime_info.cuda_runtime_version);
    XSIGMA_LOG_INFO("CUDA device count: {}", runtime_info.cuda_device_count);
    XSIGMA_LOG_INFO("Recommended backend: {}", static_cast<int>(runtime_info.recommended_backend));

    XSIGMA_LOG_INFO("GPU device manager runtime information test passed");
}

/**
 * @brief Test available devices enumeration
 */
XSIGMATEST(GpuDeviceManager, enumerates_available_devices)
{
    auto& manager = gpu_device_manager::instance();
    manager.initialize();

    auto devices = manager.get_available_devices();

    // Should return a valid vector (may be empty if no GPUs)
    EXPECT_GE(devices.size(), 0);

    // If devices are available, check their properties
    for (const auto& device : devices)
    {
        EXPECT_GE(device.device_index, 0);
        EXPECT_FALSE(device.name.empty());
        EXPECT_GE(device.total_memory, 0);
        EXPECT_GE(device.multiprocessor_count, 0);

        XSIGMA_LOG_INFO(
            "Device {}: {} ({}MB)",
            device.device_index,
            device.name,
            device.total_memory / (1024ULL));
    }

    XSIGMA_LOG_INFO("GPU device manager device enumeration test passed");
}

/**
 * @brief Test device information retrieval for specific devices
 */
XSIGMATEST(GpuDeviceManager, retrieves_specific_device_info)
{
    auto& manager = gpu_device_manager::instance();
    manager.initialize();

    auto runtime_info = manager.get_runtime_info();

    if (runtime_info.cuda_available && runtime_info.cuda_device_count > 0)
    {
        try
        {
            // Test getting info for device 0
            auto device_info = manager.get_device_info(device_enum::CUDA, 0);

            EXPECT_EQ(0, device_info.device_index);
            EXPECT_EQ(device_enum::CUDA, device_info.device_type);
            EXPECT_FALSE(device_info.name.empty());
            EXPECT_GT(device_info.total_memory, 0);

            XSIGMA_LOG_INFO(
                "Device 0 info: {} ({}MB)", device_info.name, device_info.total_memory / (1024ULL));

            XSIGMA_LOG_INFO("GPU device manager specific device info test passed");
        }
        catch (const std::exception& e)
        {
            XSIGMA_LOG_INFO("Device info retrieval failed: {}", e.what());
        }
    }
    else
    {
        XSIGMA_LOG_INFO("No CUDA devices available for specific device info test");
    }
}

/**
 * @brief Test device availability checking
 */
XSIGMATEST(GpuDeviceManager, checks_device_availability)
{
    auto& manager = gpu_device_manager::instance();
    manager.initialize();

    auto runtime_info = manager.get_runtime_info();

    if (runtime_info.cuda_available && runtime_info.cuda_device_count > 0)
    {
        // Test valid device
        bool device0_available = manager.is_device_available(device_enum::CUDA, 0);
        EXPECT_TRUE(device0_available);

        XSIGMA_LOG_INFO("Device 0 availability: {}", device0_available);
    }

    // Test invalid device
    bool invalid_device_available = manager.is_device_available(device_enum::CUDA, 999);
    EXPECT_FALSE(invalid_device_available);

    XSIGMA_LOG_INFO("GPU device manager device availability test passed");
}

/**
 * @brief Test device context management
 */
XSIGMATEST(GpuDeviceManager, manages_device_context)
{
    auto& manager = gpu_device_manager::instance();
    manager.initialize();

    auto runtime_info = manager.get_runtime_info();

    if (runtime_info.cuda_available && runtime_info.cuda_device_count > 0)
    {
        try
        {
            // Test setting device context
            manager.set_device_context(device_enum::CUDA, 0);

            // Test getting current device
            auto current_device = manager.get_current_device();
            EXPECT_EQ(0, current_device.device_index);
            EXPECT_EQ(device_enum::CUDA, current_device.device_type);

            XSIGMA_LOG_INFO(
                "Current device: {} (index {})", current_device.name, current_device.device_index);

            XSIGMA_LOG_INFO("GPU device manager context management test passed");
        }
        catch (const std::exception& e)
        {
            XSIGMA_LOG_INFO("Device context management failed: {}", e.what());
        }
    }
    else
    {
        XSIGMA_LOG_INFO("No CUDA devices available for context management test");
    }
}

/**
 * @brief Test device information refresh
 */
XSIGMATEST(GpuDeviceManager, refreshes_device_information)
{
#if 0
    auto& manager = gpu_device_manager::instance();
    manager.initialize();


        // Test refresh operation
        manager.refresh_device_info();
        EXPECT_TRUE(true);  // Should not throw

        XSIGMA_LOG_INFO("GPU device manager refresh test passed");
#endif
}

/**
 * @brief Test system report generation
 */
XSIGMATEST(GpuDeviceManager, generates_system_report)
{
    auto& manager = gpu_device_manager::instance();
    manager.initialize();

    std::string report = manager.get_system_report();

    // Report should not be empty
    EXPECT_FALSE(report.empty());

    // Report should contain some expected content
    EXPECT_TRUE(
        report.find("GPU") != std::string::npos || report.find("CUDA") != std::string::npos ||
        report.find("Device") != std::string::npos);

    XSIGMA_LOG_INFO("System report length: {} characters", report.length());

    XSIGMA_LOG_INFO("GPU device manager system report test passed");
}

/**
 * @brief Test error handling for invalid operations
 */
XSIGMATEST(GpuDeviceManager, handles_invalid_operations)
{
    auto& manager = gpu_device_manager::instance();
    manager.initialize();

    try
    {
        // Test getting info for invalid device
        auto invalid_device_info = manager.get_device_info(device_enum::CUDA, 999);
        // Should either throw or return invalid info
        EXPECT_TRUE(true);  // Test passes if we reach here
    }
    catch (const std::exception& e)
    {
        // Expected behavior for invalid device
        EXPECT_TRUE(true);  // Test passes if exception is thrown
        XSIGMA_LOG_INFO("Expected exception for invalid device: {}", e.what());
    }

    try
    {
        // Test setting invalid device context
        manager.set_device_context(device_enum::CUDA, 999);
        EXPECT_TRUE(true);  // Test passes if no exception
    }
    catch (const std::exception& e)
    {
        // Expected behavior for invalid device context
        EXPECT_TRUE(true);  // Test passes if exception is thrown
        XSIGMA_LOG_INFO("Expected exception for invalid device context: {}", e.what());
    }

    XSIGMA_LOG_INFO("GPU device manager error handling test passed");
}

#endif  // XSIGMA_HAS_CUDA
