
add_subdirectory(pthreads)

install(FILES
    vcos_assert.h
    vcos_atomic_flags.h
    vcos_dlfcn.h
    vcos_event_flags.h
    vcos_event.h
    vcos.h
    vcos_init.h
    vcos_isr.h
    vcos_legacy_isr.h
    vcos_logging.h
    vcos_lowlevel_thread.h
    vcos_mem.h
    vcos_mempool.h
    vcos_msgqueue.h
    vcos_mutex.h
    vcos_named_semaphore.h
    vcos_queue.h
    vcos_quickslow_mutex.h
    vcos_reentrant_mutex.h
    vcos_semaphore.h
    vcos_string.h
    vcos_thread_attr.h
    vcos_thread.h
    vcos_timer.h
    vcos_tls.h
    vcos_types.h

    DESTINATION include/interface/vcos)

