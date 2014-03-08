
add_definitions(-Werror)
add_executable(vcfiled vcfiled.c)

target_link_libraries(vcfiled
                      vchostif
                      vchiq_arm
                      vcos)

install(TARGETS vcfiled 
        RUNTIME DESTINATION sbin)

configure_file (etc/init.d/vcfiled ${PROJECT_BINARY_DIR}/etc/init.d/vcfiled)

install(PROGRAMS ${PROJECT_BINARY_DIR}/etc/init.d/vcfiled
        DESTINATION /etc/init.d)
# install locally to the installation directory too
install(PROGRAMS ${PROJECT_BINARY_DIR}/etc/init.d/vcfiled
        DESTINATION ${VMCS_INSTALL_PREFIX}/share/install)


