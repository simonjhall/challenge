

add_library(vcos_generic
            vcos_generic_event_flags.c
            vcos_mem_from_malloc.c
            vcos_generic_named_sem.c
            vcos_generic_reentrant_mtx.c
            vcos_abort.c
            vcos_msgqueue.c
            vcos_logcat.c
           )


install(FILES
   vcos_common.h
   vcos_generic_event_flags.h
   vcos_generic_named_sem.h
   vcos_generic_quickslow_mutex.h
   vcos_generic_reentrant_mtx.h
   vcos_generic_tls.h
   vcos_joinable_thread_from_plain.h
   vcos_latch_from_sem.h
   vcos_mem_from_malloc.h
   vcos_mutexes_are_reentrant.h
   vcos_thread_reaper.h
   DESTINATION include/interface/vcos/generic)

