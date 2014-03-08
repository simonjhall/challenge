/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "middleware/khronos/glsl/glsl_common.h"
#include "middleware/khronos/glsl/glsl_dataflow.h"
#include "middleware/khronos/glsl/2708/glsl_allocator_4.h"
#include "middleware/khronos/gl20/gl20_config.h"
//#include "v3d/verification/tools/2760sim/simpenrose.h"
#include "middleware/khronos/glxx/2708/glxx_shader_4.h"
#include "middleware/khronos/glxx/2708/glxx_attr_sort_4.h"
#include "middleware/khronos/gl20/2708/gl20_shader_4.h"
#if !defined(V3D_LEAN)
#include "helpers/vclib/vclib.h"
#else
#define vclib_memcmp memcmp
#endif
#include <assert.h>

static Dataflow *fetch_all_es20_attributes(Dataflow **attrib, GLXX_ATTRIB_ABSTRACT_T *abstract, uint32_t attribs_live, uint32_t * attribs_order)
{
   uint32_t i;
   uint32_t uniform_index[GLXX_CONFIG_MAX_VERTEX_ATTRIBS];
   Dataflow *dep;
   
   dep = NULL;
   
   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
      uniform_index[i] = GLXX_ATTRIB_OFFSET(i);
      
   return glxx_fetch_all_attributes(abstract, uniform_index, attrib, dep, attribs_live, attribs_order);

//   return dep;
}

static Dataflow * init_frag_vary(Dataflow **frag_vary, uint32_t num_varyings, uint32_t primitive_type)
{
   uint32_t i;
   Dataflow *dep = NULL;

   memset(frag_vary, 0, 4*64);
   if (primitive_type == 0)
   {
      /* points */
      dep = frag_vary[0] = glxx_vary(0, dep);
      dep = frag_vary[1] = glxx_vary(1, dep);
   }
   else
   {
      if (primitive_type == 1)  /* lines */
         dep = glxx_vary(0, dep);
      frag_vary[0] = glxx_cfloat(0.0f);
      frag_vary[1] = glxx_cfloat(0.0f);
   }

   for (i = 0; i < num_varyings; i++)
      frag_vary[32+i] = glxx_vary(32 + i, dep);

   return dep;
}

uint32_t xxx_shader;
bool gl20_hw_emit_shaders(GL20_LINK_RESULT_T *link_result, GLXX_LINK_RESULT_KEY_T *key, GLXX_LINK_RESULT_DATA_T *data, void *base)
{
   Dataflow *shaded[GL20_LINK_RESULT_NODE_COUNT-5];
   Dataflow *frag_color[5];
   Dataflow *attrib[4*GLXX_CONFIG_MAX_VERTEX_ATTRIBS];
   Dataflow *last_vpm_read, *last_vpm_write, *root;
   Dataflow *frag_vary[64];
   Dataflow *point_size;
   Dataflow *dep;
   uint32_t vary_map[32];
   uint32_t vary_count;
   bool result = true;
   bool has_point_size;
   bool use_offline;
   bool boilerplate = false;
   uint32_t fragment_shader_type = GLSL_BACKEND_TYPE_FRAGMENT;
   bool *texture_rb_swap = NULL;

#if GL_EXT_texture_format_BGRA8888
   texture_rb_swap = key->texture_rb_swap;
#endif

#ifdef WORKAROUND_HW2924
   if (key->workaround_hw2924)
   {
      boilerplate = true;
      if (key->blend.ms && (glxx_hw_blend_enabled(key->blend) || key->blend.color_mask != 0xffffffff))
         fragment_shader_type |= GLSL_BACKEND_TYPE_MULTISAMPLE_HW2924;
      else
         fragment_shader_type |= GLSL_BACKEND_TYPE_SIMPLE_HW2924;
   }
#endif


   /* fragment shader */
   dep = init_frag_vary(frag_vary, link_result->vary_count, key->primitive_type);
   glsl_dataflow_copy(5, frag_color, (Dataflow **)link_result->nodes, base, frag_vary, 64, DATAFLOW_VARYING, texture_rb_swap);
   root = glxx_backend(key->blend, glxx_vec4(frag_color[0], frag_color[1], frag_color[2], frag_color[3]), frag_color[4], key->use_stencil, key->render_alpha, key->rgb565, boilerplate);
   glxx_iodep(root, dep);
   data->threaded = true;
   if (!glxx_schedule(root, fragment_shader_type, &data->mh_fcode, &data->mh_funiform_map, &data->threaded, vary_map, &vary_count, key->fb_rb_swap))
   {
      /* try again as non-threaded shader */
      init_frag_vary(frag_vary, link_result->vary_count, key->primitive_type);
      glsl_dataflow_copy(5, frag_color, (Dataflow **)link_result->nodes, base, frag_vary, 64, DATAFLOW_VARYING, texture_rb_swap);
      root = glxx_backend(key->blend, glxx_vec4(frag_color[0], frag_color[1], frag_color[2], frag_color[3]), frag_color[4], key->use_stencil, key->render_alpha, key->rgb565, boilerplate);
      data->threaded = false;
      result &= glxx_schedule(root, fragment_shader_type, &data->mh_fcode, &data->mh_funiform_map, &data->threaded, vary_map, &vary_count, key->fb_rb_swap);
   }
   data->num_varyings = vary_count;

   /*{
      printf("***Shader %08x is program %d\n", simpenrose_hw_addr(mem_lock(data->mh_fcode)), xxx_shader);
      mem_unlock(data->mh_fcode);
   }*/

   use_offline = false;   /* TODO */
   if (use_offline)
   {
      /* offline vertex shader */
      last_vpm_read = fetch_all_es20_attributes(attrib, key->attribs, link_result->vattribs_live, data->vattribs_order);
      glsl_dataflow_copy(5+link_result->vary_count, shaded, (Dataflow **)link_result->nodes+5, base, attrib, 4*GLXX_CONFIG_MAX_VERTEX_ATTRIBS, DATAFLOW_ATTRIBUTE, NULL);
      has_point_size = key->primitive_type == 0 && shaded[4] != NULL;
      point_size = has_point_size ? shaded[4] : NULL;
      last_vpm_write = glxx_vertex_backend(shaded[0], shaded[1], shaded[2], shaded[3], point_size, true, true, shaded+5, vary_map, vary_count, key->rso_format);
      result &= glxx_schedule(last_vpm_write, GLSL_BACKEND_TYPE_OFFLINE_VERTEX, &data->mh_vcode, &data->mh_vuniform_map, NULL, NULL, NULL, key->fb_rb_swap);
   }
   else
   {
      /* coordinate shader */
      last_vpm_read = fetch_all_es20_attributes(attrib, key->attribs, link_result->cattribs_live, data->cattribs_order);
      glsl_dataflow_copy(5, shaded, (Dataflow **)link_result->nodes+5, base, attrib, 4*GLXX_CONFIG_MAX_VERTEX_ATTRIBS, DATAFLOW_ATTRIBUTE, NULL);
      has_point_size = key->primitive_type == 0 && shaded[4] != NULL;
      point_size = has_point_size ? shaded[4] : NULL;
      last_vpm_write = glxx_vertex_backend(shaded[0], shaded[1], shaded[2], shaded[3], point_size, true, false, NULL, NULL, 0, key->rso_format);
      glxx_iodep(last_vpm_write, last_vpm_read);
      result &= glxx_schedule(last_vpm_write, GLSL_BACKEND_TYPE_VERTEX_OR_COORD, &data->mh_ccode, &data->mh_cuniform_map, NULL, NULL, NULL, key->fb_rb_swap);

      //if (xxx_shader == 122) glsl_allocator_dump();

      /* vertex shader */
      last_vpm_read = fetch_all_es20_attributes(attrib, key->attribs, link_result->vattribs_live, data->vattribs_order);
      glsl_dataflow_copy(5+link_result->vary_count, shaded, (Dataflow **)link_result->nodes+5, base, attrib, 4*GLXX_CONFIG_MAX_VERTEX_ATTRIBS, DATAFLOW_ATTRIBUTE, NULL);
      point_size = has_point_size ? shaded[4] : NULL;
      last_vpm_write = glxx_vertex_backend(shaded[0], shaded[1], shaded[2], shaded[3], point_size, false, true, shaded+5, vary_map, vary_count, key->rso_format);
      glxx_iodep(last_vpm_write, last_vpm_read);
      result &= glxx_schedule(last_vpm_write, GLSL_BACKEND_TYPE_VERTEX_OR_COORD, &data->mh_vcode, &data->mh_vuniform_map, NULL, NULL, NULL, key->fb_rb_swap);

      //if (xxx_shader == 72) glsl_allocator_dump();
   }
   data->has_point_size = has_point_size;
   data->use_offline = use_offline;

   return result;
}


void gl20_link_result_term(void *v, uint32_t size)
{
   GL20_LINK_RESULT_T *result = (GL20_LINK_RESULT_T *)v;
   uint32_t i;

   UNUSED(size);

   MEM_ASSIGN(result->mh_blob, MEM_INVALID_HANDLE);

   for (i = 0; i < GL20_LINK_RESULT_CACHE_SIZE; i++)
   {
      MEM_ASSIGN(result->cache[i].data.mh_vcode, MEM_INVALID_HANDLE);
      MEM_ASSIGN(result->cache[i].data.mh_ccode, MEM_INVALID_HANDLE);
      MEM_ASSIGN(result->cache[i].data.mh_fcode, MEM_INVALID_HANDLE);
      MEM_ASSIGN(result->cache[i].data.mh_vuniform_map, MEM_INVALID_HANDLE);
      MEM_ASSIGN(result->cache[i].data.mh_cuniform_map, MEM_INVALID_HANDLE);
      MEM_ASSIGN(result->cache[i].data.mh_funiform_map, MEM_INVALID_HANDLE);
   }
}

static bool create_shader(GL20_LINK_RESULT_T *link_result, uint32_t cache_index)
{
   void *base;
   bool result;

   base = mem_lock(link_result->mh_blob);

   glsl_fastmem_init();
   if (setjmp(g_ErrorHandlerEnv) != 0)
	{
		// We must be jumping back from an error.
      glsl_fastmem_term();
      mem_unlock(link_result->mh_blob);
		return false;
	}

   glsl_init_primitive_values();

   result = gl20_hw_emit_shaders(link_result, &link_result->cache[cache_index].key, &link_result->cache[cache_index].data, base);

   glsl_fastmem_term();
   mem_unlock(link_result->mh_blob);
   return result;
}

bool gl20_link_result_get_shaders(
   GL20_LINK_RESULT_T *link_result,
   GLXX_HW_SHADER_RECORD_T *shader_out,
   MEM_HANDLE_T *cunifmap_out,
   MEM_HANDLE_T *vunifmap_out,
   MEM_HANDLE_T *funifmap_out,
   GLXX_SERVER_STATE_T *state,
   GLXX_ATTRIB_T *attrib,
   uint32_t *mergeable_attribs,
   uint32_t * cattribs_order_out,
   uint32_t * vattribs_order_out)
{
   uint32_t i, j;
   GLXX_LINK_RESULT_KEY_T *key;
   bool found;

   key = &state->shader.common;
   
   //TODO: vertex hiding
   //vcos_assert(0);
   /*for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
   {
      if ((link_result->vattribs_live & 15<<(4*i)) && attrib[i].enabled)
      {
         key.attribs[i].size = attrib[i].size;
         key.attribs[i].type = attrib[i].type;
         key.attribs[i].norm = !!attrib[i].normalized;
      }
      else
      {
         key.attribs[i].size = 0;
         key.attribs[i].type = 0;
         key.attribs[i].norm = false;
      }
   }*/

   found = false;

#if !defined(V3D_LEAN)
   vclib_obtain_VRF(1);
#endif

   for (i = 0; i < link_result->cache_used; i++)
   {
      if (link_result->cache[i].used && !vclib_memcmp(&link_result->cache[i].key, key, sizeof(GLXX_LINK_RESULT_KEY_T)))
      {
         found = true;
         break;
      }
   }

#if !defined(V3D_LEAN)
   vclib_release_VRF();
#endif

   if (!found)
   {
      /* Compile new version of this shader and add it to the cache. */
      i = link_result->cache_next;
      link_result->cache_next = (i + 1) % GL20_LINK_RESULT_CACHE_SIZE;
      if (link_result->cache_used < GL20_LINK_RESULT_CACHE_SIZE) link_result->cache_used++;

      link_result->cache[i].key = *key;
      link_result->cache[i].used = true;
      
      glxx_sort_attributes(attrib, link_result->cattribs_live, link_result->vattribs_live, mergeable_attribs, 
            link_result->cache[i].data.cattribs_order, link_result->cache[i].data.vattribs_order);
      
      if (!create_shader(link_result, i))
      {
         link_result->cache[i].used = false;
         return false;
      }
   }

   for (j = 0; j < GLXX_CONFIG_MAX_VERTEX_ATTRIBS*2; j++)
   {
      cattribs_order_out[j] = link_result->cache[i].data.cattribs_order[j];      
      vattribs_order_out[j] = link_result->cache[i].data.vattribs_order[j];
   }

   shader_out->flags =
      !link_result->cache[i].data.threaded |
      link_result->cache[i].data.has_point_size<<1 |
      1<<2;
   shader_out->num_varyings = link_result->cache[i].data.num_varyings;

   /* SW-5891 hardware can only do 65536 vertices at a time */
   /* store copy of handles in here, so can properly copy shader record */
   shader_out->fshader = (uint32_t)link_result->cache[i].data.mh_fcode;
   shader_out->vshader = (uint32_t)link_result->cache[i].data.mh_vcode;
   /* */

   glxx_big_mem_insert(&shader_out->fshader, link_result->cache[i].data.mh_fcode, 0);
   glxx_big_mem_insert(&shader_out->vshader, link_result->cache[i].data.mh_vcode, 0);

   /* check big_mem_insert didn't change our handle copies */
   vcos_assert(shader_out->fshader == (uint32_t)link_result->cache[i].data.mh_fcode);
   vcos_assert(shader_out->vshader == (uint32_t)link_result->cache[i].data.mh_vcode);

#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_mem_buffer(link_result->cache[i].data.mh_fcode, L_FRAGMENT_SHADER, RECORD_BUFFER_IS_BOTH, mem_get_align(link_result->cache[i].data.mh_fcode));
   record_map_mem_buffer(link_result->cache[i].data.mh_vcode, L_VERTEX_SHADER, RECORD_BUFFER_IS_BOTH, mem_get_align(link_result->cache[i].data.mh_vcode));
#endif

   *vunifmap_out = link_result->cache[i].data.mh_vuniform_map;
   *funifmap_out = link_result->cache[i].data.mh_funiform_map;

   if (link_result->cache[i].data.use_offline) {
      shader_out->flags |= 1 << 3;        /* Shaded coordinates have a clipping header */

      shader_out->cshader = (uint32_t)MEM_INVALID_HANDLE;
   } else {
      /* store copy of handle in here, so can properly copy shader record */
      shader_out->cshader = (uint32_t)link_result->cache[i].data.mh_ccode;

      glxx_big_mem_insert(&shader_out->cshader, link_result->cache[i].data.mh_ccode, 0);

      /* check big_mem_insert didn't change our handle copies */
      vcos_assert(shader_out->cshader == (uint32_t) link_result->cache[i].data.mh_ccode);
#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_mem_buffer(link_result->cache[i].data.mh_ccode, L_COORD_SHADER, RECORD_BUFFER_IS_BOTH, mem_get_align(link_result->cache[i].data.mh_ccode));
#endif
      *cunifmap_out = link_result->cache[i].data.mh_cuniform_map;
   }
   return true;
}
