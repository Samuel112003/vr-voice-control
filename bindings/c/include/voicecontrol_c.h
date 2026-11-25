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

#ifdef __cplusplus
}
#endif