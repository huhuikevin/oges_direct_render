/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <stdbool.h>
#include <jni.h>
#include <errno.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

typedef struct native_window_priv native_window_priv;
typedef native_window_priv *(*ptr_ANativeWindowPriv_connect) (ANativeWindow *);
typedef int (*ptr_ANativeWindowPriv_disconnect) (native_window_priv *);
typedef int (*ptr_ANativeWindowPriv_setUsage) (native_window_priv *, bool, int );
typedef int (*ptr_ANativeWindowPriv_setBuffersGeometry) (native_window_priv *, int, int, int );
typedef int (*ptr_ANativeWindowPriv_getMinUndequeued) (native_window_priv *, unsigned int *);
typedef int (*ptr_ANativeWindowPriv_getMaxBufferCount) (native_window_priv *, unsigned int *);
typedef int (*ptr_ANativeWindowPriv_setBufferCount) (native_window_priv *, unsigned int );
typedef int (*ptr_ANativeWindowPriv_setCrop) (native_window_priv *, int, int, int, int);
typedef int (*ptr_ANativeWindowPriv_dequeue) (native_window_priv *, void **);
typedef int (*ptr_ANativeWindowPriv_lock) (native_window_priv *, void *);
typedef int (*ptr_ANativeWindowPriv_queue) (native_window_priv *, void *);
typedef int (*ptr_ANativeWindowPriv_cancel) (native_window_priv *, void *);
typedef int (*ptr_ANativeWindowPriv_lockData) (native_window_priv *, void **, ANativeWindow_Buffer *);
typedef int (*ptr_ANativeWindowPriv_unlockData) (native_window_priv *, void *, bool b_render);
typedef int (*ptr_ANativeWindowPriv_setOrientation) (native_window_priv *, int);

typedef struct
{
    ptr_ANativeWindowPriv_connect connect;
    ptr_ANativeWindowPriv_disconnect disconnect;
    ptr_ANativeWindowPriv_setUsage setUsage;
    ptr_ANativeWindowPriv_setBuffersGeometry setBuffersGeometry;
    ptr_ANativeWindowPriv_getMinUndequeued getMinUndequeued;
    ptr_ANativeWindowPriv_getMaxBufferCount getMaxBufferCount;
    ptr_ANativeWindowPriv_setBufferCount setBufferCount;
    ptr_ANativeWindowPriv_setCrop setCrop;
    ptr_ANativeWindowPriv_dequeue dequeue;
    ptr_ANativeWindowPriv_lock lock;
    ptr_ANativeWindowPriv_lockData lockData;
    ptr_ANativeWindowPriv_unlockData unlockData;
    ptr_ANativeWindowPriv_queue queue;
    ptr_ANativeWindowPriv_cancel cancel;
    ptr_ANativeWindowPriv_setOrientation setOrientation;
} native_window_priv_api_t;


/**
 * Our saved state data.
 */
struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

    int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    struct saved_state state;
};

static native_window_priv_api_t native_win_api;
static native_window_priv *pnative;
static int
LoadNativeWindowPrivAPI(native_window_priv_api_t *native)
{
#if 0
    void *p_library = dlopen("/data/app/com.example.native_activity-2/lib/arm/libnativewin.so", RTLD_NOW);
    if (!p_library)
    {
        LOGI("dlopen libnativewin.so failt, return \n");
        return;
    }
    #undef RTLD_DEFAULT
    #define RTLD_DEFAULT p_library
#endif
    native->connect = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_connect");
    LOGI("connect=%p\n", native->connect);
    native->disconnect = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_disconnect");
    native->setUsage = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_setUsage");
    native->setBuffersGeometry = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_setBuffersGeometry");
    native->getMinUndequeued = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_getMinUndequeued");
    native->getMaxBufferCount = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_getMaxBufferCount");
    native->setBufferCount = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_setBufferCount");
    native->setCrop = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_setCrop");
    native->dequeue = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_dequeue");
    native->lock = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_lock");
    native->lockData = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_lockData");
    native->unlockData = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_unlockData");
    native->queue = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_queue");
    native->cancel = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_cancel");
    native->setOrientation = dlsym(RTLD_DEFAULT, "ANativeWindowPriv_setOrientation");
    
    return native->connect && native->disconnect && native->setUsage &&
        native->setBuffersGeometry && native->getMinUndequeued &&
        native->getMaxBufferCount && native->setBufferCount && native->setCrop &&
        native->dequeue && native->lock && native->lockData && native->unlockData &&
        native->queue && native->cancel && native->setOrientation? 0 : -1;
}

static int loaded = 0;
static void *pNativeBuffer[3];
static void engine_surface_init(struct engine* engine)
{
    int ret;
    void *pbuffer = NULL;

    LOGI("surface init\n");
    if (!loaded && LoadNativeWindowPrivAPI(&native_win_api) == -1){
        LOGI("load function failt \n");
        return;
    }
    loaded = 1;
    pnative = native_win_api.connect((ANativeWindow *)engine->app->window);

    pbuffer = NULL;
    ret = native_win_api.dequeue(pnative, &pbuffer);
    pNativeBuffer[0] = pbuffer;
    ret = native_win_api.queue(pnative, pbuffer);
    LOGI("dequeue=%d, pbuffer=%p\n", ret, pbuffer);

    pbuffer = NULL;
    ret = native_win_api.dequeue(pnative, &pbuffer);
    pNativeBuffer[1] = pbuffer;
    ret = native_win_api.queue(pnative, pbuffer);

    LOGI("dequeue=%d, pbuffer=%p\n", ret, pbuffer);
    pbuffer = NULL;
    ret = native_win_api.dequeue(pnative, &pbuffer);
    pNativeBuffer[2] = pbuffer;
    ret = native_win_api.queue(pnative, pbuffer);

    LOGI("dequeue=%d, pbuffer=%p\n", ret, pbuffer);  


    //native_win_api.disconnect(pnative);  
}


static void engine_queue(void)
{
    void *pbuffer = NULL;
    int ret;
    ret = native_win_api.dequeue(pnative, &pbuffer);
    ret = native_win_api.queue(pnative, pbuffer);

    LOGI("dequeue=%d, pbuffer=%p\n", ret, pbuffer);
}

static void engine_enqueue(void)
{
    int ret = 0;
    int i = 0;
    do {
        ret = native_win_api.queue(pnative, pNativeBuffer[i]);
        LOGI("=============%d---ret=%d========\n", i, ret);
        if (ret == 0)
            break;
    }while(++i < 3);
}

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine) {
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLint w, h, dummy, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

   
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);

    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first EGLConfig that matches our criteria */
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
    context = eglCreateContext(display, config, NULL, NULL);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;
    engine->state.angle = 0;

    // Initialize GL state.
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);
    engine_surface_init(engine);
    LOGI("=============================\n");

    //engine_enqueue();
    return 0;
}

static int _swap = 0;
static int _frames = 0;
/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
    if (engine->display == NULL) {
        // No display.
        return;
    }

    // Just fill the screen with a color.
    glClearColor(((float)engine->state.x)/engine->width, engine->state.angle,
            ((float)engine->state.y)/engine->height, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    if (_swap == 0){
        engine_enqueue();
        _swap = 1;
    }
    _frames ++;
#if 0    
    if (_swap == 0) {
        //eglSwapBuffers(engine->display, engine->surface);
        _swap = 1;
    }else if (_swap == 1){
        int ret = 0;
        int i = 0;
        //glFinish();
        do {
            ret = native_win_api.queue(pnative, pNativeBuffer[i]);
            LOGI("=============%d---ret=%d========\n", i, ret);
            if (ret == 0)
                break;
        }while(++i < 3);
        glFinish();      
        _swap = 2;
    }else if (_swap == 2){
        glFlush();
        glFinish();
        _frames ++;
        if (_frames && (_frames % 60 == 0))
            LOGI("frames = %d\n", _frames);
    }
#endif
    engine->state.x ++;
    engine->state.y ++;
    LOGI("frames = %d\n", _frames);
    //engine_surface_init(engine);
    //LOGI("=============================\n");
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        engine->state.x = AMotionEvent_getX(event, 0);
        engine->state.y = AMotionEvent_getY(event, 0);
        return 1;
    }
    return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                        engine->accelerometerSensor, (1000L/60)*1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
            }
            // Also stop animating.
            engine->animating = 0;
            engine_draw_frame(engine);
            break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    struct engine engine;

    // Make sure glue isn't stripped.
    app_dummy();

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // Prepare to monitor accelerometer
    engine.sensorManager = ASensorManager_getInstance();
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
            ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
            state->looper, LOOPER_ID_USER, NULL, NULL);

    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    }

    // loop waiting for stuff to do.
    //engine->animating = 1;
    //engine->state.x =  121;//AMotionEvent_getX(event, 0);
    //engine->state.y = 112;//AMotionEvent_getY(event, 0);
    while (1) {
    #if 1    
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
                (void**)&source)) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                            &event, 1) > 0) {
                       // LOGI("accelerometer: x1=%f y=%f z=%f",
                         //       event.acceleration.x, event.acceleration.y,
                           //     event.acceleration.z);
                    }
                }
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }
    #endif
        if (engine.animating) {
            // Done with events; draw next animation frame.
            engine.state.angle += .3f;
            if (engine.state.angle > 1) {
                engine.state.angle = 0;
            }

            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            engine_draw_frame(&engine);
            usleep(500*1000);
        }
    }
}
//END_INCLUDE(all)
