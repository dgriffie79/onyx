#ifndef ONYXPARSER_H
#define ONYXPARSER_H

#define BH_NO_STRING
#include "bh.h"

#include "onyxlex.h"
#include "onyxmsgs.h"

typedef union OnyxAstNode OnyxAstNode;
typedef struct OnyxAstNodeLocal OnyxAstNodeLocal;
typedef struct OnyxAstNodeScope OnyxAstNodeScope;
typedef struct OnyxAstNodeBlock OnyxAstNodeBlock;
typedef struct OnyxAstNodeParam OnyxAstNodeParam;
typedef struct OnyxAstNodeFuncDef OnyxAstNodeFuncDef;

typedef struct OnyxParser {
	OnyxTokenizer *tokenizer; // NOTE: not used since all tokens are lexed before parsing starts
	OnyxToken *prev_token;
	OnyxToken *curr_token;

	// BUG: This way of handling identifiers will work for now,
	// but it will not allow for shadowing. Also, variable names
	// cannot be the same as any function or global variable.
	// That will get annoying to program.
	// NOTE: A table of the current identifiers in the current scope.
	// If the identifier doesn't at the time of parsing, it is an error.
	// Cleared at the end of a block.
	bh_hash(OnyxAstNode*) identifiers;
	OnyxAstNodeScope *curr_scope;

	OnyxMessages *msgs;

	bh_allocator allocator;
} OnyxParser;

typedef enum OnyxAstNodeKind {
	ONYX_AST_NODE_KIND_ERROR,
	ONYX_AST_NODE_KIND_PROGRAM,

	ONYX_AST_NODE_KIND_FUNCDEF,
	ONYX_AST_NODE_KIND_BLOCK,
	ONYX_AST_NODE_KIND_SCOPE,
	ONYX_AST_NODE_KIND_LOCAL,

	ONYX_AST_NODE_KIND_ADD,
	ONYX_AST_NODE_KIND_MINUS,
	ONYX_AST_NODE_KIND_MULTIPLY,
	ONYX_AST_NODE_KIND_DIVIDE,
	ONYX_AST_NODE_KIND_MODULUS,
	ONYX_AST_NODE_KIND_NEGATE,

	ONYX_AST_NODE_KIND_TYPE,
	ONYX_AST_NODE_KIND_LITERAL,
	ONYX_AST_NODE_KIND_CAST,
	ONYX_AST_NODE_KIND_PARAM,
	ONYX_AST_NODE_KIND_CALL,
	ONYX_AST_NODE_KIND_ASSIGNMENT,
	ONYX_AST_NODE_KIND_RETURN,

	ONYX_AST_NODE_KIND_EQUAL,
	ONYX_AST_NODE_KIND_NOT_EQUAL,
	ONYX_AST_NODE_KIND_GREATER,
	ONYX_AST_NODE_KIND_GREATER_EQUAL,
	ONYX_AST_NODE_KIND_LESS,
	ONYX_AST_NODE_KIND_LESS_EQUAL,
	ONYX_AST_NODE_KIND_NOT,

	ONYX_AST_NODE_KIND_IF,
	ONYX_AST_NODE_KIND_LOOP,

	ONYX_AST_NODE_KIND_COUNT
} OnyxAstNodeKind;

typedef enum OnyxTypeInfoKind {
	ONYX_TYPE_INFO_KIND_UNKNOWN,
	ONYX_TYPE_INFO_KIND_VOID,
	ONYX_TYPE_INFO_KIND_BOOL,

	ONYX_TYPE_INFO_KIND_UINT8,
	ONYX_TYPE_INFO_KIND_UINT16,
	ONYX_TYPE_INFO_KIND_UINT32,
	ONYX_TYPE_INFO_KIND_UINT64,

	ONYX_TYPE_INFO_KIND_INT8,
	ONYX_TYPE_INFO_KIND_INT16,
	ONYX_TYPE_INFO_KIND_INT32,
	ONYX_TYPE_INFO_KIND_INT64,

	ONYX_TYPE_INFO_KIND_FLOAT32,
	ONYX_TYPE_INFO_KIND_FLOAT64,
	ONYX_TYPE_INFO_KIND_SOFT_FLOAT, // 64-bits of data but could be treated as 32-bit
} OnyxTypeInfoKind;

typedef struct OnyxTypeInfo {
	OnyxTypeInfoKind kind;
	u32 size; // in bytes
	const char* name;
	u32 is_int : 1;
	u32 is_unsigned : 1;
	u32 is_float : 1;
	u32 is_bool : 1;
} OnyxTypeInfo;

extern OnyxTypeInfo builtin_types[];

// NOTE: Some of these flags will overlap since there are
// only 32-bits of flags to play with
typedef enum OnyxAstFlags {
	// Top-level flags
	ONYX_AST_FLAG_EXPORTED   = BH_BIT(1),
} OnyxAstFlags;

struct OnyxAstNodeLocal {
	OnyxAstNodeKind kind;
	u32 flags;
	OnyxToken *token;
	OnyxTypeInfo *type;
	OnyxAstNodeLocal *prev_local;
	OnyxAstNode *shadowed;
	OnyxAstNode *unused2;
};

struct OnyxAstNodeScope {
	OnyxAstNodeKind kind;
	u32 flags;
	OnyxToken *token;	// NOTE: UNUSED
	OnyxTypeInfo *type; // NOTE: UNUSED
	OnyxAstNodeScope *prev_scope;
	OnyxAstNodeLocal *last_local;
	OnyxAstNode *unused;
};

struct OnyxAstNodeBlock {
	OnyxAstNodeKind kind;
	u32 flags;
	OnyxToken *token;
	OnyxTypeInfo *return_type;
	OnyxAstNode *next;
	OnyxAstNode *body;
	OnyxAstNodeScope *scope; // NOTE: Only set on blocks belonging to functions
};

struct OnyxAstNodeParam {
	OnyxAstNodeKind kind;
	u32 flags;
	OnyxToken *token;			// Symbol name i.e. 'a', 'b'
	OnyxTypeInfo *type;
	OnyxAstNodeParam *next;
	u64 param_count;
	OnyxAstNode *right;
};

struct OnyxAstNodeFuncDef {
	OnyxAstNodeKind kind;
	u32 flags;
	OnyxToken *token; // This will point to the symbol token to identify it
	OnyxTypeInfo *return_type;
	OnyxAstNode *next;
	OnyxAstNodeBlock *body;
	OnyxAstNodeParam *params;
};

union OnyxAstNode {

	// Generic node structure for capturing all binary ops and statements
	struct {
		OnyxAstNodeKind kind;
		u32 flags;
		OnyxToken *token;
		OnyxTypeInfo *type;
		OnyxAstNode *next;
		OnyxAstNode *left;
		OnyxAstNode *right;
	};

	OnyxAstNodeBlock as_block;
	OnyxAstNodeFuncDef as_funcdef;
	OnyxAstNodeParam as_param;
	OnyxAstNodeLocal as_local;
	OnyxAstNodeScope as_scope;
};

const char* onyx_ast_node_kind_string(OnyxAstNodeKind kind);
OnyxAstNode* onyx_ast_node_new(bh_allocator alloc, OnyxAstNodeKind kind);
OnyxParser onyx_parser_create(bh_allocator alloc, OnyxTokenizer *tokenizer, OnyxMessages* msgs);
void onyx_parser_free(OnyxParser* parser);
OnyxAstNode* onyx_parse(OnyxParser *parser);

#endif // #ifndef ONYXPARSER_H
