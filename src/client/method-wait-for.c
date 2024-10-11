/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <signal.h>

#include "client.h"
#include "method-wait-for.h"
#include "usage.h"

#include "libbluechi/common/common.h"

static int parse_status_from_changed_properties(sd_bus_message *m, char **ret_status) {
        if (ret_status == NULL) {
                fprintf(stderr, "NULL pointer to agent status not allowed");
                return -EINVAL;
        }

        int r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
        if (r < 0) {
                fprintf(stderr, "Failed to read changed properties: %s\n", strerror(-r));
                return r;
        }

        for (;;) {
                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv");
                if (r <= 0) {
                        break;
                }

                const char *key = NULL;
                r = sd_bus_message_read(m, "s", &key);
                if (r < 0) {
                        fprintf(stderr, "Failed to read next changed property: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }

                /* only process property changes for the node status */
                if (!streq(key, "Status")) {
                        break;
                }

                /* node status is always of type string */
                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "s");
                if (r < 0) {
                        fprintf(stderr, "Failed to enter content of variant type: %s", strerror(-r));
                        return r;
                }
                char *status = NULL;
                r = sd_bus_message_read(m, "s", &status);
                if (r < 0) {
                        fprintf(stderr, "Failed to read value of changed property: %s\n", strerror(-r));
                        return r;
                }
                *ret_status = strdup(status);

                r = sd_bus_message_exit_container(m);
                if (r < 0) {
                        fprintf(stderr, "Failed to exit container: %s\n", strerror(-r));
                        return r;
                }
        }

        return sd_bus_message_exit_container(m);
}

static int on_agent_status_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        const char *desired_status = userdata;

        (void) sd_bus_message_skip(m, "s");

        _cleanup_free_ char *status = NULL;
        int r = parse_status_from_changed_properties(m, &status);
        if (r < 0) {
                return r;
        }
        if (status == NULL) {
                fprintf(stderr, "Received agent status change signal with missing 'Status' key.");
                return 0;
        }

        if (streq(desired_status, status)) {
                printf("done\n");

                if (raise(SIGTERM) < 0) {
                        fprintf(stderr, "Failed to raise SIGTERM\n");
                        exit(1);
                }
        }

        return 0;
}

static int method_wait_for_on(Client *client, char *desired_status) {
        _cleanup_sd_bus_error_ sd_bus_error prop_error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *prop_reply = NULL;
        const char *status = NULL;

        int r = sd_bus_get_property(
                        client->api_bus,
                        BC_AGENT_DBUS_NAME,
                        BC_AGENT_OBJECT_PATH,
                        AGENT_INTERFACE,
                        "Status",
                        &prop_error,
                        &prop_reply,
                        "s");
        if (r < 0) {
                fprintf(stderr, "Failed to get property: %s\n", strerror(-r));
                if (streq(desired_status, "offline")) {
                        return r;
                }
        } else {
                r = sd_bus_message_read(prop_reply, "s", &status);
                if (r < 0) {
                        fprintf(stderr, "Failed to read message: %s\n", strerror(-r));
                        if (streq(desired_status, "offline")) {
                                return r;
                        }
                }

                if (streq(desired_status, status)) {
                        printf("BlueChi Agent is already %s\n", status);
                        return 0;
                }
        }

        r = sd_bus_match_signal(
                        client->api_bus,
                        NULL,
                        BC_AGENT_DBUS_NAME,
                        BC_AGENT_OBJECT_PATH,
                        "org.freedesktop.DBus.Properties",
                        "PropertiesChanged",
                        on_agent_status_changed,
                        desired_status);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for AgentStatusChanged: %s\n", strerror(-r));
                return r;
        }

        printf("Waiting for BlueChi Agent to be %s...", desired_status);
        fflush(stdout);

        return client_start_event_loop(client);
}

int method_wait_for(Command *command, void *userdata) {
        const Method *method = command->method;
        char *desired_status = command->opargv[0];

        if (!streq(desired_status, "offline") && !streq(desired_status, "online")) {
                fprintf(stderr, "Wrong argument for method %s\n", method->name);
                return -EINVAL;
        }

        return method_wait_for_on(userdata, desired_status);
}

void usage_method_wait_for() {
        usage_print_header();
        usage_print_description("Wait for BlueChi Agent to be the desired status");
        usage_print_usage("bluechictl wait-for [offline|online]");
}
