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
#include "middleware/khronos/gl11/2708/gl11_shader_4.h"
//#include "v3d/verification/tools/2760sim/simpenrose.h"
#include "middleware/khronos/glxx/2708/glxx_shader_4.h"

/*
    VP = vec3(glLightSource[i].position);
    VP = normalize(VP);
    attenuation = 1.0;
*/
static GLXX_VEC3_T *lighting_calc_vp_nonlocal(Dataflow **attenuation, int i)
{
   GLXX_VEC3_T *VP;

   VP = glxx_u3(GLXX_STATE_OFFSET(lights[i].position));
   VP = glxx_normalize3(VP);
   *attenuation = glxx_cfloat(1.0f);

   return VP;
}

/*
    VP = gl_LightSource[i].position3 - ecPosition3;
    d = length(VP);

    VP = normalize(VP);
    attenuation = 1.0 / (gl_LightSource[i].constantAttenuation +
                         gl_LightSource[i].linearAttenuation * d +
                         gl_LightSource[i].quadraticAttenuation * d * d);
*/

static GLXX_VEC3_T *lighting_calc_vp_local(Dataflow **attenuation, int i, GLXX_VEC3_T *position)
{
   GLXX_VEC3_T *VP;
   Dataflow *d, *dd, *rd;
   Dataflow *k0, *k1, *k2;

   VP = glxx_sub3(glxx_u3(GLXX_STATE_OFFSET(lights[i].position3)), position);
   dd = glxx_dot3(VP, VP);
   rd = glxx_accu_rsqrt(dd);
   d = glxx_accu_recip(rd);

   k0 = glxx_u(GLXX_STATE_OFFSET(lights[i].attenuation.constant), 0);
   k1 = glxx_u(GLXX_STATE_OFFSET(lights[i].attenuation.linear), 0);
   k2 = glxx_u(GLXX_STATE_OFFSET(lights[i].attenuation.quadratic), 0);

   VP = glxx_scale3(rd, VP);    //== normalize(VP);
   *attenuation = glxx_recip(glxx_sum3(k0, glxx_mul(k1, d), glxx_mul(k2, dd)));

   return VP;
}

 /* 

    spotDot = glxx_dot(-VP, gl_LightSource[i].spotDirection);
    if (spotDot < gl_lightSource[i].spotCosCutoff)
       spotAttenuation = 0.0;
    else
       spotAttenuation = pow(spotDot, gl_LightSource[i].spotExponent);
    attenuation *= spotAttenuation;
*/

static Dataflow *lighting_apply_spot(Dataflow *attenuation, GLXX_VEC3_T *VP, int i)
{
   Dataflow *spotDot;
   Dataflow *spotAttenuation;
   GLXX_VEC3_T *spot_direction;
   Dataflow *spot_exponent, *cos_cutoff;

   spot_direction = glxx_u3(GLXX_STATE_OFFSET(lights[i].spot.direction));
   spot_exponent = glxx_u(GLXX_STATE_OFFSET(lights[i].spot.exponent), 0);
   cos_cutoff = glxx_u(GLXX_STATE_OFFSET(lights[i].cos_cutoff), 0);

   spotDot = glxx_negate(glxx_dot3(VP, spot_direction));

   spotAttenuation = glxx_cond(
      glxx_less(spotDot, cos_cutoff),
      glxx_cfloat(0.0f),
      glxx_accu_pow(spotDot, spot_exponent));
   
   return glxx_mul(attenuation, spotAttenuation);
}





 /* 
    nDotVP = max(0.0, glxx_dot(normal, VP));
    nDotHV = max(0.0, glxx_dot(normal, normalize(VP + eye)));
    if (nDotVP == 0.0)
       pf = 0.0;
    else
       pf = pow(nDotHV, gl_FrontMaterial.shininess);
    ambient += glLightSource[i].ambient;  // * attenuation???? XXX TODO
    diffuse += glLightSource[i].diffuse * nDotVP * attenuation;
    specular += glLightSource[i].specular * pf * attenuation;
 */


static void lighting_accumulate(GLXX_VEC3_T *VP, Dataflow *attenuation, GLXX_VEC3_T *normal, GLXX_VEC3_T **ambient, GLXX_VEC3_T **diffuse, GLXX_VEC3_T **specular, int i)
{
   Dataflow *nDotVP, *nDotHV, *pf, *zero;
   Dataflow *m_shininess;
   GLXX_VEC3_T *l_ambient, *l_diffuse, *l_specular;
   GLXX_VEC3_T *eye;
   GLXX_VEC3_T *new_ambient, *new_diffuse, *new_specular;

   zero = glxx_cfloat(0.0f);
   eye = glxx_vec3(zero, zero, glxx_cfloat(1.0f));

   m_shininess = glxx_u(GLXX_STATE_OFFSET(material.shininess), 0);
   l_ambient = glxx_u3(GLXX_STATE_OFFSET(lights[i].ambient));
   l_diffuse = glxx_u3(GLXX_STATE_OFFSET(lights[i].diffuse));
   l_specular = glxx_u3(GLXX_STATE_OFFSET(lights[i].specular));

   nDotVP = glxx_fmax(zero, glxx_dot3(normal, VP));
   nDotHV = glxx_fmax(zero, glxx_dot3(normal, glxx_normalize3(glxx_add3(VP, eye))));

   pf = glxx_cond(
      glxx_equal(nDotVP, zero),
      zero,
      glxx_accu_pow(nDotHV, m_shininess));

   new_ambient  = glxx_add3(*ambient,  glxx_scale3(attenuation, l_ambient));
   new_diffuse  = glxx_add3(*diffuse,  glxx_scale3(glxx_mul(nDotVP, attenuation), l_diffuse));
   new_specular = glxx_add3(*specular, glxx_scale3(glxx_mul(pf, attenuation), l_specular));

   /* TODO: these fake iodependencies are a big hack (SW-5130) */
   glxx_iodep3(new_ambient, *diffuse); glxx_iodep3(new_ambient, *specular);
   glxx_iodep3(new_diffuse, *ambient); glxx_iodep3(new_diffuse, *specular);
   glxx_iodep3(new_specular,*ambient); glxx_iodep3(new_specular,*diffuse);

   *ambient = new_ambient;
   *diffuse = new_diffuse;
   *specular = new_specular;
}

static GLXX_VEC4_T *lighting_combine(GLXX_VEC3_T *ambient, GLXX_VEC3_T *diffuse, GLXX_VEC3_T *specular, GLXX_VEC4_T *color_material)
{
   GLXX_VEC3_T *combined;
   GLXX_VEC3_T *m_emission, *m_ambient, *m_specular;
   GLXX_VEC4_T *m_diffuse;
   GLXX_VEC4_T *result;

   m_emission = glxx_u3(GLXX_STATE_OFFSET(material.emission));
   m_specular = glxx_u3(GLXX_STATE_OFFSET(material.specular));
   if (color_material)
   {
      m_ambient = glxx_dropw(color_material);
      m_diffuse = color_material;
   }
   else
   {
      m_ambient = glxx_u3(GLXX_STATE_OFFSET(material.ambient));
      m_diffuse = glxx_u4(GLXX_STATE_OFFSET(material.diffuse));
   }

   combined = m_emission;
   combined = glxx_add3(combined, glxx_mul3(ambient, m_ambient));
   combined = glxx_add3(combined, glxx_mul3(diffuse, glxx_dropw(m_diffuse)));
   combined = glxx_add3(combined, glxx_mul3(specular, m_specular));

   result = glxx_vec4(combined->x, combined->y, combined->z, m_diffuse->w);
   return glxx_clamp4(result);
}

static void lighting(GL11_CACHE_LIGHT_ABSTRACT_T *lights, GLXX_VEC3_T *position, GLXX_VEC3_T *normal, GLXX_VEC4_T *color_material, GLboolean two_side, GLXX_VEC4_T **front_out, GLXX_VEC4_T **back_out)
{
   Dataflow *zero;
   GLXX_VEC3_T *ambient, *diffuse, *specular;
   GLXX_VEC3_T *ambientb, *diffuseb, *specularb, *normalb = NULL;//initialised to avoid warnings
   int i;
   bool first_light = true;

   zero = glxx_cfloat(0.0f);
   ambient = glxx_u3(GLXX_STATE_OFFSET(lightmodel.ambient));
   diffuse = specular = glxx_vec3(zero, zero, zero);

   if (two_side)
   {
      ambientb = ambient;
      diffuseb = diffuse;
      specularb = specular;
      normalb = glxx_negate3(normal);
   }

   for (i = 0; i < GL11_CONFIG_MAX_LIGHTS; i++) {
      GL11_CACHE_LIGHT_ABSTRACT_T *light = &lights[i];
      GLXX_VEC3_T *VP;
      Dataflow *attenuation;

      if (light->enabled) {
         if (light->position_w_is_0)
            VP = lighting_calc_vp_nonlocal(&attenuation, i);
         else
            VP = lighting_calc_vp_local(&attenuation, i, position);

         if (!light->spot_cutoff_is_180)
            attenuation = lighting_apply_spot(attenuation, VP, i);

         lighting_accumulate(VP, attenuation, normal, &ambient, &diffuse, &specular, i);

         if (two_side)
         {
            if(!first_light)
            {
               /* TODO: more hacky fake iodependencies (SW-5130) */
               //schedule the calculation of the current front side light after the previous backside light
               glxx_iodep3(ambient, ambientb); 
               glxx_iodep3(diffuse, diffuseb); 
               glxx_iodep3(specular, specularb);
            }
            lighting_accumulate(VP, attenuation, normalb, &ambientb, &diffuseb, &specularb, i);
            /* TODO: more hacky fake iodependencies (SW-5130) */
            //schedule the calculation of the current back side light after the current front side light
            glxx_iodep3(ambientb, ambient); 
            glxx_iodep3(diffuseb, diffuse); 
            glxx_iodep3(specularb, specular);
         }
         first_light = false;
      }
   }

   *front_out = lighting_combine(ambient, diffuse, specular, color_material);
   
   if (two_side)
   {
      *back_out = lighting_combine(ambientb, diffuseb, specularb, color_material);
   }
}


static Dataflow *vertex_calculate_point_sizes(GLXX_VEC3_T *eyespace, Dataflow *size)
{
   Dataflow *dd, *d;
   Dataflow *thing;
   Dataflow *a, *b, *c;
   Dataflow *size_min, *size_max;

   dd = glxx_dot3(eyespace, eyespace);
   d = glxx_recip(glxx_rsqrt(dd));

   a = glxx_u(GLXX_STATE_OFFSET(point_params.distance_attenuation), 0);
   b = glxx_u(GLXX_STATE_OFFSET(point_params.distance_attenuation), 4);
   c = glxx_u(GLXX_STATE_OFFSET(point_params.distance_attenuation), 8);
   size_min = glxx_u(GLXX_STATE_OFFSET(point_params.size_min_clamped), 0);
   size_max = glxx_u(GLXX_STATE_OFFSET(point_params.size_max), 0);

   thing = glxx_sum3(a, glxx_mul(b, d), glxx_mul(c, dd));
   thing = glxx_mul(size, glxx_rsqrt(thing));

   return glxx_fmin(glxx_fmax(thing, size_min), size_max);
}

static Dataflow *fetch_all_attributes(
   GL11_CACHE_KEY_T *v,
   GLXX_VEC4_T **attr_color,
   GLXX_VEC4_T **attr_normal,
   GLXX_VEC4_T **attr_vertex,
   GLXX_VEC4_T **attr_texture_coord,
   GLXX_VEC4_T **attr_point_size,
   uint32_t * attribs_order,
   Dataflow *dep)
{
   int i;
   Dataflow *attrib[4*GLXX_CONFIG_MAX_VERTEX_ATTRIBS];
   uint32_t uniform_index[GLXX_CONFIG_MAX_VERTEX_ATTRIBS];
   Dataflow *result;
   
   //setup uniform indexes
   uniform_index[GL11_IX_COLOR] = GLXX_ATTRIB_OFFSET(GL11_IX_COLOR);   
   uniform_index[GL11_IX_NORMAL] = GLXX_ATTRIB_OFFSET(GL11_IX_NORMAL);   
   uniform_index[GL11_IX_VERTEX] = 0;
   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++)    
      uniform_index[GL11_IX_TEXTURE_COORD+i] = GLXX_ATTRIB_OFFSET(GL11_IX_TEXTURE_COORD+i);
   uniform_index[GL11_IX_POINT_SIZE] = GLXX_ATTRIB_OFFSET(GL11_IX_POINT_SIZE);

   //call generic routine
   result = glxx_fetch_all_attributes(v->common.attribs, uniform_index, attrib, dep, v->vattribs_live, attribs_order);
   
   //extract out 
   *attr_color = glxx_vec4(attrib[4*GL11_IX_COLOR],attrib[4*GL11_IX_COLOR+1],attrib[4*GL11_IX_COLOR+2],attrib[4*GL11_IX_COLOR+3]);
   *attr_normal = glxx_vec4(attrib[4*GL11_IX_NORMAL],attrib[4*GL11_IX_NORMAL+1],attrib[4*GL11_IX_NORMAL+2],attrib[4*GL11_IX_NORMAL+3]);
   *attr_vertex = glxx_vec4(attrib[4*GL11_IX_VERTEX],attrib[4*GL11_IX_VERTEX+1],attrib[4*GL11_IX_VERTEX+2],attrib[4*GL11_IX_VERTEX+3]);
   
   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++)
   {
      attr_texture_coord[i] = glxx_vec4(attrib[4*(GL11_IX_TEXTURE_COORD+i)],attrib[4*(GL11_IX_TEXTURE_COORD+i)+1],
                                       attrib[4*(GL11_IX_TEXTURE_COORD+i)+2],attrib[4*(GL11_IX_TEXTURE_COORD+i)+3]);
   }
   *attr_point_size = glxx_vec4(attrib[4*GL11_IX_POINT_SIZE],attrib[4*GL11_IX_POINT_SIZE+1],attrib[4*GL11_IX_POINT_SIZE+2],attrib[4*GL11_IX_POINT_SIZE+3]);
   
   return result;
}

static GLXX_VERTEX_CARD_T *vshader(GL11_CACHE_KEY_T *v, uint32_t * attribs_order)
{
   int i;

   GLXX_VERTEX_CARD_T *result;

   GLXX_VEC4_T *attr_color = 0, *attr_normal = 0, *attr_vertex = 0, *attr_texture_coord[GL11_CONFIG_MAX_TEXTURE_UNITS] = {0,0,0,0}, *attr_point_size = 0;
   GLXX_VEC3_T *eyespace = 0;

   result = (GLXX_VERTEX_CARD_T *)malloc_fast(sizeof(GLXX_VERTEX_CARD_T));
   /* clear the varyings as non NULL = activated */
   for (i = 0; i < GLXX_NUM_VARYINGS; i++)
      result->varying[i] = NULL;

   result->force_vpm_read = fetch_all_attributes(v,&attr_color,&attr_normal,&attr_vertex,attr_texture_coord,&attr_point_size,attribs_order,NULL);

   if(!v->drawtex)
   {
      result->vertex = glxx_transform4x4(
         glxx_u4t(GLXX_STATE_OFFSET(projection_modelview[0])),
         glxx_u4t(GLXX_STATE_OFFSET(projection_modelview[1])),
         glxx_u4t(GLXX_STATE_OFFSET(projection_modelview[2])),
         glxx_u4t(GLXX_STATE_OFFSET(projection_modelview[3])),
         attr_vertex);
   }
   else   
      result->vertex = attr_vertex;
      
   if (v->fog_mode || v->common.primitive_type == 0 || v->lighting)
   {  
      if(!v->drawtex) {
         eyespace = glxx_dehomogenize(glxx_transform4x4(
            glxx_u4t(GLXX_STATE_OFFSET(current_modelview[0])),
            glxx_u4t(GLXX_STATE_OFFSET(current_modelview[1])),
            glxx_u4t(GLXX_STATE_OFFSET(current_modelview[2])),
            glxx_u4t(GLXX_STATE_OFFSET(current_modelview[3])),
            attr_vertex));
      }
      else
         eyespace = glxx_vec3(glxx_cfloat(0.0f),glxx_cfloat(0.0f),glxx_cfloat(0.0f));
   }
   
   if (v->fog_mode)
      result->varying[GLXX_VARYING_EYESPACE] = glxx_nullw(eyespace);

   if (v->common.primitive_type == 0)
   {
      result->point_size = vertex_calculate_point_sizes(eyespace, attr_point_size->x);
   
      if (v->point_smooth)
         result->varying[GLXX_VARYING_POINT_SIZE] = glxx_vec4(result->point_size, NULL, NULL, NULL);
   }
   else
      result->point_size = 0;

   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++)
   {
      if (v->texunits[i].props.active)
      {
         if(!v->drawtex)
         {
            result->varying[GLXX_VARYING_TEX_COORD(i)] = glxx_transform4x4(
               glxx_u4t(GLXX_STATE_OFFSET(texunits[i].current_matrix[0])),
               glxx_u4t(GLXX_STATE_OFFSET(texunits[i].current_matrix[1])),
               glxx_u4t(GLXX_STATE_OFFSET(texunits[i].current_matrix[2])),
               glxx_u4t(GLXX_STATE_OFFSET(texunits[i].current_matrix[3])),
               attr_texture_coord[i]);
         }
         else
            result->varying[GLXX_VARYING_TEX_COORD(i)] = attr_texture_coord[i];
      }
   }

   if (v->lighting && !v->drawtex)
   {
      GLXX_VEC4_T *color_material;
      GLXX_VEC3_T *normal;

      /*
         transform normals into eye space and normalize
      */

      normal = glxx_transform3x3(
         glxx_u3(GLXX_STATE_OFFSET(modelview_inv[0])),
         glxx_u3(GLXX_STATE_OFFSET(modelview_inv[4])),
         glxx_u3(GLXX_STATE_OFFSET(modelview_inv[8])),
         glxx_dropw(attr_normal));

      if (v->rescale_normal || v->normalize)
      {
         normal = glxx_normalize3(normal);
      }

      /*
         run lighting calculation
      */

      if(v->color_material)
         color_material = attr_color;
      else
         color_material = NULL;

      lighting(v->lights, eyespace, normal, color_material, v->two_side, &result->varying[GLXX_VARYING_COLOR], &result->varying[GLXX_VARYING_BACK_COLOR]);
   }
   else
   {
      /* No lighting */
      result->varying[GLXX_VARYING_COLOR] = glxx_clamp4(attr_color);
   }
   return result;
}

static GLXX_VEC4_T *texture_arg(GLenum src, GLenum op, GLXX_VEC4_T *texture, GLXX_VEC4_T *constant, GLXX_VEC4_T *primary_color, GLXX_VEC4_T *previous)
{
   GLXX_VEC4_T *color;

   switch (src)
   {
   case GL_TEXTURE:
      color = texture;
      break;
   case GL_CONSTANT:
      color = constant;
      break;
   case GL_PRIMARY_COLOR:
      color = primary_color;
      break;
   case GL_PREVIOUS:
      color = previous;
      break;
   case 0:
      //color = vec4(glxx_cfloat(0.0f),glxx_cfloat(0.0f),glxx_cfloat(0.0f),glxx_cfloat(0.0f));
      //break;
      return glxx_vec4(NULL,NULL,NULL,NULL);      /* Hopefully isn't being used. TODO: is this safe? */
   default:
      UNREACHABLE();
      return 0;
   }

   switch (op)
   {
   case GL_SRC_COLOR:
      return color;
   case GL_ONE_MINUS_SRC_COLOR:
      return glxx_sub4(glxx_crep4(1.0f), color);
   case GL_SRC_ALPHA:
      vcos_assert(color->w != NULL);
      return glxx_rep4(color->w);
   case GL_ONE_MINUS_SRC_ALPHA:
      return glxx_rep4(glxx_sub(glxx_cfloat(1.0f), color->w));
   default:
      UNREACHABLE();
      return 0;
   }
}

static GLXX_VEC4_T *texture_combine(GLenum combine, GLXX_VEC4_T *arg0, GLXX_VEC4_T *arg1, GLXX_VEC4_T *arg2)
{
   switch (combine)
   {
   case GL_REPLACE:
      return arg0;
   case GL_MODULATE:
      return glxx_mul4(arg0, arg1);
   case GL_ADD:
      return glxx_add4(arg0, arg1);
   case GL_ADD_SIGNED:
      return glxx_sub4(glxx_add4(arg0, arg1), glxx_crep4(0.5f));
   case GL_INTERPOLATE:
      return glxx_interp4(arg1, arg0, arg2);
   case GL_SUBTRACT:
      return glxx_sub4(arg0, arg1);
   case GL_DOT3_RGB:
   case GL_DOT3_RGBA:
   {
      GLXX_VEC3_T *tmp0, *tmp1;
      tmp0 = glxx_sub3(glxx_dropw(arg0), glxx_crep3(0.5f));
      tmp1 = glxx_sub3(glxx_dropw(arg1), glxx_crep3(0.5f));
      return glxx_rep4(glxx_mul(glxx_dot3(tmp0, tmp1),glxx_cfloat(4.0f)));
   }
   default:
      UNREACHABLE();
      return 0;
   }
}

static GLXX_VEC4_T *texture_clamp(GLXX_VEC4_T *color, bool need_clamp_alpha_below, bool need_clamp_alpha_above, bool need_clamp_rgb_below, bool need_clamp_rgb_above)
{
   Dataflow *r, *g, *b, *a;

   r = color->x;
   g = color->y;
   b = color->z;
   a = color->w;

   if (need_clamp_alpha_below)
      a = glxx_fmax(a, glxx_cfloat(0.0f));
   if (need_clamp_alpha_above)
      a = glxx_fmin(a, glxx_cfloat(1.0f));

   if (need_clamp_rgb_below)
   {
      r = glxx_fmax(r, glxx_cfloat(0.0f));
      g = glxx_fmax(g, glxx_cfloat(0.0f));
      b = glxx_fmax(b, glxx_cfloat(0.0f));
   }
   if (need_clamp_rgb_above)
   {
      r = glxx_fmin(r, glxx_cfloat(1.0f));
      g = glxx_fmin(g, glxx_cfloat(1.0f));
      b = glxx_fmin(b, glxx_cfloat(1.0f));
   }
   return glxx_vec4(r, g, b, a);
}

static GLXX_VEC4_T *texture_function(GLenum function, bool has_color, bool has_alpha, GLXX_VEC4_T *texture, GLXX_VEC4_T *constant, GLXX_VEC4_T *primary_color, GLXX_VEC4_T *previous)
{
   GLXX_VEC4_T *color, *alpha;
   UNUSED(primary_color);
   switch (function)
   {
   case GL_REPLACE:
      color = has_color ? texture : previous;
      alpha = has_alpha ? texture : previous;
      break;
   case GL_MODULATE:
      color = has_color ? glxx_mul4(previous, texture) : previous;
      alpha = has_alpha ? glxx_mul4(previous, texture) : previous;
      break;
   case GL_DECAL:
      color = has_alpha ? glxx_blend4(previous, texture, texture->w) : texture;
      alpha = previous;
      break;
   case GL_BLEND:
      color = has_color ? glxx_interp4(previous, constant, texture) : previous;
      alpha = has_alpha ? glxx_mul4(previous, texture) : previous;
      break;
   case GL_ADD:
      color = has_color ? glxx_add4(previous, texture) : previous;
      alpha = has_alpha ? glxx_mul4(previous, texture) : previous;
      break;
   default:
      UNREACHABLE();
      return 0;
   }

   return glxx_vec4(color->x, color->y, color->z, alpha->w);
}

static bool need_clamp_below(GLenum combine)
{
   switch (combine) {
   case GL_REPLACE:
   case GL_MODULATE:
   case GL_ADD:
   case GL_INTERPOLATE:
      return GL_FALSE;
   case GL_ADD_SIGNED:
   case GL_SUBTRACT:
   case GL_DOT3_RGB:
   case GL_DOT3_RGBA:
      return GL_TRUE;
   default:
      UNREACHABLE();
      return 0;
   }
}

static bool need_clamp_above(GLenum combine)
{
   switch (combine) {
   case GL_REPLACE:
   case GL_MODULATE:
   case GL_INTERPOLATE:
   case GL_SUBTRACT:
      return GL_FALSE;
   case GL_ADD:
   case GL_ADD_SIGNED:
   case GL_DOT3_RGB:
   case GL_DOT3_RGBA:
      return GL_TRUE;
   default:
      UNREACHABLE();
      return 0;
   }
}

static GLXX_VEC4_T *texture_lookup_simple(uint32_t i, GLXX_VEC4_T *coord, bool *texture_rb_swap, bool point_coord, bool rso_format)
{
   Dataflow *gadget_set_t, *gadget_set_s, *gadget_set_b, *gadget_get_rgba;
   Dataflow *gadget_get_r, *gadget_get_g, *gadget_get_b, *gadget_get_a;
   Dataflow *sampler;
   bool rb_swap = false;

   sampler = glsl_dataflow_construct_const_sampler(0, PRIM_SAMPLERCUBE);
   sampler->u.const_sampler.location = i;

/*   gadget_set_b = glsl_dataflow_construct_texture_lookup_set(0,
      DATAFLOW_TEX_SET_BIAS,
      sampler,
      glxx_cfloat(0.03f),
      NULL,
      NULL,
      NULL);*/
   gadget_set_b = NULL;

#ifdef BCG_FB_LAYOUT
   if ((point_coord) && (rso_format))
   {
      Dataflow * omy = glxx_sub(glxx_cfloat(1.0f), coord->y);
      gadget_set_t = glsl_dataflow_construct_texture_lookup_set(0,
         DATAFLOW_TEX_SET_COORD_T,
         sampler,
         omy,
         NULL,
         NULL,
         NULL);
   }
   else
#endif
   {
      gadget_set_t = glsl_dataflow_construct_texture_lookup_set(0,
         DATAFLOW_TEX_SET_COORD_T,
         sampler,
         coord->y,
         NULL,
         NULL,
         NULL);
   }

   gadget_set_s = glsl_dataflow_construct_texture_lookup_set(0,
      DATAFLOW_TEX_SET_COORD_S,
      sampler,
      coord->x,
      gadget_set_b,
      gadget_set_t,
      NULL/*gadget_set_r*/);
   //TODO: thread switch
   gadget_get_rgba = glsl_dataflow_construct_texture_lookup_get(0,
      DATAFLOW_TEX_GET_CMP_R,
      sampler,
      gadget_set_s,
      NULL,
      NULL);

   if (texture_rb_swap)
      rb_swap = texture_rb_swap[i];

   gadget_get_r = glsl_dataflow_construct_unpack(0,
      rb_swap ? DATAFLOW_UNPACK_COL_B : DATAFLOW_UNPACK_COL_R,
      gadget_get_rgba);
   gadget_get_g = glsl_dataflow_construct_unpack(0,
      DATAFLOW_UNPACK_COL_G,
      gadget_get_rgba);
   gadget_get_b = glsl_dataflow_construct_unpack(0,
      rb_swap ? DATAFLOW_UNPACK_COL_R : DATAFLOW_UNPACK_COL_B,
      gadget_get_rgba);
   gadget_get_a = glsl_dataflow_construct_unpack(0,
      DATAFLOW_UNPACK_COL_A,
      gadget_get_rgba);

   return glxx_vec4(gadget_get_r, gadget_get_g, gadget_get_b, gadget_get_a);
}

static GLXX_VEC4_T *texture_lookup_complex(uint32_t i, GLXX_VEC4_T *coord, bool *texture_rb_swap, bool point_coord, bool rso_format)
{
   Dataflow *rw = glxx_recip(coord->w);
   
   coord = glxx_vec4(glxx_mul(coord->x, rw), glxx_mul(coord->y, rw), NULL, NULL);
   return texture_lookup_simple(i, coord, texture_rb_swap, point_coord, rso_format);
}

static GLXX_VEC4_T *texturing(GLXX_FRAGMENT_CARD_T *card, GL11_CACHE_TEXUNIT_ABSTRACT_T *texunits, bool points, GLXX_VEC4_T *primary_color, Dataflow *point_x, Dataflow *point_y, bool *texture_rb_swap, bool rso_format)
{
   uint32_t i, j;
   GLXX_VEC4_T *previous;

   previous = primary_color;

   for (i = 0; i < GL11_CONFIG_MAX_TEXTURE_UNITS; i++)
   {
      GL11_CACHE_TEXUNIT_ABSTRACT_T *texunit = &texunits[i];
      GLXX_VEC4_T *constant;

      constant = glxx_u4(GLXX_STATE_OFFSET(texunits[i].color));

      if (texunit->props.active)
      {
         GLXX_VEC4_T *combined;
         GLXX_VEC4_T *texture;
         GLXX_VEC4_T *coord;

         if (points && texunit->coord_replace)
            coord = glxx_vec4(point_x, point_y, glxx_cfloat(0.0f), glxx_cfloat(1.0f));
         else
            coord = card->varying[GLXX_VARYING_TEX_COORD(i)];

         if (texunit->props.complex)
            texture = texture_lookup_complex(i, coord, texture_rb_swap, points && texunit->coord_replace, rso_format);
         else
            texture = texture_lookup_simple(i, coord, texture_rb_swap, points && texunit->coord_replace, rso_format);

         if (texunit->mode == GL_COMBINE)
         {
            GLXX_VEC4_T *arg[3];
            GLXX_VEC4_T *combined_c, *combined_a;
            Dataflow *scale_c, *scale_a;
            bool need_clamp_alpha_below, need_clamp_alpha_above, need_clamp_rgb_below, need_clamp_rgb_above;

            for (j = 0; j < 3; j++)
            {
               GLXX_VEC4_T *arg_c, *arg_a;

               arg_c = texture_arg(texunit->rgb.source[j],   texunit->rgb.operand[j],   texture, constant, primary_color, previous);
               arg_a = texture_arg(texunit->alpha.source[j], texunit->alpha.operand[j], texture, constant, primary_color, previous);

               arg[j] = glxx_vec4(arg_c->x, arg_c->y, arg_c->z, arg_a->w);
            }

            combined_c = texture_combine(texunit->rgb.combine,   arg[0], arg[1], arg[2]);

            if (texunit->rgb.combine == GL_DOT3_RGBA)
               combined_a = combined_c;
            else
               combined_a = texture_combine(texunit->alpha.combine, arg[0], arg[1], arg[2]);

            scale_c = glxx_cfloat(texunit->rgb.scale);
            scale_a = glxx_cfloat(texunit->alpha.scale);

            combined = glxx_vec4(
               glxx_mul(combined_c->x, scale_c),
               glxx_mul(combined_c->y, scale_c),
               glxx_mul(combined_c->z, scale_c),
               glxx_mul(combined_a->w, scale_a));

            need_clamp_alpha_below = need_clamp_below(texunit->alpha.combine) || texunit->rgb.combine == GL_DOT3_RGBA;
            need_clamp_alpha_above = need_clamp_above(texunit->alpha.combine) || texunit->rgb.combine == GL_DOT3_RGBA || texunit->alpha.scale != 1.0f;

            need_clamp_rgb_below = need_clamp_below(texunit->rgb.combine);
            need_clamp_rgb_above = need_clamp_above(texunit->rgb.combine) || texunit->rgb.scale != 1.0f;

            combined = texture_clamp(combined, need_clamp_alpha_below, need_clamp_alpha_above, need_clamp_rgb_below, need_clamp_rgb_above);
         }
         else
         {
            combined = texture_function(texunit->mode, texunit->props.has_color, texunit->props.has_alpha, texture, constant, primary_color, previous);

            if (texunit->props.has_color && texunit->mode == GL_ADD)
               combined = texture_clamp(combined, false, false, false, true);
         }

         previous = combined;
      }
   }

   return previous;
}

static GLXX_VEC4_T *fog(GLXX_FRAGMENT_CARD_T *card, GLenum fog_mode, GLXX_VEC4_T *color)
{
   Dataflow *cc;
   Dataflow *f;
   Dataflow *coeff, *end;
   GLXX_VEC3_T *fog_color;
   GLXX_VEC3_T *eyespace;

   eyespace = glxx_dropw(card->varying[GLXX_VARYING_EYESPACE]);
   /* cc = glxx_dot3(eyespace, eyespace); */
   cc = glxx_mul(eyespace->z, eyespace->z);

   switch (fog_mode)
   {
   case GL_LINEAR:
      end = glxx_u(GLXX_STATE_OFFSET(fog.end), 0);
      coeff = glxx_u(GLXX_STATE_OFFSET(fog.scale), 0);
      f = glxx_fmax(glxx_mul(glxx_sub(end, glxx_sqrt1(cc)), coeff), glxx_cfloat(0.0f));
      break;
   case GL_EXP:
      coeff = glxx_u(GLXX_STATE_OFFSET(fog.coeff_exp), 0);
      f = glxx_exp2(glxx_mul(coeff, glxx_sqrt1(cc)));
      break;
   case GL_EXP2:
      coeff = glxx_u(GLXX_STATE_OFFSET(fog.coeff_exp2), 0);
      f = glxx_exp2(glxx_mul(coeff, cc));
      break;
   default:
      UNREACHABLE();
      return 0;
   }

   fog_color = glxx_u3(GLXX_STATE_OFFSET(fog.color));
   f = glxx_fmin(f, glxx_cfloat(1.0f));

   fog_color = glxx_blend3(fog_color, glxx_dropw(color), f);
   return glxx_vec4(fog_color->x, fog_color->y, fog_color->z, color->w);
}

static GLXX_VEC4_T *apply_antialiased_point(GLXX_FRAGMENT_CARD_T *card, GLXX_VEC4_T *color, Dataflow *point_x, Dataflow *point_y)
{
   Dataflow *x;
   Dataflow *point_size;

   point_size = card->varying[GLXX_VARYING_POINT_SIZE]->x;

   x = glxx_add(glxx_square(glxx_sub(point_x, glxx_cfloat(0.5))), glxx_square(glxx_sub(point_y, glxx_cfloat(0.5))));
   x = glxx_mul(glxx_sub(glxx_cfloat(0.25), x), point_size);
   x = glxx_clamp(x);

   return glxx_vec4(color->x, color->y, color->z, glxx_mul(x, color->w));
}

static GLXX_VEC4_T *apply_antialiased_line(GLXX_VEC4_T *color, Dataflow *line_x)
{
   Dataflow *x, *pixels, *coverage;
   Dataflow *line_width, *half_line, *half_line_floor;

   /* line_x varies across line from 0.0 to 1.0 across line
      calculated at centre of pixel
      so if 0.5 < line_x*line_width || line_x*line_width > (line_width-0.5)
      then we will have partial coverage */

   line_width = glxx_u(GLXX_STATE_OFFSET(line_width), 0);
   half_line = glxx_mul(glxx_cfloat(0.5f),line_width);

   x = line_x;
   /* mirror at 0.5 */
   x = glxx_abs(glxx_sub(glxx_cfloat(0.5f), x));   
   pixels = glxx_mul(line_width,x);/* number of pixels from centre of line */
   /* make this a 1/2 pixel shorter so we get a full ramp to zero */
   half_line_floor = glxx_sub(half_line,glxx_cfloat(1.0f));

   /* fraction of pixel covered */
   coverage = glxx_cond(glxx_less(pixels,half_line_floor),
                        glxx_cfloat(1.0f),
                        glxx_sub(glxx_cfloat(1.0f),glxx_sub(pixels,half_line_floor)));

   return glxx_vec4(color->x, color->y, color->z, glxx_mul(coverage, color->w));

   /* return glxx_vec4(
      glxx_cond(glxx_less(pixels,half_line_floor),glxx_cfloat(1.0f),glxx_cfloat(0.0f)),
      glxx_cond(glxx_less(pixels,half_line_floor),glxx_cfloat(0.0f),glxx_cfloat(1.0f)),
      glxx_cfloat(0.0f),     
      glxx_cfloat(1.0f)      
      ); */
}

static Dataflow *perform_user_clip(int user_clip, bool rso_format)
{
   GLXX_VEC4_T *coord;
   Dataflow *scalar;

#ifdef BCG_FB_LAYOUT
   if (rso_format)
   {
      coord = glxx_vec4(
         glxx_sub(glxx_i_to_f(glxx_fragment_get(DATAFLOW_FRAG_GET_X)), glxx_u(GLXX_STATE_OFFSET(viewport.internal[2]), 0)),
         glxx_sub(glxx_sub(glxx_ustandard_int(BACKEND_UNIFORM_FBHEIGHT), glxx_i_to_f(glxx_fragment_get(DATAFLOW_FRAG_GET_Y))), glxx_u(GLXX_STATE_OFFSET(viewport.internal[3]), 0)),
         glxx_sub(glxx_i_to_f(glxx_fragment_get(DATAFLOW_FRAG_GET_Z)), glxx_u(GLXX_STATE_OFFSET(viewport.internal[11]), 0)),
         glxx_cfloat(1.0f));
   }
   else
#endif /* BCG_FB_LAYOUT */
   {
      coord = glxx_vec4(
         glxx_sub(glxx_i_to_f(glxx_fragment_get(DATAFLOW_FRAG_GET_X)), glxx_u(GLXX_STATE_OFFSET(viewport.internal[2]), 0)),
         glxx_sub(glxx_i_to_f(glxx_fragment_get(DATAFLOW_FRAG_GET_Y)), glxx_u(GLXX_STATE_OFFSET(viewport.internal[3]), 0)),
         glxx_sub(glxx_i_to_f(glxx_fragment_get(DATAFLOW_FRAG_GET_Z)), glxx_u(GLXX_STATE_OFFSET(viewport.internal[11]), 0)),
         glxx_cfloat(1.0f));
   }

   scalar = glxx_mul(glxx_dot4(coord, glxx_u4(GLXX_STATE_OFFSET(projected_clip_plane))), glxx_fragment_get(DATAFLOW_FRAG_GET_W));

   vcos_assert(user_clip == -1 || user_clip == 1);
   if (user_clip > 0)
      return glxx_less(scalar, glxx_cfloat(0.0f));
   else
      return glxx_lequal(scalar, glxx_cfloat(0.0f));
}

static Dataflow *alpha_test(GLenum test, Dataflow *a, Dataflow *b)
{
   switch (test)
   {
   case GL_NEVER:
      return glxx_cbool(false);
   case GL_LESS:
      return glxx_less(a, b);
   case GL_EQUAL:
      return glxx_equal(a, b);
   case GL_LEQUAL:
      return glxx_lequal(a, b);
   case GL_GREATER:
      return glxx_less(b, a);
   case GL_NOTEQUAL:
      return glxx_logicnot(glxx_equal(a, b));
   case GL_GEQUAL:
      return glxx_lequal(b, a);
   case GL_ALWAYS:
      return glxx_cbool(true);
   default:
      UNREACHABLE();
      return NULL;
   }
}

static GLXX_FRAGMENT_CARD_T *fshader(GL11_CACHE_KEY_T *v)
{
   GLXX_VEC4_T *color;
   GLXX_FRAGMENT_CARD_T *card;
   Dataflow *point_x, *point_y, *line_x = NULL, *dep, *discard;
   uint32_t i, j;

   card = (GLXX_FRAGMENT_CARD_T *)malloc_fast(sizeof(GLXX_FRAGMENT_CARD_T));

   dep = NULL;
   if (v->common.primitive_type == 0)
   {
      /* points */
      dep = point_x = glxx_vary(0, dep);
      dep = point_y = glxx_vary(1, dep);
   }
   else
   {
      if (v->common.primitive_type == 1)  /* lines */
      {
         if (v->line_smooth)
            dep = line_x = glxx_vary_non_perspective(0, dep);
         else
            dep = glxx_vary(0, dep);  /*glxx_vary_blah(0, dep);*/
      }
      point_x = glxx_cfloat(0.0f);
      point_y = glxx_cfloat(0.0f);
   }

   for (i = 0; i < GLXX_NUM_VARYINGS; i++)
   {
      Dataflow *vary[4];
      for (j = 0; j < 4; j++)
      {
         vary[j] = glxx_vary(32 + 4 * i + j, dep);
      }
      card->varying[i] = glxx_vec4(vary[0], vary[1], vary[2], vary[3]);
   }

   color = card->varying[GLXX_VARYING_COLOR];
   if (v->two_side)
      color = glxx_cond4(glxx_fragment_get(DATAFLOW_FRAG_GET_FF), color, card->varying[GLXX_VARYING_BACK_COLOR]);

   color = texturing(card, v->texunits, v->common.primitive_type == 0, color, point_x, point_y,
#if GL_EXT_texture_format_BGRA8888
      v->common.texture_rb_swap
#else
      NULL
#endif
      , v->common.rso_format);
   if (v->fog_mode)
      color = fog(card, v->fog_mode, color);
   if (v->point_smooth)
   {
      vcos_assert(v->common.primitive_type == 0);
      color = apply_antialiased_point(card, color, point_x, point_y);
   }
   if (v->line_smooth)
   {
      vcos_assert(v->common.primitive_type == 1);
      color = apply_antialiased_line(color, line_x);
   }

   //TODO: sample coverage
   discard = glxx_cbool(false);
   if (v->alpha_func != GL_ALWAYS)
      discard = glxx_logicor(discard, glxx_logicnot(alpha_test(v->alpha_func, color->w, glxx_u(GLXX_STATE_OFFSET(alpha_func.ref), 0))));
   if (v->user_clip_plane != 0)
      discard = glxx_logicor(discard, perform_user_clip(v->user_clip_plane, v->common.rso_format));

   /* Create fragment shader */

#ifdef WORKAROUND_HW2924
   vcos_assert(!v->common.workaround_hw2924);   /* Separate forward/rev stencil config only possible in OpenGLES 2.0 */
#endif
   card->root = glxx_backend(v->common.blend, color, discard, v->common.use_stencil, v->common.render_alpha, v->common.rgb565, false);

   glxx_iodep(card->root, dep);

#ifdef WORKAROUND_HW2924
   vcos_assert(!v->common.workaround_hw2924);   /* Separate forward/rev stencil config only possible in OpenGLES 2.0 */
#endif

   card->root = glxx_backend(v->common.blend, color, discard, v->common.use_stencil, v->common.render_alpha, v->common.rgb565, false);
   glxx_iodep(card->root, dep);
   return card;
}

bool gl11_hw_emit_shaders(GL11_CACHE_KEY_T *v, GLXX_LINK_RESULT_DATA_T *data, uint32_t *color_varyings)
{
   GLXX_VERTEX_CARD_T *vcard;
   GLXX_FRAGMENT_CARD_T *fcard;
   Dataflow **vertex_vary;
   Dataflow *last_vpm_write;
   uint32_t i;
   uint32_t vary_map[32];
   uint32_t vary_count=0;
   bool result = true;

   vcos_assert(v->two_side == (v->lighting && v->two_side));

   glsl_fastmem_init();
   if (setjmp(g_ErrorHandlerEnv) != 0)
	{
		// We must be jumping back from an error.
      glsl_fastmem_term();
		return false;
	}

   glsl_init_primitive_values();


   /* fragment shader */
   fcard = fshader(v);
   data->threaded = true;
   result &= glxx_schedule(fcard->root, GLSL_BACKEND_TYPE_FRAGMENT, &data->mh_fcode, &data->mh_funiform_map, &data->threaded, vary_map, &vary_count, v->common.fb_rb_swap);
   data->num_varyings = vary_count;

   *color_varyings = 0;
   for (i = 0; i < vary_count; i++)
   {
      if (/*vary_map[i] >= 4*GLXX_VARYING_COLOR &&*/ vary_map[i] <= 4*GLXX_VARYING_COLOR+3)
         *color_varyings |= 1<<i;
      if (vary_map[i] >= 4*GLXX_VARYING_BACK_COLOR && vary_map[i] <= 4*GLXX_VARYING_BACK_COLOR+3)
         *color_varyings |= 1<<i;
   }

   /* coordinate shader */
   vcard = vshader(v,data->vattribs_order);
   last_vpm_write = glxx_vertex_backend(vcard->vertex->x, vcard->vertex->y, vcard->vertex->z, vcard->vertex->w, vcard->point_size, true, false, NULL, NULL, 0, v->common.rso_format);
   glxx_iodep(last_vpm_write, vcard->force_vpm_read);
   result &= glxx_schedule(last_vpm_write, GLSL_BACKEND_TYPE_VERTEX_OR_COORD, &data->mh_ccode, &data->mh_cuniform_map, NULL, NULL, NULL, v->common.fb_rb_swap);

   /* vertex shader */

   vcard = vshader(v,data->vattribs_order);
   vertex_vary = (Dataflow **)malloc_fast(4 * GLXX_NUM_VARYINGS * sizeof(Dataflow *));
   for (i = 0; i < GLXX_NUM_VARYINGS; i++)
   {
      if (vcard->varying[i])
      {
         vertex_vary[4*i+0] = vcard->varying[i]->x;
         vertex_vary[4*i+1] = vcard->varying[i]->y;
         vertex_vary[4*i+2] = vcard->varying[i]->z;
         vertex_vary[4*i+3] = vcard->varying[i]->w;
      }
   }
   last_vpm_write = glxx_vertex_backend(vcard->vertex->x, vcard->vertex->y, vcard->vertex->z, vcard->vertex->w, vcard->point_size, false, true, vertex_vary, vary_map, vary_count, v->common.rso_format);
   glxx_iodep(last_vpm_write, vcard->force_vpm_read);
   result &= glxx_schedule(last_vpm_write, GLSL_BACKEND_TYPE_VERTEX_OR_COORD, &data->mh_vcode, &data->mh_vuniform_map, NULL, NULL, NULL, v->common.fb_rb_swap);

   data->has_point_size = vcard->point_size != NULL;
   data->use_offline = false;

   glsl_fastmem_term();

   return result;
}
