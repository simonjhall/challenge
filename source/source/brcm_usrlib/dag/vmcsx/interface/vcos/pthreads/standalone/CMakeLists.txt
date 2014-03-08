
cmake_minimum_required(VERSION 2.8)
project (vcos)
set (vmcs_root ../../../..)
include(${vmcs_root}/makefiles/cmake/vmcs.cmake)
include_directories(${vmcs_root})
include_directories(${vmcs_root}/interface/vcos/pthreads)
add_subdirectory(../.. vcos)

