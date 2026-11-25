#include "voicecontrol_c.h"
#include "voicecontrol/voicecontrol.hpp"

#include <new>
#include <deque>
#include <string>

namespace {

    // Empty C++ string -> NULL for the C side ("no target", "no choice").
    const char* orNull(const std::string& text) {
        return text.empty() ? nullptr : text.c_str();
    }

} // namespace

// One pending order, stored as C++ strings so the text stays alive.
struct PendingEvent {
    std::string actor;
    std::string action;
    std::string target;   // empty = no target
    std::string choice;   // empty = no choice
    float value = 0.0f;
    bool hasValue = false;
};

struct vc_controller {
    vc::VoiceController controller;
    std::deque<PendingEvent> queue;
    PendingEvent current;
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

int vc_handle_text(vc_controller* controller, const char* text) {
    if (controller == nullptr || text == nullptr) {
        return -1;
    }
    try {
        return static_cast<int>(controller->controller.handleText(text).size());
    } catch (...) {
        return -1;
    }
}

int vc_poll(vc_controller* controller, vc_event* out) {
    if (controller == nullptr || out == nullptr) {
        return -1;
    }
    if (controller->queue.empty()) {
        return 0;
    }
    controller->current = std::move(controller->queue.front());
    controller->queue.pop_front();
    out->actor = controller->current.actor.c_str();
    out->action = controller->current.action.c_str();
    out->target = orNull(controller->current.target);
    out->choice = orNull(controller->current.choice);
    out->value = controller->current.value;
    out->has_value = controller->current.hasValue ? 1 : 0;
    return 1;
}