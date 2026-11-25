/* Pure C test of the voicecontrol_c API: proves the header compiles as C
 * and that the DLL exports work. */
#include "voicecontrol_c.h"

#include <stdio.h>

static int failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #cond);     \
            ++failures;                                                   \
        }                                                                 \
    } while (0)

int main(void) {
    vc_controller* controller;

    CHECK(vc_api_version() == 1);

    controller = vc_create();
    CHECK(controller != NULL);
    vc_destroy(controller);

    vc_destroy(NULL); /* must be accepted */

    if (failures == 0) {
        printf("test_c_api: all checks passed\n");
    }
    return failures == 0 ? 0 : 1;
}
