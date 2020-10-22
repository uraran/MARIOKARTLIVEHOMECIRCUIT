/***
  This file is part of systemd.

  Copyright 2008-2012 Kay Sievers <kay@vrfy.org>

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "libudev.h"
#include "libudev-private.h"
#include "missing.h"

/**
 * SECTION:libudev
 * @short_description: libudev context
 *
 * The context contains the default values read from the udev config file,
 * and is passed to all library operations.
 */

/**
 * udev:
 *
 * Opaque object representing the library context.
 */
struct udev {
        int refcount;
        void (*log_fn)(struct udev *udev,
                       int priority, const char *file, int line, const char *fn,
                       const char *format, va_list args);
        void *userdata;
        struct udev_list properties_list;
        int log_priority;
};

void udev_log(struct udev *udev,
              int priority, const char *file, int line, const char *fn,
              const char *format, ...)
{
        va_list args;

        va_start(args, format);
        udev->log_fn(udev, priority, file, line, fn, format, args);
        va_end(args);
}

_printf_(6,0)
static void log_stderr(struct udev *udev,
                       int priority, const char *file, int line, const char *fn,
                       const char *format, va_list args)
{
        fprintf(stderr, "libudev: %s: ", fn);
        vfprintf(stderr, format, args);
}

/**
 * udev_get_userdata:
 * @udev: udev library context
 *
 * Retrieve stored data pointer from library context. This might be useful
 * to access from callbacks like a custom logging function.
 *
 * Returns: stored userdata
 **/
_public_ void *udev_get_userdata(struct udev *udev)
{
        if (udev == NULL)
                return NULL;
        return udev->userdata;
}

/**
 * udev_set_userdata:
 * @udev: udev library context
 * @userdata: data pointer
 *
 * Store custom @userdata in the library context.
 **/
_public_ void udev_set_userdata(struct udev *udev, void *userdata)
{
        if (udev == NULL)
                return;
        udev->userdata = userdata;
}

/**
 * udev_new:
 *
 * Create udev library context. This reads the udev configuration
 * file, and fills in the default values.
 *
 * The initial refcount is 1, and needs to be decremented to
 * release the resources of the udev library context.
 *
 * Returns: a new udev library context
 **/
_public_ struct udev *udev_new(void)
{
        struct udev *udev;
        FILE *f;

        udev = new0(struct udev, 1);
        if (udev == NULL)
                return NULL;
        udev->refcount = 1;
        udev->log_fn = log_stderr;
        udev->log_priority = LOG_INFO;
        udev_list_init(udev, &udev->properties_list, true);

        f = fopen( UDEV_CONF_FILE, "re");
        if (f != NULL) {
                char line[UTIL_LINE_SIZE];
                int line_nr = 0;

                while (fgets(line, sizeof(line), f)) {
                        size_t len;
                        char *key;
                        char *val;

                        line_nr++;

                        /* find key */
                        key = line;
                        while (isspace(key[0]))
                                key++;

                        /* comment or empty line */
                        if (key[0] == '#' || key[0] == '\0')
                                continue;

                        /* split key/value */
                        val = strchr(key, '=');
                        if (val == NULL) {
                                udev_err(udev, "missing <key>=<value> in " UDEV_CONF_FILE "[%i]; skip line\n", line_nr);
                                continue;
                        }
                        val[0] = '\0';
                        val++;

                        /* find value */
                        while (isspace(val[0]))
                                val++;

                        /* terminate key */
                        len = strlen(key);
                        if (len == 0)
                                continue;
                        while (isspace(key[len-1]))
                                len--;
                        key[len] = '\0';

                        /* terminate value */
                        len = strlen(val);
                        if (len == 0)
                                continue;
                        while (isspace(val[len-1]))
                                len--;
                        val[len] = '\0';

                        if (len == 0)
                                continue;

                        /* unquote */
                        if (val[0] == '"' || val[0] == '\'') {
                                if (val[len-1] != val[0]) {
                                        udev_err(udev, "inconsistent quoting in " UDEV_CONF_FILE"[%i]; skip line\n", line_nr);
                                        continue;
                                }
                                val[len-1] = '\0';
                                val++;
                        }

                        if (streq(key, "udev_log")) {
                                udev_set_log_priority(udev, util_log_priority(val));
                                continue;
                        }
                }
                fclose(f);
        }

        return udev;
}

/**
 * udev_ref:
 * @udev: udev library context
 *
 * Take a reference of the udev library context.
 *
 * Returns: the passed udev library context
 **/
_public_ struct udev *udev_ref(struct udev *udev)
{
        if (udev == NULL)
                return NULL;
        udev->refcount++;
        return udev;
}

/**
 * udev_unref:
 * @udev: udev library context
 *
 * Drop a reference of the udev library context. If the refcount
 * reaches zero, the resources of the context will be released.
 *
 * Returns: the passed udev library context if it has still an active reference, or #NULL otherwise.
 **/
_public_ struct udev *udev_unref(struct udev *udev)
{
        if (udev == NULL)
                return NULL;
        udev->refcount--;
        if (udev->refcount > 0)
                return udev;
        udev_list_cleanup(&udev->properties_list);
        free(udev);
        return NULL;
}

/**
 * udev_set_log_fn:
 * @udev: udev library context
 * @log_fn: function to be called for logging messages
 *
 * The built-in logging writes to stderr. It can be
 * overridden by a custom function, to plug log messages
 * into the users' logging functionality.
 *
 **/
_public_ void udev_set_log_fn(struct udev *udev,
                     void (*log_fn)(struct udev *udev,
                                    int priority, const char *file, int line, const char *fn,
                                    const char *format, va_list args))
{
        udev->log_fn = log_fn;
        udev_dbg(udev, "custom logging function %p registered\n", log_fn);
}

/**
 * udev_get_log_priority:
 * @udev: udev library context
 *
 * The initial logging priority is read from the udev config file
 * at startup.
 *
 * Returns: the current logging priority
 **/
_public_ int udev_get_log_priority(struct udev *udev)
{
        return udev->log_priority;
}

/**
 * udev_set_log_priority:
 * @udev: udev library context
 * @priority: the new logging priority
 *
 * Set the current logging priority. The value controls which messages
 * are logged.
 **/
_public_ void udev_set_log_priority(struct udev *udev, int priority)
{
        char num[32];

        udev->log_priority = priority;
        snprintf(num, sizeof(num), "%u", udev->log_priority);
        udev_add_property(udev, "UDEV_LOG", num);
}

struct udev_list_entry *udev_add_property(struct udev *udev, const char *key, const char *value)
{
        if (value == NULL) {
                struct udev_list_entry *list_entry;

                list_entry = udev_get_properties_list_entry(udev);
                list_entry = udev_list_entry_get_by_name(list_entry, key);
                if (list_entry != NULL)
                        udev_list_entry_delete(list_entry);
                return NULL;
        }
        return udev_list_entry_add(&udev->properties_list, key, value);
}

struct udev_list_entry *udev_get_properties_list_entry(struct udev *udev)
{
        return udev_list_get_entry(&udev->properties_list);
}
