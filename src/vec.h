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

#pragma once

#include "dcc.h"

#define DECLARE_VEC(type, name)               \
  typedef struct {                            \
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
    VEC_FOREACH_PTR(type, elem, vec) {          \
      destructor(elem);                         \
    }                                           \
    free(vec->data);                            \
  }

#define DEFINE_VEC_LAST(type, name)                         \
  type* name##_last(name##_t *vec) {                        \
    return (vec->size > 0) ? &vec->data[vec->size - 1] : 0; \
  }

#define DEFINE_VEC_POP(type, name)   \
  type name##_pop(name##_t *vec) {   \
    dcc_assert(vec->size > 0);       \
    return vec->data[vec->size - 1]; \
  }

#define DEFINE_VEC3(type, name, destructor) \
  DEFINE_VEC_NEW(type, name)                \
  DEFINE_VEC_PUSH(type, name)               \
  DEFINE_VEC_FREE(type, name, destructor)   \
  DEFINE_VEC_LAST(type, name)               \
  DEFINE_VEC_POP(type, name)

inline void null_destructor(void* elem) {}
#define DEFINE_VEC2(type, name) DEFINE_VEC3(type, name, null_destructor)

#define VEC_FOREACH(type, elem, vec) \
  int __i = 0;                       \
  for (type elem = vec->data[0];     \
       __i < vec->size;              \
       __i++, elem = vec->data[__i])

#define VEC_FOREACH_PTR(type, elem, vec) \
  int __i = 0;                           \
  for (type *elem = &vec->data[0];       \
       __i < vec->size;                  \
       __i++, elem = &vec->data[__i])

