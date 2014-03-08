// generated 2010-09-16 22:10:01
/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "middleware/khronos/vg/2708/vg_tess_stroke_shader_4.h"
#include <assert.h>
#ifdef SIMPENROSE
#include <stddef.h>
#include "v3d/verification/tools/2760sim/simpenrose.h"

static unsigned int const annotations_0[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_OPEN, 0x00000000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_27[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0xffffffff,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_REGION_SET, 0x00000016,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_36[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0xfffffffe,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_REGION_SET, 0x00000002,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_513[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000300,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_514[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000300,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_531[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000f0f0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_532[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_533[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_577[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000054,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_586[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000fffc,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_588[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000fffc,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_599[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00004000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_602[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00004000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_615[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000fcfc,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_622[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000030,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_623[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000300,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_627[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000fffe,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_632[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000c00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_633[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_634[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00003030,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_669[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_698[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_701[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_708[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000c00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_713[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00003030,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_715[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000303,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_728[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000cc00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_740[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_743[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_750[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000c00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_755[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00003030,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_757[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000303,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_770[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000cc00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_782[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_785[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_792[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000c00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_797[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00003030,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_799[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000303,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_812[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000cc00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_824[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_827[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_834[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000c00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_839[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00003030,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_841[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000303,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_854[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000cc00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_866[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_869[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_876[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000c00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_881[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00003030,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_883[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000303,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_896[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000cc00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_908[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_911[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_918[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000c00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_923[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00003030,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_925[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000303,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_938[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000cc00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_950[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_953[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_960[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000c00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_965[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00003030,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_967[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000303,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_980[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000cc00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_992[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_995[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1002[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000c00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1003[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1129[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_VERTS, 0x00000000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1130[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_VERTS, 0x00000002,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1131[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_VERTS, 0x00000004,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_TRIS_ADD, 0x00000000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1148[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00008000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1170[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000002,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1172[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000006,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1174[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000000e,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1176[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000001e,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1178[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000003e,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1191[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000fffe,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1221[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00005555,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1228[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_VERTS, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1229[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_VERTS, 0x00000003,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1231[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_VERTS, 0x00000005,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_TRIS_ADD, 0x00000000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1238[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00008000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1271[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0x00000004,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1301[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0x00000000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1358[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1456[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1536[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1584[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0x00000002,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1625[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0x00000002,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1857[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00008000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_1870[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00008000,
   SIMPENROSE_SHADER_ANNOTATION_END};

unsigned int const *const vg_tess_stroke_shader_annotations_array[] = {
annotations_0,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_27,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_36,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_513,
annotations_514,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_531,
annotations_532,
annotations_533,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_577,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_586,
NULL,
annotations_588,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_599,
NULL,
NULL,
annotations_602,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_615,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_622,
annotations_623,
NULL,
NULL,
NULL,
annotations_627,
NULL,
NULL,
NULL,
NULL,
annotations_632,
annotations_633,
annotations_634,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_669,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_698,
NULL,
NULL,
annotations_701,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_708,
NULL,
NULL,
NULL,
NULL,
annotations_713,
NULL,
annotations_715,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_728,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_740,
NULL,
NULL,
annotations_743,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_750,
NULL,
NULL,
NULL,
NULL,
annotations_755,
NULL,
annotations_757,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_770,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_782,
NULL,
NULL,
annotations_785,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_792,
NULL,
NULL,
NULL,
NULL,
annotations_797,
NULL,
annotations_799,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_812,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_824,
NULL,
NULL,
annotations_827,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_834,
NULL,
NULL,
NULL,
NULL,
annotations_839,
NULL,
annotations_841,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_854,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_866,
NULL,
NULL,
annotations_869,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_876,
NULL,
NULL,
NULL,
NULL,
annotations_881,
NULL,
annotations_883,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_896,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_908,
NULL,
NULL,
annotations_911,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_918,
NULL,
NULL,
NULL,
NULL,
annotations_923,
NULL,
annotations_925,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_938,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_950,
NULL,
NULL,
annotations_953,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_960,
NULL,
NULL,
NULL,
NULL,
annotations_965,
NULL,
annotations_967,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_980,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_992,
NULL,
NULL,
annotations_995,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1002,
annotations_1003,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1129,
annotations_1130,
annotations_1131,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1148,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1170,
NULL,
annotations_1172,
NULL,
annotations_1174,
NULL,
annotations_1176,
NULL,
annotations_1178,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1191,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1221,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1228,
annotations_1229,
NULL,
annotations_1231,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1238,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1271,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1301,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1358,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1456,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1536,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1584,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1625,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1857,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_1870,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
};
#endif

static unsigned int const array[] = {
// ::vg_tess_stroke_shader
/* [0x00000000] */ 0xf59e6fc0, 0x1002581e, // mov  r0, qpu_num ; mov  ra_scalar, 0  @geomd_open
/* [0x00000008] */ 0x949c31ff, 0xd0024862, // and  r1, r0, 3   ; mov  r2, 3
/* [0x00000010] */ 0xce9c21d7, 0xd0024822, // shr  r0, r0, 2   ; v8adds  r2, r2, 2
/* [0x00000018] */ 0x519c43c2, 0xd0024860, // shl  r1, r1, 4   ; mul24  r0, r0, r2
/* [0x00000020] */ 0xec9e7040, 0x10024800, // add  r0, r0, r1  ; mov  rb_scalar, 0
/* [0x00000028] */ 0x00101a00, 0xe0020867, // mov  r1, vpm_setup(1, 1, h32(0))
/* [0x00000030] */ 0x0d989dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_chunks_vpm_setup
/* [0x00000038] */ 0x0c9e7200, 0x100407a7, // add.ifz  ra_scalar, r1, r0
/* [0x00000040] */ 0x0c9e7200, 0x10020c67, // add  vr_setup, r1, r0
/* [0x00000048] */ 0x8cc27236, 0x100248e1, // add  r3, r1, r0 ; mov  r1, vpm
/* [0x00000050] */ 0x00006000, 0xe20229e7, // mov.setf  -, elem_range(rsi_alloc_p, 2)
/* [0x00000058] */ 0x809fd009, 0xd000d9de, // nop ; mov.ifnz  ra_scalar, r1 >> rsi_alloc_p
/* [0x00000060] */ 0x80010000, 0xe0020867, // mov  r1, vdr_setup_0(1, 16, dma_h32(0, 0), 0, 8)
/* [0x00000068] */ 0x119c41c0, 0xd00208a7, // shl  r2, r0, 4
/* [0x00000070] */ 0x0d988dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_chunks_vdr_setup_0
/* [0x00000078] */ 0x0c9e7280, 0x100407a7, // add.ifz  ra_scalar, r1, r2
/* [0x00000080] */ 0x0c9e7280, 0x10020c67, // add  vr_setup, r1, r2
/* [0x00000088] */ 0x15827d80, 0x10020ca7, // mov  vr_addr, unif
/* [0x00000090] */ 0x00101a02, 0xe0020867, // mov  r1, vpm_setup(1, 1, h32(2))
/* [0x00000098] */ 0x0d98adc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_out_vpm_setup
/* [0x000000a0] */ 0x0c9e7200, 0x100407a7, // add.ifz  ra_scalar, r1, r0
/* [0x000000a8] */ 0x80904100, 0xe0020867, // mov  r1, vdw_setup_0(1, 16, dma_h32(2, 0))
/* [0x000000b0] */ 0x119c71c0, 0xd00208a7, // shl  r2, r0, 7
/* [0x000000b8] */ 0x0d98cdc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_out_vdw_setup_0
/* [0x000000c0] */ 0x0c9e7280, 0x100407a7, // add.ifz  ra_scalar, r1, r2
/* [0x000000c8] */ 0x15ca7d80, 0x100009e7, // mov  -, vr_wait
/* [0x000000d0] */ 0x159e76c0, 0x10020c67, // mov  vr_setup, r3
/* [0x000000d8] */ 0x15c27d80, 0x10020827, // mov  r0, vpm  @geomd_i(geomd_i_clip_inner) @geomd_region_set_a(11)
/* [0x000000e0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000000e8] */ 0x157a7d80, 0x100208a7, // mov r2, ra_scalar
/* [0x000000f0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000000f8] */ 0x809f8012, 0xd00049e5, // nop ; mov r5rep, r2 << rsi_chunks_vdr_setup_0
/* [0x00000100] */ 0x959e7b6d, 0x10024c61, // mov vr_setup, r5 ; mov r1, r5
/* [0x00000108] */ 0x15827d80, 0x10020ca7, // mov vr_addr, unif
/* [0x00000110] */ 0x15ca7d80, 0x100009e7, // mov  -, vr_wait
/* [0x00000118] */ 0x159e76c0, 0x10020c67, // mov  vr_setup, r3
/* [0x00000120] */ 0x95c27db6, 0x100240e2, // mov ra_scalar3, vpm ; mov r2, vpm  @geomd_i(geomd_i_clip_outer) @geomd_region_set_a(rs3i_clip_outer)
/* [0x00000128] */ 0xbf800000, 0xe0021967, // mov r5rep, f(-1.0)
/* [0x00000130] */ 0x0d98bdc0, 0xd00229e7, // sub.setf -, elem_num, rsi_m_cos_theta
/* [0x00000138] */ 0x209f4015, 0xd00099de, // nop ; fmul.ifz (ra_scalar >> rs3i_cos_theta) << rsi_m_cos_theta, r2, r5
/* [0x00000140] */ 0x0d98bdc0, 0xd00229e7, // sub.setf -, elem_num, rbsi_m_cos_theta_inter
/* [0x00000148] */ 0x209f3015, 0xd00089c0, // nop ; fmul.ifz (rb_scalar >> rs3i_cos_theta_inter) << rbsi_m_cos_theta_inter, r2, r5
/* [0x00000150] */ 0x809f3012, 0xd00049e5, // nop ; mov r5rep, r2 << rs3i_bitfield
/* [0x00000158] */ 0x0e9cabc0, 0xd0021967, // shr r5rep, r5, 10
/* [0x00000160] */ 0x149cfbc0, 0xd0021967, // and r5rep, r5, 0xf
/* [0x00000168] */ 0x0d982dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_dash_phase_i
/* [0x00000170] */ 0x159e7b40, 0x10040067, // mov.ifz ra_scalar2, r5
/* [0x00000178] */ 0x809f1012, 0xd00049e5, // nop ; mov r5rep, r2 << rs3i_initial_dash_phase
/* [0x00000180] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_dash_phase
/* [0x00000188] */ 0x159e7b40, 0x10040067, // mov.ifz ra_scalar2, r5
/* [0x00000190] */ 0x0d98fdc0, 0xd00229e7, // sub.setf -, elem_num, rsi_dash_clerp_lbl
/* [0x00000198] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:dash_clerp_not_dashing
/* [0x000001a0] */ 0x809f3012, 0xd00049e5, // nop ; mov r5rep, r2 << rs3i_bitfield
/* [0x000001a8] */ 0x149ccbc0, 0xd0021967, // and r5rep, r5, 0xc
/* [0x000001b0] */ 0x0d9c4bc0, 0xd00229e7, // sub.setf -, r5, 4
/* [0x000001b8] */ 0xdeadbeef, 0xe0061967, // mov.ifnz r5rep, a:flush_join_q_bevel
/* [0x000001c0] */ 0xdeadbeef, 0xe0081967, // mov.ifn r5rep, a:flush_join_q_miter
/* [0x000001c8] */ 0xdeadbeef, 0xe0041967, // mov.ifz r5rep, a:flush_join_q_round
/* [0x000001d0] */ 0x0d985dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_flush_join_q_lbl
/* [0x000001d8] */ 0x159e7b40, 0x10040067, // mov.ifz ra_scalar2, r5
/* [0x000001e0] */ 0x809f3012, 0xd00049e5, // nop ; mov r5rep, r2 << rs3i_bitfield
/* [0x000001e8] */ 0x149c3bc0, 0xd0021967, // and r5rep, r5, 0x3
/* [0x000001f0] */ 0x0d9c1bc0, 0xd00229e7, // sub.setf -, r5, 1
/* [0x000001f8] */ 0xdeadbeef, 0xe0061967, // mov.ifnz r5rep, a:flush_cap_q_square
/* [0x00000200] */ 0xdeadbeef, 0xe0081967, // mov.ifn r5rep, a:flush_cap_q_butt
/* [0x00000208] */ 0xdeadbeef, 0xe0041967, // mov.ifz r5rep, a:flush_cap_q_round
/* [0x00000210] */ 0x0d984dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_flush_cap_q_lbl
/* [0x00000218] */ 0x159e7b40, 0x10040067, // mov.ifz ra_scalar2, r5
/* [0x00000220] */ 0x809f3012, 0xd00049e5, // nop ; mov r5rep, r2 << rs3i_bitfield
/* [0x00000228] */ 0x00000010, 0xe00208a7, // mov r2, 0x10
/* [0x00000230] */ 0x149e7a80, 0x100229e7, // and.setf -, r5, r2
/* [0x00000238] */ 0x00000040, 0xf00809e7, // brr.allz -, r:1f
/* [0x00000240] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000248] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000250] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000258] */ 0x159e7240, 0x10020c67, // mov vr_setup, r1
/* [0x00000260] */ 0x15827d80, 0x10020ca7, // mov vr_addr, unif
/* [0x00000268] */ 0x15ca7d80, 0x100009e7, // mov  -, vr_wait
/* [0x00000270] */ 0x159e76c0, 0x10020c67, // mov  vr_setup, r3
/* [0x00000278] */ 0x15c27d80, 0x100201e7, // mov ra_dash_pattern, vpm
/* [0x00000280] */ 0x0d98fdc0, 0xd00229e7, // sub.setf -, elem_num, rsi_dash_clerp_lbl
/* [0x00000288] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:dash_clerp_dashing
/* [0x00000290] */ 0xffffffff, 0xe0021a27, // mov  unif_addr_rel, -1
// :1
/* [0x00000298] */ 0xfffffffe, 0xe0021a27, // mov  unif_addr_rel, -2
/* [0x000002a0] */ 0x957a7036, 0x100269e1, // mov.setf  -, r0 ; mov  r1, ra_scalar
/* [0x000002a8] */ 0x0d982dc0, 0xd00429e7, // sub.ifz.setf  -, elem_num, 2
/* [0x000002b0] */ 0x809f8009, 0xd00049e1, // nop ; mov  r1, r1 << rsi_chunks_vdr_setup_0
/* [0x000002b8] */ 0x8d9d03c9, 0xd0025871, // sub  r1, r1, -16 ; mov  vr_setup, r1
/* [0x000002c0] */ 0x00000028, 0xf02809e7, // brr.anyz  -, r:1f
/* [0x000002c8] */ 0x809ff000, 0xd00059f2, // nop ; mov  vr_addr, r0 << 1
/* [0x000002d0] */ 0x0d982dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_chunk_lr
/* [0x000002d8] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:load
/* [0x000002e0] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:load_interp
/* [0x000002e8] */ 0x8d989dc9, 0xd00279f1, // sub.setf  -, elem_num, rsi_chunks_vpm_setup ; mov  vr_setup, r1
/* [0x000002f0] */ 0x809fe000, 0xd00059f2, // nop ; mov  vr_addr, r0 << 2
/* [0x000002f8] */ 0x00100000, 0xe0020867, // mov  r1, 1 << 20
/* [0x00000300] */ 0x0c7a7c40, 0x100407a7, // add.ifz  ra_scalar, ra_scalar, r1
// :1
/* [0x00000308] */ 0x000000f8, 0xe20229e7, // mov.setf  -, elem_range(rsi_tess_i, 5)
/* [0x00000310] */ 0x809f3000, 0xd000d9de, // nop ; mov.ifnz  ra_scalar, r0 >> rsi_tess_i
/* [0x00000318] */ 0x0d986dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_chunks_m_size
/* [0x00000320] */ 0x0d780f80, 0xd00407a7, // sub.ifz  ra_scalar, 0, ra_scalar
/* [0x00000328] */ 0x809fb000, 0xd00059c5, // nop ; mov  ra_user_to_surface_clip_inner, r0 << 5
/* [0x00000330] */ 0x0f0fcccc, 0xe2020267, // mov  ra_subd_flags1, 2 * [-2, -2, -1, -1, 0, 0, 1, 1]
/* [0x00000338] */ 0x0800fc00, 0xe20202e7, // mov  ra_subd_flags2, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1, 1, 1, 1, 1]
/* [0x00000340] */ 0x350efdb7, 0xd0024865, // mov  r1, ra_scalar3 ; fmul  r5rep, ra_scalar3, f(0.5)
/* [0x00000348] */ 0x2d98bded, 0xd00269e5, // sub.setf  -, elem_num, 11 ; fmul  r5rep, r5, r5
/* [0x00000350] */ 0x959f6b49, 0xd0024365, // mov  ra_subd_consts, r5 ; mov  r5rep, r1 << rs3i_oo_flatness_sq
/* [0x00000358] */ 0x029c0f40, 0xd0060367, // fsub.ifnz  ra_subd_consts, f(0.0), r5
/* [0x00000360] */ 0x2edbe6ff, 0xe00217e7, // mov rb_eps, f(EPS)
/* [0x00000368] */ 0x809f1000, 0xd00049e5, // nop ; mov  r5rep, r0 << 15
/* [0x00000370] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00000378] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00000380] */ 0x95ce7dad, 0x100049e1, // mov  -, mutex ; mov  r1, r5
/* [0x00000388] */ 0x159e7000, 0x10021967, // mov  r5rep, r0
/* [0x00000390] */ 0x00210a0f, 0xe0020c67, // mov  vr_setup, vpm_setup(2, 16, h32(15))
/* [0x00000398] */ 0x94c01df6, 0xd0024822, // and  r0, vpm, 1           ; mov  r2, vpm
/* [0x000003a0] */ 0x8d9b0d7f, 0x100269e3, // sub.setf  -, elem_num, r5 ; mov  r3, vpm
/* [0x000003a8] */ 0x169e7040, 0x100429e7, // xor.ifz.setf  -, r0, r1
/* [0x000003b0] */ 0x00000028, 0xf02809e7, // brr.anyz  -, r:1f
/* [0x000003b8] */ 0x8d9a6f9b, 0x100269e0, // sub.setf  -, qpu_num, elem_num ; mov  r0, r3
/* [0x000003c0] */ 0x8d9a7d6d, 0x1002a9e3, // sub.setf  -, elem_num, r5      ; mov.ifz  r3, r5
/* [0x000003c8] */ 0x00100a1f, 0xe0021c67, // mov  vw_setup, vpm_setup(1, 0, h32(31))
/* [0x000003d0] */ 0x8d9a6f89, 0x1002a9e2, // sub.setf  -, qpu_num, elem_num ; mov.ifz  r2, r1
/* [0x000003d8] */ 0xffffffff, 0xe0028823, // mov  r0, -1                    ; mov.ifz  r3, -1
/* [0x000003e0] */ 0x8d9e776d, 0x1002a9e3, // sub.setf  -, r3, r5            ; mov.ifz  r3, r5
/* [0x000003e8] */ 0x00210a0f, 0xe0021c67, // mov  vw_setup, vpm_setup(2, 16, h32(15))
/* [0x000003f0] */ 0x959df4bf, 0xd0028c23, // mov  vpm, r2                   ; mov.ifz  r3, -1
// :1
/* [0x000003f8] */ 0x159e76c0, 0x10020c27, // mov  vpm, r3
/* [0x00000400] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00000408] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00000410] */ 0x00000000, 0xe0020ce7, // mov  mutex, 0
/* [0x00000418] */ 0xed9e7140, 0x10024821, // sub  r0, r0, r5 ; mov  r1, 0
/* [0x00000420] */ 0x0d9e63c0, 0x10021967, // sub  r5rep, r1, qpu_num
/* [0x00000428] */ 0x00004000, 0xe20229e7, // mov.setf  -, elems(rsi_alloc_size)
/* [0x00000430] */ 0x809f0000, 0xd00049e5, // nop ; mov  r5rep, r0 >> r5
/* [0x00000438] */ 0x159e7b40, 0x100629e7, // mov.ifnz.setf  -, r5
/* [0x00000440] */ 0x00000000, 0xe00607a7, // mov.ifnz  ra_scalar, 0
/* [0x00000448] */ 0x00000000, 0xe0021267, // mov  rb_list_tail, 0x0
/* [0x00000450] */ 0x0d982dc0, 0xd00229e7, // sub.setf -, elem_num, 2
/* [0x00000458] */ 0xbfff0000, 0xe0041267, // mov.ifz rb_list_tail, 0xbfff0000
/* [0x00000460] */ 0x0d983dc0, 0xd00229e7, // sub.setf -, elem_num, 3
/* [0x00000468] */ 0x00000012, 0xe0041267, // mov.ifz  rb_list_tail, 18
/* [0x00000470] */ 0x40004000, 0xe20229e7, // mov.setf  -, neg_elems(elems(rsi_alloc_size))
/* [0x00000478] */ 0x000000d4, 0xe0020827, // mov  r0, 212
/* [0x00000480] */ 0x0d7a7c00, 0x100829e7, // sub.ifn.setf  -, ra_scalar, r0 ; nop
/* [0x00000488] */ 0xdeadbeef, 0xe00202a7, // mov  ra_temp, a:1f
/* [0x00000490] */ 0x00003940, 0xf06809e7, // brr.anyn  -, r:alloc_first
/* [0x00000498] */ 0x009e7000, 0x100009e7, // nop
/* [0x000004a0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000004a8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000004b0] */ 0x8c790df6, 0xd0024821, // add  r0, ra_scalar, -16  ; mov  r1, ra_scalar
/* [0x000004b8] */ 0xfff40000, 0xe00208a7, // mov  r2, -(12 << 16)
/* [0x000004c0] */ 0x8c9f6289, 0xd0024871, // add  r1, r1, r2         ; mov  vw_setup, r1 << rsi_out_vpm_setup
/* [0x000004c8] */ 0x809c903f, 0x100049f0, // nop                     ; mov  vpm, rb_list_tail
/* [0x000004d0] */ 0x809f4009, 0xd00049f1, // nop                     ; mov  vw_setup, r1 << rsi_out_vdw_setup_0
/* [0x000004d8] */ 0x00006000, 0xe20229e7, // mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
/* [0x000004e0] */ 0x959f3000, 0xd00647b2, // mov.ifnz  ra_scalar, r0 ; mov  vw_addr, r0 << rsi_alloc_p
// :1
/* [0x000004e8] */ 0x0d983dc0, 0xd00229e7, // sub.setf  -, elem_num, 3
/* [0x000004f0] */ 0x10010101, 0xe0041267, // mov.ifz  rb_list_tail, 0x10010101
/* [0x000004f8] */ 0x0d985dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_chunks_b
/* [0x00000500] */ 0x157a7d80, 0x100429e7, // mov.ifz.setf  -, ra_scalar
/* [0x00000508] */ 0x000000a8, 0xf02809e7, // brr.anyz  -, r:load
/* [0x00000510] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000518] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000520] */ 0x009e7000, 0x100009e7, // nop
// :load_interp
/* [0x00000528] */ 0x157a7d80, 0x10020827, // mov  r0, ra_scalar        ; nop
/* [0x00000530] */ 0x95c86dbf, 0xd00049e1, // mov  -, vr_wait           ; mov  r1, rsi_chunks_m_size
/* [0x00000538] */ 0x8d9b7c40, 0xd00279f1, // sub.setf  -, elem_num, r1 ; mov  vr_setup, r0 << rsi_chunks_vpm_setup
/* [0x00000540] */ 0x00000040, 0xe0020867, // mov  r1, VG_TESS_CHUNK_NORMAL_SIZE
/* [0x00000548] */ 0x8c9f9040, 0xd00469e5, // add.ifz.setf  -, r0, r1   ; mov  r5rep, r0 << rsi_t
/* [0x00000550] */ 0x829e0f6d, 0xd00248e2, // fsub  r3, f(1.0), r5      ; mov  r2, r5
/* [0x00000558] */ 0x35c27d9e, 0x10024723, // mov  ra_chunk, vpm        ; fmul  r3, r3, vpm
/* [0x00000560] */ 0x000000c8, 0xf02809e7, // brr.anyz  -, r:chunk_normal
/* [0x00000568] */ 0x35730d97, 0x18025962, // mov  r5rep, ra_chunk.8a   ; fmul  r2, r2, vpm
/* [0x00000570] */ 0xe19e74c0, 0x1002989e, // fadd  r2, r2, r3          ; mov.ifz  ra_scalar, 0
/* [0x00000578] */ 0x959f8b40, 0xd00269e3, // mov.setf  -, r5           ; mov  r3, r0 << rsi_chunks_vdr_setup_0
/* [0x00000580] */ 0x00000028, 0xe0040867, // mov.ifz  r1, VG_TESS_CHUNK_CUBIC_SIZE
/* [0x00000588] */ 0x8c9e705b, 0x10025831, // add  r0, r0, r1           ; mov  vr_setup, r3
/* [0x00000590] */ 0xdeadbeef, 0xe00202a7, // mov  ra_temp, a:chunk_normal
/* [0x00000598] */ 0x809fc000, 0xd00059f2, // nop                       ; mov  vr_addr, r0 << rsi_chunks_a
/* [0x000005a0] */ 0xdeadbeef, 0xe00402a7, // mov.ifz  ra_temp, a:chunk_cubic
/* [0x000005a8] */ 0x0d9d07c0, 0xd0020c67, // sub  vr_setup, r3, -16    ; nop
/* [0x000005b0] */ 0x00000000, 0xf0f549e7, // bra  -, ra_temp
/* [0x000005b8] */ 0x809fb000, 0xd00059f2, // nop                       ; mov  vr_addr, r0 << rsi_chunks_b
/* [0x000005c0] */ 0x00000070, 0xe20229e7, // mov.setf  -, elems(rsi_chunks_a, rsi_chunks_b, rsi_chunks_m_size)
/* [0x000005c8] */ 0x159e7000, 0x100607a7, // mov.ifnz  ra_scalar, r0   ; nop
// :load
/* [0x000005d0] */ 0x157a7d80, 0x10020827, // mov  r0, ra_scalar        ; nop
/* [0x000005d8] */ 0x95c86dbf, 0xd00049e1, // mov  -, vr_wait           ; mov  r1, rsi_chunks_m_size
/* [0x000005e0] */ 0x8d9b7c40, 0xd00279f1, // sub.setf  -, elem_num, r1 ; mov  vr_setup, r0 << rsi_chunks_vpm_setup
/* [0x000005e8] */ 0x00000040, 0xe0020867, // mov  r1, VG_TESS_CHUNK_NORMAL_SIZE
/* [0x000005f0] */ 0x0c9e7040, 0x100429e7, // add.ifz.setf  -, r0, r1   ; nop
/* [0x000005f8] */ 0x00000030, 0xf02809e7, // brr.anyz  -, r:chunk_normal
/* [0x00000600] */ 0xd69cc03f, 0xd00447a3, // mov.ifz  ra_scalar, 0     ; v8adds  r3, 12, 12
/* [0x00000608] */ 0x95c27db6, 0x10025962, // mov  r5rep, vpm           ; mov  r2, vpm
/* [0x00000610] */ 0x919e7ad2, 0x100279dc, // shl.setf  -, r5, r3       ; mov  ra_chunk, r2
/* [0x00000618] */ 0x00000028, 0xe0040867, // mov.ifz  r1, VG_TESS_CHUNK_CUBIC_SIZE
/* [0x00000620] */ 0x8c9f8040, 0xd0025831, // add  r0, r0, r1           ; mov  vr_setup, r0 << rsi_chunks_vdr_setup_0
/* [0x00000628] */ 0x00000870, 0xf00809e7, // brr.allz  -, r:chunk_cubic
/* [0x00000630] */ 0x809fc000, 0xd00059f2, // nop                       ; mov  vr_addr, r0 << rsi_chunks_a
/* [0x00000638] */ 0x00000050, 0xe20229e7, // mov.setf  -, elems(rsi_chunks_a, rsi_chunks_m_size)
/* [0x00000640] */ 0x159e7000, 0x100607a7, // mov.ifnz  ra_scalar, r0   ; nop
// :chunk_normal
/* [0x00000648] */ 0x809ff012, 0xd00059da, // nop ; mov ra_chunk_normal_locals, r2 << 1
/* [0x00000650] */ 0x15727d80, 0x10020827, // mov r0, ra_chunk
/* [0x00000658] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000660] */ 0x809f1000, 0xd00049e5, // nop ; mov r5rep, r0 << 15
/* [0x00000668] */ 0x159e7b40, 0x10020727, // mov ra_chunk, r5
/* [0x00000670] */ 0x0d98bdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_is_cubic
/* [0x00000678] */ 0x00000000, 0xe0040067, // mov.ifz ra_scalar2, 0
/* [0x00000680] */ 0x15727d80, 0x180229e7, // mov.setf -, ra_chunk.8a
/* [0x00000688] */ 0x000000d8, 0xf00809e7, // brr.allz -, r:chunk_normal_loop_preamble
/* [0x00000690] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000698] */ 0x009e7000, 0x100009e7, // nop
/* [0x000006a0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000006a8] */ 0x00000081, 0xe20229e7, // mov.setf -, elems(rs2i_x, rs2i_y)
/* [0x000006b0] */ 0x156a7d80, 0x10060067, // mov.ifnz ra_scalar2, ra_chunk_normal_locals
/* [0x000006b8] */ 0x0d98edc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_tx
/* [0x000006c0] */ 0x3f800000, 0xe0040067, // mov.ifz ra_scalar2, f(1.0)
/* [0x000006c8] */ 0x0d98fdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_ty
/* [0x000006d0] */ 0x00000000, 0xe0040067, // mov.ifz ra_scalar2, f(0.0)
/* [0x000006d8] */ 0x0d983dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_close_x
/* [0x000006e0] */ 0x7f800002, 0xe0040067, // mov.ifz ra_scalar2, CLOSE_X_DEGENERATE
/* [0x000006e8] */ 0x150e7d80, 0x10020827, // mov r0, ra_scalar3
/* [0x000006f0] */ 0x00004000, 0xe0020867, // mov r1, (1<<14)
/* [0x000006f8] */ 0x809f3000, 0xd00049e5, // nop ; mov r5rep, r0 << rs3i_bitfield
/* [0x00000700] */ 0x149e7340, 0x100229e7, // and.setf -, r1, r5
/* [0x00000708] */ 0x00000038, 0xf02809e7, // brr.anyz -, r:1f
/* [0x00000710] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000718] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000720] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000728] */ 0x0e9cabc0, 0xd0021967, // shr r5rep, r5, 10
/* [0x00000730] */ 0x149cfbc0, 0xd0021967, // and r5rep, r5, 0xf
/* [0x00000738] */ 0x0d982dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_dash_phase_i
/* [0x00000740] */ 0x159e7b40, 0x10040067, // mov.ifz ra_scalar2, r5
/* [0x00000748] */ 0x809f1000, 0xd00049e5, // nop ; mov r5rep, r0 << rs3i_initial_dash_phase
/* [0x00000750] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_dash_phase
/* [0x00000758] */ 0x159e7b40, 0x10040067, // mov.ifz ra_scalar2, r5
// :1
/* [0x00000760] */ 0x00000280, 0xf0f809e7, // brr -, r:chunk_normal_loop_next_iteration
/* [0x00000768] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000770] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000778] */ 0x009e7000, 0x100009e7, // nop
// :chunk_normal_loop_preamble
/* [0x00000780] */ 0x15727d80, 0x1e0229e7, // mov.setf -, ra_chunk.8d
/* [0x00000788] */ 0x00000298, 0xf00809e7, // brr.allz -, r:chunk_normal_loop_end
/* [0x00000790] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000798] */ 0x009e7000, 0x100009e7, // nop
/* [0x000007a0] */ 0x009e7000, 0x100009e7, // nop
// :chunk_normal_loop_begin
/* [0x000007a8] */ 0x15067d80, 0x10020827, // mov r0, ra_scalar2
/* [0x000007b0] */ 0x026a7c00, 0x10022827, // fsub.setf r0, ra_chunk_normal_locals, r0
/* [0x000007b8] */ 0x00000081, 0xe20629e7, // mov.ifnz.setf -, elems(rsi_chunk_normal_locals_x0, rsi_chunk_normal_locals_y0)
/* [0x000007c0] */ 0x00000038, 0xf03809e7, // brr.anynz -, r:1f
/* [0x000007c8] */ 0x0d701dc0, 0xda0229e7, // sub.setf -, ra_chunk.8b, 1
/* [0x000007d0] */ 0x0d701dc0, 0xde0429e7, // sub.ifz.setf -, ra_chunk.8d, 1
/* [0x000007d8] */ 0x0d983dc0, 0xd00429e7, // sub.ifz.setf -, elem_num, rs2i_close_x
/* [0x000007e0] */ 0x7f800002, 0xe0021967, // mov r5rep, CLOSE_X_DEGENERATE
/* [0x000007e8] */ 0x0d067d40, 0x100429e7, // sub.ifz.setf -, ra_scalar2, r5
/* [0x000007f0] */ 0x000001f0, 0xf01809e7, // brr.allnz -, r:chunk_normal_loop_next_iteration
/* [0x000007f8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000800] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000808] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000810] */ 0x00000000, 0xe0620727, // mov ra_chunk.8c, 0
// :1
/* [0x00000818] */ 0x209e7000, 0x100049e1, // nop ; fmul r1, r0, r0
/* [0x00000820] */ 0x809fe000, 0xd00049e2, // nop ; mov r2, r0 >> (rsi_chunk_normal_locals_tx - rsi_chunk_normal_locals_x0)
/* [0x00000828] */ 0x0d98ff80, 0xd00229e7, // sub.setf -, rsi_chunk_normal_locals_ty, elem_num
/* [0x00000830] */ 0x809f8000, 0xd00089e2, // nop ; mov.ifz r2, r0 >> (rsi_chunk_normal_locals_ty - rsi_chunk_normal_locals_y0)
/* [0x00000838] */ 0x159e7240, 0x10021967, // mov r5rep, r1<<rsi_chunk_normal_locals_x0
/* [0x00000840] */ 0x959f9b49, 0xd0024865, // mov r1, r5  ; mov r5rep, r1<<rsi_chunk_normal_locals_y0
/* [0x00000848] */ 0x019e7340, 0x10020867, // fadd r1, r1, r5
/* [0x00000850] */ 0x029dfe40, 0x100229e7, // fsub.setf -, rb_eps, r1
/* [0x00000858] */ 0x159e7240, 0x10020d67, // mov rsqrt, r1
/* [0x00000860] */ 0x3fc00000, 0xe0021967, // mov r5rep, f(1.5)
/* [0x00000868] */ 0x209ef00f, 0xd00049e1, // nop ; fmul r1, r1, f(0.5)
/* [0x00000870] */ 0x209e700c, 0x100049e1, // nop ; fmul r1, r1, r4
/* [0x00000878] */ 0x209e700c, 0x100049e1, // nop ; fmul r1, r1, r4
/* [0x00000880] */ 0x029e7a40, 0x10020867, // fsub r1, r5, r1
/* [0x00000888] */ 0x209e7021, 0x100049e1, // nop ; fmul r1, r4, r1
/* [0x00000890] */ 0x15067d80, 0x100a0827, // mov.ifnn r0, ra_scalar2
/* [0x00000898] */ 0x009e7000, 0x100009e7, // nop
/* [0x000008a0] */ 0x209e7011, 0x100109e0, // nop ; fmul.ifn r0, r2, r1
/* [0x000008a8] */ 0x0000c000, 0xe20229e7, // mov.setf -, elems(rsi_chunk_normal_locals_tx, rsi_chunk_normal_locals_ty)
/* [0x000008b0] */ 0x159e7000, 0x100606a7, // mov.ifnz ra_chunk_normal_locals, r0
/* [0x000008b8] */ 0x8007d036, 0xd00049e5, // nop ; mov r5rep, ra_scalar2<<rs2i_close_x
/* [0x000008c0] */ 0x7f800002, 0xe0020827, // mov r0, CLOSE_X_DEGENERATE
/* [0x000008c8] */ 0x0d9e7a00, 0x100229e7, // sub.setf -, r5, r0
/* [0x000008d0] */ 0xdeadbeef, 0xf00009e7, // bra.allz -, a:2f
/* [0x000008d8] */ 0x8007e036, 0xd00049e5, // nop ; mov r5rep, ra_scalar2<<rs2i_dash_phase_i
/* [0x000008e0] */ 0x149c1bc0, 0xd00229e7, // and.setf -, r5, 0x1
/* [0x000008e8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000008f0] */ 0xdeadbeef, 0xf01009e7, // bra.allnz -, a:3f
/* [0x000008f8] */ 0x156a7d80, 0x10020867, // mov r1, ra_chunk_normal_locals
/* [0x00000900] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x00000908] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:3f
/* [0x00000910] */ 0x00003280, 0xf0f809e7, // brr -, r:add_join
/* [0x00000918] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000920] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000928] */ 0x009e7000, 0x100009e7, // nop
// :2
/* [0x00000930] */ 0x8007e036, 0xd00049e5, // nop ; mov r5rep, ra_scalar2 << rs2i_dash_phase_i
/* [0x00000938] */ 0x149c1bc0, 0xd00229e7, // and.setf -, r5, 0x1
/* [0x00000940] */ 0x00000020, 0xf00809e7, // brr.allz -, r:1f
/* [0x00000948] */ 0x0d983dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_close_x
/* [0x00000950] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000958] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000960] */ 0x00000040, 0xf0f809e7, // brr -, r:3f
/* [0x00000968] */ 0x7f800003, 0xe0040067, // mov.ifz ra_scalar2, CLOSE_X_NOT_IN_DASH
/* [0x00000970] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000978] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00000980] */ 0x95067db6, 0x10024825, // mov r0, ra_scalar2 ; mov r5rep, ra_scalar2<<rs2i_x
/* [0x00000988] */ 0x8d988ded, 0xd002b9c1, // sub.setf -, elem_num, rs2i_close_y ; mov.ifz ra_scalar2, r5
/* [0x00000990] */ 0x956b9d80, 0xd0024825, // mov r0, ra_chunk_normal_locals ; mov r5rep, r0<<rs2i_y
/* [0x00000998] */ 0x8d989ded, 0xd002b9c1, // sub.setf -, elem_num, rs2i_close_tx ; mov.ifz ra_scalar2, r5
/* [0x000009a0] */ 0x809f2000, 0xd00049e5, // nop ; mov r5rep, r0<<rsi_chunk_normal_locals_tx
/* [0x000009a8] */ 0x8d98aded, 0xd002b9c1, // sub.setf -, elem_num, rs2i_close_ty ; mov.ifz ra_scalar2, r5
/* [0x000009b0] */ 0x809f1000, 0xd00049e5, // nop ; mov r5rep, r0<<rsi_chunk_normal_locals_ty
/* [0x000009b8] */ 0x809e702d, 0x100099c1, // nop ; mov.ifz ra_scalar2, r5
// :3
/* [0x000009c0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000009c8] */ 0x156a7d80, 0x10020867, // mov r1, ra_chunk_normal_locals
/* [0x000009d0] */ 0x0d98ddc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_dash_clerp_lr
/* [0x000009d8] */ 0xdeadbeef, 0xe0040067, // mov.ifz ra_scalar2, a:1f
/* [0x000009e0] */ 0x00000000, 0xf0f7c9e7, // bra -, ra_scalar
/* [0x000009e8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000009f0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000009f8] */ 0x009e7000, 0x100009e7, // nop
// :1
// :chunk_normal_loop_next_iteration
/* [0x00000a00] */ 0x156a7d80, 0x10020827, // mov r0, ra_chunk_normal_locals
/* [0x00000a08] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000a10] */ 0x809ff000, 0xd00059da, // nop ; mov ra_chunk_normal_locals, r0 << 1
/* [0x00000a18] */ 0x0d701dc0, 0xde722727, // sub.setf ra_chunk.8d, ra_chunk.8d, 1
/* [0x00000a20] */ 0xfffffd68, 0xf01809e7, // brr.allnz -, r:chunk_normal_loop_begin
/* [0x00000a28] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000a30] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000a38] */ 0x009e7000, 0x100009e7, // nop
// :chunk_normal_loop_end
/* [0x00000a40] */ 0x15727d80, 0x1a0229e7, // mov.setf -, ra_chunk.8b
/* [0x00000a48] */ 0x000001e8, 0xf00809e7, // brr.allz -, r:chunk_normal_end
/* [0x00000a50] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000a58] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000a60] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000a68] */ 0x15727d80, 0x1c0229e7, // mov.setf -, ra_chunk.8c
/* [0x00000a70] */ 0x000000b0, 0xf00809e7, // brr.allz -, r:1f
/* [0x00000a78] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000a80] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000a88] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000a90] */ 0x0d983dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_close_x
/* [0x00000a98] */ 0x7f800003, 0xe0020827, // mov r0, CLOSE_X_NOT_IN_DASH
/* [0x00000aa0] */ 0x0d067c00, 0x100429e7, // sub.ifz.setf -, ra_scalar2, r0
/* [0x00000aa8] */ 0x00000078, 0xf02809e7, // brr.anyz -, r:1f
/* [0x00000ab0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000ab8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000ac0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000ac8] */ 0x0d982dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_dash_phase_i
/* [0x00000ad0] */ 0x14041dc0, 0xd00429e7, // and.ifz.setf -, ra_scalar2, 0x1
/* [0x00000ad8] */ 0x00000048, 0xf01809e7, // brr.allnz -, r:1f
/* [0x00000ae0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000ae8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000af0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000af8] */ 0x15067d80, 0x10020827, // mov r0, ra_scalar2
/* [0x00000b00] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000b08] */ 0x809f5000, 0xd00049e1, // nop ; mov r1, r0>>5
/* [0x00000b10] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x00000b18] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:chunk_normal_end
/* [0x00000b20] */ 0x00003070, 0xf0f809e7, // brr -, r:add_join
/* [0x00000b28] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000b30] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000b38] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00000b40] */ 0x0d983dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_close_x
/* [0x00000b48] */ 0x7f800003, 0xe0020827, // mov r0, CLOSE_X_NOT_IN_DASH
/* [0x00000b50] */ 0x0d067c00, 0x100429e7, // sub.ifz.setf -, ra_scalar2, r0
/* [0x00000b58] */ 0x00000070, 0xf02809e7, // brr.anyz -, r:1f
/* [0x00000b60] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000b68] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000b70] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000b78] */ 0x15067d80, 0x10020867, // mov r1, ra_scalar2
/* [0x00000b80] */ 0x0d987dc0, 0xd00229e7, // sub.setf -, elem_num, 7
/* [0x00000b88] */ 0x809fd009, 0xd00049e0, // nop ; mov r0, r1<<rs2i_close_x
/* [0x00000b90] */ 0x809ff009, 0xd00089e0, // nop ; mov.ifz r0, r1<<(rs2i_close_y-7)
/* [0x00000b98] */ 0x0d98edc0, 0xd00229e7, // sub.setf -, elem_num, 14
/* [0x00000ba0] */ 0x809f5009, 0xd00149e0, // nop ; mov.ifnn r0, r1<<(rs2i_close_ty-15)
/* [0x00000ba8] */ 0x809f5009, 0xd00089e0, // nop ; mov.ifz r0, r1<<(rs2i_close_tx-14)
/* [0x00000bb0] */ 0x029c0e00, 0xd00a0827, // fsub.ifnn r0, f(0.0), r0
/* [0x00000bb8] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x00000bc0] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:1f
/* [0x00000bc8] */ 0x000030d0, 0xf0f809e7, // brr -, r:add_cap
/* [0x00000bd0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000bd8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000be0] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00000be8] */ 0x0d982dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_dash_phase_i
/* [0x00000bf0] */ 0x14041dc0, 0xd00429e7, // and.ifz.setf -, ra_scalar2, 0x1
/* [0x00000bf8] */ 0x00000038, 0xf01809e7, // brr.allnz -, r:chunk_normal_end
/* [0x00000c00] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000c08] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000c10] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000c18] */ 0x15067d80, 0x10020827, // mov r0, ra_scalar2
/* [0x00000c20] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x00000c28] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:chunk_normal_end
/* [0x00000c30] */ 0x00003068, 0xf0f809e7, // brr -, r:add_cap
/* [0x00000c38] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000c40] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000c48] */ 0x009e7000, 0x100009e7, // nop
// :chunk_normal_end
/* [0x00000c50] */ 0x957bedb6, 0xd00269e5, // mov.setf  -, ra_scalar ; mov  r5rep, ra_scalar << rsi_chunk_lr
/* [0x00000c58] */ 0x8d986ded, 0xd00479ca, // sub.ifz.setf  -, elem_num, rsi_chunks_m_size ; mov  ra_temp, r5
/* [0x00000c60] */ 0x009e7000, 0x100009e7, // nop                    ; nop
/* [0x00000c68] */ 0x00000000, 0xf01549e7, // bra.allnz  -, ra_temp
/* [0x00000c70] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000c78] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000c80] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000c88] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x00000c90] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:1f
/* [0x00000c98] */ 0x0d980dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_clerp_q_n
/* [0x00000ca0] */ 0x157a7d80, 0x100429e7, // mov.ifz.setf  -, ra_scalar
/* [0x00000ca8] */ 0x00001a70, 0xf01809e7, // brr.allnz -, r:flush_clerp_q
/* [0x00000cb0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000cb8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000cc0] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00000cc8] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x00000cd0] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:1f
/* [0x00000cd8] */ 0x0d981dc0, 0xd00229e7, // sub.setf  -, elem_num, rbsi_join_q_n
/* [0x00000ce0] */ 0x159c0fc0, 0x100429e7, // mov.ifz.setf  -, rb_scalar
/* [0x00000ce8] */ 0x15067d80, 0x10020827, // mov r0, ra_scalar2
/* [0x00000cf0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000cf8] */ 0x809fa000, 0xd00059ca, // nop ; mov ra_temp, (r0 << rs2i_flush_join_q_lbl) >> 15
/* [0x00000d00] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000d08] */ 0x00000000, 0xf01549e7, // bra.allnz -, ra_temp
/* [0x00000d10] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000d18] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000d20] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00000d28] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x00000d30] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:1f
/* [0x00000d38] */ 0x0d980dc0, 0xd00229e7, // sub.setf  -, elem_num, rbsi_cap_q_n
/* [0x00000d40] */ 0x159c0fc0, 0x100429e7, // mov.ifz.setf  -, rb_scalar
/* [0x00000d48] */ 0x15067d80, 0x10020827, // mov r0, ra_scalar2
/* [0x00000d50] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000d58] */ 0x809fb000, 0xd00059ca, // nop ; mov ra_temp, (r0 << rs2i_flush_cap_q_lbl) >> 15
/* [0x00000d60] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000d68] */ 0x00000000, 0xf01549e7, // bra.allnz -, ra_temp
/* [0x00000d70] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000d78] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000d80] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00000d88] */ 0x40004000, 0xe20229e7, // mov.setf  -, neg_elems(elems(rsi_alloc_size))
/* [0x00000d90] */ 0x00000040, 0xe0020827, // mov  r0, 64
/* [0x00000d98] */ 0x0d7a7c00, 0x100829e7, // sub.ifn.setf  -, ra_scalar, r0 ; nop
/* [0x00000da0] */ 0x00002fe0, 0xf06809e7, // brr.anyn  -, r:alloc
/* [0x00000da8] */ 0xdeadbeef, 0xe00202a7, // mov  ra_temp, a:1f
/* [0x00000db0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000db8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000dc0] */ 0x157a7d80, 0x10020827, // mov  r0, ra_scalar
/* [0x00000dc8] */ 0xfff10000, 0xe0020867, // mov  r1, -(15 << 16)
/* [0x00000dd0] */ 0x8c9f6040, 0xd0024871, // add  r1, r0, r1 ; mov  vw_setup, r0 << rsi_out_vpm_setup
/* [0x00000dd8] */ 0x0d9c41c0, 0xd0020827, // sub  r0, r0, 4
/* [0x00000de0] */ 0x159f2fc0, 0x100009e7, // mov  -, vw_wait
/* [0x00000de8] */ 0x042a0101, 0xe0020c27, // mov  vpm, TRI_LIST_HEAD
/* [0x00000df0] */ 0x809f4009, 0xd00049f1, // nop ; mov  vw_setup, r1 << rsi_out_vdw_setup_0
/* [0x00000df8] */ 0x809f3000, 0xd00049f2, // nop ; mov  vw_addr, r0 << rsi_alloc_p
/* [0x00000e00] */ 0x00006000, 0xe20229e7, // mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
/* [0x00000e08] */ 0x0c790dc0, 0xd00607a7, // add.ifnz  ra_scalar, ra_scalar, -16
// :1
/* [0x00000e10] */ 0x0c99fdc0, 0xd00229e7, // add.setf  -, elem_num, -1
/* [0x00000e18] */ 0x9579fff6, 0xd0024822, // mov  r0, -1 ; mov  r2, ra_scalar
/* [0x00000e20] */ 0x00000030, 0xe0020867, // mov  r1, 48
/* [0x00000e28] */ 0x809f3012, 0xd00049e5, // nop ; mov  r5rep, r2 << rsi_alloc_p
/* [0x00000e30] */ 0x0d9e7a40, 0x10040827, // sub.ifz  r0, r5, r1
/* [0x00000e38] */ 0x0c9ccbc0, 0xd0080827, // add.ifn  r0, r5, 12
/* [0x00000e40] */ 0x0c9c15c0, 0xd00208a7, // add  r2, r2, 1
/* [0x00000e48] */ 0x00000080, 0xe00208e7, // mov  r3, 1 << 7
/* [0x00000e50] */ 0x8c7b6cd2, 0xd00248b1, // add  r2, ra_scalar, r3 ; mov  vw_setup, r2 << rsi_out_vpm_setup
/* [0x00000e58] */ 0x809e7000, 0x100049f0, // nop                    ; mov  vpm, r0
/* [0x00000e60] */ 0x957b4d92, 0xd0024831, // mov  r0, ra_scalar     ; mov  vw_setup, r2 << rsi_out_vdw_setup_0
/* [0x00000e68] */ 0x8d7a0c7f, 0x10024872, // sub  r1, ra_scalar, r1 ; mov  vw_addr, unif
/* [0x00000e70] */ 0x809f7000, 0xd00049f1, // nop ; mov  vw_setup, r0 << rsi_chunks_vpm_setup
/* [0x00000e78] */ 0x809f3009, 0xd00049f0, // nop ; mov  vpm, r1 << rsi_alloc_p
/* [0x00000e80] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00000e88] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00000e90] */ 0x159f2fc0, 0x100009e7, // mov  -, vw_wait
/* [0x00000e98] */ 0x00000001, 0xe00209a7, // mov  interrupt, 1
/* [0x00000ea0] */ 0x009e7000, 0x300009e7, // thrend
/* [0x00000ea8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000eb0] */ 0x009e7000, 0x100009e7, // nop
// :chunk_cubic
/* [0x00000eb8] */ 0x15727d80, 0x10021967, // mov  r5rep, ra_chunk
/* [0x00000ec0] */ 0x959ffb52, 0xd0024701, // mov  ra_chunk, r5 ; mov  rb_cubics[0], r2 << 1
/* [0x00000ec8] */ 0x0d98bdc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_is_cubic
/* [0x00000ed0] */ 0x00000001, 0xe0040067, // mov.ifz  ra_scalar2, 1
/* [0x00000ed8] */ 0x15727d80, 0x1a0229e7, // mov.setf  -, ra_chunk.8b
/* [0x00000ee0] */ 0x000000b8, 0xf00809e7, // brr.allz  -, r:1f
/* [0x00000ee8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000ef0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000ef8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000f00] */ 0x0d980dc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_x
/* [0x00000f08] */ 0x809ff012, 0xd00099c1, // nop ; mov.ifz  ra_scalar2, (r2 << 1) >> rs2i_x
/* [0x00000f10] */ 0x0d987dc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_y
/* [0x00000f18] */ 0x809f5012, 0xd00099c1, // nop ; mov.ifz  ra_scalar2, (r2 << 2) >> rs2i_y
/* [0x00000f20] */ 0x0d98edc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_tx
/* [0x00000f28] */ 0x3f800000, 0xe0040067, // mov.ifz  ra_scalar2, f(1.0)
/* [0x00000f30] */ 0x0d98fdc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_ty
/* [0x00000f38] */ 0x00000000, 0xe0040067, // mov.ifz  ra_scalar2, f(0.0)
/* [0x00000f40] */ 0x0d983dc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_close_x
/* [0x00000f48] */ 0x7f800002, 0xe0040067, // mov.ifz  ra_scalar2, CLOSE_X_DEGENERATE
/* [0x00000f50] */ 0x150e7d80, 0x10020827, // mov  r0, ra_scalar3
/* [0x00000f58] */ 0x00004000, 0xe0020867, // mov  r1, (1 << 14)
/* [0x00000f60] */ 0x809f3000, 0xd00049e5, // nop ; mov  r5rep, r0 << rs3i_bitfield
/* [0x00000f68] */ 0x149e7a40, 0x100229e7, // and.setf  -, r5, r1
/* [0x00000f70] */ 0x00000028, 0xf00809e7, // brr.allz  -, r:1f
/* [0x00000f78] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000f80] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000f88] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000f90] */ 0x0d981dc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_phase
/* [0x00000f98] */ 0x809f2000, 0xd00099c1, // nop ; mov.ifz  ra_scalar2, (r0 << rs3i_initial_dash_phase) >> rs2i_dash_phase
/* [0x00000fa0] */ 0x0d982dc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_phase_i
/* [0x00000fa8] */ 0x0e9cabc0, 0xd0021967, // shr  r5rep, r5, 10
/* [0x00000fb0] */ 0x149cfbc0, 0xd0040067, // and.ifz  ra_scalar2, r5, 0xf
// :1
/* [0x00000fb8] */ 0x809fe012, 0xd00049e0, // nop                          ; mov  r0, r2 << 2
/* [0x00000fc0] */ 0x827be436, 0xd00269e5, // fsub.setf  -, r2, r0         ; mov  r5rep, ra_scalar << rsi_chunk_lr
/* [0x00000fc8] */ 0x95727dad, 0x1c0479ca, // mov.ifz.setf  -, ra_chunk.8c ; mov  ra_temp, r5
/* [0x00000fd0] */ 0x0000007e, 0xe20629e7, // mov.ifnz.setf  -, [0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0]
/* [0x00000fd8] */ 0x00000000, 0xf00549e7, // bra.allz  -, ra_temp
/* [0x00000fe0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000fe8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000ff0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00000ff8] */ 0x809f9012, 0xd00049e2, // nop                       ; mov  r2, r2 << 7
/* [0x00001000] */ 0x0300ff03, 0xe20229e7, // mov.setf  -, [1, 1, 0, 0, 0, 0, 0, 0, -1, -1, 1, 1, 1, 1, 1, 1]
/* [0x00001008] */ 0x959fe492, 0xd0030822, // mov  r0, r2               ; mov.ifn  r2, r2 << 2         @mul_used(8, 9)
/* [0x00001010] */ 0x809f8000, 0xd00109e0, // nop                       ; mov.ifn  r0, r0 >> 8         @mul_used(8, 9)
/* [0x00001018] */ 0x809fc012, 0xd00089e2, // nop                       ; mov.ifz  r2, r2 << 4
/* [0x00001020] */ 0x809fe000, 0xd00089e0, // nop                       ; mov.ifz  r0, r0 << 2
/* [0x00001028] */ 0x959fe012, 0xd00682a2, // mov.ifnz  ra_temp, r0     ; mov.ifz  r2, r2 << 2
/* [0x00001030] */ 0x809fe000, 0xd00099ca, // nop                       ; mov.ifz  ra_temp, r0 << 2
/* [0x00001038] */ 0x95078d92, 0xd0024822, // mov  r0, ra_scalar2       ; mov  r2, r2 << 8
/* [0x00001040] */ 0x802be036, 0xd00049e1, // nop                       ; mov  r1, ra_temp << 2
/* [0x00001048] */ 0x829fa280, 0xd00b0861, // fsub.ifnn  r1, r1, r2     ; mov.ifn  r1, (r0 << rs2i_tx) >> 8
/* [0x00001050] */ 0x00035557, 0xe20229e7, // mov.setf  -, [-1, -1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0]
/* [0x00001058] */ 0x209e7009, 0x100049e0, // nop                       ; fmul  r0, r1, r1
/* [0x00001060] */ 0x209ff009, 0xd00049e2, // nop                       ; fmul  r2 >> 1, r1, r1
/* [0x00001068] */ 0x819f1080, 0xd0064822, // fadd.ifnz  r0, r0, r2     ; mov  r2, r0 >> 1
/* [0x00001070] */ 0x819f8089, 0xd0050821, // fadd.ifz  r0, r0, r2      ; mov.ifn  r1, r1 << 8
/* [0x00001078] */ 0x159e7000, 0x10020d67, // mov  rsqrt, r0            ; nop
/* [0x00001080] */ 0x029df1c0, 0x100229e7, // fsub.setf  -, r0, rb_eps  ; nop
/* [0x00001088] */ 0x809f2009, 0xd00109e1, // nop                       ; mov.ifn  r1, r1 >> 2
/* [0x00001090] */ 0x359cefcc, 0xd0034821, // mov  r0, 14               ; fmul.ifnn  r1, r1, r4
/* [0x00001098] */ 0x809f2009, 0xd00109e1, // nop                       ; mov.ifn  r1, r1 >> 2         @mul_used(4, 5, 6, 7, 12, 13, 14, 15)
/* [0x000010a0] */ 0x8d9b2c09, 0xd00329e1, // sub.setf  -, elem_num, r0 ; mov.ifn  r1, r1 >> 2         @mul_used(6, 7, 14, 15)
/* [0x000010a8] */ 0x809f8009, 0xd00159dc, // nop                       ; mov.ifnn  ra_chunk, r1 >> 8  @mul_used(14, 15)
/* [0x000010b0] */ 0x809fc009, 0xd00059da, // nop                       ; mov  ra_cubics[0], r1 << 4
/* [0x000010b8] */ 0x0d983dc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_close_x
/* [0x000010c0] */ 0x7f800002, 0xe0020827, // mov  r0, CLOSE_X_DEGENERATE
/* [0x000010c8] */ 0x8d067c36, 0x100469e0, // sub.ifz.setf  -, ra_scalar2, r0 ; mov  r0, ra_scalar2
/* [0x000010d0] */ 0x00000050, 0xf01809e7, // brr.allnz  -, r:1f
/* [0x000010d8] */ 0x0d982dc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_phase_i
/* [0x000010e0] */ 0x149c11c0, 0xd00429e7, // and.ifz.setf  -, r0, 0x1
/* [0x000010e8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000010f0] */ 0x00000050, 0xf01809e7, // brr.allnz  -, r:2f
/* [0x000010f8] */ 0x0d983dc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_close_x
/* [0x00001100] */ 0x7f800003, 0xe0040067, // mov.ifz  ra_scalar2, CLOSE_X_NOT_IN_DASH
/* [0x00001108] */ 0x009e7000, 0x100009e7, // nop
/* [0x00001110] */ 0x809f3000, 0xd00099c1, // nop ; mov.ifz  ra_scalar2, (r0 << rs2i_x) >> rs2i_close_x
/* [0x00001118] */ 0x0d988dc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_close_y
/* [0x00001120] */ 0x00000020, 0xf0f809e7, // brr  -, r:2f
/* [0x00001128] */ 0x809f1000, 0xd00099c1, // nop ; mov.ifz  ra_scalar2, (r0 << rs2i_y) >> rs2i_close_y
/* [0x00001130] */ 0x00000600, 0xe20229e7, // mov.setf  -, elems(rs2i_close_tx, rs2i_close_ty)
/* [0x00001138] */ 0x809fb009, 0xd000d9c1, // nop ; mov.ifnz  ra_scalar2, (r1 << 14) >> rs2i_close_tx
// :1
/* [0x00001140] */ 0x00002a50, 0xf02802a7, // brr.anyz  ra_temp, r:add_join
/* [0x00001148] */ 0x0d981dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_q_lr
/* [0x00001150] */ 0x152a7d80, 0x100407a7, // mov.ifz  ra_scalar, ra_temp
/* [0x00001158] */ 0x009e7000, 0x100009e7, // nop
// :2
/* [0x00001160] */ 0x156a7d80, 0x10020827, // mov  r0, ra_cubics[0]
/* [0x00001168] */ 0x0000c000, 0xe20229e7, // mov.setf  -, elems(rs2i_tx, rs2i_ty)
/* [0x00001170] */ 0x809f4000, 0xd000d9c1, // nop ; mov.ifnz  ra_scalar2, (r0 << 10) >> rs2i_tx
/* [0x00001178] */ 0x0d988dc0, 0xd00229e7, // sub.setf  -, elem_num, 8
/* [0x00001180] */ 0x0f000f00, 0xe2020867, // mov  r1, [0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0]
/* [0x00001188] */ 0x0c0f0c0c, 0xe60208a7, // mov  r2, [2, 2, 3, 3, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0]
/* [0x00001190] */ 0x969c12bf, 0x10024860, // xor  r1, r1, r2       ; mov  r0, rb_cubics[0]
/* [0x00001198] */ 0x0ffc0ccf, 0xe20208a7, // mov  r2, [1, 1, -1, -1, -2, -2, -1, -1, -2, -2, -1, -1, 0, 0, 0, 0]
/* [0x000011a0] */ 0x889f8240, 0xd00342a0, // itof  ra_temp, r1     ; mov.ifnn  r0, r0 << 8
/* [0x000011a8] */ 0x889e7480, 0x10024841, // itof  r1, r2          ; mov  rb_cubics[0], r0
/* [0x000011b0] */ 0x033ff303, 0xe20208a7, // mov  r2, [-1, -1, -2, -2, -2, -2, 0, 0, -1, -1, 0, 0, 1, 1, 1, 1]
/* [0x000011b8] */ 0x003c3030, 0xe20208e7, // mov  r3, [0, 0, -2, -2, -1, -1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0]
/* [0x000011c0] */ 0x2c9f24c8, 0xd00248a1, // add  r2, r2, r3       ; fmul  r1 << 2, r1, r0
/* [0x000011c8] */ 0x089e7480, 0x100208a7, // itof  r2, r2          ; nop
/* [0x000011d0] */ 0x3c000c00, 0xe2020127, // mov  ra_temp2, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -2, -2, 0, 0]
/* [0x000011d8] */ 0x3c3f3c0c, 0xe60208e7, // mov  r3, [2, 2, 3, 3, 2, 2, 0, 0, 0, 0, 3, 3, 3, 3, 0, 0]
/* [0x000011e0] */ 0x36134790, 0xd00248e2, // xor  r3, r3, ra_temp2 ; fmul  r2 << 4, r2, r0
/* [0x000011e8] */ 0x089e76c0, 0x100208e7, // itof  r3, r3          ; nop
/* [0x000011f0] */ 0x222a7470, 0x10024862, // fsub  r1, r2, r1      ; fmul  r2, ra_temp, r0
/* [0x000011f8] */ 0x219f6298, 0xd0024863, // fadd  r1, r1, r2      ; fmul  r3 << 6, r3, r0
/* [0x00001200] */ 0x019e72c0, 0x10020867, // fadd  r1, r1, r3      ; nop
/* [0x00001208] */ 0x809f1009, 0xd00059ca, // nop                   ; mov  ra_temp, r1 >> 1  @mul_used(2, 4, 6)
/* [0x00001210] */ 0x809f3009, 0xd00049e2, // nop                   ; mov  r2, r1 >> 3
/* [0x00001218] */ 0x202a700e, 0x100059ca, // nop                   ; fmul  ra_temp, r1, ra_temp
/* [0x00001220] */ 0x00100044, 0xe20208e7, // mov  r3, [0, 0, 1, 0, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0]
/* [0x00001228] */ 0x289ff6ca, 0xd00248e2, // itof  r3, r3          ; fmul  r2 >> 1, r1, r2
/* [0x00001230] */ 0x822a25bf, 0xd00248a5, // fsub  r2, r2, ra_temp ; mov  r5rep, f(4.0)
/* [0x00001238] */ 0x359e0fd3, 0xd00308e1, // mov  r3, f(1.0)       ; fmul.ifn  r1, r2, r3
/* [0x00001240] */ 0x359e7249, 0x10026d20, // mov.setf  recip, r1 ; fmul  r0, r1, r1
/* [0x00001248] */ 0x369fc00d, 0xd0025962, // mov  r5rep, f(0.0)  ; fmul  r2 >> 4, r1, r5
/* [0x00001250] */ 0x229f2aca, 0xd0025962, // fsub  r5rep, r5, r3 ; fmul  r2 << 2, r1, r2  @mul_unused(0, 1)
/* [0x00001258] */ 0x229ef0a7, 0xd0024d63, // fsub  rsqrt, r0, r2 ; fmul  r3, r4, f(0.5)
/* [0x00001260] */ 0x829f209b, 0xd0024823, // fsub  r0, r0, r2    ; mov  r3, r3 >> 2       @mul_unused(0, 1)
/* [0x00001268] */ 0x359e7028, 0x100369e0, // mov.setf  -, r0     ; fmul.ifnn  r0, r5, r0
/* [0x00001270] */ 0x209e7004, 0x100049e0, // nop                 ; fmul  r0, r0, r4
/* [0x00001278] */ 0x829fe049, 0xd0024d22, // fsub  recip, r0, r1 ; mov  r2, r1 << 2
/* [0x00001280] */ 0x229e1057, 0xd0024822, // fsub  r0, r0, r1    ; fmul  r2, r2, f(2.0)
/* [0x00001288] */ 0x359e7b43, 0x100948e3, // mov.ifn  r3, r5     ; fmul.ifnn  r3, r0, r3
/* [0x00001290] */ 0x359e7b54, 0x100948a2, // mov.ifn  r2, r5     ; fmul.ifnn  r2, r2, r4
/* [0x00001298] */ 0x219ef4ef, 0xd0024825, // fadd  r0, r2, r3                  ; fmul  r5rep, r5, f(0.5)
/* [0x000012a0] */ 0xe29e07c0, 0xd00269e1, // fsub.setf  -, r3, f(1.0)          ; mov  r1, f(0.0)
/* [0x000012a8] */ 0x229e72c5, 0x100868e0, // fsub.ifn.setf  r3, r1, r3         ; fmul  r0, r0, r5
/* [0x000012b0] */ 0x829e05ff, 0xd00369e0, // fsub.setf  -, r2, f(1.0)          ; mov.ifnn  r0, f(1.0)
/* [0x000012b8] */ 0x826bf2b6, 0xd00868a1, // fsub.ifn.setf  r2, r1, r2         ; mov  r1, ra_cubics[0] << 1  @mul_used(14)
/* [0x000012c0] */ 0x959e76d2, 0x100b48a3, // mov.ifnn  r2, r3                  ; mov.ifnn  r3, r2
/* [0x000012c8] */ 0x866a0c7f, 0xd00369e0, // fmaxabs.setf  -, ra_cubics[0], r1 ; mov.ifnn  r0, f(1.0)
/* [0x000012d0] */ 0x809ff000, 0xd001c9e0, // nop                               ; mov.ifnc  r0, r0 << 1       @mul_used(14)
/* [0x000012d8] */ 0x809fc012, 0xd00069e5, // nop                               ; mov.setf  r5rep, r2 << 4
/* [0x000012e0] */ 0x956b6d80, 0xd0034865, // mov  r1, ra_cubics[0]             ; mov.ifnn  r5rep, r0 << 10
/* [0x000012e8] */ 0x0001ff7f, 0xe20229e7, // mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
/* [0x000012f0] */ 0x959c1fff, 0x1002981c, // mov  r0, rb_cubics[0] ; mov.ifz  ra_chunk, rb_cubics[0]
/* [0x000012f8] */ 0x009e7000, 0x100009e7, // nop                   ; nop
/* [0x00001300] */ 0x959fab40, 0xd00339dc, // mov.setf  -, r5       ; mov.ifn  ra_chunk, r0 << 6
/* [0x00001308] */ 0x00000250, 0xf05809e7, // brr.allnn  -, r:cubic_subd_0
/* [0x00001310] */ 0x00f03cff, 0xe20229e7, // mov.setf  -, [1, 1, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, 1, 1, 0, 0]
/* [0x00001318] */ 0x829fc749, 0xd0088861, // fsub.ifn  r1, r3, r5  ; mov.ifz  r1, r1 >> 12
/* [0x00001320] */ 0xdeadbeef, 0xe00206a7, // mov  ra_cubics[0], a:chunk_cubic_tail
/* [0x00001328] */ 0x819e0f49, 0xd0025d1a, // fadd  recip, f(1.0), r5         ; mov  ra_cubics[0], r1
// :chunk_cubic_split
/* [0x00001330] */ 0x219e0f68, 0xd0024862, // fadd  r1, f(1.0), r5            ; fmul  r2, r5, r0
/* [0x00001338] */ 0x209f2008, 0xd00049e3, // nop                             ; fmul  r3 << 2, r1, r0           @mul_unused(0, 1, 8, 9)
/* [0x00001340] */ 0x226a76b4, 0x1003189a, // fsub  r2, r3, r2                ; fmul.ifn  ra_cubics[0], ra_cubics[0], r4
/* [0x00001348] */ 0x209e702a, 0x100059ca, // nop                             ; fmul  ra_temp, r5, r2
/* [0x00001350] */ 0x359f248a, 0xd0024123, // mov  ra_temp2, r2               ; fmul  r3 << 2, r1, r2
/* [0x00001358] */ 0x822bc792, 0xd00248a3, // fsub  r2, r3, ra_temp           ; mov  r3, r2 << 4
/* [0x00001360] */ 0x359e74aa, 0x1002528a, // mov  rb_temp1, r2               ; fmul  ra_temp, r5, r2
/* [0x00001368] */ 0x35132d8a, 0xd00a48a1, // mov.ifnn  r2, ra_temp2          ; fmul  r1 << 2, r1, r2
/* [0x00001370] */ 0x822be392, 0xd00302a3, // fsub  ra_temp, r1, ra_temp      ; mov.ifn  r3, r2 << 2            @mul_used(4, 5)
/* [0x00001378] */ 0x826be6b6, 0xd0068861, // fsub.ifnz  r1, r3, r2           ; mov.ifz  r1, ra_cubics[0] << 2  @mul_used(8, 9)
/* [0x00001380] */ 0x34981dc9, 0xd00269e2, // and.setf  -, elem_num, 1        ; fmul  r2, r1, r1
/* [0x00001388] */ 0x209ff009, 0xd00049e3, // nop                             ; fmul  r3 >> 1, r1, r1
/* [0x00001390] */ 0x019e74c0, 0x100208a7, // fadd  r2, r2, r3                ; nop
/* [0x00001398] */ 0x809f1012, 0xd000c9e2, // nop                             ; mov.ifnz  r2, r2 >> 1           @mul_unused(0)
/* [0x000013a0] */ 0x959df4bf, 0xd0024d65, // mov  rsqrt, r2                  ; mov  r5rep, -1
/* [0x000013a8] */ 0x029df5c0, 0x100229e7, // fsub.setf  -, r2, rb_eps        ; nop
/* [0x000013b0] */ 0x809fc009, 0xd00109e1, // nop                             ; mov.ifn  r1, r1 << 4
/* [0x000013b8] */ 0x359cafcc, 0x100348a1, // mov  r2, rb_temp1               ; fmul.ifnn  r1, r1, r4
/* [0x000013c0] */ 0x93276b89, 0xd00329e1, // max.setf  -, r5, ra_subd_flags1 ; mov.ifn  r1, r1 >> 6            @mul_used(10, 11)
/* [0x000013c8] */ 0x952b4d89, 0xd00248e1, // mov  r3, ra_temp                ; mov  r1, r1 >> 4                @mul_used(14, 15)
/* [0x000013d0] */ 0x9513edb6, 0xd00e8801, // mov.ifnc  r0, ra_temp2    ; mov.ifz  rb_cubics[0], ra_temp2 << 2  @mul_used(4, 5, 12, 13)
/* [0x000013d8] */ 0x000002d0, 0xf0f80627, // brr  ra_cubics[1], r:cubic_subd_1
/* [0x000013e0] */ 0x959fc6d2, 0xd00b0801, // mov.ifnn  r0, r3          ; mov.ifn  rb_cubics[0], r2 << 4
/* [0x000013e8] */ 0x959fa49b, 0xd0058801, // mov.ifz  r0, r2           ; mov.ifc  rb_cubics[0], r3 << 6
/* [0x000013f0] */ 0x956bcd89, 0xd009185a, // mov.ifn  r1, ra_cubics[0] ; mov.ifn  ra_cubics[0], r1 << 4
/* [0x000013f8] */ 0x95681ff6, 0x10024821, // mov  r0, rb_cubics[0]        ; mov  r1, ra_cubics[0]
/* [0x00001400] */ 0x0001ff7f, 0xe20229e7, // mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
/* [0x00001408] */ 0x959f4009, 0xd0094861, // mov.ifn  r1, r0              ; mov.ifnn  r1, r1 >> 4
/* [0x00001410] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
/* [0x00001418] */ 0x809f6000, 0xd00089e1, // nop                          ; mov.ifz  r1, r0 >> 6
/* [0x00001420] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
/* [0x00001428] */ 0x152a7d80, 0x10040067, // mov.ifz  ra_scalar2, ra_temp ; nop
/* [0x00001430] */ 0x00100010, 0xe20229e7, // mov.setf  -, [0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
/* [0x00001438] */ 0x95681dbf, 0x10086860, // mov.ifn.setf  r1, ra_cubics[0] ; mov  r0, rb_cubics[0]
/* [0x00001440] */ 0x00000268, 0xf05809e7, // brr.allnn  -, r:cubic_subd_1
/* [0x00001448] */ 0x956bcd89, 0xd0024865, // mov  r1, ra_cubics[0]          ; mov  r5rep, r1 << 4
/* [0x00001450] */ 0xdeadbeef, 0xe0020627, // mov  ra_cubics[1], a:chunk_cubic_tail
/* [0x00001458] */ 0x00f03cff, 0xe20229e7, // mov.setf  -, [1, 1, 1, 1, -1, -1, -1, -1, 0, 0, 1, 1, 1, 1, 0, 0]
/* [0x00001460] */ 0xfffffeb0, 0xf0f809e7, // brr  -, r:chunk_cubic_split
/* [0x00001468] */ 0xf59e0fc0, 0xd0031d1a, // mov  recip, f(1.0) ; mov.ifn  ra_cubics[0], 0
/* [0x00001470] */ 0x009e7000, 0x100009e7, // nop                ; nop
/* [0x00001478] */ 0x009e7000, 0x100009e7, // nop                ; nop
// :chunk_cubic_tail
/* [0x00001480] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
/* [0x00001488] */ 0x15727d80, 0x10020867, // mov  r1, ra_chunk            ; nop
/* [0x00001490] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
/* [0x00001498] */ 0x152a7d80, 0x10040067, // mov.ifz  ra_scalar2, ra_temp ; nop
/* [0x000014a0] */ 0x807be036, 0xd00049e5, // nop                          ; mov  r5rep, ra_scalar << rsi_chunk_lr
/* [0x000014a8] */ 0x8d981ded, 0xd00279ca, // sub.setf  -, elem_num, 1     ; mov  ra_temp, r5
/* [0x000014b0] */ 0x15727d80, 0x1c0429e7, // mov.ifz.setf  -, ra_chunk.8c ; nop
/* [0x000014b8] */ 0x00000000, 0xf02549e7, // bra.anyz  -, ra_temp
/* [0x000014c0] */ 0x0d981dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_q_lr ; nop
/* [0x000014c8] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:1f
/* [0x000014d0] */ 0x0d983dc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_close_x ; nop
/* [0x000014d8] */ 0x7f800003, 0xe0020827, // mov  r0, CLOSE_X_NOT_IN_DASH
/* [0x000014e0] */ 0x8d067c36, 0x100469e1, // sub.ifz.setf  -, ra_scalar2, r0 ; mov  r1, ra_scalar2
/* [0x000014e8] */ 0x969f5009, 0xd00248a0, // mov  r2, f(0.0)                 ; mov  r0, (r1 << rs2i_close_tx) >> 14  @mul_used(14, 15)
/* [0x000014f0] */ 0x000027a8, 0xf01809e7, // brr.allnz  -, r:add_cap
/* [0x000014f8] */ 0x0001ff7f, 0xe20229e7, // mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
/* [0x00001500] */ 0x829fd409, 0xd00b0820, // fsub.ifnn  r0, r2, r0           ; mov.ifn  r0, r1 << rs2i_close_x
/* [0x00001508] */ 0x809ff009, 0xd00089e0, // nop                             ; mov.ifz  r0, (r1 << rs2i_close_y) >> 7
// :1
/* [0x00001510] */ 0x0d982dc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_phase_i ; nop
/* [0x00001518] */ 0x14041dc0, 0xd00429e7, // and.ifz.setf  -, ra_scalar2, 0x1 ; nop
/* [0x00001520] */ 0x00002778, 0xf02802a7, // brr.anyz  ra_temp, r:add_cap
/* [0x00001528] */ 0x0d981dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_q_lr ; nop
/* [0x00001530] */ 0x152a7d80, 0x100407a7, // mov.ifz  ra_scalar, ra_temp ; nop
/* [0x00001538] */ 0x15067d80, 0x10020827, // mov  r0, ra_scalar2 ; nop
/* [0x00001540] */ 0x807be036, 0xd00049e5, // nop ; mov  r5rep, ra_scalar << rsi_chunk_lr
/* [0x00001548] */ 0x159e7b40, 0x100202a7, // mov  ra_temp, r5
/* [0x00001550] */ 0x009e7000, 0x100009e7, // nop
/* [0x00001558] */ 0x00000000, 0xf0f549e7, // bra  -, ra_temp
/* [0x00001560] */ 0x009e7000, 0x100009e7, // nop
/* [0x00001568] */ 0x009e7000, 0x100009e7, // nop
/* [0x00001570] */ 0x009e7000, 0x100009e7, // nop
// :cubic_subd_0
/* [0x00001578] */ 0x952f2d80, 0xd00269e2, // mov.setf  -, ra_subd_flags2   ; mov  r2, r0 >> 2
/* [0x00001580] */ 0x829e7400, 0x10044841, // fsub.ifz  r1, r2, r0          ; mov  rb_cubics[i], r0
/* [0x00001588] */ 0x219e7089, 0x10025806, // fadd  r0, r0, r2              ; fmul  ra_temp3, r1, r1
/* [0x00001590] */ 0x959fc009, 0xd00242a2, // mov  ra_temp, r0              ; mov  r2, r1 << 4
/* [0x00001598] */ 0x829f2280, 0xd0024223, // fsub  ra_temp4, r1, r2        ; mov  r3, r0 >> 2
/* [0x000015a0] */ 0x819ff0c9, 0xd0028122, // fadd  ra_temp2, r0, r3        ; mov.ifz  r2, r1 << 1
/* [0x000015a8] */ 0x229c0e8a, 0xd00648a0, // fsub.ifnz  r2, f(0.0), r2     ; fmul  r0, r1, r2
/* [0x000015b0] */ 0x957bdd89, 0xd00288e2, // mov  r3, ra_scalar            ; mov.ifz  r2, r1 << 3
/* [0x000015b8] */ 0x801bf036, 0xd00049e5, // nop                           ; mov  r5rep, ra_temp3 << 1
/* [0x000015c0] */ 0x211b1b8a, 0xd0025962, // fadd  r5rep, r5, ra_temp3     ; fmul  r2 << 1, r1, r2
/* [0x000015c8] */ 0x222270b6, 0x10028821, // fsub  r0, r0, r2              ; fmul.ifz  r1, ra_temp4, ra_temp4
/* [0x000015d0] */ 0x3523fb76, 0xd00a88ca, // mov.ifnn  r3, r5              ; fmul.ifz  rb_temp >> 1, ra_temp4, ra_temp4  @mul_used(0)
/* [0x000015d8] */ 0x22367d40, 0x100252e2, // fsub  rb_temp2, ra_subd_consts, r5 ; fmul  r2, r0, r0
/* [0x000015e0] */ 0x2134a3d6, 0x10025946, // fadd  r5rep, r1, rb_temp      ; fmul  ra_temp3, r2, ra_subd_consts
/* [0x000015e8] */ 0x95132b76, 0xd0024d62, // mov  rsqrt, r5                ; mov  r2, ra_temp2 >> 2                      @mul_used(6, 7, 14, 15)
/* [0x000015f0] */ 0x8218bdc0, 0x100939c6, // fsub.ifn.setf  -, ra_temp3, rb_temp2 ; mov.ifn  ra_temp3, r0
/* [0x000015f8] */ 0x811005bf, 0x10030823, // fadd  r0, r2, ra_temp2        ; mov.ifn  r3, rb_scalar
/* [0x00001600] */ 0x811a77a4, 0x100269e3, // fadd.setf  -, r3, ra_temp3    ; mov  r3, r4
/* [0x00001608] */ 0x08820882, 0xe20829e7, // mov.ifn.setf  -, [0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0]
/* [0x00001610] */ 0x00000000, 0xf05749e7, // bra.allnn  -, ra_cubics[i]
/* [0x00001618] */ 0x8221fbf6, 0x100269e2, // fsub.setf  -, r5, rb_eps      ; mov  r2, ra_temp4
/* [0x00001620] */ 0x359fa253, 0xd00948a2, // mov.ifn  r2, r1               ; fmul.ifnn  r2 << 10, r2, r3                 @mul_used(10, 11)
/* [0x00001628] */ 0x1325ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags1 ; nop
/* [0x00001630] */ 0x219edfc7, 0xd00248e0, // fadd  r3, f(0.125), f(0.125) ; fmul  r0, r0, f(0.125)
/* [0x00001638] */ 0x202af037, 0xd00059ca, // nop                          ; fmul  ra_temp, ra_temp, f(0.5)
/* [0x00001640] */ 0x35101ff3, 0x10024123, // mov  ra_temp2, rb_cubics[i]  ; fmul  r3, ra_temp2, r3
/* [0x00001648] */ 0x952be6f6, 0xd0048801, // mov.ifz  r0, r3              ; mov.ifz  rb_cubics[i], ra_temp << 2  @mul_used(4, 5, 12, 13)
/* [0x00001650] */ 0x952bcd9b, 0xd0090801, // mov.ifn  r0, ra_temp         ; mov.ifn  rb_cubics[i], r3 << 4
/* [0x00001658] */ 0x9513ad80, 0xd00d8801, // mov.ifc  r0, ra_temp2        ; mov.ifc  rb_cubics[i], r0 << 6       @mul_used(0, 1, 8, 9)
/* [0x00001660] */ 0x00000048, 0xf0f80627, // brr  ra_cubics[i + 1], r:1f
/* [0x00001668] */ 0x809fe009, 0xd00099da, // nop                          ; mov.ifz  ra_cubics[i], r1 << 2
/* [0x00001670] */ 0x809fe012, 0xd00199da, // nop                          ; mov.ifc  ra_cubics[i], r2 << 2
/* [0x00001678] */ 0x809f4012, 0xd00149e1, // nop                          ; mov.ifnn  r1, r2 >> 4
/* [0x00001680] */ 0x95681dbf, 0x10024822, // mov  r0, ra_cubics[i] ; mov  r2, rb_cubics[i]
/* [0x00001688] */ 0x0001ff7f, 0xe20229e7, // mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
/* [0x00001690] */ 0x809f6000, 0xd00049e1, // nop                   ; mov  r1, r0 >> 6
/* [0x00001698] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
/* [0x000016a0] */ 0x959f6492, 0xd0088861, // mov.ifn  r1, r2       ; mov.ifz  r1, r2 >> 6
/* [0x000016a8] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
/* [0x000016b0] */ 0x152a7d80, 0x10040067, // mov.ifz  ra_scalar2, ra_temp ; nop
/* [0x000016b8] */ 0x95681ff6, 0x10025818, // mov  r0, rb_cubics[i] ; mov  ra_cubics[i + 1], ra_cubics[i]
/* [0x000016c0] */ 0x806b2036, 0xd00049e1, // nop                   ; mov  r1, ra_cubics[i] >> 2  @mul_used(10, 11, 14, 15)
// :1
// :cubic_subd_1
/* [0x000016c8] */ 0x952f2d80, 0xd00269e2, // mov.setf  -, ra_subd_flags2   ; mov  r2, r0 >> 2
/* [0x000016d0] */ 0x829e7400, 0x10044842, // fsub.ifz  r1, r2, r0          ; mov  rb_cubics[i], r0
/* [0x000016d8] */ 0x219e7089, 0x10025806, // fadd  r0, r0, r2              ; fmul  ra_temp3, r1, r1
/* [0x000016e0] */ 0x959fc009, 0xd00242a2, // mov  ra_temp, r0              ; mov  r2, r1 << 4
/* [0x000016e8] */ 0x829f2280, 0xd0024223, // fsub  ra_temp4, r1, r2        ; mov  r3, r0 >> 2
/* [0x000016f0] */ 0x819ff0c9, 0xd0028122, // fadd  ra_temp2, r0, r3        ; mov.ifz  r2, r1 << 1
/* [0x000016f8] */ 0x229c0e8a, 0xd00648a0, // fsub.ifnz  r2, f(0.0), r2     ; fmul  r0, r1, r2
/* [0x00001700] */ 0x957bdd89, 0xd00288e2, // mov  r3, ra_scalar            ; mov.ifz  r2, r1 << 3
/* [0x00001708] */ 0x801bf036, 0xd00049e5, // nop                           ; mov  r5rep, ra_temp3 << 1
/* [0x00001710] */ 0x211b1b8a, 0xd0025962, // fadd  r5rep, r5, ra_temp3     ; fmul  r2 << 1, r1, r2
/* [0x00001718] */ 0x222270b6, 0x10028821, // fsub  r0, r0, r2              ; fmul.ifz  r1, ra_temp4, ra_temp4
/* [0x00001720] */ 0x3523fb76, 0xd00a88ca, // mov.ifnn  r3, r5              ; fmul.ifz  rb_temp >> 1, ra_temp4, ra_temp4  @mul_used(0)
/* [0x00001728] */ 0x22367d40, 0x100252e2, // fsub  rb_temp2, ra_subd_consts, r5 ; fmul  r2, r0, r0
/* [0x00001730] */ 0x2134a3d6, 0x10025946, // fadd  r5rep, r1, rb_temp      ; fmul  ra_temp3, r2, ra_subd_consts
/* [0x00001738] */ 0x95132b76, 0xd0024d62, // mov  rsqrt, r5                ; mov  r2, ra_temp2 >> 2                      @mul_used(6, 7, 14, 15)
/* [0x00001740] */ 0x8218bdc0, 0x100939c6, // fsub.ifn.setf  -, ra_temp3, rb_temp2 ; mov.ifn  ra_temp3, r0
/* [0x00001748] */ 0x811005bf, 0x10030823, // fadd  r0, r2, ra_temp2        ; mov.ifn  r3, rb_scalar
/* [0x00001750] */ 0x811a77a4, 0x100269e3, // fadd.setf  -, r3, ra_temp3    ; mov  r3, r4
/* [0x00001758] */ 0x08820882, 0xe20829e7, // mov.ifn.setf  -, [0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0]
/* [0x00001760] */ 0x00000000, 0xf05709e7, // bra.allnn  -, ra_cubics[i]
/* [0x00001768] */ 0x8221fbf6, 0x100269e2, // fsub.setf  -, r5, rb_eps      ; mov  r2, ra_temp4
/* [0x00001770] */ 0x359fa253, 0xd00948a2, // mov.ifn  r2, r1               ; fmul.ifnn  r2 << 10, r2, r3                 @mul_used(10, 11)
/* [0x00001778] */ 0x1325ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags1 ; nop
/* [0x00001780] */ 0x219edfc7, 0xd00248e0, // fadd  r3, f(0.125), f(0.125) ; fmul  r0, r0, f(0.125)
/* [0x00001788] */ 0x202af037, 0xd00059ca, // nop                          ; fmul  ra_temp, ra_temp, f(0.5)
/* [0x00001790] */ 0x35102ff3, 0x10024123, // mov  ra_temp2, rb_cubics[i]  ; fmul  r3, ra_temp2, r3
/* [0x00001798] */ 0x952be6f6, 0xd0048802, // mov.ifz  r0, r3              ; mov.ifz  rb_cubics[i], ra_temp << 2  @mul_used(4, 5, 12, 13)
/* [0x000017a0] */ 0x952bcd9b, 0xd0090802, // mov.ifn  r0, ra_temp         ; mov.ifn  rb_cubics[i], r3 << 4
/* [0x000017a8] */ 0x9513ad80, 0xd00d8802, // mov.ifc  r0, ra_temp2        ; mov.ifc  rb_cubics[i], r0 << 6       @mul_used(0, 1, 8, 9)
/* [0x000017b0] */ 0x00000048, 0xf0f805a7, // brr  ra_cubics[i + 1], r:1f
/* [0x000017b8] */ 0x809fe009, 0xd00099d8, // nop                          ; mov.ifz  ra_cubics[i], r1 << 2
/* [0x000017c0] */ 0x809fe012, 0xd00199d8, // nop                          ; mov.ifc  ra_cubics[i], r2 << 2
/* [0x000017c8] */ 0x809f4012, 0xd00149e1, // nop                          ; mov.ifnn  r1, r2 >> 4
/* [0x000017d0] */ 0x95602dbf, 0x10024822, // mov  r0, ra_cubics[i] ; mov  r2, rb_cubics[i]
/* [0x000017d8] */ 0x0001ff7f, 0xe20229e7, // mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
/* [0x000017e0] */ 0x809f6000, 0xd00049e1, // nop                   ; mov  r1, r0 >> 6
/* [0x000017e8] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
/* [0x000017f0] */ 0x959f6492, 0xd0088861, // mov.ifn  r1, r2       ; mov.ifz  r1, r2 >> 6
/* [0x000017f8] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
/* [0x00001800] */ 0x152a7d80, 0x10040067, // mov.ifz  ra_scalar2, ra_temp ; nop
/* [0x00001808] */ 0x95602ff6, 0x10025816, // mov  r0, rb_cubics[i] ; mov  ra_cubics[i + 1], ra_cubics[i]
/* [0x00001810] */ 0x80632036, 0xd00049e1, // nop                   ; mov  r1, ra_cubics[i] >> 2  @mul_used(10, 11, 14, 15)
// :1
/* [0x00001818] */ 0x952f2d80, 0xd00269e2, // mov.setf  -, ra_subd_flags2   ; mov  r2, r0 >> 2
/* [0x00001820] */ 0x829e7400, 0x10044843, // fsub.ifz  r1, r2, r0          ; mov  rb_cubics[i], r0
/* [0x00001828] */ 0x219e7089, 0x10025806, // fadd  r0, r0, r2              ; fmul  ra_temp3, r1, r1
/* [0x00001830] */ 0x959fc009, 0xd00242a2, // mov  ra_temp, r0              ; mov  r2, r1 << 4
/* [0x00001838] */ 0x829f2280, 0xd0024223, // fsub  ra_temp4, r1, r2        ; mov  r3, r0 >> 2
/* [0x00001840] */ 0x819ff0c9, 0xd0028122, // fadd  ra_temp2, r0, r3        ; mov.ifz  r2, r1 << 1
/* [0x00001848] */ 0x229c0e8a, 0xd00648a0, // fsub.ifnz  r2, f(0.0), r2     ; fmul  r0, r1, r2
/* [0x00001850] */ 0x957bdd89, 0xd00288e2, // mov  r3, ra_scalar            ; mov.ifz  r2, r1 << 3
/* [0x00001858] */ 0x801bf036, 0xd00049e5, // nop                           ; mov  r5rep, ra_temp3 << 1
/* [0x00001860] */ 0x211b1b8a, 0xd0025962, // fadd  r5rep, r5, ra_temp3     ; fmul  r2 << 1, r1, r2
/* [0x00001868] */ 0x222270b6, 0x10028821, // fsub  r0, r0, r2              ; fmul.ifz  r1, ra_temp4, ra_temp4
/* [0x00001870] */ 0x3523fb76, 0xd00a88ca, // mov.ifnn  r3, r5              ; fmul.ifz  rb_temp >> 1, ra_temp4, ra_temp4  @mul_used(0)
/* [0x00001878] */ 0x22367d40, 0x100252e2, // fsub  rb_temp2, ra_subd_consts, r5 ; fmul  r2, r0, r0
/* [0x00001880] */ 0x2134a3d6, 0x10025946, // fadd  r5rep, r1, rb_temp      ; fmul  ra_temp3, r2, ra_subd_consts
/* [0x00001888] */ 0x95132b76, 0xd0024d62, // mov  rsqrt, r5                ; mov  r2, ra_temp2 >> 2                      @mul_used(6, 7, 14, 15)
/* [0x00001890] */ 0x8218bdc0, 0x100939c6, // fsub.ifn.setf  -, ra_temp3, rb_temp2 ; mov.ifn  ra_temp3, r0
/* [0x00001898] */ 0x811005bf, 0x10030823, // fadd  r0, r2, ra_temp2        ; mov.ifn  r3, rb_scalar
/* [0x000018a0] */ 0x811a77a4, 0x100269e3, // fadd.setf  -, r3, ra_temp3    ; mov  r3, r4
/* [0x000018a8] */ 0x08820882, 0xe20829e7, // mov.ifn.setf  -, [0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0]
/* [0x000018b0] */ 0x00000000, 0xf056c9e7, // bra.allnn  -, ra_cubics[i]
/* [0x000018b8] */ 0x8221fbf6, 0x100269e2, // fsub.setf  -, r5, rb_eps      ; mov  r2, ra_temp4
/* [0x000018c0] */ 0x359fa253, 0xd00948a2, // mov.ifn  r2, r1               ; fmul.ifnn  r2 << 10, r2, r3                 @mul_used(10, 11)
/* [0x000018c8] */ 0x1325ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags1 ; nop
/* [0x000018d0] */ 0x219edfc7, 0xd00248e0, // fadd  r3, f(0.125), f(0.125) ; fmul  r0, r0, f(0.125)
/* [0x000018d8] */ 0x202af037, 0xd00059ca, // nop                          ; fmul  ra_temp, ra_temp, f(0.5)
/* [0x000018e0] */ 0x35103ff3, 0x10024123, // mov  ra_temp2, rb_cubics[i]  ; fmul  r3, ra_temp2, r3
/* [0x000018e8] */ 0x952be6f6, 0xd0048803, // mov.ifz  r0, r3              ; mov.ifz  rb_cubics[i], ra_temp << 2  @mul_used(4, 5, 12, 13)
/* [0x000018f0] */ 0x952bcd9b, 0xd0090803, // mov.ifn  r0, ra_temp         ; mov.ifn  rb_cubics[i], r3 << 4
/* [0x000018f8] */ 0x9513ad80, 0xd00d8803, // mov.ifc  r0, ra_temp2        ; mov.ifc  rb_cubics[i], r0 << 6       @mul_used(0, 1, 8, 9)
/* [0x00001900] */ 0x00000048, 0xf0f80527, // brr  ra_cubics[i + 1], r:1f
/* [0x00001908] */ 0x809fe009, 0xd00099d6, // nop                          ; mov.ifz  ra_cubics[i], r1 << 2
/* [0x00001910] */ 0x809fe012, 0xd00199d6, // nop                          ; mov.ifc  ra_cubics[i], r2 << 2
/* [0x00001918] */ 0x809f4012, 0xd00149e1, // nop                          ; mov.ifnn  r1, r2 >> 4
/* [0x00001920] */ 0x95583dbf, 0x10024822, // mov  r0, ra_cubics[i] ; mov  r2, rb_cubics[i]
/* [0x00001928] */ 0x0001ff7f, 0xe20229e7, // mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
/* [0x00001930] */ 0x809f6000, 0xd00049e1, // nop                   ; mov  r1, r0 >> 6
/* [0x00001938] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
/* [0x00001940] */ 0x959f6492, 0xd0088861, // mov.ifn  r1, r2       ; mov.ifz  r1, r2 >> 6
/* [0x00001948] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
/* [0x00001950] */ 0x152a7d80, 0x10040067, // mov.ifz  ra_scalar2, ra_temp ; nop
/* [0x00001958] */ 0x95583ff6, 0x10025814, // mov  r0, rb_cubics[i] ; mov  ra_cubics[i + 1], ra_cubics[i]
/* [0x00001960] */ 0x805b2036, 0xd00049e1, // nop                   ; mov  r1, ra_cubics[i] >> 2  @mul_used(10, 11, 14, 15)
// :1
/* [0x00001968] */ 0x952f2d80, 0xd00269e2, // mov.setf  -, ra_subd_flags2   ; mov  r2, r0 >> 2
/* [0x00001970] */ 0x829e7400, 0x10044844, // fsub.ifz  r1, r2, r0          ; mov  rb_cubics[i], r0
/* [0x00001978] */ 0x219e7089, 0x10025806, // fadd  r0, r0, r2              ; fmul  ra_temp3, r1, r1
/* [0x00001980] */ 0x959fc009, 0xd00242a2, // mov  ra_temp, r0              ; mov  r2, r1 << 4
/* [0x00001988] */ 0x829f2280, 0xd0024223, // fsub  ra_temp4, r1, r2        ; mov  r3, r0 >> 2
/* [0x00001990] */ 0x819ff0c9, 0xd0028122, // fadd  ra_temp2, r0, r3        ; mov.ifz  r2, r1 << 1
/* [0x00001998] */ 0x229c0e8a, 0xd00648a0, // fsub.ifnz  r2, f(0.0), r2     ; fmul  r0, r1, r2
/* [0x000019a0] */ 0x957bdd89, 0xd00288e2, // mov  r3, ra_scalar            ; mov.ifz  r2, r1 << 3
/* [0x000019a8] */ 0x801bf036, 0xd00049e5, // nop                           ; mov  r5rep, ra_temp3 << 1
/* [0x000019b0] */ 0x211b1b8a, 0xd0025962, // fadd  r5rep, r5, ra_temp3     ; fmul  r2 << 1, r1, r2
/* [0x000019b8] */ 0x222270b6, 0x10028821, // fsub  r0, r0, r2              ; fmul.ifz  r1, ra_temp4, ra_temp4
/* [0x000019c0] */ 0x3523fb76, 0xd00a88ca, // mov.ifnn  r3, r5              ; fmul.ifz  rb_temp >> 1, ra_temp4, ra_temp4  @mul_used(0)
/* [0x000019c8] */ 0x22367d40, 0x100252e2, // fsub  rb_temp2, ra_subd_consts, r5 ; fmul  r2, r0, r0
/* [0x000019d0] */ 0x2134a3d6, 0x10025946, // fadd  r5rep, r1, rb_temp      ; fmul  ra_temp3, r2, ra_subd_consts
/* [0x000019d8] */ 0x95132b76, 0xd0024d62, // mov  rsqrt, r5                ; mov  r2, ra_temp2 >> 2                      @mul_used(6, 7, 14, 15)
/* [0x000019e0] */ 0x8218bdc0, 0x100939c6, // fsub.ifn.setf  -, ra_temp3, rb_temp2 ; mov.ifn  ra_temp3, r0
/* [0x000019e8] */ 0x811005bf, 0x10030823, // fadd  r0, r2, ra_temp2        ; mov.ifn  r3, rb_scalar
/* [0x000019f0] */ 0x811a77a4, 0x100269e3, // fadd.setf  -, r3, ra_temp3    ; mov  r3, r4
/* [0x000019f8] */ 0x08820882, 0xe20829e7, // mov.ifn.setf  -, [0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0]
/* [0x00001a00] */ 0x00000000, 0xf05689e7, // bra.allnn  -, ra_cubics[i]
/* [0x00001a08] */ 0x8221fbf6, 0x100269e2, // fsub.setf  -, r5, rb_eps      ; mov  r2, ra_temp4
/* [0x00001a10] */ 0x359fa253, 0xd00948a2, // mov.ifn  r2, r1               ; fmul.ifnn  r2 << 10, r2, r3                 @mul_used(10, 11)
/* [0x00001a18] */ 0x1325ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags1 ; nop
/* [0x00001a20] */ 0x219edfc7, 0xd00248e0, // fadd  r3, f(0.125), f(0.125) ; fmul  r0, r0, f(0.125)
/* [0x00001a28] */ 0x202af037, 0xd00059ca, // nop                          ; fmul  ra_temp, ra_temp, f(0.5)
/* [0x00001a30] */ 0x35104ff3, 0x10024123, // mov  ra_temp2, rb_cubics[i]  ; fmul  r3, ra_temp2, r3
/* [0x00001a38] */ 0x952be6f6, 0xd0048804, // mov.ifz  r0, r3              ; mov.ifz  rb_cubics[i], ra_temp << 2  @mul_used(4, 5, 12, 13)
/* [0x00001a40] */ 0x952bcd9b, 0xd0090804, // mov.ifn  r0, ra_temp         ; mov.ifn  rb_cubics[i], r3 << 4
/* [0x00001a48] */ 0x9513ad80, 0xd00d8804, // mov.ifc  r0, ra_temp2        ; mov.ifc  rb_cubics[i], r0 << 6       @mul_used(0, 1, 8, 9)
/* [0x00001a50] */ 0x00000048, 0xf0f804a7, // brr  ra_cubics[i + 1], r:1f
/* [0x00001a58] */ 0x809fe009, 0xd00099d4, // nop                          ; mov.ifz  ra_cubics[i], r1 << 2
/* [0x00001a60] */ 0x809fe012, 0xd00199d4, // nop                          ; mov.ifc  ra_cubics[i], r2 << 2
/* [0x00001a68] */ 0x809f4012, 0xd00149e1, // nop                          ; mov.ifnn  r1, r2 >> 4
/* [0x00001a70] */ 0x95504dbf, 0x10024822, // mov  r0, ra_cubics[i] ; mov  r2, rb_cubics[i]
/* [0x00001a78] */ 0x0001ff7f, 0xe20229e7, // mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
/* [0x00001a80] */ 0x809f6000, 0xd00049e1, // nop                   ; mov  r1, r0 >> 6
/* [0x00001a88] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
/* [0x00001a90] */ 0x959f6492, 0xd0088861, // mov.ifn  r1, r2       ; mov.ifz  r1, r2 >> 6
/* [0x00001a98] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
/* [0x00001aa0] */ 0x152a7d80, 0x10040067, // mov.ifz  ra_scalar2, ra_temp ; nop
/* [0x00001aa8] */ 0x95504ff6, 0x10025812, // mov  r0, rb_cubics[i] ; mov  ra_cubics[i + 1], ra_cubics[i]
/* [0x00001ab0] */ 0x80532036, 0xd00049e1, // nop                   ; mov  r1, ra_cubics[i] >> 2  @mul_used(10, 11, 14, 15)
// :1
/* [0x00001ab8] */ 0x952f2d80, 0xd00269e2, // mov.setf  -, ra_subd_flags2   ; mov  r2, r0 >> 2
/* [0x00001ac0] */ 0x829e7400, 0x10044845, // fsub.ifz  r1, r2, r0          ; mov  rb_cubics[i], r0
/* [0x00001ac8] */ 0x219e7089, 0x10025806, // fadd  r0, r0, r2              ; fmul  ra_temp3, r1, r1
/* [0x00001ad0] */ 0x959fc009, 0xd00242a2, // mov  ra_temp, r0              ; mov  r2, r1 << 4
/* [0x00001ad8] */ 0x829f2280, 0xd0024223, // fsub  ra_temp4, r1, r2        ; mov  r3, r0 >> 2
/* [0x00001ae0] */ 0x819ff0c9, 0xd0028122, // fadd  ra_temp2, r0, r3        ; mov.ifz  r2, r1 << 1
/* [0x00001ae8] */ 0x229c0e8a, 0xd00648a0, // fsub.ifnz  r2, f(0.0), r2     ; fmul  r0, r1, r2
/* [0x00001af0] */ 0x957bdd89, 0xd00288e2, // mov  r3, ra_scalar            ; mov.ifz  r2, r1 << 3
/* [0x00001af8] */ 0x801bf036, 0xd00049e5, // nop                           ; mov  r5rep, ra_temp3 << 1
/* [0x00001b00] */ 0x211b1b8a, 0xd0025962, // fadd  r5rep, r5, ra_temp3     ; fmul  r2 << 1, r1, r2
/* [0x00001b08] */ 0x222270b6, 0x10028821, // fsub  r0, r0, r2              ; fmul.ifz  r1, ra_temp4, ra_temp4
/* [0x00001b10] */ 0x3523fb76, 0xd00a88ca, // mov.ifnn  r3, r5              ; fmul.ifz  rb_temp >> 1, ra_temp4, ra_temp4  @mul_used(0)
/* [0x00001b18] */ 0x22367d40, 0x100252e2, // fsub  rb_temp2, ra_subd_consts, r5 ; fmul  r2, r0, r0
/* [0x00001b20] */ 0x2134a3d6, 0x10025946, // fadd  r5rep, r1, rb_temp      ; fmul  ra_temp3, r2, ra_subd_consts
/* [0x00001b28] */ 0x95132b76, 0xd0024d62, // mov  rsqrt, r5                ; mov  r2, ra_temp2 >> 2                      @mul_used(6, 7, 14, 15)
/* [0x00001b30] */ 0x8218bdc0, 0x100939c6, // fsub.ifn.setf  -, ra_temp3, rb_temp2 ; mov.ifn  ra_temp3, r0
/* [0x00001b38] */ 0x811005bf, 0x10030823, // fadd  r0, r2, ra_temp2        ; mov.ifn  r3, rb_scalar
/* [0x00001b40] */ 0x811a77a4, 0x100269e3, // fadd.setf  -, r3, ra_temp3    ; mov  r3, r4
/* [0x00001b48] */ 0x08820882, 0xe20829e7, // mov.ifn.setf  -, [0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0]
/* [0x00001b50] */ 0x00000000, 0xf05649e7, // bra.allnn  -, ra_cubics[i]
/* [0x00001b58] */ 0x8221fbf6, 0x100269e2, // fsub.setf  -, r5, rb_eps      ; mov  r2, ra_temp4
/* [0x00001b60] */ 0x359fa253, 0xd00948a2, // mov.ifn  r2, r1               ; fmul.ifnn  r2 << 10, r2, r3                 @mul_used(10, 11)
/* [0x00001b68] */ 0x1325ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags1 ; nop
/* [0x00001b70] */ 0x219edfc7, 0xd00248e0, // fadd  r3, f(0.125), f(0.125) ; fmul  r0, r0, f(0.125)
/* [0x00001b78] */ 0x202af037, 0xd00059ca, // nop                          ; fmul  ra_temp, ra_temp, f(0.5)
/* [0x00001b80] */ 0x35105ff3, 0x10024123, // mov  ra_temp2, rb_cubics[i]  ; fmul  r3, ra_temp2, r3
/* [0x00001b88] */ 0x952be6f6, 0xd0048805, // mov.ifz  r0, r3              ; mov.ifz  rb_cubics[i], ra_temp << 2  @mul_used(4, 5, 12, 13)
/* [0x00001b90] */ 0x952bcd9b, 0xd0090805, // mov.ifn  r0, ra_temp         ; mov.ifn  rb_cubics[i], r3 << 4
/* [0x00001b98] */ 0x9513ad80, 0xd00d8805, // mov.ifc  r0, ra_temp2        ; mov.ifc  rb_cubics[i], r0 << 6       @mul_used(0, 1, 8, 9)
/* [0x00001ba0] */ 0x00000048, 0xf0f80427, // brr  ra_cubics[i + 1], r:1f
/* [0x00001ba8] */ 0x809fe009, 0xd00099d2, // nop                          ; mov.ifz  ra_cubics[i], r1 << 2
/* [0x00001bb0] */ 0x809fe012, 0xd00199d2, // nop                          ; mov.ifc  ra_cubics[i], r2 << 2
/* [0x00001bb8] */ 0x809f4012, 0xd00149e1, // nop                          ; mov.ifnn  r1, r2 >> 4
/* [0x00001bc0] */ 0x95485dbf, 0x10024822, // mov  r0, ra_cubics[i] ; mov  r2, rb_cubics[i]
/* [0x00001bc8] */ 0x0001ff7f, 0xe20229e7, // mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
/* [0x00001bd0] */ 0x809f6000, 0xd00049e1, // nop                   ; mov  r1, r0 >> 6
/* [0x00001bd8] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
/* [0x00001be0] */ 0x959f6492, 0xd0088861, // mov.ifn  r1, r2       ; mov.ifz  r1, r2 >> 6
/* [0x00001be8] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
/* [0x00001bf0] */ 0x152a7d80, 0x10040067, // mov.ifz  ra_scalar2, ra_temp ; nop
/* [0x00001bf8] */ 0x95485ff6, 0x10025810, // mov  r0, rb_cubics[i] ; mov  ra_cubics[i + 1], ra_cubics[i]
/* [0x00001c00] */ 0x804b2036, 0xd00049e1, // nop                   ; mov  r1, ra_cubics[i] >> 2  @mul_used(10, 11, 14, 15)
// :1
/* [0x00001c08] */ 0x952f2d80, 0xd00269e2, // mov.setf  -, ra_subd_flags2   ; mov  r2, r0 >> 2
/* [0x00001c10] */ 0x829e7400, 0x10044846, // fsub.ifz  r1, r2, r0          ; mov  rb_cubics[i], r0
/* [0x00001c18] */ 0x219e7089, 0x10025806, // fadd  r0, r0, r2              ; fmul  ra_temp3, r1, r1
/* [0x00001c20] */ 0x959fc009, 0xd00242a2, // mov  ra_temp, r0              ; mov  r2, r1 << 4
/* [0x00001c28] */ 0x829f2280, 0xd0024223, // fsub  ra_temp4, r1, r2        ; mov  r3, r0 >> 2
/* [0x00001c30] */ 0x819ff0c9, 0xd0028122, // fadd  ra_temp2, r0, r3        ; mov.ifz  r2, r1 << 1
/* [0x00001c38] */ 0x229c0e8a, 0xd00648a0, // fsub.ifnz  r2, f(0.0), r2     ; fmul  r0, r1, r2
/* [0x00001c40] */ 0x957bdd89, 0xd00288e2, // mov  r3, ra_scalar            ; mov.ifz  r2, r1 << 3
/* [0x00001c48] */ 0x801bf036, 0xd00049e5, // nop                           ; mov  r5rep, ra_temp3 << 1
/* [0x00001c50] */ 0x211b1b8a, 0xd0025962, // fadd  r5rep, r5, ra_temp3     ; fmul  r2 << 1, r1, r2
/* [0x00001c58] */ 0x222270b6, 0x10028821, // fsub  r0, r0, r2              ; fmul.ifz  r1, ra_temp4, ra_temp4
/* [0x00001c60] */ 0x3523fb76, 0xd00a88ca, // mov.ifnn  r3, r5              ; fmul.ifz  rb_temp >> 1, ra_temp4, ra_temp4  @mul_used(0)
/* [0x00001c68] */ 0x22367d40, 0x100252e2, // fsub  rb_temp2, ra_subd_consts, r5 ; fmul  r2, r0, r0
/* [0x00001c70] */ 0x2134a3d6, 0x10025946, // fadd  r5rep, r1, rb_temp      ; fmul  ra_temp3, r2, ra_subd_consts
/* [0x00001c78] */ 0x95132b76, 0xd0024d62, // mov  rsqrt, r5                ; mov  r2, ra_temp2 >> 2                      @mul_used(6, 7, 14, 15)
/* [0x00001c80] */ 0x8218bdc0, 0x100939c6, // fsub.ifn.setf  -, ra_temp3, rb_temp2 ; mov.ifn  ra_temp3, r0
/* [0x00001c88] */ 0x811005bf, 0x10030823, // fadd  r0, r2, ra_temp2        ; mov.ifn  r3, rb_scalar
/* [0x00001c90] */ 0x811a77a4, 0x100269e3, // fadd.setf  -, r3, ra_temp3    ; mov  r3, r4
/* [0x00001c98] */ 0x08820882, 0xe20829e7, // mov.ifn.setf  -, [0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0]
/* [0x00001ca0] */ 0x00000000, 0xf05609e7, // bra.allnn  -, ra_cubics[i]
/* [0x00001ca8] */ 0x8221fbf6, 0x100269e2, // fsub.setf  -, r5, rb_eps      ; mov  r2, ra_temp4
/* [0x00001cb0] */ 0x359fa253, 0xd00948a2, // mov.ifn  r2, r1               ; fmul.ifnn  r2 << 10, r2, r3                 @mul_used(10, 11)
/* [0x00001cb8] */ 0x1325ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags1 ; nop
/* [0x00001cc0] */ 0x219edfc7, 0xd00248e0, // fadd  r3, f(0.125), f(0.125) ; fmul  r0, r0, f(0.125)
/* [0x00001cc8] */ 0x202af037, 0xd00059ca, // nop                          ; fmul  ra_temp, ra_temp, f(0.5)
/* [0x00001cd0] */ 0x35106ff3, 0x10024123, // mov  ra_temp2, rb_cubics[i]  ; fmul  r3, ra_temp2, r3
/* [0x00001cd8] */ 0x952be6f6, 0xd0048806, // mov.ifz  r0, r3              ; mov.ifz  rb_cubics[i], ra_temp << 2  @mul_used(4, 5, 12, 13)
/* [0x00001ce0] */ 0x952bcd9b, 0xd0090806, // mov.ifn  r0, ra_temp         ; mov.ifn  rb_cubics[i], r3 << 4
/* [0x00001ce8] */ 0x9513ad80, 0xd00d8806, // mov.ifc  r0, ra_temp2        ; mov.ifc  rb_cubics[i], r0 << 6       @mul_used(0, 1, 8, 9)
/* [0x00001cf0] */ 0x00000048, 0xf0f803a7, // brr  ra_cubics[i + 1], r:1f
/* [0x00001cf8] */ 0x809fe009, 0xd00099d0, // nop                          ; mov.ifz  ra_cubics[i], r1 << 2
/* [0x00001d00] */ 0x809fe012, 0xd00199d0, // nop                          ; mov.ifc  ra_cubics[i], r2 << 2
/* [0x00001d08] */ 0x809f4012, 0xd00149e1, // nop                          ; mov.ifnn  r1, r2 >> 4
/* [0x00001d10] */ 0x95406dbf, 0x10024822, // mov  r0, ra_cubics[i] ; mov  r2, rb_cubics[i]
/* [0x00001d18] */ 0x0001ff7f, 0xe20229e7, // mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
/* [0x00001d20] */ 0x809f6000, 0xd00049e1, // nop                   ; mov  r1, r0 >> 6
/* [0x00001d28] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
/* [0x00001d30] */ 0x959f6492, 0xd0088861, // mov.ifn  r1, r2       ; mov.ifz  r1, r2 >> 6
/* [0x00001d38] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
/* [0x00001d40] */ 0x152a7d80, 0x10040067, // mov.ifz  ra_scalar2, ra_temp ; nop
/* [0x00001d48] */ 0x95406ff6, 0x1002580e, // mov  r0, rb_cubics[i] ; mov  ra_cubics[i + 1], ra_cubics[i]
/* [0x00001d50] */ 0x80432036, 0xd00049e1, // nop                   ; mov  r1, ra_cubics[i] >> 2  @mul_used(10, 11, 14, 15)
// :1
/* [0x00001d58] */ 0x952f2d80, 0xd00269e2, // mov.setf  -, ra_subd_flags2   ; mov  r2, r0 >> 2
/* [0x00001d60] */ 0x829e7400, 0x10044847, // fsub.ifz  r1, r2, r0          ; mov  rb_cubics[i], r0
/* [0x00001d68] */ 0x219e7089, 0x10025806, // fadd  r0, r0, r2              ; fmul  ra_temp3, r1, r1
/* [0x00001d70] */ 0x959fc009, 0xd00242a2, // mov  ra_temp, r0              ; mov  r2, r1 << 4
/* [0x00001d78] */ 0x829f2280, 0xd0024223, // fsub  ra_temp4, r1, r2        ; mov  r3, r0 >> 2
/* [0x00001d80] */ 0x819ff0c9, 0xd0028122, // fadd  ra_temp2, r0, r3        ; mov.ifz  r2, r1 << 1
/* [0x00001d88] */ 0x229c0e8a, 0xd00648a0, // fsub.ifnz  r2, f(0.0), r2     ; fmul  r0, r1, r2
/* [0x00001d90] */ 0x957bdd89, 0xd00288e2, // mov  r3, ra_scalar            ; mov.ifz  r2, r1 << 3
/* [0x00001d98] */ 0x801bf036, 0xd00049e5, // nop                           ; mov  r5rep, ra_temp3 << 1
/* [0x00001da0] */ 0x211b1b8a, 0xd0025962, // fadd  r5rep, r5, ra_temp3     ; fmul  r2 << 1, r1, r2
/* [0x00001da8] */ 0x222270b6, 0x10028821, // fsub  r0, r0, r2              ; fmul.ifz  r1, ra_temp4, ra_temp4
/* [0x00001db0] */ 0x3523fb76, 0xd00a88ca, // mov.ifnn  r3, r5              ; fmul.ifz  rb_temp >> 1, ra_temp4, ra_temp4  @mul_used(0)
/* [0x00001db8] */ 0x22367d40, 0x100252e2, // fsub  rb_temp2, ra_subd_consts, r5 ; fmul  r2, r0, r0
/* [0x00001dc0] */ 0x2134a3d6, 0x10025946, // fadd  r5rep, r1, rb_temp      ; fmul  ra_temp3, r2, ra_subd_consts
/* [0x00001dc8] */ 0x95132b76, 0xd0024d62, // mov  rsqrt, r5                ; mov  r2, ra_temp2 >> 2                      @mul_used(6, 7, 14, 15)
/* [0x00001dd0] */ 0x8218bdc0, 0x100939c6, // fsub.ifn.setf  -, ra_temp3, rb_temp2 ; mov.ifn  ra_temp3, r0
/* [0x00001dd8] */ 0x811005bf, 0x10030823, // fadd  r0, r2, ra_temp2        ; mov.ifn  r3, rb_scalar
/* [0x00001de0] */ 0x811a77a4, 0x100269e3, // fadd.setf  -, r3, ra_temp3    ; mov  r3, r4
/* [0x00001de8] */ 0x08820882, 0xe20829e7, // mov.ifn.setf  -, [0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0]
/* [0x00001df0] */ 0x00000000, 0xf055c9e7, // bra.allnn  -, ra_cubics[i]
/* [0x00001df8] */ 0x8221fbf6, 0x100269e2, // fsub.setf  -, r5, rb_eps      ; mov  r2, ra_temp4
/* [0x00001e00] */ 0x359fa253, 0xd00948a2, // mov.ifn  r2, r1               ; fmul.ifnn  r2 << 10, r2, r3                 @mul_used(10, 11)
/* [0x00001e08] */ 0x1325ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags1 ; nop
/* [0x00001e10] */ 0x219edfc7, 0xd00248e0, // fadd  r3, f(0.125), f(0.125) ; fmul  r0, r0, f(0.125)
/* [0x00001e18] */ 0x202af037, 0xd00059ca, // nop                          ; fmul  ra_temp, ra_temp, f(0.5)
/* [0x00001e20] */ 0x35107ff3, 0x10024123, // mov  ra_temp2, rb_cubics[i]  ; fmul  r3, ra_temp2, r3
/* [0x00001e28] */ 0x952be6f6, 0xd0048807, // mov.ifz  r0, r3              ; mov.ifz  rb_cubics[i], ra_temp << 2  @mul_used(4, 5, 12, 13)
/* [0x00001e30] */ 0x952bcd9b, 0xd0090807, // mov.ifn  r0, ra_temp         ; mov.ifn  rb_cubics[i], r3 << 4
/* [0x00001e38] */ 0x9513ad80, 0xd00d8807, // mov.ifc  r0, ra_temp2        ; mov.ifc  rb_cubics[i], r0 << 6       @mul_used(0, 1, 8, 9)
/* [0x00001e40] */ 0x00000048, 0xf0f80327, // brr  ra_cubics[i + 1], r:1f
/* [0x00001e48] */ 0x809fe009, 0xd00099ce, // nop                          ; mov.ifz  ra_cubics[i], r1 << 2
/* [0x00001e50] */ 0x809fe012, 0xd00199ce, // nop                          ; mov.ifc  ra_cubics[i], r2 << 2
/* [0x00001e58] */ 0x809f4012, 0xd00149e1, // nop                          ; mov.ifnn  r1, r2 >> 4
/* [0x00001e60] */ 0x95387dbf, 0x10024822, // mov  r0, ra_cubics[i] ; mov  r2, rb_cubics[i]
/* [0x00001e68] */ 0x0001ff7f, 0xe20229e7, // mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
/* [0x00001e70] */ 0x809f6000, 0xd00049e1, // nop                   ; mov  r1, r0 >> 6
/* [0x00001e78] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_dash_clerp_lbl) >> 15
/* [0x00001e80] */ 0x959f6492, 0xd0088861, // mov.ifn  r1, r2       ; mov.ifz  r1, r2 >> 6
/* [0x00001e88] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
/* [0x00001e90] */ 0x152a7d80, 0x10040067, // mov.ifz  ra_scalar2, ra_temp ; nop
/* [0x00001e98] */ 0x95387ff6, 0x1002580c, // mov  r0, rb_cubics[i] ; mov  ra_cubics[i + 1], ra_cubics[i]
/* [0x00001ea0] */ 0x803b2036, 0xd00049e1, // nop                   ; mov  r1, ra_cubics[i] >> 2  @mul_used(10, 11, 14, 15)
// :1
/* [0x00001ea8] */ 0x952f2d80, 0xd00269e2, // mov.setf  -, ra_subd_flags2   ; mov  r2, r0 >> 2
/* [0x00001eb0] */ 0x829e7400, 0x10044848, // fsub.ifz  r1, r2, r0          ; mov  rb_cubics[i], r0
/* [0x00001eb8] */ 0x219e7089, 0x10025806, // fadd  r0, r0, r2              ; fmul  ra_temp3, r1, r1
/* [0x00001ec0] */ 0x959fc009, 0xd00242a2, // mov  ra_temp, r0              ; mov  r2, r1 << 4
/* [0x00001ec8] */ 0x829f2280, 0xd0024223, // fsub  ra_temp4, r1, r2        ; mov  r3, r0 >> 2
/* [0x00001ed0] */ 0x819ff0c9, 0xd0028122, // fadd  ra_temp2, r0, r3        ; mov.ifz  r2, r1 << 1
/* [0x00001ed8] */ 0x229c0e8a, 0xd00648a0, // fsub.ifnz  r2, f(0.0), r2     ; fmul  r0, r1, r2
/* [0x00001ee0] */ 0x957bdd89, 0xd00288e2, // mov  r3, ra_scalar            ; mov.ifz  r2, r1 << 3
/* [0x00001ee8] */ 0x801bf036, 0xd00049e5, // nop                           ; mov  r5rep, ra_temp3 << 1
/* [0x00001ef0] */ 0x211b1b8a, 0xd0025962, // fadd  r5rep, r5, ra_temp3     ; fmul  r2 << 1, r1, r2
/* [0x00001ef8] */ 0x222270b6, 0x10028821, // fsub  r0, r0, r2              ; fmul.ifz  r1, ra_temp4, ra_temp4
/* [0x00001f00] */ 0x3523fb76, 0xd00a88ca, // mov.ifnn  r3, r5              ; fmul.ifz  rb_temp >> 1, ra_temp4, ra_temp4  @mul_used(0)
/* [0x00001f08] */ 0x22367d40, 0x100252e2, // fsub  rb_temp2, ra_subd_consts, r5 ; fmul  r2, r0, r0
/* [0x00001f10] */ 0x2134a3d6, 0x10025946, // fadd  r5rep, r1, rb_temp      ; fmul  ra_temp3, r2, ra_subd_consts
/* [0x00001f18] */ 0x95132b76, 0xd0024d62, // mov  rsqrt, r5                ; mov  r2, ra_temp2 >> 2                      @mul_used(6, 7, 14, 15)
/* [0x00001f20] */ 0x8218bdc0, 0x100939c6, // fsub.ifn.setf  -, ra_temp3, rb_temp2 ; mov.ifn  ra_temp3, r0
/* [0x00001f28] */ 0x811005bf, 0x10030823, // fadd  r0, r2, ra_temp2        ; mov.ifn  r3, rb_scalar
/* [0x00001f30] */ 0x811a77a4, 0x100269e3, // fadd.setf  -, r3, ra_temp3    ; mov  r3, r4
/* [0x00001f38] */ 0x08820882, 0xe20829e7, // mov.ifn.setf  -, [0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0]
/* [0x00001f40] */ 0x00000000, 0xf05589e7, // bra.allnn  -, ra_cubics[i]
/* [0x00001f48] */ 0x8221fbf6, 0x100269e2, // fsub.setf  -, r5, rb_eps      ; mov  r2, ra_temp4
/* [0x00001f50] */ 0x359fa253, 0xd00948a2, // mov.ifn  r2, r1               ; fmul.ifnn  r2 << 10, r2, r3                 @mul_used(10, 11)
/* [0x00001f58] */ 0x95334d92, 0xd00248a1, // mov  r2, ra_cubics[i] ; mov  r1, r2 >> 4  @mul_used(14, 15)
/* [0x00001f60] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rs2i_dash_clerp_lr ; nop
/* [0x00001f68] */ 0x809fe012, 0xd00099c1, // nop                   ; mov.ifz  ra_scalar2, (r2 << 15) >> rs2i_dash_clerp_lr
/* [0x00001f70] */ 0x00000000, 0xf0f7c9e7, // bra  -, (ra_scalar << rsi_dash_clerp_lbl) >> 15
/* [0x00001f78] */ 0x0001ff7f, 0xe20229e7, // mov.setf  -, [-1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1]
/* [0x00001f80] */ 0x359edfc7, 0xd0029961, // mov  r5rep, f(0.125)  ; fmul.ifz  r1, r0, f(0.125)
/* [0x00001f88] */ 0x209fa005, 0xd00109e1, // nop                   ; fmul.ifn  r1 >> 6, r0, r5
// :clip
/* [0x00001f90] */ 0x15167d80, 0x10020827, // mov  r0, ra_user_to_surface_clip_inner
/* [0x00001f98] */ 0x009e7000, 0x100009e7, // nop
/* [0x00001fa0] */ 0x809fd000, 0xd00049e5, // nop ; mov  r5rep, r0 << 3
/* [0x00001fa8] */ 0x209ca03d, 0x100049e1, // nop ; fmul  r1, xs, r5
/* [0x00001fb0] */ 0x809fc000, 0xd00049e5, // nop ; mov  r5rep, r0 << 4
/* [0x00001fb8] */ 0x209cb03d, 0x100049e2, // nop ; fmul  r2, ys, r5
/* [0x00001fc0] */ 0x019e7280, 0x10020867, // fadd  r1, r1, r2
/* [0x00001fc8] */ 0x809fb000, 0xd00049e5, // nop ; mov  r5rep, r0 << 5
/* [0x00001fd0] */ 0x019e7340, 0x10020867, // fadd  r1, r1, r5
/* [0x00001fd8] */ 0x809ff000, 0xd00049e5, // nop ; mov  r5rep, r0 << 1
/* [0x00001fe0] */ 0x209cb03d, 0x100049e3, // nop ; fmul  r3, ys, r5
/* [0x00001fe8] */ 0x809e7000, 0x100049e5, // nop ; mov  r5rep, r0 << 0
/* [0x00001ff0] */ 0x209ca03d, 0x100049e2, // nop ; fmul  r2, xs, r5
/* [0x00001ff8] */ 0x019e74c0, 0x100208a7, // fadd  r2, r2, r3
/* [0x00002000] */ 0x809fe000, 0xd00049e5, // nop ; mov  r5rep, r0 << 2
/* [0x00002008] */ 0x019e7540, 0x100212a7, // fadd  xs, r2, r5
/* [0x00002010] */ 0x159e7240, 0x100212e7, // mov ys, r1
/* [0x00002018] */ 0x809fd000, 0xd00049e5, // nop ; mov  r5rep, r0 << 3
/* [0x00002020] */ 0x209cc03d, 0x100049e1, // nop ; fmul  r1, xs, r5
/* [0x00002028] */ 0x809fc000, 0xd00049e5, // nop ; mov  r5rep, r0 << 4
/* [0x00002030] */ 0x209cd03d, 0x100049e2, // nop ; fmul  r2, ys, r5
/* [0x00002038] */ 0x019e7280, 0x10020867, // fadd  r1, r1, r2
/* [0x00002040] */ 0x809fb000, 0xd00049e5, // nop ; mov  r5rep, r0 << 5
/* [0x00002048] */ 0x019e7340, 0x10020867, // fadd  r1, r1, r5
/* [0x00002050] */ 0x809ff000, 0xd00049e5, // nop ; mov  r5rep, r0 << 1
/* [0x00002058] */ 0x209cd03d, 0x100049e3, // nop ; fmul  r3, ys, r5
/* [0x00002060] */ 0x809e7000, 0x100049e5, // nop ; mov  r5rep, r0 << 0
/* [0x00002068] */ 0x209cc03d, 0x100049e2, // nop ; fmul  r2, xs, r5
/* [0x00002070] */ 0x019e74c0, 0x100208a7, // fadd  r2, r2, r3
/* [0x00002078] */ 0x809fe000, 0xd00049e5, // nop ; mov  r5rep, r0 << 2
/* [0x00002080] */ 0x019e7540, 0x10021327, // fadd  xs, r2, r5
/* [0x00002088] */ 0x159e7240, 0x10021367, // mov ys, r1
/* [0x00002090] */ 0x809fd000, 0xd00049e5, // nop ; mov  r5rep, r0 << 3
/* [0x00002098] */ 0x209ce03d, 0x100049e1, // nop ; fmul  r1, xs, r5
/* [0x000020a0] */ 0x809fc000, 0xd00049e5, // nop ; mov  r5rep, r0 << 4
/* [0x000020a8] */ 0x209cf03d, 0x100049e2, // nop ; fmul  r2, ys, r5
/* [0x000020b0] */ 0x019e7280, 0x10020867, // fadd  r1, r1, r2
/* [0x000020b8] */ 0x809fb000, 0xd00049e5, // nop ; mov  r5rep, r0 << 5
/* [0x000020c0] */ 0x019e7340, 0x10020867, // fadd  r1, r1, r5
/* [0x000020c8] */ 0x809ff000, 0xd00049e5, // nop ; mov  r5rep, r0 << 1
/* [0x000020d0] */ 0x209cf03d, 0x100049e3, // nop ; fmul  r3, ys, r5
/* [0x000020d8] */ 0x809e7000, 0x100049e5, // nop ; mov  r5rep, r0 << 0
/* [0x000020e0] */ 0x209ce03d, 0x100049e2, // nop ; fmul  r2, xs, r5
/* [0x000020e8] */ 0x019e74c0, 0x100208a7, // fadd  r2, r2, r3
/* [0x000020f0] */ 0x809fe000, 0xd00049e5, // nop ; mov  r5rep, r0 << 2
/* [0x000020f8] */ 0x019e7540, 0x100213a7, // fadd  xs, r2, r5
/* [0x00002100] */ 0x159e7240, 0x100213e7, // mov ys, r1
/* [0x00002108] */ 0x159defc0, 0x10020227, // mov  ra_temp4, rb_inactive_elems ; nop
/* [0x00002110] */ 0x809fa000, 0xd00049e5, // nop                              ; mov  r5rep, r0 << 6
/* [0x00002118] */ 0x029cabc0, 0x100229e7, // fsub.setf  -, r5, rb_temp1       ; nop
/* [0x00002120] */ 0x029ccbc0, 0x100a29e7, // fsub.ifnn.setf  -, r5, rb_temp3  ; nop
/* [0x00002128] */ 0x029cebc0, 0x100a29e7, // fsub.ifnn.setf  -, r5, rb_temp5  ; nop
/* [0x00002130] */ 0x969f9000, 0xd00a4225, // mov.ifnn  ra_temp4, 0            ; mov  r5rep, r0 << 7
/* [0x00002138] */ 0x029cbbc0, 0x100229e7, // fsub.setf  -, r5, rb_temp2       ; nop
/* [0x00002140] */ 0x029cdbc0, 0x100a29e7, // fsub.ifnn.setf  -, r5, rb_temp4  ; nop
/* [0x00002148] */ 0x029cfbc0, 0x100a29e7, // fsub.ifnn.setf  -, r5, rb_temp6  ; nop
/* [0x00002150] */ 0x969f8000, 0xd00a4225, // mov.ifnn  ra_temp4, 0            ; mov  r5rep, r0 << 8
/* [0x00002158] */ 0x029caf40, 0x100229e7, // fsub.setf  -, rb_temp1, r5       ; nop
/* [0x00002160] */ 0x029ccf40, 0x100a29e7, // fsub.ifnn.setf  -, rb_temp3, r5  ; nop
/* [0x00002168] */ 0x029cef40, 0x100a29e7, // fsub.ifnn.setf  -, rb_temp5, r5  ; nop
/* [0x00002170] */ 0x969f7000, 0xd00a4225, // mov.ifnn  ra_temp4, 0            ; mov  r5rep, r0 << 9
/* [0x00002178] */ 0x029cbf40, 0x100229e7, // fsub.setf  -, rb_temp2, r5       ; nop
/* [0x00002180] */ 0x820cdf76, 0x100a69e0, // fsub.ifnn.setf  -, rb_temp4, r5  ; mov  r0, ra_scalar3
/* [0x00002188] */ 0x029cff40, 0x100a29e7, // fsub.ifnn.setf  -, rb_temp6, r5  ; nop
/* [0x00002190] */ 0x969ff000, 0xd00a4225, // mov.ifnn  ra_temp4, 0            ; mov  r5rep, r0 << rs3i_clip_outer
/* [0x00002198] */ 0x029cabc0, 0x100229e7, // fsub.setf  -, r5, rb_temp1       ; nop
/* [0x000021a0] */ 0x029ccbc0, 0x100829e7, // fsub.ifn.setf  -, r5, rb_temp3   ; nop
/* [0x000021a8] */ 0x029cebc0, 0x100829e7, // fsub.ifn.setf  -, r5, rb_temp5   ; nop
/* [0x000021b0] */ 0x9523ed80, 0xd00869e5, // mov.ifn.setf  -, ra_temp4        ; mov  r5rep, r0 << (rs3i_clip_outer + 1)
/* [0x000021b8] */ 0x029cbbc0, 0x100829e7, // fsub.ifn.setf  -, r5, rb_temp2   ; nop
/* [0x000021c0] */ 0x029cdbc0, 0x100829e7, // fsub.ifn.setf  -, r5, rb_temp4   ; nop
/* [0x000021c8] */ 0x029cfbc0, 0x100829e7, // fsub.ifn.setf  -, r5, rb_temp6   ; nop
/* [0x000021d0] */ 0x809fd000, 0xd00049e5, // nop                              ; mov  r5rep, r0 << (rs3i_clip_outer + 2)
/* [0x000021d8] */ 0x029caf40, 0x100829e7, // fsub.ifn.setf  -, rb_temp1, r5   ; nop
/* [0x000021e0] */ 0x029ccf40, 0x100829e7, // fsub.ifn.setf  -, rb_temp3, r5   ; nop
/* [0x000021e8] */ 0x029cef40, 0x100829e7, // fsub.ifn.setf  -, rb_temp5, r5   ; nop
/* [0x000021f0] */ 0x809fc000, 0xd00049e5, // nop                              ; mov  r5rep, r0 << (rs3i_clip_outer + 3)
/* [0x000021f8] */ 0x029cbf40, 0x100829e7, // fsub.ifn.setf  -, rb_temp2, r5   ; nop
/* [0x00002200] */ 0x029cdf40, 0x100829e7, // fsub.ifn.setf  -, rb_temp4, r5   ; nop
/* [0x00002208] */ 0xe29cff40, 0x100879c6, // fsub.ifn.setf  -, rb_temp6, r5   ; mov  ra_temp3, 0
/* [0x00002210] */ 0x000001a8, 0xf05809e7, // brr.allnn  -, r:clip_out_end
/* [0x00002218] */ 0x000000c4, 0xe0020827, // mov  r0, 196
/* [0x00002220] */ 0x8d79fc3f, 0xd00339c6, // sub.setf  -, ra_scalar, r0 ; mov.ifn  ra_temp3, -1
/* [0x00002228] */ 0x40004000, 0xe20829e7, // mov.ifn.setf  -, neg_elems(elems(rsi_alloc_size))
/* [0x00002230] */ 0x00001b50, 0xf06802a7, // brr.anyn  ra_temp, r:alloc
/* [0x00002238] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002240] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002248] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002250] */ 0x41800000, 0xe0021967, // mov  r5rep, f(16.0)
/* [0x00002258] */ 0x0000ffff, 0xe00202a7, // mov  ra_temp, 0xffff
/* [0x00002260] */ 0x209ca03d, 0x100049e0, // nop                       ; fmul  r0, rb_temp1, r5
/* [0x00002268] */ 0x019ef1c0, 0xd0022827, // fadd.setf  r0, r0, f(0.5) ; nop
/* [0x00002270] */ 0x029e01c0, 0xd0080827, // fsub.ifn  r0, r0, f(1.0)  ; nop
/* [0x00002278] */ 0x279cb03d, 0x10024821, // ftoi  r0, r0              ; fmul  r1, rb_temp2, r5
/* [0x00002280] */ 0x019ef3c0, 0xd0022867, // fadd.setf  r1, r1, f(0.5) ; nop
/* [0x00002288] */ 0x029e03c0, 0xd0080867, // fsub.ifn  r1, r1, f(1.0)  ; nop
/* [0x00002290] */ 0x079e7240, 0x10020867, // ftoi  r1, r1              ; nop
/* [0x00002298] */ 0x912903c6, 0xd0024860, // shl  r1, r1, -16          ; v8min  r0, r0, ra_temp
/* [0x000022a0] */ 0x359cc07d, 0x10024821, // or  r0, r0, r1            ; fmul  r1, rb_temp3, r5
/* [0x000022a8] */ 0x811af3c6, 0xd0026860, // fadd.setf  r1, r1, f(0.5) ; v8min  r0, r0, ra_temp3
/* [0x000022b0] */ 0x029e03c0, 0xd0080867, // fsub.ifn  r1, r1, f(1.0)  ; nop
/* [0x000022b8] */ 0x279cd27d, 0x10024862, // ftoi  r1, r1              ; fmul  r2, rb_temp4, r5
/* [0x000022c0] */ 0x019ef5c0, 0xd00228a7, // fadd.setf  r2, r2, f(0.5) ; nop
/* [0x000022c8] */ 0x029e05c0, 0xd00808a7, // fsub.ifn  r2, r2, f(1.0)  ; nop
/* [0x000022d0] */ 0x079e7480, 0x100208a7, // ftoi  r2, r2              ; nop
/* [0x000022d8] */ 0x912905ce, 0xd00248a1, // shl  r2, r2, -16          ; v8min  r1, r1, ra_temp
/* [0x000022e0] */ 0x359ce2bd, 0x10024862, // or  r1, r1, r2            ; fmul  r2, rb_temp5, r5
/* [0x000022e8] */ 0x811af5ce, 0xd00268a1, // fadd.setf  r2, r2, f(0.5) ; v8min  r1, r1, ra_temp3
/* [0x000022f0] */ 0x029e05c0, 0xd00808a7, // fsub.ifn  r2, r2, f(1.0)  ; nop
/* [0x000022f8] */ 0x279cf4bd, 0x100248a3, // ftoi  r2, r2              ; fmul  r3, rb_temp6, r5
/* [0x00002300] */ 0x019ef7c0, 0xd00228e7, // fadd.setf  r3, r3, f(0.5) ; nop
/* [0x00002308] */ 0x029e07c0, 0xd00808e7, // fsub.ifn  r3, r3, f(1.0)  ; nop
/* [0x00002310] */ 0x079e76c0, 0x100208e7, // ftoi  r3, r3              ; nop
/* [0x00002318] */ 0x912907d6, 0xd00248e2, // shl  r3, r3, -16          ; v8min  r2, r2, ra_temp
/* [0x00002320] */ 0x159e74c0, 0x100208a7, // or  r2, r2, r3            ; nop
/* [0x00002328] */ 0x801a7016, 0x100049e2, // nop                       ; v8min  r2, r2, ra_temp3
/* [0x00002330] */ 0x157a7d80, 0x100208e7, // mov r3, ra_scalar
/* [0x00002338] */ 0x159f2fc0, 0x100009e7, // mov -, vw_wait
/* [0x00002340] */ 0x809f601b, 0xd00049f1, // nop ; mov vw_setup, r3 << rsi_out_vpm_setup
/* [0x00002348] */ 0x159e7000, 0x10020c27, // mov vpm, r0  @geomd_verts_a(0)
/* [0x00002350] */ 0x159e7240, 0x10020c27, // mov vpm, r1  @geomd_verts_a(1)
/* [0x00002358] */ 0x159e7480, 0x10020c27, // mov vpm, r2  @geomd_verts_a(2) @geomd_tris_add
/* [0x00002360] */ 0x157a7d80, 0x10020827, // mov r0, ra_scalar
/* [0x00002368] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002370] */ 0x809f4000, 0xd00049e5, // nop ; mov r5rep, r0 << rsi_out_vdw_setup_0
/* [0x00002378] */ 0x0772c000, 0xe0020867, // mov r1, ((15<<23)-(13<<16)-(1<<14))
/* [0x00002380] */ 0x0c9e7a40, 0x10021967, // add r5rep, r5, r1
/* [0x00002388] */ 0x159e7b40, 0x10021c67, // mov vw_setup, r5
/* [0x00002390] */ 0x000000c0, 0xe0021967, // mov r5rep, (16*12)
/* [0x00002398] */ 0x00006000, 0xe20229e7, // mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
/* [0x000023a0] */ 0x0d7a7d40, 0x100607a7, // sub.ifnz  ra_scalar, ra_scalar, r5
/* [0x000023a8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000023b0] */ 0x157a7d80, 0x10020827, // mov  r0, ra_scalar
/* [0x000023b8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000023c0] */ 0x809f3000, 0xd00049e5, // nop ; mov r5rep, r0 << rsi_alloc_p
/* [0x000023c8] */ 0xc0000000, 0xe0021c67, // mov vw_setup, vdw_setup_1(0)
/* [0x000023d0] */ 0x159e7b40, 0x10021ca7, // mov vw_addr, r5
// :clip_out_end
/* [0x000023d8] */ 0x171a7d80, 0x100229e7, // not.setf  -, ra_temp3
/* [0x000023e0] */ 0x80073036, 0xd00059ca, // nop ; mov  ra_temp, (ra_scalar2 << rs2i_clip_lr) >> 15  @mul_used(15)
/* [0x000023e8] */ 0x15227d80, 0x100829e7, // mov.ifn.setf  -, ra_temp4
/* [0x000023f0] */ 0x00000000, 0xf05549e7, // bra.allnn  -, ra_temp
/* [0x000023f8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002400] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002408] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002410] */ 0x960e7036, 0x100a4220, // mov.ifnn  ra_temp4, 0 ; mov  r0, ra_scalar3
/* [0x00002418] */ 0x000f015f, 0xe20221a7, // mov.setf  ra_temp3, [-1, -1, -1, -1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0]
/* [0x00002420] */ 0x809ff000, 0xd00119c6, // nop ; mov.ifn  ra_temp3, r0 << rs3i_clip_outer
/* [0x00002428] */ 0x0000000c, 0xe20229e7, // mov.setf  -, [0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
/* [0x00002430] */ 0x02180f80, 0xd00601a7, // fsub.ifnz  ra_temp3, f(0.0), ra_temp3
// :clip_outer_loop
/* [0x00002438] */ 0x0d981dc0, 0xd00229e7, // sub.setf  -, elem_num, 1
/* [0x00002440] */ 0x15227d80, 0x100829e7, // mov.ifn.setf  -, ra_temp4
/* [0x00002448] */ 0x00000260, 0xf05809e7, // brr.allnn  -, r:clip_outer_loop_next
/* [0x00002450] */ 0x569c803f, 0xd0084220, // mov.ifn  ra_temp4, 0 ; mul24  r0, 8, 8
/* [0x00002458] */ 0x40004000, 0xe20229e7, // mov.setf  -, neg_elems(elems(rsi_alloc_size))
/* [0x00002460] */ 0x0d7a7c00, 0x100829e7, // sub.ifn.setf  -, ra_scalar, r0
/* [0x00002468] */ 0x00001918, 0xf06802a7, // brr.anyn  ra_temp, r:alloc
/* [0x00002470] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002478] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002480] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002488] */ 0x9598fdbf, 0x100269e0, // mov.setf  -, elem_num ; mov  r0, rb_temp6
/* [0x00002490] */ 0x809f1000, 0xd00049e0, // nop ; mov  r0, r0 >> 1  @mul_used(1)
/* [0x00002498] */ 0x809ce03f, 0x100089e0, // nop ; mov.ifz  r0, rb_temp5
/* [0x000024a0] */ 0x809f1000, 0xd00049e0, // nop ; mov  r0, r0 >> 1  @mul_used(1, 2)
/* [0x000024a8] */ 0x809cd03f, 0x100089e0, // nop ; mov.ifz  r0, rb_temp4
/* [0x000024b0] */ 0x809f1000, 0xd00049e0, // nop ; mov  r0, r0 >> 1  @mul_used(1, 2, 3)
/* [0x000024b8] */ 0x809cc03f, 0x100089e0, // nop ; mov.ifz  r0, rb_temp3
/* [0x000024c0] */ 0x809f1000, 0xd00049e0, // nop ; mov  r0, r0 >> 1  @mul_used(1, 2, 3, 4)
/* [0x000024c8] */ 0x809cb03f, 0x100089e0, // nop ; mov.ifz  r0, rb_temp2
/* [0x000024d0] */ 0x809f1000, 0xd00049e0, // nop ; mov  r0, r0 >> 1  @mul_used(1, 2, 3, 4, 5)
/* [0x000024d8] */ 0x809ca03f, 0x100089e0, // nop ; mov.ifz  r0, rb_temp1
/* [0x000024e0] */ 0x8d986dff, 0xd00279ca, // sub.setf  -, elem_num, 6 ; mov  ra_temp, 6
/* [0x000024e8] */ 0x809f6000, 0xd00149e0, // nop ; mov.ifnn  r0, r0 >> 6
// :clip_edges_loop
/* [0x000024f0] */ 0x951bfdb6, 0xd0025946, // mov  r5rep, ra_temp3        ; mov  ra_temp3, ra_temp3 << 1
/* [0x000024f8] */ 0x94981dc0, 0xd00269e2, // and.setf  -, elem_num, 1    ; mov  r2, r0
/* [0x00002500] */ 0x959f1000, 0xd004c861, // mov.ifz  r1, r0             ; mov.ifnz  r1, r0 >> 1
/* [0x00002508] */ 0x829fe340, 0xd0026121, // fsub.setf  ra_temp2, r1, r5 ; mov  r1, r0 << 2
/* [0x00002510] */ 0x959e7489, 0x10090862, // mov.ifn  r1, r2             ; mov.ifn  r2, r1
/* [0x00002518] */ 0x029e7280, 0x10020d27, // fsub  recip, r1, r2         ; nop
/* [0x00002520] */ 0x152a7d80, 0x100229e7, // mov.setf  -, ra_temp        ; nop
/* [0x00002528] */ 0x829dfabf, 0xd00298c4, // fsub  r3, r5, r2            ; mov.ifz  ra_temp2, -1
/* [0x00002530] */ 0x34981ddc, 0xd00269e3, // and.setf  -, elem_num, 1    ; fmul  r3, r3, r4
/* [0x00002538] */ 0x959f1b5b, 0xd004c863, // mov.ifz  r1, r5             ; mov.ifnz  r3, r3 >> 1  @mul_unused(0)
/* [0x00002540] */ 0xe39e07c0, 0xd00648e5, // fmin.ifnz  r3, r3, f(1.0)   ; mov  r5rep, 0
/* [0x00002548] */ 0x229e0ecb, 0xd006c8e1, // fsub.ifnz  r3, f(1.0), r3   ; fmul.ifnz  r1, r1, r3
/* [0x00002550] */ 0x35127d93, 0x1002c8e2, // mov  r3, ra_temp2           ; fmul.ifnz  r2, r2, r3
/* [0x00002558] */ 0x019e7280, 0x10060867, // fadd.ifnz  r1, r1, r2       ; nop
// :clip_verts_loop
/* [0x00002560] */ 0x8d9bed5b, 0xd00279c4, // sub.setf  -, elem_num, r5 ; mov  ra_temp2, r3 << 2
/* [0x00002568] */ 0x959f06c0, 0xd00369e2, // mov.setf  -, r3           ; mov.ifnn  r2, r0 >> r5
/* [0x00002570] */ 0xec282bf7, 0xd00a594a, // add.ifnn  r5rep, r5, 2    ; v8subs  ra_temp, ra_temp, 2
/* [0x00002578] */ 0x8d9bed40, 0xd00269e0, // sub.setf  -, elem_num, r5 ; mov  r0, r0 << 2
/* [0x00002580] */ 0x952b0d89, 0xd00369e2, // mov.setf  -, ra_temp      ; mov.ifnn  r2, r1 >> r5
/* [0x00002588] */ 0xffffffb8, 0xf01809e7, // brr.allnz  -, r:clip_verts_loop
/* [0x00002590] */ 0x9613e789, 0xd00269e1, // xor.setf  -, r3, ra_temp2 ; mov  r1, r1 << 2
/* [0x00002598] */ 0x8c102bf6, 0xd0085963, // add.ifn  r5rep, r5, 2     ; mov  r3, ra_temp2
/* [0x000025a0] */ 0x0d9a7d40, 0x100229e7, // sub.setf  -, elem_num, r5    ; nop
/* [0x000025a8] */ 0x951b0d92, 0xd00369e2, // mov.setf  -, ra_temp3        ; mov.ifnn  r2, r2 >> r5
/* [0x000025b0] */ 0x8d988ded, 0xd00479ca, // sub.ifz.setf  -, elem_num, 8 ; mov  ra_temp, r5
/* [0x000025b8] */ 0x809f1012, 0xd00049e0, // nop                          ; mov  r0, r2 >> 1
/* [0x000025c0] */ 0x95181ff6, 0xd00248e1, // mov  r3, 1                   ; mov  r1, ra_temp3
/* [0x000025c8] */ 0xffffff08, 0xf02809e7, // brr.anyz  -, r:clip_edges_loop
/* [0x000025d0] */ 0x949bccc9, 0xd00269e5, // and.setf  -, elem_num, r3    ; mov  r5rep, r1 << 4
/* [0x000025d8] */ 0x959feb40, 0xd002a9e0, // mov.setf  -, r5              ; mov.ifz  r0, r0 << 2
/* [0x000025e0] */ 0x029c0e00, 0xd0060827, // fsub.ifnz  r0, f(0.0), r0    ; nop
/* [0x000025e8] */ 0x0d286dc0, 0xd00229e7, // sub.setf  -, ra_temp, 6
/* [0x000025f0] */ 0x0d28ef80, 0xd00a29e7, // sub.ifnn.setf  -, 14, ra_temp
/* [0x000025f8] */ 0x00000098, 0xf04809e7, // brr.alln  -, r:1f
/* [0x00002600] */ 0x209e4007, 0xd00049e0, // nop                       ; fmul  r0, r0, f(16.0)
/* [0x00002608] */ 0x019ef1c0, 0xd0022827, // fadd.setf  r0, r0, f(0.5) ; nop
/* [0x00002610] */ 0x029e01c0, 0xd0080827, // fsub.ifn  r0, r0, f(1.0)  ; nop
/* [0x00002618] */ 0x879f203f, 0x10020827, // ftoi  r0, r0              ; mov  -, vw_wait
/* [0x00002620] */ 0x919d01ff, 0xd0024821, // shl  r0, r0, -16          ; mov  r1, -16
/* [0x00002628] */ 0x8e9ff040, 0xd0024821, // shr  r0, r0, r1           ; mov  r1, r0 << 1  @mul_used(0, 2, 4, 6, 8, 10, 12, 14)
/* [0x00002630] */ 0xae281dc1, 0xd0024860, // shr  r1, ra_temp, 1       ; v8max  r0, r0, r1
/* [0x00002638] */ 0x8d782e76, 0xd0024862, // sub  r1, 2, r1            ; mov  r2, ra_scalar
/* [0x00002640] */ 0x959c13ff, 0xd0025963, // or  r5rep, r1, 1          ; mov  r3, 1
/* [0x00002648] */ 0x949b6cd2, 0xd00269f1, // and.setf  -, elem_num, r3 ; mov  vw_setup, r2 << rsi_out_vpm_setup
/* [0x00002650] */ 0x959f0000, 0xd002d960, // mov  r5rep, r0            ; mov.ifnz  r0, r0 >> r5
/* [0x00002658] */ 0x0c9a7c40, 0x100229e7, // add.setf  -, elem_num, r1
/* [0x00002660] */ 0x91296ded, 0xd0030870, // shl  r1, ra_temp, -10     ; mov.ifn  vpm, r5       @geomd_verts_m(0)
/* [0x00002668] */ 0x8c9fe280, 0xd0030870, // add  r1, r1, r2           ; mov.ifn  vpm, r0 << 2  @geomd_verts_m(1)
/* [0x00002670] */ 0xfe72c000, 0xe00208e7, // mov  r3, -(3 << 23) - (13 << 16) - (1 << 14)
/* [0x00002678] */ 0x8c9fc2c0, 0xd0030870, // add  r1, r1, r3           ; mov.ifn  vpm, r0 << 4  @geomd_verts_m(2) @geomd_tris_add
/* [0x00002680] */ 0x0d284dc0, 0xd00208e7, // sub  r3, ra_temp, 4       ; nop
/* [0x00002688] */ 0x409c601f, 0xd00049e3, // nop                       ; mul24  r3, r3, 6
/* [0x00002690] */ 0x8d9f44c9, 0xd00248b1, // sub  r2, r2, r3           ; mov  vw_setup, r1 << rsi_out_vdw_setup_0
/* [0x00002698] */ 0xc0000000, 0xe0021c67, // mov  vw_setup, vdw_setup_1(0)
/* [0x000026a0] */ 0x00006000, 0xe20229e7, // mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
/* [0x000026a8] */ 0x959f3492, 0xd00647b2, // mov.ifnz  ra_scalar, r2   ; mov  vw_addr, r2 << rsi_alloc_p
// :1
/* [0x000026b0] */ 0x80073036, 0xd00059ca, // nop ; mov  ra_temp, (ra_scalar2 << rs2i_clip_lr) >> 15  @mul_used(15)
/* [0x000026b8] */ 0x15227d80, 0x100229e7, // mov.setf  -, ra_temp4
/* [0x000026c0] */ 0x00000000, 0xf05549e7, // bra.allnn  -, ra_temp
// :clip_outer_loop_next
/* [0x000026c8] */ 0x9520adbf, 0x10024821, // mov  r0, ra_temp4 ; mov  r1, rb_temp1
/* [0x000026d0] */ 0x809cb03f, 0x100049e2, // nop ; mov  r2, rb_temp2
/* [0x000026d8] */ 0x809ff000, 0xd00059c8, // nop ; mov  ra_temp4, r0 << 1
/* [0x000026e0] */ 0x809ff009, 0xd00049ca, // nop ; mov  rb_temp1, r1 << 1
/* [0x000026e8] */ 0x809ff012, 0xd00049cb, // nop ; mov  rb_temp2, r2 << 1
/* [0x000026f0] */ 0x809cc03f, 0x100049e0, // nop ; mov  r0, rb_temp3
/* [0x000026f8] */ 0x809cd03f, 0x100049e1, // nop ; mov  r1, rb_temp4
/* [0x00002700] */ 0x809ff000, 0xd00049cc, // nop ; mov  rb_temp3, r0 << 1
/* [0x00002708] */ 0x809ff009, 0xd00049cd, // nop ; mov  rb_temp4, r1 << 1
/* [0x00002710] */ 0x809ce03f, 0x100049e0, // nop ; mov  r0, rb_temp5
/* [0x00002718] */ 0xfffffd00, 0xf0f809e7, // brr  -, r:clip_outer_loop
/* [0x00002720] */ 0x809cf03f, 0x100049e1, // nop ; mov  r1, rb_temp6
/* [0x00002728] */ 0x809ff000, 0xd00049ce, // nop ; mov  rb_temp5, r0 << 1
/* [0x00002730] */ 0x809ff009, 0xd00049cf, // nop ; mov  rb_temp6, r1 << 1
// :flush_clerp_q
/* [0x00002738] */ 0x8d980df6, 0xd00269e0, // sub.setf  -, elem_num, rsi_clerp_q_n    ; mov  r0, elem_num
/* [0x00002740] */ 0x967a7036, 0x100447a5, // mov.ifz  ra_scalar, 0                   ; mov  r5rep, ra_scalar << rsi_clerp_q_n
/* [0x00002748] */ 0x8d0e7176, 0x100257a0, // sub  rb_inactive_elems, r0, r5          ; mov  r0, ra_scalar3
/* [0x00002750] */ 0x7f800002, 0xe0020867, // mov  r1, CLERP_Q_TX0_NORMAL
/* [0x00002758] */ 0x2d5efc47, 0xd00269e5, // sub.setf  -, ra_clerp_q_tx0, r1         ; fmul  r5rep, r0 << rs3i_line_width, f(0.5)
/* [0x00002760] */ 0x959f9b40, 0xd0025421, // mov  rb_temp7, r5                       ; mov  r1, r0 << rs3i_cos_theta
/* [0x00002768] */ 0x809f7000, 0xd00049e5, // nop                                     ; mov  r5rep, r0 << rs3i_sin_theta
/* [0x00002770] */ 0x959d6fed, 0x100445d1, // mov.ifz  ra_clerp_q_tx0, rb_clerp_q_tx1 ; mov  rb_temp8, r5
/* [0x00002778] */ 0x959d7fc9, 0x10044665, // mov.ifz  ra_clerp_q_ty0, rb_clerp_q_ty1 ; mov  r5rep, r1
/* [0x00002780] */ 0x355d7b77, 0x100254a1, // mov  rb_temp9, r5                       ; fmul  r1, ra_clerp_q_tx0, rb_clerp_q_ty1
/* [0x00002788] */ 0x20656037, 0x100049e2, // nop                                     ; fmul  r2, ra_clerp_q_ty0, rb_clerp_q_tx1
/* [0x00002790] */ 0x809f5000, 0xd00049e5, // nop                                     ; mov  r5rep, r0 << rs3i_width_hack_clerp
/* [0x00002798] */ 0x355deff5, 0x1002d4d7, // mov  rb_temp10, rb_inactive_elems       ; fmul.ifnz  ra_clerp_q_tx0, ra_clerp_q_tx0, r5
/* [0x000027a0] */ 0x20667035, 0x1000d9d9, // nop                                     ; fmul.ifnz  ra_clerp_q_ty0, ra_clerp_q_ty0, r5
/* [0x000027a8] */ 0x369d603d, 0x1002c816, // mov  r0, f(0.0)                         ; fmul.ifnz  rb_clerp_q_tx1, rb_clerp_q_tx1, r5
/* [0x000027b0] */ 0x229d72bd, 0x1002e9d7, // fsub.setf  -, r1, r2                    ; fmul.ifnz  rb_clerp_q_ty1, rb_clerp_q_ty1, r5
/* [0x000027b8] */ 0x029d11c0, 0x100a1467, // fsub.ifnn  rb_temp8, r0, rb_temp8       ; nop  @geomd_i(geomd_i_stroke_clerp_a)
// :1
/* [0x000027c0] */ 0x205d2037, 0x100049e0, // nop                               ; fmul  r0, ra_clerp_q_tx0, rb_temp9
/* [0x000027c8] */ 0x20651037, 0x100049e1, // nop                               ; fmul  r1, ra_clerp_q_ty0, rb_temp8
/* [0x000027d0] */ 0x21652077, 0x10024821, // fadd  r0, r0, r1                  ; fmul  r1, ra_clerp_q_ty0, rb_temp9
/* [0x000027d8] */ 0x355d1037, 0x10025522, // mov  rb_temp11, r0                ; fmul  r2, ra_clerp_q_tx0, rb_temp8
/* [0x000027e0] */ 0x229d7287, 0x10024862, // fsub  r1, r1, r2                  ; fmul  r2, r0, rb_clerp_q_ty1
/* [0x000027e8] */ 0x359d624f, 0x10025563, // mov  rb_temp12, r1                ; fmul  r3, r1, rb_clerp_q_tx1
/* [0x000027f0] */ 0x209d1017, 0x100049e2, // nop                               ; fmul  r2, r2, rb_temp8
/* [0x000027f8] */ 0x209d101f, 0x100049e3, // nop                               ; fmul  r3, r3, rb_temp8
/* [0x00002800] */ 0x159defc0, 0x100229e7, // mov.setf  -, rb_inactive_elems    ; nop
/* [0x00002808] */ 0x225d04f7, 0x100877a2, // fsub.ifn.setf  rb_inactive_elems, r2, r3 ; fmul  r2, ra_clerp_q_tx0, rb_temp7
/* [0x00002810] */ 0x00000078, 0xf05809e7, // brr.allnn  -, r:1f
/* [0x00002818] */ 0x35650ff7, 0x10025963, // mov  r5rep, rb_temp7              ; fmul  r3, ra_clerp_q_ty0, rb_temp7
/* [0x00002820] */ 0x223e7cc5, 0x100252a0, // fsub  rb_temp1, ra_clerp_q_x0, r3 ; fmul  r0, r0, r5
/* [0x00002828] */ 0x21467c8d, 0x100252e1, // fadd  rb_temp2, ra_clerp_q_y0, r2 ; fmul  r1, r1, r5
/* [0x00002830] */ 0x823e7c76, 0x10025321, // fsub  rb_temp3, ra_clerp_q_x0, r1 ; mov  r1, ra_clerp_q_x0
/* [0x00002838] */ 0xfffff738, 0xf0f802a7, // brr  ra_temp, r:clip
/* [0x00002840] */ 0x81467c36, 0x10025360, // fadd  rb_temp4, ra_clerp_q_y0, r0 ; mov  r0, ra_clerp_q_y0
/* [0x00002848] */ 0x8d98cdc9, 0xd00269ce, // sub.setf  -, elem_num, rs2i_clip_lr ; mov  rb_temp5, r1
/* [0x00002850] */ 0x952a7d80, 0x1004404f, // mov.ifz  ra_scalar2, ra_temp      ; mov  rb_temp6, r0
/* [0x00002858] */ 0x355d0ff7, 0x10025962, // mov  r5rep, rb_temp7              ; fmul  r2, ra_clerp_q_tx0, rb_temp7
/* [0x00002860] */ 0x3565eff5, 0x100269e3, // mov.setf  -, rb_inactive_elems    ; fmul  r3, ra_clerp_q_ty0, r5
/* [0x00002868] */ 0x953d4dbf, 0x10031397, // mov  rb_temp5, ra_clerp_q_x0      ; mov.ifn  ra_clerp_q_tx0, rb_temp11
/* [0x00002870] */ 0x213d4cfd, 0x100252a0, // fadd  rb_temp1, ra_clerp_q_x0, r3 ; fmul  r0, rb_temp11, r5
/* [0x00002878] */ 0x22455cbd, 0x100252e1, // fsub  rb_temp2, ra_clerp_q_y0, r2 ; fmul  r1, rb_temp12, r5
/* [0x00002880] */ 0x813d5c7f, 0x10031319, // fadd  rb_temp3, ra_clerp_q_x0, r1 ; mov.ifn  ra_clerp_q_ty0, rb_temp12
/* [0x00002888] */ 0xfffff6e8, 0xf0f809e7, // brr  -, r:clip
/* [0x00002890] */ 0x82467c36, 0x10025360, // fsub  rb_temp4, ra_clerp_q_y0, r0 ; mov  r0, ra_clerp_q_y0
/* [0x00002898] */ 0x8d98cdc0, 0xd00269cf, // sub.setf  -, elem_num, rs2i_clip_lr ; mov  rb_temp6, r0
/* [0x000028a0] */ 0xdeadbeef, 0xe0040067, // mov.ifz  ra_scalar2, a:1b
// :1
/* [0x000028a8] */ 0x213d6cfd, 0x100243e0, // fadd  ra_cw0x, ra_clerp_q_x0, r3          ; fmul  r0, rb_clerp_q_tx1, r5  @geomd_i(geomd_i_stroke_clerp_b)
/* [0x000028b0] */ 0x22457cbd, 0x10024461, // fsub  ra_cw0y, ra_clerp_q_y0, r2          ; fmul  r1, rb_clerp_q_ty1, r5
/* [0x000028b8] */ 0x824cac7f, 0x10025597, // fsub  rb_ccw1x, ra_clerp_q_x1, r1         ; mov  ra_ccw0x, rb_temp1
/* [0x000028c0] */ 0x8154bc3f, 0x100255d9, // fadd  rb_ccw1y, ra_clerp_q_y1, r0         ; mov  ra_ccw0y, rb_temp2
/* [0x000028c8] */ 0x814d3c7f, 0x100244de, // fadd  ra_cw1x, ra_clerp_q_x1, r1          ; mov  rb_inactive_elems, rb_temp10
/* [0x000028d0] */ 0x8254cc3f, 0xd0024565, // fsub  ra_cw1y, ra_clerp_q_y1, r0          ; mov  r5rep, rs2i_clip_lr
/* [0x000028d8] */ 0x823cadf6, 0x1002480c, // fsub  r0, ra_cw0x, rb_temp1               ; mov  rb_temp3, ra_cw0x
/* [0x000028e0] */ 0x8244bdc0, 0x10024850, // fsub  r1, ra_cw0y, rb_temp2               ; mov  rb_temp7, r0
/* [0x000028e8] */ 0x824cadc9, 0x10024891, // fsub  r2, ra_cw1x, rb_temp1               ; mov  rb_temp8, r1
/* [0x000028f0] */ 0x2254bdca, 0x100258ca, // fsub  r3, ra_cw1y, rb_temp2               ; fmul  ra_temp, r1, r2
/* [0x000028f8] */ 0x225d6f83, 0x10024812, // fsub  r0, rb_ccw1x, ra_ccw0x              ; fmul  rb_temp9, r0, r3
/* [0x00002900] */ 0x22657f83, 0x10025844, // fsub  r1, rb_ccw1y, ra_ccw0y              ; fmul  ra_temp2, r0, r3
/* [0x00002908] */ 0x224d6dca, 0x10024893, // fsub  r2, ra_cw1x, rb_ccw1x               ; fmul  rb_temp10, r1, r2
/* [0x00002910] */ 0x82557df6, 0x100248cf, // fsub  r3, ra_cw1y, rb_ccw1y               ; mov  rb_temp6, ra_cw1y
/* [0x00002918] */ 0x359d1fc7, 0x10025806, // mov  r0, rb_temp8                         ; fmul  ra_temp3, r0, rb_temp8
/* [0x00002920] */ 0x359d0fcf, 0x10024850, // mov  r1, rb_temp7                         ; fmul  rb_temp7, r1, rb_temp7
/* [0x00002928] */ 0x22292dd0, 0x100242a0, // fsub  ra_temp, ra_temp, rb_temp9          ; fmul  r0, r2, r0
/* [0x00002930] */ 0x22113dd9, 0x10025461, // fsub  rb_temp8, ra_temp2, rb_temp10       ; fmul  r1, r3, r1
/* [0x00002938] */ 0x82467076, 0x1002480d, // fsub  r0, r0, r1                          ; mov  rb_temp4, ra_cw0y
/* [0x00002940] */ 0x96291dc0, 0x100274b4, // xor.setf  rb_temp9, ra_temp, rb_temp8     ; mov  recip, r0
/* [0x00002948] */ 0x864e7036, 0x1002480e, // fmaxabs  r0, r0, r0                       ; mov  rb_temp5, ra_cw1x
/* [0x00002950] */ 0x02190f80, 0x10020867, // fsub  r1, rb_temp7, ra_temp3              ; nop
/* [0x00002958] */ 0x229dfe0c, 0x100874a0, // fsub.ifn.setf  rb_temp9, rb_eps, r0       ; fmul  r0, r1, r4
/* [0x00002960] */ 0x359defd0, 0x100874a2, // mov.ifn.setf  rb_temp9, rb_inactive_elems ; fmul  r2, r2, r0
/* [0x00002968] */ 0x219d6e98, 0x10025423, // fadd  rb_temp7, rb_ccw1x, r2              ; fmul  r3, r3, r0
/* [0x00002970] */ 0x019d7ec0, 0x10021467, // fadd  rb_temp8, rb_ccw1y, r3              ; nop
/* [0x00002978] */ 0xfffff5f8, 0xf0f802a7, // brr  ra_temp, r:clip
/* [0x00002980] */ 0x159d0fc0, 0x100813a7, // mov.ifn  rb_temp5, rb_temp7               ; nop
/* [0x00002988] */ 0x8d991d7f, 0x100329cf, // sub.setf  -, elem_num, r5                 ; mov.ifn  rb_temp6, rb_temp8
/* [0x00002990] */ 0x152a7d80, 0x10040067, // mov.ifz  ra_scalar2, ra_temp              ; nop
/* [0x00002998] */ 0x804e7036, 0x100049cc, // nop                          ; mov  rb_temp3, ra_cw1x
/* [0x000029a0] */ 0x80567036, 0x100049cd, // nop                          ; mov  rb_temp4, ra_cw1y
/* [0x000029a8] */ 0x955ccff6, 0xd002480e, // mov  r0, rs2i_clip_lr        ; mov  rb_temp5, ra_ccw0x
/* [0x000029b0] */ 0x95652ff6, 0x100269cf, // mov.setf  -, rb_temp9        ; mov  rb_temp6, ra_ccw0y
/* [0x000029b8] */ 0x809d003f, 0x100109ce, // nop                          ; mov.ifn  rb_temp5, rb_temp7
/* [0x000029c0] */ 0xfffff5b0, 0xf0f802a7, // brr  ra_temp, r:clip
/* [0x000029c8] */ 0x809d103f, 0x100109cf, // nop                          ; mov.ifn  rb_temp6, rb_temp8
/* [0x000029d0] */ 0x8d996c3f, 0x100269ca, // sub.setf  -, elem_num, r0    ; mov  rb_temp1, rb_ccw1x
/* [0x000029d8] */ 0x95297dbf, 0x1004404b, // mov.ifz  ra_scalar2, ra_temp ; mov  rb_temp2, rb_ccw1y
/* [0x000029e0] */ 0x807bf036, 0xd00049e5, // nop                          ; mov  r5rep, ra_scalar << rsi_q_lr
/* [0x000029e8] */ 0x959d2fed, 0x1002778a, // mov.setf  rb_inactive_elems, rb_temp9 ; mov  ra_temp, r5
/* [0x000029f0] */ 0x809e702d, 0x100049d2, // nop                          ; mov  rb_temp9, r5
/* [0x000029f8] */ 0x00000000, 0xf05549e7, // bra.allnn  -, ra_temp
/* [0x00002a00] */ 0x805e7036, 0x100049ca, // nop                          ; mov  rb_temp1, ra_ccw0x
/* [0x00002a08] */ 0x9564cff6, 0xd002480b, // mov  r0, rs2i_clip_lr        ; mov  rb_temp2, ra_ccw0y
/* [0x00002a10] */ 0x809d603f, 0x100049cc, // nop                          ; mov  rb_temp3, rb_ccw1x
/* [0x00002a18] */ 0xfffff558, 0xf0f802a7, // brr  ra_temp, r:clip
/* [0x00002a20] */ 0x809d703f, 0x100049cd, // nop                          ; mov  rb_temp4, rb_ccw1y
/* [0x00002a28] */ 0x8d990c3f, 0x100269ce, // sub.setf  -, elem_num, r0    ; mov  rb_temp5, rb_temp7
/* [0x00002a30] */ 0x95291dbf, 0x1004404f, // mov.ifz  ra_scalar2, ra_temp ; mov  rb_temp6, rb_temp8
/* [0x00002a38] */ 0x803e7036, 0x100049ca, // nop                           ; mov  rb_temp1, ra_cw0x
/* [0x00002a40] */ 0x80467036, 0x100049cb, // nop                           ; mov  rb_temp2, ra_cw0y
/* [0x00002a48] */ 0x954ccff6, 0xd002480c, // mov  r0, rs2i_clip_lr         ; mov  rb_temp3, ra_cw1x
/* [0x00002a50] */ 0xfffff520, 0xf0f809e7, // brr  -, r:clip
/* [0x00002a58] */ 0x809d003f, 0x100049ce, // nop                           ; mov  rb_temp5, rb_temp7
/* [0x00002a60] */ 0x8d991c3f, 0x100269cf, // sub.setf  -, elem_num, r0     ; mov  rb_temp6, rb_temp8
/* [0x00002a68] */ 0x95552ff6, 0x1004404d, // mov.ifz  ra_scalar2, rb_temp9 ; mov  rb_temp4, ra_cw1y
// :flush_join_q_miter
/* [0x00002a70] */ 0x159c0fc0, 0x10020827, // mov r0, rb_scalar  @geomd_i(geomd_i_stroke_join)
/* [0x00002a78] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002a80] */ 0x809ff000, 0xd00049e5, // nop ; mov r5rep, r0 << rbsi_join_q_n
/* [0x00002a88] */ 0x0d9a7d40, 0x100217a7, // sub rb_inactive_elems, elem_num, r5
/* [0x00002a90] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rbsi_join_q_n
/* [0x00002a98] */ 0x00000000, 0xe0041027, // mov.ifz rb_scalar, 0
/* [0x00002aa0] */ 0x00000000, 0xe0021967, // mov r5rep, f(0)
/* [0x00002aa8] */ 0x159dafc0, 0x10020827, // mov r0, rb_join_q_tx0
/* [0x00002ab0] */ 0x209dd007, 0x100049e1, // nop ; fmul r1, r0, rb_join_q_ty1
/* [0x00002ab8] */ 0x159dbfc0, 0x100208a7, // mov r2, rb_join_q_ty0
/* [0x00002ac0] */ 0x209dc017, 0x100049e3, // nop ; fmul r3, r2, rb_join_q_tx1
/* [0x00002ac8] */ 0x029e7640, 0x100229e7, // fsub.setf -, r3, r1
/* [0x00002ad0] */ 0x029dcbc0, 0x100816a7, // fsub.ifn rb_join_q_tx0, r5, rb_join_q_tx1
/* [0x00002ad8] */ 0x029e7a00, 0x10081727, // fsub.ifn rb_join_q_tx1, r5, r0
/* [0x00002ae0] */ 0x029ddbc0, 0x100816e7, // fsub.ifn rb_join_q_ty0, r5, rb_join_q_ty1
/* [0x00002ae8] */ 0x029e7a80, 0x10081767, // fsub.ifn rb_join_q_ty1, r5, r2
/* [0x00002af0] */ 0x159dafc0, 0x10020127, // mov ra_temp2, rb_join_q_tx0
/* [0x00002af8] */ 0x159dbfc0, 0x100201a7, // mov ra_temp3, rb_join_q_ty0
/* [0x00002b00] */ 0x0211cdc0, 0x10022827, // fsub.setf r0, ra_temp2, rb_join_q_tx1
/* [0x00002b08] */ 0x829c0e00, 0xd0094861, // fsub.ifn r1, f(0.0), r0 ; mov.ifnn r1, r0
/* [0x00002b10] */ 0x0219ddc0, 0x100228a7, // fsub.setf r2, ra_temp3, rb_join_q_ty1
/* [0x00002b18] */ 0x829c0e92, 0xd00948e3, // fsub.ifn r3, f(0.0), r2 ; mov.ifnn r3, r2
/* [0x00002b20] */ 0x029e03c0, 0xd00229e7, // fsub.setf -, r1, f(1.0)
/* [0x00002b28] */ 0x029e07c0, 0xd00829e7, // fsub.ifn.setf -, r3, f(1.0)
/* [0x00002b30] */ 0x0119ddc0, 0x10080827, // fadd.ifn r0, ra_temp3, rb_join_q_ty1
/* [0x00002b38] */ 0x029c0e00, 0xd0080827, // fsub.ifn r0, f(0.0), r0
/* [0x00002b40] */ 0x0111cdc0, 0x100808a7, // fadd.ifn r2, ra_temp2, rb_join_q_tx1
/* [0x00002b48] */ 0x209e7000, 0x100049e1, // nop ; fmul r1, r0, r0
/* [0x00002b50] */ 0x209e7012, 0x100049e3, // nop ; fmul r3, r2, r2
/* [0x00002b58] */ 0x019e72c0, 0x10020867, // fadd r1, r1, r3
/* [0x00002b60] */ 0x359ef24f, 0xd0024d61, // mov rsqrt, r1 ; fmul r1, r1, f(0.5)
/* [0x00002b68] */ 0x3fc00000, 0xe0021967, // mov r5rep, f(1.5)
/* [0x00002b70] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002b78] */ 0x209e700c, 0x100049e1, // nop ; fmul r1, r1, r4
/* [0x00002b80] */ 0x209e700c, 0x100049e1, // nop ; fmul r1, r1, r4
/* [0x00002b88] */ 0x029e7a40, 0x10020867, // fsub r1, r5, r1
/* [0x00002b90] */ 0x209e7021, 0x100049e1, // nop ; fmul r1, r4, r1
/* [0x00002b98] */ 0x209e7001, 0x100049e0, // nop ; fmul r0, r0, r1
/* [0x00002ba0] */ 0x209e7011, 0x100049e2, // nop ; fmul r2, r2, r1
/* [0x00002ba8] */ 0x209da007, 0x100049e0, // nop ; fmul r0, r0, rb_join_q_tx0
/* [0x00002bb0] */ 0x209db017, 0x100049e2, // nop ; fmul r2, r2, rb_join_q_ty0
/* [0x00002bb8] */ 0x019e7080, 0x10020827, // fadd r0, r0, r2
/* [0x00002bc0] */ 0x359e7000, 0x10025421, // mov rb_temp7, r0 ; fmul r1, r0, r0
/* [0x00002bc8] */ 0x029e0e40, 0xd0021467, // fsub rb_temp8, f(1.0), r1
/* [0x00002bd0] */ 0x150e7d80, 0x10021967, // mov r5rep, ra_scalar3
/* [0x00002bd8] */ 0x209ef02f, 0xd00049e5, // nop ; fmul r5rep, r5, f(0.5)
/* [0x00002be0] */ 0x209da03d, 0x100049da, // nop ; fmul rb_join_q_tx0, rb_join_q_tx0, r5
/* [0x00002be8] */ 0x209db03d, 0x100049db, // nop ; fmul rb_join_q_ty0, rb_join_q_ty0, r5
/* [0x00002bf0] */ 0x209dc03d, 0x100049dc, // nop ; fmul rb_join_q_tx1, rb_join_q_tx1, r5
/* [0x00002bf8] */ 0x209dd03d, 0x100049dd, // nop ; fmul rb_join_q_ty1, rb_join_q_ty1, r5
/* [0x00002c00] */ 0x159d8fc0, 0x10020827, // mov r0, rb_join_q_x
/* [0x00002c08] */ 0x159d9fc0, 0x10020867, // mov r1, rb_join_q_y
/* [0x00002c10] */ 0x029db1c0, 0x10021327, // fsub rb_temp3, r0, rb_join_q_ty0
/* [0x00002c18] */ 0x019da3c0, 0x10021367, // fadd rb_temp4, r1, rb_join_q_tx0
/* [0x00002c20] */ 0x029dd1c0, 0x100213a7, // fsub rb_temp5, r0, rb_join_q_ty1
/* [0x00002c28] */ 0x019dc3c0, 0x100213e7, // fadd rb_temp6, r1, rb_join_q_tx1
/* [0x00002c30] */ 0x159d8fc0, 0x100212a7, // mov rb_temp1, rb_join_q_x
/* [0x00002c38] */ 0x159d9fc0, 0x100212e7, // mov rb_temp2, rb_join_q_y
/* [0x00002c40] */ 0xfffff330, 0xf0f809e7, // brr -, r:clip
/* [0x00002c48] */ 0x0d98cdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_clip_lr
/* [0x00002c50] */ 0xdeadbeef, 0xe0040067, // mov.ifz ra_scalar2, a:1f
/* [0x00002c58] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00002c60] */ 0x150e7d80, 0x10020827, // mov r0, ra_scalar3
/* [0x00002c68] */ 0x159d1fc0, 0x10020d67, // mov rsqrt, rb_temp8
/* [0x00002c70] */ 0x2edbe6ff, 0xe0021967, // mov r5rep, f(EPS)
/* [0x00002c78] */ 0x029d1bc0, 0x100229e7, // fsub.setf -, r5, rb_temp8
/* [0x00002c80] */ 0x809fb000, 0xd00049e5, // nop ; mov r5rep, r0 << rs3i_miter_limit
/* [0x00002c88] */ 0x3f000000, 0xe0020867, // mov r1, f(0.5)
/* [0x00002c90] */ 0x209d1039, 0x100049e1, // nop ; fmul r1, rb_temp8, r1
/* [0x00002c98] */ 0x209e7024, 0x100049e0, // nop ; fmul r0, r4, r4
/* [0x00002ca0] */ 0x209e7008, 0x100049e0, // nop ; fmul r0, r1, r0
/* [0x00002ca8] */ 0x3fc00000, 0xe0020867, // mov r1, f(1.5)
/* [0x00002cb0] */ 0x029e7200, 0x10020827, // fsub r0, r1, r0
/* [0x00002cb8] */ 0x209e7020, 0x100049e0, // nop ; fmul r0, r4, r0
/* [0x00002cc0] */ 0x029e7140, 0x100829e7, // fsub.ifn.setf -, r0, r5
/* [0x00002cc8] */ 0x00000000, 0xe00a17a7, // mov.ifnn rb_inactive_elems, 0
/* [0x00002cd0] */ 0x209d0038, 0x100049e0, // nop ; fmul r0, rb_temp7, r0
/* [0x00002cd8] */ 0x209da038, 0x100049e1, // nop ; fmul r1, rb_join_q_tx0, r0
/* [0x00002ce0] */ 0x159d8fc0, 0x100208a7, // mov r2, rb_join_q_x
/* [0x00002ce8] */ 0x029db5c0, 0x10021327, // fsub rb_temp3, r2, rb_join_q_ty0
/* [0x00002cf0] */ 0x029dd5c0, 0x100213a7, // fsub rb_temp5, r2, rb_join_q_ty1
/* [0x00002cf8] */ 0x019cce40, 0x100212a7, // fadd rb_temp1, rb_temp3, r1
/* [0x00002d00] */ 0x209db038, 0x100049e1, // nop ; fmul r1, rb_join_q_ty0, r0
/* [0x00002d08] */ 0x159d9fc0, 0x100208e7, // mov r3, rb_join_q_y
/* [0x00002d10] */ 0x019da7c0, 0x10021367, // fadd rb_temp4, r3, rb_join_q_tx0
/* [0x00002d18] */ 0x019dc7c0, 0x100213e7, // fadd rb_temp6, r3, rb_join_q_tx1
/* [0x00002d20] */ 0x019cde40, 0x100212e7, // fadd rb_temp2, rb_temp4, r1
/* [0x00002d28] */ 0xfffff248, 0xf0f809e7, // brr -, r:clip
/* [0x00002d30] */ 0x0d98cdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_clip_lr
/* [0x00002d38] */ 0xdeadbeef, 0xe0040067, // mov.ifz ra_scalar2, a:1f
/* [0x00002d40] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00002d48] */ 0x807bf036, 0xd00049e5, // nop ; mov  r5rep, ra_scalar << rsi_q_lr
/* [0x00002d50] */ 0x159e7b40, 0x100202a7, // mov  ra_temp, r5
/* [0x00002d58] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002d60] */ 0x00000000, 0xf0f549e7, // bra -, ra_temp
/* [0x00002d68] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002d70] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002d78] */ 0x009e7000, 0x100009e7, // nop
// :flush_join_q_round
/* [0x00002d80] */ 0x159c0fc0, 0x10020827, // mov r0, rb_scalar  @geomd_i(geomd_i_stroke_join)
/* [0x00002d88] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002d90] */ 0x809ff000, 0xd00049e5, // nop ; mov r5rep, r0 << rbsi_join_q_n
/* [0x00002d98] */ 0x0d9a7d40, 0x100217a7, // sub rb_inactive_elems, elem_num, r5
/* [0x00002da0] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rbsi_join_q_n
/* [0x00002da8] */ 0x00000000, 0xe0041027, // mov.ifz rb_scalar, 0
/* [0x00002db0] */ 0x00000000, 0xe0021967, // mov r5rep, f(0)
/* [0x00002db8] */ 0x159dafc0, 0x10020827, // mov r0, rb_join_q_tx0
/* [0x00002dc0] */ 0x209dd007, 0x100049e1, // nop ; fmul r1, r0, rb_join_q_ty1
/* [0x00002dc8] */ 0x159dbfc0, 0x100208a7, // mov r2, rb_join_q_ty0
/* [0x00002dd0] */ 0x209dc017, 0x100049e3, // nop ; fmul r3, r2, rb_join_q_tx1
/* [0x00002dd8] */ 0x029e7640, 0x100229e7, // fsub.setf -, r3, r1
/* [0x00002de0] */ 0x029dcbc0, 0x100816a7, // fsub.ifn rb_join_q_tx0, r5, rb_join_q_tx1
/* [0x00002de8] */ 0x029e7a00, 0x10081727, // fsub.ifn rb_join_q_tx1, r5, r0
/* [0x00002df0] */ 0x029ddbc0, 0x100816e7, // fsub.ifn rb_join_q_ty0, r5, rb_join_q_ty1
/* [0x00002df8] */ 0x029e7a80, 0x10081767, // fsub.ifn rb_join_q_ty1, r5, r2
/* [0x00002e00] */ 0x150e7d80, 0x10020867, // mov r1, ra_scalar3
/* [0x00002e08] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002e10] */ 0x809f9009, 0xd00049e5, // nop ; mov r5rep, r1<<rs3i_cos_theta
/* [0x00002e18] */ 0x159e7b40, 0x100208a7, // mov r2, r5
/* [0x00002e20] */ 0x809f7009, 0xd00049e5, // nop ; mov r5rep, r1<<rs3i_sin_theta
/* [0x00002e28] */ 0x159e7b40, 0x100208e7, // mov r3, r5
/* [0x00002e30] */ 0x809f4009, 0xd00049e5, // nop ; mov r5rep, r1<<rs3i_width_hack_round
/* [0x00002e38] */ 0x209da03a, 0x100049e0, // nop ; fmul r0, rb_join_q_tx0, r2
/* [0x00002e40] */ 0x209db03b, 0x100049e1, // nop ; fmul r1, rb_join_q_ty0, r3
/* [0x00002e48] */ 0x219db07a, 0x10024821, // fadd r0, r0, r1 ; fmul r1, rb_join_q_ty0, r2
/* [0x00002e50] */ 0x209da03b, 0x100049e2, // nop ; fmul r2, rb_join_q_tx0, r3
/* [0x00002e58] */ 0x229e7285, 0x10024860, // fsub r1, r1, r2 ; fmul r0, r0, r5
/* [0x00002e60] */ 0x209e700d, 0x100049e1, // nop ; fmul r1, r1, r5
/* [0x00002e68] */ 0x00000050, 0xf0f809e7, // brr -, r:flush_join_q_round_loop_merge_point
/* [0x00002e70] */ 0x159defc0, 0x10021427, // mov rb_temp7, rb_inactive_elems
/* [0x00002e78] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002e80] */ 0x009e7000, 0x100009e7, // nop
// :flush_join_q_round_loop_start
/* [0x00002e88] */ 0x150e7d80, 0x10020867, // mov r1, ra_scalar3
/* [0x00002e90] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002e98] */ 0x809f9009, 0xd00049e5, // nop ; mov r5rep, r1<<rs3i_cos_theta
/* [0x00002ea0] */ 0x159e7b40, 0x100208a7, // mov r2, r5
/* [0x00002ea8] */ 0x809f7009, 0xd00049e5, // nop ; mov r5rep, r1<<rs3i_sin_theta
/* [0x00002eb0] */ 0x209da03a, 0x100049e0, // nop ; fmul r0, rb_join_q_tx0, r2
/* [0x00002eb8] */ 0x209db03d, 0x100049e1, // nop ; fmul r1, rb_join_q_ty0, r5
/* [0x00002ec0] */ 0x219db07a, 0x10024821, // fadd r0, r0, r1 ; fmul r1, rb_join_q_ty0, r2
/* [0x00002ec8] */ 0x209da03d, 0x100049e2, // nop ; fmul r2, rb_join_q_tx0, r5
/* [0x00002ed0] */ 0x029e7280, 0x10020867, // fsub r1, r1, r2
// :flush_join_q_round_loop_merge_point
/* [0x00002ed8] */ 0x209dc039, 0x100049e2, // nop ; fmul r2, rb_join_q_tx1, r1
/* [0x00002ee0] */ 0x209dd038, 0x100049e3, // nop ; fmul r3, rb_join_q_ty1, r0
/* [0x00002ee8] */ 0x159d0fc0, 0x100229e7, // mov.setf -, rb_temp7
/* [0x00002ef0] */ 0x029e7680, 0x10083427, // fsub.ifn.setf rb_temp7, r3, r2
/* [0x00002ef8] */ 0x159dcfc0, 0x100a0827, // mov.ifnn r0, rb_join_q_tx1
/* [0x00002f00] */ 0x159ddfc0, 0x100a0867, // mov.ifnn r1, rb_join_q_ty1
/* [0x00002f08] */ 0x150e7d80, 0x10021967, // mov r5rep, ra_scalar3
/* [0x00002f10] */ 0x209ef02f, 0xd00049e5, // nop ; fmul r5rep, r5, f(0.5)
/* [0x00002f18] */ 0x159d8fc0, 0x100212a7, // mov rb_temp1, rb_join_q_x
/* [0x00002f20] */ 0x159d9fc0, 0x100212e7, // mov rb_temp2, rb_join_q_y
/* [0x00002f28] */ 0x209db03d, 0x100049e2, // nop ; fmul r2, rb_join_q_ty0, r5
/* [0x00002f30] */ 0x029d8e80, 0x10021327, // fsub rb_temp3, rb_join_q_x, r2
/* [0x00002f38] */ 0x209da03d, 0x100049e2, // nop ; fmul r2, rb_join_q_tx0, r5
/* [0x00002f40] */ 0x019d9e80, 0x10021367, // fadd rb_temp4, rb_join_q_y, r2
/* [0x00002f48] */ 0x209e700d, 0x100049e2, // nop ; fmul r2, r1, r5
/* [0x00002f50] */ 0x029d8e80, 0x100213a7, // fsub rb_temp5, rb_join_q_x, r2
/* [0x00002f58] */ 0x209e7005, 0x100049e2, // nop ; fmul r2, r0, r5
/* [0x00002f60] */ 0x019d9e80, 0x100213e7, // fadd rb_temp6, rb_join_q_y, r2
/* [0x00002f68] */ 0x159e7000, 0x100216a7, // mov rb_join_q_tx0, r0
/* [0x00002f70] */ 0x159e7240, 0x100216e7, // mov rb_join_q_ty0, r1
/* [0x00002f78] */ 0xffffeff8, 0xf0f809e7, // brr -, r:clip
/* [0x00002f80] */ 0x0d98cdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_clip_lr
/* [0x00002f88] */ 0xdeadbeef, 0xe0040067, // mov.ifz ra_scalar2, a:1f
/* [0x00002f90] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00002f98] */ 0x159d0fc0, 0x100229e7, // mov.setf -, rb_temp7
/* [0x00002fa0] */ 0x00000000, 0xe00a17a7, // mov.ifnn rb_inactive_elems, 0
/* [0x00002fa8] */ 0xfffffec0, 0xf06809e7, // brr.anyn -, r:flush_join_q_round_loop_start
/* [0x00002fb0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002fb8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002fc0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002fc8] */ 0x807bf036, 0xd00049e5, // nop ; mov  r5rep, ra_scalar << rsi_q_lr
/* [0x00002fd0] */ 0x159e7b40, 0x100202a7, // mov  ra_temp, r5
/* [0x00002fd8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002fe0] */ 0x00000000, 0xf0f549e7, // bra -, ra_temp
/* [0x00002fe8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002ff0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00002ff8] */ 0x009e7000, 0x100009e7, // nop
// :flush_join_q_bevel
/* [0x00003000] */ 0x159c0fc0, 0x10020827, // mov r0, rb_scalar  @geomd_i(geomd_i_stroke_join)
/* [0x00003008] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003010] */ 0x809ff000, 0xd00049e5, // nop ; mov r5rep, r0 << rbsi_join_q_n
/* [0x00003018] */ 0x0d9a7d40, 0x100217a7, // sub rb_inactive_elems, elem_num, r5
/* [0x00003020] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rbsi_join_q_n
/* [0x00003028] */ 0x00000000, 0xe0041027, // mov.ifz rb_scalar, 0
/* [0x00003030] */ 0x00000000, 0xe0021967, // mov r5rep, f(0)
/* [0x00003038] */ 0x159dafc0, 0x10020827, // mov r0, rb_join_q_tx0
/* [0x00003040] */ 0x209dd007, 0x100049e1, // nop ; fmul r1, r0, rb_join_q_ty1
/* [0x00003048] */ 0x159dbfc0, 0x100208a7, // mov r2, rb_join_q_ty0
/* [0x00003050] */ 0x209dc017, 0x100049e3, // nop ; fmul r3, r2, rb_join_q_tx1
/* [0x00003058] */ 0x029e7640, 0x100229e7, // fsub.setf -, r3, r1
/* [0x00003060] */ 0x029dcbc0, 0x100816a7, // fsub.ifn rb_join_q_tx0, r5, rb_join_q_tx1
/* [0x00003068] */ 0x029e7a00, 0x10081727, // fsub.ifn rb_join_q_tx1, r5, r0
/* [0x00003070] */ 0x029ddbc0, 0x100816e7, // fsub.ifn rb_join_q_ty0, r5, rb_join_q_ty1
/* [0x00003078] */ 0x029e7a80, 0x10081767, // fsub.ifn rb_join_q_ty1, r5, r2
/* [0x00003080] */ 0x150e7d80, 0x10021967, // mov r5rep, ra_scalar3
/* [0x00003088] */ 0x209ef02f, 0xd00049e5, // nop ; fmul r5rep, r5, f(0.5)
/* [0x00003090] */ 0x209da03d, 0x100049e0, // nop ; fmul r0, rb_join_q_tx0, r5
/* [0x00003098] */ 0x209db03d, 0x100049e1, // nop ; fmul r1, rb_join_q_ty0, r5
/* [0x000030a0] */ 0x209dc03d, 0x100049e2, // nop ; fmul r2, rb_join_q_tx1, r5
/* [0x000030a8] */ 0x209dd03d, 0x100049e3, // nop ; fmul r3, rb_join_q_ty1, r5
/* [0x000030b0] */ 0x159d8fc0, 0x100212a7, // mov rb_temp1, rb_join_q_x
/* [0x000030b8] */ 0x159d9fc0, 0x100212e7, // mov rb_temp2, rb_join_q_y
/* [0x000030c0] */ 0x029d8e40, 0x10021327, // fsub rb_temp3, rb_join_q_x, r1
/* [0x000030c8] */ 0x019d9e00, 0x10021367, // fadd rb_temp4, rb_join_q_y, r0
/* [0x000030d0] */ 0x029d8ec0, 0x100213a7, // fsub rb_temp5, rb_join_q_x, r3
/* [0x000030d8] */ 0x019d9e80, 0x100213e7, // fadd rb_temp6, rb_join_q_y, r2
/* [0x000030e0] */ 0x0d98cdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_clip_lr
/* [0x000030e8] */ 0xdeadbeef, 0xe0040067, // mov.ifz ra_scalar2, a:1f
/* [0x000030f0] */ 0xffffee80, 0xf0f809e7, // brr -, r:clip
/* [0x000030f8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003100] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003108] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00003110] */ 0x807bf036, 0xd00049e5, // nop ; mov  r5rep, ra_scalar << rsi_q_lr
/* [0x00003118] */ 0x159e7b40, 0x100202a7, // mov  ra_temp, r5
/* [0x00003120] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003128] */ 0x00000000, 0xf0f549e7, // bra -, ra_temp
/* [0x00003130] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003138] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003140] */ 0x009e7000, 0x100009e7, // nop
// :flush_cap_q_butt
/* [0x00003148] */ 0x807bf036, 0xd00049e5, // nop ; mov  r5rep, ra_scalar << rsi_q_lr
/* [0x00003150] */ 0x159e7b40, 0x100202a7, // mov  ra_temp, r5
/* [0x00003158] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003160] */ 0x00000000, 0xf0f549e7, // bra  -, ra_temp
/* [0x00003168] */ 0x0d980dc0, 0xd00229e7, // sub.setf  -, elem_num, rbsi_cap_q_n
/* [0x00003170] */ 0x00000000, 0xe0041027, // mov.ifz  rb_scalar, 0
/* [0x00003178] */ 0x009e7000, 0x100009e7, // nop
// :flush_cap_q_square
/* [0x00003180] */ 0x159c0fc0, 0x10020827, // mov r0, rb_scalar  @geomd_i(geomd_i_stroke_cap)
/* [0x00003188] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003190] */ 0x809e7000, 0x100049e5, // nop ; mov r5rep, r0 << rbsi_cap_q_n
/* [0x00003198] */ 0x0d9a7d40, 0x100217a7, // sub rb_inactive_elems, elem_num, r5
/* [0x000031a0] */ 0x0d980dc0, 0xd00229e7, // sub.setf -, elem_num, rbsi_cap_q_n
/* [0x000031a8] */ 0x00000000, 0xe0041027, // mov.ifz rb_scalar, 0
/* [0x000031b0] */ 0x150e7d80, 0x10021967, // mov r5rep, ra_scalar3
/* [0x000031b8] */ 0x209ef02f, 0xd00049e5, // nop ; fmul r5rep, r5, f(0.5)
/* [0x000031c0] */ 0x207e7035, 0x100059df, // nop ; fmul ra_cap_q_tx, ra_cap_q_tx, r5
/* [0x000031c8] */ 0x20027035, 0x100059c0, // nop ; fmul ra_cap_q_ty, ra_cap_q_ty, r5
/* [0x000031d0] */ 0x156e7d80, 0x10020827, // mov r0, ra_cap_q_x
/* [0x000031d8] */ 0x15767d80, 0x10020867, // mov r1, ra_cap_q_y
/* [0x000031e0] */ 0x02027180, 0x100212a7, // fsub rb_temp1, r0, ra_cap_q_ty
/* [0x000031e8] */ 0x017e7380, 0x100212e7, // fadd rb_temp2, r1, ra_cap_q_tx
/* [0x000031f0] */ 0x01027180, 0x10021327, // fadd rb_temp3, r0, ra_cap_q_ty
/* [0x000031f8] */ 0x027e7380, 0x10021367, // fsub rb_temp4, r1, ra_cap_q_tx
/* [0x00003200] */ 0x017caf80, 0x100213a7, // fadd rb_temp5, rb_temp1, ra_cap_q_tx
/* [0x00003208] */ 0x0100bf80, 0x100213e7, // fadd rb_temp6, rb_temp2, ra_cap_q_ty
/* [0x00003210] */ 0x0d98cdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_clip_lr
/* [0x00003218] */ 0xdeadbeef, 0xe0040067, // mov.ifz ra_scalar2, a:1f
/* [0x00003220] */ 0xffffed50, 0xf0f809e7, // brr -, r:clip
/* [0x00003228] */ 0x159cefc0, 0x10021427, // mov rb_temp7, rb_temp5
/* [0x00003230] */ 0x159cffc0, 0x10021467, // mov rb_temp8, rb_temp6
/* [0x00003238] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00003240] */ 0x956d0dbf, 0x1002480a, // mov r0, ra_cap_q_x ; mov rb_temp1, rb_temp7
/* [0x00003248] */ 0x95751dbf, 0x1002484b, // mov r1, ra_cap_q_y ; mov rb_temp2, rb_temp8
/* [0x00003250] */ 0x01027180, 0x100213a7, // fadd rb_temp5, r0, ra_cap_q_ty
/* [0x00003258] */ 0x027e7380, 0x100213e7, // fsub rb_temp6, r1, ra_cap_q_tx
/* [0x00003260] */ 0x017cef80, 0x10021327, // fadd rb_temp3, rb_temp5, ra_cap_q_tx
/* [0x00003268] */ 0x0100ff80, 0x10021367, // fadd rb_temp4, rb_temp6, ra_cap_q_ty
/* [0x00003270] */ 0xffffed00, 0xf0f809e7, // brr -, r:clip
/* [0x00003278] */ 0x0d98cdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_clip_lr
/* [0x00003280] */ 0xdeadbeef, 0xe0040067, // mov.ifz ra_scalar2, a:1f
/* [0x00003288] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00003290] */ 0x807bf036, 0xd00049e5, // nop ; mov  r5rep, ra_scalar << rsi_q_lr
/* [0x00003298] */ 0x159e7b40, 0x100202a7, // mov  ra_temp, r5
/* [0x000032a0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000032a8] */ 0x00000000, 0xf0f549e7, // bra -, ra_temp
/* [0x000032b0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000032b8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000032c0] */ 0x009e7000, 0x100009e7, // nop
// :flush_cap_q_round
/* [0x000032c8] */ 0x159c0fc0, 0x10020827, // mov r0, rb_scalar  @geomd_i(geomd_i_stroke_cap)
/* [0x000032d0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000032d8] */ 0x809e7000, 0x100049e5, // nop ; mov r5rep, r0 << rbsi_cap_q_n
/* [0x000032e0] */ 0x0d9a7d40, 0x100217a7, // sub rb_inactive_elems, elem_num, r5
/* [0x000032e8] */ 0x0d980dc0, 0xd00229e7, // sub.setf -, elem_num, rbsi_cap_q_n
/* [0x000032f0] */ 0x00000000, 0xe0041027, // mov.ifz rb_scalar, 0
/* [0x000032f8] */ 0x150e7d80, 0x10021967, // mov r5rep, ra_scalar3
/* [0x00003300] */ 0x209ef02f, 0xd00049e5, // nop ; fmul r5rep, r5, f(0.5)
/* [0x00003308] */ 0x207e7035, 0x100059df, // nop ; fmul ra_cap_q_tx, ra_cap_q_tx, r5
/* [0x00003310] */ 0x20027035, 0x100059c0, // nop ; fmul ra_cap_q_ty, ra_cap_q_ty, r5
/* [0x00003318] */ 0x956e7db6, 0x100252a0, // mov rb_temp1, ra_cap_q_x ; mov r0, ra_cap_q_x
/* [0x00003320] */ 0x95767db6, 0x100252e1, // mov rb_temp2, ra_cap_q_y ; mov r1, ra_cap_q_y
/* [0x00003328] */ 0x02027180, 0x10021327, // fsub rb_temp3, r0, ra_cap_q_ty
/* [0x00003330] */ 0x017e7380, 0x10021367, // fadd rb_temp4, r1, ra_cap_q_tx
/* [0x00003338] */ 0x01027180, 0x100213a7, // fadd rb_temp5, r0, ra_cap_q_ty
/* [0x00003340] */ 0x027e7380, 0x100213e7, // fsub rb_temp6, r1, ra_cap_q_tx
/* [0x00003348] */ 0x0d98cdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_clip_lr
/* [0x00003350] */ 0xdeadbeef, 0xe0040067, // mov.ifz ra_scalar2, a:1f
/* [0x00003358] */ 0xffffec18, 0xf0f809e7, // brr -, r:clip
/* [0x00003360] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003368] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003370] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00003378] */ 0x150e7d80, 0x10020867, // mov r1, ra_scalar3
/* [0x00003380] */ 0x40490fdb, 0xe0020827, // mov r0, f(PI)
/* [0x00003388] */ 0x809fa009, 0xd00049e5, // nop ; mov r5rep, r1<<rs3i_oo_theta
/* [0x00003390] */ 0x209e7005, 0x100049e0, // nop ; fmul r0, r0, r5
/* [0x00003398] */ 0x079e7000, 0x10021427, // ftoi rb_temp7, r0
/* [0x000033a0] */ 0x027c0f80, 0xd0021467, // fsub rb_temp8, f(0.0), ra_cap_q_tx
/* [0x000033a8] */ 0x02000f80, 0xd00214a7, // fsub rb_temp9, f(0.0), ra_cap_q_ty
/* [0x000033b0] */ 0x00000000, 0xe00214e7, // mov rb_temp10, 0
/* [0x000033b8] */ 0x809f9009, 0xd00049e5, // nop ; mov r5rep, r1<<rs3i_cos_theta
/* [0x000033c0] */ 0x159e7b40, 0x10021527, // mov rb_temp11, r5
/* [0x000033c8] */ 0x809f7009, 0xd00049e5, // nop ; mov r5rep, r1<<rs3i_sin_theta
/* [0x000033d0] */ 0x159e7b40, 0x10021567, // mov rb_temp12, r5
/* [0x000033d8] */ 0x809f4009, 0xd00049e5, // nop ; mov r5rep, r1<<rs3i_width_hack_round
/* [0x000033e0] */ 0x207d4037, 0x100049e0, // nop ; fmul r0, ra_cap_q_tx, rb_temp11
/* [0x000033e8] */ 0x20015037, 0x100049e1, // nop ; fmul r1, ra_cap_q_ty, rb_temp12
/* [0x000033f0] */ 0x21014077, 0x10024821, // fadd r0, r0, r1 ; fmul r1, ra_cap_q_ty, rb_temp11
/* [0x000033f8] */ 0x207d5037, 0x100049e2, // nop ; fmul r2, ra_cap_q_tx, rb_temp12
/* [0x00003400] */ 0x229e7285, 0x10024860, // fsub r1, r1, r2 ; fmul r0, r0, r5
/* [0x00003408] */ 0x209e700d, 0x100049e1, // nop ; fmul r1, r1, r5
/* [0x00003410] */ 0x956e7db6, 0x100252a2, // mov rb_temp1, ra_cap_q_x ; mov r2, ra_cap_q_x
/* [0x00003418] */ 0x95767db6, 0x100252e3, // mov rb_temp2, ra_cap_q_y ; mov r3, ra_cap_q_y
/* [0x00003420] */ 0x02027580, 0x10021327, // fsub rb_temp3, r2, ra_cap_q_ty
/* [0x00003428] */ 0x017e7780, 0x10021367, // fadd rb_temp4, r3, ra_cap_q_tx
/* [0x00003430] */ 0x829e7440, 0x1002539f, // fsub rb_temp5, r2, r1 ; mov ra_cap_q_tx, r0
/* [0x00003438] */ 0x819e7609, 0x100253c0, // fadd rb_temp6, r3, r0 ; mov ra_cap_q_ty, r1
/* [0x00003440] */ 0x0d98cdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_clip_lr
/* [0x00003448] */ 0xdeadbeef, 0xe0040067, // mov.ifz ra_scalar2, a:1f
/* [0x00003450] */ 0xffffeb20, 0xf0f809e7, // brr -, r:clip
/* [0x00003458] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003460] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003468] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00003470] */ 0x00000001, 0xe0020827, // mov r0, 1
/* [0x00003478] */ 0x0c9d3e00, 0x100214e7, // add rb_temp10, rb_temp10, r0
// :flush_cap_q_round_loop_regular_start
/* [0x00003480] */ 0x207d4037, 0x100049e0, // nop ; fmul r0, ra_cap_q_tx, rb_temp11
/* [0x00003488] */ 0x20015037, 0x100049e1, // nop ; fmul r1, ra_cap_q_ty, rb_temp12
/* [0x00003490] */ 0x21014077, 0x10024821, // fadd r0, r0, r1 ; fmul r1, ra_cap_q_ty, rb_temp11
/* [0x00003498] */ 0x207d5037, 0x100049e2, // nop ; fmul r2, ra_cap_q_tx, rb_temp12
/* [0x000034a0] */ 0x029e7280, 0x10020867, // fsub r1, r1, r2
/* [0x000034a8] */ 0x956e7db6, 0x100252a2, // mov rb_temp1, ra_cap_q_x ; mov r2, ra_cap_q_x
/* [0x000034b0] */ 0x95767db6, 0x100252e3, // mov rb_temp2, ra_cap_q_y ; mov r3, ra_cap_q_y
/* [0x000034b8] */ 0x02027580, 0x10021327, // fsub rb_temp3, r2, ra_cap_q_ty
/* [0x000034c0] */ 0x017e7780, 0x10021367, // fadd rb_temp4, r3, ra_cap_q_tx
/* [0x000034c8] */ 0x829e7440, 0x1002539f, // fsub rb_temp5, r2, r1 ; mov ra_cap_q_tx, r0
/* [0x000034d0] */ 0x819e7609, 0x100253c0, // fadd rb_temp6, r3, r0 ; mov ra_cap_q_ty, r1
/* [0x000034d8] */ 0x0d98cdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_clip_lr
/* [0x000034e0] */ 0xdeadbeef, 0xe0040067, // mov.ifz ra_scalar2, a:1f
/* [0x000034e8] */ 0xffffea88, 0xf0f809e7, // brr -, r:clip
/* [0x000034f0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000034f8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003500] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00003508] */ 0x00000001, 0xe0020867, // mov r1, 1
/* [0x00003510] */ 0x0c9d3e40, 0x10020827, // add r0, rb_temp10, r1
/* [0x00003518] */ 0x0c9d3e40, 0x100214e7, // add rb_temp10, rb_temp10, r1
/* [0x00003520] */ 0x0d9d01c0, 0x100229e7, // sub.setf -, r0, rb_temp7
/* [0x00003528] */ 0xffffff38, 0xf01809e7, // brr.allnz -, r:flush_cap_q_round_loop_regular_start
/* [0x00003530] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003538] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003540] */ 0x009e7000, 0x100009e7, // nop
// :flush_cap_q_round_loop_final_start
/* [0x00003548] */ 0x956e7db6, 0x100252a2, // mov rb_temp1, ra_cap_q_x ; mov r2, ra_cap_q_x
/* [0x00003550] */ 0x95767db6, 0x100252e3, // mov rb_temp2, ra_cap_q_y ; mov r3, ra_cap_q_y
/* [0x00003558] */ 0x02027580, 0x10021327, // fsub rb_temp3, r2, ra_cap_q_ty
/* [0x00003560] */ 0x017e7780, 0x10021367, // fadd rb_temp4, r3, ra_cap_q_tx
/* [0x00003568] */ 0x029d25c0, 0x100213a7, // fsub rb_temp5, r2, rb_temp9
/* [0x00003570] */ 0x019d17c0, 0x100213e7, // fadd rb_temp6, r3, rb_temp8
/* [0x00003578] */ 0x0d98cdc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_clip_lr
/* [0x00003580] */ 0xdeadbeef, 0xe0040067, // mov.ifz ra_scalar2, a:1f
/* [0x00003588] */ 0xffffe9e8, 0xf0f809e7, // brr -, r:clip
/* [0x00003590] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003598] */ 0x009e7000, 0x100009e7, // nop
/* [0x000035a0] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x000035a8] */ 0x807bf036, 0xd00049e5, // nop ; mov  r5rep, ra_scalar << rsi_q_lr
/* [0x000035b0] */ 0x159e7b40, 0x100202a7, // mov  ra_temp, r5
/* [0x000035b8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000035c0] */ 0x00000000, 0xf0f549e7, // bra -, ra_temp
/* [0x000035c8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000035d0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000035d8] */ 0x009e7000, 0x100009e7, // nop
// :dash_clerp_dashing
/* [0x000035e0] */ 0x159e7240, 0x100200a7, // mov ra_dc_locals, r1
/* [0x000035e8] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_dc_locals_prev_phase
/* [0x000035f0] */ 0x15067d80, 0x100400a7, // mov.ifz ra_dc_locals, ra_scalar2
/* [0x000035f8] */ 0x15067d80, 0x10020827, // mov r0, ra_scalar2
/* [0x00003600] */ 0x020a7c00, 0x10020827, // fsub r0, ra_dc_locals, r0
/* [0x00003608] */ 0x359e7000, 0x10025320, // mov rb_temp3, r0 ; fmul r0, r0, r0
/* [0x00003610] */ 0x200e7036, 0x100049e1, // nop ; fmul r1, ra_scalar3, ra_scalar3
/* [0x00003618] */ 0x809f9000, 0xd00049e5, // nop ; mov r5rep, r0<<7
/* [0x00003620] */ 0x819f2a09, 0xd0025961, // fadd r5rep, r5, r0 ; mov r1, r1<<rs3i_dash_oo_scale
/* [0x00003628] */ 0x209e7029, 0x100069e1, // nop ; fmul.setf r1, r5, r1
/* [0x00003630] */ 0x159e7240, 0x10020d67, // mov recipsqrt, r1
/* [0x00003638] */ 0x7f7fffff, 0xe0041967, // mov.ifz r5rep, FLT_MAX
/* [0x00003640] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003648] */ 0x159e7900, 0x10061967, // mov.ifnz r5rep, r4
/* [0x00003650] */ 0x0d982dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_dc_locals_sod
/* [0x00003658] */ 0x159e7b40, 0x100400a7, // mov.ifz ra_dc_locals, r5
/* [0x00003660] */ 0x209e7029, 0x100049e5, // nop ; fmul r5rep, r5, r1
/* [0x00003668] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_dash_phase
/* [0x00003670] */ 0x01067d40, 0x10040067, // fadd.ifz ra_scalar2, ra_scalar2, r5
/* [0x00003678] */ 0x151e7d80, 0x10020827, // mov r0, ra_dash_pattern
/* [0x00003680] */ 0x8007e036, 0xd00049e5, // nop ; mov r5rep, ra_scalar2 << rs2i_dash_phase_i
/* [0x00003688] */ 0x00000010, 0xe0020867, // mov r1, 16
/* [0x00003690] */ 0x0d9e7340, 0x10021967, // sub r5rep, r1, r5
/* [0x00003698] */ 0x009e7000, 0x100009e7, // nop
/* [0x000036a0] */ 0x809f0000, 0xd00049e5, // nop ; mov r5rep, r0 >> r5
/* [0x000036a8] */ 0x8d981ded, 0xd00269ca, // sub.setf -, elem_num, rs2i_dash_phase ; mov rb_temp1, r5
/* [0x000036b0] */ 0x00000001, 0xe00629e7, // mov.ifnz.setf -, 1
/* [0x000036b8] */ 0x02067d40, 0x100429e7, // fsub.ifz.setf -, ra_scalar2, r5
/* [0x000036c0] */ 0x000002d8, 0xf06809e7, // brr.anyn -, r:dash_clerp_dashing_loop_end
/* [0x000036c8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000036d0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000036d8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000036e0] */ 0x800be036, 0xd00049e5, // nop ; mov r5rep, ra_dc_locals<<rsi_dc_locals_sod
/* [0x000036e8] */ 0x159ccfc0, 0x10020827, // mov r0, rb_temp3
/* [0x000036f0] */ 0x00000810, 0xe20229e7, // mov.setf -, elems(rsi_dc_locals_dx, rsi_dc_locals_dy)
/* [0x000036f8] */ 0x35074d85, 0xd002d802, // mov r0, ra_scalar2 ; fmul.ifnz ra_dc_locals<<4, r0, r5
/* [0x00003700] */ 0x00003fff, 0xe20229e7, // mov.setf -, not_elems(elems(rs2i_tx, rs2i_ty))
/* [0x00003708] */ 0x809f5000, 0xd00049e5, // nop ; mov r5rep, r0 << rs2i_is_cubic
/* [0x00003710] */ 0x159e7b40, 0x100429e7, // mov.ifz.setf -, r5
/* [0x00003718] */ 0x150a7d80, 0x10040067, // mov.ifz ra_scalar2, ra_dc_locals
// :dash_clerp_dashing_loop_start
/* [0x00003720] */ 0x800bf036, 0xd00049e5, // nop ; mov r5rep, ra_dc_locals << rsi_dc_locals_prev_phase
/* [0x00003728] */ 0x15067d80, 0x10020827, // mov r0, ra_scalar2
/* [0x00003730] */ 0x8208af76, 0x10025961, // fsub r5rep, rb_temp1, r5 ; mov r1, ra_dc_locals
/* [0x00003738] */ 0x809f3000, 0xd00049e0, // nop ; mov r0<<3, r0
/* [0x00003740] */ 0x209ff00d, 0xd00049e1, // nop ; fmul r1>>1, r1, r5
/* [0x00003748] */ 0x00000408, 0xe20229e7, // mov.setf -, elems(rsi_dc_locals_next_x, rsi_dc_locals_next_y)
/* [0x00003750] */ 0x019e7040, 0x100600a7, // fadd.ifnz ra_dc_locals, r0, r1
/* [0x00003758] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_dc_locals_prev_phase
/* [0x00003760] */ 0x159cafc0, 0x100400a7, // mov.ifz ra_dc_locals, rb_temp1
/* [0x00003768] */ 0x0d982dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_dash_phase_i
/* [0x00003770] */ 0x14041dc0, 0xd00429e7, // and.ifz.setf -, ra_scalar2, 0x1
/* [0x00003778] */ 0x00000078, 0xf02809e7, // brr.anyz -, r:3f
/* [0x00003780] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003788] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003790] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003798] */ 0x150a7d80, 0x10020867, // mov r1, ra_dc_locals
/* [0x000037a0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000037a8] */ 0x809fd009, 0xd00049e0, // nop ; mov r0, r1<<rsi_dc_locals_next_x
/* [0x000037b0] */ 0x0000c000, 0xe20229e7, // mov.setf -, elems(14,15)
/* [0x000037b8] */ 0x02040f80, 0xd0060827, // fsub.ifnz r0, f(0.0), ra_scalar2
/* [0x000037c0] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x000037c8] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:1f
/* [0x000037d0] */ 0x000004c8, 0xf0f809e7, // brr -, r:add_cap
/* [0x000037d8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000037e0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000037e8] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x000037f0] */ 0x000000b0, 0xf0f809e7, // brr -, r:2f
/* [0x000037f8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003800] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003808] */ 0x009e7000, 0x100009e7, // nop
// :3
/* [0x00003810] */ 0x150a7d80, 0x10020867, // mov r1, ra_dc_locals
/* [0x00003818] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003820] */ 0x809fd009, 0xd00049e1, // nop ; mov r1, r1<<rsi_dc_locals_next_x
/* [0x00003828] */ 0x0000c000, 0xe20229e7, // mov.setf -, elems(14,15)
/* [0x00003830] */ 0x15067d80, 0x10060867, // mov.ifnz r1, ra_scalar2
/* [0x00003838] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x00003840] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:1f
/* [0x00003848] */ 0x00000238, 0xf0f809e7, // brr -, r:add_clerp
/* [0x00003850] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003858] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003860] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00003868] */ 0x150a7d80, 0x10020827, // mov r0, ra_dc_locals
/* [0x00003870] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003878] */ 0x809fd000, 0xd00049e0, // nop ; mov r0, r0<<rsi_dc_locals_next_x
/* [0x00003880] */ 0x0000c000, 0xe20229e7, // mov.setf -, elems(14,15)
/* [0x00003888] */ 0x15067d80, 0x10060827, // mov.ifnz r0, ra_scalar2
/* [0x00003890] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x00003898] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:1f
/* [0x000038a0] */ 0x000003f8, 0xf0f809e7, // brr -, r:add_cap
/* [0x000038a8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000038b0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000038b8] */ 0x009e7000, 0x100009e7, // nop
// :1
// :2
/* [0x000038c0] */ 0x150a7d80, 0x10020827, // mov r0, ra_dc_locals
/* [0x000038c8] */ 0x00000081, 0xe20229e7, // mov.setf -, elems(rs2i_x, rs2i_y)
/* [0x000038d0] */ 0x809fd000, 0xd000d9c1, // nop ; mov.ifnz ra_scalar2, r0<<3
/* [0x000038d8] */ 0x0d982dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_dash_phase_i
/* [0x000038e0] */ 0x0c041dc0, 0xd0040067, // add.ifz ra_scalar2, ra_scalar2, 1
/* [0x000038e8] */ 0x150e7d80, 0x10020827, // mov r0, ra_scalar3
/* [0x000038f0] */ 0x0000001f, 0xe0020867, // mov r1, 0x1f
/* [0x000038f8] */ 0x809f3000, 0xd00049e5, // nop ; mov r5rep, r0<<rs3i_bitfield
/* [0x00003900] */ 0x0e9c5bc0, 0xd0021967, // shr r5rep, r5, 5
/* [0x00003908] */ 0x149e7a40, 0x10021967, // and r5rep, r5, r1
/* [0x00003910] */ 0x0d067b80, 0x100429e7, // sub.ifz.setf -, r5, ra_scalar2
/* [0x00003918] */ 0x00000018, 0xf01809e7, // brr.allnz -, r:1f
/* [0x00003920] */ 0x00000000, 0xe0040067, // mov.ifz ra_scalar2, 0
/* [0x00003928] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rs2i_dash_phase
/* [0x00003930] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003938] */ 0x02060dc0, 0xd0040067, // fsub.ifz ra_scalar2, ra_scalar2, f(1.0)
/* [0x00003940] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_dc_locals_prev_phase
/* [0x00003948] */ 0x020a0dc0, 0xd00400a7, // fsub.ifz ra_dc_locals, ra_dc_locals, f(1.0)
// :1
/* [0x00003950] */ 0x151e7d80, 0x10020827, // mov r0, ra_dash_pattern
/* [0x00003958] */ 0x8007e036, 0xd00049e5, // nop ; mov r5rep, ra_scalar2 << rs2i_dash_phase_i
/* [0x00003960] */ 0x00000010, 0xe0020867, // mov r1, 16
/* [0x00003968] */ 0x0d9e7340, 0x10021967, // sub r5rep, r1, r5
/* [0x00003970] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003978] */ 0x809f0000, 0xd00049e5, // nop ; mov r5rep, r0 >> r5
/* [0x00003980] */ 0x8d981ded, 0xd00269ca, // sub.setf -, elem_num, rs2i_dash_phase ; mov rb_temp1, r5
/* [0x00003988] */ 0x00000001, 0xe00629e7, // mov.ifnz.setf -, 1
/* [0x00003990] */ 0x02067d40, 0x100429e7, // fsub.ifz.setf -, ra_scalar2, r5
/* [0x00003998] */ 0xfffffd68, 0xf05809e7, // brr.allnn -, r:dash_clerp_dashing_loop_start
/* [0x000039a0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000039a8] */ 0x009e7000, 0x100009e7, // nop
/* [0x000039b0] */ 0x009e7000, 0x100009e7, // nop
// :dash_clerp_dashing_loop_end
/* [0x000039b8] */ 0x8007e036, 0xd00049e5, // nop ; mov r5rep, ra_scalar2<<rs2i_dash_phase_i
/* [0x000039c0] */ 0x149c1bc0, 0xd00229e7, // and.setf -, r5, 0x1
/* [0x000039c8] */ 0x00000020, 0xf01809e7, // brr.allnz -, r:1f
/* [0x000039d0] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x000039d8] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:1f
/* [0x000039e0] */ 0x150a7d80, 0x10020867, // mov r1, ra_dc_locals
/* [0x000039e8] */ 0x00000098, 0xf0f809e7, // brr -, r:add_clerp
/* [0x000039f0] */ 0x009e7000, 0x100009e7, // nop
/* [0x000039f8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003a00] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00003a08] */ 0x80072036, 0xd00059ca, // nop ; mov ra_temp, (ra_scalar2 << rs2i_dash_clerp_lr) >> 15  @mul_used(15)
/* [0x00003a10] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003a18] */ 0x00000000, 0xf0f549e7, // bra -, ra_temp
/* [0x00003a20] */ 0x0000c081, 0xe20229e7, // mov.setf -, elems(rs2i_x, rs2i_y, rs2i_tx, rs2i_ty)
/* [0x00003a28] */ 0x150a7d80, 0x10060067, // mov.ifnz ra_scalar2, ra_dc_locals
/* [0x00003a30] */ 0x009e7000, 0x100009e7, // nop
// :dash_clerp_not_dashing
/* [0x00003a38] */ 0x159e7240, 0x100200a7, // mov ra_dc_locals, r1
/* [0x00003a40] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rsi_q_lr
/* [0x00003a48] */ 0xdeadbeef, 0xe00407a7, // mov.ifz ra_scalar, a:1f
/* [0x00003a50] */ 0x00000030, 0xf0f809e7, // brr -, r:add_clerp
/* [0x00003a58] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003a60] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003a68] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00003a70] */ 0x80072036, 0xd00059ca, // nop ; mov ra_temp, (ra_scalar2 << rs2i_dash_clerp_lr) >> 15  @mul_used(15)
/* [0x00003a78] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003a80] */ 0x00000000, 0xf0f549e7, // bra -, ra_temp
/* [0x00003a88] */ 0x0000c081, 0xe20229e7, // mov.setf -, elems(rs2i_x, rs2i_y, rs2i_tx, rs2i_ty)
/* [0x00003a90] */ 0x150a7d80, 0x10060067, // mov.ifnz ra_scalar2, ra_dc_locals
/* [0x00003a98] */ 0x009e7000, 0x100009e7, // nop
// :add_clerp
/* [0x00003aa0] */ 0x157a7d80, 0x10021967, // mov r5rep, ra_scalar
/* [0x00003aa8] */ 0x159a7d80, 0x100229e7, // mov.setf -, elem_num
/* [0x00003ab0] */ 0x0c9c1bc0, 0xd00407a7, // add.ifz ra_scalar, r5, 1
/* [0x00003ab8] */ 0x0d9a7d40, 0x100229e7, // sub.setf -, elem_num, r5
/* [0x00003ac0] */ 0x95067db6, 0x10025960, // mov r5rep, ra_scalar2 ; mov r0, ra_scalar2
/* [0x00003ac8] */ 0x159e7b40, 0x100403e7, // mov.ifz ra_clerp_q_x0, r5
/* [0x00003ad0] */ 0x809f9000, 0xd00049e5, // nop ; mov r5rep, r0<<7
/* [0x00003ad8] */ 0x159e7b40, 0x10040467, // mov.ifz ra_clerp_q_y0, r5
/* [0x00003ae0] */ 0x809f1000, 0xd00049e5, // nop ; mov r5rep, r0<<15
/* [0x00003ae8] */ 0x159e7b40, 0x10040667, // mov.ifz ra_clerp_q_ty0, r5
/* [0x00003af0] */ 0x159e7240, 0x10021967, // mov r5rep, r1
/* [0x00003af8] */ 0x159e7b40, 0x100404e7, // mov.ifz ra_clerp_q_x1, r5
/* [0x00003b00] */ 0x809f9009, 0xd00049e5, // nop ; mov r5rep, r1<<7
/* [0x00003b08] */ 0x159e7b40, 0x10040567, // mov.ifz ra_clerp_q_y1, r5
/* [0x00003b10] */ 0x809f2009, 0xd00049e5, // nop ; mov r5rep, r1<<14
/* [0x00003b18] */ 0x159e7b40, 0x100415a7, // mov.ifz rb_clerp_q_tx1, r5
/* [0x00003b20] */ 0x809f1009, 0xd00049e5, // nop ; mov r5rep, r1<<15
/* [0x00003b28] */ 0x159e7b40, 0x100415e7, // mov.ifz rb_clerp_q_ty1, r5
/* [0x00003b30] */ 0x809f2000, 0xd00049e5, // nop ; mov r5rep, r0<<14
/* [0x00003b38] */ 0x159e7b40, 0x100405e7, // mov.ifz ra_clerp_q_tx0, r5
/* [0x00003b40] */ 0x809f5000, 0xd00049e5, // nop ; mov r5rep, r0<<rs2i_is_cubic
/* [0x00003b48] */ 0x159e7b40, 0x100429e7, // mov.ifz.setf -, r5
/* [0x00003b50] */ 0x7f800002, 0xe00405e7, // mov.ifz ra_clerp_q_tx0, CLERP_Q_TX0_NORMAL
/* [0x00003b58] */ 0x807bf036, 0xd00049e5, // nop ; mov  r5rep, ra_scalar << rsi_q_lr
/* [0x00003b60] */ 0x8d980ded, 0xd00279ca, // sub.setf  -, elem_num, rsi_clerp_q_n ; mov  ra_temp, r5
/* [0x00003b68] */ 0x0c790dc0, 0xd00429e7, // add.ifz.setf  -, ra_scalar, -16
/* [0x00003b70] */ 0x00000000, 0xf01549e7, // bra.allnz  -, ra_temp
/* [0x00003b78] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003b80] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003b88] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003b90] */ 0xffffeb88, 0xf0f809e7, // brr  -, r:flush_clerp_q
/* [0x00003b98] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003ba0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003ba8] */ 0x009e7000, 0x100009e7, // nop
// :add_join
/* [0x00003bb0] */ 0x159c0fc0, 0x100208a7, // mov r2, rb_scalar
/* [0x00003bb8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003bc0] */ 0x809ff012, 0xd00049e5, // nop ; mov r5rep, r2 << rbsi_join_q_n
/* [0x00003bc8] */ 0x0d981dc0, 0xd00229e7, // sub.setf -, elem_num, rbsi_join_q_n
/* [0x00003bd0] */ 0x0c9c15c0, 0xd0041027, // add.ifz rb_scalar, r2, 1
/* [0x00003bd8] */ 0x0d9a7d40, 0x100229e7, // sub.setf -, elem_num, r5
/* [0x00003be0] */ 0x95067db6, 0x10025960, // mov r5rep, ra_scalar2 ; mov r0, ra_scalar2
/* [0x00003be8] */ 0x159e7b40, 0x10041627, // mov.ifz rb_join_q_x, r5
/* [0x00003bf0] */ 0x809f9000, 0xd00049e5, // nop ; mov r5rep, r0<<rs2i_y
/* [0x00003bf8] */ 0x159e7b40, 0x10041667, // mov.ifz rb_join_q_y, r5
/* [0x00003c00] */ 0x809f2000, 0xd00049e5, // nop ; mov r5rep, r0<<rs2i_tx
/* [0x00003c08] */ 0x159e7b40, 0x100416a7, // mov.ifz rb_join_q_tx0, r5
/* [0x00003c10] */ 0x809f1000, 0xd00049e5, // nop ; mov r5rep, r0<<rs2i_ty
/* [0x00003c18] */ 0x159e7b40, 0x100416e7, // mov.ifz rb_join_q_ty0, r5
/* [0x00003c20] */ 0x809f2009, 0xd00049e5, // nop ; mov r5rep, r1<<14
/* [0x00003c28] */ 0x159e7b40, 0x10041727, // mov.ifz rb_join_q_tx1, r5
/* [0x00003c30] */ 0x809f1009, 0xd00049e5, // nop ; mov r5rep, r1<<15
/* [0x00003c38] */ 0x159e7b40, 0x10041767, // mov.ifz rb_join_q_ty1, r5
/* [0x00003c40] */ 0x807bf036, 0xd00049e5, // nop ; mov  r5rep, ra_scalar << rsi_q_lr
/* [0x00003c48] */ 0x8d981ded, 0xd00279ca, // sub.setf  -, elem_num, rbsi_join_q_n ; mov  ra_temp, r5
/* [0x00003c50] */ 0x0d9cf5c0, 0xd00429e7, // sub.ifz.setf  -, r2, 15
/* [0x00003c58] */ 0x00000000, 0xf01549e7, // bra.allnz  -, ra_temp
/* [0x00003c60] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003c68] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003c70] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003c78] */ 0x15067d80, 0x10020827, // mov  r0, ra_scalar2
/* [0x00003c80] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003c88] */ 0x809fa000, 0xd00059ca, // nop ; mov  ra_temp, (r0 << rs2i_flush_join_q_lbl) >> 15
/* [0x00003c90] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003c98] */ 0x00000000, 0xf0f549e7, // bra  -, ra_temp
/* [0x00003ca0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003ca8] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003cb0] */ 0x009e7000, 0x100009e7, // nop
// :add_cap
/* [0x00003cb8] */ 0x159c0fc0, 0x100208a7, // mov r2, rb_scalar
/* [0x00003cc0] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003cc8] */ 0x809e7012, 0x100049e5, // nop ; mov r5rep, r2 << rbsi_cap_q_n
/* [0x00003cd0] */ 0x0d980dc0, 0xd00229e7, // sub.setf -, elem_num, rbsi_cap_q_n
/* [0x00003cd8] */ 0x0c9c15c0, 0xd0041027, // add.ifz rb_scalar, r2, 1
/* [0x00003ce0] */ 0x0d9a7d40, 0x100229e7, // sub.setf -, elem_num, r5
/* [0x00003ce8] */ 0x159e7000, 0x10021967, // mov r5rep, r0
/* [0x00003cf0] */ 0x159e7b40, 0x100406e7, // mov.ifz ra_cap_q_x, r5
/* [0x00003cf8] */ 0x809f9000, 0xd00049e5, // nop ; mov r5rep, r0<<7
/* [0x00003d00] */ 0x159e7b40, 0x10040767, // mov.ifz ra_cap_q_y, r5
/* [0x00003d08] */ 0x809f2000, 0xd00049e5, // nop ; mov r5rep, r0<<14
/* [0x00003d10] */ 0x159e7b40, 0x100407e7, // mov.ifz ra_cap_q_tx, r5
/* [0x00003d18] */ 0x809f1000, 0xd00049e5, // nop ; mov r5rep, r0<<15
/* [0x00003d20] */ 0x159e7b40, 0x10040027, // mov.ifz ra_cap_q_ty, r5
/* [0x00003d28] */ 0x807bf036, 0xd00049e5, // nop ; mov  r5rep, ra_scalar << rsi_q_lr
/* [0x00003d30] */ 0x8d980ded, 0xd00279ca, // sub.setf  -, elem_num, rbsi_cap_q_n ; mov  ra_temp, r5
/* [0x00003d38] */ 0x0d9cf5c0, 0xd00429e7, // sub.ifz.setf  -, r2, 15
/* [0x00003d40] */ 0x00000000, 0xf01549e7, // bra.allnz  -, ra_temp
/* [0x00003d48] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003d50] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003d58] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003d60] */ 0x15067d80, 0x10020827, // mov  r0, ra_scalar2
/* [0x00003d68] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003d70] */ 0x809fb000, 0xd00059ca, // nop ; mov  ra_temp, (r0 << rs2i_flush_cap_q_lbl) >> 15
/* [0x00003d78] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003d80] */ 0x00000000, 0xf0f549e7, // bra  -, ra_temp
/* [0x00003d88] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003d90] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003d98] */ 0x009e7000, 0x100009e7, // nop
// :alloc
/* [0x00003da0] */ 0x157a7d80, 0x10020827, // mov  r0, ra_scalar
/* [0x00003da8] */ 0xfff10000, 0xe0020867, // mov  r1, -(15 << 16)
/* [0x00003db0] */ 0x8c9f6040, 0xd0024871, // add  r1, r0, r1 ; mov  vw_setup, r0 << rsi_out_vpm_setup
/* [0x00003db8] */ 0x0d9c41c0, 0xd0020827, // sub  r0, r0, 4
/* [0x00003dc0] */ 0x159f2fc0, 0x100009e7, // mov  -, vw_wait
/* [0x00003dc8] */ 0x042a0101, 0xe0020c27, // mov  vpm, TRI_LIST_HEAD
/* [0x00003dd0] */ 0x809f4009, 0xd00049f1, // nop             ; mov  vw_setup, r1 << rsi_out_vdw_setup_0
/* [0x00003dd8] */ 0x809f3000, 0xd00049f2, // nop             ; mov  vw_addr, r0 << rsi_alloc_p
/* [0x00003de0] */ 0x0d984dc0, 0xd00229e7, // sub.setf  -, elem_num, 4
/* [0x00003de8] */ 0x809f7000, 0xd00089c9, // nop             ; mov.ifz  rb_list_tail, (r0 << rsi_alloc_p) >> 4
// :alloc_first
/* [0x00003df0] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00003df8] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00003e00] */ 0x15ce7d80, 0x100009e7, // mov  -, mutex             ; nop
/* [0x00003e08] */ 0x00100a0f, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(15))
/* [0x00003e10] */ 0x00000ffe, 0xe00208a7, // mov  r2, 0xffe
/* [0x00003e18] */ 0x807bd036, 0xd00049e5, // nop                       ; mov  r5rep, ra_scalar << rsi_tess_i
/* [0x00003e20] */ 0x0d9a7d40, 0x100229e7, // sub.setf  -, elem_num, r5 ; nop
/* [0x00003e28] */ 0x8dc02df6, 0xd0024821, // sub  r0, vpm, 2           ; mov  r1, vpm
/* [0x00003e30] */ 0x809f0009, 0xd00049e3, // nop ; mov r3, r1 >> r5
/* [0x00003e38] */ 0x949e7289, 0x1004e860, // and.ifz.setf  r1, r1, r2  ; mov.ifnz  r0, r1
/* [0x00003e40] */ 0x00000120, 0xf01809e7, // brr.allnz  -, r:1f
/* [0x00003e48] */ 0x00100a0f, 0xe0021c67, // mov  vw_setup, vpm_setup(1, 0, h32(15))
/* [0x00003e50] */ 0x000003fe, 0xe00208e7, // mov  r3, VG_TESS_SUBBLOCK_SIZE >> 1
/* [0x00003e58] */ 0x559e700b, 0x10024c21, // mov  vpm, r0              ; mul24  r1, r1, r3
/* [0x00003e60] */ 0x159e7b40, 0x10020c27, // mov  vpm, r5              ; nop
/* [0x00003e68] */ 0x80814780, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 1, dma_h32(15, 0))
/* [0x00003e70] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if
/* [0x00003e78] */ 0x159f2fc0, 0x100009e7, // mov  -, vw_wait           ; nop
/* [0x00003e80] */ 0x00000001, 0xe00209a7, // mov  interrupt, 1         ; nop
/* [0x00003e88] */ 0x009e7000, 0x000009e7, // nop                       ; nop ; bkpt
/* [0x00003e90] */ 0x0d9a7d40, 0x100229e7, // sub.setf  -, elem_num, r5 ; nop
/* [0x00003e98] */ 0x801100f0, 0xe0020c67, // mov  vr_setup, vdr_setup_0(1, 1, dma_h32(15, 0), 0, 8)
/* [0x00003ea0] */ 0xdeadbeef, 0xe0020ca7, // mov  vr_addr, a:vg_tess_shader_if + 4
/* [0x00003ea8] */ 0x15ca7d80, 0x100009e7, // mov  -, vr_wait           ; nop
/* [0x00003eb0] */ 0x00100a0f, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(15))
/* [0x00003eb8] */ 0x0dc02dc0, 0xd0021967, // sub  r5rep, vpm, 2        ; nop
/* [0x00003ec0] */ 0x8c9c2bed, 0xd0028860, // add  r1, r5, 2            ; mov.ifz  r0, r5
/* [0x00003ec8] */ 0x949e7280, 0x10026870, // and.setf  r1, r1, r2      ; mov  vpm, r0
/* [0x00003ed0] */ 0x00000090, 0xf01809e7, // brr.allnz  -, r:1f
/* [0x00003ed8] */ 0x409e700b, 0x100049e1, // nop                       ; mul24  r1, r1, r3
/* [0x00003ee0] */ 0x807bd036, 0xd00049e5, // nop                       ; mov  r5rep, ra_scalar << rsi_tess_i
/* [0x00003ee8] */ 0x0d9a7d40, 0x100229e7, // sub.setf  -, elem_num, r5 ; nop
/* [0x00003ef0] */ 0x149c11c0, 0xd0040827, // and.ifz  r0, r0, 1        ; nop
/* [0x00003ef8] */ 0x957a7036, 0x10024c20, // mov  vpm, r0              ; mov  r0, ra_scalar
/* [0x00003f00] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00003f08] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00003f10] */ 0x00000000, 0xe0020ce7, // mov  mutex, 0             ; nop
/* [0x00003f18] */ 0x809f6000, 0xd00049f1, // nop                       ; mov  vw_setup, r0 << rsi_out_vpm_setup
/* [0x00003f20] */ 0xffffffff, 0xe00049f0, // nop                       ; mov  vpm, -1
/* [0x00003f28] */ 0x809f4000, 0xd00049f1, // nop                       ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
/* [0x00003f30] */ 0x80827036, 0x100049f2, // nop                       ; mov  vw_addr, unif
/* [0x00003f38] */ 0x809f7000, 0xd00049f1, // nop                       ; mov  vw_setup, r0 << rsi_chunks_vpm_setup
/* [0x00003f40] */ 0x00000000, 0xe00049f0, // nop                       ; mov  vpm, 0
/* [0x00003f48] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00003f50] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00003f58] */ 0x159f2fc0, 0x100009e7, // mov  -, vw_wait
/* [0x00003f60] */ 0x00000001, 0xe00209a7, // mov  interrupt, 1
/* [0x00003f68] */ 0x009e7000, 0x300009e7, // thrend
/* [0x00003f70] */ 0x009e7000, 0x100009e7, // nop
/* [0x00003f78] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00003f80] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00003f88] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00003f90] */ 0x00000000, 0xe0020ce7, // mov  mutex, 0
/* [0x00003f98] */ 0x00000014, 0xe0020127, // mov  ra_temp2, 20
/* [0x00003fa0] */ 0x0d9dee80, 0xd00208a7, // sub  r2, -2, r2           ; nop
/* [0x00003fa8] */ 0x947a70b6, 0x10024822, // and  r0, r0, r2           ; mov  r2, ra_scalar
/* [0x00003fb0] */ 0xec9d007d, 0xd0024825, // add  r0, r0, r1           ; v8subs  r5rep, -16, r5
/* [0x00003fb8] */ 0x8c9f66d2, 0xd00248f1, // add  r3, r3, r3           ; mov  vw_setup, r2 << rsi_out_vpm_setup
/* [0x00003fc0] */ 0x809f0000, 0xd00049e5, // nop                       ; mov  r5rep, r0 >> r5
/* [0x00003fc8] */ 0xfff50000, 0xe0020827, // mov  r0, -(11 << 16)
/* [0x00003fd0] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_alloc_p ; nop
/* [0x00003fd8] */ 0x8c7b2c3f, 0x10020827, // add  r0, ra_scalar, r0    ; mov  -, vw_wait
/* [0x00003fe0] */ 0x8d109bbf, 0x100447b0, // sub.ifz  ra_scalar, r5, ra_temp2 ; mov  vpm, rb_list_tail
/* [0x00003fe8] */ 0x0d98edc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_alloc_size ; nop
/* [0x00003ff0] */ 0x8d134780, 0xd00447b1, // sub.ifz  ra_scalar, r3, ra_temp2 ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
/* [0x00003ff8] */ 0x0d127b80, 0x10021ca7, // sub  vw_addr, r5, ra_temp2      ; nop
/* [0x00004000] */ 0x00000000, 0xf0f549e7, // bra  -, ra_temp
/* [0x00004008] */ 0x009e7000, 0x100009e7, // nop
/* [0x00004010] */ 0x009e7000, 0x100009e7, // nop
/* [0x00004018] */ 0x009e7000, 0x100009e7, // nop
};

void vg_tess_stroke_shader_link(void *p_in, unsigned int base
   , unsigned int vg_tess_shader_if
   )
{
   unsigned int *p = (unsigned int *)p_in;
   unsigned int i;
   for (i = 0; i != (VG_TESS_STROKE_SHADER_SIZE / 4); ++i) {
      p[i] = array[i];
   }
   assert(p[102] == 0xdeadbeef);
   p[102] = base + 14904;
   assert(p[110] == 0xdeadbeef);
   p[110] = base + 12288;
   assert(p[112] == 0xdeadbeef);
   p[112] = base + 10864;
   assert(p[114] == 0xdeadbeef);
   p[114] = base + 11648;
   assert(p[126] == 0xdeadbeef);
   p[126] = base + 12672;
   assert(p[128] == 0xdeadbeef);
   p[128] = base + 12616;
   assert(p[130] == 0xdeadbeef);
   p[130] = base + 13000;
   assert(p[162] == 0xdeadbeef);
   p[162] = base + 13792;
   assert(p[182] == 0xdeadbeef);
   p[182] = base + 1488;
   assert(p[184] == 0xdeadbeef);
   p[184] = base + 1320;
   assert(p[290] == 0xdeadbeef);
   p[290] = base + 1256;
   assert(p[356] == 0xdeadbeef);
   p[356] = base + 1608;
   assert(p[360] == 0xdeadbeef);
   p[360] = base + 3768;
   assert(p[564] == 0xdeadbeef);
   p[564] = base + 2352;
   assert(p[572] == 0xdeadbeef);
   p[572] = base + 2496;
   assert(p[578] == 0xdeadbeef);
   p[578] = base + 2496;
   assert(p[630] == 0xdeadbeef);
   p[630] = base + 2560;
   assert(p[710] == 0xdeadbeef);
   p[710] = base + 3152;
   assert(p[752] == 0xdeadbeef);
   p[752] = base + 3048;
   assert(p[778] == 0xdeadbeef);
   p[778] = base + 3152;
   assert(p[804] == 0xdeadbeef);
   p[804] = base + 3272;
   assert(p[820] == 0xdeadbeef);
   p[820] = base + 3368;
   assert(p[844] == 0xdeadbeef);
   p[844] = base + 3464;
   assert(p[874] == 0xdeadbeef);
   p[874] = base + 3600;
   assert(p[1224] == 0xdeadbeef);
   p[1224] = base + 5248;
   assert(p[1300] == 0xdeadbeef);
   p[1300] = base + 5248;
   assert(p[1330] == 0xdeadbeef);
   p[1330] = base + 5392;
   assert(p[2600] == 0xdeadbeef);
   p[2600] = base + 10176;
   assert(p[2836] == 0xdeadbeef);
   p[2836] = base + 11360;
   assert(p[2894] == 0xdeadbeef);
   p[2894] = base + 11592;
   assert(p[3042] == 0xdeadbeef);
   p[3042] = base + 12184;
   assert(p[3130] == 0xdeadbeef);
   p[3130] = base + 12560;
   assert(p[3206] == 0xdeadbeef);
   p[3206] = base + 12864;
   assert(p[3232] == 0xdeadbeef);
   p[3232] = base + 12944;
   assert(p[3284] == 0xdeadbeef);
   p[3284] = base + 13176;
   assert(p[3346] == 0xdeadbeef);
   p[3346] = base + 13424;
   assert(p[3384] == 0xdeadbeef);
   p[3384] = base + 13576;
   assert(p[3424] == 0xdeadbeef);
   p[3424] = base + 13736;
   assert(p[3570] == 0xdeadbeef);
   p[3570] = base + 14320;
   assert(p[3600] == 0xdeadbeef);
   p[3600] = base + 14440;
   assert(p[3622] == 0xdeadbeef);
   p[3622] = base + 14528;
   assert(p[3702] == 0xdeadbeef);
   p[3702] = base + 14856;
   assert(p[3730] == 0xdeadbeef);
   p[3730] = base + 14960;
   assert(p[3996] == 0xdeadbeef);
   p[3996] = vg_tess_shader_if + 0;
   assert(p[4008] == 0xdeadbeef);
   p[4008] = vg_tess_shader_if + 4;
}
