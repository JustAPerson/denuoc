/*
  Copyright (C) 2018  Jason Priest

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdarg.h>
#include <time.h>

#include "dcc.h"

void* dcc_malloc(size_t size) {
  void *result = malloc(size);
  if (!result) {
    dcc_ice("out of memory\n");
  }
  return result;
}

void* dcc_calloc(size_t size, size_t num) {
  void *result = calloc(size, num);
  if (!result) {
    dcc_ice("out of memory\n");
  }
  return result;
}

void* dcc_realloc(void* buffer, size_t size) {
  void *result = realloc(buffer, size);
  if (!result) {
    dcc_ice("out of memory\n");
  }
  return result;
}

void dcc_ice(const char* format, ...) {
  va_list vlist;

  dcc_log(LOG_FATAL, "internal compiler error: ");
  va_start(vlist, format);
  vfprintf(stderr, format, vlist);
  va_end(vlist);
  exit(1);
}

void dcc_nyi(const char* feature) {
  if (feature) {
    dcc_ice("not yet implemented: %s\n", feature);
  } else {
    dcc_ice("not yet implemented");
  }
}

extern log_level active_log_level;

void dcc_log(log_level level, const char* format, ...) {
  if (level >= LOG_COUNT) {
    dcc_ice("invalid log level: %d\n", level);
  }
  if (level < active_log_level) {
    return;
  }

  static char *LOG_STRINGS[] = {
    "trace",
    "debug",
    "error",
    "fatal",
  };


  struct timespec now;
  if (clock_gettime(CLOCK_REALTIME, &now) != CLOCK_REALTIME) {
    time(&now.tv_sec);
    now.tv_nsec = 999999999;
  }
  struct tm *nower = gmtime(&now.tv_sec);
  static char buffer[16];
  strftime(buffer, 16, "%H:%M:%S", nower);

  va_list vlist;
  fprintf(stderr, "%s.%6.6lu %s: ", buffer, now.tv_nsec / 1000, LOG_STRINGS[level]);
  va_start(vlist, format);
  vfprintf(stderr, format, vlist);
  va_end(vlist);
}
