/* Tests of the C API, written in C on purpose: if this compiles,
   voicecontrol_c.h is usable from any language. */
#include <stdio.h>
#include "voicecontrol_c.h"

static int failures = 0;

/* Prints OK or FAILED for one condition and counts the failures. */
static void check(int condition, const char* description) {
    printf("%s  %s\n", condition ? "OK    " : "FAILED", description);
    if (!condition) {
        failures++;
    }
}

int main(void) {
    check(vc_api_version() == 1, "vc_api_version returns 1");

    vc_controller* controller = vc_create();
    check(controller != NULL, "vc_create returns a controller");

    /* vc_handle_text */
    check(vc_handle_text(NULL, "cube avance") == -1, "NULL controller rejected");
    check(vc_handle_text(controller, NULL) == -1, "NULL text rejected");
    check(vc_handle_text(controller, "cube avance") == 0, "empty vocabulary executes nothing");

    /* vc_poll */
    vc_event event;
    check(vc_poll(NULL, &event) == -1, "poll rejects NULL controller");
    check(vc_poll(controller, NULL) == -1, "poll rejects NULL event");
    check(vc_poll(controller, &event) == 0, "queue starts empty");

    vc_destroy(controller);
    vc_destroy(NULL);   /* must not crash */
    check(1, "vc_destroy accepts NULL");

    printf(failures == 0 ? "C API: all tests passed\n" : "C API: %d test(s) FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}