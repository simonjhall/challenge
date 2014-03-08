/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#define PC_BUILD
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ghw_composer_impl.h"


namespace ghw {

/**
 * GhwComposer Implementation
 */
GhwComposer* GhwComposer::create(u32 width, u32 height, u32 num_layers)
{
    GhwComposerV3d *ghw_composer;

    ghw_composer = new GhwComposerV3d();
    if (ghw_composer == NULL) {
        LOGE("%s failed pid[%d]tid[%d] \n", __FUNCTION__, /*getpid(), gettid(), */ -1, -1);
        return NULL;
    }

    if (ghw_composer->init() == GHW_ERROR_NONE) {
        LOGT("%s success[%p] \n", __FUNCTION__, ghw_composer);
        return ghw_composer;
    } else {
        LOGE("%s failed pid[%d]tid[%d] \n", __FUNCTION__, /*getpid(), gettid(),*/ -1, -1);
//        LOGE("%s failed pid[%d]tid[%d] \n", __FUNCTION__, /*getpid(), gettid()*/, -1, -1);
        delete ghw_composer;
        return NULL;
    }

    return ghw_composer;
}

GhwComposer::~GhwComposer()
{
    LOGT("%s[%p] \n", __FUNCTION__, this);
}

/**
 * GhwComposerV3d Implementation
 */
GhwComposerV3d::GhwComposerV3d()
    : fdV3d(-1), shaderAlloc(NULL),
      binMem(NULL), binMemUsed(0), rendMemUsed(0), mJobFB(NULL),
      composeReqs(-1), fbImg(NULL), ditherFlag(GHW_DITHER_NONE)
{
    mName[0] = '\0';
    mName[31] = '\0';
    for (int i=0; i<NUM_SHADERS; i++) {
        v3dShaders[i] = NULL;
    }
    pthread_mutex_init(&mLock,NULL);
    LOGT("%s[%p] \n", __FUNCTION__, this);
}

GhwComposerV3d::~GhwComposerV3d()
{
    LOGT("%s[%p] \n", __FUNCTION__, this);

    barrier();

    /* close v3d devices */
    if (fdV3d >= 0) {
        close(fdV3d);
    }
    fdV3d = -1;

    if (shaderAlloc) {
        delete shaderAlloc;
        shaderAlloc = NULL;
    }
    for (int i=0; i<NUM_SHADERS; i++) {
        v3dShaders[i] = NULL;
    }

    binMem          = NULL;
    binMemUsed      = 0;
    composeReqs     = -1;
    fbImg           = NULL;
    ditherFlag      = GHW_DITHER_NONE;
    mName[0]        = '\0';
    pthread_mutex_destroy(&mLock);
}

/* init: open device driver, allocate and copy shaders */
ghw_error_e GhwComposerV3d::init()
{
    u32 ipa_addr, size;
    void *virt_addr;
    ghw_error_e ret;

    /* open v3d devices */
    fdV3d = open(V3D_DEVICE,    O_RDONLY);
    if(fdV3d == -1) {
        LOGE("%s[%p] failed to open [%s] \n", __FUNCTION__, this, V3D_DEVICE);
        return GHW_ERROR_FAIL;
    }

    /* install allocator for v3dShaders */
    shaderAlloc = GhwMemAllocator::create(GhwMemAllocator::GHW_MEM_ALLOC_RETAIN_ONE|GhwMemAllocator::GHW_MEM_ALLOC_CACHED, 1*1024*1024, 8);
    shaderAlloc->setName("Shader Allocator");

    /* copy v3dShaderss */
    for(int i=0; i<NUM_SHADERS; i++) {
        if ((v3dShaders[i] = shaderAlloc->alloc(ghwV3dShaderSizes[i])) != NULL) {
            v3dShaders[i]->setName("Shader ");
            if (v3dShaders[i]->lock(ipa_addr, virt_addr, size) == GHW_ERROR_NONE) {
                memcpy(virt_addr, ghwV3dShaders[i], ghwV3dShaderSizes[i]);
                v3dShaders[i]->unlock();
            } else {
                LOGE("%s[%p] failed to lock v3dShaders[%d] \n",  __FUNCTION__, this, i);
                return GHW_ERROR_FAIL;
            }
        } else {
            LOGE("%s[%p] failed to allocate v3dShaders[%d] \n",  __FUNCTION__, this, i);
            return GHW_ERROR_FAIL;
        }
    }

   LOGT("%s[%p] fd[%d] \n", __FUNCTION__, this, fdV3d);
    return GHW_ERROR_NONE;
}

/* Provide an identifier */
ghw_error_e GhwComposerV3d::setName(const char *name)
{
    snprintf(mName, 31, "%s", name);
    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::postJob(GhwMemHandle* bin_list_handle, u32 bin_size,
    GhwMemHandle* rend_list_handle, u32 rend_size, Job* job)
{
#ifndef PC_BUILD
    v3d_job_post_t job_post;
    v3d_job_status_t job_status;
#endif
    u32 bin_ipa_addr, rend_ipa_addr, size;
    void *virt_addr;
    LOGT("%s[%p] fd[%d] \n", __FUNCTION__, this, fdV3d);

    if (bin_list_handle) {
        bin_list_handle->lock(bin_ipa_addr, virt_addr, size);
    }
    rend_list_handle->lock(rend_ipa_addr, virt_addr, size);
    cacheFlush();

#ifndef PC_BUILD
    if (bin_list_handle) {
        job_post.job_type = V3D_JOB_BIN_REND;
        job_post.v3d_ct0ca = bin_ipa_addr;
        job_post.v3d_ct0ea = bin_ipa_addr + bin_size;
    }else {
        job_post.job_type = V3D_JOB_REND;
        job_post.v3d_ct0ca = 0;
        job_post.v3d_ct0ea = 0;
    }
    job_post.job_id = 0;
    job_post.v3d_ct1ca = rend_ipa_addr;
    job_post.v3d_ct1ea = rend_ipa_addr + rend_size;

    job_status.job_status = V3D_JOB_STATUS_INVALID;
    job_status.timeout = -1;
    job_status.job_id = 0;
    if (ioctl(fdV3d, V3D_IOCTL_POST_JOB, &job_post) < 0) {
        LOGE("ioctl [0x%x] failed \n", V3D_IOCTL_POST_JOB);
    }
#endif
	mList.addElement(job,0);

    if (bin_list_handle) {
        bin_list_handle->unlock();
    }
    rend_list_handle->unlock();
    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::waitJobCompletion()
{
    LOGT("%s[%p] fd[%d] \n", __FUNCTION__, this, fdV3d);
#ifndef PC_BUILD
    v3d_job_status_t job_status;

    /* Wait for all v3d jobs submitted from this session */
    job_status.job_status = V3D_JOB_STATUS_INVALID;
    job_status.timeout = -1;
    if (ioctl(fdV3d, V3D_IOCTL_WAIT_JOB, &job_status) < 0) {
        LOGE("%s[%p] ioctl[0x%x] failed \n", __FUNCTION__, this, V3D_IOCTL_WAIT_JOB);
    }
    if(job_status.job_status != V3D_JOB_STATUS_SUCCESS)
    	printf("%s[%p] job status[%d] \n", __FUNCTION__, this, job_status.job_status);
#endif
    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::cacheFlush()
{
    LOGT("%s[%p] \n", __FUNCTION__, this);
#ifndef PC_BUILD
    if(cacheops('F',0x0,0x0,0x20000)<0)
    {
        LOGE("Error in flushing cache\n");
        return GHW_ERROR_FAIL;
    }
#endif
    return GHW_ERROR_NONE;
}


ghw_error_e GhwComposerV3d::createBinList(GhwImgBuf* dst_img, GhwMemHandle*& bin_list_handle,
        GhwMemHandle*& tile_alloc_handle, GhwMemHandle*& tile_state_handle, u32& bin_size)
{
    u32               width, height;
    u32               w_tiles, h_tiles;
    u32               ta_ipa_addr, ts_ipa_addr, ipa_addr, size;
    void              *virt_addr;
    u32               tile_alloc_size,tile_state_size;
    tile_bin_config_t tile_bin_config;
    unsigned char*    instr;

    dst_img->getGeometry(width, height);
    w_tiles = (width + 31) >> 5;
    h_tiles = (height + 31) >> 5;

    tile_alloc_size = w_tiles * h_tiles * 64;
    tile_state_size = w_tiles * h_tiles * 48;

	tile_alloc_handle = shaderAlloc->alloc(tile_alloc_size);
	if( tile_alloc_handle == NULL ) {
		return GHW_ERROR_FAIL;
		}
	tile_alloc_handle->setName("iT A ");

	tile_state_handle = shaderAlloc->alloc(tile_state_size);
	if( tile_state_handle == NULL ) {
		tile_alloc_handle->release();
		return GHW_ERROR_FAIL;
		}
	tile_state_handle->setName("iT S ");

	bin_list_handle = shaderAlloc->alloc(4096);
	if( bin_list_handle == NULL ) {
		tile_alloc_handle->release();
		tile_state_handle->release();
		return GHW_ERROR_FAIL;
		}
	bin_list_handle->setName("iBin ");
	bin_size = 0;


/* tile alloc and state lock taken and won't be released */
    tile_alloc_handle->lock(ta_ipa_addr, virt_addr, size);
    tile_state_handle->lock(ts_ipa_addr, virt_addr, size);

    tile_bin_config.height           = h_tiles;
    tile_bin_config.width            = w_tiles;
    tile_bin_config.tile_alloc_addr  = ta_ipa_addr;
    tile_bin_config.tile_buffer_size = tile_alloc_size;
    tile_bin_config.tile_state_addr  = ts_ipa_addr;
    tile_bin_config.other            = (1<<2) | (0<<5);

/* bin list taken and won't be released */
    bin_list_handle->lock(ipa_addr, virt_addr, size);
    instr = (unsigned char*)virt_addr + bin_size;

    add_byte (&instr, 112/*KHRN_HW_INSTR_STATE_TILE_BINNING_MODE*/);    //(16)
    memcpy(instr, &tile_bin_config, sizeof(tile_bin_config));
    instr += sizeof(tile_bin_config_t);

    add_byte (&instr, 6 /*KHRN_HW_INSTR_START_TILE_BINNING*/);          //(1)

    /* Ensure primitive format is reset. TODO: is this necessary? */
    add_byte (&instr, 56 /*KHRN_HW_INSTR_PRIMITIVE_LIST_FORMAT*/);      //(2)
    add_byte (&instr, 0x12);     /* 16 bit triangle */


    add_byte (&instr, 102/*KHRN_HW_INSTR_STATE_CLIP*/);                 //(9)
    add_short(&instr, 0);
    add_short(&instr, 0);
    add_short(&instr, width);
    add_short(&instr, height);

    add_byte (&instr, 96 /*KHRN_HW_INSTR_STATE_CFG*/);                  //(4)
    add_byte (&instr, 1 | (1<<1) );      //enfwd, enrev
    add_byte (&instr, 0);             /* zfunc=never, !enzu */
    add_byte (&instr, 1<<1);              //not enez, enezu

    add_byte (&instr, 103 /* KHRN_HW_INSTR_STATE_VIEWPORT_OFFSET*/);    //(5)
    add_short(&instr, 0);
    add_short(&instr, 0);

    bin_size = instr - (unsigned char*)virt_addr;

    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::appendRBSwapShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op,
    GhwMemHandle* bin_list_handle, u32& bin_size , Job* job)
{
    u32             shader_ipa_addr, list_ipa_addr, src_ipa_addr, size;
    void            *shader_virt_addr, *list_virt_addr, *src_virt_addr, *virt_addr;
    GhwMemHandle    *src_handle;
    unsigned char   *instr;
    shader_record_t *shader_record;
    u32             src_width, src_height;
    u32             src_format, src_layout, src_blend;
    u32             src_l, src_t, src_r, src_b, src_crop_w, src_crop_h;
    u32             op_l, op_t, op_r, op_b, transform;
    u32             dst_format, dst_layout, dst_blend;
    u32             *uniform_virt_addr, *vertices_virt_addr;
    u32             code, rb_swap;
    float           l_ratio, r_ratio, t_ratio, b_ratio;
	GhwMemHandle* shader_rec_handle;

	shader_rec_handle = shaderAlloc->alloc(4096);
	if(shader_rec_handle == NULL) {
		return GHW_ERROR_FAIL;
		}
	shader_rec_handle->setName("iS Rec ");

	job->addHandle(shader_rec_handle);
	shader_rec_handle->release();

    op->getDstWindow(op_l,op_t,op_r,op_b);
    op->getTransform(transform);
    op_t &= 0xFFF;
    op_b &= 0xFFF;
    op_l &= 0xFFF;
    op_r &= 0xFFF;

/* shader rec lock taken and will be released */
    shader_rec_handle->lock(shader_ipa_addr, shader_virt_addr, size);
    shader_record = (shader_record_t*)shader_virt_addr;
    shader_record->config = (1) | ( ((3+RB_SWAP_SHADER_NUM_VARYINGS)*4) << 8) | (RB_SWAP_SHADER_NUM_VARYINGS<<24);
    shader_record->uniforms = shader_ipa_addr + sizeof(shader_record_t)*4;
    uniform_virt_addr = ((u32*)shader_virt_addr) + sizeof(shader_record_t);
    shader_record->vertices = shader_record->uniforms + sizeof(rb_swap_shader_uniforms)*4;
    vertices_virt_addr = (u32*)uniform_virt_addr + sizeof(rb_swap_shader_uniforms);
    memcpy(uniform_virt_addr, rb_swap_shader_uniforms, sizeof(rb_swap_shader_uniforms));
    memcpy(vertices_virt_addr, rb_swap_shader_vertices, sizeof(rb_swap_shader_vertices));

    v3dShaders[RB_SWAP_SHADER]->lock(code, virt_addr, size);
    shader_record->code = code;

    vertices_virt_addr[((3+RB_SWAP_SHADER_NUM_VARYINGS)*0)+0] = op_t << 20 | op_l << 4;
    vertices_virt_addr[((3+RB_SWAP_SHADER_NUM_VARYINGS)*1)+0] = op_t << 20 | op_r << 4;
    vertices_virt_addr[((3+RB_SWAP_SHADER_NUM_VARYINGS)*2)+0] = op_b << 20 | op_r << 4;
    vertices_virt_addr[((3+RB_SWAP_SHADER_NUM_VARYINGS)*3)+0] = op_b << 20 | op_l << 4;

    /* bin list lock taken and will be released */
    bin_list_handle->lock(list_ipa_addr, list_virt_addr, size);
    instr = (unsigned char*)list_virt_addr + bin_size;

    /* Update BIN CLE */
    add_byte(&instr, 65 /*KHRN_HW_INSTR_NV_SHADER*/);     //(5)
    add_word(&instr, shader_ipa_addr);

    // Emit a GLDRAWARRAYS instruction
    add_byte(&instr, 33 /*KHRN_HW_INSTR_GLDRAWARRAYS*/);  //(10)
    add_byte(&instr, 6);//Primitive mode (triangle_fan)
    add_word(&instr, 4);//Length (number of vertices)
    add_word(&instr, 0);//Index of first vertex

    bin_size = instr - (unsigned char*)list_virt_addr;
    bin_list_handle->unlock();

    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::appendFbShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op,
    GhwMemHandle* bin_list_handle, u32& bin_size, Job* job)
{
    u32             shader_ipa_addr, list_ipa_addr, src_ipa_addr, size;
    void            *shader_virt_addr, *list_virt_addr, *src_virt_addr, *virt_addr;
    GhwMemHandle    *src_handle;
    unsigned char   *instr;
    shader_record_t *shader_record;
    u32             src_width, src_height;
    u32             src_format, src_layout, src_blend;
    u32             src_l, src_t, src_r, src_b, src_crop_w, src_crop_h;
    u32             op_l, op_t, op_r, op_b, transform;
    u32             dst_format, dst_layout, dst_blend;
    u32             *uniform_virt_addr, *vertices_virt_addr;
    u32             code, rb_swap;
    float           l_ratio, r_ratio, t_ratio, b_ratio;

	GhwMemHandle* shader_rec_handle;

	shader_rec_handle = shaderAlloc->alloc(4096);
	if(shader_rec_handle == NULL) {
		return GHW_ERROR_FAIL;
		}

	shader_rec_handle->setName("iS Rec ");

	job->addHandle(shader_rec_handle);
	shader_rec_handle->release();

    src_img->getMemHandle(src_handle);
    src_img->getGeometry(src_width, src_height);
    src_img->getFormat(src_format, src_layout, src_blend);
    src_img->getCrop(src_l, src_t, src_r, src_b);
    src_handle->lock(src_ipa_addr, src_virt_addr, size);
    src_crop_w = src_r - src_l;
    src_crop_h = src_b - src_t;

    dst_img->getFormat(dst_format, dst_layout, dst_blend);

    op->getDstWindow(op_l,op_t,op_r,op_b);
    op->getTransform(transform);
    op_t &= 0xFFF;
    op_b &= 0xFFF;
    op_l &= 0xFFF;
    op_r &= 0xFFF;

/* shader rec lock taken and will be released */
    shader_rec_handle->lock(shader_ipa_addr, shader_virt_addr, size);
    shader_record = (shader_record_t*)shader_virt_addr;
    shader_record->config = (0) | ( ((3+FB_COMP_SHADER_NUM_VARYINGS)*4) << 8) | (FB_COMP_SHADER_NUM_VARYINGS<<24);
    shader_record->uniforms = shader_ipa_addr + sizeof(shader_record_t)*4;
    uniform_virt_addr = (u32*)shader_virt_addr + sizeof(shader_record_t);
    shader_record->vertices = shader_record->uniforms + sizeof(fb_comp_shader_uniforms)*4;
    vertices_virt_addr = (u32*)uniform_virt_addr + sizeof(fb_comp_shader_uniforms);
    memcpy(uniform_virt_addr, fb_comp_shader_uniforms, sizeof(fb_comp_shader_uniforms));
    memcpy(vertices_virt_addr, fb_comp_shader_vertices, sizeof(fb_comp_shader_vertices));

    rb_swap = 0;
    switch (dst_format)
    {
        case GHW_PIXEL_FORMAT_RGBX_8888:
        case GHW_PIXEL_FORMAT_RGBA_8888:
        case GHW_PIXEL_FORMAT_RGB_565:
            rb_swap = 16;
            break;
    }
    uniform_virt_addr[0] = src_ipa_addr;
    uniform_virt_addr[1] = (src_height << 20)|(src_width << 8) | (0x5);
    uniform_virt_addr[4] = 16 - rb_swap;
    v3dShaders[RGBX_SHADER]->lock(code, virt_addr, size);
    switch (src_format)
    {
        case GHW_PIXEL_FORMAT_RGBA_8888:
            v3dShaders[ARGB_SHADER]->lock(code, virt_addr, size);
            break;

        case GHW_PIXEL_FORMAT_BGRA_8888:
            v3dShaders[ARGB_SHADER]->lock(code, virt_addr, size);
            uniform_virt_addr[4] = rb_swap;
            break;

        case GHW_PIXEL_FORMAT_RGBX_8888:
            uniform_virt_addr[0] |= 0x10;
            break;

        case GHW_PIXEL_FORMAT_BGRX_8888:
            uniform_virt_addr[0] |= 0x10;
            uniform_virt_addr[4] = rb_swap;
            break;

        case GHW_PIXEL_FORMAT_RGB_565:
            uniform_virt_addr[0] |= 0x40;
            break;

        case GHW_PIXEL_FORMAT_BGR_565:
            uniform_virt_addr[0] |= 0x40;
            uniform_virt_addr[4] = rb_swap;
            break;

        case GHW_PIXEL_FORMAT_RGBA_4444:
            v3dShaders[ARGB_SHADER]->lock(code, virt_addr, size);
            uniform_virt_addr[0] |= 0x20;
            break;

        case GHW_PIXEL_FORMAT_RGBA_5551:
            v3dShaders[ARGB_SHADER]->lock(code, virt_addr, size);
            uniform_virt_addr[0] |= 0x30;
            break;

        default:
            LOGE("%s[%p] Invalid src format [%d] \n", __FUNCTION__, this, src_format);
            return GHW_ERROR_FAIL;
            break;
    }
    shader_record->code = code;

    l_ratio = (float)src_l;
    l_ratio = l_ratio/src_width;
    r_ratio = (float)src_r;
    r_ratio = r_ratio/src_width;
    t_ratio = (float)src_t;
    t_ratio = t_ratio/src_height;
    b_ratio = (float)src_b;
    b_ratio = b_ratio/src_height;

    vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+0] = op_t << 20 | op_l << 4;
    ((float*)vertices_virt_addr)[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+3] = t_ratio;
    ((float*)vertices_virt_addr)[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+4] = l_ratio;

    vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+0] = op_t << 20 | op_r << 4;
    ((float*)vertices_virt_addr)[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+3] = t_ratio;
    ((float*)vertices_virt_addr)[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+4] = r_ratio;

    vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+0] = op_b << 20 | op_r << 4;
    ((float*)vertices_virt_addr)[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+3] = b_ratio;
    ((float*)vertices_virt_addr)[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+4] = r_ratio;

    vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+0] = op_b << 20 | op_l << 4;
    ((float*)vertices_virt_addr)[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+3] = b_ratio;
    ((float*)vertices_virt_addr)[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+4] = l_ratio;

    u32 temp1, temp2;
    if(transform & GHW_TRANSFORM_ROT_90) {
        temp1 = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+3];
        temp2 = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+3] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+3];
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+4] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+3] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+3];
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+4] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+3] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+3];
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+4] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+3] = temp1;
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+4] = temp2;

    }

    if(transform & GHW_TRANSFORM_FLIP_H) {
        temp1 = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+3];
        temp2 = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+3] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+3];
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+4] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+3] = temp1;
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+4] = temp2;

        temp1 = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+3];
        temp2 = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+3] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+3];
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+4] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+3] = temp1;
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+4] = temp2;

    }

    if(transform & GHW_TRANSFORM_FLIP_V) {
        temp1 = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+3];
        temp2 = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+3] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+3];
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*0)+4] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+3] = temp1;
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*3)+4] = temp2;

        temp1 = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+3];
        temp2 = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+3] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+3];
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*1)+4] = vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+4];

        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+3] = temp1;
        vertices_virt_addr[((3+FB_COMP_SHADER_NUM_VARYINGS)*2)+4] = temp2;

    }

    /* bin list lock taken and will be released */
    bin_list_handle->lock(list_ipa_addr, list_virt_addr, size);
    instr = (unsigned char*)list_virt_addr + bin_size;

    /* Update BIN CLE */
    add_byte(&instr, 65 /*KHRN_HW_INSTR_NV_SHADER*/);     //(5)
    add_word(&instr, shader_ipa_addr);

    // Emit a GLDRAWARRAYS instruction
    add_byte(&instr, 33 /*KHRN_HW_INSTR_GLDRAWARRAYS*/);  //(10)
    add_byte(&instr, 6);//Primitive mode (triangle_fan)
    add_word(&instr, 4);//Length (number of vertices)
    add_word(&instr, 0);//Index of first vertex

    bin_size = instr - (unsigned char*)list_virt_addr;
    bin_list_handle->unlock();

    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::appendDitherShaderRec(GhwImgBuf* dst_img, GhwMemHandle* bin_list_handle, u32& bin_size , Job* job)
{
    u32             shader_ipa_addr, list_ipa_addr, src_ipa_addr, size;
    void            *shader_virt_addr, *list_virt_addr, *src_virt_addr, *virt_addr;
    GhwMemHandle    *src_handle;
    unsigned char   *instr;
    shader_record_t *shader_record;
    u32             op_l, op_t, op_r, op_b;
    u32             dst_format, dst_layout, dst_blend;
    u32             *uniform_virt_addr, *vertices_virt_addr;
    u32             code;

	GhwMemHandle* shader_rec_handle;
	shader_rec_handle = shaderAlloc->alloc(4096);
	if(shader_rec_handle == NULL) {
		return GHW_ERROR_FAIL;
		}
	shader_rec_handle->setName("iS Rec ");

	job->addHandle(shader_rec_handle);
	shader_rec_handle->release();

    dst_img->getFormat(dst_format, dst_layout, dst_blend);
    dst_img->getCrop(op_l,op_t,op_r,op_b);
    op_t &= 0xFFF;
    op_b &= 0xFFF;
    op_l &= 0xFFF;
    op_r &= 0xFFF;

/* shader rec lock taken and will be released */
    shader_rec_handle->lock(shader_ipa_addr, shader_virt_addr, size);
    shader_record = (shader_record_t*)shader_virt_addr;
    shader_record->config = ( ((3+DITHER_SHADER_NUM_VARYINGS)*4) << 8) | (DITHER_SHADER_NUM_VARYINGS<<24);
    shader_record->uniforms = shader_ipa_addr + sizeof(shader_record_t)*4;
    uniform_virt_addr = (u32*)shader_virt_addr + sizeof(shader_record_t);
    shader_record->vertices = shader_record->uniforms + sizeof(dither_uniforms)*4;
    vertices_virt_addr = (u32*)uniform_virt_addr + sizeof(dither_uniforms);
    memcpy(uniform_virt_addr, dither_uniforms, sizeof(dither_uniforms));
	v3dShaders[DITHER_SHADER_MATRIX]->lock(code, virt_addr, size);
	uniform_virt_addr[DITHER_MATRIX_OFFSET] = code;
	v3dShaders[DITHER_SHADER_MATRIX]->unlock();
    memcpy(vertices_virt_addr, dither_shader_vertices, sizeof(dither_shader_vertices));
    vertices_virt_addr[(3+DITHER_SHADER_NUM_VARYINGS)*0] = op_t << 20 | op_l << 4;
    vertices_virt_addr[(3+DITHER_SHADER_NUM_VARYINGS)*1] = op_t << 20 | op_r << 4;
    vertices_virt_addr[(3+DITHER_SHADER_NUM_VARYINGS)*2] = op_b << 20 | op_r << 4;
    vertices_virt_addr[(3+DITHER_SHADER_NUM_VARYINGS)*3] = op_b << 20 | op_l << 4;
    v3dShaders[DITHER_SHADER]->lock(code, virt_addr, size);
    shader_record->code = code;

    /* bin list lock taken and will be released */
    bin_list_handle->lock(list_ipa_addr, list_virt_addr, size);
    instr = (unsigned char*)list_virt_addr + bin_size;

    /* Update BIN CLE */
    add_byte(&instr, 65 /*KHRN_HW_INSTR_NV_SHADER*/);     //(5)
    add_word(&instr, shader_ipa_addr);

    // Emit a GLDRAWARRAYS instruction
    add_byte(&instr, 33 /*KHRN_HW_INSTR_GLDRAWARRAYS*/);  //(10)
    add_byte(&instr, 6);//Primitive mode (triangle_fan)
    add_word(&instr, 4);//Length (number of vertices)
    add_word(&instr, 0);//Index of first vertex

    bin_size = instr - (unsigned char*)list_virt_addr;
    bin_list_handle->unlock();

    return GHW_ERROR_NONE;
}
ghw_error_e GhwComposerV3d::appendRgb2YuvShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op,
    GhwMemHandle* bin_list_handle, u32& bin_size ,Job* job)
{
    u32             shader_ipa_addr, list_ipa_addr, src_ipa_addr, size;
    void            *shader_virt_addr, *list_virt_addr, *src_virt_addr, *virt_addr;
    GhwMemHandle    *src_handle;
    unsigned char   *instr;
    shader_record_t *shader_record;
    u32             src_width, src_height;
    u32             src_format, src_layout, src_blend;
    u32             src_l, src_t, src_r, src_b, src_crop_w, src_crop_h;
    u32             op_l, op_t, op_r, op_b, transform;
    u32             dst_format, dst_layout, dst_blend;
    u32             *uniform_virt_addr, *vertices_virt_addr;
    u32             code;
    float           l_ratio, r_ratio, t_ratio, b_ratio;
    float           l1_ratio, r1_ratio, t1_ratio, b1_ratio;

	GhwMemHandle* shader_rec_handle;
	shader_rec_handle = shaderAlloc->alloc(4096);
	if(shader_rec_handle == NULL) {
		return GHW_ERROR_FAIL;
		}
	shader_rec_handle->setName("iS Rec ");

	job->addHandle(shader_rec_handle);
	shader_rec_handle->release();

    src_img->getMemHandle(src_handle);
    src_img->getGeometry(src_width, src_height);
    src_img->getFormat(src_format, src_layout, src_blend);
    src_img->getCrop(src_l, src_t, src_r, src_b);
    src_handle->lock(src_ipa_addr, src_virt_addr, size);
    src_crop_w = src_r - src_l;
    src_crop_h = src_b - src_t;

    dst_img->getFormat(dst_format, dst_layout, dst_blend);

    op->getDstWindow(op_l,op_t,op_r,op_b);
    op->getTransform(transform);
	op_r = op_r/2;
	
    op_t &= 0xFFF;
    op_b &= 0xFFF;
    op_l &= 0xFFF;
    op_r &= 0xFFF;

/* shader rec lock taken and will be released */
    shader_rec_handle->lock(shader_ipa_addr, shader_virt_addr, size);
    shader_record = (shader_record_t*)shader_virt_addr;
    shader_record->config = (0) | ( ((3+RGB2YUV422_SHADER_NUM_VARYINGS)*4) << 8) | (RGB2YUV422_SHADER_NUM_VARYINGS<<24);
    shader_record->uniforms = shader_ipa_addr + sizeof(shader_record_t)*4;
    uniform_virt_addr = (u32*)shader_virt_addr + sizeof(shader_record_t);
    if ((dst_format >= GHW_PIXEL_FORMAT_UYVY_422) && (dst_format <= GHW_PIXEL_FORMAT_YVYU_422))
    {
        shader_record->vertices = shader_record->uniforms + sizeof(rgb2yuv422i_shader_uniformsf)*4;
        vertices_virt_addr = (u32*)uniform_virt_addr + sizeof(rgb2yuv422i_shader_uniformsf);

        memcpy(((char*)uniform_virt_addr) , rgb2yuv422i_shader_uniformsf, sizeof(rgb2yuv422i_shader_uniformsf));
		uniform_virt_addr[RGB2YUV422I_TEX_PA_OFFSET] = src_ipa_addr;
		uniform_virt_addr[RGB2YUV422I_TEX_STRIDE_OFFSET] = src_width *4;
		uniform_virt_addr[RGB2YUV422I_X_SCALE_OFFSET] = 8;
		uniform_virt_addr[RGB2YUV422I_TEX_PA_PLUS4] = 4;

		switch(dst_format) {
			case GHW_PIXEL_FORMAT_UYVY_422:
				uniform_virt_addr[RGB2YUV422I_UV] = 0;
				uniform_virt_addr[RGB2YUV422I_Y] = 0;
				uniform_virt_addr[RGB2YUV422I_ROR] = 8;
				break;
			case GHW_PIXEL_FORMAT_VYUY_422:
				uniform_virt_addr[RGB2YUV422I_UV] = 1;
				uniform_virt_addr[RGB2YUV422I_Y] = 0;
				uniform_virt_addr[RGB2YUV422I_ROR] = 8;
				break;
			case GHW_PIXEL_FORMAT_YUYV_422:
				uniform_virt_addr[RGB2YUV422I_UV] = 0;
				uniform_virt_addr[RGB2YUV422I_Y] = 1;
				uniform_virt_addr[RGB2YUV422I_ROR] = 8;
				break;
			case GHW_PIXEL_FORMAT_YVYU_422:
				uniform_virt_addr[RGB2YUV422I_UV] = 1;
				uniform_virt_addr[RGB2YUV422I_Y] = 1;
				uniform_virt_addr[RGB2YUV422I_ROR] = 8;
				break;
			}
		
		switch (src_format)
		{
			case GHW_PIXEL_FORMAT_RGBX_8888:
			case GHW_PIXEL_FORMAT_RGBA_8888:
				break;

			default:
				LOGE("%s[%p] Invalid src format [%d] \n", __FUNCTION__, this, src_format);
				return GHW_ERROR_FAIL;
		}

        v3dShaders[RGB2YUV422I_SHADER]->lock(code, virt_addr, size);
    } else {
        LOGE("%s[%p] Invalid src format [%d] \n", __FUNCTION__, this, src_format);
        return GHW_ERROR_FAIL;
    }

    shader_record->code = code;
    memcpy(vertices_virt_addr, rgb2yuv422i_shader_vertices, sizeof(rgb2yuv422i_shader_vertices));


    vertices_virt_addr[((3+RGB2YUV422_SHADER_NUM_VARYINGS)*0)+0] = op_t << 20 | op_l << 4;
    vertices_virt_addr[((3+RGB2YUV422_SHADER_NUM_VARYINGS)*1)+0] = op_t << 20 | op_r << 4;
    vertices_virt_addr[((3+RGB2YUV422_SHADER_NUM_VARYINGS)*2)+0] = op_b << 20 | op_r << 4;
    vertices_virt_addr[((3+RGB2YUV422_SHADER_NUM_VARYINGS)*3)+0] = op_b << 20 | op_l << 4;

    /* bin list lock taken and will be released */
    bin_list_handle->lock(list_ipa_addr, list_virt_addr, size);
    instr = (unsigned char*)list_virt_addr + bin_size;

    /* Update BIN CLE */
    add_byte(&instr, 65 /*KHRN_HW_INSTR_NV_SHADER*/);     //(5)
    add_word(&instr, shader_ipa_addr);

    // Emit a GLDRAWARRAYS instruction
    add_byte(&instr, 33 /*KHRN_HW_INSTR_GLDRAWARRAYS*/);  //(10)
    add_byte(&instr, 6);//Primitive mode (triangle_fan)
    add_word(&instr, 4);//Length (number of vertices)
    add_word(&instr, 0);//Index of first vertex

    bin_size = instr - (unsigned char*)list_virt_addr;
    bin_list_handle->unlock();

    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::appendYUV444ShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op,
    GhwMemHandle* bin_list_handle, u32& bin_size ,Job* job)
{
    u32             shader_ipa_addr, list_ipa_addr, src_ipa_addr, size;
    void            *shader_virt_addr, *list_virt_addr, *src_virt_addr, *virt_addr;
    GhwMemHandle    *src_handle;
    unsigned char   *instr;
    shader_record_t *shader_record;
    u32             src_width, src_height;
    u32             src_format, src_layout, src_blend;
    u32             src_l, src_t, src_r, src_b, src_crop_w, src_crop_h;
    u32             op_l, op_t, op_r, op_b, transform;
    u32             dst_format, dst_layout, dst_blend;
    u32             *uniform_virt_addr, *vertices_virt_addr;
    u32             code;
    float           l_ratio, r_ratio, t_ratio, b_ratio;
	float			v1_ratio, h1_ratio;

	GhwMemHandle* shader_rec_handle;
	shader_rec_handle = shaderAlloc->alloc(4096);
	if(shader_rec_handle == NULL) {
		return GHW_ERROR_FAIL;
		}
	shader_rec_handle->setName("iS Rec ");

	job->addHandle(shader_rec_handle);
	shader_rec_handle->release();

    src_img->getMemHandle(src_handle);
    src_img->getGeometry(src_width, src_height);
    src_img->getFormat(src_format, src_layout, src_blend);
    src_img->getCrop(src_l, src_t, src_r, src_b);
    src_handle->lock(src_ipa_addr, src_virt_addr, size);
    src_crop_w = src_r - src_l;
    src_crop_h = src_b - src_t;

    dst_img->getFormat(dst_format, dst_layout, dst_blend);

    op->getDstWindow(op_l,op_t,op_r,op_b);
    op->getTransform(transform);

	v1_ratio = 1;
	v1_ratio = v1_ratio/(op_r - op_l);
	h1_ratio = 1;
	h1_ratio = h1_ratio/(op_b - op_t);

	op_r = (op_r)/2;
	op_b = op_b;

    op_t &= 0xFFF;
    op_b &= 0xFFF;
    op_l &= 0xFFF;
    op_r &= 0xFFF;

/* shader rec lock taken and will be released */
    shader_rec_handle->lock(shader_ipa_addr, shader_virt_addr, size);
    shader_record = (shader_record_t*)shader_virt_addr;
    shader_record->config = (0) | ( ((3+YUV444_SHADER_NUM_VARYINGS)*4) << 8) | (YUV444_SHADER_NUM_VARYINGS<<24);
    shader_record->uniforms = shader_ipa_addr + sizeof(shader_record_t)*4;
    uniform_virt_addr = (u32*)shader_virt_addr + sizeof(shader_record_t);
    if ((dst_format >= GHW_PIXEL_FORMAT_UYVY_422) && (dst_format <= GHW_PIXEL_FORMAT_YVYU_422))
    {
        shader_record->vertices = shader_record->uniforms + sizeof(yuv444_shader_uniforms)*4;
        vertices_virt_addr = (u32*)uniform_virt_addr + sizeof(yuv444_shader_uniforms);

        memcpy(((char*)uniform_virt_addr) , yuv444_shader_uniforms, sizeof(yuv444_shader_uniforms));
		uniform_virt_addr[YUV444_TEX_PA_OFFSET0] = src_ipa_addr ;
		uniform_virt_addr[YUV444_TEX_STRIDE_OFFSET0] = (src_height << 20)|(src_width << 8) | (0x5);
		uniform_virt_addr[YUV444_TEX_PA_OFFSET1] = src_ipa_addr ;
		uniform_virt_addr[YUV444_TEX_STRIDE_OFFSET1] = (src_height << 20)|(src_width << 8) | (0x5);
		switch(dst_format) {
			case GHW_PIXEL_FORMAT_YUYV_422:
				uniform_virt_addr[YUV444_TEX_ROTATE_OFFSET] = 0;
				uniform_virt_addr[YUV444_TEX_UV_SWAP_OFFSET] = 0;
				break;
			case GHW_PIXEL_FORMAT_YVYU_422:
				uniform_virt_addr[YUV444_TEX_ROTATE_OFFSET] = 0;
				uniform_virt_addr[YUV444_TEX_UV_SWAP_OFFSET] = 1;
				break;
			case GHW_PIXEL_FORMAT_VYUY_422:
				uniform_virt_addr[YUV444_TEX_ROTATE_OFFSET] = 24;
				uniform_virt_addr[YUV444_TEX_UV_SWAP_OFFSET] = 0;
				break;
			case GHW_PIXEL_FORMAT_UYVY_422:
				uniform_virt_addr[YUV444_TEX_ROTATE_OFFSET] = 24;
				uniform_virt_addr[YUV444_TEX_UV_SWAP_OFFSET] = 1;
				break;
			}

        v3dShaders[YUV444_SHADER]->lock(code, virt_addr, size);
    } else {
        LOGE("%s[%p] Invalid src format [%d] \n", __FUNCTION__, this, src_format);
        return GHW_ERROR_FAIL;
    }

    shader_record->code = code;
    memcpy(vertices_virt_addr, yuv444_shader_vertices, sizeof(yuv444_shader_vertices));

    l_ratio = (float)src_l;
    l_ratio = l_ratio/src_width;
    r_ratio = (float)src_r;
    r_ratio = r_ratio/src_width;
    t_ratio = (float)src_t;
    t_ratio = t_ratio/src_height;
    b_ratio = (float)src_b;
    b_ratio = b_ratio/src_height;

	vertices_virt_addr[((3+YUV444_SHADER_NUM_VARYINGS)*0)+0] = op_t << 20 | op_l << 4;
	vertices_virt_addr[((3+YUV444_SHADER_NUM_VARYINGS)*1)+0] = op_t << 20 | op_r << 4;
	vertices_virt_addr[((3+YUV444_SHADER_NUM_VARYINGS)*2)+0] = op_b << 20 | op_r << 4;
	vertices_virt_addr[((3+YUV444_SHADER_NUM_VARYINGS)*3)+0] = op_b << 20 | op_l << 4;
	switch(transform) {
		case GHW_TRANSFORM_ROT_90:
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+5] = b_ratio - h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+6] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+3] = t_ratio + h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+6] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+3] = t_ratio + h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+6] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+5] = b_ratio - h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+6] = r_ratio;
		break;
	case GHW_TRANSFORM_FLIP_H:
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+4] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+6] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+6] = r_ratio - v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+6] = r_ratio - v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+4] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+6] = l_ratio;
		break;
	case GHW_TRANSFORM_FLIP_V:
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+6] = l_ratio + v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+4] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+6] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+4] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+6] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+6] = l_ratio + v1_ratio;
		break;
	case GHW_TRANSFORM_ROT_180:
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+4] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+6] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+6] = r_ratio - v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+6] = r_ratio - v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+4] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+6] = l_ratio;
		break;
	case GHW_TRANSFORM_ROT_270:
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+3] = b_ratio - h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+6] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+5] = t_ratio + h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+6] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+5] = t_ratio + h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+6] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+3] = b_ratio - h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+6] = r_ratio;
		break;
	case GHW_TRANSFORM_NONE:
	default:
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*0)+6] = l_ratio + v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+4] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*1)+6] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+4] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*2)+6] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YUV444_SHADER_NUM_VARYINGS)*3)+6] = l_ratio + v1_ratio;
		break;
		}

    /* bin list lock taken and will be released */
    bin_list_handle->lock(list_ipa_addr, list_virt_addr, size);
    instr = (unsigned char*)list_virt_addr + bin_size;

    /* Update BIN CLE */
    add_byte(&instr, 65 /*KHRN_HW_INSTR_NV_SHADER*/);     //(5)
    add_word(&instr, shader_ipa_addr);

    // Emit a GLDRAWARRAYS instruction
    add_byte(&instr, 33 /*KHRN_HW_INSTR_GLDRAWARRAYS*/);  //(10)
    add_byte(&instr, 6);//Primitive mode (triangle_fan)
    add_word(&instr, 4);//Length (number of vertices)
    add_word(&instr, 0);//Index of first vertex

    bin_size = instr - (unsigned char*)list_virt_addr;
    bin_list_handle->unlock();

    return GHW_ERROR_NONE;
}


ghw_error_e GhwComposerV3d::appendYscaleShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op,
    GhwMemHandle* bin_list_handle, u32& bin_size ,Job* job)
{
    u32             shader_ipa_addr, list_ipa_addr, src_ipa_addr, size;
    void            *shader_virt_addr, *list_virt_addr, *src_virt_addr, *virt_addr;
    GhwMemHandle    *src_handle;
    unsigned char   *instr;
    shader_record_t *shader_record;
    u32             src_width, src_height;
    u32             src_format, src_layout, src_blend;
    u32             src_l, src_t, src_r, src_b, src_crop_w, src_crop_h;
    u32             op_l, op_t, op_r, op_b, transform;
    u32             dst_format, dst_layout, dst_blend;
    u32             *uniform_virt_addr, *vertices_virt_addr;
    u32             code;
    float           l_ratio, r_ratio, t_ratio, b_ratio;
	float			v1_ratio, h1_ratio;

	GhwMemHandle* shader_rec_handle;
	shader_rec_handle = shaderAlloc->alloc(4096);
	if(shader_rec_handle == NULL) {
		return GHW_ERROR_FAIL;
		}
	shader_rec_handle->setName("iS Rec ");

	job->addHandle(shader_rec_handle);
	shader_rec_handle->release();

    src_img->getMemHandle(src_handle);
    src_img->getGeometry(src_width, src_height);
    src_img->getFormat(src_format, src_layout, src_blend);
    src_img->getCrop(src_l, src_t, src_r, src_b);
    src_handle->lock(src_ipa_addr, src_virt_addr, size);
    src_crop_w = src_r - src_l;
    src_crop_h = src_b - src_t;

    dst_img->getFormat(dst_format, dst_layout, dst_blend);

    op->getDstWindow(op_l,op_t,op_r,op_b);
    op->getTransform(transform);

	v1_ratio = 1;
	v1_ratio = v1_ratio/(op_r - op_l);
	h1_ratio = 1;
	h1_ratio = h1_ratio/(op_b - op_t);

	op_r = (op_r)/4;
	op_b = op_b;

    op_t &= 0xFFF;
    op_b &= 0xFFF;
    op_l &= 0xFFF;
    op_r &= 0xFFF;

/* shader rec lock taken and will be released */
    shader_rec_handle->lock(shader_ipa_addr, shader_virt_addr, size);
    shader_record = (shader_record_t*)shader_virt_addr;
    shader_record->config = (0) | ( ((3+YSCALE_SHADER_NUM_VARYINGS)*4) << 8) | (YSCALE_SHADER_NUM_VARYINGS<<24);
    shader_record->uniforms = shader_ipa_addr + sizeof(shader_record_t)*4;
    uniform_virt_addr = (u32*)shader_virt_addr + sizeof(shader_record_t);
    if ((dst_format >= GHW_PIXEL_FORMAT_Y) && (dst_format <= GHW_PIXEL_FORMAT_V))
    {
        shader_record->vertices = shader_record->uniforms + sizeof(yscale_shader_uniforms)*4;
        vertices_virt_addr = (u32*)uniform_virt_addr + sizeof(yscale_shader_uniforms);

        memcpy(((char*)uniform_virt_addr) , yscale_shader_uniforms, sizeof(yscale_shader_uniforms));
		uniform_virt_addr[YSCALE_TEX_PA_OFFSET0] = src_ipa_addr | 0x50;
		uniform_virt_addr[YSCALE_TEX_STRIDE_OFFSET0] = (src_height << 20)|(src_width << 8) | (0x5); ;
		uniform_virt_addr[YSCALE_TEX_PA_OFFSET1] = src_ipa_addr | 0x50;
		uniform_virt_addr[YSCALE_TEX_STRIDE_OFFSET1] = (src_height << 20)|(src_width << 8) | (0x5); ;
		uniform_virt_addr[YSCALE_TEX_PA_OFFSET2] = src_ipa_addr | 0x50;
		uniform_virt_addr[YSCALE_TEX_STRIDE_OFFSET2] = (src_height << 20)|(src_width << 8) | (0x5); ;
		uniform_virt_addr[YSCALE_TEX_PA_OFFSET3] = src_ipa_addr | 0x50;
		uniform_virt_addr[YSCALE_TEX_STRIDE_OFFSET3] = (src_height << 20)|(src_width << 8) | (0x5); ;

        v3dShaders[YSCALE_SHADER]->lock(code, virt_addr, size);
    } else {
        LOGE("%s[%p] Invalid src format [%d] \n", __FUNCTION__, this, src_format);
        return GHW_ERROR_FAIL;
    }

    shader_record->code = code;
    memcpy(vertices_virt_addr, yscale_shader_vertices, sizeof(yscale_shader_vertices));

    l_ratio = (float)src_l;
    l_ratio = l_ratio/src_width;
    r_ratio = (float)src_r;
    r_ratio = r_ratio/src_width;
    t_ratio = (float)src_t;
    t_ratio = t_ratio/src_height;
    b_ratio = (float)src_b;
    b_ratio = b_ratio/src_height;

	vertices_virt_addr[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+0] = op_t << 20 | op_l << 4;
	vertices_virt_addr[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+0] = op_t << 20 | op_r << 4;
	vertices_virt_addr[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+0] = op_b << 20 | op_r << 4;
	vertices_virt_addr[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+0] = op_b << 20 | op_l << 4;
	switch(transform) {
		case GHW_TRANSFORM_ROT_90:
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+5] = b_ratio - h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+6] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+7] = b_ratio - 2*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+8] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+9] = b_ratio - 3*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+10] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+3] = t_ratio + 3*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+5] = t_ratio + 2*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+6] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+7] = t_ratio + h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+8] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+9] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+10] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+3] = t_ratio + 3*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+5] = t_ratio + 2*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+6] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+7] = t_ratio + h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+8] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+9] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+10] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+5] = b_ratio - h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+6] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+7] = b_ratio - 2*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+8] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+9] = b_ratio - 3*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+10] = r_ratio;
		break;
	case GHW_TRANSFORM_FLIP_H:
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+4] = l_ratio + 3*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+6] = l_ratio + 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+7] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+8] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+9] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+10] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+6] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+7] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+8] = r_ratio - 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+9] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+10] = r_ratio - 3*v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+6] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+7] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+8] = r_ratio - 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+9] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+10] = r_ratio - 3*v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+4] = l_ratio + 3*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+6] = l_ratio + 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+7] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+8] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+9] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+10] = l_ratio;
		break;
	case GHW_TRANSFORM_FLIP_V:
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+6] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+7] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+8] = l_ratio + 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+9] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+10] = l_ratio + 3*v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+4] = r_ratio - 3*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+6] = r_ratio - 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+7] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+8] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+9] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+10] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+4] = r_ratio - 3*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+6] = r_ratio - 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+7] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+8] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+9] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+10] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+6] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+7] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+8] = l_ratio + 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+9] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+10] = l_ratio + 3*v1_ratio;
		break;
	case GHW_TRANSFORM_ROT_180:
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+4] = l_ratio + 3*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+6] = l_ratio + 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+7] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+8] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+9] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+10] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+6] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+7] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+8] = r_ratio - 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+9] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+10] = r_ratio - 3*v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+6] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+7] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+8] = r_ratio - 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+9] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+10] = r_ratio - 3*v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+4] = l_ratio  + 3*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+6] = l_ratio + 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+7] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+8] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+9] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+10] = l_ratio;
		break;
	case GHW_TRANSFORM_ROT_270:
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+3] = b_ratio - 3*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+5] = b_ratio - 2*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+6] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+7] = b_ratio - h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+8] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+9] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+10] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+5] = t_ratio + h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+6] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+7] = t_ratio + 2*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+8] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+9] = t_ratio + 3*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+10] = l_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+5] = t_ratio + h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+6] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+7] = t_ratio + 2*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+8] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+9] = t_ratio + 3*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+10] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+3] = b_ratio  - 3*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+4] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+5] = b_ratio - 2*h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+6] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+7] = b_ratio - h1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+8] = r_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+9] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+10] = r_ratio;
		break;
	case GHW_TRANSFORM_NONE:
	default:
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+6] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+7] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+8] = l_ratio + 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+9] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*0)+10] = l_ratio + 3*v1_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+3] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+4] = r_ratio - 3*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+5] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+6] = r_ratio - 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+7] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+8] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+9] = t_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*1)+10] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+4] = r_ratio - 3*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+6] = r_ratio - 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+7] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+8] = r_ratio - v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+9] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*2)+10] = r_ratio;

	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+3] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+4] = l_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+5] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+6] = l_ratio + v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+7] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+8] = l_ratio + 2*v1_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+9] = b_ratio;
	    ((float*)vertices_virt_addr)[((3+YSCALE_SHADER_NUM_VARYINGS)*3)+10] = l_ratio + 3*v1_ratio;
		break;
		}


    /* bin list lock taken and will be released */
    bin_list_handle->lock(list_ipa_addr, list_virt_addr, size);
    instr = (unsigned char*)list_virt_addr + bin_size;

    /* Update BIN CLE */
    add_byte(&instr, 65 /*KHRN_HW_INSTR_NV_SHADER*/);     //(5)
    add_word(&instr, shader_ipa_addr);

    // Emit a GLDRAWARRAYS instruction
    add_byte(&instr, 33 /*KHRN_HW_INSTR_GLDRAWARRAYS*/);  //(10)
    add_byte(&instr, 6);//Primitive mode (triangle_fan)
    add_word(&instr, 4);//Length (number of vertices)
    add_word(&instr, 0);//Index of first vertex

    bin_size = instr - (unsigned char*)list_virt_addr;
    bin_list_handle->unlock();

    return GHW_ERROR_NONE;
}



ghw_error_e GhwComposerV3d::appendYtileShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op,
    GhwMemHandle* bin_list_handle, u32& bin_size ,Job* job)
{
    u32             shader_ipa_addr, list_ipa_addr, src_ipa_addr, size;
    void            *shader_virt_addr, *list_virt_addr, *src_virt_addr, *virt_addr;
    GhwMemHandle    *src_handle;
    unsigned char   *instr;
    shader_record_t *shader_record;
    u32             src_width, src_height;
    u32             src_format, src_layout, src_blend;
    u32             src_l, src_t, src_r, src_b, src_crop_w, src_crop_h;
    u32             op_l, op_t, op_r, op_b, transform;
    u32             dst_format, dst_layout, dst_blend;
    u32             *uniform_virt_addr, *vertices_virt_addr;
    u32             code;
    float           l_ratio, r_ratio, t_ratio, b_ratio;
    float           l1_ratio, r1_ratio, t1_ratio, b1_ratio;

	GhwMemHandle* shader_rec_handle;
	shader_rec_handle = shaderAlloc->alloc(4096);
	if(shader_rec_handle == NULL) {
		return GHW_ERROR_FAIL;
		}
	shader_rec_handle->setName("iS Rec ");

	job->addHandle(shader_rec_handle);
	shader_rec_handle->release();

    src_img->getMemHandle(src_handle);
    src_img->getGeometry(src_width, src_height);
    src_img->getFormat(src_format, src_layout, src_blend);
    src_img->getCrop(src_l, src_t, src_r, src_b);
    src_handle->lock(src_ipa_addr, src_virt_addr, size);
    src_crop_w = src_r - src_l;
    src_crop_h = src_b - src_t;

    dst_img->getFormat(dst_format, dst_layout, dst_blend);

    op->getDstWindow(op_l,op_t,op_r,op_b);
    op->getTransform(transform);
	op_r = op_r/2;
	op_b = op_b/2;
	
    op_t &= 0xFFF;
    op_b &= 0xFFF;
    op_l &= 0xFFF;
    op_r &= 0xFFF;

/* shader rec lock taken and will be released */
    shader_rec_handle->lock(shader_ipa_addr, shader_virt_addr, size);
    shader_record = (shader_record_t*)shader_virt_addr;
    shader_record->config = (0) | ( ((3+YTILE_SHADER_NUM_VARYINGS)*4) << 8) | (YTILE_SHADER_NUM_VARYINGS<<24);
    shader_record->uniforms = shader_ipa_addr + sizeof(shader_record_t)*4;
    uniform_virt_addr = (u32*)shader_virt_addr + sizeof(shader_record_t);
    if ((dst_format >= GHW_PIXEL_FORMAT_Y) && (dst_format <= GHW_PIXEL_FORMAT_V))
    {
        shader_record->vertices = shader_record->uniforms + sizeof(ytile_shader_uniforms)*4;
        vertices_virt_addr = (u32*)uniform_virt_addr + sizeof(ytile_shader_uniforms);

        memcpy(((char*)uniform_virt_addr) , ytile_shader_uniforms, sizeof(ytile_shader_uniforms));
		uniform_virt_addr[YTILE_TEX_PA_OFFSET] = src_ipa_addr;
		uniform_virt_addr[YTILE_TEX_STRIDE_OFFSET] = src_width ;

        v3dShaders[YTILE_SHADER]->lock(code, virt_addr, size);
    } else {
        LOGE("%s[%p] Invalid src format [%d] \n", __FUNCTION__, this, src_format);
        return GHW_ERROR_FAIL;
    }

    shader_record->code = code;
    memcpy(vertices_virt_addr, ytile_shader_vertices, sizeof(ytile_shader_vertices));


    vertices_virt_addr[((3+YTILE_SHADER_NUM_VARYINGS)*0)+0] = op_t << 20 | op_l << 4;
    vertices_virt_addr[((3+YTILE_SHADER_NUM_VARYINGS)*1)+0] = op_t << 20 | op_r << 4;
    vertices_virt_addr[((3+YTILE_SHADER_NUM_VARYINGS)*2)+0] = op_b << 20 | op_r << 4;
    vertices_virt_addr[((3+YTILE_SHADER_NUM_VARYINGS)*3)+0] = op_b << 20 | op_l << 4;

    /* bin list lock taken and will be released */
    bin_list_handle->lock(list_ipa_addr, list_virt_addr, size);
    instr = (unsigned char*)list_virt_addr + bin_size;

    /* Update BIN CLE */
    add_byte(&instr, 65 /*KHRN_HW_INSTR_NV_SHADER*/);     //(5)
    add_word(&instr, shader_ipa_addr);

    // Emit a GLDRAWARRAYS instruction
    add_byte(&instr, 33 /*KHRN_HW_INSTR_GLDRAWARRAYS*/);  //(10)
    add_byte(&instr, 6);//Primitive mode (triangle_fan)
    add_word(&instr, 4);//Length (number of vertices)
    add_word(&instr, 0);//Index of first vertex

    bin_size = instr - (unsigned char*)list_virt_addr;
    bin_list_handle->unlock();

    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::appendYuvTileShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op,
    GhwMemHandle* bin_list_handle, u32& bin_size ,Job* job)
{
    u32             shader_ipa_addr, list_ipa_addr, src_ipa_addr, size;
    void            *shader_virt_addr, *list_virt_addr, *src_virt_addr, *virt_addr;
    GhwMemHandle    *src_handle;
    unsigned char   *instr;
    shader_record_t *shader_record;
    u32             src_width, src_height;
    u32             src_format, src_layout, src_blend;
    u32             src_l, src_t, src_r, src_b, src_crop_w, src_crop_h;
    u32             op_l, op_t, op_r, op_b, transform;
    u32             dst_format, dst_layout, dst_blend;
    u32             *uniform_virt_addr, *vertices_virt_addr;
    u32             code;
    float           l_ratio, r_ratio, t_ratio, b_ratio;
    float           l1_ratio, r1_ratio, t1_ratio, b1_ratio;

	GhwMemHandle* shader_rec_handle;
	shader_rec_handle = shaderAlloc->alloc(4096);
	if(shader_rec_handle == NULL) {
		return GHW_ERROR_FAIL;
		}
	shader_rec_handle->setName("iS Rec ");

	job->addHandle(shader_rec_handle);
	shader_rec_handle->release();

    src_img->getMemHandle(src_handle);
    src_img->getGeometry(src_width, src_height);
    src_img->getFormat(src_format, src_layout, src_blend);
    src_img->getCrop(src_l, src_t, src_r, src_b);
    src_handle->lock(src_ipa_addr, src_virt_addr, size);
    src_crop_w = src_r - src_l;
    src_crop_h = src_b - src_t;

    dst_img->getFormat(dst_format, dst_layout, dst_blend);

    op->getDstWindow(op_l,op_t,op_r,op_b);
    op->getTransform(transform);
	
    op_t &= 0xFFF;
    op_b &= 0xFFF;
    op_l &= 0xFFF;
    op_r &= 0xFFF;

/* shader rec lock taken and will be released */
    shader_rec_handle->lock(shader_ipa_addr, shader_virt_addr, size);
    shader_record = (shader_record_t*)shader_virt_addr;
    shader_record->config = (0) | ( ((3+YUVTILE_SHADER_NUM_VARYINGS)*4) << 8) | (YUVTILE_SHADER_NUM_VARYINGS<<24);
    shader_record->uniforms = shader_ipa_addr + sizeof(shader_record_t)*4;
    uniform_virt_addr = (u32*)shader_virt_addr + sizeof(shader_record_t);
    if ((src_format >= GHW_PIXEL_FORMAT_UYVY_422) && 
		(src_format <= GHW_PIXEL_FORMAT_YVYU_422) && 
		(dst_format == GHW_PIXEL_FORMAT_YUV_444)  )
    {
        shader_record->vertices = shader_record->uniforms + sizeof(yuvtile_shader_uniforms)*4;
        vertices_virt_addr = (u32*)uniform_virt_addr + sizeof(yuvtile_shader_uniforms);

        memcpy(((char*)uniform_virt_addr) , yuvtile_shader_uniforms, sizeof(yuvtile_shader_uniforms));
		uniform_virt_addr[YUVTILE_TEX_PA_OFFSET] = src_ipa_addr;
		uniform_virt_addr[YUVTILE_TEX_STRIDE_OFFSET] = src_width*2 ;
		switch(src_format) {
			case GHW_PIXEL_FORMAT_YUYV_422:
				uniform_virt_addr[YUVTILE_TEX_ROTATE_OFFSET] = 0;
				uniform_virt_addr[YUVTILE_TEX_UV_SWAP_OFFSET] = 0;
				break;
			case GHW_PIXEL_FORMAT_YVYU_422:
				uniform_virt_addr[YUVTILE_TEX_ROTATE_OFFSET] = 0;
				uniform_virt_addr[YUVTILE_TEX_UV_SWAP_OFFSET] = 1;
				break;
			case GHW_PIXEL_FORMAT_VYUY_422:
				uniform_virt_addr[YUVTILE_TEX_ROTATE_OFFSET] = 24;
				uniform_virt_addr[YUVTILE_TEX_UV_SWAP_OFFSET] = 0;
				break;
			case GHW_PIXEL_FORMAT_UYVY_422:
				uniform_virt_addr[YUVTILE_TEX_ROTATE_OFFSET] = 24;
				uniform_virt_addr[YUVTILE_TEX_UV_SWAP_OFFSET] = 1;
				break;
			}

        v3dShaders[YUVTILE_SHADER]->lock(code, virt_addr, size);
    } else {
        LOGE("%s[%p] Invalid src format [%d] \n", __FUNCTION__, this, src_format);
        return GHW_ERROR_FAIL;
    }

    shader_record->code = code;
    memcpy(vertices_virt_addr, yuvtile_shader_vertices, sizeof(yuvtile_shader_vertices));


    vertices_virt_addr[((3+YUVTILE_SHADER_NUM_VARYINGS)*0)+0] = op_t << 20 | op_l << 4;
    vertices_virt_addr[((3+YUVTILE_SHADER_NUM_VARYINGS)*1)+0] = op_t << 20 | op_r << 4;
    vertices_virt_addr[((3+YUVTILE_SHADER_NUM_VARYINGS)*2)+0] = op_b << 20 | op_r << 4;
    vertices_virt_addr[((3+YUVTILE_SHADER_NUM_VARYINGS)*3)+0] = op_b << 20 | op_l << 4;

    /* bin list lock taken and will be released */
    bin_list_handle->lock(list_ipa_addr, list_virt_addr, size);
    instr = (unsigned char*)list_virt_addr + bin_size;

    /* Update BIN CLE */
    add_byte(&instr, 65 /*KHRN_HW_INSTR_NV_SHADER*/);     //(5)
    add_word(&instr, shader_ipa_addr);

    // Emit a GLDRAWARRAYS instruction
    add_byte(&instr, 33 /*KHRN_HW_INSTR_GLDRAWARRAYS*/);  //(10)
    add_byte(&instr, 6);//Primitive mode (triangle_fan)
    add_word(&instr, 4);//Length (number of vertices)
    add_word(&instr, 0);//Index of first vertex

    bin_size = instr - (unsigned char*)list_virt_addr;
    bin_list_handle->unlock();

    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::appendYuvShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op,
    GhwMemHandle* bin_list_handle, u32& bin_size ,Job* job)
{
    u32             shader_ipa_addr, list_ipa_addr, src_ipa_addr, size;
    void            *shader_virt_addr, *list_virt_addr, *src_virt_addr, *virt_addr;
    GhwMemHandle    *src_handle;
    unsigned char   *instr;
    shader_record_t *shader_record;
    u32             src_width, src_height;
    u32             src_format, src_layout, src_blend;
    u32             src_l, src_t, src_r, src_b, src_crop_w, src_crop_h;
    u32             op_l, op_t, op_r, op_b, transform;
    u32             dst_format, dst_layout, dst_blend;
    u32             *uniform_virt_addr, *vertices_virt_addr;
    u32             code;

	GhwMemHandle* shader_rec_handle;
	shader_rec_handle = shaderAlloc->alloc(4096);
	if(shader_rec_handle == NULL) {
		return GHW_ERROR_FAIL;
		}
	shader_rec_handle->setName("iS Rec ");

	job->addHandle(shader_rec_handle);
	shader_rec_handle->release();

    src_img->getMemHandle(src_handle);
    src_img->getGeometry(src_width, src_height);
    src_img->getFormat(src_format, src_layout, src_blend);
    src_img->getCrop(src_l, src_t, src_r, src_b);
    src_handle->lock(src_ipa_addr, src_virt_addr, size);
    src_crop_w = src_r - src_l;
    src_crop_h = src_b - src_t;

    dst_img->getFormat(dst_format, dst_layout, dst_blend);

    op->getDstWindow(op_l,op_t,op_r,op_b);
    op->getTransform(transform);
	
    op_t &= 0xFFF;
    op_b &= 0xFFF;
    op_l &= 0xFFF;
    op_r &= 0xFFF;

/* shader rec lock taken and will be released */
    shader_rec_handle->lock(shader_ipa_addr, shader_virt_addr, size);
    shader_record = (shader_record_t*)shader_virt_addr;
    shader_record->config = (0) | ( ((3+YUV420_SHADER_NUM_VARYINGS)*4) << 8) | (YUV420_SHADER_NUM_VARYINGS<<24);
    shader_record->uniforms = shader_ipa_addr + sizeof(shader_record_t)*4;
    uniform_virt_addr = (u32*)shader_virt_addr + sizeof(shader_record_t);
    if ((src_format == GHW_PIXEL_FORMAT_YCbCr_420_SP) || (src_format == GHW_PIXEL_FORMAT_YCrCb_420_SP))
    {
        shader_record->vertices = shader_record->uniforms + sizeof(yuv420sp_shader_uniforms)*4;
        vertices_virt_addr = (u32*)uniform_virt_addr + sizeof(yuv420sp_shader_uniforms);
        memcpy(uniform_virt_addr, yuv420sp_shader_uniforms, sizeof(yuv420sp_shader_uniforms));
        uniform_virt_addr[YUV420SP_SHADER_WIDTH_OFFSET] = src_crop_w;
        uniform_virt_addr[YUV420SP_SHADER_YBASE_OFFSET] = src_ipa_addr;
        uniform_virt_addr[YUV420SP_SHADER_UVBASE_OFFSET] = src_ipa_addr + (src_crop_w*src_crop_h);
        if (src_format == GHW_PIXEL_FORMAT_YCrCb_420_SP) {
            uniform_virt_addr[YUV420SP_SHADER_USHIFT_OFFSET] = 8;
            uniform_virt_addr[YUV420SP_SHADER_VSHIFT_OFFSET] = 0;
        } else { //  if(src_format == GHW_PIXEL_FORMAT_YCbCr_420_SP)
            uniform_virt_addr[YUV420SP_SHADER_USHIFT_OFFSET] = 0;
            uniform_virt_addr[YUV420SP_SHADER_VSHIFT_OFFSET] = 8;
        }
        v3dShaders[YUV420SP_SHADER]->lock(code, virt_addr, size);
    } else if ((src_format == GHW_PIXEL_FORMAT_YCbCr_420_P) || (src_format == GHW_PIXEL_FORMAT_YV12)){
        shader_record->vertices = shader_record->uniforms + sizeof(yuv420p_shader_uniforms)*4;
        vertices_virt_addr = (u32*)uniform_virt_addr + sizeof(yuv420p_shader_uniforms);
        memcpy(uniform_virt_addr, yuv420p_shader_uniforms, sizeof(yuv420p_shader_uniforms));
        uniform_virt_addr[YUV420P_SHADER_WIDTH_OFFSET] = src_crop_w;
        uniform_virt_addr[YUV420P_SHADER_YBASE_OFFSET] = src_ipa_addr;
        if (src_format == GHW_PIXEL_FORMAT_YCbCr_420_P) {
            uniform_virt_addr[YUV420P_SHADER_UBASE_OFFSET] = src_ipa_addr + (src_crop_w*src_crop_h);
            uniform_virt_addr[YUV420P_SHADER_VBASE_OFFSET] = src_ipa_addr + (src_crop_w*src_crop_h)+ (src_crop_w*src_crop_h/4);
        } else { //  if(src_format == GHW_PIXEL_FORMAT_YV12)
            uniform_virt_addr[YUV420P_SHADER_VBASE_OFFSET] = src_ipa_addr + (src_crop_w*src_crop_h);
            uniform_virt_addr[YUV420P_SHADER_UBASE_OFFSET] = src_ipa_addr + (src_crop_w*src_crop_h)+ (src_crop_w*src_crop_h/4);
        }
        v3dShaders[YUV420P_SHADER]->lock(code, virt_addr, size);
    } else if ((src_format >= GHW_PIXEL_FORMAT_UYVY_422) || (src_format <= GHW_PIXEL_FORMAT_YVYU_422)){
        shader_record->vertices = shader_record->uniforms + sizeof(yuv422i_shader_uniforms)*4;
        vertices_virt_addr = (u32*)uniform_virt_addr + sizeof(yuv422i_shader_uniforms);
        memcpy(uniform_virt_addr, yuv422i_shader_uniforms, sizeof(yuv422i_shader_uniforms));
        uniform_virt_addr[YUV422I_SHADER_WIDTH_OFFSET] = src_crop_w;
        uniform_virt_addr[YUV422I_SHADER_YBASE_OFFSET] = src_ipa_addr;
        if (src_format == GHW_PIXEL_FORMAT_UYVY_422) {
            uniform_virt_addr[YUV422I_SHADER_YSHIFT_OFFSET] = 8;
            uniform_virt_addr[YUV422I_SHADER_USHIFT_OFFSET] = 0;
            uniform_virt_addr[YUV422I_SHADER_VSHIFT_OFFSET] = 16;
        } else if(src_format == GHW_PIXEL_FORMAT_VYUY_422) {
            uniform_virt_addr[YUV422I_SHADER_YSHIFT_OFFSET] = 8;
            uniform_virt_addr[YUV422I_SHADER_USHIFT_OFFSET] = 16;
            uniform_virt_addr[YUV422I_SHADER_VSHIFT_OFFSET] = 0;
        } else if(src_format == GHW_PIXEL_FORMAT_YUYV_422) {
            uniform_virt_addr[YUV422I_SHADER_YSHIFT_OFFSET] = 0;
            uniform_virt_addr[YUV422I_SHADER_USHIFT_OFFSET] = 8;
            uniform_virt_addr[YUV422I_SHADER_VSHIFT_OFFSET] = 24;
        } else if(src_format == GHW_PIXEL_FORMAT_YVYU_422) {
            uniform_virt_addr[YUV422I_SHADER_YSHIFT_OFFSET] = 0;
            uniform_virt_addr[YUV422I_SHADER_USHIFT_OFFSET] = 24;
            uniform_virt_addr[YUV422I_SHADER_VSHIFT_OFFSET] = 8;
        }
        v3dShaders[YUV422I_SHADER]->lock(code, virt_addr, size);
    } else {
        LOGE("%s[%p] Invalid src format [%d] \n", __FUNCTION__, this, src_format);
        return GHW_ERROR_FAIL;
    }
    shader_record->code = code;
    memcpy(vertices_virt_addr, yuv420_shader_vertices, sizeof(yuv420_shader_vertices));
    vertices_virt_addr[(3+YUV420_SHADER_NUM_VARYINGS)*0] = op_t << 20 | op_l << 4;
    vertices_virt_addr[(3+YUV420_SHADER_NUM_VARYINGS)*1] = op_t << 20 | op_r << 4;
    vertices_virt_addr[(3+YUV420_SHADER_NUM_VARYINGS)*2] = op_b << 20 | op_r << 4;
    vertices_virt_addr[(3+YUV420_SHADER_NUM_VARYINGS)*3] = op_b << 20 | op_l << 4;

    /* bin list lock taken and will be released */
    bin_list_handle->lock(list_ipa_addr, list_virt_addr, size);
    instr = (unsigned char*)list_virt_addr + bin_size;

    /* Update BIN CLE */
    add_byte(&instr, 65 /*KHRN_HW_INSTR_NV_SHADER*/);     //(5)
    add_word(&instr, shader_ipa_addr);

    // Emit a GLDRAWARRAYS instruction
    add_byte(&instr, 33 /*KHRN_HW_INSTR_GLDRAWARRAYS*/);  //(10)
    add_byte(&instr, 6);//Primitive mode (triangle_fan)
    add_word(&instr, 4);//Length (number of vertices)
    add_word(&instr, 0);//Index of first vertex

    bin_size = instr - (unsigned char*)list_virt_addr;
    bin_list_handle->unlock();

    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::closeBinList(GhwMemHandle* bin_list_handle, u32& bin_size)
{
    u32               ipa_addr, size;
    void              *virt_addr;
    unsigned char*    instr;

/* bin list lock taken and will be released */
    bin_list_handle->lock(ipa_addr, virt_addr, size);
    instr = (unsigned char*)virt_addr + bin_size;

#ifdef USE_HW_THREAD_SYNC
    add_byte(&instr, 7);
#endif
    add_byte(&instr, 5 );
    add_byte(&instr, 1 );

    bin_size = instr - (unsigned char*)virt_addr;
    bin_list_handle->unlock();

    return GHW_ERROR_NONE;
}


ghw_error_e GhwComposerV3d::createRendList(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwMemHandle*& rend_list_handle, GhwMemHandle* tile_alloc_mem, u32& rend_size)
{
    ghw_error_e          ret = GHW_ERROR_NONE;
    u32                  width, height;
    u32          src_format, src_layout, src_blend;
    u32          dst_format, dst_layout, dst_blend;
    GhwMemHandle         *handle;
    u32                  ta_ipa_addr, src_ipa_addr, dst_ipa_addr, list_ipa_addr, size;
    void                 *virt_addr, *list_virt_addr;
    tile_render_config_t tile_render_config;
    unsigned char*       instr;
    u32                  v3d_format_code;
    u32                  v3d_load_format;
    u32                  v3d_load_layout;

    dst_img->getGeometry(width, height);

	u32 w_tiles = (width + 31) >> 5;
	u32 h_tiles = (height + 31) >> 5;
	size = ((w_tiles * h_tiles * 20) + 37);
    rend_list_handle = shaderAlloc->alloc(size);
    rend_list_handle->setName("iRend ");

/* src img lock taken and won't be released */
    if (src_img) {
        src_img->getMemHandle(handle);
		src_img->getFormat(src_format, src_layout, src_blend);
        handle->lock(src_ipa_addr, virt_addr, size);
		v3d_load_layout= 1;
		if (src_layout == GHW_MEM_LAYOUT_TILED) {
			v3d_load_layout |= 0x10;
		}
		v3d_load_format = 0;
		switch(src_format) {
			case GHW_PIXEL_FORMAT_RGB_565:
			case GHW_PIXEL_FORMAT_BGR_565:
			case GHW_PIXEL_FORMAT_RGBA_4444:
			case GHW_PIXEL_FORMAT_RGBA_5551:
				v3d_load_format |= 2;
				break;
			}
    }

/* rend img lock taken and won't be released */
    dst_img->getMemHandle(handle);
    dst_img->getGeometry(width, height);
    dst_img->getFormat(dst_format, dst_layout, dst_blend);
    handle->lock(dst_ipa_addr, virt_addr, size);

/* tiled - rgba: 0x44, 565: 0x48 ;  565 dither: 0x40 */
/* raster- rgba: 0x04, 565: 0x08 ;  565 dither: 0x00 */
    v3d_format_code = 0;
    if (dst_layout == GHW_MEM_LAYOUT_TILED) {
        v3d_format_code |= 0x40;
    }
    switch (dst_format) {
        case GHW_PIXEL_FORMAT_RGBA_8888:
        case GHW_PIXEL_FORMAT_BGRA_8888:
        case GHW_PIXEL_FORMAT_RGBX_8888:
        case GHW_PIXEL_FORMAT_BGRX_8888:
            v3d_format_code |= 0x4;
            break;
        case GHW_PIXEL_FORMAT_RGB_565:
		case GHW_PIXEL_FORMAT_BGR_565:
            v3d_format_code |= 0x8;
            break;
		case GHW_PIXEL_FORMAT_YVYU_422:
		case GHW_PIXEL_FORMAT_YUYV_422:
		case GHW_PIXEL_FORMAT_UYVY_422:
		case GHW_PIXEL_FORMAT_VYUY_422:
			v3d_format_code |= 0x4;
			break;
        default:
            LOGE("%s[%p] Dst format[%d] not supported \n", __FUNCTION__, this, dst_format);
            ret = GHW_ERROR_FAIL;
			return ret;
    }
    w_tiles = (width + 31) >> 5;
    h_tiles = (height + 31) >> 5;

/* tile alloc lock taken and will be released */
    if (tile_alloc_mem) {
        tile_alloc_mem->lock(ta_ipa_addr, virt_addr, size);
    }

    /* has to be width if not used for tile conv */
    tile_render_config.width   = width;
    tile_render_config.height  = height;
    tile_render_config.format  = v3d_format_code;
    tile_render_config.other   = 0;
    tile_render_config.fb_addr = dst_ipa_addr;

/* rend list lock taken and won't be released */
    rend_list_handle->lock(list_ipa_addr, list_virt_addr, size);
    instr = (unsigned char*)list_virt_addr + rend_size;


    add_byte (&instr, 114); //(14)
    add_word (&instr, 0);
    add_word (&instr, 0);
    add_word (&instr, 0);
    add_byte (&instr, 0);

    add_byte (&instr, 113); //(11)
    memcpy(instr,&tile_render_config,sizeof(tile_render_config));
    instr += sizeof(tile_render_config);

    add_byte (&instr, 115); //(10) TILE coords
    add_byte (&instr, 0);
    add_byte (&instr, 0);
    add_byte (&instr, 28);
    add_short(&instr, 0);     /* store = none */
    add_word (&instr, 0);     /* no address needed */

    add_byte (&instr, 1);  // (2)
    add_byte (&instr, 1);

#ifdef USE_HW_THREAD_SYNC
    add_byte(&instr, 8);
    add_byte(&instr, 2);
#endif

    for(u32 y=0; y<h_tiles;y++) {
		for(u32 x=0; x<w_tiles; x++) {
			if (src_img) {
				/* has to be src format & layout if not used for tile conv */
                add_byte (&instr,29);  // LOAD general
                add_byte (&instr,v3d_load_layout); // COLOR raster
                add_byte (&instr,v3d_load_format);   // RGBA8888
                add_word (&instr, src_ipa_addr);
            }

            add_byte (&instr, 115);  //(3) TILE coords
            add_byte (&instr, x);
            add_byte (&instr, y);
            add_byte (&instr, 56); //(2)
            add_byte (&instr, 0x12);

            add_byte (&instr, 17); //(5)
            add_word (&instr, ta_ipa_addr +((x + y * w_tiles) * 32));        //(5)

            if ((x == w_tiles - 1) && (y == h_tiles - 1))
            {
                add_byte(&instr, 25);   // (1). Last tile needs special store instruction
            }
            else
            {
                add_byte(&instr, 24);
            }
        }
    }

    rend_size = instr - (unsigned char*)list_virt_addr;

    if (tile_alloc_mem) {
        tile_alloc_mem->unlock();
    }

    return ret;
}

/*    Check for valid images and valid conversions. If invalid print error and return error */
ghw_error_e GhwComposerV3d::isImgProcessValid(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op)
{
    if ((src_img == NULL) || (dst_img == NULL) || (op == NULL)) {
        LOGE("%s[%p] NULL param src[%p] dst[%p] op[%p] \n", __FUNCTION__, this, src_img, dst_img, op);
        return GHW_ERROR_ARG;
    }
    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::barrier(void)
{
    u32 tail_id;

    LOGT("%s[%p] \n", __FUNCTION__, this);
    pthread_mutex_lock(&mLock);
	JobList list;
	JobNode* node = mList.getHead();
	while(node) {
		list.addElement(node->get(),0);
		mList.removeNode(node);
		node = mList.getHead();
		}
    pthread_mutex_unlock(&mLock);

    waitJobCompletion();

    pthread_mutex_lock(&mLock);
	node = list.getHead();
	while(node) {
		delete node->get();
		list.removeNode(node);
		node = list.getHead();
		}
//	shaderAlloc->dump(1);

    pthread_mutex_unlock(&mLock);

    /* Clear the lists created */
    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::imgProcess(GhwImgBuf* src, GhwImgBuf* dst, GhwImgOp* op, u32 sync_flag)
{
    ghw_error_e  ret;
    GhwMemHandle *src_handle, *dst_handle;
    u32          src_format, src_layout, src_blend;
    u32          dst_format, dst_layout, dst_blend;
    GhwMemHandle *rend_list_handle;
    u32          rend_size, rend_list_size, job_id;

    LOGT("%s[%p] \n", __FUNCTION__, this);
    pthread_mutex_lock(&mLock);
    if((ret = isImgProcessValid(src, dst, op)) != GHW_ERROR_NONE) {
        LOGE("%s[%p] failed[%d] \n", __FUNCTION__, this, ret);
        pthread_mutex_unlock(&mLock);
        return ret;
    }
	Job* job = new Job;
	GhwImgBuf* src_img = new GhwImgBufImpl(src);
	GhwImgBuf* dst_img = new GhwImgBufImpl(dst);

    /* Acquire the src and dst buffer and add them to work list for later cleanup */
    src_img->getMemHandle(src_handle);
    src_img->getFormat(src_format, src_layout, src_blend);
	job->addHandle(src_handle);
    dst_img->getMemHandle(dst_handle);
    dst_img->getFormat(dst_format, dst_layout, dst_blend);
	job->addHandle(dst_handle);

	GhwMemHandle *bin_list_handle = NULL, *tile_alloc_handle= NULL, *tile_state_handle= NULL, *shader_rec_handle= NULL;
	u32 		 bin_size, bin_list_size, tile_alloc_size, tile_state_size, shader_rec_size;

	if ( (dst_format >= GHW_PIXEL_FORMAT_UYVY_422) && (dst_format <= GHW_PIXEL_FORMAT_YVYU_422)) {
			u32 dst_width ,dst_height;
			dst_img->getGeometry(dst_width,dst_height);
			dst_img->setGeometry(dst_width/2,dst_height);
		}
	if (src_format == dst_format) {
		u32 src_width ,src_height;
		u32 dst_width ,dst_height;
		src_img->getGeometry(src_width,src_height);
		dst_img->getGeometry(dst_width,dst_height);

		if ( (src_format == GHW_PIXEL_FORMAT_RGB_565) ||
			 (src_format == GHW_PIXEL_FORMAT_BGR_565) ||
			 (src_format == GHW_PIXEL_FORMAT_RGBA_5551) ||
			 (src_format == GHW_PIXEL_FORMAT_RGBA_4444) ) {
	
			if( (src_width != dst_width) || (src_height != dst_height) ) {
				delete job;
				delete src_img ;
				delete dst_img ;
				LOGE("%s[%p] failed[%d] \n", __FUNCTION__, this, GHW_ERROR_ARG);
				pthread_mutex_unlock(&mLock);
				return GHW_ERROR_ARG;
			   }
			dst_img->setGeometry(src_width/2,src_height);
			src_img->setGeometry(src_width/2,src_height);
			dst_img->setFormat(GHW_PIXEL_FORMAT_RGBA_8888,dst_layout,dst_blend);
			src_img->setFormat(GHW_PIXEL_FORMAT_RGBA_8888,src_layout,src_blend);
			}
		if ( (src_format == GHW_PIXEL_FORMAT_Y) ||
			 (src_format == GHW_PIXEL_FORMAT_U) ||
			 (src_format == GHW_PIXEL_FORMAT_V) ) {

			 if((src_layout == GHW_MEM_LAYOUT_RASTER) &&
			 	(dst_layout == GHW_MEM_LAYOUT_TILED) ) {
			 	dst_img->setGeometry(dst_width/2,dst_height/2);
				}
			 if((src_layout == GHW_MEM_LAYOUT_TILED) &&
			 	(dst_layout == GHW_MEM_LAYOUT_RASTER) ) {
			 	dst_img->setGeometry(dst_width/4,dst_height);
			 	}
			}
		}

	/* Allocate bin mem, scratch mem for binning and create bin list */
	bin_size =0;
	createBinList(dst_img, bin_list_handle, tile_alloc_handle, tile_state_handle, bin_size);
    job->addHandle(tile_alloc_handle);
    job->addHandle(tile_state_handle);
    job->addHandle(bin_list_handle);

    if ( ( (src_format >= GHW_PIXEL_FORMAT_YCbCr_420_SP) && (src_format <= GHW_PIXEL_FORMAT_YV12) ) ||
        ( (src_format >= GHW_PIXEL_FORMAT_UYVY_422) && (src_format <= GHW_PIXEL_FORMAT_YVYU_422) ) ) {

		if( dst_format == GHW_PIXEL_FORMAT_YUV_444) {
			ret = appendYuvTileShaderRec(src_img, dst_img, op, bin_list_handle, bin_size ,job) ;
			dst_img->setFormat(GHW_PIXEL_FORMAT_RGBA_8888,dst_layout,dst_blend);
			}
		else {
			shader_rec_size = 0;
			ret = appendYuvShaderRec(src_img, dst_img, op, bin_list_handle, bin_size ,job) ;
			}
		// do not load the YUV data to TLB
		delete src_img;
		src_img = NULL;

	} else if ( (dst_format >= GHW_PIXEL_FORMAT_UYVY_422) && (dst_format <= GHW_PIXEL_FORMAT_YVYU_422)) {
		if( src_format == GHW_PIXEL_FORMAT_YUV_444) {
			appendYUV444ShaderRec(src_img, dst_img, op, bin_list_handle, bin_size ,job);
			}
		else if (src_format == GHW_PIXEL_FORMAT_RGBA_8888) {
			appendRgb2YuvShaderRec(src_img, dst_img, op, bin_list_handle, bin_size ,job);
			}
		delete src_img;
		src_img = NULL;
	}else if(  ((dst_format == GHW_PIXEL_FORMAT_BGRA_8888) && (src_format == GHW_PIXEL_FORMAT_RGBA_8888))  ||
				((dst_format == GHW_PIXEL_FORMAT_RGBA_8888) && (src_format == GHW_PIXEL_FORMAT_BGRA_8888))  ||
				((dst_format == GHW_PIXEL_FORMAT_BGRX_8888) && (src_format == GHW_PIXEL_FORMAT_RGBX_8888))  ||
				((dst_format == GHW_PIXEL_FORMAT_RGBX_8888) && (src_format == GHW_PIXEL_FORMAT_BGRX_8888))  ||
				((dst_format == GHW_PIXEL_FORMAT_RGB_565) && (src_format == GHW_PIXEL_FORMAT_BGR_565))  ||
				((dst_format == GHW_PIXEL_FORMAT_BGR_565) && (src_format == GHW_PIXEL_FORMAT_RGB_565))  )  {
			/* Allocate mem and add a new shader record */
			ret = appendRBSwapShaderRec(src_img, dst_img, op, bin_list_handle, bin_size, job) ;
	}
	else if ( ( ((dst_format == GHW_PIXEL_FORMAT_Y) && (src_format == GHW_PIXEL_FORMAT_Y)) ||
			    ((dst_format == GHW_PIXEL_FORMAT_U) && (src_format == GHW_PIXEL_FORMAT_U)) ||
			    ((dst_format == GHW_PIXEL_FORMAT_V) && (src_format == GHW_PIXEL_FORMAT_V))  ) ) {
			    
			if((src_layout == GHW_MEM_LAYOUT_RASTER) &&
			   (dst_layout == GHW_MEM_LAYOUT_TILED) ) {
				ret = appendYtileShaderRec(src_img, dst_img, op, bin_list_handle, bin_size, job);
				dst_img->setFormat(GHW_PIXEL_FORMAT_RGBA_8888,dst_layout,dst_blend);
				delete src_img;
				src_img = NULL;
			   }
			if((src_layout == GHW_MEM_LAYOUT_TILED) &&
			   (dst_layout == GHW_MEM_LAYOUT_RASTER) ) {
				ret = appendYscaleShaderRec(src_img, dst_img, op, bin_list_handle, bin_size, job);
				dst_img->setFormat(GHW_PIXEL_FORMAT_RGBA_8888,dst_layout,dst_blend);
				delete src_img;
				src_img = NULL;
			   }
		
	}


	/* Terminate the bin list */
	closeBinList(bin_list_handle, bin_size);

    /* Create rend list */
    rend_size = 0;
    createRendList(src_img, dst_img, rend_list_handle, tile_alloc_handle, rend_size);
    LOGT("%s[%p] bin[%d %d] rend[%d %d] ta[%d] ts[%d] \n",
        __FUNCTION__, this, bin_list_size, bin_size, rend_list_size, rend_size, tile_alloc_size, tile_state_size);

    /* Post the job */
	tile_alloc_handle->release();
	tile_state_handle->release();
	bin_list_handle->release();
	job->addHandle(rend_list_handle);
	rend_list_handle->release();
    postJob(bin_list_handle, bin_size, rend_list_handle, rend_size, job);

	if(src_img) delete src_img ;
	if(dst_img) delete dst_img ;
    pthread_mutex_unlock(&mLock);

    if (sync_flag) {
        barrier();
    }

    return GHW_ERROR_NONE;
}

ghw_error_e GhwComposerV3d::compSetFb(GhwImgBuf* fb_img, u32 dither_flag)
{
    u32 bin_list_size, tile_alloc_size, tile_state_size;
	GhwMemHandle* fb_handle = NULL;
	ghw_error_e ret;

    LOGT("%s[%p] \n", __FUNCTION__, this);
    pthread_mutex_lock(&mLock);
    if (composeReqs >= 0) {
        LOGE("%s[%p] setFb without commit \n", __FUNCTION__, this);
        pthread_mutex_unlock(&mLock);
        compCommit(1);
        pthread_mutex_lock(&mLock);
    }
	if( fb_img == NULL) {
        LOGE("%s[%p] fb image is NULL \n", __FUNCTION__, this);
        pthread_mutex_unlock(&mLock);
		return GHW_ERROR_ARG;
		}

    GhwMemHandle        *tileAllocMem = NULL;
    GhwMemHandle        *tileStateMem = NULL;

	/* Acquire the dst buffer */
    composeReqs = 0;
    fbImg = new GhwImgBufImpl(fb_img);
	mJobFB = new Job;
	if(dither_flag <= GHW_DITHER_666) {
	    ditherFlag = dither_flag;
		}

	/* Allocate bin mem, scratch mem for binning and create bin list */
	if( (ret = createBinList(fbImg, binMem, tileAllocMem, tileStateMem, binMemUsed) ) != GHW_ERROR_NONE) {
		delete fbImg;
		delete mJobFB;
		pthread_mutex_unlock(&mLock);
		return ret;
		}

	/* Allocate mem and create rend list */

	rendMemUsed = 0;
	if( (ret = createRendList(NULL, fbImg, rendMem, tileAllocMem, rendMemUsed) ) != GHW_ERROR_NONE) {
		delete fbImg;
		delete mJobFB;
		pthread_mutex_unlock(&mLock);
		return ret;
		}

	mJobFB->addHandle(tileAllocMem);
	tileAllocMem->release();
	mJobFB->addHandle(tileStateMem);
	tileStateMem->release();
	mJobFB->addHandle(binMem);
	mJobFB->addHandle(rendMem);

    fbImg->getMemHandle(fb_handle);
	mJobFB->addHandle(fb_handle);

    pthread_mutex_unlock(&mLock);

    return GHW_ERROR_NONE;

}

ghw_error_e GhwComposerV3d::compDrawRect(GhwImgBuf* src_img, GhwImgOp* op)
{
	ghw_error_e ret = GHW_ERROR_NONE;
    GhwMemHandle *src_handle, *shader_rec_handle;
    u32 shader_rec_size;

    LOGT("%s[%p] \n", __FUNCTION__, this);
    if (composeReqs < 0) {
        LOGE("%s[%p] failed compSetFb not called[%p] \n", __FUNCTION__, this, fbImg);
        return GHW_ERROR_FAIL;
    }

    pthread_mutex_lock(&mLock);

/* Acquire the src buffer and add it to work list for later cleanup */
    src_img->getMemHandle(src_handle);
	mJobFB->addHandle(src_handle);

    composeReqs++;
    ret =  appendFbShaderRec(src_img, fbImg, op, binMem, binMemUsed, mJobFB) ;
    pthread_mutex_unlock(&mLock);
    return ret;
}

ghw_error_e GhwComposerV3d::compCommit(u32 sync_flag)
{
    GhwMemHandle* rend_list_handle, *fb_handle;
    u32           rend_list_size, rend_size, job_id;

    LOGT("%s[%p] \n", __FUNCTION__, this);
    if (composeReqs < 0) {
        LOGE("%s[%p] failed compSetFb not called[%p] \n", __FUNCTION__, this, fbImg);
        return GHW_ERROR_FAIL;
    }

    pthread_mutex_lock(&mLock);

    if (composeReqs > 0) {
        GhwMemHandle *src_handle, *shader_rec_handle;
        u32          shader_rec_size;
        /* One or more texture compose requests happened */
        if (ditherFlag != GHW_DITHER_NONE) {
			appendDitherShaderRec(fbImg, binMem, binMemUsed,mJobFB);
        }
        /* Terminate the bin list */
        closeBinList(binMem, binMemUsed);

        /* Post the job */
        job_id = 2;
        postJob(binMem, binMemUsed, rendMem, rendMemUsed, mJobFB);
		mJobFB = NULL;
    }
	else {
		delete mJobFB;
		mJobFB = NULL;
    }

/* Zero or more texture compose requests happened - cleanup */
	binMem->release();
	rendMem->release();
    binMem          = NULL;
    binMemUsed      = 0;
    rendMem          = NULL;
    rendMemUsed      = 0;
    composeReqs     = -1;
    ditherFlag      = GHW_DITHER_NONE;
    delete fbImg;
    fbImg           = NULL;
    pthread_mutex_unlock(&mLock);

/* Wait for all jobs submitted and purge garbage list */
    if (sync_flag) {
        barrier();
    }

    return GHW_ERROR_NONE;
}

/* Dump complete info */
ghw_error_e GhwComposerV3d::dump(u32 level)
{
    char buf[128];
    int size_left = 127;
    int size_used = 0;
    int size;

    size = snprintf(&buf[size_used], size_left, "\n%s[%p]: fd[%d] ", mName, this, fdV3d);
    size_used += size; size_left -= size;
    LOGD("%s\n", buf);
    if (shaderAlloc) {
        shaderAlloc->dump(level);
    }

	LOGD("workList \n");
	JobNode* node = mList.getHead();
	while(node) {
		node->get()->dump();
		node = node->getNext();
		}

    return GHW_ERROR_NONE;
}

/**
 * GhwImgBuf Implementation
 */
GhwImgBuf* GhwImgBuf::create()
{
    GhwImgBufImpl *img_buf;

    img_buf = new GhwImgBufImpl();
    if (img_buf == NULL) {
        LOGE("%s failed pid[%d]tid[%d] \n", __FUNCTION__, /*getpid(), gettid()*/-1, -1);
        return NULL;
    }
    LOGT("%s success[%p] \n", __FUNCTION__, img_buf);

    return img_buf;
}

GhwImgBuf::~GhwImgBuf()
{
    LOGT("%s[%p] \n", __FUNCTION__, this);
}

/**
 * GhwImgBufImpl Implementation
 */
GhwImgBufImpl::GhwImgBufImpl()
    : memHandle(NULL), mWidth(0), mHeight(0), mFormat(0), mLayout(0), blendType(0), mLeft(0), mTop(0), mRight(0), mBottom(0)
{
    LOGT("%s[%p] \n", __FUNCTION__, this);
}

GhwImgBufImpl::GhwImgBufImpl(GhwImgBuf* buf)
{
    buf->getMemHandle(memHandle);
    if (memHandle) {
        memHandle->acquire();
    }
    buf->getGeometry(mWidth, mHeight);
    buf->getFormat(mFormat, mLayout, blendType);
    buf->getCrop(mLeft, mTop, mRight, mBottom);
    LOGT("%s[%p] \n", __FUNCTION__, this);
}

GhwImgBufImpl::~GhwImgBufImpl()
{
    if(memHandle) {
        memHandle->release();
        memHandle = NULL;
    }
    mWidth    = 0;
    mHeight   = 0;
    mFormat   = 0;
    mLayout   = 0;
    blendType = 0;
    mLeft     = 0;
    mTop      = 0;
    mRight    = 0;
    mBottom   = 0;
    LOGT("%s[%p] \n", __FUNCTION__, this);
}

/* Attach a memory handle to the image buffer object. Acquires (new reference to) memory */
ghw_error_e GhwImgBufImpl::setMemHandle(GhwMemHandle* mem_handle)
{
    ghw_error_e ret= GHW_ERROR_NONE;

    LOGT("%s[%p] mem_handle[%p] memHandle[%p] \n", __FUNCTION__, this, mem_handle, memHandle);
    if (memHandle) {
        memHandle->release();
        memHandle = NULL;
    }
    if(mem_handle) {
        ret = mem_handle->acquire();
        if (ret == GHW_ERROR_NONE) {
            memHandle = mem_handle;
        }else {
            LOGE("%s[%p] mem_handle[%p] acquire failed \n", __FUNCTION__, this, mem_handle);
        }
    }
    return ret;
}

ghw_error_e GhwImgBufImpl::getMemHandle(GhwMemHandle*& mem_handle)
{
    mem_handle = memHandle;
    LOGT("%s[%p] memHandle[%p] \n", __FUNCTION__, this, memHandle);
    return GHW_ERROR_NONE;
}

/* Set the width and height (incl padding) */
ghw_error_e GhwImgBufImpl::setGeometry(u32 width, u32 height)
{
    LOGT("%s[%p] width[%d] height[%d] \n", __FUNCTION__, this, width, height);
    mWidth  = width;
    mHeight = height;
    return GHW_ERROR_NONE;
}

ghw_error_e GhwImgBufImpl::getGeometry(u32& width, u32& height)
{
    width = mWidth;
    height = mHeight;
    LOGT("%s[%p] width[%d] height[%d] \n", __FUNCTION__, this, width, height);
    return GHW_ERROR_NONE;
}

/* Set the format, layout and blending */
ghw_error_e GhwImgBufImpl::setFormat(u32 format, u32 layout,u32 blend_type)
{
    LOGT("%s[%p] format[%d] layout[%d] blend_type[%d] \n", __FUNCTION__, this, format, layout, blend_type);
    if ((format < 1) || format >= GHW_PIXEL_FORMAT_INVALID) {
        LOGE("%s[%p] Invalid format[%d] \n", __FUNCTION__, this, format);
        return GHW_ERROR_ARG;
    }
    if ((layout < 1) || (layout > GHW_MEM_LAYOUT_TILED)) {
        LOGE("%s[%p] Invalid layout[%d] \n", __FUNCTION__, this, layout);
        return GHW_ERROR_ARG;
    }
    if ((blend_type < 1) || (blend_type > GHW_BLEND_SRC_PREMULT)) {
        LOGE("%s[%p] Invalid blend_type[%d] \n", __FUNCTION__, this, blend_type);
        return GHW_ERROR_ARG;
    }
    mFormat = format;
    mLayout = layout;
    blendType = blend_type;
    return GHW_ERROR_NONE;
}

ghw_error_e GhwImgBufImpl::getFormat(u32& format, u32& layout,u32& blend_type)
{
    format = mFormat;
    layout = mLayout;
    blend_type = blendType;
    LOGT("%s[%p] format[%d] layout[%d] blend_type[%d] \n", __FUNCTION__, this, format, layout, blend_type);
    return GHW_ERROR_NONE;
}

/* Set the crop window (valid image) */
ghw_error_e GhwImgBufImpl::setCrop(u32 left, u32 top, u32 right, u32 bottom)
{
    LOGT("%s[%p] left[%d] top[%d] right[%d] bottom[%d] \n", __FUNCTION__, this, left, top, right, bottom);
    mLeft   = left;
    mTop    = top;
    mRight  = right;
    mBottom = bottom;
    return GHW_ERROR_NONE;
}

/* Set the crop window (valid image) */
ghw_error_e GhwImgBufImpl::setCrop(u32 width, u32 height)
{
    LOGT("%s[%p] width[%d] height[%d] \n", __FUNCTION__, this, width, height);
    mLeft   = 0;
    mTop    = 0;
    mRight  = width;
    mBottom = height;
    return GHW_ERROR_NONE;
}

ghw_error_e GhwImgBufImpl::getCrop(u32& left, u32& top, u32& right, u32& bottom)
{
    left   = mLeft;
    top    = mTop;
    right  = mRight;
    bottom = mBottom;
    LOGT("%s[%p] left[%d] top[%d] right[%d] bottom[%d] \n", __FUNCTION__, this, left, top, right, bottom);
    return GHW_ERROR_NONE;
}

/* Dump Image info */
ghw_error_e GhwImgBufImpl::dump(u32 level)
{
    if (level == 0) {
        LOGD("ImgBuf[%p]: memHandle[%p] mWidth[%d] mHeight[%d] mFormat[%d] mLayout[%d] \n",
            this, memHandle, mWidth, mHeight, mFormat, mLayout);
    } else {
        LOGD("IB[%p]: [%p] [%4d x %4d] f[%2d %d %d] c[%3d %3d %4d %4d] \n", this, memHandle, mWidth, mHeight,
            mFormat, mLayout, blendType, mLeft, mTop, mRight, mBottom);
        if (memHandle) {
            memHandle->dump(level);
        }
    }
    return GHW_ERROR_NONE;
}

/**
 * GhwImgOp Implementation
 */
GhwImgOp* GhwImgOp::create()
{
    GhwImgOp *img_op;

    img_op = new GhwImgOpImpl();
    if (img_op == NULL) {
        LOGE("%s failed pid[%d]tid[%d] \n", __FUNCTION__, /*getpid(), gettid()*/-1, -1);
        return NULL;
    }
    LOGT("%s success[%p] \n", __FUNCTION__, img_op);

    return img_op;
}

GhwImgOp::~GhwImgOp()
{
    LOGT("%s[%p] \n", __FUNCTION__, this);
}

/**
 * GhwImgOpImpl Implementation
 */
GhwImgOpImpl::GhwImgOpImpl()
    : mTransform(0), mLeft(0), mTop(0), mRight(0), mBottom(0)
{
    LOGT("%s[%p] \n", __FUNCTION__, this);
}

GhwImgOpImpl::~GhwImgOpImpl()
{
    mTransform = 0;
    mLeft      = 0;
    mTop       = 0;
    mRight     = 0;
    mBottom    = 0;
    LOGT("%s[%p] \n", __FUNCTION__, this);
}

/* Set the dest image (framebuffer) window  - scaling, translation parameter */
ghw_error_e GhwImgOpImpl::setDstWindow(u32 left, u32 top, u32 right, u32 bottom)
{
    LOGT("%s[%p] left[%d] top[%d] right[%d] bottom[%d] \n", __FUNCTION__, this, left, top, right, bottom);
    mLeft   = left;
    mTop    = top;
    mRight  = right;
    mBottom = bottom;
    return GHW_ERROR_NONE;
}

ghw_error_e GhwImgOpImpl::getDstWindow(u32& left, u32& top, u32& right, u32& bottom)
{
    left   = mLeft;
    top    = mTop;
    right  = mRight;
    bottom = mBottom;
    LOGT("%s[%p] left[%d] top[%d] right[%d] bottom[%d] \n", __FUNCTION__, this, left, top, right, bottom);
    return GHW_ERROR_NONE;
}

/* Set the rotation parameter */
ghw_error_e GhwImgOpImpl::setTransform(u32 transform)
{
    LOGT("%s[%p] transform[%d] \n", __FUNCTION__, this, transform);
    mTransform = transform;
    return GHW_ERROR_NONE;
};

ghw_error_e GhwImgOpImpl::getTransform(u32& transform)
{
    transform = mTransform;
    LOGT("%s[%p] transform[%d] \n", __FUNCTION__, this, transform);
    return GHW_ERROR_NONE;
};

/* Dump Image Operation info */
ghw_error_e GhwImgOpImpl::dump(u32 level)
{
    LOGD("ImgOp[%p]: mTransform[%d] Window[%3d %3d %4d %4d] \n",
        this, mTransform, mLeft, mTop, mRight, mBottom);
    return GHW_ERROR_NONE;
}

};

