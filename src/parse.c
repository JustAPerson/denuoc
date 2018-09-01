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

#include "dcc.h"
#include "parse.h"
#include "tokenize.h"
#include "vec_types.h"
#include "util.h"


// TODO FIXME destructors
DEFINE_VEC2(init_decltor_t*, init_decltor_vec);
DEFINE_VEC2(decl_t*, decl_vec);
DEFINE_VEC2(block_item_t, block_item_vec);
DEFINE_VEC3(external_decl_t*, external_decl_vec, free);
DEFINE_VEC2(exp_t*, exp_vec);
DEFINE_VEC2(struct_decl_t, struct_decl_vec);
DEFINE_VEC2(struct_decltor_t, struct_decltor_vec);
DEFINE_VEC2(direct_decltor_t, direct_decltor_vec);
DEFINE_VEC2(param_decl_t*, param_decl_vec);
DEFINE_VEC2(type_qual_t, type_qual_vec);
DEFINE_VEC2(designator_t*, designator_vec);
DEFINE_VEC2(initialization_t, initialization_vec);

typedef struct {
  enum token_tag token;
  enum exp_tag exp;
} token_exp_tag_pair;

////////////////////////////////////////////////////////////////////////////////
// Stream operations
////////////////////////////////////////////////////////////////////////////////

// A stream owns the vector of tokens and a stack representing the parser's
// current position therein.
typedef struct {
  token_vec_t tokens;
  int_vec_t stack;
} stream_t;

// Begin attempting to parse a new feature, so record the previous location such
// that we can revert if necessary
static void stream_push(stream_t *stream) {
  if (stream->stack.size == 0) {
    int_vec_push(&stream->stack, 0);
  } else {
    int curr = *int_vec_last(&stream->stack);
    int_vec_push(&stream->stack, curr);
  }
}

// Forget the current stream position after failing to parse a feature
static void stream_pop(stream_t *stream) {
  int_vec_pop(&stream->stack);
}

// Commit this stream position in the case parsing succeeds
static void stream_commit(stream_t *stream) {
  int now = int_vec_pop(&stream->stack);
  int *last = int_vec_last(&stream->stack);
  dcc_assert(last);
  *last = now;
}

// Return ptr to next token
static token_t* stream_peek(stream_t *stream) {
  int *curr = int_vec_last(&stream->stack);
  dcc_assert(curr);
  dcc_assert(*curr < stream->tokens.size);
  return &stream->tokens.data[*curr];
}

// Advance to next token
static void stream_next(stream_t *stream) {
  int *curr = int_vec_last(&stream->stack);
  dcc_assert(curr);
  (*curr) += 1;
}

// Assert that the next token is of a specific type
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

// Print consistent error message
static void stream_expected(stream_t *stream, char* phrase) {
  dcc_ice("expected token `%s` found `%s`\n",
          phrase,
          dcc_token_tag_str(stream_peek(stream)->tag));
}

// Query
static bool stream_is(stream_t *stream, token_tag_t tag) {
  return stream_peek(stream)->tag == tag;
}

#define STREAM_ACTION(result, ...)                   \
  TRACE("");                                         \
  for (int i = 0; i < stream->stack.size - 1; ++i) { \
    eprintf("  ");                                   \
  }                                                  \
  eprintf("%s %s ", result, __func__);               \
  eprintf(__VA_ARGS__);                              \
  eprintf("\n");

/* #define STREAM_PUSH() STREAM_ACTION("attempt"); stream_push(stream); */
/* #define STREAM_COMMIT() stream_commit(stream); STREAM_ACTION("commit"); */
/* #define STREAM_POP() STREAM_ACTION("abort"); stream_pop(stream); */

#define STREAM_PUSH() STREAM_ACTION("attempt", ""); stream_push(stream);
#define STREAM_COMMIT() stream_commit(stream); STREAM_ACTION("commit", "");
#define STREAM_POP() stream_pop(stream); STREAM_ACTION("abort", "");

#define STREAM_PUSHa(...) STREAM_ACTION("attempt", __VA_ARGS__); stream_push(stream);
#define STREAM_COMMITa(...) stream_commit(stream); STREAM_ACTION("commit", __VA_ARGS__);
#define STREAM_POPa(...) stream_pop(stream); STREAM_ACTION("abort", __VA_ARGS__);

////////////////////////////////////////////////////////////////////////////////
// stdspec.6.4 Constants
////////////////////////////////////////////////////////////////////////////////

static constant_t* parse_constant(stream_t *stream) {
  stream_push(stream);

  constant_t constant;
  if (stream_is(stream, TOKEN_INTEGER)) {
    constant.tag = CONSTANT_INTEGER;
    constant.integer = stream_peek(stream)->val.integer;
  } else if (stream_is(stream, TOKEN_FLOATING)) {
    constant.tag = CONSTANT_FLOAT;
    constant.floating = stream_peek(stream)->val.floating;
    stream_next(stream);
  } else {
    // TODO FIXME char/enum constants
    // enum constants will probably have to be inferred out of just variable
    // names in another pass
    stream_pop(stream);
    return 0;
  }

  stream_commit(stream);
  constant_t *output = dcc_malloc(sizeof(constant_t));
  *output = constant;
  return output;
}

////////////////////////////////////////////////////////////////////////////////
// stdspec.6.5 Expressions
////////////////////////////////////////////////////////////////////////////////

static exp_t* parse_exp(stream_t *stream);
static exp_t* parse_assignment_exp(stream_t *stream);
static exp_t* parse_cast_exp(stream_t *stream);
static type_name_t* parse_type_name(stream_t *stream);
static initialization_vec_t* parse_initialization_list(stream_t *stream);

// stdspec.6.5.1
static exp_t* parse_primary_exp(stream_t *stream) {
  STREAM_PUSH();

  exp_t *exp = dcc_malloc(sizeof(exp_t));
  if (stream_is(stream, TOKEN_IDENT)) {
    exp->tag = EXP_IDENT;
    exp->token = stream_peek(stream);
    stream_next(stream);
  } else if (stream_is(stream, TOKEN_STRING)) {
    exp->tag = EXP_STRING;
    exp->token = stream_peek(stream);
    stream_next(stream);
  } else if (stream_is(stream, TOKEN_LPAREN)) {
    stream_next(stream);
    exp = parse_exp(stream);
    if (!exp) {
      stream_expected(stream, "expression");
    }
    stream_expect(stream, TOKEN_RPAREN);
  } else {
    constant_t *constant = parse_constant(stream);
    if (constant) {
      exp->tag = EXP_CONSTANT;
      exp->constant = constant;
    } else {
      free(exp);
      STREAM_POP();
      return 0;
    }
  }

  STREAM_COMMIT();
  return exp;
}

// stdspec.6.5.2.2
static exp_vec_t* parse_argument_exp_list(stream_t *stream) {
  STREAM_PUSH();
  exp_vec_t args = exp_vec_new();

  while (true) {
    if (args.size) {
      // if there are prior arguments, they should be separated by commas
      stream_expect(stream, TOKEN_COMMA);
    }

    exp_t *arg = parse_assignment_exp(stream);
    if (!arg) {
      break;
    }

    exp_vec_push(&args, arg);
  }

  exp_vec_t *output = dcc_malloc(sizeof args);
  *output = args;
  STREAM_COMMIT();
  return output;
}

// stdspec.6.5.2.1
static exp_t* parse_postfix_exp(stream_t *stream) {
  STREAM_PUSH();
  exp_t *previous = parse_primary_exp(stream);
  if (!previous && stream_is(stream, TOKEN_LPAREN)) {
    stream_next(stream);
    type_name_t *tname = parse_type_name(stream);
    stream_expect(stream, TOKEN_RPAREN);

    initialization_vec_t *inits = parse_initialization_list(stream);
    if (!inits) {
      dcc_ice("expected initializer-list after `)`");
    }

    previous = dcc_malloc(sizeof *previous);
    previous->tag = EXP_STRUCT;
    previous->struct_init.tname = tname;
    previous->struct_init.inits = inits;
  }

  if (!previous) {
    STREAM_POP();
    return 0;
  }


  while (true) {
    exp_t *exp = dcc_malloc(sizeof(exp_t));
    if (stream_is(stream, TOKEN_LSQUARE)) {
      stream_next(stream);
      exp_t *index = parse_exp(stream);
      if (!index) {
        stream_expected(stream, "expression for index after `[`");
      }
      stream_expect(stream, TOKEN_RSQUARE);
      exp->tag = EXP_INDEX;
      exp->binary.lhs = previous;
      exp->binary.rhs = index;
    } else if (stream_is(stream, TOKEN_LPAREN)) {
      stream_next(stream);
      exp->tag = EXP_CALL;
      exp->call.lhs = previous;
      exp->call.args = parse_argument_exp_list(stream);
      stream_expect(stream, TOKEN_RPAREN);
    } else if (stream_is(stream, TOKEN_DOT)) {
      stream_next(stream);
      exp->tag = EXP_DOT;
      exp->child.lhs = previous;
      exp->child.name = stream_peek(stream);
      stream_expect(stream, TOKEN_IDENT);
    } else if (stream_is(stream, TOKEN_ARROW)) {
      stream_next(stream);
      exp->tag = EXP_ARROW;
      exp->child.lhs = previous;
      exp->child.name = stream_peek(stream);
      stream_expect(stream, TOKEN_IDENT);
    } else if (stream_is(stream, TOKEN_INCREMENT)) {
      stream_next(stream);
      exp->tag = EXP_POSTINCREMENT;
      exp->unary = previous;
    } else if (stream_is(stream, TOKEN_DECREMENT)) {
      stream_next(stream);
      exp->tag = EXP_POSTDECREMENT;
      exp->unary = previous;
    } else {
      stream_pop(stream);
      free(exp);
      break;
    }
    previous = exp;
  }

  STREAM_COMMIT();
  return previous;
}

// stdspec.6.5.3
static exp_t* parse_unary_exp(stream_t *stream) {
  exp_t *postfix = parse_postfix_exp(stream);
  if (postfix) {
    return postfix;
  }

  token_exp_tag_pair UNARY_PAIRS[] = {
    {TOKEN_INCREMENT, EXP_PREINCREMENT},
    {TOKEN_DECREMENT, EXP_PREDECREMENT},
    {TOKEN_KEYWORD_SIZEOF, EXP_SIZEOFEXP},
    {0, 0},
  };

  STREAM_PUSH();
  for (token_exp_tag_pair *pair = UNARY_PAIRS; pair->token; ++pair) {
    if (stream_peek(stream)->tag == pair->token) {
      stream_next(stream);
      exp_t *unary = parse_unary_exp(stream);
      if (!unary) {
        stream_expected(stream, "unary expression");
        // Should we instead backtrack here? See below
      }
      exp_t *output = dcc_malloc(sizeof(exp_t));
      output->tag = pair->exp;
      output->unary = unary;

      STREAM_COMMIT();
      return output;
    }
  }

  token_exp_tag_pair CAST_PAIRS[] = {
    {TOKEN_AMP, EXP_ADDRESSOF},
    {TOKEN_STAR, EXP_DEREFERENCE},
    {TOKEN_PLUS, 0},
    {TOKEN_MINUS, EXP_NEGATE},
    {TOKEN_SQUIGGLE, EXP_BITNOT},
    {TOKEN_EXCLAIM, EXP_LOGICNOT},
    {0, 0},
  };

  for (token_exp_tag_pair *pair = CAST_PAIRS; pair->token; ++pair) {
    if (stream_peek(stream)->tag == pair->token) {
      stream_next(stream);
      exp_t *cast = parse_cast_exp(stream);
      if (!cast) {
        stream_expected(stream, "cast/unary expression");
        // Should we instead backtrack here? See above
      }
      exp_t *output;
      if (pair->exp) { // anything but TOKEN_PLUS
        output = dcc_malloc(sizeof(exp_t));
        output->tag = pair->exp;
        output->unary = cast;
      } else {
        // TOKEN_PLUS is a no-op
        output = cast;
      }
      STREAM_COMMIT();
      return output;
    }
  }

  STREAM_POP();
  stream_expected(stream, "unary operator");
  // unreachable
  return 0;
}

// stdspec.6.5.4
static exp_t* parse_cast_exp(stream_t *stream) {
  // TODO FIXME cast expression
  return parse_unary_exp(stream);
}

#define RECURSIVE_BINOP(name, inner, pairs)                       \
static exp_t* name(stream_t *stream) {                            \
  STREAM_PUSH();                                                  \
                                                                  \
  exp_t *lhs = inner(stream);                                     \
  if (!lhs) {                                                     \
    STREAM_POP();                                                 \
    return 0;                                                     \
  }                                                               \
                                                                  \
  while (true) {                                                  \
    for (token_exp_tag_pair *pair = pairs; pair->token; pair++) { \
      if (!stream_is(stream, pair->token)) { continue; }          \
      stream_next(stream);                                        \
                                                                  \
      exp_t *rhs = inner(stream);                                 \
      if (!rhs) {                                                 \
        stream_expected(stream, "expression");                    \
      }                                                           \
                                                                  \
      exp_t *output = dcc_malloc(sizeof(exp_t));                  \
      output->tag = pair->exp;                                    \
      output->binary.lhs = lhs;                                   \
      output->binary.rhs = rhs;                                   \
      lhs = output;                                               \
      goto success;                                               \
    }                                                             \
    break;                                                        \
success:                                                          \
    continue;                                                     \
  }                                                               \
                                                                  \
  STREAM_COMMIT();                                                \
  return lhs;                                                     \
}

// stdspec.6.5.5
token_exp_tag_pair MULTIPLICATIVE_EXPS[] = {
  {TOKEN_STAR, EXP_MULTIPLY},
  {TOKEN_FORWARD, EXP_DIVIDE},
  {TOKEN_PERCENT, EXP_MODULO},
  {0, 0},
};
// stdspec.6.5.6
token_exp_tag_pair ADDITIVE_EXPS[] = {
  {TOKEN_PLUS, EXP_ADD},
  {TOKEN_MINUS, EXP_SUBTRACT},
  {0, 0},
};
// stdspec.6.5.7
token_exp_tag_pair SHIFT_EXPS[] = {
  {TOKEN_LEFT, EXP_SHIFTLEFT},
  {TOKEN_RIGHT, EXP_SHIFTRIGHT},
  {0, 0},
};
// stdspec.6.5.8
token_exp_tag_pair RELATIONAL_EXPS[] = {
  {TOKEN_LESS, EXP_LESS},
  {TOKEN_MORE, EXP_MORE},
  {TOKEN_LESSEQ, EXP_LESSEQ},
  {TOKEN_MOREEQ, EXP_MOREEQ},
  {0, 0},
};
// stdspec.6.5.9
token_exp_tag_pair EQUALITY_EXPS[] = {
  {TOKEN_EQEQ, EXP_EQUAL},
  {TOKEN_NOTEQ, EXP_NOTEQUAL},
  {0, 0},
};
// stdspec.6.5.10
token_exp_tag_pair AND_EXPS[] = {
  {TOKEN_AMP, EXP_BITAND},
  {0, 0},
};
// stdspec.6.5.11
token_exp_tag_pair EXCLUSIVE_OR_EXPS[] = {
  {TOKEN_CARET, EXP_BITXOR},
  {0, 0},
};
// stdspec.6.5.12
token_exp_tag_pair INCLUSIVE_OR_EXPS[] = {
  {TOKEN_PIPE, EXP_BITOR},
  {0, 0},
};
// stdspec.6.5.13
token_exp_tag_pair LOGICAL_AND_EXPS[] = {
  {TOKEN_AMPAMP, EXP_LOGICAND},
  {0, 0},
};
// stdspec.6.5.14
token_exp_tag_pair LOGICAL_OR_EXPS[] = {
  {TOKEN_PIPEPIPE, EXP_LOGICOR},
  {0, 0},
};

RECURSIVE_BINOP(parse_multiplicative, parse_cast_exp, MULTIPLICATIVE_EXPS);
RECURSIVE_BINOP(parse_additive_exp, parse_multiplicative, ADDITIVE_EXPS);
RECURSIVE_BINOP(parse_shift_exp, parse_additive_exp, SHIFT_EXPS);
RECURSIVE_BINOP(parse_relational_exp, parse_shift_exp, RELATIONAL_EXPS);
RECURSIVE_BINOP(parse_equality_exp, parse_relational_exp, EQUALITY_EXPS);
RECURSIVE_BINOP(parse_and_exp, parse_equality_exp, AND_EXPS);
RECURSIVE_BINOP(parse_exclusive_or_exp, parse_and_exp, EXCLUSIVE_OR_EXPS);
RECURSIVE_BINOP(parse_inclusive_or_exp, parse_exclusive_or_exp, INCLUSIVE_OR_EXPS);
RECURSIVE_BINOP(parse_logical_and_exp, parse_inclusive_or_exp, LOGICAL_AND_EXPS);
RECURSIVE_BINOP(parse_logical_or_exp, parse_logical_and_exp, LOGICAL_OR_EXPS);

// stdspec.6.5.15
static exp_t* parse_cond_exp(stream_t *stream) {
  STREAM_PUSH();
  stream_push(stream);

  exp_t *cond = parse_logical_or_exp(stream);
  if (!cond) {
    stream_pop(stream);
    STREAM_POP();
    return 0;
  }

  exp_t *output = cond;
  if (stream_is(stream, TOKEN_QUEST)) {
    stream_next(stream);

    exp_t *true_exp = parse_exp(stream);
    if (!true_exp) {
      stream_expected(stream, "expression");
    }
    stream_expect(stream, TOKEN_COLON);

    exp_t *false_exp = parse_cond_exp(stream);
    if (!false_exp) {
      dcc_ice("expected expression found %s\n",
              dcc_token_tag_str(stream_peek(stream)->tag));
    }

    output = dcc_malloc(sizeof(exp_t));
    output->tag = EXP_TERNARY;
    output->ternary.cond = cond;
    output->ternary.true_exp = true_exp;
    output->ternary.false_exp = false_exp;
  }

  stream_commit(stream);
  STREAM_COMMIT();
  return output;
}

// stdspec.6.5.16
static exp_t* parse_assignment_exp(stream_t *stream) {
  STREAM_PUSH();
  stream_push(stream);

  static token_exp_tag_pair ASSIGNMENTS[] = {
    { TOKEN_EQUAL, EXP_EQUAL },
    { TOKEN_STAREQ, EXP_MULTIPLY },
    { TOKEN_FORWARDEQ, EXP_DIVIDE },
    { TOKEN_PERCENTEQ, EXP_MODULO },
    { TOKEN_PLUSEQ, EXP_ADD },
    { TOKEN_MINUSEQ, EXP_SUBTRACT },
    { TOKEN_LEFTEQ, EXP_SHIFTLEFT },
    { TOKEN_RIGHTEQ, EXP_SHIFTRIGHT },
    { TOKEN_AMPEQ, EXP_BITAND },
    { TOKEN_CARETEQ, EXP_BITXOR },
    { TOKEN_PIPEEQ, EXP_BITOR },
    {0, 0},
  };

  exp_t *unary = parse_unary_exp(stream);
  for (token_exp_tag_pair *assignment = ASSIGNMENTS; assignment->token; assignment++) {
    if (stream_is(stream, assignment->token)) {
      stream_next(stream);
      exp_t *rhs = parse_assignment_exp(stream);
      if (!rhs) {
        stream_expected(stream, "expression after assignment"); // TODO which assignment
      }
      stream_commit(stream);
      exp_t *output = dcc_malloc(sizeof(exp_t));
      output->assignment.lhs = unary;
      output->assignment.rhs = rhs;
      output->assignment.operator = assignment->exp;
      STREAM_COMMIT();
      return output;
    }
  }
  stream_pop(stream);
  STREAM_POP();

  return parse_cond_exp(stream);
}

static exp_t* parse_exp(stream_t *stream) {
  exp_t *exp = parse_assignment_exp(stream);
  if (!exp) {
    return 0;
  }

  if (stream_is(stream, TOKEN_COMMA)) {
    exp_t *output = dcc_malloc(sizeof(exp_t));
    output->tag = EXP_LIST;
    output->list = exp_vec_new();
    do {
      stream_next(stream); // consume comma
      exp_vec_push(&output->list, exp);
      exp = parse_assignment_exp(stream);
      if (!exp) {
        stream_expected(stream, "expression after `,`");
      }
    } while (stream_is(stream, TOKEN_COMMA));
    STREAM_COMMIT();
    return output;
  } else {
    return exp;
  }
}

////////////////////////////////////////////////////////////////////////////////
// stdspec.6.7 Declarations
////////////////////////////////////////////////////////////////////////////////

static decltor_t* parse_decltor(stream_t *stream);
static storage_spec_t parse_storage_spec(stream_t *stream);
static type_spec_t* parse_type_spec(stream_t *stream);
static type_qual_t parse_type_qual(stream_t *stream);
static func_spec_t parse_func_spec(stream_t *stream);
static initializer_t* parse_initializer(stream_t *stream);

static init_decltor_t* parse_init_decltor(stream_t *stream) {
  STREAM_PUSH();

  decltor_t *declarator = parse_decltor(stream);
  if (!declarator) {
    STREAM_POP();
    return 0;
  }

  initializer_t *initializer;
  if (stream_is(stream, TOKEN_EQUAL)) {
    stream_next(stream);

    initializer = parse_initializer(stream);
    if (!initializer) {
      stream_expected(stream, "initializer after `=`");
      // unreachable
      return 0;
    }
  }

  init_decltor_t *output = dcc_malloc(sizeof(init_decltor_t));
  output->declarator = declarator;
  output->initializer = initializer;

  STREAM_COMMITa("parsed init_decltor (%s initializer)",
                initializer ? "with" : "without");
  return output;
}

static init_decltor_vec_t parse_init_decltor_list(stream_t *stream) {
  STREAM_PUSH();

  init_decltor_vec_t vec = init_decltor_vec_new();

  init_decltor_t *init = parse_init_decltor(stream);
  if (!init) {
    goto exit;
  }

  init_decltor_vec_push(&vec, init);
  while(stream_is(stream, TOKEN_COMMA)) {
    stream_next(stream);
    init = parse_init_decltor(stream);
    if (!init) {
      stream_expected(stream, "(initialized) declarator after `,`");
    }
    init_decltor_vec_push(&vec, init);
  }

 exit:
  STREAM_COMMITa("got 0 init_decltors");
  return vec;
}

static decl_spec_t* parse_decl_specs(stream_t *stream) {

  decl_spec_t decl_spec = {0, 0, 0, 0};
  bool succeeded = false;
  while (true) {
    STREAM_PUSH();

    storage_spec_t storage = parse_storage_spec(stream);
    if (storage != AST_STORAGE_NONE) {
      if (storage & decl_spec.storage) {
        dcc_ice("cannot repeat storage specifier");
      }
      decl_spec.storage |= storage;
      goto success;
    }

    type_spec_t *tspec = parse_type_spec(stream);
    if (tspec) {
      decl_spec.type_spec = tspec;
      goto success;
    }

    type_qual_t tqual = parse_type_qual(stream);
    if (tqual != TYPE_QUAL_NONE) {
      if (tqual & decl_spec.type_qual) {
        dcc_ice("cannot repeat type qualifier");
      }
      decl_spec.type_qual |= tqual;
      goto success;
    }

    func_spec_t fspec = parse_func_spec(stream);
    if (fspec) {
      if (fspec & decl_spec.func_spec) {
        dcc_ice("cannot repeat function specifier");
      }
      decl_spec.func_spec |= fspec;
      goto success;
    }

    STREAM_POP();
    break;
  success:
    STREAM_COMMIT();
    succeeded = true;
  }

  if (succeeded) {
    decl_spec_t *output = dcc_malloc(sizeof *output);
    *output = decl_spec;
    return output;
  } else {
    return 0;
  }
}


static decl_t* parse_decl(stream_t *stream) {
  STREAM_PUSH();

  decl_spec_t *specifiers = parse_decl_specs(stream);
  if (!specifiers) {
    STREAM_POP();
    return 0;
  }

  init_decltor_vec_t init_decltors = parse_init_decltor_list(stream);
  stream_expect(stream, TOKEN_SEMI);

  decl_t *output = dcc_malloc(sizeof *output);
  output->specifiers = specifiers;
  output->init_decltors = init_decltors;
  STREAM_COMMIT();
  return output;
}

////////////////////////////////////////////////////////////////////////////////
// stdspec.6.7.1 Storage-class specifiers
////////////////////////////////////////////////////////////////////////////////

static storage_spec_t parse_storage_spec(stream_t *stream) {
  static struct pair {
    token_tag_t token;
    storage_spec_t storage;
  } PAIRS[] = {
    { TOKEN_KEYWORD_TYPEDEF, AST_STORAGE_TYPEDEF },
    { TOKEN_KEYWORD_EXTERN, AST_STORAGE_EXTERN },
    { TOKEN_KEYWORD_STATIC, AST_STORAGE_STATIC },
    { TOKEN_KEYWORD_AUTO, AST_STORAGE_AUTO },
    { TOKEN_KEYWORD_REGISTER, AST_STORAGE_REGISTER },
    { 0, 0 }
  };
  token_tag_t tag = stream_peek(stream)->tag;
  for (struct pair *pair = PAIRS; pair->token; ++pair) {
    if (pair->token == tag) {
      return pair->storage;
    }
  }
  return AST_STORAGE_NONE;
}

////////////////////////////////////////////////////////////////////////////////
// stdspec.6.7.2 Type specifiers
////////////////////////////////////////////////////////////////////////////////

static type_squal_t* parse_type_squal(stream_t *stream) {
  bool success = false;

  type_squal_t squal = { 0, 0 };
  while (true) {
    type_spec_t *tspec = parse_type_spec(stream);
    if (tspec) {
      if (squal.spec) {
        dcc_ice("struct decl must included only one specifier");
      }
      squal.spec = tspec;
      success = true;
      continue;
    }

    type_qual_t tqual = parse_type_qual(stream);
    if (tqual) {
      squal.qual |= tqual; // TODO does stdspec allow ignoring duplicate typequals
      success = true;
      continue;
    }

    break;
  }
  if (success && !squal.spec) {
    dcc_ice("struct decl must include a type specifier");
  }

  if (success) {
    type_squal_t *output = dcc_malloc(sizeof *output);
    *output = squal;
    return output;
  } else {
    return 0;
  }
}

static void parse_struct_decls(stream_t *stream, struct_decl_vec_t *sdecls) {
  if (stream_is(stream, TOKEN_RCURLY)) {
    return; // now rest of function can assume actually trying to parse a decl
  }

  while (true) {
    STREAM_PUSH();

    type_squal_t *squal = parse_type_squal(stream);
    if (!squal) {
      STREAM_POP();
      break;
    }

    struct_decltor_vec_t sdecltors = struct_decltor_vec_new();
    while(true) {
      if (sdecltors.size > 0 && !stream_is(stream, TOKEN_COMMA)) {
        // break if no comma after previous sdecltor
        break;
      }
      decltor_t *decltor = parse_decltor(stream);
      if (!decltor) {
        break;
      }

      exp_t *exp = 0;
      if (stream_is(stream, TOKEN_COLON)) {
        stream_next(stream);
        exp = parse_exp(stream);
        // TODO FIXME only parse constant expressions here, for bitfields
      }

      struct_decltor_t sdecltor = {
        .decltor = decltor,
        .exp = exp,
      };
      struct_decltor_vec_push(&sdecltors, sdecltor);
    }
    if (sdecltors.size == 0) {
      dcc_ice("struct field must have at least one declarator");
    }

    STREAM_COMMIT();
    stream_expect(stream, TOKEN_SEMI);
    struct_decl_t sdecl = {
      .squal = squal,
      .sdecltors = sdecltors,
    };
    struct_decl_vec_push(sdecls, sdecl);
  }
}

static sunion_spec_t* parse_sunion_spec(stream_t *stream) {
  STREAM_PUSH();

  token_tag_t tag = stream_peek(stream)->tag;
  if (tag != TOKEN_KEYWORD_STRUCT && tag != TOKEN_KEYWORD_UNION) {
    STREAM_POP();
    return 0;
  }
  stream_next(stream);

  token_t *ident = 0;
  if (stream_is(stream, TOKEN_IDENT)) {
    ident = stream_peek(stream);
    stream_next(stream);
  }

  struct_decl_vec_t decls = struct_decl_vec_new();
  if (stream_is(stream, TOKEN_LCURLY)) {
    stream_next(stream);
    parse_struct_decls(stream, &decls);
    stream_expect(stream, TOKEN_RCURLY);
  }

  sunion_spec_t *output = dcc_malloc(sizeof *output);
  output->ident = ident;
  output->decls = decls;
  STREAM_COMMIT();
  return output;
}

static type_spec_t* parse_type_spec(stream_t *stream) {
  static struct pair {
    token_tag_t token;
    enum type_spec_tag type;
  } PAIRS[] = {
    { TOKEN_KEYWORD_VOID, AST_TYPE_VOID },
    { TOKEN_KEYWORD_CHAR, AST_TYPE_CHAR },
    { TOKEN_KEYWORD_SHORT, AST_TYPE_SHORT },
    { TOKEN_KEYWORD_INT, AST_TYPE_INT },
    { TOKEN_KEYWORD_LONG, AST_TYPE_LONG },
    { TOKEN_KEYWORD_DOUBLE, AST_TYPE_DOUBLE },
    { TOKEN_KEYWORD_SIGNED, AST_TYPE_SIGNED },
    { TOKEN_KEYWORD_UNSIGNED, AST_TYPE_UNSIGNED },
    { TOKEN_KEYWORD__BOOL, AST_TYPE__BOOL },
    { TOKEN_KEYWORD__COMPLEX, AST_TYPE__COMPLEX },
    { 0, 0 }
  };

  token_tag_t tag = stream_peek(stream)->tag;
  for (struct pair *pair = PAIRS; pair->token; ++pair) {
    if (pair->token == tag) {
      type_spec_t *output = dcc_malloc(sizeof *output);
      stream_next(stream);
      output->tag = pair->type;
      return output;
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// stdspec.6.7.3 Type qualifiers
////////////////////////////////////////////////////////////////////////////////

static type_qual_t parse_type_qual(stream_t *stream) {
  token_tag_t tag = stream_peek(stream)->tag;
  type_qual_t output = TYPE_QUAL_NONE;

  if (tag == TOKEN_KEYWORD_CONST) {
    output = TYPE_QUAL_CONST;
  } else if (tag == TOKEN_KEYWORD_RESTRICT) {
    output = TYPE_QUAL_RESTRICT;
  } else if (tag == TOKEN_KEYWORD_VOLATILE) {
    output = TYPE_QUAL_VOLATILE;
  }

  if (output != TYPE_QUAL_NONE) {
    stream_next(stream);
  }

  return output;
}


////////////////////////////////////////////////////////////////////////////////
// stdspec.6.7.4 Function specifiers
////////////////////////////////////////////////////////////////////////////////

static func_spec_t parse_func_spec(stream_t *stream) {
  if (stream_is(stream, TOKEN_KEYWORD_INLINE)) {
    stream_next(stream);
    return AST_FUNC_SPEC_INLINE;
  } else {
    return AST_FUNC_SPEC_NONE;
  }
}


////////////////////////////////////////////////////////////////////////////////
// stdspec.6.7.5 Declarators
////////////////////////////////////////////////////////////////////////////////

static decltor_t* parse_abstract_decltor(stream_t *stream);

static token_vec_t* parse_ident_list(stream_t *stream) {
  STREAM_PUSH();
  token_t *token = stream_peek(stream);
  if (token->tag != TOKEN_IDENT) {
    STREAM_POP();
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
  STREAM_COMMITa("parsed ident_list (%d idents)", idents->size)
  return idents;
}

static param_decl_t* parse_param_decl(stream_t *stream) {
  STREAM_PUSH();

  decl_spec_t *specifiers = parse_decl_specs(stream);
  if (!specifiers) {
    STREAM_POP();
    return 0;
  }

  param_decl_t *output = dcc_malloc(sizeof *output);
  output->is_abstract = false;
  output->decltor = parse_decltor(stream);
  if (!output->decltor) {
    output->decltor = parse_abstract_decltor(stream);
    output->is_abstract = true;
  }

  STREAM_COMMIT();
  return output;
}

static param_type_list_t* parse_param_type_list(stream_t *stream) {
  STREAM_PUSH();


  param_decl_t *pdecl = parse_param_decl(stream);
  if (!pdecl) {
    STREAM_POP();
    return 0;
  }

  bool is_vararg;
  param_decl_vec_t decls = param_decl_vec_new();
  param_decl_vec_push(&decls, pdecl);

  while (stream_is(stream, TOKEN_COMMA)) {
    stream_next(stream);

    if ((pdecl = parse_param_decl(stream))) {
      param_decl_vec_push(&decls, pdecl);
      continue;
    }

    if (stream_is(stream, TOKEN_ELLIPSE)) {
      stream_next(stream);
      is_vararg = true;
    }
    break;
  }

  param_type_list_t *output = dcc_malloc(sizeof *output);
  output->decls = decls;
  output->is_vararg = is_vararg;
  return output;
}

// NOTE: basically a copy of parse_abstract_decltor()
static decltor_t* parse_decltor(stream_t *stream) {
  STREAM_PUSH();

  type_qual_vec_t pointers = type_qual_vec_new();
  while (stream_is(stream, TOKEN_STAR)) {
    stream_next(stream);

    type_qual_t total = TYPE_QUAL_NONE;
    type_qual_t qual = parse_type_qual(stream);
    while (qual != TYPE_QUAL_NONE) {
      if (total & qual) { // already matched this qualifier, throw an error
        dcc_ice("cannot repeat type qualifier");
      }
      total |= qual;
    }

    type_qual_vec_push(&pointers, total);
  }


  token_t *token = stream_peek(stream);
  token_tag_t tag = token->tag;
  if (!(tag == TOKEN_IDENT || tag == TOKEN_LPAREN)) {
    type_qual_vec_free(&pointers);
    STREAM_POP();
    return 0;
  }
  stream_next(stream);

  direct_decltor_vec_t directs = direct_decltor_vec_new();
  direct_decltor_t direct;
  if (tag == TOKEN_IDENT) {
    direct.tag = AST_DECLTOR_IDENT;
    direct.ident = token;
  } else if (tag == TOKEN_LPAREN) {
    direct.tag = AST_DECLTOR_NESTED;
    direct.nested = parse_abstract_decltor(stream);
    if (!direct.nested) {
      stream_expected(stream, "declarator after `(`");
    }
    stream_expect(stream, TOKEN_RPAREN);
  }
  direct_decltor_vec_push(&directs, direct);

  while(true) {
    if (stream_is(stream, TOKEN_LPAREN)) {
      stream_next(stream);

      token_vec_t *idents;
      param_type_list_t *params = parse_param_type_list(stream);
      if (params) {
        direct.tag = AST_DECLTOR_FUNC_TYPES;
        direct.params = parse_param_type_list(stream);
      } else {
        direct.tag = AST_DECLTOR_FUNC_IDENTS;
        direct.idents = parse_ident_list(stream);
        // param list may be empty, which returns null
      }

      stream_expect(stream, TOKEN_RPAREN);
    } else if (stream_is(stream, TOKEN_LSQUARE)) {
      direct.tag = AST_DECLTOR_ARRAY;
      direct.array.is_static = false;
      direct.array.is_vla = false;

      if (stream_is(stream, TOKEN_KEYWORD_STATIC)) {
        stream_next(stream);
        direct.array.is_static = true;
      }

      direct.array.qualifiers = parse_type_qual(stream);
      if (stream_is(stream, TOKEN_STAR)) {
        stream_next(stream);
        direct.array.is_vla = true;
        goto vla;
      }


      if (stream_is(stream, TOKEN_KEYWORD_STATIC)) {
        if (direct.array.is_static) {
          dcc_ice("cannot repeat `static`");
        }
        stream_next(stream);
        direct.array.is_static = true;
      }

      direct.array.exp = parse_assignment_exp(stream);
      if (direct.array.is_static && !direct.array.exp) {
        dcc_ice("assignment-expression must follow static inside declarator");
      }

    vla:
      stream_expect(stream, TOKEN_RSQUARE);
   } else {
      break;
    }
    direct_decltor_vec_push(&directs, direct);
  }

  decltor_t *output = dcc_malloc(sizeof *output);
  output->pointers = pointers;
  output->directs = directs;
  STREAM_COMMIT();
  return output;
}

////////////////////////////////////////////////////////////////////////////////
// stdspec.6.7.6 Type names
////////////////////////////////////////////////////////////////////////////////

// NOTE: basically a copy of parse_decltor()
static decltor_t* parse_abstract_decltor(stream_t *stream) {
  STREAM_PUSH();

  type_qual_vec_t pointers = type_qual_vec_new();
  while (stream_is(stream, TOKEN_STAR)) {
    stream_next(stream);

    type_qual_t total = TYPE_QUAL_NONE;
    type_qual_t qual = parse_type_qual(stream);
    while (qual != TYPE_QUAL_NONE) {
      if (total & qual) { // already matched this qualifier, throw an error
        dcc_ice("cannot repeat type qualifier");
      }
      total |= qual;
    }

    type_qual_vec_push(&pointers, total);
  }

  direct_decltor_vec_t directs = direct_decltor_vec_new();
  direct_decltor_t direct;
  if (stream_is(stream, TOKEN_RPAREN)) {
    stream_next(stream);

    direct.tag = AST_DECLTOR_NESTED;
    direct.nested = parse_abstract_decltor(stream);
    if (!direct.nested) {
      stream_expected(stream, "abstract-declarator after `(`");
    }
    direct_decltor_vec_push(&directs, direct);
    stream_expect(stream, TOKEN_RPAREN);
  }

  while(true) {
    if (stream_is(stream, TOKEN_LPAREN)) {
      stream_next(stream);

      direct.tag = AST_DECLTOR_FUNC_TYPES;
      direct.params = parse_param_type_list(stream);

      stream_expect(stream, TOKEN_RPAREN);
    } else if (stream_is(stream, TOKEN_LSQUARE)) {
      direct.tag = AST_DECLTOR_ARRAY;
      direct.array.is_static = false;
      direct.array.is_vla = false;
      direct.array.qualifiers = 0;

      if (stream_is(stream, TOKEN_STAR)) {
        stream_next(stream);
        direct.array.is_vla = true;
        goto vla;
      } else if (stream_is(stream, TOKEN_KEYWORD_STATIC)) {
        stream_next(stream);
        direct.array.is_static = true;
      }

      direct.array.qualifiers = parse_type_qual(stream);

      if (stream_is(stream, TOKEN_KEYWORD_STATIC)) {
        if (direct.array.is_static) {
          dcc_ice("cannot repeat `static`");
        }
        stream_next(stream);
        direct.array.is_static = true;
      }

      direct.array.exp = parse_assignment_exp(stream);
      if (direct.array.is_static && !direct.array.exp) {
        dcc_ice("assignment-expression must follow static inside declarator");
      }

    vla:
      stream_expect(stream, TOKEN_RSQUARE);
   } else {
      break;
    }
    direct_decltor_vec_push(&directs, direct);
  }

  if (pointers.size > 0 || directs.size > 0) {
    decltor_t *output = dcc_malloc(sizeof *output);
    output->pointers = pointers;
    output->directs = directs;
    STREAM_COMMIT();
    return output;
  } else {
    STREAM_POP();
    return 0;
  }
}

static type_name_t* parse_type_name(stream_t *stream) {
  STREAM_PUSH();

  type_name_t tname = { 0, 0 };

  tname.squal = parse_type_squal(stream);
  if (!tname.squal) {
    STREAM_POP();
    return 0;
  }

  tname.decltor = parse_abstract_decltor(stream);

  type_name_t *output = dcc_malloc(sizeof *output);
  *output = tname;
  STREAM_COMMIT();
  return output;
}

////////////////////////////////////////////////////////////////////////////////
// stdspec.6.7.7 Type definitions
// only includes an alias of `typedef-name = ident`
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// stdspec.6.7.8 Initialization
////////////////////////////////////////////////////////////////////////////////

static designator_t* parse_designator(stream_t *stream) {
  STREAM_PUSH();

  designator_t designator;
  if (stream_is(stream, TOKEN_LSQUARE)) {
    stream_next(stream);
    designator.tag = DESIGNATOR_EXP;
    designator.exp = parse_exp(stream); // TODO FIXME parse constant expression
    if (!designator.exp || !stream_is(stream, TOKEN_RSQUARE)) {
      STREAM_POP();
      return 0;
      // should this throw an error? The eternal question....
    }
    stream_next(stream);
  } else if (stream_is(stream, TOKEN_DOT)) {
    stream_next(stream);
    token_t *token = stream_peek(stream);
    if (token->tag != TOKEN_IDENT) {
      STREAM_POP();
      return 0;
    }
    stream_next(stream);
    designator.tag = DESIGNATOR_IDENT;
    designator.ident = token;
  } else {
    STREAM_POP();
    return 0;
  }

  designator_t *output = dcc_malloc(sizeof *output);
  *output = designator;
  STREAM_COMMIT();
  return output;
}

static designator_vec_t* parse_designators(stream_t *stream) {
  STREAM_PUSH();

  designator_t *designator = parse_designator(stream);
  if (!designator) {
    STREAM_POP();
    return 0;
  }

  designator_vec_t *output = dcc_malloc(sizeof *output);
  *output = designator_vec_new();
  while (designator) {
    designator_vec_push(output, designator);
    designator = parse_designator(stream);
  }
  stream_expect(stream, TOKEN_EQUAL);

  STREAM_COMMIT();
  return output;
}

static initialization_vec_t* parse_initialization_list(stream_t *stream) {
  STREAM_PUSH();

  if (!stream_is(stream, TOKEN_LCURLY)) {
    STREAM_POP();
    return 0;
  }
  stream_next(stream);

  initialization_vec_t *output = dcc_malloc(sizeof *output);
  *output = initialization_vec_new();

  while (true) {
    STREAM_PUSH();
    if (output->size > 0) {
      stream_expect(stream, TOKEN_COMMA);
    }

    designator_vec_t *designators = parse_designators(stream);
    initializer_t *initializer = parse_initializer(stream);
    if (!initializer) {
      STREAM_POP();
      break;
    }

    initialization_vec_push(output, (initialization_t) {
        .designators = designators,
        .initializer = initializer,
    });
    STREAM_COMMIT();
  }

  STREAM_COMMIT();
  return output;
}

static initializer_t* parse_initializer(stream_t *stream) {
  STREAM_PUSH();

  initialization_vec_t *inits = parse_initialization_list(stream);
  if (inits) {
    initializer_t *output = dcc_malloc(sizeof *output);
    output->tag = INIT_LIST;
    output->inits = inits;

    STREAM_COMMIT();
    return output;
  }

  exp_t *assignment = parse_assignment_exp(stream);
  if (assignment) {
    initializer_t *output = dcc_malloc(sizeof *output);
    output->tag = INIT_EXP;
    output->expression = assignment;

    STREAM_COMMIT();
    return output;
  }

  STREAM_POP();
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// stdspec.6.8 Statements and blocks
////////////////////////////////////////////////////////////////////////////////

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
  /* dcc_log(LOG_TRACE, "parsed compound_statement (%d items)\n", items->size); */
  return items;
}

static func_def_t* parse_func_def(stream_t *stream) {
  STREAM_PUSH();

  decl_spec_t *specs = 0;
  decltor_t *decltor = 0;
  specs = parse_decl_specs(stream);
  if (!specs) {
    goto error;
  }

  decltor = parse_decltor(stream);
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

  STREAM_COMMIT();
  return output;
 error:
  if (specs) { free(specs); }
  if (decltor) { free(decltor); }

  STREAM_POP();
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// stdspec.6.9 External definitions
////////////////////////////////////////////////////////////////////////////////

static external_decl_t* parse_external_decl(stream_t *stream) {
  STREAM_PUSH();

  external_decl_t parsed;
  func_def_t *function = parse_func_def(stream);
  if (function) {
    parsed.tag = AST_EXT_FUNCTION;
    parsed.function = function;
  } else {
    decl_t *declaration = parse_decl(stream);
    if (!declaration) {
      STREAM_POP();
      return 0;
    }
    parsed.tag = AST_EXT_DECLARATION;
    parsed.declaration = declaration;
  }

  external_decl_t *output = dcc_malloc(sizeof(external_decl_t));
  *output = parsed;
  STREAM_COMMITa("%s", external_decl_tag_str(output->tag));
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

static const char *BLOCK_ITEM_STRINGS[] = {"statement", "declaration", };
static const char *EXTERNAL_DECL_STRINGS[] = {"function definition", "declaration", };

STRING_GETTER(block_item, enum block_item_tag, BLOCK_ITEM_STRINGS);
STRING_GETTER(external_decl, enum external_decl_tag, EXTERNAL_DECL_STRINGS);
