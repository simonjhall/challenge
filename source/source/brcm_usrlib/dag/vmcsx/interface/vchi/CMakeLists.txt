
# VCHI - just needed for condition variable implementation.
# Should be hoisted.

add_library(vchi os/vcos/vcos_cv.c)

install(TARGETS vchi DESTINATION lib)

