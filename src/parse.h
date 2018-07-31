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

#include "dcc.h"
#include "vec.h"
#include "tokenize.h"



#define DECLARE_STRING_GETTER(name)                 \
  const char* name##_tag_str(enum name##_tag tag);

typedef enum storage_spec_tag {
  AST_STORAGE_NONE, // avoid returning a ptr
  AST_STORAGE_TYPEDEF,
  AST_STORAGE_EXTERN,
  AST_STORAGE_STATIC,
  AST_STORAGE_AUTO,
  AST_STORAGE_REGISTER,
} storage_spec_t;
DECLARE_STRING_GETTER(storage_spec);

typedef struct {
  enum type_spec_tag {
    AST_TYPE_VOID,
  } tag;
  union {};
} type_spec_t;
DECLARE_STRING_GETTER(type_spec);

typedef enum type_qual_tag {
  AST_TYPE_QUAL_CONST,
  AST_TYPE_QUAL_RESTRICT,
  AST_TYPE_QUAL_VOLATILE,
} type_qual_t;
DECLARE_VEC(type_qual_t, qual_vec);
DECLARE_VEC(type_qual_t, pointer_vec);
DECLARE_STRING_GETTER(type_qual);

typedef struct {
  enum decl_spec_tag {
    AST_STORAGE_CLASS_SPECIFIER,
    AST_TYPE_SPECIFIER,
    AST_TYPE_QUALIFIER,
    AST_FUNCTION_SPECIFIER,
  } tag ;
  union {
    storage_spec_t storage;
    type_spec_t *type_spec;
    type_qual_t *type_qual;
  };
} decl_spec_t;
DECLARE_VEC(decl_spec_t, decl_spec_vec);
DECLARE_STRING_GETTER(decl_spec);

typedef struct decltor {
  pointer_vec_t pointer;
  enum decltor_tag {
    AST_DECLTOR_IDENT,
    AST_DECLTOR_IDENTLIST, // has inner
  } tag;
  union {
    token_t *ident;
    token_vec_t *idents;
  };
  struct decltor *inner;
} decltor_t;
DECLARE_STRING_GETTER(decltor);

typedef struct {
  decl_spec_vec_t *specifiers;
} decl_t;
DECLARE_VEC(decl_t, decl_vec);

typedef struct {
  /* enum s{ */
  /* } tag; */
  /* union {}; */
} statement_t;
typedef struct {
  enum block_item_tag {
    AST_STATEMENT,
    AST_DECLARATION,
  } tag;
  union {
    statement_t *statement;
    decl_t *declaration;
  };
} block_item_t;
DECLARE_VEC(block_item_t, block_item_vec);
DECLARE_STRING_GETTER(block_item);

typedef struct {
  decl_spec_vec_t *specifiers;
  decltor_t *declarator;
  decl_vec_t *declarations;
  block_item_vec_t *block_items;
} func_def_t;

typedef struct {
  enum external_decl_tag {
    AST_EXT_FUNCTION,
    AST_EXT_DECLARATION,
  } tag;
  union {
    func_def_t *function;
    decl_t *declaration;
  };
} external_decl_t;
DECLARE_VEC(external_decl_t*, external_decl_vec);
DECLARE_STRING_GETTER(external_decl);

external_decl_vec_t dcc_parse(token_vec_t *tokens);
