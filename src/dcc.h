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

#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

void* dcc_malloc(size_t size);
void* dcc_calloc(size_t size, size_t num);
void* dcc_realloc(void* buffer, size_t size);

void dcc_ice(const char* format, ...);
void dcc_nyi(const char* feature);

typedef enum  {
  LOG_TRACE,
  LOG_DEBUG,
  LOG_ERROR,
  LOG_FATAL,

  LOG_COUNT,
} log_level;
void dcc_log(log_level level, const char* format, ...);

#define TRACE(...) dcc_log(LOG_TRACE, __VA_ARGS__)
#define DEBUG(...) dcc_log(LOG_DEBUG, __VA_ARGS__)
#define ERROR(...) dcc_log(LOG_ERROR, __VA_ARGS__)
#define FATAL(...) dcc_log(LOG_FATAL, __VA_ARGS__)

#define dcc_assert(cond)                                                \
if (!(cond)) {                                                          \
  dcc_ice("%s:%d: assertion failed (%s)\n", __FILE__, __LINE__, #cond); \
}
