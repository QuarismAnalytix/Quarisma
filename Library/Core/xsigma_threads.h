#ifndef XSIGMA_THREADS_H
#define XSIGMA_THREADS_H

#if defined(_WIN32) || defined(_WIN64)
#define XSIGMA_USE_WIN32_THREADS 1
#else
#define XSIGMA_USE_PTHREADS 1
#endif

#define XSIGMA_MAX_THREADS 64

#endif  // XSIGMA_THREADS_H
