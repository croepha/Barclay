// this is largely pulled from kmscube, using it verbatim for now.  Need to dig
//  into it later, try to get rid of dependancies on old Xf86 libraries and
//  libdrm

/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 * Copyright (c) 2012 Rob Clark <rob@ti.com>
 * Copyright (c) 2017 Miouyouyou <Myy> <myy@miouyouyou.fr>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Based on a egl cube test app originally written by Arvin Schnell */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>


#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <assert.h>

#include <easy/profiler.h>


#include "common.hpp"

extern EGLDisplay egl_display;
extern EGLSurface egl_surface;
void egl_init1();
void egl_init2();
void egl_init3(void* native_window);
void egl_frame();

extern bool _debug_should_quit;
extern bool _debug_ignoring_events;

static struct {
    struct gbm_device *dev;
    struct gbm_surface *surface;
} gbm;

static struct {
    int fd;
    drmModeModeInfo *mode;
    uint32_t crtc_id;
    uint32_t connector_id;
} drm;

struct drm_fb {
    struct gbm_bo *bo;
    uint32_t fb_id;
};



static uint32_t find_crtc_for_encoder(const drmModeRes *resources,
                                      const drmModeEncoder *encoder) {
    int i;
    
    for (i = 0; i < resources->count_crtcs; i++) {
        /* possible_crtcs is a bitmask as described here:
   * https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
   */
        const uint32_t crtc_mask = 1 << i;
        const uint32_t crtc_id = resources->crtcs[i];
        if (encoder->possible_crtcs & crtc_mask) {
            return crtc_id;
        }
    }
    
    /* no match found */
    return (u32)-1;
}

static uint32_t find_crtc_for_connector(const drmModeRes *resources,
                                        const drmModeConnector *connector) {
    int i;
    
    for (i = 0; i < connector->count_encoders; i++) {
        const uint32_t encoder_id = connector->encoders[i];
        drmModeEncoder *encoder = drmModeGetEncoder(drm.fd, encoder_id);
        
        if (encoder) {
            const uint32_t crtc_id = find_crtc_for_encoder(resources, encoder);
            
            drmModeFreeEncoder(encoder);
            if (crtc_id != 0) {
                return crtc_id;
            }
        }
    }
    
    /* no match found */
    return (u32)-1;
}




static void page_flip_handler(int , unsigned int ,
                              unsigned int , unsigned int , void *data)
{
    int *waiting_for_flip = (int*)data;
    *waiting_for_flip = 0;
}




static void
drm_fb_destroy_callback(struct gbm_bo*, void *data)
{
    struct drm_fb *fb = (struct drm_fb *)data;
    //struct gbm_device *gbm = (struct gbm_device *)gbm_bo_get_device(bo);
    
    if (fb->fb_id)
        drmModeRmFB(drm.fd, fb->fb_id);
    
    free(fb);
}

static struct drm_fb * drm_fb_get_from_bo(struct gbm_bo *bo)
{
    struct drm_fb *fb = (struct drm_fb *)gbm_bo_get_user_data(bo);
    uint32_t width, height, stride, handle;
    int ret;
    
    if (fb)
        return fb;
    
    fb = (struct drm_fb*)calloc(1, sizeof *fb);
    fb->bo = bo;
    
    width = gbm_bo_get_width(bo);
    height = gbm_bo_get_height(bo);
    stride = gbm_bo_get_stride(bo);
    handle = gbm_bo_get_handle(bo).u32;
    
    ret = drmModeAddFB(drm.fd, width, height, 24, 32, stride, handle, &fb->fb_id);
    if (ret) {
        printf("failed to create fb: %s\n", strerror(errno));
        free(fb);
        return NULL;
    }
    
    gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);
    
    return fb;
}



struct gbm_bo *bo;
struct drm_fb *fb;

fd_set fds;
drmEventContext evctx = {
    .version = DRM_EVENT_CONTEXT_VERSION,
    .page_flip_handler = page_flip_handler,
};

int ret;


void ds_platform_init() {
    
    egl_init1();
    
    drmModeRes *resources;
    drmModeConnector *connector = NULL;
    drmModeEncoder *encoder = NULL;
    int i, area;
    
    drm.fd = open("/dev/dri/card0", O_RDWR);
    
    if (drm.fd < 0) {
        printf("could not open drm device\n");
        assert(0);
    }
    
    resources = drmModeGetResources(drm.fd);
    if (!resources) {
        printf("drmModeGetResources failed: %s\n", strerror(errno));
        assert(0);
    }
    
    /* find a connected connector: */
    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) {
            /* it's connected, let's use this! */
            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }
    
    if (!connector) {
        /* we could be fancy and listen for hotplug events and wait for
   * a connector..
   */
        printf("no connected connector!\n");
        assert(0);
    }
    
    /* find prefered mode or the highest resolution mode: */
    for (i = 0, area = 0; i < connector->count_modes; i++) {
        drmModeModeInfo *current_mode = &connector->modes[i];
        
        printf("Display mode: %hu %hu %d\n",
               current_mode->hdisplay,
               current_mode->vdisplay,
               !!(current_mode->type & DRM_MODE_TYPE_PREFERRED)
               );
        
        // TODO: this is just for debuging because we dont really handle 
        //    variable sized screens, but also, because virtualbox doesn't really 
        //    make good choices about how big to make the screen....
        if (current_mode->hdisplay == SCREEN_WIDTH &&
            current_mode->vdisplay == SCREEN_HEIGHT) {
            drm.mode = current_mode;
            break;
        }
        
        
        if (current_mode->type & DRM_MODE_TYPE_PREFERRED) {
            drm.mode = current_mode;
        }
        
        int current_area = current_mode->hdisplay * current_mode->vdisplay;
        if (current_area > area) {
            drm.mode = current_mode;
            area = current_area;
        }
    }
    
    if (!drm.mode) {
        printf("could not find mode!\n");
        assert(0);
    }
    
    /* find encoder: */
    for (i = 0; i < resources->count_encoders; i++) {
        encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
        if (encoder->encoder_id == connector->encoder_id)
            break;
        drmModeFreeEncoder(encoder);
        encoder = NULL;
    }
    
    if (encoder) {
        drm.crtc_id = encoder->crtc_id;
    } else {
        uint32_t crtc_id = find_crtc_for_connector(resources, connector);
        if (crtc_id == 0) {
            printf("no crtc found!\n");
            assert(0);
        }
        
        drm.crtc_id = crtc_id;
    }
    
    drm.connector_id = connector->connector_id;
    
    gbm.dev = gbm_create_device(drm.fd);
    
    gbm.surface = gbm_surface_create(gbm.dev,
                                     drm.mode->hdisplay, drm.mode->vdisplay,
                                     GBM_FORMAT_XRGB8888,
                                     GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!gbm.surface) {
        printf("failed to create gbm surface\n");
        assert(0);
    }
    
    PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;
    get_platform_display =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
    assert(get_platform_display != NULL);
    
    egl_display = get_platform_display(EGL_PLATFORM_GBM_KHR, gbm.dev, NULL);
    
    egl_init2();
    
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    FD_SET(drm.fd, &fds);
    
    auto native_window = (EGLNativeWindowType)gbm.surface;
    
    egl_init3(native_window);
    
    bo = gbm_surface_lock_front_buffer(gbm.surface);
    fb = drm_fb_get_from_bo(bo);
    
    ret = drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0,
                         &drm.connector_id, 1, drm.mode);
    if (ret) {
        printf("failed to set mode: %s\n", strerror(errno));
        assert(0);
    }
    
}



void ds_platform_frame() {
    EASY_FUNCTION();
    
    egl_frame();
    
    struct gbm_bo *next_bo;
    int waiting_for_flip = 1;
    
    next_bo = gbm_surface_lock_front_buffer(gbm.surface);
    fb = drm_fb_get_from_bo(next_bo);
    
    ret = drmModePageFlip(drm.fd, drm.crtc_id, fb->fb_id,
                          DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
    if (ret) {
        printf("failed to queue page flip: %s\n", strerror(errno));
        assert(0);
    }
    
    while (waiting_for_flip) {
        ret = select(drm.fd + 1, &fds, NULL, NULL, NULL);
        if (ret < 0) {
            printf("select err: %s\n", strerror(errno));
            assert(0);
        } else if (ret == 0) {
            printf("select timeout!\n");
            assert(0);
        } else if (FD_ISSET(0, &fds)) {
            printf("user interrupted!\n");
            break;
        }
        drmHandleEvent(drm.fd, &evctx);
    }
    
    /* release last buffer to render on again: */
    gbm_surface_release_buffer(gbm.surface, bo);
    bo = next_bo;
    
    
}



