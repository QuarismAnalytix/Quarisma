# smp_new Code Review

## High Severity
- Missing exported definition for `internal::GetIntraopPool()` causes every translation unit that instantiates the templates in `parallel_api.hxx` to fail at link time. The header declares the symbol (`Library/Core/smp_new/parallel/parallel_api.hxx:22`), but the only implementation lives in an anonymous namespace (`Library/Core/smp_new/parallel/parallel_api.cxx:43`), so no external definition is emitted.
- `parallelize_1d` blocks on `internal::GetIntraopPool().WaitWorkComplete()` (`Library/Core/smp_new/parallel/parallelize_1d.cxx:118`). Because the intra-op pool is a shared singleton, this waits for *all* outstanding work in that pool, not just the partitions queued for the current coordinator. Any unrelated work (or new submissions racing in) can stall the call indefinitely and serialise other users of the pool.
- The “serial fast-path” of `parallel_reduce` returns before it restores the thread-local state (`Library/Core/smp_new/parallel/parallel_api.hxx:185-198`). After a small reduction, `in_parallel_region()` keeps reporting `true` and the thread id remains stuck at `0`, breaking the API contract for subsequent callers.

## Medium Severity
- Work stealing enqueues additional ranges without bumping `pending_work_`, but `mark_complete()` still decrements it (`Library/Core/smp_new/parallel/parallelize_1d.cxx:104`, `Library/Core/smp_new/parallel/parallelize_1d.cxx:130`, `Library/Core/smp_new/parallel/parallelize_1d.cxx:197`). Once stealing happens the counter underflows, so `wait_complete()` can never observe zero and will block forever if it is ever used.
- `parallel_for` / `parallel_reduce` leave `internal::set_thread_num()` pointing at the last task id instead of restoring the caller’s previous value (`Library/Core/smp_new/parallel/parallel_api.hxx:120-133`, `Library/Core/smp_new/parallel/parallel_api.hxx:214-229`). Any code that queries `get_thread_num()` after the parallel section sees stale data.
- The per-task counters in `parallel_for` / `parallel_reduce` are `std::atomic<int>` (`Library/Core/smp_new/parallel/parallel_api.hxx:84-101`, `Library/Core/smp_new/parallel/parallel_api.hxx:208-219`). Large iteration spaces easily exceed `INT_MAX`, causing overflow and undefined behaviour while indexing the per-chunk buffers.

## Additional Observations
- `Parallelize1DCoordinator` sizes its work-stealing dequeues using `DefaultNumThreads()` instead of the actual intra-op pool size (`Library/Core/smp_new/parallel/parallelize_1d.cxx:72`), so it may spawn more coordinator tasks than worker threads. Consider querying the pool for consistency once the pool access bug is fixed.
- The thread-local helpers for the TBB backend never populate `g_thread_id`, so `GetTBBThreadNum()` always reports `0` (`Library/Core/smp_new/tbb/parallel_tbb.cxx:94`). If the API needs real thread ids, that logic still has to be implemented.
