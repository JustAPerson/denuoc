/*
  Copyright (c) 2018 Jason Priest

  Permission is hereby  granted, free of charge, to any  person obtaining a copy
  of this software and associated  documentation files (the "Software"), to deal
  in the Software  without restriction, including without  limitation the rights
  to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
  copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
  IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
  FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
  AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
  LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <stdarg.h>
#include <time.h>

#include "dcc.h"

void* dcc_malloc(size_t size) {
  void *result = malloc(size);
  if (!result) {
    dcc_ice("out of memory");
  }
  return result;
}

void* dcc_calloc(size_t size, size_t num) {
  void *result = calloc(size, num);
  if (!result) {
    dcc_ice("out of memory");
  }
  return result;
}

void* dcc_realloc(void* buffer, size_t size) {
  void *result = realloc(buffer, size);
  if (!result) {
    dcc_ice("out of memory");
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
