/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/egl/egl_platform.h"
#include "middleware/khronos/egl/egl_server.h"

#ifdef EGL_BRCM_driver_monitor

#include <stdio.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

bool eglInitDriverMonitorBRCM_impl(EGLint hw_bank, EGLint l3c_bank)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   if (state->driver_monitor_refcount == 0) 
   {
      khrn_reset_driver_counters(hw_bank, l3c_bank);
   }

   state->driver_monitor_refcount++;

   return true;
}

void eglTermDriverMonitorBRCM_impl()
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   vcos_assert(state->driver_monitor_refcount > 0);

   state->driver_monitor_refcount--;

   if (state->driver_monitor_refcount == 0) 
   {
      khrn_reset_driver_counters(-1, -1);
   }
}

static void xml_line32(char *xml, EGLint bufsiz, const char *tag, uint32_t value)
{
   char buf[512];
   sprintf(buf, "<%s>0x%08X</%s>\n", tag, value, tag);
   strncat(xml, buf, bufsiz - strlen(xml) - 1);
   xml[bufsiz - 1] = '\0';
}

static void xml_line64(char *xml, EGLint bufsiz, const char *tag, uint64_t value)
{
   char buf[512];
   sprintf(buf, "<%s>0x%08X%08X</%s>\n", tag, (uint32_t)(value >> 32), (uint32_t)(value & 0xFFFFFFFF), tag);
   strncat(xml, buf, bufsiz - strlen(xml) - 1);
   xml[bufsiz - 1] = '\0';
}

void eglGetDriverMonitorXMLBRCM_impl(EGLint bufSize, char *xmlStats)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   if (state->driver_monitor_refcount && bufSize > 0 && xmlStats != NULL) 
   {
      KHRN_DRIVER_COUNTERS_T *counters = khrn_driver_counters();

      xmlStats[0] = '\0';

      xml_line32(xmlStats, bufSize, "hard_clears",           counters->hard_clears);            
      xml_line32(xmlStats, bufSize, "soft_clears",           counters->soft_clears);            
      xml_line32(xmlStats, bufSize, "frame_color_loads",     counters->tb_grp_color_loads);     
      xml_line32(xmlStats, bufSize, "frame_ms_color_loads",  counters->tb_grp_ms_color_loads);  
      xml_line32(xmlStats, bufSize, "frame_ds_loads",        counters->tb_grp_ds_loads);        
      xml_line32(xmlStats, bufSize, "frame_color_stores",    counters->tb_grp_color_stores);    
      xml_line32(xmlStats, bufSize, "frame_ms_color_stores", counters->tb_grp_ms_color_stores); 
      xml_line32(xmlStats, bufSize, "frame_ds_stores",       counters->tb_grp_ds_stores);       
      xml_line32(xmlStats, bufSize, "tile_color_loads",      counters->tb_color_loads);         
      xml_line32(xmlStats, bufSize, "tile_ms_color_loads",   counters->tb_ms_color_loads);      
      xml_line32(xmlStats, bufSize, "tile_ds_loads",         counters->tb_ds_loads);            
      xml_line32(xmlStats, bufSize, "tile_color_stores",     counters->tb_color_stores);        
      xml_line32(xmlStats, bufSize, "tile_ms_color_stores",  counters->tb_ms_color_stores);     
      xml_line32(xmlStats, bufSize, "tile_ds_stores",        counters->tb_ds_stores);           

      if (counters->hw_group_active == 0)
      {
         xml_line64(xmlStats, bufSize, "qpu_cycles_idle",       counters->hw.hw_0.qpu_cycles_idle);      
         xml_line64(xmlStats, bufSize, "qpu_cycles_vert_shade", counters->hw.hw_0.qpu_cycles_vert_shade);
         xml_line64(xmlStats, bufSize, "qpu_cycles_frag_shade", counters->hw.hw_0.qpu_cycles_frag_shade);
         xml_line64(xmlStats, bufSize, "qpu_cycles_exe_valid",  counters->hw.hw_0.qpu_cycles_exe_valid);
         xml_line64(xmlStats, bufSize, "qpu_cycles_wait_tmu",   counters->hw.hw_0.qpu_cycles_wait_tmu);
         xml_line64(xmlStats, bufSize, "qpu_cycles_wait_scb",   counters->hw.hw_0.qpu_cycles_wait_scb);
         xml_line64(xmlStats, bufSize, "qpu_cycles_wait_vary",  counters->hw.hw_0.qpu_cycles_wait_vary);
         xml_line64(xmlStats, bufSize, "qpu_icache_hits",       counters->hw.hw_0.qpu_icache_hits);
         xml_line64(xmlStats, bufSize, "qpu_icache_miss",       counters->hw.hw_0.qpu_icache_miss);
         xml_line64(xmlStats, bufSize, "qpu_ucache_hits",       counters->hw.hw_0.qpu_ucache_hits);
         xml_line64(xmlStats, bufSize, "qpu_ucache_miss",       counters->hw.hw_0.qpu_ucache_miss);
         xml_line64(xmlStats, bufSize, "tmu_total_quads",       counters->hw.hw_0.tmu_total_quads);
         xml_line64(xmlStats, bufSize, "tmu_cache_miss",        counters->hw.hw_0.tmu_cache_miss);
         xml_line64(xmlStats, bufSize, "l2c_hits",              counters->hw.hw_0.l2c_hits);
         xml_line64(xmlStats, bufSize, "l2c_miss",              counters->hw.hw_0.l2c_miss);
      }
      else if (counters->hw_group_active == 1)
      {
         xml_line64(xmlStats, bufSize, "fep_valid_prims",                    counters->hw.hw_1.fep_valid_prims);
         xml_line64(xmlStats, bufSize, "fep_valid_prims_no_pixels",          counters->hw.hw_1.fep_valid_prims_no_pixels);
         xml_line64(xmlStats, bufSize, "fep_earlyz_clipped_quads",           counters->hw.hw_1.fep_earlyz_clipped_quads);
         xml_line64(xmlStats, bufSize, "fep_valid_quads",                    counters->hw.hw_1.fep_valid_quads);
         xml_line64(xmlStats, bufSize, "tlb_quads_no_stencil_pass_pixels",   counters->hw.hw_1.tlb_quads_no_stencil_pass_pixels);
         xml_line64(xmlStats, bufSize, "tlb_quads_no_z_stencil_pass_pixels", counters->hw.hw_1.tlb_quads_no_z_stencil_pass_pixels);
         xml_line64(xmlStats, bufSize, "tlb_quads_z_stencil_pass_pixels",    counters->hw.hw_1.tlb_quads_z_stencil_pass_pixels);
         xml_line64(xmlStats, bufSize, "tlb_quads_all_pixels_zero_cvg",      counters->hw.hw_1.tlb_quads_all_pixels_zero_cvg);
         xml_line64(xmlStats, bufSize, "tlb_quads_all_pixels_nonzero_cvg",   counters->hw.hw_1.tlb_quads_all_pixels_nonzero_cvg);
         xml_line64(xmlStats, bufSize, "tlb_quads_valid_pixels_written",     counters->hw.hw_1.tlb_quads_valid_pixels_written);
         xml_line64(xmlStats, bufSize, "ptb_prims_viewport_discarded",       counters->hw.hw_1.ptb_prims_viewport_discarded);
         xml_line64(xmlStats, bufSize, "ptb_prims_needing_clip",             counters->hw.hw_1.ptb_prims_needing_clip);
         xml_line64(xmlStats, bufSize, "pse_prims_reverse_discarded",        counters->hw.hw_1.pse_prims_reverse_discarded);
         xml_line64(xmlStats, bufSize, "vpm_cycles_vdw_stalled",             counters->hw.hw_1.vpm_cycles_vdw_stalled);
         xml_line64(xmlStats, bufSize, "vpm_cycles_vcd_stalled",             counters->hw.hw_1.vpm_cycles_vcd_stalled);
      }

      if (counters->l3c_group_active == 0)
      {
         xml_line64(xmlStats, bufSize, "l3c_v3d_read_bandwidth", counters->l3c.l3c_read_bw_0 * 16);
         xml_line64(xmlStats, bufSize, "l3c_mem_read_bandwidth", counters->l3c_mem.l3c_mem_read_bw_0 * 32);
      }
      else if (counters->l3c_group_active == 1)
      {
         xml_line64(xmlStats, bufSize, "l3c_v3d_write_bandwidth", counters->l3c.l3c_write_bw_1 * 16);
         xml_line64(xmlStats, bufSize, "l3c_mem_write_bandwidth", counters->l3c_mem.l3c_mem_write_bw_1 * 32);
      }

      xmlStats[bufSize-1] = '\0';
   }
}

#endif
