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

#include "dcc.h"

#define DECLARE_VEC(type, name)               \
  typedef struct name {                       \
    type *data;                               \
    size_t size, capacity;                    \
  } name##_t;                                 \
  name##_t name##_new();                      \
  void name##_push(name##_t *vec, type elem); \
  void name##_free(name##_t *vec);            \
  type* name##_last(name##_t *vec);           \
  type name##_pop(name##_t *vec);


#define DEFINE_VEC_NEW(type, name) \
  name##_t name##_new() {          \
    name##_t v = { 0, 0, 0 };      \
    return v;                      \
  }

#define DEFINE_VEC_PUSH(type, name)                                       \
  void name##_push(name##_t *vec, type elem) {                            \
    if (vec->size >=  vec->capacity) {                                    \
      if (vec->capacity == 0) {                                           \
        vec->capacity = 4;                                                \
        vec->data = dcc_malloc(sizeof(type) * 4);                         \
      } else {                                                            \
        vec->capacity *= 2;                                               \
        vec->data = dcc_realloc(vec->data, sizeof(type) * vec->capacity); \
      }                                                                   \
    }                                                                     \
    vec->data[vec->size++] = elem;                                        \
  }

#define DEFINE_VEC_FREE(type, name, destructor) \
  void name##_free(name##_t *vec) {             \
    if (destructor) {                           \
      VEC_FOREACH_PTR(type, elem, vec) {        \
        (destructor)(elem);                     \
      }                                         \
    }                                           \
    free(vec->data);                            \
  }

#define DEFINE_VEC_LAST(type, name)                         \
  type* name##_last(name##_t *vec) {                        \
    return (vec->size > 0) ? &vec->data[vec->size - 1] : 0; \
  }

#define DEFINE_VEC_POP(type, name) \
  type name##_pop(name##_t *vec) { \
    dcc_assert(vec->size > 0);     \
    return vec->data[--vec->size]; \
  }

#define DEFINE_VEC3(type, name, destructor) \
  DEFINE_VEC_NEW(type, name)                \
  DEFINE_VEC_PUSH(type, name)               \
  DEFINE_VEC_FREE(type, name, destructor)   \
  DEFINE_VEC_LAST(type, name)               \
  DEFINE_VEC_POP(type, name)

#define DEFINE_VEC2(type, name) DEFINE_VEC3(type, name, (void(*)(type *))0)

#define VEC_FOREACH(type, elem, vec) \
  int __i = 0;                       \
  for (type elem = (vec)->data[0];   \
       __i < (vec)->size;            \
       __i++, elem = (vec)->data[__i])

#define VEC_FOREACH_PTR(type, elem, vec) \
  int __i = 0;                           \
  for (type *elem = &(vec)->data[0];     \
       __i < (vec)->size;                \
       __i++, elem = &(vec)->data[__i])

