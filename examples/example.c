#include <stdio.h>
#include "sentry.h"

int main(void) {
        sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://15160f69b5dc9faa778f1d8ee9358b0e@o4508507428749312.ingest.us.sentry.io/4508508876374016");
    sentry_options_set_debug(options, 1); // Enable debug output

    sentry_options_set_handler_path(options, "../crashpad_build/handler/crashpad_handler");

    if (sentry_init(options) != 0) {
        fprintf(stderr, "Sentry initialization failed!\n");
        return 1;
    }

    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_INFO,
        "example",
        "Hello, Sentry!"
    );
    sentry_capture_event(event);

    sentry_shutdown();
    return 0;
}
