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

#include "middleware/khronos/glsl/glsl_header.h"
#include "middleware/khronos/glsl/glsl_mendenhall.h"

// xstr(s) expands first, then stringifies it.
#define xstr(s) str(s)
#define str(s) #s

//
// Angle and Trigonometry Functions
//

const char *radians__const_float__const_float__body = 
"float radians(float degrees)"
"{"
"   const float pi_on_180 = 0.01745329252;"
"   return pi_on_180 * degrees;"
"}";

const char *radians__const_vec2__const_vec2__body = 
"vec2 radians(vec2 degrees)"
"{"
"   vec2 r;"
"   const float pi_on_180 = 0.01745329252;"
"   "
"   r[0] = pi_on_180 * degrees[0];"
"   r[1] = pi_on_180 * degrees[1];"
"   "
"   return r;"
"}";

const char *radians__const_vec3__const_vec3__body = 
"vec3 radians(vec3 degrees)"
"{"
"   vec3 r;"
"   const float pi_on_180 = 0.01745329252;"
"   "
"   r[0] = pi_on_180 * degrees[0];"
"   r[1] = pi_on_180 * degrees[1];"
"   r[2] = pi_on_180 * degrees[2];"
"   "
"   return r;"
"}";

const char *radians__const_vec4__const_vec4__body = 
"vec4 radians(vec4 degrees)"
"{"
"   vec4 r;"
"   const float pi_on_180 = 0.01745329252;"
"   "
"   r[0] = pi_on_180 * degrees[0];"
"   r[1] = pi_on_180 * degrees[1];"
"   r[2] = pi_on_180 * degrees[2];"
"   r[3] = pi_on_180 * degrees[3];"
"   "
"   return r;"
"}";

const char *degrees__const_float__const_float__body = 
"float degrees(float radians)"
"{"
"   const float _180_on_pi = 57.29577951;"
"   return _180_on_pi * radians;"
"}";

const char *degrees__const_vec2__const_vec2__body = 
"vec2 degrees(vec2 radians)"
"{"
"   vec2 r;"
"   const float _180_on_pi = 57.29577951;"
"   "
"   r[0] = _180_on_pi * radians[0];"
"   r[1] = _180_on_pi * radians[1];"
"   "
"   return r;"
"}";

const char *degrees__const_vec3__const_vec3__body = 
"vec3 degrees(vec3 radians)"
"{"
"   vec3 r;"
"   const float _180_on_pi = 57.29577951;"
"   "
"   r[0] = _180_on_pi * radians[0];"
"   r[1] = _180_on_pi * radians[1];"
"   r[2] = _180_on_pi * radians[2];"
"   "
"   return r;"
"}";

const char *degrees__const_vec4__const_vec4__body = 
"vec4 degrees(vec4 radians)"
"{"
"   vec4 r;"
"   const float _180_on_pi = 57.29577951;"
"   "
"   r[0] = _180_on_pi * radians[0];"
"   r[1] = _180_on_pi * radians[1];"
"   r[2] = _180_on_pi * radians[2];"
"   r[3] = _180_on_pi * radians[3];"
"   "
"   return r;"
"}";

const char *sin__const_float__const_float__body = 
"float sin(float angle)"
"{"
"   float x,xint,xx,xxxx,sinpart,cospart,sinsemi,cossemi,sinfinal,cosfinal;"
"   float cospart2,sinpart2,cos2part,sin2part,cos2semi,sin2semi,sincospart,sincossemi;"
#ifndef SKIP_CORRECTION
"   float sinuncorr,cosuncorr,hypsq,errormul;"
#endif
   
"   x = angle * " xstr(F_CONST_RECIP2PI) ";"
"   xint = float($$nearest(x));"
"   x = x - xint;"
"   xx = x * x;"
"   xxxx = xx * xx;"
"   sinpart = xx * " xstr(F_COEFF_SIN7) ";"
"   cospart = xx * " xstr(F_COEFF_COS6) ";"
"   sinpart = sinpart + " xstr(F_COEFF_SIN5) ";"
"   cospart = cospart + " xstr(F_COEFF_COS4) ";"
"   sinpart2 = xx * " xstr(F_COEFF_SIN3) ";"
"   sinpart = sinpart * xxxx;"
"   cospart2 = xx * " xstr(F_COEFF_COS2) ";"
"   cospart = cospart * xxxx;"
"   sinpart = sinpart + sinpart2;"
"   cospart = cospart + cospart2;"
"   sinpart = sinpart + " xstr(F_COEFF_SIN1) ";"
"   cospart = cospart + " xstr(F_COEFF_COS0) ";"
"   sinpart = sinpart * x;"
"   cos2part = cospart * cospart;"
"   sin2part = sinpart * sinpart;"
"   sincospart = sinpart * cospart;"
"   cossemi = cos2part - sin2part;"
"   sinsemi = sincospart * 2.0;"

"   sin2semi = sinsemi * sinsemi;"
"   cos2semi = cossemi * cossemi;"
"   sincossemi = sinsemi * cossemi;"

#ifndef SKIP_CORRECTION
"   hypsq = cos2semi + sin2semi;"
"   cosuncorr = cos2semi - sin2semi;"
"   errormul = " xstr(F_CONST_TWO) " - hypsq;"
"   sinuncorr = sincossemi * 2.0;"
"   cosfinal = errormul * cosuncorr;"
"   sinfinal = errormul * sinuncorr;"
#else
"   cosfinal = cos2semi - sin2semi;"
"   sinfinal = sincossemi * 2.0;"
#endif

"   return sinfinal;"
"}";

const char *sin__const_vec2__const_vec2__body = 
"vec2 sin(vec2 angle)"
"{"
"   return vec2(sin(angle[0]), sin(angle[1]));"
"}";

const char *sin__const_vec3__const_vec3__body = 
"vec3 sin(vec3 angle)"
"{"
"   return vec3(sin(angle[0]), sin(angle[1]), sin(angle[2]));"
"}";

const char *sin__const_vec4__const_vec4__body = 
"vec4 sin(vec4 angle)"
"{"
"   return vec4(sin(angle[0]), sin(angle[1]), sin(angle[2]), sin(angle[3]));"
"}";

const char *cos__const_float__const_float__body = 
"float cos(float angle)"
"{"
"   float x,xint,xx,xxxx,sinpart,cospart,sinsemi,cossemi,sinfinal,cosfinal;"
"   float cospart2,sinpart2,cos2part,sin2part,cos2semi,sin2semi,sincospart,sincossemi;"
#ifndef SKIP_CORRECTION
"   float sinuncorr,cosuncorr,hypsq,errormul;"
#endif
   
"   x = angle * " xstr(F_CONST_RECIP2PI) ";"
"   xint = float($$nearest(x));"
"   x = x - xint;"
"   xx = x * x;"
"   xxxx = xx * xx;"
"   sinpart = xx * " xstr(F_COEFF_SIN7) ";"
"   cospart = xx * " xstr(F_COEFF_COS6) ";"
"   sinpart = sinpart + " xstr(F_COEFF_SIN5) ";"
"   cospart = cospart + " xstr(F_COEFF_COS4) ";"
"   sinpart2 = xx * " xstr(F_COEFF_SIN3) ";"
"   sinpart = sinpart * xxxx;"
"   cospart2 = xx * " xstr(F_COEFF_COS2) ";"
"   cospart = cospart * xxxx;"
"   sinpart = sinpart + sinpart2;"
"   cospart = cospart + cospart2;"
"   sinpart = sinpart + " xstr(F_COEFF_SIN1) ";"
"   cospart = cospart + " xstr(F_COEFF_COS0) ";"
"   sinpart = sinpart * x;"
"   cos2part = cospart * cospart;"
"   sin2part = sinpart * sinpart;"
"   sincospart = sinpart * cospart;"
"   cossemi = cos2part - sin2part;"
"   sinsemi = sincospart * 2.0;"

"   sin2semi = sinsemi * sinsemi;"
"   cos2semi = cossemi * cossemi;"
"   sincossemi = sinsemi * cossemi;"

#ifndef SKIP_CORRECTION
"   hypsq = cos2semi + sin2semi;"
"   cosuncorr = cos2semi - sin2semi;"
"   errormul = " xstr(F_CONST_TWO) " - hypsq;"
"   sinuncorr = sincossemi * 2.0;"
"   cosfinal = errormul * cosuncorr;"
"   sinfinal = errormul * sinuncorr;"
#else
"   cosfinal = cos2semi - sin2semi;"
"   sinfinal = sincossemi * 2.0;"
#endif

"   return cosfinal;"
"}";

const char *cos__const_vec2__const_vec2__body = 
"vec2 cos(vec2 angle)"
"{"
"   return vec2(cos(angle[0]), cos(angle[1]));"
"}";

const char *cos__const_vec3__const_vec3__body = 
"vec3 cos(vec3 angle)"
"{"
"   return vec3(cos(angle[0]), cos(angle[1]), cos(angle[2]));"
"}";

const char *cos__const_vec4__const_vec4__body = 
"vec4 cos(vec4 angle)"
"{"
"   return vec4(cos(angle[0]), cos(angle[1]), cos(angle[2]), cos(angle[3]));"
"}";

const char *tan__const_float__const_float__body = 
"float tan(float angle)"
"{"
"   float x,xint,xx,xxxx,sinpart,cospart,sinsemi,cossemi,sinfinal,cosfinal;"
"   float cospart2,sinpart2,cos2part,sin2part,cos2semi,sin2semi,sincospart,sincossemi;"
#ifndef SKIP_CORRECTION
"   float sinuncorr,cosuncorr,hypsq,errormul;"
#endif
   
"   x = angle * " xstr(F_CONST_RECIP2PI) ";"
"   xint = float($$nearest(x));"
"   x = x - xint;"
"   xx = x * x;"
"   xxxx = xx * xx;"
"   sinpart = xx * " xstr(F_COEFF_SIN7) ";"
"   cospart = xx * " xstr(F_COEFF_COS6) ";"
"   sinpart = sinpart + " xstr(F_COEFF_SIN5) ";"
"   cospart = cospart + " xstr(F_COEFF_COS4) ";"
"   sinpart2 = xx * " xstr(F_COEFF_SIN3) ";"
"   sinpart = sinpart * xxxx;"
"   cospart2 = xx * " xstr(F_COEFF_COS2) ";"
"   cospart = cospart * xxxx;"
"   sinpart = sinpart + sinpart2;"
"   cospart = cospart + cospart2;"
"   sinpart = sinpart + " xstr(F_COEFF_SIN1) ";"
"   cospart = cospart + " xstr(F_COEFF_COS0) ";"
"   sinpart = sinpart * x;"
"   cos2part = cospart * cospart;"
"   sin2part = sinpart * sinpart;"
"   sincospart = sinpart * cospart;"
"   cossemi = cos2part - sin2part;"
"   sinsemi = sincospart * 2.0;"

"   sin2semi = sinsemi * sinsemi;"
"   cos2semi = cossemi * cossemi;"
"   sincossemi = sinsemi * cossemi;"

#ifndef SKIP_CORRECTION
"   hypsq = cos2semi + sin2semi;"
"   cosuncorr = cos2semi - sin2semi;"
"   errormul = " xstr(F_CONST_TWO) " - hypsq;"
"   sinuncorr = sincossemi * 2.0;"
"   cosfinal = errormul * cosuncorr;"
"   sinfinal = errormul * sinuncorr;"
#else
"   cosfinal = cos2semi - sin2semi;"
"   sinfinal = sincossemi * 2.0;"
#endif

"   return sinfinal / cosfinal;"
"}";

const char *tan__const_vec2__const_vec2__body = 
"vec2 tan(vec2 angle)"
"{"
"   return vec2(tan(angle[0]), tan(angle[1]));"
"}";

const char *tan__const_vec3__const_vec3__body = 
"vec3 tan(vec3 angle)"
"{"
"   return vec3(tan(angle[0]), tan(angle[1]), tan(angle[2]));"
"}";

const char *tan__const_vec4__const_vec4__body = 
"vec4 tan(vec4 angle)"
"{"
"   return vec4(tan(angle[0]), tan(angle[1]), tan(angle[2]), tan(angle[3]));"
"}";

const char *sqrt__const_float__const_float__body = 
"float sqrt(float x)"
"{"
"   return $$rcp($$rsqrt(x));"
"}";

const char *sqrt__const_vec2__const_vec2__body = 
"vec2 sqrt(vec2 x)"
"{"
"   return vec2(sqrt(x[0]), sqrt(x[1]));"
"}";

const char *sqrt__const_vec3__const_vec3__body = 
"vec3 sqrt(vec3 x)"
"{"
"   return vec3(sqrt(x[0]), sqrt(x[1]), sqrt(x[2]));"
"}";

const char *sqrt__const_vec4__const_vec4__body = 
"vec4 sqrt(vec4 x)"
"{"
"   return vec4(sqrt(x[0]), sqrt(x[1]), sqrt(x[2]), sqrt(x[3]));"
"}";

const char *inversesqrt__const_float__const_float__body = 
"float inversesqrt(float x)"
"{"
"   return $$rsqrt(x);"
"}";

const char *inversesqrt__const_vec2__const_vec2__body = 
"vec2 inversesqrt(vec2 x)"
"{"
"   return vec2(inversesqrt(x[0]), inversesqrt(x[1]));"
"}";

const char *inversesqrt__const_vec3__const_vec3__body = 
"vec3 inversesqrt(vec3 x)"
"{"
"   return vec3(inversesqrt(x[0]), inversesqrt(x[1]), inversesqrt(x[2]));"
"}";

const char *inversesqrt__const_vec4__const_vec4__body = 
"vec4 inversesqrt(vec4 x)"
"{"
"   return vec4(inversesqrt(x[0]), inversesqrt(x[1]), inversesqrt(x[2]), inversesqrt(x[3]));"
"}";

const char *atan__const_float__const_float__const_float__body = 
"float atan(float y, float x)"
"{"
"   float invhyp = inversesqrt(x*x + y*y);"
"   float num;"
"   if (y >= 0.0) {"
"      num = 0.0;"
"   } else {"
"      num = -1.0;"
"      y = - y;"
"      x = - x;"
"   }"
"   x = x * invhyp;"
"   y = y * invhyp;"
"   for(int i = 0; i < 12; i++) {"
"      float newx = x*x - y*y;"
"      float newy = 2.0*y*x;"
"      num = num * 2.0;"
"      if (x > 0.0) {"
"         x = newx;"
"         y = newy;"
"      } else {"
"         num = num + 1.0;"
"         x = -newx;"
"         y = -newy;"
"      }"
"   }"
"   const float pi_on_4096 = 7.6699039e-4;"
"   return num * pi_on_4096;"
"}";

const char *atan__const_vec2__const_vec2__const_vec2__body = 
"vec2 atan(vec2 y, vec2 x)"
"{"
"   return vec2(atan(y[0], x[0]), atan(y[1], x[1]));"
"}";

const char *atan__const_vec3__const_vec3__const_vec3__body = 
"vec3 atan(vec3 y, vec3 x)"
"{"
"   return vec3(atan(y[0], x[0]), atan(y[1], x[1]), atan(y[2], x[2]));"
"}";

const char *atan__const_vec4__const_vec4__const_vec4__body = 
"vec4 atan(vec4 y, vec4 x)"
"{"
"   return vec4(atan(y[0], x[0]), atan(y[1], x[1]), atan(y[2], x[2]), atan(y[3], x[3]));"
"}";

const char *atan__const_float__const_float__body = 
"float atan(float y_over_x)"
"{"
"   return atan(y_over_x, 1.0);"
"}";

const char *atan__const_vec2__const_vec2__body = 
"vec2 atan(vec2 y_over_x)"
"{"
"   return vec2(atan(y_over_x[0]), atan(y_over_x[1]));"
"}";

const char *atan__const_vec3__const_vec3__body = 
"vec3 atan(vec3 y_over_x)"
"{"
"   return vec3(atan(y_over_x[0]), atan(y_over_x[1]), atan(y_over_x[2]));"
"}";

const char *atan__const_vec4__const_vec4__body = 
"vec4 atan(vec4 y_over_x)"
"{"
"   return vec4(atan(y_over_x[0]), atan(y_over_x[1]), atan(y_over_x[2]), atan(y_over_x[3]));"
"}";

const char *asin__const_float__const_float__body = 
"float asin(float x)"
"{"
"   return atan(x, sqrt(1.0-x*x));"
"}";

const char *asin__const_vec2__const_vec2__body = 
"vec2 asin(vec2 x)"
"{"
"   return vec2(asin(x[0]), asin(x[1]));"
"}";

const char *asin__const_vec3__const_vec3__body = 
"vec3 asin(vec3 x)"
"{"
"   return vec3(asin(x[0]), asin(x[1]), asin(x[2]));"
"}";

const char *asin__const_vec4__const_vec4__body = 
"vec4 asin(vec4 x)"
"{"
"   return vec4(asin(x[0]), asin(x[1]), asin(x[2]), asin(x[3]));"
"}";

const char *acos__const_float__const_float__body = 
"float acos(float x)"
"{"
"   return atan(sqrt(1.0-x*x), x);"
"}";

const char *acos__const_vec2__const_vec2__body = 
"vec2 acos(vec2 x)"
"{"
"   return vec2(acos(x[0]), acos(x[1]));"
"}";

const char *acos__const_vec3__const_vec3__body = 
"vec3 acos(vec3 x)"
"{"
"   return vec3(acos(x[0]), acos(x[1]), acos(x[2]));"
"}";

const char *acos__const_vec4__const_vec4__body = 
"vec4 acos(vec4 x)"
"{"
"   return vec4(acos(x[0]), acos(x[1]), acos(x[2]), acos(x[3]));"
"}";

//
// Exponential Functions
//

const char *pow__const_float__const_float__const_float__body = 
"float pow(float x, float y)"
"{"
"   float temp = y * $$log2(x);"
#ifndef WORKAROUND_HW1451
"      return $$exp2(temp);"
#else
"   if (temp > -127.0) {"   //workaround for HW-1451
"      return $$exp2(temp);"
"   } else {"
"      return 0.0;"
"   }"
#endif
"}";

const char *pow__const_vec2__const_vec2__const_vec2__body = 
"vec2 pow(vec2 x, vec2 y)"
"{"
"   return vec2($$exp2(y[0] * $$log2(x[0])), $$exp2(y[1] * $$log2(x[1])));"
"}";

const char *pow__const_vec3__const_vec3__const_vec3__body = 
"vec3 pow(vec3 x, vec3 y)"
"{"
"   return vec3($$exp2(y[0] * $$log2(x[0])), $$exp2(y[1] * $$log2(x[1])), $$exp2(y[2] * $$log2(x[2])));"
"}";

const char *pow__const_vec4__const_vec4__const_vec4__body = 
"vec4 pow(vec4 x, vec4 y)"
"{"
"   return vec4($$exp2(y[0] * $$log2(x[0])), $$exp2(y[1] * $$log2(x[1])), $$exp2(y[2] * $$log2(x[2])), $$exp2(y[3] * $$log2(x[3])));"
"}";

const char *exp2__const_float__const_float__body = 
"float exp2(float x)"
"{"
#ifndef WORKAROUND_HW1451
"   return $$exp2(x);"
#else
"   if (x > -127.0) {"   /* workaround for HW-1451 */
"      return $$exp2(x);"
"   } else {"
"      return 0.0;"
"   }"
#endif
"}";

const char *exp2__const_vec2__const_vec2__body = 
"vec2 exp2(vec2 x)"
"{"
"   return vec2(exp2(x[0]), exp2(x[1]));"
"}";

const char *exp2__const_vec3__const_vec3__body = 
"vec3 exp2(vec3 x)"
"{"
"   return vec3(exp2(x[0]), exp2(x[1]), exp2(x[2]));"
"}";

const char *exp2__const_vec4__const_vec4__body = 
"vec4 exp2(vec4 x)"
"{"
"   return vec4(exp2(x[0]), exp2(x[1]), exp2(x[2]), exp2(x[3]));"
"}";

const char *log2__const_float__const_float__body = 
"float log2(float x)"
"{"
"   return $$log2(x);"
"}";

const char *log2__const_vec2__const_vec2__body = 
"vec2 log2(vec2 x)"
"{"
"   return vec2(log2(x[0]), log2(x[1]));"
"}";

const char *log2__const_vec3__const_vec3__body = 
"vec3 log2(vec3 x)"
"{"
"   return vec3(log2(x[0]), log2(x[1]), log2(x[2]));"
"}";

const char *log2__const_vec4__const_vec4__body = 
"vec4 log2(vec4 x)"
"{"
"   return vec4(log2(x[0]), log2(x[1]), log2(x[2]), log2(x[3]));"
"}";

const char *exp__const_float__const_float__body = 
"float exp(float x)"
"{"
"   const float e = 2.718281828;"
"   return pow(e, x);"
"}";

const char *exp__const_vec2__const_vec2__body = 
"vec2 exp(vec2 x)"
"{"
"   return vec2(exp(x[0]), exp(x[1]));"
"}";

const char *exp__const_vec3__const_vec3__body = 
"vec3 exp(vec3 x)"
"{"
"   return vec3(exp(x[0]), exp(x[1]), exp(x[2]));"
"}";

const char *exp__const_vec4__const_vec4__body = 
"vec4 exp(vec4 x)"
"{"
"   return vec4(exp(x[0]), exp(x[1]), exp(x[2]), exp(x[3]));"
"}";

const char *log__const_float__const_float__body = 
"float log(float x)"
"{"
"   const float recip_log2_e = 0.6931471805;"
"   return log2(x) * recip_log2_e;"
"}";

const char *log__const_vec2__const_vec2__body = 
"vec2 log(vec2 x)"
"{"
"   return vec2(log(x[0]), log(x[1]));"
"}";

const char *log__const_vec3__const_vec3__body = 
"vec3 log(vec3 x)"
"{"
"   return vec3(log(x[0]), log(x[1]), log(x[2]));"
"}";

const char *log__const_vec4__const_vec4__body = 
"vec4 log(vec4 x)"
"{"
"   return vec4(log(x[0]), log(x[1]), log(x[2]), log(x[3]));"
"}";

//
// Common Functions
//

const char *abs__const_float__const_float__body = 
"float abs(float x)"
"{"
"   return x >= 0.0 ? x : -x;"
"}";

const char *abs__const_vec2__const_vec2__body = 
"vec2 abs(vec2 x)"
"{"
"   return vec2(abs(x[0]), abs(x[1]));"
"}";

const char *abs__const_vec3__const_vec3__body = 
"vec3 abs(vec3 x)"
"{"
"   return vec3(abs(x[0]), abs(x[1]), abs(x[2]));"
"}";

const char *abs__const_vec4__const_vec4__body = 
"vec4 abs(vec4 x)"
"{"
"   return vec4(abs(x[0]), abs(x[1]), abs(x[2]), abs(x[3]));"
"}";

const char *sign__const_float__const_float__body = 
"float sign(float x)"
"{"
#ifdef __VIDEOCORE4__
"   return $$min(1.0, $$max(-1.0, x * 340000000000000000000000000000000000000.0));"  /* TODO: does this work? */
#else
"   return $$sign(x);"
#endif
"}";

const char *sign__const_vec2__const_vec2__body = 
"vec2 sign(vec2 x)"
"{"
"   return vec2(sign(x[0]), sign(x[1]));"
"}";

const char *sign__const_vec3__const_vec3__body = 
"vec3 sign(vec3 x)"
"{"
"   return vec3(sign(x[0]), sign(x[1]), sign(x[2]));"
"}";

const char *sign__const_vec4__const_vec4__body = 
"vec4 sign(vec4 x)"
"{"
"   return vec4(sign(x[0]), sign(x[1]), sign(x[2]), sign(x[3]));"
"}";

const char *floor__const_float__const_float__body = 
"float floor(float x)"
"{"
#ifdef __VIDEOCORE4__
"   float result = float(int(x));"
"   if (result > x) result = result - 1.0;"
"   return result;"
#else
"   return $$floor(x + 0.000001);"
#endif
"}";

const char *floor__const_vec2__const_vec2__body = 
"vec2 floor(vec2 x)"
"{"
"   return vec2(floor(x[0]), floor(x[1]));"
"}";

const char *floor__const_vec3__const_vec3__body = 
"vec3 floor(vec3 x)"
"{"
"   return vec3(floor(x[0]), floor(x[1]), floor(x[2]));"
"}";

const char *floor__const_vec4__const_vec4__body = 
"vec4 floor(vec4 x)"
"{"
"   return vec4(floor(x[0]), floor(x[1]), floor(x[2]), floor(x[3]));"
"}";

const char *ceil__const_float__const_float__body = 
"float ceil(float x)"
"{"
#ifdef __VIDEOCORE4__
"   float result = float(int(x));"
"   if (result < x) result = result + 1.0;"
"   return result;"
#else
"   return $$ceil(x);"
#endif
"}";

const char *ceil__const_vec2__const_vec2__body = 
"vec2 ceil(vec2 x)"
"{"
"   return vec2(ceil(x[0]), ceil(x[1]));"
"}";

const char *ceil__const_vec3__const_vec3__body = 
"vec3 ceil(vec3 x)"
"{"
"   return vec3(ceil(x[0]), ceil(x[1]), ceil(x[2]));"
"}";

const char *ceil__const_vec4__const_vec4__body = 
"vec4 ceil(vec4 x)"
"{"
"   return vec4(ceil(x[0]), ceil(x[1]), ceil(x[2]), ceil(x[3]));"
"}";

const char *fract__const_float__const_float__body = 
"float fract(float x)"
"{"
"   return x - floor(x);"
"}";

const char *fract__const_vec2__const_vec2__body = 
"vec2 fract(vec2 x)"
"{"
"   return vec2(fract(x[0]), fract(x[1]));"
"}";

const char *fract__const_vec3__const_vec3__body = 
"vec3 fract(vec3 x)"
"{"
"   return vec3(fract(x[0]), fract(x[1]), fract(x[2]));"
"}";

const char *fract__const_vec4__const_vec4__body = 
"vec4 fract(vec4 x)"
"{"
"   return vec4(fract(x[0]), fract(x[1]), fract(x[2]), fract(x[3]));"
"}";

const char *mod__const_float__const_float__const_float__body = 
"float mod(float x, float y)"
"{"
"   return x - y * floor(x / y);"
"}";

const char *mod__const_vec2__const_vec2__const_float__body = 
"vec2 mod(vec2 x, float y)"
"{"
"   return vec2(mod(x[0], y), mod(x[1], y));"
"}";

const char *mod__const_vec3__const_vec3__const_float__body = 
"vec3 mod(vec3 x, float y)"
"{"
"   return vec3(mod(x[0], y), mod(x[1], y), mod(x[2], y));"
"}";

const char *mod__const_vec4__const_vec4__const_float__body = 
"vec4 mod(vec4 x, float y)"
"{"
"   return vec4(mod(x[0], y), mod(x[1], y), mod(x[2], y), mod(x[3], y));"
"}";

const char *mod__const_vec2__const_vec2__const_vec2__body = 
"vec2 mod(vec2 x, vec2 y)"
"{"
"   return vec2(mod(x[0], y[0]), mod(x[1], y[1]));"
"}";

const char *mod__const_vec3__const_vec3__const_vec3__body = 
"vec3 mod(vec3 x, vec3 y)"
"{"
"   return vec3(mod(x[0], y[0]), mod(x[1], y[1]), mod(x[2], y[2]));"
"}";

const char *mod__const_vec4__const_vec4__const_vec4__body = 
"vec4 mod(vec4 x, vec4 y)"
"{"
"   return vec4(mod(x[0], y[0]), mod(x[1], y[1]), mod(x[2], y[2]), mod(x[3], y[3]));"
"}";

const char *min__const_float__const_float__const_float__body = 
"float min(float x, float y)"
"{"
"   return $$min(x, y);"
"}";

const char *min__const_vec2__const_vec2__const_vec2__body = 
"vec2 min(vec2 x, vec2 y)"
"{"
"   return vec2($$min(x[0], y[0]), $$min(x[1], y[1]));"
"}";

const char *min__const_vec3__const_vec3__const_vec3__body = 
"vec3 min(vec3 x, vec3 y)"
"{"
"   return vec3($$min(x[0], y[0]), $$min(x[1], y[1]), $$min(x[2], y[2]));"
"}";

const char *min__const_vec4__const_vec4__const_vec4__body = 
"vec4 min(vec4 x, vec4 y)"
"{"
"   return vec4($$min(x[0], y[0]), $$min(x[1], y[1]), $$min(x[2], y[2]), $$min(x[3], y[3]));"
"}";

const char *min__const_vec2__const_vec2__const_float__body = 
"vec2 min(vec2 x, float y)"
"{"
"   return vec2($$min(x[0], y), $$min(x[1], y));"
"}";

const char *min__const_vec3__const_vec3__const_float__body = 
"vec3 min(vec3 x, float y)"
"{"
"   return vec3($$min(x[0], y), $$min(x[1], y), $$min(x[2], y));"
"}";

const char *min__const_vec4__const_vec4__const_float__body = 
"vec4 min(vec4 x, float y)"
"{"
"   return vec4($$min(x[0], y), $$min(x[1], y), $$min(x[2], y), $$min(x[3], y));"
"}";

const char *max__const_float__const_float__const_float__body = 
"float max(float x, float y)"
"{"
"   return $$max(x, y);"
"}";

const char *max__const_vec2__const_vec2__const_vec2__body = 
"vec2 max(vec2 x, vec2 y)"
"{"
"   return vec2($$max(x[0], y[0]), $$max(x[1], y[1]));"
"}";

const char *max__const_vec3__const_vec3__const_vec3__body = 
"vec3 max(vec3 x, vec3 y)"
"{"
"   return vec3($$max(x[0], y[0]), $$max(x[1], y[1]), $$max(x[2], y[2]));"
"}";

const char *max__const_vec4__const_vec4__const_vec4__body = 
"vec4 max(vec4 x, vec4 y)"
"{"
"   return vec4($$max(x[0], y[0]), $$max(x[1], y[1]), $$max(x[2], y[2]), $$max(x[3], y[3]));"
"}";

const char *max__const_vec2__const_vec2__const_float__body = 
"vec2 max(vec2 x, float y)"
"{"
"   return vec2($$max(x[0], y), $$max(x[1], y));"
"}";

const char *max__const_vec3__const_vec3__const_float__body = 
"vec3 max(vec3 x, float y)"
"{"
"   return vec3($$max(x[0], y), $$max(x[1], y), $$max(x[2], y));"
"}";

const char *max__const_vec4__const_vec4__const_float__body = 
"vec4 max(vec4 x, float y)"
"{"
"   return vec4($$max(x[0], y), $$max(x[1], y), $$max(x[2], y), $$max(x[3], y));"
"}";

const char *clamp__const_float__const_float__const_float__const_float__body = 
"float clamp(float x, float minVal, float maxVal)"
"{"
"   return $$min($$max(x, minVal), maxVal);"
"}";

const char *clamp__const_vec2__const_vec2__const_vec2__const_vec2__body = 
"vec2 clamp(vec2 x, vec2 minVal, vec2 maxVal)"
"{"
"   return vec2("
"      $$min($$max(x[0], minVal[0]), maxVal[0]),"
"      $$min($$max(x[1], minVal[1]), maxVal[1]));"
"}";

const char *clamp__const_vec3__const_vec3__const_vec3__const_vec3__body = 
"vec3 clamp(vec3 x, vec3 minVal, vec3 maxVal)"
"{"
"   return vec3("
"      $$min($$max(x[0], minVal[0]), maxVal[0]),"
"      $$min($$max(x[1], minVal[1]), maxVal[1]),"
"      $$min($$max(x[2], minVal[2]), maxVal[2]));"
"}";

const char *clamp__const_vec4__const_vec4__const_vec4__const_vec4__body = 
"vec4 clamp(vec4 x, vec4 minVal, vec4 maxVal)"
"{"
"   return vec4("
"      $$min($$max(x[0], minVal[0]), maxVal[0]),"
"      $$min($$max(x[1], minVal[1]), maxVal[1]),"
"      $$min($$max(x[2], minVal[2]), maxVal[2]),"
"      $$min($$max(x[3], minVal[3]), maxVal[3]));"
"}";

const char *clamp__const_vec2__const_vec2__const_float__const_float__body = 
"vec2 clamp(vec2 x, float minVal, float maxVal)"
"{"
"   return vec2("
"      $$min($$max(x[0], minVal), maxVal),"
"      $$min($$max(x[1], minVal), maxVal));"
"}";

const char *clamp__const_vec3__const_vec3__const_float__const_float__body = 
"vec3 clamp(vec3 x, float minVal, float maxVal)"
"{"
"   return vec3("
"      $$min($$max(x[0], minVal), maxVal),"
"      $$min($$max(x[1], minVal), maxVal),"
"      $$min($$max(x[2], minVal), maxVal));"
"}";

const char *clamp__const_vec4__const_vec4__const_float__const_float__body = 
"vec4 clamp(vec4 x, float minVal, float maxVal)"
"{"
"   return vec4("
"      $$min($$max(x[0], minVal), maxVal),"
"      $$min($$max(x[1], minVal), maxVal),"
"      $$min($$max(x[2], minVal), maxVal),"
"      $$min($$max(x[3], minVal), maxVal));"
"}";

const char *mix__const_float__const_float__const_float__const_float__body = 
"float mix(float x, float y, float a)"
"{"
"   return x * (1.0 - a) + y * a;"
"}";

const char *mix__const_vec2__const_vec2__const_vec2__const_vec2__body = 
"vec2 mix(vec2 x, vec2 y, vec2 a)"
"{"
"   return vec2(mix(x[0], y[0], a[0]), mix(x[1], y[1], a[1]));"
"}";

const char *mix__const_vec3__const_vec3__const_vec3__const_vec3__body = 
"vec3 mix(vec3 x, vec3 y, vec3 a)"
"{"
"   return vec3(mix(x[0], y[0], a[0]), mix(x[1], y[1], a[1]), mix(x[2], y[2], a[2]));"
"}";

const char *mix__const_vec4__const_vec4__const_vec4__const_vec4__body = 
"vec4 mix(vec4 x, vec4 y, vec4 a)"
"{"
"   return vec4(mix(x[0], y[0], a[0]), mix(x[1], y[1], a[1]), mix(x[2], y[2], a[2]), mix(x[3], y[3], a[3]));"
"}";

const char *mix__const_vec2__const_vec2__const_vec2__const_float__body = 
"vec2 mix(vec2 x, vec2 y, float a)"
"{"
"   return vec2(mix(x[0], y[0], a), mix(x[1], y[1], a));"
"}";

const char *mix__const_vec3__const_vec3__const_vec3__const_float__body = 
"vec3 mix(vec3 x, vec3 y, float a)"
"{"
"   return vec3(mix(x[0], y[0], a), mix(x[1], y[1], a), mix(x[2], y[2], a));"
"}";

const char *mix__const_vec4__const_vec4__const_vec4__const_float__body = 
"vec4 mix(vec4 x, vec4 y, float a)"
"{"
"   return vec4(mix(x[0], y[0], a), mix(x[1], y[1], a), mix(x[2], y[2], a), mix(x[3], y[3], a));"
"}";

const char *step__const_float__const_float__const_float__body = 
"float step(float edge, float x)"
"{"
"   if (x < edge)"
"      return 0.0;"
"   else"
"      return 1.0;"
"}";

const char *step__const_vec2__const_vec2__const_vec2__body = 
"vec2 step(vec2 edge, vec2 x)"
"{"
"   return vec2("
"      step(edge[0], x[0]),"
"      step(edge[1], x[1]));"
"}";

const char *step__const_vec3__const_vec3__const_vec3__body = 
"vec3 step(vec3 edge, vec3 x)"
"{"
"   return vec3("
"      step(edge[0], x[0]),"
"      step(edge[1], x[1]),"
"      step(edge[2], x[2]));"
"}";

const char *step__const_vec4__const_vec4__const_vec4__body = 
"vec4 step(vec4 edge, vec4 x)"
"{"
"   return vec4("
"      step(edge[0], x[0]),"
"      step(edge[1], x[1]),"
"      step(edge[2], x[2]),"
"      step(edge[3], x[3]));"
"}";

const char *step__const_vec2__const_float__const_vec2__body = 
"vec2 step(float edge, vec2 x)"
"{"
"   return vec2("
"      step(edge, x[0]),"
"      step(edge, x[1]));"
"}";

const char *step__const_vec3__const_float__const_vec3__body = 
"vec3 step(float edge, vec3 x)"
"{"
"   return vec3("
"      step(edge, x[0]),"
"      step(edge, x[1]),"
"      step(edge, x[2]));"
"}";

const char *step__const_vec4__const_float__const_vec4__body = 
"vec4 step(float edge, vec4 x)"
"{"
"   return vec4("
"      step(edge, x[0]),"
"      step(edge, x[1]),"
"      step(edge, x[2]),"
"      step(edge, x[3]));"
"}";

const char *smoothstep__const_float__const_float__const_float__const_float__body = 
"float smoothstep(float edge0, float edge1, float x)"
"{"
"   float t;"
"   t = clamp ((x - edge0) / (edge1 - edge0), 0.0, 1.0);"
"   return t * t * (3.0 - 2.0 * t);"
"}";

const char *smoothstep__const_vec2__const_vec2__const_vec2__const_vec2__body = 
"vec2 smoothstep(vec2 edge0, vec2 edge1, vec2 x)"
"{"
"   return vec2("
"      smoothstep(edge0[0], edge1[0], x[0]),"
"      smoothstep(edge0[1], edge1[1], x[1]));"
"}";

const char *smoothstep__const_vec3__const_vec3__const_vec3__const_vec3__body = 
"vec3 smoothstep(vec3 edge0, vec3 edge1, vec3 x)"
"{"
"   return vec3("
"      smoothstep(edge0[0], edge1[0], x[0]),"
"      smoothstep(edge0[1], edge1[1], x[1]),"
"      smoothstep(edge0[2], edge1[2], x[2]));"
"}";

const char *smoothstep__const_vec4__const_vec4__const_vec4__const_vec4__body = 
"vec4 smoothstep(vec4 edge0, vec4 edge1, vec4 x)"
"{"
"   return vec4("
"      smoothstep(edge0[0], edge1[0], x[0]),"
"      smoothstep(edge0[1], edge1[1], x[1]),"
"      smoothstep(edge0[2], edge1[2], x[2]),"
"      smoothstep(edge0[3], edge1[3], x[3]));"
"}";

const char *smoothstep__const_vec2__const_float__const_float__const_vec2__body = 
"vec2 smoothstep(float edge0, float edge1, vec2 x)"
"{"
"   return vec2("
"      smoothstep(edge0, edge1, x[0]),"
"      smoothstep(edge0, edge1, x[1]));"
"}";

const char *smoothstep__const_vec3__const_float__const_float__const_vec3__body = 
"vec3 smoothstep(float edge0, float edge1, vec3 x)"
"{"
"   return vec3("
"      smoothstep(edge0, edge1, x[0]),"
"      smoothstep(edge0, edge1, x[1]),"
"      smoothstep(edge0, edge1, x[2]));"
"}";

const char *smoothstep__const_vec4__const_float__const_float__const_vec4__body = 
"vec4 smoothstep(float edge0, float edge1, vec4 x)"
"{"
"   return vec4("
"      smoothstep(edge0, edge1, x[0]),"
"      smoothstep(edge0, edge1, x[1]),"
"      smoothstep(edge0, edge1, x[2]),"
"      smoothstep(edge0, edge1, x[3]));"
"}";

//
// Geometric Functions
//

const char *dot__const_float__const_float__const_float__body = 
"float dot(float x, float y)"
"{"
"   return x*y;"
"}";

const char *dot__const_float__const_vec2__const_vec2__body = 
"float dot(vec2 x, vec2 y)"
"{"
"   return x[0]*y[0] + x[1]*y[1];"
"}";

const char *dot__const_float__const_vec3__const_vec3__body = 
"float dot(vec3 x, vec3 y)"
"{"
"   return x[0]*y[0] + x[1]*y[1] + x[2]*y[2];"
"}";

const char *dot__const_float__const_vec4__const_vec4__body = 
"float dot(vec4 x, vec4 y)"
"{"
"   return x[0]*y[0] + x[1]*y[1] + x[2]*y[2] + x[3]*y[3];"
"}";

const char *length__const_float__const_float__body = 
"float length(float x)"
"{"
"   return abs(x);"
"}";

const char *length__const_float__const_vec2__body = 
"float length(vec2 x)"
"{"
"   return $$rcp($$rsqrt(dot(x, x)));"
"}";

const char *length__const_float__const_vec3__body = 
"float length(vec3 x)"
"{"
"   return $$rcp($$rsqrt(dot(x, x)));"
"}";

const char *length__const_float__const_vec4__body = 
"float length(vec4 x)"
"{"
"   return $$rcp($$rsqrt(dot(x, x)));"
"}";

const char *distance__const_float__const_float__const_float__body = 
"float distance(float p0, float p1)"
"{"
"   return length(p0 - p1);"
"}";

const char *distance__const_float__const_vec2__const_vec2__body = 
"float distance(vec2 p0, vec2 p1)"
"{"
"   return length(p0 - p1);"
"}";

const char *distance__const_float__const_vec3__const_vec3__body = 
"float distance(vec3 p0, vec3 p1)"
"{"
"   return length(p0 - p1);"
"}";

const char *distance__const_float__const_vec4__const_vec4__body = 
"float distance(vec4 p0, vec4 p1)"
"{"
"   return length(p0 - p1);"
"}";

const char *cross__const_vec3__const_vec3__const_vec3__body = 
"vec3 cross(vec3 x, vec3 y)"
"{"
"   return vec3(x[1]*y[2] - x[2]*y[1], x[2]*y[0] - x[0]*y[2], x[0]*y[1] - x[1]*y[0]);"
"}";

const char *normalize__const_float__const_float__body = 
"float normalize(float x)"
"{"
"   return sign(x);"
"}";

const char *normalize__const_vec2__const_vec2__body = 
"vec2 normalize(vec2 x)"
"{"
"   return x * $$rsqrt(dot(x, x));"
"}";

const char *normalize__const_vec3__const_vec3__body = 
"vec3 normalize(vec3 x)"
"{"
"   return x * $$rsqrt(dot(x, x));"
"}";

const char *normalize__const_vec4__const_vec4__body = 
"vec4 normalize(vec4 x)"
"{"
"   return x * $$rsqrt(dot(x, x));"
"}";

const char *faceforward__const_float__const_float__const_float__const_float__body = 
"float faceforward(float N, float I, float Nref)"
"{"
"   if (dot(Nref, I) < 0.0)"
"      return N;"
"   else"
"      return -N;"
"}";

const char *faceforward__const_vec2__const_vec2__const_vec2__const_vec2__body = 
"vec2 faceforward(vec2 N, vec2 I, vec2 Nref)"
"{"
"   if (dot(Nref, I) < 0.0)"
"      return N;"
"   else"
"      return -N;"
"}";

const char *faceforward__const_vec3__const_vec3__const_vec3__const_vec3__body = 
"vec3 faceforward(vec3 N, vec3 I, vec3 Nref)"
"{"
"   if (dot(Nref, I) < 0.0)"
"      return N;"
"   else"
"      return -N;"
"}";

const char *faceforward__const_vec4__const_vec4__const_vec4__const_vec4__body = 
"vec4 faceforward(vec4 N, vec4 I, vec4 Nref)"
"{"
"   if (dot(Nref, I) < 0.0)"
"      return N;"
"   else"
"      return -N;"
"}";

const char *reflect__const_float__const_float__const_float__body = 
"float reflect(float I, float N)"
"{"
"   return I - 2.0 * dot(N, I) * N;"
"}";

const char *reflect__const_vec2__const_vec2__const_vec2__body = 
"vec2 reflect(vec2 I, vec2 N)"
"{"
"   return I - 2.0 * dot(N, I) * N;"
"}";

const char *reflect__const_vec3__const_vec3__const_vec3__body = 
"vec3 reflect(vec3 I, vec3 N)"
"{"
"   return I - 2.0 * dot(N, I) * N;"
"}";

const char *reflect__const_vec4__const_vec4__const_vec4__body = 
"vec4 reflect(vec4 I, vec4 N)"
"{"
"   return I - 2.0 * dot(N, I) * N;"
"}";

const char *refract__const_float__const_float__const_float__const_float__body = 
"float refract(float I, float N, float eta)"
"{"
"   float d = dot(N, I);"
"   float k = 1.0 - eta * eta * (1.0 - d * d);"
"   if (k < 0.0)"
"      return float(0.0);"
"   else"
"      return eta * I - (eta * d + sqrt(k)) * N;"
"}";

const char *refract__const_vec2__const_vec2__const_vec2__const_float__body = 
"vec2 refract(vec2 I, vec2 N, float eta)"
"{"
"   float d = dot(N, I);"
"   float k = 1.0 - eta * eta * (1.0 - d * d);"
"   if (k < 0.0)"
"      return vec2(0.0);"
"   else"
"      return eta * I - (eta * d + sqrt(k)) * N;"
"}";

const char *refract__const_vec3__const_vec3__const_vec3__const_float__body = 
"vec3 refract(vec3 I, vec3 N, float eta)"
"{"
"   float d = dot(N, I);"
"   float k = 1.0 - eta * eta * (1.0 - d * d);"
"   if (k < 0.0)"
"      return vec3(0.0);"
"   else"
"      return eta * I - (eta * d + sqrt(k)) * N;"
"}";

const char *refract__const_vec4__const_vec4__const_vec4__const_float__body = 
"vec4 refract(vec4 I, vec4 N, float eta)"
"{"
"   float d = dot(N, I);"
"   float k = 1.0 - eta * eta * (1.0 - d * d);"
"   if (k < 0.0)"
"      return vec4(0.0);"
"   else"
"      return eta * I - (eta * d + sqrt(k)) * N;"
"}";

//
// Matrix Functions
//

const char *matrixCompMult__const_mat2__const_mat2__const_mat2__body = 
"mat2 matrixCompMult(mat2 x, mat2 y)"
"{"
"   return mat2(x[0] * y[0], x[1] * y[1]);"
"}";

const char *matrixCompMult__const_mat3__const_mat3__const_mat3__body = 
"mat3 matrixCompMult(mat3 x, mat3 y)"
"{"
"   return mat3(x[0] * y[0], x[1] * y[1], x[2] * y[2]);"
"}";

const char *matrixCompMult__const_mat4__const_mat4__const_mat4__body = 
"mat4 matrixCompMult(mat4 x, mat4 y)"
"{"
"   return mat4(x[0] * y[0], x[1] * y[1], x[2] * y[2], x[3] * y[3]);"
"}";

//
// Vector Relational Functions
//

const char *lessThan__const_bvec2__const_vec2__const_vec2__body = 
"bvec2 lessThan(vec2 x, vec2 y)"
"{"
"   return bvec2(x[0] < y[0], x[1] < y[1]);"
"}";

const char *lessThan__const_bvec3__const_vec3__const_vec3__body = 
"bvec3 lessThan(vec3 x, vec3 y)"
"{"
"   return bvec3(x[0] < y[0], x[1] < y[1], x[2] < y[2]);"
"}";

const char *lessThan__const_bvec4__const_vec4__const_vec4__body = 
"bvec4 lessThan(vec4 x, vec4 y)"
"{"
"   return bvec4(x[0] < y[0], x[1] < y[1], x[2] < y[2], x[3] < y[3]);"
"}";

const char *lessThan__const_bvec2__const_ivec2__const_ivec2__body = 
"bvec2 lessThan(ivec2 x, ivec2 y)"
"{"
"   return bvec2(x[0] < y[0], x[1] < y[1]);"
"}";

const char *lessThan__const_bvec3__const_ivec3__const_ivec3__body = 
"bvec3 lessThan(ivec3 x, ivec3 y)"
"{"
"   return bvec3(x[0] < y[0], x[1] < y[1], x[2] < y[2]);"
"}";

const char *lessThan__const_bvec4__const_ivec4__const_ivec4__body = 
"bvec4 lessThan(ivec4 x, ivec4 y)"
"{"
"   return bvec4(x[0] < y[0], x[1] < y[1], x[2] < y[2], x[3] < y[3]);"
"}";

const char *lessThanEqual__const_bvec2__const_vec2__const_vec2__body = 
"bvec2 lessThanEqual(vec2 x, vec2 y)"
"{"
"   return bvec2(x[0] <= y[0], x[1] <= y[1]);"
"}";

const char *lessThanEqual__const_bvec3__const_vec3__const_vec3__body = 
"bvec3 lessThanEqual(vec3 x, vec3 y)"
"{"
"   return bvec3(x[0] <= y[0], x[1] <= y[1], x[2] <= y[2]);"
"}";

const char *lessThanEqual__const_bvec4__const_vec4__const_vec4__body = 
"bvec4 lessThanEqual(vec4 x, vec4 y)"
"{"
"   return bvec4(x[0] <= y[0], x[1] <= y[1], x[2] <= y[2], x[3] <= y[3]);"
"}";

const char *lessThanEqual__const_bvec2__const_ivec2__const_ivec2__body = 
"bvec2 lessThanEqual(ivec2 x, ivec2 y)"
"{"
"   return bvec2(x[0] <= y[0], x[1] <= y[1]);"
"}";

const char *lessThanEqual__const_bvec3__const_ivec3__const_ivec3__body = 
"bvec3 lessThanEqual(ivec3 x, ivec3 y)"
"{"
"   return bvec3(x[0] <= y[0], x[1] <= y[1], x[2] <= y[2]);"
"}";

const char *lessThanEqual__const_bvec4__const_ivec4__const_ivec4__body = 
"bvec4 lessThanEqual(ivec4 x, ivec4 y)"
"{"
"   return bvec4(x[0] <= y[0], x[1] <= y[1], x[2] <= y[2], x[3] <= y[3]);"
"}";

const char *greaterThan__const_bvec2__const_vec2__const_vec2__body = 
"bvec2 greaterThan(vec2 x, vec2 y)"
"{"
"   return bvec2(x[0] > y[0], x[1] > y[1]);"
"}";

const char *greaterThan__const_bvec3__const_vec3__const_vec3__body = 
"bvec3 greaterThan(vec3 x, vec3 y)"
"{"
"   return bvec3(x[0] > y[0], x[1] > y[1], x[2] > y[2]);"
"}";

const char *greaterThan__const_bvec4__const_vec4__const_vec4__body = 
"bvec4 greaterThan(vec4 x, vec4 y)"
"{"
"   return bvec4(x[0] > y[0], x[1] > y[1], x[2] > y[2], x[3] > y[3]);"
"}";

const char *greaterThan__const_bvec2__const_ivec2__const_ivec2__body = 
"bvec2 greaterThan(ivec2 x, ivec2 y)"
"{"
"   return bvec2(x[0] > y[0], x[1] > y[1]);"
"}";

const char *greaterThan__const_bvec3__const_ivec3__const_ivec3__body = 
"bvec3 greaterThan(ivec3 x, ivec3 y)"
"{"
"   return bvec3(x[0] > y[0], x[1] > y[1], x[2] > y[2]);"
"}";

const char *greaterThan__const_bvec4__const_ivec4__const_ivec4__body = 
"bvec4 greaterThan(ivec4 x, ivec4 y)"
"{"
"   return bvec4(x[0] > y[0], x[1] > y[1], x[2] > y[2], x[3] > y[3]);"
"}";

const char *greaterThanEqual__const_bvec2__const_vec2__const_vec2__body = 
"bvec2 greaterThanEqual(vec2 x, vec2 y)"
"{"
"   return bvec2(x[0] >= y[0], x[1] >= y[1]);"
"}";

const char *greaterThanEqual__const_bvec3__const_vec3__const_vec3__body = 
"bvec3 greaterThanEqual(vec3 x, vec3 y)"
"{"
"   return bvec3(x[0] >= y[0], x[1] >= y[1], x[2] >= y[2]);"
"}";

const char *greaterThanEqual__const_bvec4__const_vec4__const_vec4__body = 
"bvec4 greaterThanEqual(vec4 x, vec4 y)"
"{"
"   return bvec4(x[0] >= y[0], x[1] >= y[1], x[2] >= y[2], x[3] >= y[3]);"
"}";

const char *greaterThanEqual__const_bvec2__const_ivec2__const_ivec2__body = 
"bvec2 greaterThanEqual(ivec2 x, ivec2 y)"
"{"
"   return bvec2(x[0] >= y[0], x[1] >= y[1]);"
"}";

const char *greaterThanEqual__const_bvec3__const_ivec3__const_ivec3__body = 
"bvec3 greaterThanEqual(ivec3 x, ivec3 y)"
"{"
"   return bvec3(x[0] >= y[0], x[1] >= y[1], x[2] >= y[2]);"
"}";

const char *greaterThanEqual__const_bvec4__const_ivec4__const_ivec4__body = 
"bvec4 greaterThanEqual(ivec4 x, ivec4 y)"
"{"
"   return bvec4(x[0] >= y[0], x[1] >= y[1], x[2] >= y[2], x[3] >= y[3]);"
"}";

const char *equal__const_bvec2__const_vec2__const_vec2__body = 
"bvec2 equal(vec2 x, vec2 y)"
"{"
"   return bvec2(x[0] == y[0], x[1] == y[1]);"
"}";

const char *equal__const_bvec3__const_vec3__const_vec3__body = 
"bvec3 equal(vec3 x, vec3 y)"
"{"
"   return bvec3(x[0] == y[0], x[1] == y[1], x[2] == y[2]);"
"}";

const char *equal__const_bvec4__const_vec4__const_vec4__body = 
"bvec4 equal(vec4 x, vec4 y)"
"{"
"   return bvec4(x[0] == y[0], x[1] == y[1], x[2] == y[2], x[3] == y[3]);"
"}";

const char *equal__const_bvec2__const_ivec2__const_ivec2__body = 
"bvec2 equal(ivec2 x, ivec2 y)"
"{"
"   return bvec2(x[0] == y[0], x[1] == y[1]);"
"}";

const char *equal__const_bvec3__const_ivec3__const_ivec3__body = 
"bvec3 equal(ivec3 x, ivec3 y)"
"{"
"   return bvec3(x[0] == y[0], x[1] == y[1], x[2] == y[2]);"
"}";

const char *equal__const_bvec4__const_ivec4__const_ivec4__body = 
"bvec4 equal(ivec4 x, ivec4 y)"
"{"
"   return bvec4(x[0] == y[0], x[1] == y[1], x[2] == y[2], x[3] == y[3]);"
"}";

const char *equal__const_bvec2__const_bvec2__const_bvec2__body = 
"bvec2 equal(bvec2 x, bvec2 y)"
"{"
"   return bvec2(x[0] == y[0], x[1] == y[1]);"
"}";

const char *equal__const_bvec3__const_bvec3__const_bvec3__body = 
"bvec3 equal(bvec3 x, bvec3 y)"
"{"
"   return bvec3(x[0] == y[0], x[1] == y[1], x[2] == y[2]);"
"}";

const char *equal__const_bvec4__const_bvec4__const_bvec4__body = 
"bvec4 equal(bvec4 x, bvec4 y)"
"{"
"   return bvec4(x[0] == y[0], x[1] == y[1], x[2] == y[2], x[3] == y[3]);"
"}";

const char *notEqual__const_bvec2__const_vec2__const_vec2__body = 
"bvec2 notEqual(vec2 x, vec2 y)"
"{"
"   return bvec2(x[0] != y[0], x[1] != y[1]);"
"}";

const char *notEqual__const_bvec3__const_vec3__const_vec3__body = 
"bvec3 notEqual(vec3 x, vec3 y)"
"{"
"   return bvec3(x[0] != y[0], x[1] != y[1], x[2] != y[2]);"
"}";

const char *notEqual__const_bvec4__const_vec4__const_vec4__body = 
"bvec4 notEqual(vec4 x, vec4 y)"
"{"
"   return bvec4(x[0] != y[0], x[1] != y[1], x[2] != y[2], x[3] != y[3]);"
"}";

const char *notEqual__const_bvec2__const_ivec2__const_ivec2__body = 
"bvec2 notEqual(ivec2 x, ivec2 y)"
"{"
"   return bvec2(x[0] != y[0], x[1] != y[1]);"
"}";

const char *notEqual__const_bvec3__const_ivec3__const_ivec3__body = 
"bvec3 notEqual(ivec3 x, ivec3 y)"
"{"
"   return bvec3(x[0] != y[0], x[1] != y[1], x[2] != y[2]);"
"}";

const char *notEqual__const_bvec4__const_ivec4__const_ivec4__body = 
"bvec4 notEqual(ivec4 x, ivec4 y)"
"{"
"   return bvec4(x[0] != y[0], x[1] != y[1], x[2] != y[2], x[3] != y[3]);"
"}";

const char *notEqual__const_bvec2__const_bvec2__const_bvec2__body = 
"bvec2 notEqual(bvec2 x, bvec2 y)"
"{"
"   return bvec2(x[0] != y[0], x[1] != y[1]);"
"}";

const char *notEqual__const_bvec3__const_bvec3__const_bvec3__body = 
"bvec3 notEqual(bvec3 x, bvec3 y)"
"{"
"   return bvec3(x[0] != y[0], x[1] != y[1], x[2] != y[2]);"
"}";

const char *notEqual__const_bvec4__const_bvec4__const_bvec4__body = 
"bvec4 notEqual(bvec4 x, bvec4 y)"
"{"
"   return bvec4(x[0] != y[0], x[1] != y[1], x[2] != y[2], x[3] != y[3]);"
"}";

const char *any__const_bool__const_bvec2__body = 
"bool any(bvec2 x)"
"{"
"   return x[0] || x[1];"
"}";

const char *any__const_bool__const_bvec3__body = 
"bool any(bvec3 x)"
"{"
"   return x[0] || x[1] || x[2];"
"}";

const char *any__const_bool__const_bvec4__body = 
"bool any(bvec4 x)"
"{"
"   return x[0] || x[1] || x[2] || x[3];"
"}";

const char *all__const_bool__const_bvec2__body = 
"bool all(bvec2 x)"
"{"
"   return x[0] && x[1];"
"}";

const char *all__const_bool__const_bvec3__body = 
"bool all(bvec3 x)"
"{"
"   return x[0] && x[1] && x[2];"
"}";

const char *all__const_bool__const_bvec4__body = 
"bool all(bvec4 x)"
"{"
"   return x[0] && x[1] && x[2] && x[3];"
"}";

const char *not__const_bvec2__const_bvec2__body = 
"bvec2 not(bvec2 x)"
"{"
"   return bvec2(!x[0], !x[1]);"
"}";

const char *not__const_bvec3__const_bvec3__body = 
"bvec3 not(bvec3 x)"
"{"
"   return bvec3(!x[0], !x[1], !x[2]);"
"}";

const char *not__const_bvec4__const_bvec4__body = 
"bvec4 not(bvec4 x)"
"{"
"   return bvec4(!x[0], !x[1], !x[2], !x[3]);"
"}";

//
// Texture Lookup Functions
//

const char* sampler_functions =
"vec4 texture2D(sampler2D sampler, vec2 coord, float bias)"
"{"
"   return $$texture_2d_bias(sampler, coord.st, bias);"
"}"

"vec4 texture2D(sampler2D sampler, vec2 coord)"
"{"
"   return $$texture_2d_bias(sampler, coord.st, 0.0);"
"}"

"vec4 texture2DProj(sampler2D sampler, vec3 coord, float bias)"
"{"
"   return $$texture_2d_bias(sampler, coord.st /= coord[2], bias);"
"}"

"vec4 texture2DProj(sampler2D sampler, vec3 coord)"
"{"
"   return $$texture_2d_bias(sampler, coord.st /= coord[2], 0.0);"
"}"

"vec4 texture2DProj(sampler2D sampler, vec4 coord, float bias)"
"{"
"   return $$texture_2d_bias(sampler, coord.st /= coord[3], bias);"
"}"

"vec4 texture2DProj(sampler2D sampler, vec4 coord)"
"{"
"   return $$texture_2d_bias(sampler, coord.st /= coord[3], 0.0);"
"}"

//"vec4 texture2DLod(sampler2D sampler, vec2 coord, float lod)"
//"{"
//"   return $$texture_2d_lod(sampler, coord.st, lod);"
//"}"
//
//"vec4 texture2DProjLod(sampler2D sampler, vec3 coord, float lod)"
//"{"
//"   return $$texture_2d_lod(sampler, coord.st /= coord[2], lod);"
//"}"
//
//"vec4 texture2DProjLod(sampler2D sampler, vec4 coord, float lod)"
//"{"
//"   return $$texture_2d_lod(sampler, coord.st /= coord[3], lod);"
//"}"

"vec4 textureCube(samplerCube sampler, vec3 coord, float bias)"
"{"
"   vec3 abs_coord = abs(coord);"
"   float maxabs = max(abs_coord[0], max(abs_coord[1], abs_coord[2]));"
"   vec3 norm_coord = coord / maxabs;"
"   return $$texture_cube_bias(sampler, norm_coord, bias);"
"}"

"vec4 textureCube(samplerCube sampler, vec3 coord)"
"{"
"   return textureCube(sampler, coord, 0.0);"
"}"

//"vec4 textureCubeLod(samplerCube sampler, vec3 coord, float lod)"
//"{"
//"   return $$texture_cube_lod(sampler, coord.stp, lod);"
//"}"

"\n"
;