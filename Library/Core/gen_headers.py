#!/usr/bin/env python3
"""Generate XSigma configuration headers."""

import sys
import os


def gen_version_macros(output_file):
    """Generate xsigma_version_macros.h"""
    content = """#ifndef XSIGMA_VERSION_MACROS_H
#define XSIGMA_VERSION_MACROS_H

#define XSIGMA_MAJOR_VERSION 1
#define XSIGMA_MINOR_VERSION 0
#define XSIGMA_BUILD_VERSION 0
#define XSIGMA_VERSION_FULL "1.0.0"

#endif // XSIGMA_VERSION_MACROS_H
"""
    with open(output_file, 'w') as f:
        f.write(content)


def gen_threads_header(output_file, use_win32=False):
    """Generate xsigma_threads.h"""
    if use_win32:
        content = """#ifndef XSIGMA_THREADS_H
#define XSIGMA_THREADS_H

#define XSIGMA_USE_WIN32_THREADS 1
#define XSIGMA_MAX_THREADS 64

#endif // XSIGMA_THREADS_H
"""
    else:
        content = """#ifndef XSIGMA_THREADS_H
#define XSIGMA_THREADS_H

#define XSIGMA_USE_PTHREADS 1
#define XSIGMA_MAX_THREADS 64

#endif // XSIGMA_THREADS_H
"""
    with open(output_file, 'w') as f:
        f.write(content)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: gen_headers.py <command> [output_file]")
        sys.exit(1)
    
    command = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    if command == 'version' and output_file:
        gen_version_macros(output_file)
    elif command == 'threads' and output_file:
        use_win32 = len(sys.argv) > 3 and sys.argv[3] == 'win32'
        gen_threads_header(output_file, use_win32)
    else:
        print(f"Unknown command: {command}")
        sys.exit(1)

