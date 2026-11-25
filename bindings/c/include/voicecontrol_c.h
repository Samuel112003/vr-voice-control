#pragma once
/*
 * C API of the voicecontrol library, for languages that cannot use C++
 * (Unity C# through P/Invoke, ...). Unreal and C++ programs use the C++ API.
 * Every function only translates to vc::VoiceController.
 */

#if defined(_WIN32)
#ifdef VC_C_API_BUILD
#define VC_C_API __declspec(dllexport)
#else
#define VC_C_API __declspec(dllimport)
#endif
#else
#define VC_C_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

    /* Opaque handle: the caller never sees what is inside. */
    typedef struct vc_controller vc_controller;

    /* Version of the C API (incremented when functions change). */
    VC_C_API int vc_api_version(void);

    /* Creates a controller with an empty vocabulary. Returns NULL on failure. */
    VC_C_API vc_controller* vc_create(void);

    /* Destroys a controller created by vc_create. NULL is accepted. */
    VC_C_API void vc_destroy(vc_controller* controller);

    /* Parses a UTF-8 sentence and executes the orders found.
    Returns the number of executed orders, or -1 on error. */
    VC_C_API int vc_handle_text(vc_controller* controller, const char* text);

    /* One recognized order for one actor. An order addressed to several
   actors produces one event per actor. */
    typedef struct vc_event {
        const char* actor;   /* who execute */
        const char* action;  /* action id */
        const char* target;  /* actor named after the action or NULL */
        const char* choice;  /* chosen word, e.g. "gauche" in "tourne a gauche", or NULL */
        float value;         /* numeric parameter, e.g. 2 in "avance de 2" */
        int has_value;       /* 1 if value was spoken, 0 otherwise */
    } vc_event;

    /* Takes the oldest pending event.
    Returns 1 if `out` was filled, 0 if the queue is empty, -1 on error.
    The strings stay valid until the next call to vc_poll or vc_destroy. */
    VC_C_API int vc_poll(vc_controller* controller, vc_event* out);

#ifdef __cplusplus
}
#endif