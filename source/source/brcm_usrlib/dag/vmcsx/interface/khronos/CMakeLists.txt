
# Khronos

add_library(EGL SHARED
            egl/egl_client_config.c
            egl/egl_client_context.c
            egl/egl_client.c
            egl/egl_client_surface.c
            ext/egl_brcm_driver_monitor_client.c
	    ext/egl_brcm_global_image_client.c
            ext/egl_khr_image_client.c
            ext/egl_khr_sync_client.c
            ext/gl_oes_egl_image_client.c
            ext/egl_khr_lock_surface_client.c
            common/khrn_int_image.c
            common/khrn_int_util.c
            common/khrn_client_global_image_map.c
            common/linux/khrn_client_rpc_linux.c
            common/linux/khrn_client_platform_linux.c
	    vg/vg_int_mat3x3.c
            vg/vg_client.c
            common/khrn_client.c
            ext/egl_openmaxil_client.c)

add_library(GLESv2 SHARED
            glxx/glxx_client.c)

add_library(khrn_client
            common/khrn_client_pointermap.c
	    common/khrn_client_vector.c
            common/khrn_int_hash.c
            common/khrn_client_cache.c
           )

add_library(WFC
            wf/wfc_client_stream.c
            wf/wfc_client_utils.c
            wf/wfc_client.c
            )

add_library(VG SHARED
            vg/vg_client.c
            vg/vg_int_mat3x3.c)

target_link_libraries(EGL vcos GLESv2 vchiq_arm -lm)
target_link_libraries(GLESv2 khrn_client vcos)
target_link_libraries(VG EGL)

install(TARGETS EGL GLESv2 khrn_client WFC VG DESTINATION lib)

