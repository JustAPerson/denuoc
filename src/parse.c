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
#include "parse.h"
#include "vec_types.h"


static void decl_free(decl_t *declaration) {
  decl_spec_vec_free(declaration->specifiers);
}
static void block_item_free(block_item_t *block_item) {
  
}
DEFINE_VEC2(decl_spec_t, decl_spec_vec);
DEFINE_VEC2(decl_t, decl_vec);
DEFINE_VEC2(block_item_t, block_item_vec);
DEFINE_VEC3(external_decl_t*, external_decl_vec, free);

typedef struct {
  token_vec_t tokens;
  int_vec_t stack;
} stream_t;

static void stream_push(stream_t *stream) {
  if (stream->stack.size == 0) {
    int_vec_push(&stream->stack, 0);
  } else {
    int curr = *int_vec_last(&stream->stack);
    int_vec_push(&stream->stack, curr);
  }
}

static void stream_pop(stream_t *stream) {
  int_vec_pop(&stream->stack);
}

static void stream_commit(stream_t *stream) {
  int now = int_vec_pop(&stream->stack);
  int *last = int_vec_last(&stream->stack);
  dcc_assert(last);
  *last = now;
}

static token_t* stream_peek(stream_t *stream) {
  int *curr = int_vec_last(&stream->stack);
  dcc_assert(curr);
  dcc_assert(*curr < stream->tokens.size);
  return &stream->tokens.data[*curr];
}

static void stream_next(stream_t *stream) {
  int *curr = int_vec_last(&stream->stack);
  dcc_assert(curr);
  (*curr) += 1;
}

static token_t* stream_expect(stream_t *stream, token_tag_t tag) {
  token_t *token = stream_peek(stream);
  if (token->tag == tag) {
    stream_next(stream);
    return token;
  } else {
    dcc_ice("expected token `%s` found `%s`\n",
            dcc_token_tag_str(tag),
            dcc_token_tag_str(token->tag));
    // unreachable
    return 0;
  }
}

static bool stream_is(stream_t *stream, token_tag_t tag) {
  return stream_peek(stream)->tag == tag;
}

static storage_spec_t parse_storage_spec(stream_t *stream) {
  token_tag_t tag = stream_peek(stream)->tag;

  /* if (tag == TOKEN_KEYWORD_) */
  return AST_STORAGE_NONE;
}

static type_spec_t* parse_type_spec(stream_t *stream) {
  type_spec_t type;

  token_tag_t tag = stream_peek(stream)->tag;
  if (tag == TOKEN_KEYWORD_VOID) {
    stream_next(stream);
    type.tag = AST_TYPE_VOID;
  } else {
    return 0;
  }

  type_spec_t *output = dcc_malloc(sizeof(type_spec_t));
  *output = type;

  dcc_log(LOG_TRACE, "parsed type (%s)\n", type_spec_tag_str(output->tag));
  return output;
}

static decl_spec_vec_t* parse_decl_specs(stream_t *stream) {
  decl_spec_vec_t specifiers = decl_spec_vec_new();

  while (true) {
    stream_push(stream);
    decl_spec_t spec;

    storage_spec_t storage = parse_storage_spec(stream);
    if (storage != AST_STORAGE_NONE) {
      spec.tag = AST_STORAGE_CLASS_SPECIFIER;
      spec.storage = storage;
      goto success;
    }

    type_spec_t *type = parse_type_spec(stream);
    if (type) {
      spec.tag = AST_TYPE_SPECIFIER;
      spec.type_spec = type;
      goto success;
    }

    stream_pop(stream);
    break;
  success:
    dcc_log(LOG_TRACE, "parsed decl_spec (%s)\n", decl_spec_tag_str(spec.tag));
    stream_commit(stream);
    decl_spec_vec_push(&specifiers, spec);
  }

  if (specifiers.size > 0) {
    decl_spec_vec_t *output = dcc_malloc(sizeof(decl_spec_vec_t));
    *output = specifiers;
    return output;
  } else {
    return 0;
  }
}

static decl_t* parse_decl(stream_t *stream) {
  stream_push(stream);

  return 0;
}

static token_vec_t* parse_ident_list(stream_t *stream) {
  token_t *token = stream_peek(stream);
  if (token->tag != TOKEN_IDENT) {
    return 0;
  }

  token_vec_t *idents = dcc_malloc(sizeof(token_vec_t));
  *idents = token_vec_new();
  token_vec_push(idents, *token);
  while (token = stream_peek(stream), token->tag == TOKEN_COMMA) {
    stream_next(stream); // skip comma
    token = stream_expect(stream, TOKEN_IDENT);
    token_vec_push(idents, *token);
  }
  dcc_log(LOG_TRACE, "parsed ident_list (%d idents)\n", idents->size);
  return idents;
}

static decltor_t* parse_decltor(stream_t *stream) {
  /* stream_push(stream); */

  token_t *token = stream_peek(stream);
  decltor_t *output;
  if (token->tag == TOKEN_IDENT) {
    stream_next(stream);
    output = dcc_malloc(sizeof(decltor_t));
    output->tag = AST_DECLTOR_IDENT;
    output->ident = token;
  } else if (token->tag == TOKEN_LPAREN) {
    dcc_nyi("LPAREN starting decltor");
  } else {
    return 0;
  }
  token = stream_peek(stream);

  if (output) {
    while (true) {
      dcc_log(LOG_TRACE, "parsed decltor (%s)\n", decltor_tag_str(output->tag));

      if (token->tag == TOKEN_LPAREN) {
        stream_next(stream);
        token_vec_t *idents = parse_ident_list(stream);

        stream_expect(stream, TOKEN_RPAREN);
        if (idents) {
          decltor_t *outter = dcc_malloc(sizeof(decltor_t));
          outter->tag = AST_DECLTOR_IDENTLIST;
          outter->idents = idents;
          outter->inner = output;
          output = outter;
        } else {
          // don't bother wrapping an empty AST_DECLTOR_IDENTLIST
          break;
        }
      } else if (output && token->tag == TOKEN_LSQUARE) {
        dcc_nyi("recursive LSQUARE decltor");
      } else {
        break;
      }
    }
  }
  /* stream_commit(stream); // TODO conditional commit/pop? */

  return output;
}


static block_item_vec_t* parse_compound_statement(stream_t *stream);
static statement_t* parse_statement(stream_t *stream) {
  return 0;
}
static block_item_t* parse_block_item(stream_t *stream) {
  return 0;
}
static block_item_vec_t* parse_compound_statement(stream_t *stream) {
  if (!stream_is(stream, TOKEN_LCURLY)) {
    return 0;
  }
  stream_next(stream); // consume LCURLY
  block_item_vec_t *items = dcc_malloc(sizeof(block_item_vec_t));
  *items = block_item_vec_new();
  while (true) {
    block_item_t* item = parse_block_item(stream);
    if (!item) {
      break;
    }
    block_item_vec_push(items, *item);
  }
  stream_expect(stream, TOKEN_RCURLY); // consume RCURLY
  dcc_log(LOG_TRACE, "parsed compound_statement (%d items)\n", items->size);
  return items;
}

static func_def_t* parse_func_def(stream_t *stream) {
  stream_push(stream);

  decl_spec_vec_t *specs = parse_decl_specs(stream);
  if (!specs) {
    goto error;
  }

  decltor_t *decltor = parse_decltor(stream);
  if (!decltor) {
    goto error;
  }

  // TODO declaration-list

  if (!stream_is(stream, TOKEN_LCURLY)) {
    goto error;
  }
  // TODO Why can we only now commit to func_def
  block_item_vec_t *block_items = parse_compound_statement(stream);
  if (!block_items) {
    goto error;
  }

  func_def_t *output = dcc_malloc(sizeof(func_def_t));
  output->specifiers = specs;
  output->declarator = decltor;
  output->block_items = block_items;

  stream_commit(stream);
  dcc_log(LOG_TRACE, "parsed function_def\n");
  return output;
 error:
  if (specs) { free(specs); }
  if (decltor) { free(decltor); }

  stream_pop(stream);
  return 0;
}

static external_decl_t* parse_external_decl(stream_t *stream) {
  external_decl_t parsed;
  func_def_t *function = parse_func_def(stream);
  if (function) {
    parsed.tag = AST_EXT_FUNCTION;
    parsed.function = function;
  } else {
    decl_t *declaration = parse_decl(stream);
    if (!declaration) {
      return 0;
    }
    parsed.tag = AST_EXT_DECLARATION;
    parsed.declaration = declaration;
  }

  external_decl_t *output = dcc_malloc(sizeof(external_decl_t));
  *output = parsed;
  dcc_log(LOG_TRACE, "parsed external_decl (%s)\n",
          external_decl_tag_str(output->tag));
  return output;
}

external_decl_vec_t dcc_parse(token_vec_t *tokens) {
  stream_t stream = {
    .tokens = *tokens, // TODO figure lifetime of tokens
    .stack = int_vec_new(),
  };
  stream_push(&stream);

  external_decl_vec_t output = external_decl_vec_new();
  external_decl_t *ext_decl;
  while (true) {
    ext_decl = parse_external_decl(&stream);
    if (!ext_decl) {
      break;
    }
    external_decl_vec_push(&output, ext_decl);
  }
  dcc_assert(stream_peek(&stream)->tag == TOKEN_EOF);

  return output;
}

// ignore warning about tautological test in macro below
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#endif
#define STRING_GETTER(name, type, strings)          \
  const char* name##_tag_str(type tag) {            \
    if (tag >= (sizeof(strings) / sizeof(char*))) { \
      dcc_ice("invalid " #name " tag %d\n", tag);   \
    }                                               \
    return strings[tag];                            \
  }

static const char *STORAGE_SPEC_STRINGS[] = {
  "none", "typedef", "extern", "static", "auto", "register"
};
static const char *TYPE_SPEC_STRINGS[]  = {
  "void",
};
static const char *TYPE_QUAL_STRINGS[] = {"const", "restrict", "volatile"};
static const char *DECL_SPEC_STRINGS[] = {
  "storage class specifier", "type specifier", "type qualifier",
  "function specifier",
};
static const char *DECLTOR_STRINGS[] = {"ident", "identlist", };
static const char *BLOCK_ITEM_STRINGS[] = {"statement", "declaration", };
static const char *EXTERNAL_DECL_STRINGS[] = {"function definition", "declaration", };

STRING_GETTER(storage_spec, enum storage_spec_tag, STORAGE_SPEC_STRINGS);
STRING_GETTER(type_spec, enum type_spec_tag, TYPE_SPEC_STRINGS);
STRING_GETTER(type_qual, enum type_qual_tag, TYPE_QUAL_STRINGS);
STRING_GETTER(decl_spec, enum decl_spec_tag, DECL_SPEC_STRINGS);
STRING_GETTER(decltor, enum decltor_tag, DECLTOR_STRINGS);
STRING_GETTER(block_item, enum block_item_tag, BLOCK_ITEM_STRINGS);
STRING_GETTER(external_decl, enum external_decl_tag, EXTERNAL_DECL_STRINGS);
