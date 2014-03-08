/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
/*
   FIXMEFIXMEFIXMEFIX! change glFog and glLightModel and GlLight and GLMaterial to reject attempts to set
   vector valued variables with scalar variants


Commentary:

Many functions come in pairs - a fixed point version and a floating point version.
For instance:
   glClearColor_impl_11(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
   glClearColorx_impl_11(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha)

For these, we define an "internal" version of the function, which always deals in
floating point. We convert from fixed point to floating point before calling this
function if necessary, using fixed_to_float (or float_to_fixed for output parameters).


Khronos documentation that applies to many functions:

   First, if a command that requires an enumerated value is passed a symbolic
   constant that is not one of those specified as allowable for that command, the
   error INVALID_ENUM results.   


We assume enums in gl.h to be contiguous in various instances, as follows:
    GL_LIGHTn, GL_CLIP_PLANEn, GL_TEXTUREn.

We assume that GLfloat and float are the same size and representation.  We assume 
that GLint and int are the same size and representation.  We take advantage of these
assumptions by merrily casting from a pointer to one to a pointer to the other.  This 
is _not_ true for GLboolean and bool.  We do, however, assume that false and true are 
represented by 0 and 1 respectively for both GLboolean and bool. We assume that int
and float are the same size.

We assume something about the valid ranges of signed and unsigned integers:
   If
      a is an int
      b is an int
      a >= 0
      b >= 0
   Then (uint32_t)a + (uint32_t)b does not overflow.

We assume that pointers to memory regions owned by the client side of the RPC do not 
overlap with the server state structure.  We note this here so that we can meet 
preconditions of non-overlapping pointers more easily.

   SHADE_MODEL                            is incorrectly flagged as Z+ not Z2 in state tables
   IMPLEMENTATION_COLOR_READ_TYPE_OES     is not listed in the state tables
   IMPLEMENTATION_COLOR_READ_FORMAT_OES         "     "     "     "
   COMPRESSED TEXTURE FORMATS             is incorrectly flagged as 10xZ rather than 10*xZ in state tables

Various state table entries don't specify tuple orders.

The spec says that the env argument to GetTexEnv() must be TEXTURE ENV, but it's pretty
clear that it should be POINT_SPRITE_OES if asking for COORD_REPLACE_OES to agree with TexEnv()

The header files incorrectly call the first param of the texenv() functions target rather than env

It's not clear what to do if a vector-valued state variable is set using a scalar setter function. Generally
we have generated the 4-vector (x, x, x, 1) to allow opaque greyscale colors to be set using the scalar 
version.

It's not clear what to do if glBufferData specifies a negative size
It's not clear what to do if a null pointer is passed in to glBufferSubData.

The line "s is masked to the number of bitplanes in the stencil buffer." doesn't make all that much sense,
since we can later change the stencil buffer without changing the stencil clear value. Also, what happens
if there is no stencil buffer?

The "current" GLES 1.1 context is the one returned by GL11_GET_SERVER_STATE(). Certain invariants
of the server-side state are valid only for the current context.

Search for "deviation" to find possible deviations.


Boilerplate that we use

   If no current error, the following conditions cause error to assume the specified value

      GL_INVALID_VALUE              size < 0
      GL_INVALID_ENUM               usage invalid
      GL_INVALID_ENUM               target invalid
      GL_INVALID_OPERATION          no buffer bound to target
      GL_OUT_OF_MEMORY              failed to allocate buffer

   if more than one condition holds, the first error is generated.


   However it is perfectly legal to bind a buffer as the backing for the vertex color array,
   delete the buffer FROM ANOTHER EGL CONTEXT SHARING WITH THIS ONE, and then expect to
   continue using the buffer.

*/

#define PI     3.14159265f


/*
   int get_texenv_integer_internal(GLenum env, GLenum pname, int *params)

   Returns an integer texture unit state variable.  A utility 
   function shared by GetTexEnviv() GetTexEnvfv() GetTexEnvxv()

   Implementation notes:

   stores the value(s) of the given state variable in the memory given by the pointer "params", and returns
   number of such values.  An UNREACHABLE() macro guards against preconditon on pname being unmet by the caller.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   pname is one of the native integer texture unit state variables listed below
   params points to sufficient storage for the pname (i.e. 1 value)

   Postconditions:

   -

   Native Integer Texture Unit State Variables

   COORD REPLACE OES GetTexEnviv (retrieved with GL_POINT_SPRITE_OES target)
   TEXTURE ENV MODE GetTexEnviv 
   COMBINE RGB GetTexEnviv 
   COMBINE ALPHA GetTexEnviv 
   SRC0 RGB GetTexEnviv 
   SRC1 RGB GetTexEnviv 
   SRC2 RGB GetTexEnviv 
   SRC0 ALPHA GetTexEnviv 
   SRC1 ALPHA GetTexEnviv 
   SRC2 ALPHA GetTexEnviv 
   OPERAND0 RGB GetTexEnviv 
   OPERAND1 RGB GetTexEnviv 
   OPERAND2 RGB GetTexEnviv 
   OPERAND0 ALPHA GetTexEnviv 
   OPERAND1 ALPHA GetTexEnviv 
   OPERAND2 ALPHA GetTexEnviv
*/

static int get_texenv_integer_internal(GLenum env, GLenum pname, int *params)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   GL11_TEXUNIT_T *texunit = &state->texunits[state->active_texture - GL_TEXTURE0];
   GL11_CACHE_TEXUNIT_ABSTRACT_T *texabs = &state->shader.texunits[state->active_texture - GL_TEXTURE0];

   int result = 0;

   switch (env) {
   case GL_POINT_SPRITE_OES:
      switch (pname) {
      case GL_COORD_REPLACE_OES:
         params[0] = texabs->coord_replace;
         result = 1;
         break;
      default:
         UNREACHABLE();
         break;
      }
      break;
   case GL_TEXTURE_ENV:
      switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         params[0] = texabs->mode;
         result = 1;
         break;
      case GL_COMBINE_RGB:
         params[0] = texunit->rgb.combine;
         result = 1;
         break;
      case GL_COMBINE_ALPHA:
         params[0] = texunit->alpha.combine;
         result = 1;
         break;
      case GL_SRC0_RGB:
      case GL_SRC1_RGB:
      case GL_SRC2_RGB:
         params[0] = texunit->rgb.source[pname - GL_SRC0_RGB];
         result = 1;
         break;
      case GL_SRC0_ALPHA:
      case GL_SRC1_ALPHA:
      case GL_SRC2_ALPHA:
         params[0] = texunit->alpha.source[pname - GL_SRC0_ALPHA];
         result = 1;
         break;
      case GL_OPERAND0_RGB:
      case GL_OPERAND1_RGB:
      case GL_OPERAND2_RGB:
         params[0] = texunit->rgb.operand[pname - GL_OPERAND0_RGB];
         result = 1;
         break;
      case GL_OPERAND0_ALPHA:
      case GL_OPERAND1_ALPHA:
      case GL_OPERAND2_ALPHA:
         params[0] = texunit->alpha.operand[pname - GL_OPERAND0_ALPHA];
         result = 1;
         break;
      default:
         UNREACHABLE();
         break;
      }
      break;
   default:
      UNREACHABLE();
      break;
   }

   GL11_UNLOCK_SERVER_STATE();

   return result;
}

/*
   int get_texenv_float_internal(GLenum env, GLenum pname, float *params)

   Returns a floating-point texture unit state variable.  A utility 
   function shared by GetTexEnviv() GetTexEnvfv() GetTexEnvxv()

   Implementation notes:

   stores the value(s) of the given state variable in the memory given by the pointer "params", and returns
   number of such values.  An UNREACHABLE() macro guards against preconditon on pname being unmet by the caller.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   pname is one of the native floating-point texture unit state variables listed below
   params points to sufficient storage for the pname (i.e. 1 or 4 values)

   Postconditions:

   -

   Native Floating-Point Texture Unit State Variables

   TEXTURE ENV COLOR GetTexEnvfv (4) .
   RGB SCALE GetTexEnvfv .
   ALPHA SCALE GetTexEnvfv .
*/

static int get_texenv_float_internal(GLenum env, GLenum pname, float *params)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   int t = state->active_texture - GL_TEXTURE0;
   int result = 0;

   switch (env) {
   case GL_TEXTURE_ENV:
      switch (pname) {
      case GL_TEXTURE_ENV_COLOR:
         {
            int i;
            for (i = 0; i < 4; i++)
               params[i] = state->texunits[t].color[i];
            result = 4;
         }
         break;
      case GL_RGB_SCALE:
         params[0] = state->texunits[t].rgb.scale;
         result = 1;
         break;
      case GL_ALPHA_SCALE:
         params[0] = state->texunits[t].alpha.scale;
         result = 1;
         break;
      default:
         UNREACHABLE();
         break;
      }
      break;
   default:
      UNREACHABLE();
      break;
   }

   GL11_UNLOCK_SERVER_STATE();

   return result;
}



/*
   get_light(GLXX_SERVER_STATE_T *state, GLenum l)

   Return the specified light structure, provided
   
      0 <= l - GL_LIGHT0 < GL11_CONFIG_MAX_LIGHTS (8)

   or NULL otherwise

   Implementation notes:

   -

   Preconditions:

   state is a valid pointer

   Postconditions:

   if not NULL, return value is a pointer, valid for as long as state is valid
*/

static GL11_LIGHT_T *get_light(GLXX_SERVER_STATE_T *state, GLenum l)
{
   if (l >= GL_LIGHT0 && l < GL_LIGHT0 + GL11_CONFIG_MAX_LIGHTS)
      return &state->lights[l - GL_LIGHT0];
   else {
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      return NULL;
   }
}

/*
   get_stack(GLXX_SERVER_STATE_T *state)

   Return the active matrix stack according to matrix_mode

   Implementation notes:

   -

   Preconditions:

   state is a valid pointer

   Postconditions:

   return value is a pointer, valid for as long as state is valid
   return value obeys invariant for GL11_MATRIX_STACK_T
*/

static GL11_MATRIX_STACK_T *get_stack(GLXX_SERVER_STATE_T *state)
{
   switch (state->matrix_mode) {
   case GL_MODELVIEW:
      return &state->modelview;
   case GL_PROJECTION:
      return &state->projection;
   case GL_TEXTURE:
      return &state->texunits[state->active_texture - GL_TEXTURE0].stack;
   default:
      UNREACHABLE();
      return NULL;
   }
}

/*
   get_matrix(GLXX_SERVER_STATE_T *state)

   Return the matrix on the top of the currently active stack
   as an array of 16 floats in column-major order

   Implementation notes:

   -

   Preconditions:

   state is a valid pointer

   Postconditions:

   return value is a pointer to 16 elements, valid for as long as state is valid
*/

static float *get_matrix(GLXX_SERVER_STATE_T *state)
{
   GL11_MATRIX_STACK_T *stack = get_stack(state);

   return stack->body[stack->pos];
}



/*
   load_matrix_internal(const float *m)

   copies its argument into the matrix at the top of the currently selected stack

   Implementation notes:

   -

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   m is a valid pointer to 16 floats not overlapping the GL state

   Postconditions:
*/

static void load_matrix_internal(const float *m)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   float *c = get_matrix(state);

   gl11_matrix_load(c, m);

   GL11_UNLOCK_SERVER_STATE();
}

/*
   mult_matrix_internal(const float *m)

   Multiply the matrix at the top of the active stack on the right by the
   supplied matrix (supplied as an array of 16 floats in column-major order)

   Implementation notes:

   -

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   m is a valid pointer to 16 floats

   Postconditions:

   -
*/

static void mult_matrix_internal(const float *m)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   GLfloat *c = get_matrix(state);

   gl11_matrix_mult(c, c, m);

   GL11_UNLOCK_SERVER_STATE();
}

/*
   get_plane(GLXX_SERVER_STATE_T *state, GLenum p)

   Return the specified clip plane from out of the server state.

   Implementation notes:

   -

   Preconditions:

   state is a valid pointer

   Postconditions:

   result is either null or a valid pointer to 4 floats (valid while state is valid)
   If p is an invalid clip plane and no current error, error becomes INVALID_ENUM.
*/

static float *get_plane(GLXX_SERVER_STATE_T *state, GLenum p)
{
   if (p >= GL_CLIP_PLANE0 && p < GL_CLIP_PLANE0 + GL11_CONFIG_MAX_PLANES)
      return state->planes[p - GL_CLIP_PLANE0];
   else {
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      return NULL;
   }
}





/*
   glAlphaFunc_impl_11(GLenum func, GLclampf ref)
   glAlphaFuncx_impl_11(GLenum func, GLclampx ref)

   Khronos documentation:

   func is a symbolic constant indicating the alpha test function; ref is a reference
   value. 
   
   ref is clamped to lie in [0, 1], and then converted to a fixed-point value according
   to the rules given for an A component in section 2.12.8. 

   The possible constants specifying the test function are NEVER, ALWAYS, LESS,
   LEQUAL, EQUAL, GEQUAL, GREATER, or NOTEQUAL, meaning pass the fragment
   never, always, if the fragment’s alpha value is less than, less than or equal to, equal
   to, greater than or equal to, greater than, or not equal to the reference value, respectively.

   Implementation notes:

   Possible deviation from spec in that we keep the value as floating point and compare with
   the floating point fragment alpha at fragment shading time.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   If supplied func is not a valid alpha function and no current error, error becomes INVALID_ENUM

   Invariants preserved:

   state.alpha_func.func and state.alpha_func.ref are valid
*/

static bool is_alpha_func(GLenum func)
{
   return func == GL_NEVER ||
          func == GL_ALWAYS ||
          func == GL_LESS ||
          func == GL_LEQUAL ||
          func == GL_EQUAL ||
          func == GL_GREATER ||
          func == GL_GEQUAL ||
          func == GL_NOTEQUAL;
}

static void alpha_func_internal(GLenum func, float ref)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   if (is_alpha_func(func)) {
      state->changed_misc = true;
      state->alpha_func.func = func;
      state->alpha_func.ref = clampf(ref, 0.0f, 1.0f);
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GL11_UNLOCK_SERVER_STATE();
}

void glAlphaFunc_impl_11 (GLenum func, GLclampf ref)
{
   alpha_func_internal(func, ref);
}

void glAlphaFuncx_impl_11 (GLenum func, GLclampx ref)
{
   alpha_func_internal(func, fixed_to_float(ref));
}

//see documentation in glxx/glxx_server.c
void glClearColorx_impl_11 (GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha)
{
   glxx_clear_color_internal(fixed_to_float(red),
                        fixed_to_float(green),
                        fixed_to_float(blue),
                        fixed_to_float(alpha));
}

//see documentation in glxx/glxx_server.c
void glClearDepthx_impl_11 (GLclampx depth)
{
   glxx_clear_depth_internal(fixed_to_float(depth));
}


/*
   glClipPlanef_impl_11 (GLenum plane, const GLfloat *equation)
   glClipPlanex_impl_11 (GLenum plane, const GLfixed *equation)

   Khronos documentation:

   The value of the first argument, p, is a symbolic constant, CLIP PLANEi, where i
   is an integer between 0 and n - 1, indicating one of n client-defined clip planes.
   eqn is an array of four values. These are the coefficients of a plane equation in
   object coordinates: p1, p2, p3, and p4 (in that order). The inverse of the current
   model-view matrix is applied to these coefficients, at the time they are specified,
   yielding

      ( p'1 p'2 p'3 p'4 ) = ( p1 p2 p3 p4 )M^-1

   (where M is the current model-view matrix; the resulting plane equation is undefined
   if M is singular and may be inaccurate if M is poorly-conditioned) to obtain
   the plane equation coefficients in eye coordinates.

   Implementation notes:

   We retrieve the top of the modelview matrix stack invert it, perform the multiplication
   and clean the result before storing it back into the state. Designed to handle
   multiple clipping planes, but in the BCM2727 instantiation GL11_CONFIG_MAX_PLANES is 1.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   equation is a valid pointer to four 32-bit elements

   Postconditions:   

   If supplied plane is not a valid clip plane and no current error, error becomes INVALID_ENUM

   Invariants preserved:

   -
*/

static void clip_plane_internal(GLenum p, const float *equation)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   float *plane = get_plane(state, p);

   if (plane) {
      float inv[16];

      vcos_assert(state->modelview.pos >= 0 && state->modelview.pos < GL11_CONFIG_MAX_STACK_DEPTH);

      state->changed_misc = true;

      gl11_matrix_invert_4x4(inv, state->modelview.body[state->modelview.pos]);
      gl11_matrix_mult_row(plane, equation, inv);
   }

   GL11_UNLOCK_SERVER_STATE();
}

void glClipPlanef_impl_11 (GLenum plane, const GLfloat *equation)
{
   clip_plane_internal(plane, equation);
}

void glClipPlanex_impl_11 (GLenum plane, const GLfixed *equation)
{
   int i;
   float temp[4];

   for (i = 0; i < 4; i++)
      temp[i] = fixed_to_float(equation[i]);

   clip_plane_internal(plane, temp);
}

/*
   glFogf_impl_11 (GLenum pname, GLfloat param)
   glFogfv_impl_11 (GLenum pname, const GLfloat *params)
   glFogx_impl_11 (GLenum pname, GLfixed param)
   glFogxv_impl_11 (GLenum pname, const GLfixed *params)

   Khronos documentation:

   If pname is FOG MODE, then param must be, or params must point to an integer
   that is one of the symbolic constants EXP, EXP2, or LINEAR, in which case
   equation 3.22, 3.23, or 3.24, respectively, is selected for the fog calculation (if,
   when 3.24 is selected, e = s, results are undefined). If pname is FOG DENSITY,
   FOG START, or FOG END, then param is or params points to a value that is d, s, or
   e, respectively. If d is specified less than zero, the error INVALID VALUE results.

   The R, G, B, and A values of Cf are specified by calling Fog with pname equal to FOG COLOR;

   Implementation notes:

   Possible deviation from spec: in the following case:
      glFogf(GL_FOG_COLOR, blah);
   Perhaps we should give an error. At the moment we interpret the color vector as (blah,blah,blah,1).

   We make the judgment call that people wish to be able to write

   glFogf(GL_FOG_MODE, GL_EXP)
   glFogx(GL_FOG_MODE, GL_EXP)

   which requires us _not_ to perform fixed-point scaling if pname is GL_FOG_MODE. This is
   supported by examination of the conformance test.
   
   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   For vector variants, params be a valid pointer to 4 elements

   Postconditions:   

   If pname is not a valid fog parameter and no current error, error becomes GL_INVALID_ENUM

   Invariants preserved:

   state.fog.mode in {EXP, EXP2, LINEAR}
   0.0 <= state.fog.color[i] <= 1.0
   state.fog.density >= 0.0
   state.fog.scale is consistent with start and end
   state.fog.coeff_exp and state.fog.coeff_exp2 are consistent with density
*/

static bool is_fog_mode(GLenum mode)
{
   return mode == GL_EXP ||
          mode == GL_EXP2 ||
          mode == GL_LINEAR;
}

static void fogv_internal(GLenum pname, const GLfloat *params)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   switch (pname) {
   case GL_FOG_MODE:
   {
      GLenum m = (GLenum)params[0];

      if (is_fog_mode(m))
      {
         state->changed_misc = true;
         state->fog.mode = m;
      }
      else
         glxx_server_state_set_error(state, GL_INVALID_ENUM);

      break;
   }
   case GL_FOG_DENSITY:
   {
      GLfloat d = params[0];

      if (d >= 0.0f) {
         state->fog.density = d;

         state->fog.coeff_exp = -d * LOG2E;
         state->fog.coeff_exp2 = -d * d * LOG2E;
      } else
         glxx_server_state_set_error(state, GL_INVALID_VALUE);

      break;
   }
   case GL_FOG_START:
      state->fog.start = params[0];

      state->fog.scale = 1.0f / (state->fog.end - state->fog.start);
      break;
   case GL_FOG_END:
      state->fog.end = params[0];

      state->fog.scale = 1.0f / (state->fog.end - state->fog.start);
      break;
   case GL_FOG_COLOR:
      {
         int i;
         for (i = 0; i < 4; i++)
            state->fog.color[i] = clampf(params[i], 0.0f, 1.0f);
      }
      break;
   default:
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      break;
   }

   GL11_UNLOCK_SERVER_STATE();
}

//see documentation in glxx/glxx_server.c
void glDepthRangex_impl_11 (GLclampx zNear, GLclampx zFar)
{
   glxx_depth_range_internal(fixed_to_float(zNear),
                        fixed_to_float(zFar));
}

void glFogf_impl_11 (GLenum pname, GLfloat param)
{
   GLfloat params[4];

   params[0] = param;
   params[1] = param;
   params[2] = param;
   params[3] = 1.0f;

   fogv_internal(pname, params);
}

void glFogfv_impl_11 (GLenum pname, const GLfloat *params)
{
   fogv_internal(pname, params);
}

static GLboolean fog_requires_scaling(GLenum pname)
{
   return pname != GL_FOG_MODE;
}

void glFogx_impl_11 (GLenum pname, GLfixed param)
{
   GLfloat floatParam = fog_requires_scaling(pname) ? fixed_to_float(param) : (GLfloat)param;
   GLfloat params[4];

   params[0] = floatParam;
   params[1] = floatParam;
   params[2] = floatParam;
   params[3] = 1.0f;

   fogv_internal(pname, params);
}

void glFogxv_impl_11 (GLenum pname, const GLfixed *params)
{
   GLfloat temp[4];

   if (fog_requires_scaling(pname)) {
      int i;
      for (i = 0; i < 4; i++)
         temp[i] = fixed_to_float(params[i]);
   } else {
      int i;
      for (i = 0; i < 4; i++)
         temp[i] = (GLfloat)params[i];
   }

   fogv_internal(pname, temp);
}

/*
   glFrustumf_impl_11 (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
   glFrustumx_impl_11 (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)

   Khronos documentation:

   There are a variety of other commands that manipulate matrices. Rotate,
   Translate, Scale, Frustum, and Ortho manipulate the current matrix. Each computes
   a matrix and then invokes MultMatrix with this matrix. In the case of

   For Frustum:
   the coordinates (l, b, -n) and (r, t, -n) specify the points on the near clipping
   plane that are mapped to the lower left and upper right corners of the window,
   respectively (assuming that the eye is located at (0, 0, 0)). f gives the distance
   from the eye to the far clipping plane. If either n or f is less than or equal to zero,
   l is equal to r, b is equal to t, or n is equal to f, the error INVALID VALUE results.
   The corresponding matrix is

      2n/(r-l)   0         (r+l)/(r-l)   0
      0          2n/(t-b)  (t+b)/(t-b)   0
      0          0         -(f+n)/(f-n)  -2fn/(f-n)
      0          0         -1            0

   Implementation notes:

   We ignore invalid floats - we do not necessarily generate an INVALID_VALUE error.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   If constraints on l, r, b, t, n, f are not met and no current error, error becomes
   INVALID_VALUE

   Invariants preserved:

   -
*/

static void frustum_internal(float l, float r, float b, float t, float n, float f)
{
   if (n > 0.0f && f > 0.0f && l != r && b != t && n != f) {
      float m[16];

      m[0]  = 2.0f * n / (r - l);
      m[1]  = 0.0f;
      m[2]  = 0.0f;
      m[3]  = 0.0f;

      m[4]  = 0.0f;
      m[5]  = 2.0f * n / (t - b);
      m[6]  = 0.0f;
      m[7]  = 0.0f;

      m[8]  = (r + l) / (r - l);
      m[9]  = (t + b) / (t - b);
      m[10] = -(f + n) / (f - n);
      m[11] = -1.0f;

      m[12] = 0.0f;
      m[13] = 0.0f;
      m[14] = -2.0f * f * n / (f - n);
      m[15] = 0.0f;

      mult_matrix_internal(m);
   } else {
      GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

      glxx_server_state_set_error(state, GL_INVALID_VALUE);

      GL11_UNLOCK_SERVER_STATE();
   }
}

void glFrustumf_impl_11 (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
   frustum_internal(left,
                    right,
                    bottom,
                    top,
                    zNear,
                    zFar);
}

void glFrustumx_impl_11 (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
   frustum_internal(fixed_to_float(left),
                    fixed_to_float(right),
                    fixed_to_float(bottom),
                    fixed_to_float(top),
                    fixed_to_float(zNear),
                    fixed_to_float(zFar));
}

/*
   glGetClipPlanef_impl_11 (GLenum pname, GLfloat eqn[4])
   glGetClipPlanex_impl_11 (GLenum pname, GLfixed eqn[4])

   Khronos documentation:

   Other commands exist to obtain state variables that are identified by a category
   (clip plane, light, material, etc.) as well as a symbolic constant. These are
      void GetClipPlane{xf}( enum plane, T eqn[4] );
      ...
   GetClipPlane always returns four values in eqn; these are the coefficients of the
   plane equation of plane in eye coordinates (these coordinates are those that were
   computed when the plane was specified).

   Implementation notes:

   -

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   eqn is a valid pointer to 4 elements

   Postconditions:   

   If pname is not a valid clipping plane and no current error, error becomes INVALID_ENUM

   Invariants preserved:

   -
*/

static void get_clip_plane_internal(GLenum pname, float eqn[4])
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   float *plane = get_plane(state, pname);

   if (plane) {
      int i;
      for (i = 0; i < 4; i++)
         eqn[i] = plane[i];
   }

   GL11_UNLOCK_SERVER_STATE();
}

void glGetClipPlanef_impl_11 (GLenum pname, GLfloat eqn[4])
{
   get_clip_plane_internal(pname, eqn);
}

void glGetClipPlanex_impl_11 (GLenum pname, GLfixed eqn[4])
{
   float temp[4];

   int i;

   get_clip_plane_internal(pname, temp);

   for (i = 0; i < 4; i++)
      eqn[i] = float_to_fixed(temp[i]);
}



int glGetFixedv_impl_11 (GLenum pname, GLfixed *params)
{
   int i;
   GLfloat temp[16];

   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   int count = glxx_get_float_or_fixed_internal(state, pname, temp);
   GLXX_UNLOCK_SERVER_STATE();

   vcos_assert(count <= 16);

   for (i = 0; i < count; i++)
      params[i] = float_to_fixed(temp[i]);

   return count;
}

/*
   glGetLightfv_impl_11 (GLenum light, GLenum pname, GLfloat *params)
   glGetLightxv_impl_11 (GLenum light, GLenum pname, GLfixed *params)

   Khronos documentation:

   GetLight places information about value (a symbolic constant) for light (also a
   symbolic constant) in data. POSITION or SPOT DIRECTION returns values in eye
   coordinates (again, these are the coordinates that were computed when the position
   or direction was specified).

   AMBIENT GetLightfv (4)
   DIFFUSE GetLightfv (4)
   SPECULAR GetLightfv (4)
   POSITION GetLightfv (4)
   CONSTANT ATTENUATION GetLightfv
   LINEAR ATTENUATION GetLightfv
   QUADRATIC ATTENUATION GetLightfv
   SPOT DIRECTION GetLightfv (3)
   SPOT EXPONENT GetLightfv
   SPOT CUTOFF GetLightfv

   Other commands exist to obtain state variables that are identified by a category
   (clip plane, light, material, etc.) as well as a symbolic constant. These are
      ...
      void GetLight{xf}v( enum light, enum value, T data );
      ...

   Implementation notes:

   -

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   params is a valid pointer to sufficient elements for the state variable specified

   Postconditions:   

   If pname is not a valid light or pname is not a valid light state
   variable, and no current error, error becomes INVALID_ENUM

   Invariants preserved:

   -
*/

static int get_lightv_internal(GLenum l, GLenum pname, float *params)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   GL11_LIGHT_T *light = get_light(state, l);

   int result;

   if (light)
      switch (pname) {
      case GL_AMBIENT:
      {
         int i;
         for (i = 0; i < 4; i++)
            params[i] = light->ambient[i];
         result = 4;
         break;
      }
      case GL_DIFFUSE:
      {
         int i;
         for (i = 0; i < 4; i++)
            params[i] = light->diffuse[i];
         result = 4;
         break;
      }
      case GL_SPECULAR:
      {
         int i;
         for (i = 0; i < 4; i++)
            params[i] = light->specular[i];
         result = 4;
         break;
      }
      case GL_POSITION:
      {
         int i;
         for (i = 0; i < 4; i++)
            params[i] = light->position[i];
         result = 4;
         break;
      }
      case GL_CONSTANT_ATTENUATION:
         params[0] = light->attenuation.constant;
         result = 1;
         break;
      case GL_LINEAR_ATTENUATION:
         params[0] = light->attenuation.linear;
         result = 1;
         break;
      case GL_QUADRATIC_ATTENUATION:
         params[0] = light->attenuation.quadratic;
         result = 1;
         break;
      case GL_SPOT_DIRECTION:
      {
         int i;
         for (i = 0; i < 3; i++)
            params[i] = light->spot.direction[i];
         result = 3;
         break;
      }
      case GL_SPOT_EXPONENT:
         params[0] = light->spot.exponent;
         result = 1;
         break;
      case GL_SPOT_CUTOFF:
         params[0] = light->spot.cutoff;
         result = 1;
         break;
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         result = 0;
         break;
      }
   else
      result = 0;

   GL11_UNLOCK_SERVER_STATE();

   return result;
}

int glGetLightfv_impl_11 (GLenum light, GLenum pname, GLfloat *params)
{
   return get_lightv_internal(light, pname, params);
}

int glGetLightxv_impl_11 (GLenum light, GLenum pname, GLfixed *params)
{
   float temp[4];

   int count = get_lightv_internal(light, pname, temp);
   int i;

   vcos_assert(count <= 4);

   for (i = 0; i < count; i++)
      params[i] = float_to_fixed(temp[i]);

   return count;
}

/*
   glGetMaterialfv_impl_11 (GLenum face, GLenum pname, GLfloat *params)
   glGetMaterialxv_impl_11 (GLenum face, GLenum pname, GLfixed *params)

   Khronos documentation:

   GetMaterial, GetTexEnv, GetTexParameter, and GetBufferParameter are
   similar to GetLight, placing information about value for the target indicated by
   their first argument into data. The face argument to GetMaterial must be either
   FRONT or BACK, indicating the front or back material, respectively.

   AMBIENT GetMaterialfv (4)
   DIFFUSE GetMaterialfv (4)
   SPECULAR GetMaterialfv (4)
   EMISSION GetMaterialfv (4)
   SHININESS GetMaterialfv

   Other commands exist to obtain state variables that are identified by a category
   (clip plane, light, material, etc.) as well as a symbolic constant. These are
      ...
      void GetMaterial{xf}v( enum face, enum value, T data );
      ...

   Implementation notes:

   From the specification of GL_MATERIAL we have 

   "For the Material command, face must be FRONT AND BACK, indicating that the 
   property name of both the front and back material, should be set."

   Under GL ES 1.1 there is no concept of separate front and back materials, so
   we merely check the face parameter for validity but otherwise ignore it.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   params is a valid pointer to sufficient elements for the state variable specified

   Postconditions:   

   If face is not GL_FRONT or GL_BACK or pname is not a valid material state
   variable, and no current error, error becomes INVALID_ENUM

   Invariants preserved:

   -
*/

static GLboolean is_single_face(GLenum face)
{
   return face == GL_FRONT ||
          face == GL_BACK;
}

static int get_materialv_internal(GLenum face, GLenum pname, float *params)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   int result;

   if (is_single_face(face))
      switch (pname) {
      case GL_AMBIENT:
         {
            int i;
            for (i = 0; i < 4; i++)
               params[i] = state->material.ambient[i];
            result = 4;
         }
         break;
      case GL_DIFFUSE:
      {
         int i;
         for (i = 0; i < 4; i++)
            params[i] = state->material.diffuse[i];
         result = 4;
         break;
      }
      case GL_SPECULAR:
      {
         int i;
         for (i = 0; i < 4; i++)
            params[i] = state->material.specular[i];
         result = 4;
         break;
      }
      case GL_EMISSION:
      {
         int i;
         for (i = 0; i < 4; i++)
            params[i] = state->material.emission[i];
         result = 4;
         break;
      }
      case GL_SHININESS:
         params[0] = state->material.shininess;
         result = 1;
         break;
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         result = 0;
         break;
      }
   else {
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      result = 0;
   }

   GL11_UNLOCK_SERVER_STATE();

   return result;
}

int glGetMaterialfv_impl_11 (GLenum face, GLenum pname, GLfloat *params)
{
   return get_materialv_internal(face, pname, params);
}

int glGetMaterialxv_impl_11 (GLenum face, GLenum pname, GLfixed *params)
{
   int i;
   float temp[4];

   int count = get_materialv_internal(face, pname, temp);

   vcos_assert(count <= 4);

   for (i = 0; i < count; i++)
      params[i] = float_to_fixed(temp[i]);

   return count;
}

/*
   int glGetTexEnviv_impl_11 (GLenum env, GLenum pname, GLint *params)

   Khronos documentation:

   The command glTexEnv() sets parameters of the texture environment that specifies 
   how texture values are interpreted when texturing a fragment; env must be TEXTURE ENV.

      :  :  :

   The point sprite texture coordinate replacement mode is set with the command glTexEnv()
   where env is POINT SPRITE OES and pname is COORD REPLACE OES

   State variables that can be obtained using any of GetBooleanv, GetIntegerv, GetFixedv, or 
   GetFloatv are listed with just one of these commands... State variables for which any other 
   command is listed as the query command can be obtained only by using that command.

   The env argument to GetTexEnv must be TEXTURE ENV*

   COORD REPLACE OES GetTexEnviv (retrieved with GL_POINT_SPRITE_OES target)
   TEXTURE ENV MODE GetTexEnviv 
   COMBINE RGB GetTexEnviv 
   COMBINE ALPHA GetTexEnviv 
   SRC0 RGB GetTexEnviv 
   SRC1 RGB GetTexEnviv 
   SRC2 RGB GetTexEnviv 
   SRC0 ALPHA GetTexEnviv 
   SRC1 ALPHA GetTexEnviv 
   SRC2 ALPHA GetTexEnviv 
   OPERAND0 RGB GetTexEnviv 
   OPERAND1 RGB GetTexEnviv 
   OPERAND2 RGB GetTexEnviv 
   OPERAND0 ALPHA GetTexEnviv 
   OPERAND1 ALPHA GetTexEnviv 
   OPERAND2 ALPHA GetTexEnviv

   TEXTURE ENV COLOR GetTexEnvfv (4) .
   RGB SCALE GetTexEnvfv .
   ALPHA SCALE GetTexEnvfv .

   * but note we don't strictly comply with this

   Other commands exist to obtain state variables that are identified by a category
   (clip plane, light, material, etc.) as well as a symbolic constant. These are
      ...
      void GetTexEnv{i}v( enum env, enum value, T data );
      ...

   Implementation notes:

   We don't believe that integer values can be retrieved from glGetTexEnv{xf}v and
   vice versa, but everyone else does. The spec says that the env argument to GetTexEnv() 
   must be TEXTURE ENV, but it's pretty clear that it should be POINT_SPRITE_OES if asking
   for COORD_REPLACE_OES to agree with TexEnv()

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   params is a valid pointer to 1 element

   Postconditions:

   If env is not a valid env or pname is not a valid texture environment integer state
   variable compatible with env, and no current error, error becomes INVALID_ENUM

   Invariants preserved:

   -
*/

int glGetTexEnviv_impl_11 (GLenum env, GLenum pname, GLint *params)
{
   switch (env) {
   case GL_POINT_SPRITE_OES:
      switch (pname) {
      case GL_COORD_REPLACE_OES:
         return get_texenv_integer_internal(env, pname, params);
      default:
         {
         GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

         glxx_server_state_set_error(state, GL_INVALID_ENUM);

         GL11_UNLOCK_SERVER_STATE();
         return 0;
         }
      }
      UNREACHABLE();
      break;
   case GL_TEXTURE_ENV:
      switch (pname) {
      case GL_TEXTURE_ENV_MODE:
      case GL_COMBINE_RGB:
      case GL_COMBINE_ALPHA:
      case GL_SRC0_RGB:
      case GL_SRC1_RGB:
      case GL_SRC2_RGB:
      case GL_SRC0_ALPHA:
      case GL_SRC1_ALPHA:
      case GL_SRC2_ALPHA:
      case GL_OPERAND0_RGB:
      case GL_OPERAND1_RGB:
      case GL_OPERAND2_RGB:
      case GL_OPERAND0_ALPHA:
      case GL_OPERAND1_ALPHA:
      case GL_OPERAND2_ALPHA:
         return get_texenv_integer_internal(env, pname, params);
      case GL_TEXTURE_ENV_COLOR:
      {
         GLfloat temp[4];
         GLuint count = get_texenv_float_internal(env, pname, temp);
         GLuint i;

         vcos_assert(count <= 4);

         for (i = 0; i < count; i++) {
            params[i] = (GLint)floor((4294967295.0f * temp[i] - 1.0f) / 2.0f + 0.5f);

            if (params[i] < 0)
               params[i] = 0x7fffffff;
         }

         return count;
      }
      case GL_RGB_SCALE:
      case GL_ALPHA_SCALE:
      {
         GLfloat temp;
         GLuint count = get_texenv_float_internal(env, pname, &temp);

         vcos_assert(count == 1);

         params[0] = float_to_int(temp);

         return count;
      }
      default:
         {
         GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

         glxx_server_state_set_error(state, GL_INVALID_ENUM);

         GL11_UNLOCK_SERVER_STATE();
         return 0;
         }
      }
      UNREACHABLE();
      break;
   default:
      {
      GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

      glxx_server_state_set_error(state, GL_INVALID_ENUM);

      GL11_UNLOCK_SERVER_STATE();
      return 0;
      }
   }
}

/*
   int glGetTexEnvfv_impl_11 (GLenum env, GLenum pname, GLfloat *params)
   int glGetTexEnvxv_impl_11 (GLenum env, GLenum pname, GLfixed *params)

   Khronos documentation:

   The command glTexEnv() sets parameters of the texture environment that specifies 
   how texture values are interpreted when texturing a fragment; env must be TEXTURE ENV.

      :  :  :
   
   The point sprite texture coordinate replacement mode is set with the command glTexEnv()
   where env is POINT SPRITE OES and pname is COORD REPLACE OES*

   State variables that can be obtained using any of GetBooleanv, GetIntegerv, GetFixedv, or 
   GetFloatv are listed with just one of these commands... State variables for which any other 
   command is listed as the query command can be obtained only by using that command.

   The env argument to GetTexEnv must be TEXTURE ENV

      COORD REPLACE OES GetTexEnviv (retrieved with GL_POINT_SPRITE_OES target)
      TEXTURE ENV MODE GetTexEnviv 
      COMBINE RGB GetTexEnviv 
      COMBINE ALPHA GetTexEnviv 
      SRC0 RGB GetTexEnviv 
      SRC1 RGB GetTexEnviv 
      SRC2 RGB GetTexEnviv 
      SRC0 ALPHA GetTexEnviv 
      SRC1 ALPHA GetTexEnviv 
      SRC2 ALPHA GetTexEnviv 
      OPERAND0 RGB GetTexEnviv 
      OPERAND1 RGB GetTexEnviv 
      OPERAND2 RGB GetTexEnviv 
      OPERAND0 ALPHA GetTexEnviv 
      OPERAND1 ALPHA GetTexEnviv 
      OPERAND2 ALPHA GetTexEnviv

      TEXTURE ENV COLOR GetTexEnvfv (4) .
      RGB SCALE GetTexEnvfv .
      ALPHA SCALE GetTexEnvfv .

   Other commands exist to obtain state variables that are identified by a category
   (clip plane, light, material, etc.) as well as a symbolic constant. These are
      ...
      void GetTexEnv{xf}v( enum env, enum value, T data );
      ...

   * this is an integer variable and so not accessible here

   Implementation notes:

   We don't believe that integer values can be retrieved from glGetTexEnv{xf}v and
   vice versa, but everyone else does. The spec says that the env argument to GetTexEnv() 
   must be TEXTURE ENV, but it's pretty clear that it should be POINT_SPRITE_OES if asking
   for COORD_REPLACE_OES to agree with TexEnv()

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   params is a valid pointer to sufficient elements for the state variable specified

   Postconditions:

   If env is not a valid env or pname is not a valid texture environment floating point state
   variable compatible with env, and no current error, error becomes INVALID_ENUM

   Invariants preserved:

   -
*/

static int get_texenv_float_or_fixed_internal (GLenum env, GLenum pname, float *params)
{
   switch (env) {
   case GL_POINT_SPRITE_OES:
      switch (pname) {
      case GL_COORD_REPLACE_OES:
      {
         int temp; 
         int count = get_texenv_integer_internal(env, pname, &temp);

         vcos_assert(count == 1);

         params[0] = (float)temp;

         return count;
      }
      default:
         {
         GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

         glxx_server_state_set_error(state, GL_INVALID_ENUM);

         GL11_UNLOCK_SERVER_STATE();
         return 0;
         }
      }
      UNREACHABLE();
      break;
   case GL_TEXTURE_ENV:
      switch (pname) {
      case GL_TEXTURE_ENV_MODE:
      case GL_COMBINE_RGB:
      case GL_COMBINE_ALPHA:
      case GL_SRC0_RGB:
      case GL_SRC1_RGB:
      case GL_SRC2_RGB:
      case GL_SRC0_ALPHA:
      case GL_SRC1_ALPHA:
      case GL_SRC2_ALPHA:
      case GL_OPERAND0_RGB:
      case GL_OPERAND1_RGB:
      case GL_OPERAND2_RGB:
      case GL_OPERAND0_ALPHA:
      case GL_OPERAND1_ALPHA:
      case GL_OPERAND2_ALPHA:
      {
         int temp;
         int count = get_texenv_integer_internal(env, pname, &temp);

         vcos_assert(count == 1);

         params[0] = (float)temp;

         return count;
      }
      case GL_TEXTURE_ENV_COLOR:
      case GL_RGB_SCALE:
      case GL_ALPHA_SCALE: 
         return get_texenv_float_internal(env, pname, params);
      default:
         {
         GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

         glxx_server_state_set_error(state, GL_INVALID_ENUM);

         GL11_UNLOCK_SERVER_STATE();
         return 0;
         }
      }
      UNREACHABLE();
      break;
   default:
      {
      GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

      glxx_server_state_set_error(state, GL_INVALID_ENUM);

      GL11_UNLOCK_SERVER_STATE();
      return 0;
      }
   }
}

int glGetTexEnvfv_impl_11 (GLenum env, GLenum pname, GLfloat *params)
{
   return get_texenv_float_or_fixed_internal(env, pname, params);
}

/*
   glLightModelf_impl_11 (GLenum pname, GLfloat param)
   glLightModelfv_impl_11 (GLenum pname, const GLfloat *params)
   glLightModelx_impl_11 (GLenum pname, GLfixed param)
   glLightModelxv_impl_11 (GLenum pname, const GLfixed *params)

   Khronos documentation:

   Lighting parameters are divided into three categories: material parameters, light
   source parameters, and lighting model parameters (see Table 2.8). Sets of lighting
   parameters are specified with
            :  :  :  :
      void LightModel{xf}( enum pname, T param );
      void LightModel{xf}v( enum pname, T params );

   Lighting Model Parameters

      LIGHT MODEL AMBIENT 4
      LIGHT MODEL TWO SIDE 1

   Implementation notes:

   Possible deviation from spec: in the following case:
      glLightModelf(GL_LIGHT_MODEL_AMBIENT, blah);
   Perhaps we should give an error. At the moment we interpret the color vector as (blah,blah,blah,1).

   The range of light, lightmodel and material color parameters is -INF to INF (i.e. unclamped)

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   For vector variants, params be a valid pointer to 4 elements

   Postconditions:   

   If pname is not a valid lightmodel parameter and no current error, error becomes GL_INVALID_ENUM

   Invariants preserved:
*/

static void lightmodelv_internal (GLenum pname, const GLfloat *params)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

   switch (pname) {
   case GL_LIGHT_MODEL_AMBIENT:
      {
         int i;
         for (i = 0; i < 4; i++)
            state->lightmodel.ambient[i] = params[i];
         break;
      }
   case GL_LIGHT_MODEL_TWO_SIDE:
      state->changed_light = true;
      state->lightmodel.two_side = params[0] != 0.0f;
      break;
   default:
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
      break;
   }

   GL11_UNLOCK_SERVER_STATE();
}

void glLightModelf_impl_11 (GLenum pname, GLfloat param)
{
   GLfloat params[4];

   params[0] = param;
   params[1] = param;
   params[2] = param;
   params[3] = 1.0f;

   lightmodelv_internal(pname, params);
}

void glLightModelfv_impl_11 (GLenum pname, const GLfloat *params)
{
   lightmodelv_internal(pname, params);
}

void glLightModelx_impl_11 (GLenum pname, GLfixed param)
{
   GLfloat params[4];

   params[0] = fixed_to_float(param);
   params[1] = fixed_to_float(param);
   params[2] = fixed_to_float(param);
   params[3] = 1.0f;

   lightmodelv_internal(pname, params);
}

void glLightModelxv_impl_11 (GLenum pname, const GLfixed *params)
{
   int i;
   GLfloat temp[4];

   for (i = 0; i < 4; i++)
      temp[i] = fixed_to_float(params[i]);

   lightmodelv_internal(pname, temp);
}

//see documentation in glxx/glxx_server.c
void glLineWidthx_impl_11 (GLfixed width)
{
   glxx_line_width_internal(fixed_to_float(width));
}

/*
   void glLoadIdentity_impl_11 (void)

   Khronos documentation:

   The two basic commands for affecting the current matrix are

      void LoadMatrix{xf}( T m[16] );
      void MultMatrix{xf}( T m[16] );

   LoadMatrix takes a pointer to a 4 * 4 matrix stored in column-major order as 16
   consecutive fixed- or floating-point values. The specified matrix replaces the 
   current matrix with the one pointed to.

   The command

      void LoadIdentity( void );

   effectively calls LoadMatrix with the identity matrix:

   Implementation notes:

   Non-overlapping precondition on load_matrix_internal() is satisfied as the 
   stack never overlaps the GL state.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   Invariants preserved:
*/

void glLoadIdentity_impl_11 (void)
{
   float m[16];

   m[0]  = 1.0f;
   m[1]  = 0.0f;
   m[2]  = 0.0f;
   m[3]  = 0.0f;

   m[4]  = 0.0f;
   m[5]  = 1.0f;
   m[6]  = 0.0f;
   m[7]  = 0.0f;

   m[8]  = 0.0f;
   m[9]  = 0.0f;
   m[10] = 1.0f;
   m[11] = 0.0f;

   m[12] = 0.0f;
   m[13] = 0.0f;
   m[14] = 0.0f;
   m[15] = 1.0f;

   load_matrix_internal(m);
}

/*
   void glLoadMatrixf_impl_11 (const GLfloat *m)
   void glLoadMatrixx_impl_11 (const GLfixed *m)

   Khronos documentation:

   The two basic commands for affecting the current matrix are

      void LoadMatrix{xf}( T m[16] );
      void MultMatrix{xf}( T m[16] );

   LoadMatrix takes a pointer to a 4 * 4 matrix stored in column-major order as 16
   consecutive fixed- or floating-point values. The specified matrix replaces the 
   current matrix with the one pointed to.

   Implementation notes:

   Non-overlapping precondition on load_matrix_internal is satisfied as m must
   never point to part of the GL state and the stack never overlaps the GL state 
   either.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   m is a valid pointer to 16 elements not overlapping the GL state

   Postconditions:   

   Invariants preserved:
*/

void glLoadMatrixf_impl_11 (const GLfloat *m)
{
   load_matrix_internal(m);
}

void glLoadMatrixx_impl_11 (const GLfixed *m)
{
   int i;
   float f[16];

   for (i = 0; i < 16; i++)
      f[i] = fixed_to_float(m[i]);

   load_matrix_internal(f);
}

/*
   void glMultMatrixf_impl_11 (const GLfloat *m)
   void glMultMatrixx_impl_11 (const GLfixed *m)

   Khronos documentation:

   The two basic commands for affecting the current matrix are

      void LoadMatrix{xf}( T m[16] );
      void MultMatrix{xf}( T m[16] );

   MultMatrix takes the same type argument as LoadMatrix, but multiplies the current
   matrix by the one pointed to and replaces the current matrix with the product. If C
   is the current matrix and M is the matrix pointed to by MultMatrix’s argument,
   then the resulting current matrix, C', is

      C' = CM.
   
   Implementation notes:

   -

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context
   m is a valid pointer to 16 elements

   Postconditions:   

   Invariants preserved:
*/

void glMultMatrixf_impl_11 (const GLfloat *m)
{
   mult_matrix_internal(m);
}

void glMultMatrixx_impl_11 (const GLfixed *m)
{
   int i;
   float f[16];

   for (i = 0; i < 16; i++)
      f[i] = fixed_to_float(m[i]);

   mult_matrix_internal(f);
}

/*
   void glRotatef_impl_11 (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
   void glRotatex_impl_11 (GLfixed angle, GLfixed x, GLfixed y, GLfixed z)

   Khronos documentation:

   There are a variety of other commands that manipulate matrices. Rotate,
   Translate, Scale, Frustum, and Ortho manipulate the current matrix. Each computes
   a matrix and then invokes MultMatrix with this matrix. In the case of

      void Rotate{xf}(T theta, T x, T y, T z );

   Theta gives an angle of rotation in degrees; the coordinates of a vector v are given by
   v = (x y z)T . The computed matrix is a counter-clockwise rotation about the line
   through the origin with the specified axis when that axis is pointing up (i.e. the
   right-hand rule determines the sense of the rotation angle). The matrix is thus

            0
        R   0
            0
      0 0 0 1

   Let u = v/||v|| = ( x' y' z' )T . If

            0    -z'    y'
      S =   z'    0    -x'
           -y'    x'    0

   then

   R = uuT + (cos theta) (I - uuT) + (sin theta) S

   Implementation notes:

   The above bits of maths have been expanded out in the implementation. Division by zero
   may occur for very short axis vectors.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   Invariants preserved:
*/

static void rotate_internal(float angle, float x, float y, float z)
{
   float s, c;
   float m[16];

   /*
      normalize axis vector
   */
   if(x != 0.0f || y != 0.0f || z != 0.0f)
   {
      float l = (float)sqrt(x * x + y * y + z * z);

      x = x / l;
      y = y / l;
      z = z / l;
   }
   /*
      convert angle to radians
   */

   angle = 2.0f * PI * angle / 360.0f;

   /*
      build matrix
   */

   s = (float)sin(angle);
   c = (float)cos(angle);

   m[0]  = x * x * (1 - c) + c;
   m[1]  = y * x * (1 - c) + z * s;
   m[2]  = z * x * (1 - c) - y * s;
   m[3]  = 0.0f;

   m[4]  = x * y * (1 - c) - z * s;
   m[5]  = y * y * (1 - c) + c;
   m[6]  = z * y * (1 - c) + x * s;
   m[7]  = 0.0f;

   m[8]  = x * z * (1 - c) + y * s;
   m[9]  = y * z * (1 - c) - x * s;
   m[10] = z * z * (1 - c) + c;
   m[11] = 0.0f;

   m[12] = 0.0f;
   m[13] = 0.0f;
   m[14] = 0.0f;
   m[15] = 1.0f;

   mult_matrix_internal(m);
}

void glRotatef_impl_11 (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   rotate_internal(angle, x, y, z);
}

void glRotatex_impl_11 (GLfixed angle, GLfixed x, GLfixed y, GLfixed z)
{
   rotate_internal(fixed_to_float(angle),
                   fixed_to_float(x),
                   fixed_to_float(y),
                   fixed_to_float(z));
}

/*
   void glScalef_impl_11 (GLfloat x, GLfloat y, GLfloat z)
   void glScalex_impl_11 (GLfixed x, GLfixed y, GLfixed z)

   Khronos documentation:

   There are a variety of other commands that manipulate matrices. Rotate,
   Translate, Scale, Frustum, and Ortho manipulate the current matrix. Each computes
   a matrix and then invokes MultMatrix with this matrix. In the case of

      void Scale{xf}( T x, T y, T z );

   produces a general scaling along the x-, y-, and z- axes. The corresponding matrix

      x 0 0 0
      0 y 0 0
      0 0 z 0
      0 0 0 1

   Implementation notes:

   We ignore invalid floats - we do not generate an INVALID_VALUE error.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   Invariants preserved:
*/

static void scale_internal(float x, float y, float z)
{
   float m[16];

   m[0]  = x;
   m[1]  = 0.0f;
   m[2]  = 0.0f;
   m[3]  = 0.0f;

   m[4]  = 0.0f;
   m[5]  = y;
   m[6]  = 0.0f;
   m[7]  = 0.0f;

   m[8]  = 0.0f;
   m[9]  = 0.0f;
   m[10] = z;
   m[11] = 0.0f;

   m[12] = 0.0f;
   m[13] = 0.0f;
   m[14] = 0.0f;
   m[15] = 1.0f;

   mult_matrix_internal(m);
}

void glScalef_impl_11 (GLfloat x, GLfloat y, GLfloat z)
{
   scale_internal(x, y, z);
}

void glScalex_impl_11 (GLfixed x, GLfixed y, GLfixed z)
{
   scale_internal(fixed_to_float(x),
                  fixed_to_float(y),
                  fixed_to_float(z));
}


/*
   void glTranslatef_impl_11 (GLfloat x, GLfloat y, GLfloat z)
   void glTranslatex_impl_11 (GLfixed x, GLfixed y, GLfixed z)

   Khronos documentation:

   There are a variety of other commands that manipulate matrices. Rotate,
   Translate, Scale, Frustum, and Ortho manipulate the current matrix. Each computes
   a matrix and then invokes MultMatrix with this matrix. In the case of

   The arguments to

      void Translate{xf}( T x, T y, T z );

   give the coordinates of a translation vector as (x y z)T . The resulting matrix is a
   translation by the specified vector:

      1 0 0 x
      0 1 0 y
      0 0 1 z
      0 0 0 1

   Implementation notes:

   We ignore invalid floats - we do not generate an INVALID_VALUE error.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   Invariants preserved:
*/

static void translate_internal(float x, float y, float z)
{
   float m[16];

   m[0]  = 1.0f;
   m[1]  = 0.0f;
   m[2]  = 0.0f;
   m[3]  = 0.0f;

   m[4]  = 0.0f;
   m[5]  = 1.0f;
   m[6]  = 0.0f;
   m[7]  = 0.0f;

   m[8]  = 0.0f;
   m[9]  = 0.0f;
   m[10] = 1.0f;
   m[11] = 0.0f;

   m[12] = x;
   m[13] = y;
   m[14] = z;
   m[15] = 1.0f;

   mult_matrix_internal(m);
}

void glTranslatef_impl_11 (GLfloat x, GLfloat y, GLfloat z)
{
   translate_internal(x, y, z);
}

void glTranslatex_impl_11 (GLfixed x, GLfixed y, GLfixed z)
{
   translate_internal(fixed_to_float(x),
                      fixed_to_float(y),
                      fixed_to_float(z));
}

/*
   void glOrthof_impl_11 (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
   void glOrthox_impl_11 (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)

   Khronos documentation:

   There are a variety of other commands that manipulate matrices. Rotate,
   Translate, Scale, Frustum, and Ortho manipulate the current matrix. Each computes
   a matrix and then invokes MultMatrix with this matrix. In the case of

   void Ortho{xf}( T l, T r, T b, T t, T n, T f );

   describes a matrix that produces parallel projection. (l b ? n)T and (r t ? n)T
   specify the points on the near clipping plane that are mapped to the lower left and
   upper right corners of the window, respectively. f gives the distance from the eye
   to the far clipping plane. If l is equal to r, b is equal to t, or n is equal to f, the
   error INVALID VALUE results. The corresponding matrix is

   2/(r-l)     0        0          -(r+l)/(r-l)
   0           2/(t-b)  0          -(t+b)/(t-b)
   0           0       -2/(f-n)    -(f+n)/(f-n)
   0           0        0           1
   
   Implementation notes:

   We ignore invalid floats - we do not generate an INVALID_VALUE error. We perform the equality test
   on the floating point versions of the parameters.

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   If constraints on l, r, b, t, n, f are not met and no current error, error becomes
   INVALID_VALUE

   Invariants preserved:
*/

static void ortho_internal(float l, float r, float b, float t, float n, float f)
{
   if (l != r && b != t && n != f) {
      float m[16];

      m[0]  = 2.0f / (r - l);
      m[1]  = 0.0f;
      m[2]  = 0.0f;
      m[3]  = 0.0f;

      m[4]  = 0.0f;
      m[5]  = 2.0f / (t - b);
      m[6]  = 0.0f;
      m[7]  = 0.0f;

      m[8]  = 0.0f;
      m[9]  = 0.0f;
      m[10] = -2.0f / (f - n);
      m[11] = 0.0f;

      m[12] = -(r + l) / (r - l);
      m[13] = -(t + b) / (t - b);
      m[14] = -(f + n) / (f - n);
      m[15] = 1.0f;

      mult_matrix_internal(m);
   } else {
      GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();

      glxx_server_state_set_error(state, GL_INVALID_VALUE);

      GL11_UNLOCK_SERVER_STATE();
   }
}

void glOrthof_impl_11 (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
   ortho_internal(left, right, bottom, top, zNear, zFar);
}

void glOrthox_impl_11 (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
   ortho_internal(fixed_to_float(left),
                  fixed_to_float(right),
                  fixed_to_float(bottom),
                  fixed_to_float(top),
                  fixed_to_float(zNear),
                  fixed_to_float(zFar));
}

//see documentation in glxx/glxx_server.c
void glPolygonOffsetx_impl_11 (GLfixed factor, GLfixed units)
{
   glxx_polygon_offset_internal(fixed_to_float(factor),
                           fixed_to_float(units));
}

/*
   void glPopMatrix_impl_11 (void)

   Khronos documentation:

   There is a stack of matrices for each of matrix modes MODELVIEW and
   PROJECTION, and for each texture unit. For MODELVIEW mode, the stack depth
   is at least 16 (that is, there is a stack of at least 16 model-view matrices). For the
   other modes, the depth is at least 2. Texture matrix stacks for all texture units have
   the same depth. The current matrix in any mode is the matrix on the top of the
   stack for that mode.

      void PopMatrix( void );

   pops the top entry off of the stack, replacing the current matrix with the matrix
   that was the second entry in the stack.

   Popping a matrix off a stack with only one entry generates the error STACK UNDERFLOW.

   Implementation notes:

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   Invariants preserved:

   For all stacks associated with state:
      0 <= pos < GL11_CONFIG_MAX_STACK_DEPTH
*/

void glPopMatrix_impl_11 (void)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();
   GL11_MATRIX_STACK_T *stack = get_stack(state);

   if (stack->pos > 0)
      stack->pos--;
   else 
      glxx_server_state_set_error(state, GL_STACK_UNDERFLOW);

   GL11_UNLOCK_SERVER_STATE();
}

/*
   void glPushMatrix_impl_11 (void)

   Khronos documentation:

   There is a stack of matrices for each of matrix modes MODELVIEW and
   PROJECTION, and for each texture unit. For MODELVIEW mode, the stack depth
   is at least 16 (that is, there is a stack of at least 16 model-view matrices). For the
   other modes, the depth is at least 2. Texture matrix stacks for all texture units have
   the same depth. The current matrix in any mode is the matrix on the top of the
   stack for that mode.

      void PushMatrix( void );

   pushes the stack down by one, duplicating the current matrix in both the top of the
   stack and the entry below it.

   Pushing a matrix onto a full stack generates STACK OVERFLOW.

   Implementation notes:

   Preconditions:

   Valid EGL server state exists
   EGL server state has a current OpenGL ES 1.1 context

   Postconditions:   

   Invariants preserved:

   For all stacks associated with state:
      0 <= pos < GL11_CONFIG_MAX_STACK_DEPTH
*/

void glPushMatrix_impl_11 (void)
{
   GLXX_SERVER_STATE_T *state = GL11_LOCK_SERVER_STATE();
   GL11_MATRIX_STACK_T *stack = get_stack(state);

   if (stack->pos + 1 < GL11_CONFIG_MAX_STACK_DEPTH) {
      gl11_matrix_load(stack->body[stack->pos + 1], stack->body[stack->pos]);

      stack->pos++;
   } else
      glxx_server_state_set_error(state, GL_STACK_OVERFLOW);

   GL11_UNLOCK_SERVER_STATE();
}
