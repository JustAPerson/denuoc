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

#define DECLARE_VEC(name, type)                            \
  struct name##_vec {                                      \
    type *data;                                            \
    size_t size, capacity;                                 \
  };                                                       \
  typedef struct name##_vec name##_vec;                    \
  struct name##_vec name##_vec_new();                      \
  void name##_vec_push(struct name##_vec *vec, type elem); \
  void name##_vec_free(struct name##_vec *vec);


#define DEFINE_VEC_NEW(name, type)     \
  struct name##_vec name##_vec_new() { \
    struct name##_vec v = { 0, 0, 0};  \
    return v;                          \
  }

#define DEFINE_VEC_PUSH(name, type)                                        \
  void name##_vec_push(struct name##_vec *vec, type elem) {                \
    if (vec->size >=  vec->capacity) {                                     \
      if (vec->capacity == 0) {                                            \
        vec->capacity = 4;                                                 \
        vec->data = dcc_malloc(sizeof(type) * 4);                          \
      } else {                                                             \
        vec->capacity *= 2;                                                \
        vec->data = dcc_realloc(vec->data, sizeof(type) * vec->capacity);  \
      }                                                                    \
    }                                                                      \
    vec->data[vec->size++] = elem;                                         \
  }

#define DEFINE_VEC_FREE(name, type)              \
  void name##_vec_free(struct name##_vec *vec) { \
    free(vec->data);                             \
  }

#define DEFINE_VEC(name, type) \
  DEFINE_VEC_NEW(name, type)   \
  DEFINE_VEC_PUSH(name, type)  \
  DEFINE_VEC_FREE(name, type)

