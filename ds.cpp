// Copyright 2019 David Butler <croepha@gmail.com>



extern EGLDisplay egl_display;
extern EGLSurface egl_surface;
extern bool _debug_should_quit;
extern bool _debug_ignoring_events;
SDL_Window* sdl_window;


// TODO get rid of libDRM.... but, if we do that, then MESA will still depend on it?


int drm_fd = -1;

void ds_do_egl_display() {
    
    // TODO Enumerate???
    drm_fd = open("/dev/dri/card0", O_RDWR);
    assert(drm_fd != -1);
    
    
    auto resources = drmModeGetResources(drm.fd);
    assert(resources);
    
    drmModeConnector* connector = 0;
    for (int i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) break;
        drmModeFreeConnector(connector);
        connector = NULL;
    }
    
    assert(connector);
    
    // TODO: Optionally use a configured resolution...
    
    
    drmModeModeInfo *mode = 0;
    /* find prefered mode or the highest resolution mode: */
    for (int i = 0, int area = 0; i < connector->count_modes; i++) {
        drmModeModeInfo *current_mode = &connector->modes[i];
        
        if (current_mode->type & DRM_MODE_TYPE_PREFERRED) {
            mode = current_mode;
        }
        int current_area = current_mode->hdisplay * current_mode->vdisplay;
        if (current_area > area) {
            mode = current_mode;
            area = current_area;
        }
    }
    assert(mode);
    
    drmModeEncoder* encoder = 0;
    for (i = 0; i < resources->count_encoders; i++) {
        encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
        if (encoder->encoder_id == connector->encoder_id)
            break;
        drmModeFreeEncoder(encoder);
        encoder = NULL;
    }
    
    u32 crtc_id = 0;
    if (encoder) {
        crtc_id = encoder->crtc_id;
    } else {
        crtc_id = find_crtc_for_connector(resources, connector);
        assert(crtc_id != 0);
    }
    
    u32 connector_id = connector->connector_id;
    
    auto gbm_device = gbm_create_device(drm_fd);
    
    auto gbm_surface = gbm_surface_create(
        gbm_device,
        drm_mode->hdisplay, drm_mode->vdisplay,
        GBM_FORMAT_XRGB8888,
        // TODO look into what these options do???
        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    assert(gbm_surface);
    
    
    PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;
    auto eglGetPlatformDisplayEXT =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)
        eglGetProcAddress("eglGetPlatformDisplayEXT");
    assert(eglGetPlatformDisplayEXT);
    
    egl_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, gbm.dev, NULL);
    
}

struct gbm_bo *drm_bo;


void ds_do_specific_setup() {
    
    
    EGLConfig config;
    EGLint num_config;
    
    static EGLint const attribute_list[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };
    
    eglChooseConfig(egl_display, attribute_list, &config, 1, &num_config);
    
    EGLint context_attr_list[] = { 
        EGL_CONTEXT_FLAGS_KHR,  EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE,
    };
    
    
    auto egl_context = eglCreateContext(egl_display, config,
                                        EGL_NO_CONTEXT, context_attr_list);
    auto r5 = eglGetError();
    assert(r5 == EGL_SUCCESS);
    assert(eglGetError() == EGL_SUCCESS);
    assert(egl_context != EGL_NO_CONTEXT);
    
    egl_surface = eglCreateWindowSurface(egl_display, config, 
                                         (EGLNativeWindowType)
                                         gbm_surface, NULL);
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    
    bo = gbm_surface_lock_front_buffer(gbm.surface);
    auto fb = drm_fb_get_from_bo(bo);
    
    auto r2 = drmModeSetCrtc(drm_fd, drm_crtc_id, fb->fb_id, 0, 0,
                             &drm_connector_id, 1, drm_mode);
    assert(!r2);
    
}

void ds_platform_frame() {
    
    
    int waiting_for_flip = 1;
    
    auto next_bo = gbm_surface_lock_front_buffer(gbm_surface);
    auto fb = drm_fb_get_from_bo(next_bo);
    
    auto r1 = drmModePageFlip(drm_fd, drm_crtc_id, fb->fb_id,
                              DRM_MODE_PAGE_FLIP_EVENT, 
                              &waiting_for_flip);
    assert(!r1);
    
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    FD_SET(drm.fd, &fds);
    
    
    drmEventContext evctx = {
        .version = DRM_EVENT_CONTEXT_VERSION,
        .page_flip_handler = page_flip_handler,
    };
    
    while (waiting_for_flip) {
        auto ret = select(drm_fd + 1, &fds, NULL, NULL, NULL);
        if (ret < 0) {
            printf("select err: %s\n", strerror(errno));
            return ret;
        } else if (ret == 0) {
            printf("select timeout!\n");
            return -1;
        } else if (FD_ISSET(0, &fds)) {
            printf("user interrupted!\n");
            break;
        }
        
        drmHandleEvent(drm_fd, &evctx);
    }
    
    gbm_surface_release_buffer(gbm_surface, drm_bo);
    drm_bo = next_bo;
    
}