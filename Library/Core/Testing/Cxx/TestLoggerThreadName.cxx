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

// Test that xsigmaLogger::GetThreadName is unaffected by concurrent accesses
// and usage of xsigmaLogger::Init()

#include <atomic>
#include <string>
#include <thread>

#include "logging/logger.h"
#include "xsigmaTest.h"

// Control the order of operations between the threads
std::atomic_bool wait1;
std::atomic_bool wait2;

void Thread1()
{
    const std::string threaName = "T1";
    while (!wait1.load()) {}

    xsigma::logger::SetThreadName(threaName);

    wait2.store(true);

    if (xsigma::logger::GetThreadName() != threaName)
    {
        XSIGMA_LOG(ERROR, "Name mismatch !");
    }
}

void Thread2()
{
    const std::string threaName = "T2";
    xsigma::logger::SetThreadName(threaName);

    wait1.store(true);
    while (!wait2.load()) {}

    xsigma::logger::Init();

    if (xsigma::logger::GetThreadName() != threaName)
    {
        XSIGMA_LOG(ERROR, "Name mismatch !");
    }
}

XSIGMATEST(Logger, thread_name)
{
    XSIGMA_UNUSED int    arg     = 0;
    XSIGMA_UNUSED char** arg_str = nullptr;

    wait1.store(false);
    wait2.store(false);
    std::thread t1(Thread1);
    std::thread t2(Thread2);

    t1.join();
    t2.join();
    END_TEST();
}
