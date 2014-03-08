
# interface/vmcs_host

add_library(vchostif
            ${VMCS_TARGET}/vcfilesys.c ${VMCS_TARGET}/vcmisc.c
            ${VMCS_TARGET}/vmcs_main.c
            vc_vchi_gencmd.c vc_vchi_filesys.c
            vc_vchi_tvservice.c vc_vchi_cecservice.c
            vc_vchi_dispmanx.c
            ../vchi/os/vcos/vcos_os.c)

# OpenMAX/IL component service
add_library(vcilcs
            vcilcs.c vcilcs_in.c vcilcs_out.c vcilcs_common.c)

# ILCS pulls in EGL for the ILCS/EGL surface API support
target_link_libraries(vcilcs EGL GLESv2 khrn_client vchiq_arm vcos)

# vchostif needs ilcore as well (vmcs_main pulls it in)
target_link_libraries(vchostif vmcs_rpc_client vchiq_arm vcos)

add_subdirectory(linux/vcfiled)

install(TARGETS vchostif bufman DESTINATION lib)

