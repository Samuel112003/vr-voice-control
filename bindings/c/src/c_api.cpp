#include "voicecontrol_c.h"
#include "voicecontrol/voicecontrol.hpp"

#include <new>

// What the opaque handle really contains.
struct vc_controller {
    vc::VoiceController controller;
};

int vc_api_version(void) {
    return 1;
}

vc_controller* vc_create(void) {
    // No exception may cross the C boundary: allocation or construction failure -> NULL.
    try {
        return new (std::nothrow) vc_controller();
    } catch (...) {
        return nullptr;
    }
}

void vc_destroy(vc_controller* controller) {
    delete controller;
}
