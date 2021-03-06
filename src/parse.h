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
#include "vec.h"
#include "tokenize.h"



#define DECLARE_STRING_GETTER(name)                 \
  const char* name##_tag_str(enum name##_tag tag);


struct decltor;
struct exp;
struct initilization;
struct initilizer;
struct struct_decltor;
struct type_squal;
typedef struct decltor decltor_t;
typedef struct exp exp_t;
typedef struct initialization initialization_t;
typedef struct initializer initializer_t;
typedef struct struct_decltor struct_decltor_t;
typedef struct type_squal type_squal_t;
DECLARE_VEC(exp_t*, exp_vec);
DECLARE_VEC(initialization_t, initialization_vec);
DECLARE_VEC(struct_decltor_t, struct_decltor_vec);


typedef enum storage_spec_tag {
  AST_STORAGE_NONE = 0,
  AST_STORAGE_TYPEDEF = 1,
  AST_STORAGE_EXTERN = 2,
  AST_STORAGE_STATIC = 4,
  AST_STORAGE_AUTO = 8,
  AST_STORAGE_REGISTER = 16,
} storage_spec_t;
DECLARE_STRING_GETTER(storage_spec);

// forward declarations

typedef struct {
  type_squal_t *squal;
  struct_decltor_vec_t sdecltors;
} struct_decl_t;
DECLARE_VEC(struct_decl_t, struct_decl_vec);

typedef struct {
  token_t *ident; // nullable
  struct_decl_vec_t decls;
} sunion_spec_t;

typedef struct {
  token_t *ident;
  exp_t *exp;
} enumtor_t;
DECLARE_VEC(enumtor_t*, enumtor_vec);

typedef struct {
  token_t *ident; // nullable
  enumtor_vec_t enumtors; // nullable
} enum_spec_t;

typedef struct {
  enum type_spec_tag {
    AST_TYPE_VOID,
    AST_TYPE_CHAR,
    AST_TYPE_SHORT,
    AST_TYPE_INT,
    AST_TYPE_LONG,
    AST_TYPE_FLOAT,
    AST_TYPE_DOUBLE,
    AST_TYPE_SIGNED,
    AST_TYPE_UNSIGNED,
    AST_TYPE__BOOL,
    AST_TYPE__COMPLEX,
    AST_TYPE_STRUCT,
    AST_TYPE_UNION,
    AST_TYPE_ENUM,
    AST_TYPE_TYPEDEF,
  } tag;
  union {
    sunion_spec_t *suspec;
    enum_spec_t *espec;
    token_t *ident;
  };
} type_spec_t;
DECLARE_STRING_GETTER(type_spec);

typedef enum type_qual {
  TYPE_QUAL_NONE = 0,
  TYPE_QUAL_CONST = 1,
  TYPE_QUAL_RESTRICT = 2,
  TYPE_QUAL_VOLATILE = 4,
} type_qual_t;
DECLARE_VEC(type_qual_t, type_qual_vec);

struct type_squal {
    type_spec_t *spec;
    type_qual_t qual;
}; // typedef forward declared

typedef enum func_spec_tag {
  AST_FUNC_SPEC_NONE,
  AST_FUNC_SPEC_INLINE,
} func_spec_t;
DECLARE_STRING_GETTER(func_spec);

typedef struct {
    storage_spec_t storage;
    type_spec_t *type_spec;
    type_qual_t type_qual;
    func_spec_t func_spec;
} decl_spec_t;

typedef struct {
  decl_spec_t *specifiers;
  decltor_t *decltor; // may be null
  bool is_abstract;
} param_decl_t;
DECLARE_VEC(param_decl_t*, param_decl_vec);

typedef struct {
  param_decl_vec_t decls;
  bool is_vararg;
} param_type_list_t;

typedef struct {
  enum direct_decltor_tag {
    AST_DECLTOR_IDENT,
    AST_DECLTOR_NESTED,
    AST_DECLTOR_ARRAY,
    AST_DECLTOR_FUNC_TYPES,
    AST_DECLTOR_FUNC_IDENTS,
  } tag;
  union {
    token_t *ident;
    decltor_t *nested;
    struct {
      bool is_static;
      bool is_vla;
      type_qual_t qualifiers;
      exp_t *exp;
    } array;
    param_type_list_t *params;
    token_vec_t *idents; // nullable
  };
} direct_decltor_t;
DECLARE_VEC(direct_decltor_t, direct_decltor_vec);

struct decltor {
  type_qual_vec_t pointers;
  direct_decltor_vec_t directs;
};

typedef struct {
  type_squal_t *squal;
  decltor_t *decltor;
} type_name_t;

typedef struct constant {
  enum constant_tag {
    CONSTANT_INTEGER,
    CONSTANT_FLOAT,
    CONSTANT_ENUM,
    CONSTANT_CHAR,
  } tag;
  union {
    int integer;
    double floating;
    
  };
} constant_t;

struct exp {
  enum exp_tag {
    EXP_UNKNOWN,
    EXP_DOT,
    EXP_ADDRESSOF,
    EXP_MULTIPLY,
    EXP_DEREFERENCE,
    EXP_ADD,
    EXP_SUBTRACT,
    EXP_BITNOT,
    EXP_LOGICNOT,
    EXP_DIVIDE,
    EXP_MODULO,
    EXP_LESS,
    EXP_MORE,
    EXP_BITXOR,
    EXP_BITAND,
    EXP_BITOR,
    EXP_TERNARY,
    EXP_ASSIGN,
    EXP_LIST, // exp, exp
    EXP_ARROW,
    EXP_PREINCREMENT,
    EXP_PREDECREMENT,
    EXP_POSTINCREMENT,
    EXP_POSTDECREMENT,
    EXP_SHIFTLEFT,
    EXP_SHIFTRIGHT,
    EXP_MOREEQ,
    EXP_LESSEQ,
    EXP_EQUAL,
    EXP_NOTEQUAL,
    EXP_LOGICAND,
    EXP_LOGICOR,
    EXP_STRUCT,
    EXP_STRING,
    EXP_IDENT,
    EXP_CONSTANT,
    EXP_INDEX,
    EXP_CALL,
    EXP_SIZEOFEXP,
    EXP_SIZEOFTYPE,
    EXP_NEGATE,
  } tag;
  union {
    struct {
      type_name_t *type;
      struct exp *value;
    } cast;
    struct exp *unary;
    struct {
      struct exp *lhs, *rhs;
    } binary;
    struct {
      struct exp *cond, *true_exp, *false_exp;
    } ternary;
    struct {
      struct exp *lhs;
      exp_vec_t *args;
    } call;
    exp_vec_t list;
    struct {
      struct exp *lhs;
      token_t *name;
    } child;
    token_t *token;
    constant_t *constant;
    struct {
      struct exp *lhs, *rhs;
      enum exp_tag operator;
    } assignment;
    struct {
      type_name_t *tname;
      initialization_vec_t *inits;
    } struct_init;
  };
};

struct struct_decltor {
  decltor_t *decltor;
  exp_t *exp;
}; // typedef forward declared

typedef struct {
  enum {
    DESIGNATOR_EXP,
    DESIGNATOR_IDENT,
  } tag;
  union {
    token_t *ident;
    exp_t *exp;
  };
} designator_t;
DECLARE_VEC(designator_t*, designator_vec);

struct initializer {
  enum {
    INIT_EXP,
    INIT_LIST,
  } tag;
  union {
    exp_t *expression;
    initialization_vec_t *inits;
  };
};
struct initialization {
  designator_vec_t *designators; // nullable
  initializer_t *initializer;
};

typedef struct init_decltor {
  decltor_t *declarator;
  initializer_t *initializer;
} init_decltor_t;
DECLARE_VEC(init_decltor_t*, init_decltor_vec);

typedef struct {
  decl_spec_t *specifiers;
  init_decltor_vec_t init_decltors;
} decl_t;
DECLARE_VEC(decl_t*, decl_vec);

struct stmt;
typedef struct stmt stmt_t;

typedef struct {
  enum block_item_tag {
    AST_STATEMENT,
    AST_DECLARATION,
  } tag;
  union {
    stmt_t *statement;
    decl_t *declaration;
  };
} block_item_t;
DECLARE_VEC(block_item_t*, block_item_vec);
DECLARE_STRING_GETTER(block_item);

struct stmt {
  enum {
    STMT_CASE,
    STMT_DEFAULT,
    STMT_LABEL,
    STMT_COMPOUND,
    STMT_EXP,
    STMT_IF,
    STMT_SWITCH,
    STMT_DO,
    STMT_WHILE,
    STMT_FOR,
    STMT_GOTO,
    STMT_CONTINUE,
    STMT_BREAK,
    STMT_RETURN,
  } tag;
  union {
    /* exp_t *exp; */
    struct {
      exp_t *exp;
      stmt_t *stmt;
    } stmt_case;
    struct {
      token_t *ident;
      stmt_t *stmt;
    } stmt_label;
    stmt_t *stmt;
    block_item_vec_t stmt_compound;
    exp_t *exp;
    struct {
      exp_t *exp;
      stmt_t *primary, *secondary;
    } stmt_select;
    struct {
      exp_t *exp;
      stmt_t *stmt;
    } stmt_whiledo;
    struct {
      exp_t *exp1, *exp2, *exp3;
      stmt_t *stmt;
    } stmt_for;
    token_t *token;
  };
};

typedef struct {
  decl_spec_t *specifiers;
  decltor_t *declarator;
  decl_vec_t *declarations;
  stmt_t *compound;
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
