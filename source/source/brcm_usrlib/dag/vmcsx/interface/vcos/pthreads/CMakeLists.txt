
add_library(vcos SHARED vcos_pthreads.c vcos_dlfcn.c ../glibc/vcos_backtrace.c)

add_subdirectory(../generic vcos_generic)

find_package(Threads REQUIRED)
target_link_libraries(vcos vcos_generic ${CMAKE_THREAD_LIBS_INIT} -lrt -ldl)

install(FILES vcos_platform.h vcos_platform_types.h
        DESTINATION include)

install(TARGETS vcos DESTINATION lib)

