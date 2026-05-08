#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <libgen.h>
#include <limits.h>

/* ================================= Errors ================================= */

#define EXIT_USER_ERROR 1
#define EXIT_RUNTIME 2
#define EXIT_INTERNAL 101

/* Error code group offsets */
#define ERR_GROUP_INTERNAL 1000
#define ERR_GROUP_DRIVER   2000
#define ERR_GROUP_LEXER    3000
#define ERR_GROUP_PARSER   4000
#define ERR_GROUP_SEMANTIC 5000
#define ERR_GROUP_RUNTIME  6000

typedef struct {
    size_t line;
    size_t column;
    const char *file;
} SourceLoc;

typedef enum {
    /* Internal errors: I001-I099 */
    ERR_I001_OUT_OF_MEMORY  = ERR_GROUP_INTERNAL + 1,
    ERR_I002_INTERNAL_ERROR = ERR_GROUP_INTERNAL + 2,

    /* Driver errors: D001-D099 */
    ERR_D001_NO_INPUT_FILE      = ERR_GROUP_DRIVER + 1,
    ERR_D002_UNKNOWN_FLAG       = ERR_GROUP_DRIVER + 2,
    ERR_D003_MULTIPLE_INPUTS    = ERR_GROUP_DRIVER + 3,
    ERR_D004_MISSING_OUTPUT_PATH = ERR_GROUP_DRIVER + 4,
    ERR_D005_FILE_NOT_FOUND     = ERR_GROUP_DRIVER + 5,
    ERR_D006_CLANG_FAILED       = ERR_GROUP_DRIVER + 6,
    ERR_D007_IMPORT_NOT_FOUND   = ERR_GROUP_DRIVER + 7,

    /* Lexer errors: L001-L099 */
    ERR_L001_INVALID_CHAR        = ERR_GROUP_LEXER + 1,
    ERR_L002_UNTERMINATED_COMMENT = ERR_GROUP_LEXER + 2,
    ERR_L003_UNTERMINATED_STRING  = ERR_GROUP_LEXER + 3,
    ERR_L004_INVALID_ESCAPE       = ERR_GROUP_LEXER + 4,
    ERR_L005_EMPTY_CHAR           = ERR_GROUP_LEXER + 5,
    ERR_L006_CHAR_TOO_LONG        = ERR_GROUP_LEXER + 6,

    /* Parser errors: P001-P099 */
    ERR_P001_EXPECTED_EXPRESSION   = ERR_GROUP_PARSER + 1,
    ERR_P002_EXPECTED_RPAREN       = ERR_GROUP_PARSER + 2,
    ERR_P003_EXPECTED_IDENTIFIER   = ERR_GROUP_PARSER + 3,
    ERR_P004_EXPECTED_EQUALS       = ERR_GROUP_PARSER + 4,
    ERR_P005_EXPECTED_STATEMENT    = ERR_GROUP_PARSER + 5,
    ERR_P006_NUMBER_TOO_LARGE      = ERR_GROUP_PARSER + 6,
    ERR_P007_EXPECTED_RBRACE       = ERR_GROUP_PARSER + 7,
    ERR_P008_EXPECTED_LPAREN_IF    = ERR_GROUP_PARSER + 8,
    ERR_P009_EXPECTED_RPAREN_IF    = ERR_GROUP_PARSER + 9,
    ERR_P010_EXPECTED_LBRACE_IF    = ERR_GROUP_PARSER + 10,
    ERR_P011_EXPECTED_LPAREN_WHILE = ERR_GROUP_PARSER + 11,
    ERR_P012_EXPECTED_RPAREN_WHILE = ERR_GROUP_PARSER + 12,
    ERR_P013_EXPECTED_LBRACE_WHILE = ERR_GROUP_PARSER + 13,
    ERR_P014_EXPECTED_COLON        = ERR_GROUP_PARSER + 14,
    ERR_P015_EXPECTED_TYPE         = ERR_GROUP_PARSER + 15,
    ERR_P016_EXPECTED_FUNC         = ERR_GROUP_PARSER + 16,
    ERR_P017_EXPECTED_FUNC_NAME    = ERR_GROUP_PARSER + 17,
    ERR_P018_EXPECTED_LPAREN_FUNC  = ERR_GROUP_PARSER + 18,
    ERR_P019_EXPECTED_RPAREN_FUNC  = ERR_GROUP_PARSER + 19,
    ERR_P020_EXPECTED_COLON_FUNC   = ERR_GROUP_PARSER + 20,
    ERR_P021_EXPECTED_RETURN_TYPE  = ERR_GROUP_PARSER + 21,
    ERR_P022_EXPECTED_EQUALS_FUNC  = ERR_GROUP_PARSER + 22,
    ERR_P023_EXPECTED_LBRACE_FUNC  = ERR_GROUP_PARSER + 23,
    ERR_P024_EXPECTED_VALUE_NAME   = ERR_GROUP_PARSER + 24,
    ERR_P025_EXPECTED_LBRACE_VALUE = ERR_GROUP_PARSER + 25,
    ERR_P026_EXPECTED_FIELD_NAME   = ERR_GROUP_PARSER + 26,
    ERR_P027_EXPECTED_RBRACE_VALUE = ERR_GROUP_PARSER + 27,
    ERR_P028_EXPECTED_FIELD_CTOR   = ERR_GROUP_PARSER + 28,
    ERR_P029_EXPECTED_COLON_CTOR   = ERR_GROUP_PARSER + 29,
    ERR_P030_EXPECTED_RBRACE_CTOR  = ERR_GROUP_PARSER + 30,
    ERR_P031_EXPECTED_ARRAY_ELEM_TYPE = ERR_GROUP_PARSER + 31,
    ERR_P032_EXPECTED_SEMICOLON_ARRAY = ERR_GROUP_PARSER + 32,
    ERR_P033_EXPECTED_ARRAY_SIZE   = ERR_GROUP_PARSER + 33,
    ERR_P034_EXPECTED_RBRACKET     = ERR_GROUP_PARSER + 34,
    ERR_P035_EXPECTED_RBRACKET_LITERAL = ERR_GROUP_PARSER + 35,
    ERR_P036_EXPECTED_REF_PARAM    = ERR_GROUP_PARSER + 36,
    ERR_P037_EXPECTED_REF_ARG      = ERR_GROUP_PARSER + 37,
    ERR_P038_EXPECTED_IN_FOR       = ERR_GROUP_PARSER + 38,
    ERR_P039_EXPECTED_DOTDOT       = ERR_GROUP_PARSER + 39,
    ERR_P040_EXPECTED_LBRACE_FOR   = ERR_GROUP_PARSER + 40,
    ERR_P041_EXPECTED_ENUM_NAME    = ERR_GROUP_PARSER + 41,
    ERR_P042_EXPECTED_LBRACE_ENUM  = ERR_GROUP_PARSER + 42,
    ERR_P043_EXPECTED_RBRACE_ENUM  = ERR_GROUP_PARSER + 43,
    ERR_P044_EXPECTED_VARIANT_NAME = ERR_GROUP_PARSER + 44,
    ERR_P045_UNKNOWN_ENUM_VARIANT  = ERR_GROUP_PARSER + 45,
    ERR_P046_EXPECTED_IMPORT_PATH  = ERR_GROUP_PARSER + 46,
    ERR_P047_EXPECTED_RBRACKET_SLICE = ERR_GROUP_PARSER + 47,
    ERR_P048_EXPECTED_RPAREN_PAYLOAD = ERR_GROUP_PARSER + 48,
    ERR_P049_EXPECTED_LPAREN_MATCH = ERR_GROUP_PARSER + 49,
    ERR_P050_EXPECTED_RPAREN_MATCH = ERR_GROUP_PARSER + 50,
    ERR_P051_EXPECTED_LBRACE_MATCH = ERR_GROUP_PARSER + 51,
    ERR_P052_EXPECTED_VARIANT_MATCH = ERR_GROUP_PARSER + 52,
    ERR_P053_EXPECTED_EQUALS_MATCH = ERR_GROUP_PARSER + 53,
    ERR_P054_EXPECTED_RBRACE_MATCH = ERR_GROUP_PARSER + 54,
    ERR_P055_EXPECTED_RPAREN_BINDING = ERR_GROUP_PARSER + 55,
    ERR_P056_PUB_NOT_DECLARATION   = ERR_GROUP_PARSER + 56,
    ERR_P057_PUB_IMPORT            = ERR_GROUP_PARSER + 57,
    ERR_P058_EXPECTED_MODULE_ALIAS = ERR_GROUP_PARSER + 58,
    ERR_P059_NATIVE_NOT_FUNC       = ERR_GROUP_PARSER + 59,
    ERR_P060_NATIVE_HAS_BODY       = ERR_GROUP_PARSER + 60,

    /* Semantic errors: S001-S099 */
    ERR_S001_DUPLICATE_VARIABLE    = ERR_GROUP_SEMANTIC + 1,
    ERR_S002_UNDECLARED_VARIABLE   = ERR_GROUP_SEMANTIC + 2,
    ERR_S003_IMMUTABLE_ASSIGNMENT  = ERR_GROUP_SEMANTIC + 3,
    ERR_S004_BREAK_OUTSIDE_LOOP    = ERR_GROUP_SEMANTIC + 4,
    ERR_S005_CONTINUE_OUTSIDE_LOOP = ERR_GROUP_SEMANTIC + 5,
    ERR_S006_TYPE_MISMATCH         = ERR_GROUP_SEMANTIC + 6,
    ERR_S007_CONDITION_NOT_BOOL    = ERR_GROUP_SEMANTIC + 7,
    ERR_S008_DUPLICATE_FUNCTION    = ERR_GROUP_SEMANTIC + 8,
    ERR_S009_MISSING_MAIN          = ERR_GROUP_SEMANTIC + 9,
    ERR_S010_INVALID_MAIN_SIG      = ERR_GROUP_SEMANTIC + 10,
    ERR_S011_DUPLICATE_PARAM       = ERR_GROUP_SEMANTIC + 11,
    ERR_S012_VOID_PARAM            = ERR_GROUP_SEMANTIC + 12,
    ERR_S013_RETURN_TYPE_MISMATCH  = ERR_GROUP_SEMANTIC + 13,
    ERR_S014_VOID_RETURN_VALUE     = ERR_GROUP_SEMANTIC + 14,
    ERR_S015_MISSING_RETURN_VALUE  = ERR_GROUP_SEMANTIC + 15,
    ERR_S016_UNDEFINED_FUNCTION    = ERR_GROUP_SEMANTIC + 16,
    ERR_S017_WRONG_ARG_COUNT       = ERR_GROUP_SEMANTIC + 17,
    ERR_S018_ARG_TYPE_MISMATCH     = ERR_GROUP_SEMANTIC + 18,
    ERR_S019_DIVISION_BY_ZERO      = ERR_GROUP_SEMANTIC + 19,
    ERR_S020_TYPE_REQUIRED         = ERR_GROUP_SEMANTIC + 20,
    ERR_S021_BRANCH_TYPE_MISMATCH  = ERR_GROUP_SEMANTIC + 21,
    ERR_S022_DUPLICATE_FIELD       = ERR_GROUP_SEMANTIC + 22,
    ERR_S023_UNKNOWN_FIELD_TYPE    = ERR_GROUP_SEMANTIC + 23,
    ERR_S024_UNKNOWN_CTOR_FIELD    = ERR_GROUP_SEMANTIC + 24,
    ERR_S025_MISSING_CTOR_FIELD    = ERR_GROUP_SEMANTIC + 25,
    ERR_S026_DUPLICATE_CTOR_FIELD  = ERR_GROUP_SEMANTIC + 26,
    ERR_S027_NOT_A_VALUE_TYPE      = ERR_GROUP_SEMANTIC + 27,
    ERR_S028_FIELD_ON_NON_VALUE    = ERR_GROUP_SEMANTIC + 28,
    ERR_S029_UNKNOWN_FIELD_ACCESS  = ERR_GROUP_SEMANTIC + 29,
    ERR_S030_FIELD_IMMUTABLE       = ERR_GROUP_SEMANTIC + 30,
    ERR_S031_ARRAY_SIZE_INVALID    = ERR_GROUP_SEMANTIC + 31,
    ERR_S032_ARRAY_LENGTH_MISMATCH = ERR_GROUP_SEMANTIC + 32,
    ERR_S033_ARRAY_ELEM_TYPE       = ERR_GROUP_SEMANTIC + 33,
    ERR_S034_INDEX_NOT_INTEGER     = ERR_GROUP_SEMANTIC + 34,
    ERR_S035_INDEX_ON_NON_ARRAY    = ERR_GROUP_SEMANTIC + 35,
    ERR_S036_INDEX_IMMUTABLE       = ERR_GROUP_SEMANTIC + 36,
    ERR_S037_REF_ON_FIELD          = ERR_GROUP_SEMANTIC + 37,
    ERR_S038_REF_MISMATCH          = ERR_GROUP_SEMANTIC + 38,
    ERR_S039_REF_NOT_ADDRESSABLE   = ERR_GROUP_SEMANTIC + 39,
    ERR_S040_MUT_REF_IMMUTABLE     = ERR_GROUP_SEMANTIC + 40,
    ERR_S041_REF_SCALAR_FIELD      = ERR_GROUP_SEMANTIC + 41,
    ERR_S042_REF_ARRAY_ELEMENT     = ERR_GROUP_SEMANTIC + 42,
    ERR_S043_STRUCT_COPY           = ERR_GROUP_SEMANTIC + 43,
    ERR_S044_STRUCT_BY_VALUE       = ERR_GROUP_SEMANTIC + 44,
    ERR_S045_EMBED_STRUCT          = ERR_GROUP_SEMANTIC + 45,
    ERR_S046_SLICE_LOCAL_VAR       = ERR_GROUP_SEMANTIC + 46,
    ERR_S047_SLICE_AS_FIELD        = ERR_GROUP_SEMANTIC + 47,
    ERR_S048_SLICE_RETURN          = ERR_GROUP_SEMANTIC + 48,
    ERR_S049_SLICE_NO_REF          = ERR_GROUP_SEMANTIC + 49,
    ERR_S050_LITERAL_OUT_OF_RANGE  = ERR_GROUP_SEMANTIC + 50,
    ERR_S051_ARENA_ALLOC_TYPE      = ERR_GROUP_SEMANTIC + 51,
    ERR_S052_ARENA_IMMUTABLE       = ERR_GROUP_SEMANTIC + 52,
    ERR_S053_SLICE_ESCAPES_ARENA   = ERR_GROUP_SEMANTIC + 53,
    ERR_S054_STRING_LITERAL_MUT    = ERR_GROUP_SEMANTIC + 54,
    ERR_S055_ARENA_RESET_IMMUTABLE = ERR_GROUP_SEMANTIC + 55,
    ERR_S056_SLICE_INVALIDATED     = ERR_GROUP_SEMANTIC + 56,
    ERR_S057_GLOBAL_NOT_CONSTANT   = ERR_GROUP_SEMANTIC + 57,
    ERR_S058_FOR_RANGE_NOT_INTEGER = ERR_GROUP_SEMANTIC + 58,
    ERR_S059_TABLE_FIELD_TYPE      = ERR_GROUP_SEMANTIC + 59,
    ERR_S060_NOT_TABLE_TYPE        = ERR_GROUP_SEMANTIC + 60,
    ERR_S061_MODULO_ON_FLOAT       = ERR_GROUP_SEMANTIC + 61,
    ERR_S062_BITWISE_ON_FLOAT      = ERR_GROUP_SEMANTIC + 62,
    ERR_S063_INVALID_CAST          = ERR_GROUP_SEMANTIC + 63,
    ERR_S064_IO_FD_TYPE            = ERR_GROUP_SEMANTIC + 64,
    ERR_S065_IO_DATA_TYPE          = ERR_GROUP_SEMANTIC + 65,
    ERR_S066_IO_BUF_IMMUTABLE      = ERR_GROUP_SEMANTIC + 66,
    ERR_S067_ENUM_ARITHMETIC       = ERR_GROUP_SEMANTIC + 67,
    ERR_S068_ENUM_COMPARE_MISMATCH = ERR_GROUP_SEMANTIC + 68,
    ERR_S069_DUPLICATE_ENUM_VARIANT = ERR_GROUP_SEMANTIC + 69,
    ERR_S070_DUPLICATE_TYPE_NAME   = ERR_GROUP_SEMANTIC + 70,
    ERR_S071_SLICE_RANGE_NOT_INT   = ERR_GROUP_SEMANTIC + 71,
    ERR_S072_SLICE_ON_NON_ARRAY    = ERR_GROUP_SEMANTIC + 72,
    ERR_S073_MATCH_NOT_ENUM        = ERR_GROUP_SEMANTIC + 73,
    ERR_S074_NON_EXHAUSTIVE_MATCH  = ERR_GROUP_SEMANTIC + 74,
    ERR_S075_DUPLICATE_MATCH_ARM   = ERR_GROUP_SEMANTIC + 75,
    ERR_S076_MATCH_ARM_TYPE_MISMATCH = ERR_GROUP_SEMANTIC + 76,
    ERR_S077_PAYLOAD_TYPE_MISMATCH = ERR_GROUP_SEMANTIC + 77,
    ERR_S078_UNEXPECTED_PAYLOAD    = ERR_GROUP_SEMANTIC + 78,
    ERR_S079_MISSING_PAYLOAD       = ERR_GROUP_SEMANTIC + 79,
    ERR_S080_TAGGED_ENUM_COMPARE   = ERR_GROUP_SEMANTIC + 80,
    ERR_S081_TAGGED_ENUM_CAST      = ERR_GROUP_SEMANTIC + 81,
    ERR_S082_PAYLOAD_NOT_VALUE     = ERR_GROUP_SEMANTIC + 82,
    ERR_S083_NOT_PUBLIC_FUNC       = ERR_GROUP_SEMANTIC + 83,
    ERR_S084_NOT_PUBLIC_TYPE       = ERR_GROUP_SEMANTIC + 84,
    ERR_S085_NOT_PUBLIC_VALUE      = ERR_GROUP_SEMANTIC + 85,
    ERR_S086_UNKNOWN_NATIVE        = ERR_GROUP_SEMANTIC + 86,
    ERR_S087_MISSING_NATIVE_DECL   = ERR_GROUP_SEMANTIC + 87,
    ERR_S088_INVALID_ASSIGN_TARGET = ERR_GROUP_SEMANTIC + 88,
    ERR_S089_CATCH_ALL_NOT_LAST    = ERR_GROUP_SEMANTIC + 89,
    ERR_S090_INVALID_NATIVE_SIGNATURE = ERR_GROUP_SEMANTIC + 90,
    ERR_S091_ASSERT_STRIP_EFFECT   = ERR_GROUP_SEMANTIC + 91,

    /* Runtime errors: R001-R099 */
    ERR_R001_ASSERTION_FAILED      = ERR_GROUP_RUNTIME + 1,
    ERR_R002_INDEX_OUT_OF_BOUNDS   = ERR_GROUP_RUNTIME + 2,
    ERR_R003_CAST_OVERFLOW         = ERR_GROUP_RUNTIME + 3,
    ERR_R004_SLICE_BOUNDS          = ERR_GROUP_RUNTIME + 4,
} ErrorCode;

static const char *error_code_str(ErrorCode code) {
    static char buffer[8];
    char prefix;
    int num;

    if (code >= ERR_GROUP_RUNTIME) {
        prefix = 'R'; num = code - ERR_GROUP_RUNTIME;
    } else if (code >= ERR_GROUP_SEMANTIC) {
        prefix = 'S'; num = code - ERR_GROUP_SEMANTIC;
    } else if (code >= ERR_GROUP_PARSER) {
        prefix = 'P'; num = code - ERR_GROUP_PARSER;
    } else if (code >= ERR_GROUP_LEXER) {
        prefix = 'L'; num = code - ERR_GROUP_LEXER;
    } else if (code >= ERR_GROUP_DRIVER) {
        prefix = 'D'; num = code - ERR_GROUP_DRIVER;
    } else {
        prefix = 'I'; num = code - ERR_GROUP_INTERNAL;
    }

    snprintf(buffer, sizeof(buffer), "%c%03d", prefix, num);
    return buffer;
}

/* Internal errors (bugs, OOM) - exits with 101 */
static void panic(ErrorCode code, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "internal error[%s]: ", error_code_str(code));
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_INTERNAL);
}

/* User errors without source location (CLI, file I/O) */
static void error(ErrorCode code, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "error[%s]: ", error_code_str(code));
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_USER_ERROR);
}

/* Global source file path for diagnostics */
static const char *g_source_file = "<unknown>";

/* Compiler binary directory (for resolving std/ imports) */
static char g_compiler_dir[PATH_MAX] = "";

/* Import tracking: already-imported files (resolved absolute paths) */
#define MAX_IMPORTS 256
static char *g_imported_files[MAX_IMPORTS];
static size_t g_imported_count = 0;

/* Source buffers from imported files (need to stay alive for token pointers) */
static char *g_import_sources[MAX_IMPORTS];
static size_t g_import_source_count = 0;

/* Module registry: each file (main + imports) is a module */
typedef struct {
    const char *path;           /* canonical path */
    const char *alias;          /* NULL for the main file */
    size_t alias_length;
} ModuleEntry;

static ModuleEntry g_modules[MAX_IMPORTS];
static size_t g_module_count = 0;
static size_t g_current_module_id = 0;

/* Sentinel value for built-in declarations (visible from all modules) */
#define MODULE_BUILTIN SIZE_MAX

/* Module alias table: maps (alias, scope_module) -> target_module.
 * Each import statement creates one entry scoped to the importing module. */
typedef struct {
    const char *alias;
    size_t alias_length;
    size_t target_module_id;
    size_t scope_module_id;
} ModuleAlias;

static ModuleAlias g_module_aliases[MAX_IMPORTS * 4];
static size_t g_module_alias_count = 0;

/* Look up a module alias in the current module's scope (parsing only) */
static int module_lookup_alias(const char *name, size_t length) {
    for (size_t i = 0; i < g_module_alias_count; i++) {
        if (g_module_aliases[i].scope_module_id == g_current_module_id &&
            g_module_aliases[i].alias_length == length &&
            memcmp(g_module_aliases[i].alias, name, length) == 0) {
            return (int)g_module_aliases[i].target_module_id;
        }
    }
    return -1;
}

/* Derive module_id from a source file path */
static size_t module_for_file(const char *file) {
    for (size_t m = 0; m < g_module_count; m++) {
        if (g_modules[m].path == file) return m;
    }
    return 0;
}

/* Error collection for multi-error reporting */
#define MAX_ERRORS 10

typedef struct {
    ErrorCode code;
    size_t line;
    size_t column;
    const char *file;
    char message[256];
} CollectedError;

static CollectedError g_errors[MAX_ERRORS];
static size_t g_error_count = 0;
static bool g_had_error = false;

/* Record error for later reporting (does not exit) */
static void diagnostic(const char *file, ErrorCode code, size_t line,
                       size_t column, const char *fmt, ...) {
    g_had_error = true;

    if (g_error_count < MAX_ERRORS) {
        CollectedError *err = &g_errors[g_error_count++];
        err->code = code;
        err->line = line;
        err->column = column;
        err->file = file;
        va_list args;
        va_start(args, fmt);
        vsnprintf(err->message, sizeof(err->message), fmt, args);
        va_end(args);
    }
}

/* Print all collected errors and exit */
static void report_errors_and_exit(void) {
    for (size_t i = 0; i < g_error_count; i++) {
        CollectedError *err = &g_errors[i];
        fprintf(stderr, "%s:%zu:%zu: error[%s]: %s\n",
                err->file, err->line, err->column,
                error_code_str(err->code), err->message);
    }

    if (g_error_count == MAX_ERRORS) {
        fprintf(stderr, "Too many errors, stopping.\n");
    }

    exit(EXIT_USER_ERROR);
}

/* Check if we should stop (too many errors) */
static bool too_many_errors(void) {
    return g_error_count >= MAX_ERRORS;
}

/* ================================== Files ================================= */

char *read_file(const char *path, size_t *out_length) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        error(ERR_D005_FILE_NOT_FOUND,
                     "Could not open file '%s': %s", path, strerror(errno));
    }

    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(length + 1);
    if (!buffer) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating buffer for file '%s'", path);
    }

    size_t bytes_read = fread(buffer, 1, length, file);
    if (bytes_read != length) {
        error(ERR_D005_FILE_NOT_FOUND,
                     "Could not read file '%s'", path);
    }

    buffer[length] = '\0';
    fclose(file);

    *out_length = length;
    return buffer;
}

/* ================================== Chars ================================= */

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static int is_valid_escape(char esc, char quote) {
    return esc == 'n' || esc == 't' || esc == 'r' || esc == '\\' ||
           esc == quote || esc == '0';
}

/* ================================= Tokens ================================= */

typedef enum {
    TOKEN_NUMBER,
    TOKEN_FLOAT,
    TOKEN_IDENTIFIER,

    TOKEN_FUNC,
    TOKEN_VAL,
    TOKEN_MUT,
    TOKEN_RETURN,
    TOKEN_ASSERT,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_MATCH,
    TOKEN_IN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_VOID,
    TOKEN_I64,
    TOKEN_I32,
    TOKEN_U8,
    TOKEN_U32,
    TOKEN_F64,
    TOKEN_BOOL,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_VALUE,
    TOKEN_STRUCT,
    TOKEN_TABLE,
    TOKEN_ENUM,
    TOKEN_IMPORT,
    TOKEN_PUB,
    TOKEN_NATIVE,
    TOKEN_REF,
    TOKEN_ARENA,
    TOKEN_STR,
    TOKEN_STRING,
    TOKEN_CHAR,

    TOKEN_PLUS,
    TOKEN_PLUS_EQUAL,
    TOKEN_MINUS,
    TOKEN_MINUS_EQUAL,
    TOKEN_STAR,
    TOKEN_STAR_EQUAL,
    TOKEN_SLASH,
    TOKEN_SLASH_EQUAL,
    TOKEN_PERCENT,
    TOKEN_PERCENT_EQUAL,
    TOKEN_AMPERSAND,
    TOKEN_AMPERSAND_EQUAL,
    TOKEN_PIPE,
    TOKEN_PIPE_EQUAL,
    TOKEN_CARET,
    TOKEN_CARET_EQUAL,
    TOKEN_TILDE,
    TOKEN_LESS_LESS,
    TOKEN_LESS_LESS_EQUAL,
    TOKEN_GREATER_GREATER,
    TOKEN_GREATER_GREATER_EQUAL,
    TOKEN_EQUALS,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG_EQUAL,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND_AND,
    TOKEN_OR_OR,
    TOKEN_BANG,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_SEMICOLON,
    TOKEN_DOTDOT,

    TOKEN_EOF,
    TOKEN_ERROR
} TokenKind;

typedef struct {
    TokenKind kind;
    const char *start;
    size_t length;
    size_t line;
    size_t column;
} Token;

static const char *token_kind_name(TokenKind kind) {
    switch (kind) {
        case TOKEN_NUMBER:        return "NUMBER";
        case TOKEN_FLOAT:         return "FLOAT";
        case TOKEN_IDENTIFIER:    return "IDENTIFIER";
        case TOKEN_FUNC:          return "FUNC";
        case TOKEN_VAL:           return "VAL";
        case TOKEN_MUT:           return "MUT";
        case TOKEN_RETURN:        return "RETURN";
        case TOKEN_ASSERT:        return "ASSERT";
        case TOKEN_IF:            return "IF";
        case TOKEN_ELSE:          return "ELSE";
        case TOKEN_WHILE:         return "WHILE";
        case TOKEN_FOR:           return "FOR";
        case TOKEN_IN:            return "IN";
        case TOKEN_BREAK:         return "BREAK";
        case TOKEN_CONTINUE:      return "CONTINUE";
        case TOKEN_VOID:          return "VOID";
        case TOKEN_I64:           return "I64";
        case TOKEN_I32:           return "I32";
        case TOKEN_U8:            return "U8";
        case TOKEN_U32:           return "U32";
        case TOKEN_F64:           return "F64";
        case TOKEN_BOOL:          return "BOOL";
        case TOKEN_TRUE:          return "TRUE";
        case TOKEN_FALSE:         return "FALSE";
        case TOKEN_VALUE:         return "VALUE";
        case TOKEN_STRUCT:        return "STRUCT";
        case TOKEN_TABLE:         return "TABLE";
        case TOKEN_ENUM:          return "ENUM";
        case TOKEN_IMPORT:        return "IMPORT";
        case TOKEN_PUB:           return "PUB";
        case TOKEN_REF:           return "REF";
        case TOKEN_ARENA:         return "ARENA";
        case TOKEN_STR:           return "STR";
        case TOKEN_STRING:        return "STRING";
        case TOKEN_CHAR:          return "CHAR";
        case TOKEN_PLUS:          return "PLUS";
        case TOKEN_PLUS_EQUAL:    return "PLUS_EQUAL";
        case TOKEN_MINUS:         return "MINUS";
        case TOKEN_MINUS_EQUAL:   return "MINUS_EQUAL";
        case TOKEN_STAR:          return "STAR";
        case TOKEN_STAR_EQUAL:    return "STAR_EQUAL";
        case TOKEN_SLASH:         return "SLASH";
        case TOKEN_SLASH_EQUAL:   return "SLASH_EQUAL";
        case TOKEN_PERCENT:       return "PERCENT";
        case TOKEN_PERCENT_EQUAL: return "PERCENT_EQUAL";
        case TOKEN_AMPERSAND:     return "AMPERSAND";
        case TOKEN_AMPERSAND_EQUAL: return "AMPERSAND_EQUAL";
        case TOKEN_PIPE:          return "PIPE";
        case TOKEN_PIPE_EQUAL:    return "PIPE_EQUAL";
        case TOKEN_CARET:         return "CARET";
        case TOKEN_CARET_EQUAL:   return "CARET_EQUAL";
        case TOKEN_TILDE:         return "TILDE";
        case TOKEN_LESS_LESS:     return "LESS_LESS";
        case TOKEN_LESS_LESS_EQUAL: return "LESS_LESS_EQUAL";
        case TOKEN_GREATER_GREATER: return "GREATER_GREATER";
        case TOKEN_GREATER_GREATER_EQUAL: return "GREATER_GREATER_EQUAL";
        case TOKEN_EQUALS:        return "EQUALS";
        case TOKEN_EQUAL_EQUAL:   return "EQUAL_EQUAL";
        case TOKEN_BANG_EQUAL:    return "BANG_EQUAL";
        case TOKEN_LESS:          return "LESS";
        case TOKEN_GREATER:       return "GREATER";
        case TOKEN_LESS_EQUAL:    return "LESS_EQUAL";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_AND_AND:       return "AND_AND";
        case TOKEN_OR_OR:         return "OR_OR";
        case TOKEN_BANG:          return "BANG";
        case TOKEN_LPAREN:        return "LPAREN";
        case TOKEN_RPAREN:        return "RPAREN";
        case TOKEN_LBRACE:        return "LBRACE";
        case TOKEN_RBRACE:        return "RBRACE";
        case TOKEN_COLON:         return "COLON";
        case TOKEN_COMMA:         return "COMMA";
        case TOKEN_DOT:           return "DOT";
        case TOKEN_LBRACKET:      return "LBRACKET";
        case TOKEN_RBRACKET:      return "RBRACKET";
        case TOKEN_SEMICOLON:     return "SEMICOLON";
        case TOKEN_DOTDOT:        return "DOTDOT";
        case TOKEN_EOF:           return "EOF";
        case TOKEN_ERROR:         return "ERROR";
        default:                  return "UNKNOWN";
    }
}

/* ================================= Lexer ================================== */

typedef struct {
    const char *source;
    const char *start;
    const char *current;
    size_t line;
    size_t column;
} Lexer;

static void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
}

static Token lexer_make_token(Lexer *lexer, TokenKind kind) {
    Token token;
    token.kind = kind;
    token.start = lexer->start;
    token.length = lexer->current - lexer->start;
    token.line = lexer->line;
    token.column = lexer->column - token.length;
    return token;
}

static Token lexer_error_token(Lexer *lexer, const char *message) {
    Token token;
    token.kind = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

static char lexer_peek(Lexer *lexer) {
    return *lexer->current;
}

static char lexer_peek_next(Lexer *lexer) {
    if (*lexer->current == '\0') return '\0';
    return lexer->current[1];
}

static char lexer_advance(Lexer *lexer) {
    char c = *lexer->current++;
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static void lexer_skip_whitespace_and_comments(Lexer *lexer) {
    for (;;) {
        char c = lexer_peek(lexer);
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                lexer_advance(lexer);
                break;
            case '/':
                if (lexer_peek_next(lexer) == '/') {
                    /* Single-line comment: skip to end of line */
                    while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0') {
                        lexer_advance(lexer);
                    }
                } else if (lexer_peek_next(lexer) == '*') {
                    /* Multi-line comment: skip until */
                    size_t start_line = lexer->line;
                    size_t start_col = lexer->column;
                    lexer_advance(lexer); /* consume / */
                    lexer_advance(lexer); /* consume * */
                    while (!(lexer_peek(lexer) == '*' && lexer_peek_next(lexer) == '/')) {
                        if (lexer_peek(lexer) == '\0') {
                            diagnostic(g_source_file, ERR_L002_UNTERMINATED_COMMENT,
                                       start_line, start_col,
                                       "Unterminated block comment");
                            return;
                        }
                        lexer_advance(lexer);
                    }
                    lexer_advance(lexer); /* consume * */
                    lexer_advance(lexer); /* consume / */
                } else {
                    return; /* Just a division operator */
                }
                break;
            default:
                return;
        }
    }
}

static Token lexer_scan_number(Lexer *lexer) {
    while (is_digit(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    /* Check for decimal point followed by digit */
    if (lexer_peek(lexer) == '.' && is_digit(lexer->current[1])) {
        lexer_advance(lexer);  /* consume '.' */
        while (is_digit(lexer_peek(lexer))) {
            lexer_advance(lexer);
        }
        return lexer_make_token(lexer, TOKEN_FLOAT);
    }
    return lexer_make_token(lexer, TOKEN_NUMBER);
}

static TokenKind lexer_identify_keyword(const char *start, size_t length) {
    switch (length) {
        case 2:
            if (memcmp(start, "if", 2) == 0) return TOKEN_IF;
            if (memcmp(start, "in", 2) == 0) return TOKEN_IN;
            if (memcmp(start, "u8", 2) == 0) return TOKEN_U8;
            break;
        case 3:
            if (memcmp(start, "val", 3) == 0) return TOKEN_VAL;
            if (memcmp(start, "mut", 3) == 0) return TOKEN_MUT;
            if (memcmp(start, "ref", 3) == 0) return TOKEN_REF;
            if (memcmp(start, "for", 3) == 0) return TOKEN_FOR;
            if (memcmp(start, "i64", 3) == 0) return TOKEN_I64;
            if (memcmp(start, "i32", 3) == 0) return TOKEN_I32;
            if (memcmp(start, "u32", 3) == 0) return TOKEN_U32;
            if (memcmp(start, "f64", 3) == 0) return TOKEN_F64;
            if (memcmp(start, "str", 3) == 0) return TOKEN_STR;
            if (memcmp(start, "pub", 3) == 0) return TOKEN_PUB;
            break;
        case 4:
            if (memcmp(start, "func", 4) == 0) return TOKEN_FUNC;
            if (memcmp(start, "void", 4) == 0) return TOKEN_VOID;
            if (memcmp(start, "else", 4) == 0) return TOKEN_ELSE;
            if (memcmp(start, "bool", 4) == 0) return TOKEN_BOOL;
            if (memcmp(start, "true", 4) == 0) return TOKEN_TRUE;
            if (memcmp(start, "enum", 4) == 0) return TOKEN_ENUM;
            break;
        case 5:
            if (memcmp(start, "while", 5) == 0) return TOKEN_WHILE;
            if (memcmp(start, "match", 5) == 0) return TOKEN_MATCH;
            if (memcmp(start, "break", 5) == 0) return TOKEN_BREAK;
            if (memcmp(start, "false", 5) == 0) return TOKEN_FALSE;
            if (memcmp(start, "value", 5) == 0) return TOKEN_VALUE;
            if (memcmp(start, "table", 5) == 0) return TOKEN_TABLE;
            if (memcmp(start, "Arena", 5) == 0) return TOKEN_ARENA;
            break;
        case 6:
            if (memcmp(start, "return", 6) == 0) return TOKEN_RETURN;
            if (memcmp(start, "assert", 6) == 0) return TOKEN_ASSERT;
            if (memcmp(start, "struct", 6) == 0) return TOKEN_STRUCT;
            if (memcmp(start, "import", 6) == 0) return TOKEN_IMPORT;
            if (memcmp(start, "native", 6) == 0) return TOKEN_NATIVE;
            break;
        case 8:
            if (memcmp(start, "continue", 8) == 0) return TOKEN_CONTINUE;
            break;
    }
    return TOKEN_IDENTIFIER;
}

static Token lexer_scan_string(Lexer *lexer) {
    size_t start_line = lexer->line;
    size_t start_col = lexer->column - 1;  /* opening quote already consumed */
    char c;
    while ((c = lexer_peek(lexer)) != '"') {
        if (c == '\0' || c == '\n') {
            diagnostic(g_source_file, ERR_L003_UNTERMINATED_STRING, start_line, start_col,
                       "Unterminated string literal");
            return lexer_make_token(lexer, TOKEN_STRING);
        }
        if (c == '\\') {
            lexer_advance(lexer);  /* consume backslash */
            char esc = lexer_peek(lexer);
            if (!is_valid_escape(esc, '"')) {
                diagnostic(g_source_file, ERR_L004_INVALID_ESCAPE, lexer->line, lexer->column,
                           "Invalid escape sequence '\\%c'", esc);
            }
        }
        lexer_advance(lexer);
    }
    lexer_advance(lexer);  /* consume closing quote */
    return lexer_make_token(lexer, TOKEN_STRING);
}

static Token lexer_scan_char(Lexer *lexer) {
    size_t start_line = lexer->line;
    size_t start_col = lexer->column - 1;  /* opening quote already consumed */
    char c = lexer_peek(lexer);
    if (c == '\'') {
        diagnostic(g_source_file, ERR_L005_EMPTY_CHAR, start_line, start_col,
                   "Empty character literal");
        lexer_advance(lexer);  /* consume closing quote */
        return lexer_make_token(lexer, TOKEN_CHAR);
    }
    if (c == '\0' || c == '\n') {
        diagnostic(g_source_file, ERR_L003_UNTERMINATED_STRING, start_line, start_col,
                   "Unterminated character literal");
        return lexer_make_token(lexer, TOKEN_CHAR);
    }
    if (c == '\\') {
        lexer_advance(lexer);  /* consume backslash */
        char esc = lexer_peek(lexer);
        if (!is_valid_escape(esc, '\'')) {
            diagnostic(g_source_file, ERR_L004_INVALID_ESCAPE, lexer->line, lexer->column,
                       "Invalid escape sequence '\\%c'", esc);
        }
    }
    lexer_advance(lexer);  /* consume the character (or escape char) */
    /* Check for closing quote */
    c = lexer_peek(lexer);
    if (c != '\'') {
        if (c == '\0' || c == '\n') {
            diagnostic(g_source_file, ERR_L003_UNTERMINATED_STRING, start_line, start_col,
                       "Unterminated character literal");
            return lexer_make_token(lexer, TOKEN_CHAR);
        }
        /* Multi-character literal — skip to closing quote or end */
        while ((c = lexer_peek(lexer)) != '\'' && c != '\0' && c != '\n') {
            lexer_advance(lexer);
        }
        diagnostic(g_source_file, ERR_L006_CHAR_TOO_LONG, start_line, start_col,
                   "Character literal must contain exactly one character");
        if (c == '\'') {
            lexer_advance(lexer);  /* consume closing quote */
        }
        return lexer_make_token(lexer, TOKEN_CHAR);
    }
    lexer_advance(lexer);  /* consume closing quote */
    return lexer_make_token(lexer, TOKEN_CHAR);
}

static Token lexer_scan_identifier(Lexer *lexer) {
    while (is_alnum(lexer_peek(lexer)) || lexer_peek(lexer) == '_') {
        lexer_advance(lexer);
    }

    size_t length = lexer->current - lexer->start;
    TokenKind kind = lexer_identify_keyword(lexer->start, length);

    return lexer_make_token(lexer, kind);
}

Token lexer_next_token(Lexer *lexer) {
    lexer_skip_whitespace_and_comments(lexer);

    lexer->start = lexer->current;

    char c = lexer_advance(lexer);

    if (c == '\0') {
        return lexer_make_token(lexer, TOKEN_EOF);
    }

    if (is_digit(c)) {
        return lexer_scan_number(lexer);
    }

    if (is_alpha(c) || c == '_') {
        return lexer_scan_identifier(lexer);
    }

    switch (c) {
        case '"': return lexer_scan_string(lexer);
        case '\'': return lexer_scan_char(lexer);
        case '(': return lexer_make_token(lexer, TOKEN_LPAREN);
        case ')': return lexer_make_token(lexer, TOKEN_RPAREN);
        case '{': return lexer_make_token(lexer, TOKEN_LBRACE);
        case '}': return lexer_make_token(lexer, TOKEN_RBRACE);
        case ':': return lexer_make_token(lexer, TOKEN_COLON);
        case ',': return lexer_make_token(lexer, TOKEN_COMMA);
        case '.':
            if (lexer_peek(lexer) == '.') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_DOTDOT);
            }
            return lexer_make_token(lexer, TOKEN_DOT);
        case '[': return lexer_make_token(lexer, TOKEN_LBRACKET);
        case ']': return lexer_make_token(lexer, TOKEN_RBRACKET);
        case ';': return lexer_make_token(lexer, TOKEN_SEMICOLON);
        case '+':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_PLUS_EQUAL);
            }
            return lexer_make_token(lexer, TOKEN_PLUS);
        case '-':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_MINUS_EQUAL);
            }
            return lexer_make_token(lexer, TOKEN_MINUS);
        case '*':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_STAR_EQUAL);
            }
            return lexer_make_token(lexer, TOKEN_STAR);
        case '/':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_SLASH_EQUAL);
            }
            return lexer_make_token(lexer, TOKEN_SLASH);
        case '%':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_PERCENT_EQUAL);
            }
            return lexer_make_token(lexer, TOKEN_PERCENT);
        case '=':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_EQUAL_EQUAL);
            }
            return lexer_make_token(lexer, TOKEN_EQUALS);
        case '!':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_BANG_EQUAL);
            }
            return lexer_make_token(lexer, TOKEN_BANG);
        case '&':
            if (lexer_peek(lexer) == '&') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_AND_AND);
            }
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_AMPERSAND_EQUAL);
            }
            return lexer_make_token(lexer, TOKEN_AMPERSAND);
        case '|':
            if (lexer_peek(lexer) == '|') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_OR_OR);
            }
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_PIPE_EQUAL);
            }
            return lexer_make_token(lexer, TOKEN_PIPE);
        case '^':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_CARET_EQUAL);
            }
            return lexer_make_token(lexer, TOKEN_CARET);
        case '~': return lexer_make_token(lexer, TOKEN_TILDE);
        case '<':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_LESS_EQUAL);
            }
            if (lexer_peek(lexer) == '<') {
                lexer_advance(lexer);
                if (lexer_peek(lexer) == '=') {
                    lexer_advance(lexer);
                    return lexer_make_token(lexer, TOKEN_LESS_LESS_EQUAL);
                }
                return lexer_make_token(lexer, TOKEN_LESS_LESS);
            }
            return lexer_make_token(lexer, TOKEN_LESS);
        case '>':
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return lexer_make_token(lexer, TOKEN_GREATER_EQUAL);
            }
            if (lexer_peek(lexer) == '>') {
                lexer_advance(lexer);
                if (lexer_peek(lexer) == '=') {
                    lexer_advance(lexer);
                    return lexer_make_token(lexer, TOKEN_GREATER_GREATER_EQUAL);
                }
                return lexer_make_token(lexer, TOKEN_GREATER_GREATER);
            }
            return lexer_make_token(lexer, TOKEN_GREATER);
    }

    return lexer_error_token(lexer, "Unexpected character");
}

static void lexer_print_tokens(Lexer *lexer, const char *source) {
    printf("=== TOKENS ===\n");
    Token token;
    do {
        token = lexer_next_token(lexer);
        printf("%-20s", token_kind_name(token.kind));
        if (token.kind == TOKEN_NUMBER || token.kind == TOKEN_FLOAT ||
            token.kind == TOKEN_IDENTIFIER || token.kind == TOKEN_STRING ||
            token.kind == TOKEN_CHAR) {
            printf(" '%.*s'", (int)token.length, token.start);
        }
        printf("\n");
    } while (token.kind != TOKEN_EOF);
    printf("\n");

    /* Reinitialize lexer for potential next phase */
    lexer_init(lexer, source);
}

/* ============================== Token Location ============================ */

static SourceLoc token_loc(Token *token) {
    return (SourceLoc){ .line = token->line, .column = token->column, .file = g_source_file };
}

/* ================================= Types ================================== */

typedef enum {
    /* Sentinel for unspecified type */
    TYPE_UNKNOWN = -1,
    /* Concrete types */
    TYPE_I64,
    TYPE_I32,
    TYPE_U8,
    TYPE_U32,
    TYPE_F64,
    TYPE_BOOL,
    TYPE_VOID,
    TYPE_ARENA,
    /* Comptime types (literals only) */
    TYPE_COMPTIME_INT,
    TYPE_COMPTIME_FLOAT,
    /* User-defined value types start at this offset */
    TYPE_VALUE_BASE = 16,
    /* Array types start at this offset */
    TYPE_ARRAY_BASE = 1024,
    /* Slice types start at this offset */
    TYPE_SLICE_BASE = 2048,
    /* Enum types start at this offset */
    TYPE_ENUM_BASE = 3072,
} Type;

/* Forward declaration - defined after ValueTypeTable */
static const char *type_name(Type type);

static bool type_is_comptime(Type type) {
    return type == TYPE_COMPTIME_INT || type == TYPE_COMPTIME_FLOAT;
}

static bool type_is_numeric(Type type) {
    return type == TYPE_I64 || type == TYPE_I32 ||
           type == TYPE_U8 || type == TYPE_U32 ||
           type == TYPE_F64 ||
           type == TYPE_COMPTIME_INT || type == TYPE_COMPTIME_FLOAT;
}

static bool type_is_integer(Type type) {
    return type == TYPE_I64 || type == TYPE_I32 ||
           type == TYPE_U8 || type == TYPE_U32 ||
           type == TYPE_COMPTIME_INT;
}

static bool type_is_unsigned(Type type) {
    return type == TYPE_U8 || type == TYPE_U32;
}

static bool type_is_arena(Type type) {
    return type == TYPE_ARENA;
}

static bool type_is_value(Type type) {
    return type >= TYPE_VALUE_BASE && type < TYPE_ARRAY_BASE;
}

static bool type_is_array(Type type) {
    return type >= TYPE_ARRAY_BASE && type < TYPE_SLICE_BASE;
}

static bool type_is_slice(Type type) {
    return type >= TYPE_SLICE_BASE && type < TYPE_ENUM_BASE;
}

static bool type_is_enum(Type type) {
    return type >= TYPE_ENUM_BASE;
}

static int type_enum_index(Type type) {
    return type - TYPE_ENUM_BASE;
}

static int type_slice_index(Type type) {
    return type - TYPE_SLICE_BASE;
}

static int type_array_index(Type type) {
    return type - TYPE_ARRAY_BASE;
}

static int type_value_index(Type type) {
    return type - TYPE_VALUE_BASE;
}

/* Forward declaration - defined after ArrayTypeTable */
static bool type_can_coerce(Type from, Type to);

/* Resolve result type for numeric binary operation.
 * Returns TYPE_VOID on error (incompatible types). */
static Type resolve_numeric_binary_type(Type left, Type right) {
    /* Both comptime_int */
    if (left == TYPE_COMPTIME_INT && right == TYPE_COMPTIME_INT) {
        return TYPE_COMPTIME_INT;
    }
    /* Both comptime, at least one float */
    if (type_is_comptime(left) && type_is_comptime(right)) {
        return TYPE_COMPTIME_FLOAT;  /* int promotes to float */
    }
    /* comptime_int + concrete numeric → concrete */
    if (left == TYPE_COMPTIME_INT && type_is_numeric(right) && !type_is_comptime(right)) {
        return right;
    }
    if (right == TYPE_COMPTIME_INT && type_is_numeric(left) && !type_is_comptime(left)) {
        return left;
    }
    /* comptime_float + f64 → f64 */
    if (left == TYPE_COMPTIME_FLOAT && right == TYPE_F64) {
        return TYPE_F64;
    }
    if (right == TYPE_COMPTIME_FLOAT && left == TYPE_F64) {
        return TYPE_F64;
    }
    /* comptime_float + integer → error */
    if ((left == TYPE_COMPTIME_FLOAT && type_is_integer(right)) ||
        (right == TYPE_COMPTIME_FLOAT && type_is_integer(left))) {
        return TYPE_VOID;  /* error */
    }
    /* Both same concrete numeric type */
    if (left == right && type_is_numeric(left)) {
        return left;
    }
    /* Mixed concrete types → error */
    return TYPE_VOID;
}

/* Resolve common type for if/else branches.
 * Returns TYPE_VOID if types are incompatible. */
static Type resolve_branch_types(Type then_type, Type else_type) {
    /* Same type (includes both-void case) */
    if (then_type == else_type) {
        return then_type;
    }
    /* comptime_int can coerce to any numeric */
    if (then_type == TYPE_COMPTIME_INT && type_is_numeric(else_type)) {
        return else_type;
    }
    if (else_type == TYPE_COMPTIME_INT && type_is_numeric(then_type)) {
        return then_type;
    }
    /* comptime_float can coerce to f64 */
    if (then_type == TYPE_COMPTIME_FLOAT && else_type == TYPE_F64) {
        return TYPE_F64;
    }
    if (else_type == TYPE_COMPTIME_FLOAT && then_type == TYPE_F64) {
        return TYPE_F64;
    }
    /* Both comptime, mixed int/float → comptime_float */
    if (type_is_comptime(then_type) && type_is_comptime(else_type)) {
        return TYPE_COMPTIME_FLOAT;
    }
    /* Incompatible */
    return TYPE_VOID;
}

/* ================================== Params ================================ */

typedef struct {
    const char *name_start;
    size_t name_length;
    Type type;
    bool is_ref;
    bool is_mut_ref;
} Parameter;

/* ============================== Value Type Table =========================== */

typedef struct {
    const char *name_start;
    size_t name_length;
    Parameter *fields;
    size_t field_count;
    SourceLoc loc;
    bool is_struct;
    bool is_public;
    size_t module_id;
} ValueTypeEntry;

typedef struct {
    ValueTypeEntry *types;
    size_t count;
    size_t capacity;
} ValueTypeTable;

static ValueTypeTable *g_value_table = NULL;

static ValueTypeTable *value_table_create(void) {
    ValueTypeTable *table = malloc(sizeof(ValueTypeTable));
    if (!table) panic(ERR_I001_OUT_OF_MEMORY, "Failed to allocate value type table");
    table->capacity = 8;
    table->count = 0;
    table->types = malloc(table->capacity * sizeof(ValueTypeEntry));
    if (!table->types) panic(ERR_I001_OUT_OF_MEMORY, "Failed to allocate value type entries");
    return table;
}

static void value_table_destroy(ValueTypeTable *table) {
    if (!table) return;
    for (size_t i = 0; i < table->count; i++) {
        free(table->types[i].fields);
    }
    free(table->types);
    free(table);
}

static ValueTypeEntry *value_table_lookup(ValueTypeTable *table,
                                           const char *name, size_t length) {
    for (size_t i = 0; i < table->count; i++) {
        if (table->types[i].name_length == length &&
            memcmp(table->types[i].name_start, name, length) == 0) {
            return &table->types[i];
        }
    }
    return NULL;
}

static ValueTypeEntry *value_table_lookup_in_module(ValueTypeTable *table,
                                                     const char *name, size_t length,
                                                     size_t module_id) {
    for (size_t i = 0; i < table->count; i++) {
        if (table->types[i].module_id == module_id &&
            table->types[i].name_length == length &&
            memcmp(table->types[i].name_start, name, length) == 0) {
            return &table->types[i];
        }
    }
    return NULL;
}

/* Look up value type visible in the current module (own module or built-in) */
static ValueTypeEntry *value_table_lookup_local(ValueTypeTable *table,
                                                 const char *name, size_t length,
                                                 size_t current_module) {
    for (size_t i = 0; i < table->count; i++) {
        if (table->types[i].name_length == length &&
            memcmp(table->types[i].name_start, name, length) == 0 &&
            (table->types[i].module_id == current_module ||
             table->types[i].module_id == MODULE_BUILTIN)) {
            return &table->types[i];
        }
    }
    return NULL;
}

static int value_table_find_field(ValueTypeEntry *entry,
                                   const char *name, size_t length) {
    for (size_t i = 0; i < entry->field_count; i++) {
        if (entry->fields[i].name_length == length &&
            memcmp(entry->fields[i].name_start, name, length) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/* Get value type entry from a Type, or NULL if not a valid value type */
static ValueTypeEntry *value_table_get(Type type) {
    if (!type_is_value(type) || !g_value_table) return NULL;
    int idx = type_value_index(type);
    if (idx < 0 || (size_t)idx >= g_value_table->count) return NULL;
    return &g_value_table->types[idx];
}

/* Register a new value/struct type, copying the fields array. Returns the assigned Type. */
static Type value_table_add(const char *name_start, size_t name_length,
                             Parameter *fields, size_t field_count,
                             SourceLoc loc, bool is_struct, bool is_public) {
    if (g_value_table->count >= g_value_table->capacity) {
        g_value_table->capacity *= 2;
        g_value_table->types = realloc(g_value_table->types,
                                        g_value_table->capacity * sizeof(ValueTypeEntry));
        if (!g_value_table->types) panic(ERR_I001_OUT_OF_MEMORY, "growing value type table");
    }
    Parameter *owned_fields = malloc(field_count * sizeof(Parameter));
    if (!owned_fields && field_count > 0) panic(ERR_I001_OUT_OF_MEMORY, "copying value type fields");
    memcpy(owned_fields, fields, field_count * sizeof(Parameter));

    ValueTypeEntry *entry = &g_value_table->types[g_value_table->count];
    entry->name_start = name_start;
    entry->name_length = name_length;
    entry->fields = owned_fields;
    entry->field_count = field_count;
    entry->loc = loc;
    entry->is_struct = is_struct;
    entry->is_public = is_public;
    entry->module_id = g_current_module_id;
    return (Type)(TYPE_VALUE_BASE + (int)g_value_table->count++);
}

static bool type_is_struct(Type type) {
    if (type == TYPE_ARENA) return true;
    ValueTypeEntry *vt = value_table_get(type);
    return vt && vt->is_struct;
}

/* ============================== Array Type Table =========================== */

typedef struct {
    Type element_type;
    size_t size;
} ArrayTypeEntry;

typedef struct {
    ArrayTypeEntry *types;
    size_t count;
    size_t capacity;
} ArrayTypeTable;

static ArrayTypeTable *g_array_table = NULL;

static ArrayTypeTable *array_table_create(void) {
    ArrayTypeTable *table = malloc(sizeof(ArrayTypeTable));
    if (!table) panic(ERR_I001_OUT_OF_MEMORY, "Failed to allocate array type table");
    table->capacity = 8;
    table->count = 0;
    table->types = malloc(table->capacity * sizeof(ArrayTypeEntry));
    if (!table->types) panic(ERR_I001_OUT_OF_MEMORY, "Failed to allocate array type entries");
    return table;
}

static void array_table_destroy(ArrayTypeTable *table) {
    if (!table) return;
    free(table->types);
    free(table);
}

static ArrayTypeEntry *array_table_get(Type type) {
    if (!type_is_array(type) || !g_array_table) return NULL;
    int idx = type_array_index(type);
    if (idx < 0 || (size_t)idx >= g_array_table->count) return NULL;
    return &g_array_table->types[idx];
}

/* Find-or-create an array type. Returns the Type. */
static Type array_table_intern(Type element_type, size_t size) {
    /* Search for existing entry */
    for (size_t i = 0; i < g_array_table->count; i++) {
        if (g_array_table->types[i].element_type == element_type &&
            g_array_table->types[i].size == size) {
            return (Type)(TYPE_ARRAY_BASE + (int)i);
        }
    }
    /* Create new entry */
    if (g_array_table->count >= g_array_table->capacity) {
        g_array_table->capacity *= 2;
        g_array_table->types = realloc(g_array_table->types,
                                        g_array_table->capacity * sizeof(ArrayTypeEntry));
        if (!g_array_table->types) panic(ERR_I001_OUT_OF_MEMORY, "growing array type table");
    }
    ArrayTypeEntry *entry = &g_array_table->types[g_array_table->count];
    entry->element_type = element_type;
    entry->size = size;
    return (Type)(TYPE_ARRAY_BASE + (int)g_array_table->count++);
}

/* ============================== Slice Type Table =========================== */

typedef struct { Type element_type; } SliceTypeEntry;

typedef struct {
    SliceTypeEntry *types;
    size_t count;
    size_t capacity;
} SliceTypeTable;

static SliceTypeTable *g_slice_table = NULL;
static bool g_has_arena = false;
static bool g_has_casts = false;
static bool g_has_io = false;
static bool g_has_mem = false;
static bool g_has_args = false;
static bool g_has_time = false;
static bool g_has_driver = false;
static bool g_require_main = true;

/* Native declaration table (separate from function table to avoid name collisions) */
typedef struct {
    const char *name_start;
    size_t name_length;
    size_t module_id;
    SourceLoc loc;
    Parameter *params;
    size_t param_count;
    Type return_type;
} NativeEntry;

#define MAX_NATIVES 64
static NativeEntry g_native_table[MAX_NATIVES];
static size_t g_native_count = 0;

/* Known native function names (compiler-provided implementations) */
static bool is_known_native(const char *name, size_t length) {
    static const struct { const char *name; size_t len; } known[] = {
        {"fd_write", 8}, {"fd_read", 7}, {"fd_open", 7},
        {"fd_close", 8}, {"fd_seek", 7}, {"mem_copy", 8}, {"exit", 4},
        {"args", 4}, {"shell_run", 9}, {"exe_dir", 7}, {"pid", 3},
        {"time_ns", 7}
    };
    for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); i++) {
        if (length == known[i].len && memcmp(name, known[i].name, length) == 0)
            return true;
    }
    return false;
}

/* Look up native declaration in a specific module */
static NativeEntry *native_table_lookup(const char *name, size_t length,
                                        size_t module_id) {
    for (size_t i = 0; i < g_native_count; i++) {
        if (g_native_table[i].name_length == length &&
            memcmp(g_native_table[i].name_start, name, length) == 0 &&
            g_native_table[i].module_id == module_id) {
            return &g_native_table[i];
        }
    }
    return NULL;
}

/* Deferred arena escape checks — processed after all functions are typechecked */
typedef struct {
    SourceLoc loc;
    const char *callee_name_start;
    size_t callee_name_length;
    const char *arena_name_start;
    size_t arena_name_length;
} DeferredArenaCheck;

static DeferredArenaCheck *g_deferred_arena_checks = NULL;
static size_t g_deferred_arena_count = 0;
static size_t g_deferred_arena_capacity = 0;

static void deferred_arena_check_add(SourceLoc loc,
                                     const char *callee_start, size_t callee_len,
                                     const char *arena_start, size_t arena_len) {
    if (g_deferred_arena_count >= g_deferred_arena_capacity) {
        g_deferred_arena_capacity = g_deferred_arena_capacity ? g_deferred_arena_capacity * 2 : 8;
        g_deferred_arena_checks = realloc(g_deferred_arena_checks,
                                          g_deferred_arena_capacity * sizeof(DeferredArenaCheck));
        if (!g_deferred_arena_checks) panic(ERR_I001_OUT_OF_MEMORY, "growing deferred arena checks");
    }
    DeferredArenaCheck *c = &g_deferred_arena_checks[g_deferred_arena_count++];
    c->loc = loc;
    c->callee_name_start = callee_start;
    c->callee_name_length = callee_len;
    c->arena_name_start = arena_start;
    c->arena_name_length = arena_len;
}

/* Dependency records for transitive returns_arena_slices propagation */
typedef struct {
    const char *func_name_start;
    size_t func_name_length;
    const char *callee_name_start;
    size_t callee_name_length;
} ArenaDependency;

static ArenaDependency *g_arena_deps = NULL;
static size_t g_arena_dep_count = 0;
static size_t g_arena_dep_capacity = 0;

static void arena_dep_add(const char *func_start, size_t func_len,
                          const char *callee_start, size_t callee_len) {
    if (g_arena_dep_count >= g_arena_dep_capacity) {
        g_arena_dep_capacity = g_arena_dep_capacity ? g_arena_dep_capacity * 2 : 8;
        g_arena_deps = realloc(g_arena_deps,
                               g_arena_dep_capacity * sizeof(ArenaDependency));
        if (!g_arena_deps) panic(ERR_I001_OUT_OF_MEMORY, "growing arena dependencies");
    }
    ArenaDependency *d = &g_arena_deps[g_arena_dep_count++];
    d->func_name_start = func_start;
    d->func_name_length = func_len;
    d->callee_name_start = callee_start;
    d->callee_name_length = callee_len;
}

static SliceTypeTable *slice_table_create(void) {
    SliceTypeTable *table = malloc(sizeof(SliceTypeTable));
    if (!table) panic(ERR_I001_OUT_OF_MEMORY, "Failed to allocate slice type table");
    table->capacity = 8;
    table->count = 0;
    table->types = malloc(table->capacity * sizeof(SliceTypeEntry));
    if (!table->types) panic(ERR_I001_OUT_OF_MEMORY, "Failed to allocate slice type entries");
    return table;
}

static void slice_table_destroy(SliceTypeTable *table) {
    if (!table) return;
    free(table->types);
    free(table);
}

static SliceTypeEntry *slice_table_get(Type type) {
    if (!type_is_slice(type) || !g_slice_table) return NULL;
    int idx = type_slice_index(type);
    if (idx < 0 || (size_t)idx >= g_slice_table->count) return NULL;
    return &g_slice_table->types[idx];
}

static Type slice_table_intern(Type element_type) {
    for (size_t i = 0; i < g_slice_table->count; i++) {
        if (g_slice_table->types[i].element_type == element_type) {
            return (Type)(TYPE_SLICE_BASE + (int)i);
        }
    }
    if (g_slice_table->count >= g_slice_table->capacity) {
        g_slice_table->capacity *= 2;
        g_slice_table->types = realloc(g_slice_table->types,
                                        g_slice_table->capacity * sizeof(SliceTypeEntry));
        if (!g_slice_table->types) panic(ERR_I001_OUT_OF_MEMORY, "growing slice type table");
    }
    SliceTypeEntry *entry = &g_slice_table->types[g_slice_table->count];
    entry->element_type = element_type;
    return (Type)(TYPE_SLICE_BASE + (int)g_slice_table->count++);
}

/* =============================== Enum Type Table =========================== */

typedef struct {
    const char *name_start;
    size_t name_length;
    long value;
    Type payload_type;      /* TYPE_VOID if no payload */
} EnumVariant;

typedef struct {
    const char *name_start;
    size_t name_length;
    EnumVariant *variants;
    size_t variant_count;
    SourceLoc loc;
    bool has_data;          /* true if any variant has a payload */
    bool has_slice_payload; /* true if any variant has a slice type */
    bool is_public;
    size_t module_id;
} EnumTypeEntry;

static EnumTypeEntry *g_enum_table = NULL;
static size_t g_enum_count = 0;
static size_t g_enum_capacity = 0;

static Type enum_table_add(const char *name_start, size_t name_length,
                            EnumVariant *variants, size_t variant_count,
                            SourceLoc loc, bool is_public) {
    if (g_enum_count >= g_enum_capacity) {
        g_enum_capacity = g_enum_capacity ? g_enum_capacity * 2 : 8;
        g_enum_table = realloc(g_enum_table,
                               g_enum_capacity * sizeof(EnumTypeEntry));
        if (!g_enum_table) panic(ERR_I001_OUT_OF_MEMORY, "growing enum type table");
    }
    EnumTypeEntry *entry = &g_enum_table[g_enum_count];
    entry->name_start = name_start;
    entry->name_length = name_length;
    entry->variants = variants;
    entry->variant_count = variant_count;
    entry->loc = loc;
    entry->has_data = false;
    entry->has_slice_payload = false;
    entry->is_public = is_public;
    entry->module_id = g_current_module_id;
    for (size_t i = 0; i < variant_count; i++) {
        if (variants[i].payload_type != TYPE_VOID) {
            entry->has_data = true;
            if (type_is_slice(variants[i].payload_type)) {
                entry->has_slice_payload = true;
                break;
            }
        }
    }
    return (Type)(TYPE_ENUM_BASE + (int)g_enum_count++);
}

static EnumTypeEntry *enum_table_get(Type type) {
    if (!type_is_enum(type)) return NULL;
    int idx = type_enum_index(type);
    if (idx < 0 || (size_t)idx >= g_enum_count) return NULL;
    return &g_enum_table[idx];
}

static EnumTypeEntry *enum_table_lookup(const char *name, size_t length) {
    for (size_t i = 0; i < g_enum_count; i++) {
        if (g_enum_table[i].name_length == length &&
            memcmp(g_enum_table[i].name_start, name, length) == 0) {
            return &g_enum_table[i];
        }
    }
    return NULL;
}

static EnumTypeEntry *enum_table_lookup_in_module(const char *name, size_t length,
                                                    size_t module_id) {
    for (size_t i = 0; i < g_enum_count; i++) {
        if (g_enum_table[i].module_id == module_id &&
            g_enum_table[i].name_length == length &&
            memcmp(g_enum_table[i].name_start, name, length) == 0) {
            return &g_enum_table[i];
        }
    }
    return NULL;
}

/* Look up enum visible in the current module (own module or built-in) */
static EnumTypeEntry *enum_table_lookup_local(const char *name, size_t length,
                                               size_t current_module) {
    for (size_t i = 0; i < g_enum_count; i++) {
        if (g_enum_table[i].name_length == length &&
            memcmp(g_enum_table[i].name_start, name, length) == 0 &&
            (g_enum_table[i].module_id == current_module ||
             g_enum_table[i].module_id == MODULE_BUILTIN)) {
            return &g_enum_table[i];
        }
    }
    return NULL;
}

static Type enum_table_type_for(EnumTypeEntry *entry) {
    int idx = (int)(entry - g_enum_table);
    return (Type)(TYPE_ENUM_BASE + idx);
}

static bool enum_has_slice_payload(Type type) {
    EnumTypeEntry *et = enum_table_get(type);
    return et && et->has_slice_payload;
}

static bool type_is_non_copyable(Type type) {
    return type_is_struct(type) || enum_has_slice_payload(type);
}

/* ============================== Table Decl Table =========================== */

typedef struct {
    const char *name_start;    /* "Particles" (points into source) */
    size_t name_length;
    char *row_name;            /* "Particles.Row" (malloc'd) */
    Type struct_type;          /* TYPE_VALUE_BASE + N */
    Type row_type;             /* TYPE_VALUE_BASE + M */
    Parameter *fields;         /* original fields (value-compatible) */
    size_t field_count;
    bool is_public;
    size_t module_id;
} TableDeclEntry;

static TableDeclEntry *g_table_decls = NULL;
static size_t g_table_decl_count = 0;
static size_t g_table_decl_capacity = 0;

static void table_decl_add(const char *name_start, size_t name_length,
                            char *row_name, Type struct_type, Type row_type,
                            Parameter *fields, size_t field_count,
                            bool is_public) {
    if (g_table_decl_count >= g_table_decl_capacity) {
        g_table_decl_capacity = g_table_decl_capacity ? g_table_decl_capacity * 2 : 8;
        g_table_decls = realloc(g_table_decls,
                                g_table_decl_capacity * sizeof(TableDeclEntry));
        if (!g_table_decls) panic(ERR_I001_OUT_OF_MEMORY, "growing table decl table");
    }
    TableDeclEntry *e = &g_table_decls[g_table_decl_count++];
    e->name_start = name_start;
    e->name_length = name_length;
    e->row_name = row_name;
    e->struct_type = struct_type;
    e->row_type = row_type;
    e->fields = fields;
    e->field_count = field_count;
    e->is_public = is_public;
    e->module_id = g_current_module_id;
}

static TableDeclEntry *table_decl_for_type(Type t) {
    for (size_t i = 0; i < g_table_decl_count; i++) {
        if (g_table_decls[i].struct_type == t) return &g_table_decls[i];
    }
    return NULL;
}

static bool is_table_type(Type t) {
    return table_decl_for_type(t) != NULL;
}

/* Get element type for arrays and slices, TYPE_UNKNOWN otherwise */
static Type type_element_type(Type type) {
    if (type_is_array(type)) {
        ArrayTypeEntry *at = array_table_get(type);
        return at ? at->element_type : TYPE_UNKNOWN;
    }
    if (type_is_slice(type)) {
        SliceTypeEntry *se = slice_table_get(type);
        return se ? se->element_type : TYPE_UNKNOWN;
    }
    return TYPE_UNKNOWN;
}

static bool type_is_str(Type type) {
    return type_is_slice(type) && type_element_type(type) == TYPE_U8;
}

/* Check if a type is a byte buffer: []u8 slice or [u8; N] array */
static bool type_is_byte_buffer(Type type) {
    if (type_is_str(type)) return true;
    if (type_is_array(type)) {
        ArrayTypeEntry *ae = array_table_get(type);
        return ae && ae->element_type == TYPE_U8;
    }
    return false;
}

static bool type_can_coerce(Type from, Type to) {
    if (from == to) return true;
    if (from == TYPE_COMPTIME_INT && type_is_numeric(to) && !type_is_comptime(to)) return true;
    if (from == TYPE_COMPTIME_FLOAT && to == TYPE_F64) return true;
    /* Array-to-array coercion: same size, element types coerce */
    if (type_is_array(from) && type_is_array(to)) {
        ArrayTypeEntry *fa = array_table_get(from);
        ArrayTypeEntry *ta = array_table_get(to);
        if (fa && ta && fa->size == ta->size) {
            return type_can_coerce(fa->element_type, ta->element_type);
        }
    }
    /* Array→slice or slice→slice coercion: element types must coerce */
    if (type_is_slice(to) && (type_is_array(from) || type_is_slice(from))) {
        Type from_elem = type_element_type(from);
        Type to_elem = type_element_type(to);
        if (from_elem != TYPE_UNKNOWN && to_elem != TYPE_UNKNOWN) {
            return type_can_coerce(from_elem, to_elem);
        }
    }
    return false;
}

static const char *type_name(Type type) {
    switch (type) {
        case TYPE_UNKNOWN:       return "unknown";
        case TYPE_I64:           return "i64";
        case TYPE_I32:           return "i32";
        case TYPE_U8:            return "u8";
        case TYPE_U32:           return "u32";
        case TYPE_F64:           return "f64";
        case TYPE_BOOL:          return "bool";
        case TYPE_VOID:          return "void";
        case TYPE_COMPTIME_INT:  return "comptime_int";
        case TYPE_COMPTIME_FLOAT: return "comptime_float";
        case TYPE_ARENA:         return "Arena";
        default: {
            /* Rotating buffers: safe when called multiple times in one printf */
            static char bufs[4][64];
            static int buf_idx = 0;
            ValueTypeEntry *vt = value_table_get(type);
            if (vt) {
                char *buf = bufs[buf_idx++ % 4];
                snprintf(buf, 64, "%.*s", (int)vt->name_length, vt->name_start);
                return buf;
            }
            ArrayTypeEntry *at = array_table_get(type);
            if (at) {
                char *buf = bufs[buf_idx++ % 4];
                snprintf(buf, 64, "[%s; %zu]", type_name(at->element_type), at->size);
                return buf;
            }
            SliceTypeEntry *se = slice_table_get(type);
            if (se) {
                char *buf = bufs[buf_idx++ % 4];
                snprintf(buf, 64, "[%s]", type_name(se->element_type));
                return buf;
            }
            EnumTypeEntry *et = enum_table_get(type);
            if (et) {
                char *buf = bufs[buf_idx++ % 4];
                snprintf(buf, 64, "%.*s", (int)et->name_length, et->name_start);
                return buf;
            }
            return "unknown";
        }
    }
}

/* ================================== AST =================================== */

typedef enum {
    AST_NUMBER,
    AST_FLOAT,
    AST_BOOLEAN,
    AST_IDENTIFIER,
    AST_BINARY,
    AST_UNARY,
    AST_FUNC_CALL,
    AST_VAL_DECL,
    AST_MUT_DECL,
    AST_RETURN,
    AST_ASSIGNMENT,
    AST_COMPOUND_ASSIGNMENT,
    AST_ASSERT,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_BREAK,
    AST_CONTINUE,
    AST_BLOCK,
    AST_FUNC_DECL,
    AST_VALUE_DECL,
    AST_VALUE_CONSTRUCTOR,
    AST_FIELD_ACCESS,
    AST_ARRAY_LITERAL,
    AST_INDEX_ACCESS,
    AST_SLICE_ACCESS,
    AST_ARENA_NEW,
    AST_ARENA_ALLOC,
    AST_ARENA_RESET,
    AST_STRING_LITERAL,
    AST_TABLE_ALLOC,
    AST_TABLE_LEN,
    AST_TABLE_GET,
    AST_TABLE_INSERT,
    AST_TYPE_CAST,
    AST_FD_WRITE,
    AST_FD_READ,
    AST_FD_OPEN,
    AST_FD_CLOSE,
    AST_FD_SEEK,
    AST_EXIT,
    AST_MEM_COPY,
    AST_ARGS,
    AST_SHELL_RUN,
    AST_EXE_DIR,
    AST_PID,
    AST_TIME_NS,
    AST_ENUM_DECL,
    AST_ENUM_VARIANT,
    AST_MATCH,
    AST_MATCH_ARM,
    AST_NATIVE_DECL,
    AST_PROGRAM
} AstKind;

typedef enum {
    MATCH_PATTERN_ENUM_VARIANT,
    MATCH_PATTERN_LITERAL,
    MATCH_PATTERN_WILDCARD,
    MATCH_PATTERN_ELSE,
} MatchPatternKind;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE,
    OP_AND,
    OP_OR,
    OP_BITAND,
    OP_BITOR,
    OP_BITXOR,
    OP_SHL,
    OP_SHR
} BinaryOp;

typedef enum {
    OP_NEG,
    OP_NOT,
    OP_BITNOT
} UnaryOp;

typedef struct {
    const char *name_start;
    size_t name_length;
    struct Ast *value;
} FieldInit;

/* Match arms normalize each source pattern into one record for sema/codegen. */
typedef struct {
    MatchPatternKind kind;
    SourceLoc loc;
    const char *variant_name;
    size_t variant_length;
    struct Ast *pattern_expr;
    long case_value;
    bool has_binding;
    const char *binding_name;
    size_t binding_length;
} MatchArmPattern;

typedef struct Ast {
    AstKind kind;
    SourceLoc loc;
    Type expr_type;  /* Filled during type checking */
    union {
        struct {
            long value;
        } number;

        struct {
            double value;
        } float_lit;

        struct {
            bool value;
        } boolean;

        struct {
            const char *start;
            size_t length;
            int module_id;  /* -1 = unqualified, >= 0 = module-qualified */
        } identifier;

        struct {
            BinaryOp op;
            struct Ast *left;
            struct Ast *right;
        } binary;

        struct {
            UnaryOp op;
            struct Ast *operand;
        } unary;

        struct {
            const char *name_start;
            size_t name_length;
            struct Ast **arguments;
            bool *arg_is_ref;
            bool *arg_is_mut_ref;
            size_t arg_count;
            int module_id;  /* -1 = unqualified, >= 0 = module-qualified */
        } func_call;

        struct {
            const char *name_start;
            size_t name_length;
            Type type;
            struct Ast *initializer;
            bool is_public;
        } val_decl;

        struct {
            const char *name_start;
            size_t name_length;
            Type type;
            struct Ast *initializer;
            bool is_public;
        } mut_decl;

        struct {
            struct Ast *value;
        } return_stmt;

        struct {
            struct Ast *target;   /* AST_IDENTIFIER or AST_FIELD_ACCESS */
            struct Ast *value;
        } assignment;

        struct {
            struct Ast *target;
            BinaryOp op;
            struct Ast *value;
        } compound_assignment;

        struct {
            struct Ast *condition;
        } assert_stmt;

        struct {
            struct Ast *condition;
            struct Ast *then_block;
            struct Ast *else_block;  /* NULL if no else */
        } if_stmt;

        struct {
            struct Ast *condition;
            struct Ast *body;
        } while_stmt;

        struct {
            const char *var_start;
            size_t var_length;
            struct Ast *start;
            struct Ast *end;
            struct Ast *body;
        } for_stmt;

        struct {
            struct Ast **statements;
            size_t count;
            size_t capacity;
            struct Ast *value_expr;  /* NULL if block ends with statement (void) */
        } block;

        struct {
            const char *name_start;
            size_t name_length;
            Parameter *params;
            size_t param_count;
            Type return_type;
            struct Ast *body;
            bool is_public;
        } func_decl;

        struct {
            const char *name_start;
            size_t name_length;
            Parameter *fields;
            size_t field_count;
            bool is_struct;
            bool is_public;
        } value_decl;

        struct {
            const char *type_name_start;
            size_t type_name_length;
            FieldInit *fields;
            size_t field_count;
        } value_constructor;

        struct {
            struct Ast *object;
            const char *field_start;
            size_t field_length;
        } field_access;

        struct {
            struct Ast **elements;
            size_t element_count;
        } array_literal;

        struct {
            struct Ast *object;
            struct Ast *index;
        } index_access;

        struct {
            struct Ast *object;
            struct Ast *start;  /* NULL means 0 */
            struct Ast *end;    /* NULL means len */
        } slice_access;

        struct {
            struct Ast *capacity;
        } arena_new;

        struct {
            struct Ast *arena;
            struct Ast *count;
        } arena_alloc;

        struct {
            struct Ast *arena;
        } arena_reset;

        struct {
            const char *start;    /* first char after opening quote */
            size_t raw_length;    /* raw source length between quotes */
            size_t byte_length;   /* actual byte count after escape processing */
        } string_literal;

        struct { struct Ast *arena; struct Ast *count; } table_alloc;
        struct { struct Ast *table; } table_len;
        struct { struct Ast *table; struct Ast *index; } table_get;
        struct { struct Ast *table; struct Ast *row; } table_insert;
        struct { Type target_type; struct Ast *operand; } type_cast;
        struct { struct Ast *fd; struct Ast *data; } fd_write;
        struct { struct Ast *fd; struct Ast *buf; } fd_read;
        struct { struct Ast *path; struct Ast *flags; } fd_open;
        struct { struct Ast *fd; } fd_close;
        struct { struct Ast *fd; struct Ast *offset; struct Ast *whence; } fd_seek;
        struct { struct Ast *code; } exit_call;
        struct { struct Ast *dst; struct Ast *src; } mem_copy;
        struct { struct Ast *arena; } args;
        struct { struct Ast *command; } shell_run;
        struct { struct Ast *arena; } exe_dir;
        struct { int unused; } pid_call;
        struct { int unused; } time_ns_call;

        struct {
            const char *name_start;
            size_t name_length;
            Parameter *params;
            size_t param_count;
            Type return_type;
        } native_decl;

        struct {
            const char *name_start;
            size_t name_length;
            size_t variant_count;
            bool is_public;
        } enum_decl;

        struct {
            Type enum_type;
            long value;
            struct Ast *payload;    /* NULL if no data */
        } enum_variant;

        struct {
            struct Ast *scrutinee;
            struct Ast **arms;
            size_t arm_count;
        } match_expr;

        struct {
            MatchArmPattern *patterns;
            size_t pattern_count;
            const char *binding_name;   /* Effective payload binding for the arm body */
            size_t binding_length;
            Type binding_type;
            struct Ast *body;           /* block */
        } match_arm;

        struct {
            struct Ast **statements;
            size_t count;
            size_t capacity;
        } program;
    } as;
} Ast;

/* ============================== AST Allocators ============================ */

static void ast_free(Ast *node);

static Ast *ast_make_number(long value, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_NUMBER;
    node->loc = loc;
    node->as.number.value = value;
    return node;
}

static Ast *ast_make_float(double value, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_FLOAT;
    node->loc = loc;
    node->as.float_lit.value = value;
    return node;
}

static Ast *ast_make_boolean(bool value, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_BOOLEAN;
    node->loc = loc;
    node->as.boolean.value = value;
    return node;
}

static Ast *ast_make_identifier(const char *start, size_t length, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_IDENTIFIER;
    node->loc = loc;
    node->as.identifier.start = start;
    node->as.identifier.length = length;
    node->as.identifier.module_id = -1;
    return node;
}

static Ast *ast_make_binary(BinaryOp op, Ast *left, Ast *right, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_BINARY;
    node->loc = loc;
    node->as.binary.op = op;
    node->as.binary.left = left;
    node->as.binary.right = right;
    return node;
}

static Ast *ast_make_unary(UnaryOp op, Ast *operand, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_UNARY;
    node->loc = loc;
    node->as.unary.op = op;
    node->as.unary.operand = operand;
    return node;
}

static Ast *ast_make_func_call(const char *name_start, size_t name_length,
                               Ast **arguments, bool *arg_is_ref,
                               bool *arg_is_mut_ref, size_t arg_count,
                               SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_FUNC_CALL;
    node->loc = loc;
    node->as.func_call.name_start = name_start;
    node->as.func_call.name_length = name_length;
    node->as.func_call.arguments = arguments;
    node->as.func_call.arg_is_ref = arg_is_ref;
    node->as.func_call.arg_is_mut_ref = arg_is_mut_ref;
    node->as.func_call.arg_count = arg_count;
    node->as.func_call.module_id = -1;
    return node;
}

static Ast *ast_make_val_decl(const char *name_start, size_t name_length,
                              Type type, Ast *initializer, bool is_public,
                              SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_VAL_DECL;
    node->loc = loc;
    node->as.val_decl.name_start = name_start;
    node->as.val_decl.name_length = name_length;
    node->as.val_decl.type = type;
    node->as.val_decl.initializer = initializer;
    node->as.val_decl.is_public = is_public;
    return node;
}

static Ast *ast_make_mut_decl(const char *name_start, size_t name_length,
                              Type type, Ast *initializer, bool is_public,
                              SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_MUT_DECL;
    node->loc = loc;
    node->as.mut_decl.name_start = name_start;
    node->as.mut_decl.name_length = name_length;
    node->as.mut_decl.type = type;
    node->as.mut_decl.initializer = initializer;
    node->as.mut_decl.is_public = is_public;
    return node;
}

static Ast *ast_make_return(Ast *value, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_RETURN;
    node->loc = loc;
    node->as.return_stmt.value = value;
    return node;
}

static Ast *ast_make_assignment(Ast *target, Ast *value, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_ASSIGNMENT;
    node->loc = loc;
    node->as.assignment.target = target;
    node->as.assignment.value = value;
    return node;
}

static Ast *ast_make_compound_assignment(Ast *target, BinaryOp op, Ast *value,
                                         SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_COMPOUND_ASSIGNMENT;
    node->loc = loc;
    node->as.compound_assignment.target = target;
    node->as.compound_assignment.op = op;
    node->as.compound_assignment.value = value;
    return node;
}

static Ast *ast_make_assert(Ast *condition, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_ASSERT;
    node->loc = loc;
    node->as.assert_stmt.condition = condition;
    return node;
}

static Ast *ast_make_if(Ast *condition, Ast *then_block, Ast *else_block,
                        SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_IF;
    node->loc = loc;
    node->as.if_stmt.condition = condition;
    node->as.if_stmt.then_block = then_block;
    node->as.if_stmt.else_block = else_block;
    return node;
}

static Ast *ast_make_while(Ast *condition, Ast *body, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_WHILE;
    node->loc = loc;
    node->as.while_stmt.condition = condition;
    node->as.while_stmt.body = body;
    return node;
}

static Ast *ast_make_for(const char *var_start, size_t var_length,
                         Ast *start, Ast *end, Ast *body, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_FOR;
    node->loc = loc;
    node->as.for_stmt.var_start = var_start;
    node->as.for_stmt.var_length = var_length;
    node->as.for_stmt.start = start;
    node->as.for_stmt.end = end;
    node->as.for_stmt.body = body;
    return node;
}

static Ast *ast_make_break(SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_BREAK;
    node->loc = loc;
    return node;
}

static Ast *ast_make_continue(SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_CONTINUE;
    node->loc = loc;
    return node;
}

static Ast *ast_make_block(SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_BLOCK;
    node->loc = loc;
    node->as.block.statements = malloc(8 * sizeof(Ast*));
    if (!node->as.block.statements) {
        free(node);
        panic(ERR_I001_OUT_OF_MEMORY, "allocating block statements");
    }
    node->as.block.count = 0;
    node->as.block.capacity = 8;
    node->as.block.value_expr = NULL;
    return node;
}

static void ast_block_add_statement(Ast *block, Ast *statement) {
    if (block->kind != AST_BLOCK) {
        panic(ERR_I002_INTERNAL_ERROR,
              "ast_block_add_statement called on non-block node");
    }

    if (block->as.block.count >= block->as.block.capacity) {
        size_t new_capacity = block->as.block.capacity * 2;
        Ast **new_statements = realloc(block->as.block.statements,
                                       new_capacity * sizeof(Ast*));
        if (!new_statements) {
            panic(ERR_I001_OUT_OF_MEMORY, "growing block statements");
        }
        block->as.block.statements = new_statements;
        block->as.block.capacity = new_capacity;
    }

    block->as.block.statements[block->as.block.count++] = statement;
}

static Ast *ast_make_enum_decl(const char *name_start, size_t name_length,
                               size_t variant_count, bool is_public,
                               SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_ENUM_DECL;
    node->loc = loc;
    node->as.enum_decl.name_start = name_start;
    node->as.enum_decl.name_length = name_length;
    node->as.enum_decl.variant_count = variant_count;
    node->as.enum_decl.is_public = is_public;
    return node;
}

static Ast *ast_make_enum_variant(Type enum_type, long value, Ast *payload,
                                   SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_ENUM_VARIANT;
    node->loc = loc;
    node->as.enum_variant.enum_type = enum_type;
    node->as.enum_variant.value = value;
    node->as.enum_variant.payload = payload;
    return node;
}

static Ast *ast_make_match(Ast *scrutinee, Ast **arms, size_t arm_count,
                            SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_MATCH;
    node->loc = loc;
    node->as.match_expr.scrutinee = scrutinee;
    node->as.match_expr.arms = arms;
    node->as.match_expr.arm_count = arm_count;
    return node;
}

/* Match-pattern ownership stays on the arm, so free them in one place. */
static void match_arm_patterns_free(MatchArmPattern *patterns,
                                    size_t pattern_count) {
    if (!patterns) return;

    for (size_t i = 0; i < pattern_count; i++) {
        if (patterns[i].pattern_expr) {
            ast_free(patterns[i].pattern_expr);
        }
    }

    free(patterns);
}

/* Build a match arm; caller transfers the parsed pattern list. */
static Ast *ast_make_match_arm(MatchArmPattern *patterns, size_t pattern_count,
                               Ast *body, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_MATCH_ARM;
    node->loc = loc;
    memset(&node->as.match_arm, 0, sizeof(node->as.match_arm));
    node->as.match_arm.patterns = patterns;
    node->as.match_arm.pattern_count = pattern_count;
    node->as.match_arm.binding_type = TYPE_VOID;
    node->as.match_arm.body = body;
    return node;
}

static Ast *ast_make_program(void) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_PROGRAM;
    node->loc = (SourceLoc){ .line = 0, .column = 0, .file = g_source_file };
    node->as.program.statements = malloc(8 * sizeof(Ast*));
    if (!node->as.program.statements) {
        free(node);
        panic(ERR_I001_OUT_OF_MEMORY, "allocating program statements");
    }
    node->as.program.count = 0;
    node->as.program.capacity = 8;
    return node;
}

static void ast_program_add_statement(Ast *program, Ast *statement) {
    if (program->kind != AST_PROGRAM) {
        panic(ERR_I002_INTERNAL_ERROR,
              "ast_program_add_statement called on non-program node");
    }

    if (program->as.program.count >= program->as.program.capacity) {
        size_t new_capacity = program->as.program.capacity * 2;
        Ast **new_statements = realloc(program->as.program.statements,
                                       new_capacity * sizeof(Ast*));
        if (!new_statements) {
            panic(ERR_I001_OUT_OF_MEMORY, "growing program statements");
        }
        program->as.program.statements = new_statements;
        program->as.program.capacity = new_capacity;
    }

    program->as.program.statements[program->as.program.count++] = statement;
}

static Ast *ast_make_func_decl(const char *name_start, size_t name_length,
                               Parameter *params, size_t param_count,
                               Type return_type, Ast *body, bool is_public,
                               SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    }
    node->kind = AST_FUNC_DECL;
    node->loc = loc;
    node->as.func_decl.name_start = name_start;
    node->as.func_decl.name_length = name_length;
    node->as.func_decl.params = params;
    node->as.func_decl.param_count = param_count;
    node->as.func_decl.return_type = return_type;
    node->as.func_decl.body = body;
    node->as.func_decl.is_public = is_public;
    return node;
}

static Ast *ast_make_native_decl(const char *name_start, size_t name_length,
                                 Parameter *params, size_t param_count,
                                 Type return_type, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_NATIVE_DECL;
    node->loc = loc;
    node->as.native_decl.name_start = name_start;
    node->as.native_decl.name_length = name_length;
    node->as.native_decl.params = params;
    node->as.native_decl.param_count = param_count;
    node->as.native_decl.return_type = return_type;
    return node;
}

static Ast *ast_make_value_decl(const char *name_start, size_t name_length,
                                 Parameter *fields, size_t field_count,
                                 bool is_struct, bool is_public,
                                 SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_VALUE_DECL;
    node->loc = loc;
    node->as.value_decl.name_start = name_start;
    node->as.value_decl.name_length = name_length;
    node->as.value_decl.fields = fields;
    node->as.value_decl.field_count = field_count;
    node->as.value_decl.is_struct = is_struct;
    node->as.value_decl.is_public = is_public;
    return node;
}

static Ast *ast_make_value_constructor(const char *type_name_start,
                                        size_t type_name_length,
                                        FieldInit *fields, size_t field_count,
                                        SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_VALUE_CONSTRUCTOR;
    node->loc = loc;
    node->as.value_constructor.type_name_start = type_name_start;
    node->as.value_constructor.type_name_length = type_name_length;
    node->as.value_constructor.fields = fields;
    node->as.value_constructor.field_count = field_count;
    return node;
}

static Ast *ast_make_field_access(Ast *object, const char *field_start,
                                   size_t field_length, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_FIELD_ACCESS;
    node->loc = loc;
    node->as.field_access.object = object;
    node->as.field_access.field_start = field_start;
    node->as.field_access.field_length = field_length;
    return node;
}

static Ast *ast_make_array_literal(Ast **elements, size_t element_count,
                                    SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_ARRAY_LITERAL;
    node->loc = loc;
    node->as.array_literal.elements = elements;
    node->as.array_literal.element_count = element_count;
    return node;
}

static Ast *ast_make_index_access(Ast *object, Ast *index, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_INDEX_ACCESS;
    node->loc = loc;
    node->as.index_access.object = object;
    node->as.index_access.index = index;
    return node;
}

static Ast *ast_make_slice_access(Ast *object, Ast *start, Ast *end, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_SLICE_ACCESS;
    node->loc = loc;
    node->as.slice_access.object = object;
    node->as.slice_access.start = start;
    node->as.slice_access.end = end;
    return node;
}

static Ast *ast_make_arena_new(Ast *capacity, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_ARENA_NEW;
    node->loc = loc;
    node->as.arena_new.capacity = capacity;
    return node;
}

static Ast *ast_make_arena_alloc(Ast *arena, Ast *count, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_ARENA_ALLOC;
    node->loc = loc;
    node->as.arena_alloc.arena = arena;
    node->as.arena_alloc.count = count;
    return node;
}

static Ast *ast_make_arena_reset(Ast *arena, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_ARENA_RESET;
    node->loc = loc;
    node->as.arena_reset.arena = arena;
    return node;
}

static Ast *ast_make_table_alloc(Ast *arena, Ast *count, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_TABLE_ALLOC;
    node->loc = loc;
    node->as.table_alloc.arena = arena;
    node->as.table_alloc.count = count;
    return node;
}

static Ast *ast_make_table_len(Ast *table, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_TABLE_LEN;
    node->loc = loc;
    node->as.table_len.table = table;
    return node;
}

static Ast *ast_make_table_get(Ast *table, Ast *index, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_TABLE_GET;
    node->loc = loc;
    node->as.table_get.table = table;
    node->as.table_get.index = index;
    return node;
}

static Ast *ast_make_table_insert(Ast *table, Ast *row, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_TABLE_INSERT;
    node->loc = loc;
    node->as.table_insert.table = table;
    node->as.table_insert.row = row;
    return node;
}

static Ast *ast_make_type_cast(Type target, Ast *operand, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_TYPE_CAST;
    node->loc = loc;
    node->as.type_cast.target_type = target;
    node->as.type_cast.operand = operand;
    return node;
}

static Ast *ast_make_fd_write(Ast *fd, Ast *data, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_FD_WRITE;
    node->loc = loc;
    node->as.fd_write.fd = fd;
    node->as.fd_write.data = data;
    return node;
}

static Ast *ast_make_fd_read(Ast *fd, Ast *buf, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_FD_READ;
    node->loc = loc;
    node->as.fd_read.fd = fd;
    node->as.fd_read.buf = buf;
    return node;
}

static Ast *ast_make_fd_open(Ast *path, Ast *flags, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_FD_OPEN;
    node->loc = loc;
    node->as.fd_open.path = path;
    node->as.fd_open.flags = flags;
    return node;
}

static Ast *ast_make_fd_close(Ast *fd, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_FD_CLOSE;
    node->loc = loc;
    node->as.fd_close.fd = fd;
    return node;
}

static Ast *ast_make_fd_seek(Ast *fd, Ast *offset, Ast *whence, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_FD_SEEK;
    node->loc = loc;
    node->as.fd_seek.fd = fd;
    node->as.fd_seek.offset = offset;
    node->as.fd_seek.whence = whence;
    return node;
}

static Ast *ast_make_exit(Ast *code, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_EXIT;
    node->loc = loc;
    node->as.exit_call.code = code;
    return node;
}

static Ast *ast_make_mem_copy(Ast *dst, Ast *src, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_MEM_COPY;
    node->loc = loc;
    node->as.mem_copy.dst = dst;
    node->as.mem_copy.src = src;
    return node;
}

static Ast *ast_make_args(Ast *arena, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_ARGS;
    node->loc = loc;
    node->as.args.arena = arena;
    return node;
}

static Ast *ast_make_shell_run(Ast *command, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_SHELL_RUN;
    node->loc = loc;
    node->as.shell_run.command = command;
    return node;
}

static Ast *ast_make_exe_dir(Ast *arena, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_EXE_DIR;
    node->loc = loc;
    node->as.exe_dir.arena = arena;
    return node;
}

/* Keep pid() seed-local until stage-0 can resolve ordinary native calls. */
static Ast *ast_make_pid(SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_PID;
    node->loc = loc;
    return node;
}

/* Benchmark timing stays seed-local for the same reason as pid(). */
static Ast *ast_make_time_ns(SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_TIME_NS;
    node->loc = loc;
    return node;
}

static size_t compute_string_byte_length(const char *start, size_t raw_len) {
    size_t count = 0;
    for (size_t i = 0; i < raw_len; i++) {
        if (start[i] == '\\' && i + 1 < raw_len) {
            i++;  /* skip escape pair */
        }
        count++;
    }
    return count;
}

static Ast *ast_make_string_literal(const char *start, size_t raw_length,
                                     size_t byte_length, SourceLoc loc) {
    Ast *node = malloc(sizeof(Ast));
    if (!node) panic(ERR_I001_OUT_OF_MEMORY, "allocating AST node");
    node->kind = AST_STRING_LITERAL;
    node->loc = loc;
    node->as.string_literal.start = start;
    node->as.string_literal.raw_length = raw_length;
    node->as.string_literal.byte_length = byte_length;
    return node;
}

static void ast_free(Ast *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_BINARY:
            ast_free(node->as.binary.left);
            ast_free(node->as.binary.right);
            break;
        case AST_UNARY:
            ast_free(node->as.unary.operand);
            break;
        case AST_FUNC_CALL:
            for (size_t i = 0; i < node->as.func_call.arg_count; i++) {
                ast_free(node->as.func_call.arguments[i]);
            }
            free(node->as.func_call.arguments);
            free(node->as.func_call.arg_is_ref);
            free(node->as.func_call.arg_is_mut_ref);
            break;
        case AST_VAL_DECL:
            ast_free(node->as.val_decl.initializer);
            break;
        case AST_MUT_DECL:
            ast_free(node->as.mut_decl.initializer);
            break;
        case AST_RETURN:
            ast_free(node->as.return_stmt.value);
            break;
        case AST_ASSIGNMENT:
            ast_free(node->as.assignment.target);
            ast_free(node->as.assignment.value);
            break;
        case AST_COMPOUND_ASSIGNMENT:
            ast_free(node->as.compound_assignment.target);
            ast_free(node->as.compound_assignment.value);
            break;
        case AST_ASSERT:
            ast_free(node->as.assert_stmt.condition);
            break;
        case AST_IF:
            ast_free(node->as.if_stmt.condition);
            ast_free(node->as.if_stmt.then_block);
            ast_free(node->as.if_stmt.else_block);
            break;
        case AST_WHILE:
            ast_free(node->as.while_stmt.condition);
            ast_free(node->as.while_stmt.body);
            break;
        case AST_FOR:
            ast_free(node->as.for_stmt.start);
            ast_free(node->as.for_stmt.end);
            ast_free(node->as.for_stmt.body);
            break;
        case AST_BLOCK:
            for (size_t i = 0; i < node->as.block.count; i++) {
                ast_free(node->as.block.statements[i]);
            }
            free(node->as.block.statements);
            ast_free(node->as.block.value_expr);
            break;
        case AST_FUNC_DECL:
            free(node->as.func_decl.params);
            ast_free(node->as.func_decl.body);
            break;
        case AST_VALUE_DECL:
            free(node->as.value_decl.fields);
            break;
        case AST_VALUE_CONSTRUCTOR:
            for (size_t i = 0; i < node->as.value_constructor.field_count; i++) {
                ast_free(node->as.value_constructor.fields[i].value);
            }
            free(node->as.value_constructor.fields);
            break;
        case AST_FIELD_ACCESS:
            ast_free(node->as.field_access.object);
            break;
        case AST_ARRAY_LITERAL:
            for (size_t i = 0; i < node->as.array_literal.element_count; i++) {
                ast_free(node->as.array_literal.elements[i]);
            }
            free(node->as.array_literal.elements);
            break;
        case AST_INDEX_ACCESS:
            ast_free(node->as.index_access.object);
            ast_free(node->as.index_access.index);
            break;
        case AST_SLICE_ACCESS:
            ast_free(node->as.slice_access.object);
            if (node->as.slice_access.start) ast_free(node->as.slice_access.start);
            if (node->as.slice_access.end) ast_free(node->as.slice_access.end);
            break;
        case AST_ARENA_NEW:
            ast_free(node->as.arena_new.capacity);
            break;
        case AST_ARENA_ALLOC:
            ast_free(node->as.arena_alloc.arena);
            ast_free(node->as.arena_alloc.count);
            break;
        case AST_ARENA_RESET:
            ast_free(node->as.arena_reset.arena);
            break;
        case AST_TABLE_ALLOC:
            ast_free(node->as.table_alloc.arena);
            ast_free(node->as.table_alloc.count);
            break;
        case AST_TABLE_LEN:
            ast_free(node->as.table_len.table);
            break;
        case AST_TABLE_GET:
            ast_free(node->as.table_get.table);
            ast_free(node->as.table_get.index);
            break;
        case AST_TABLE_INSERT:
            ast_free(node->as.table_insert.table);
            ast_free(node->as.table_insert.row);
            break;
        case AST_TYPE_CAST:
            ast_free(node->as.type_cast.operand);
            break;
        case AST_FD_WRITE:
            ast_free(node->as.fd_write.fd);
            ast_free(node->as.fd_write.data);
            break;
        case AST_FD_READ:
            ast_free(node->as.fd_read.fd);
            ast_free(node->as.fd_read.buf);
            break;
        case AST_FD_OPEN:
            ast_free(node->as.fd_open.path);
            ast_free(node->as.fd_open.flags);
            break;
        case AST_FD_CLOSE:
            ast_free(node->as.fd_close.fd);
            break;
        case AST_FD_SEEK:
            ast_free(node->as.fd_seek.fd);
            ast_free(node->as.fd_seek.offset);
            ast_free(node->as.fd_seek.whence);
            break;
        case AST_EXIT:
            ast_free(node->as.exit_call.code);
            break;
        case AST_MEM_COPY:
            ast_free(node->as.mem_copy.dst);
            ast_free(node->as.mem_copy.src);
            break;
        case AST_ARGS:
            ast_free(node->as.args.arena);
            break;
        case AST_SHELL_RUN:
            ast_free(node->as.shell_run.command);
            break;
        case AST_EXE_DIR:
            ast_free(node->as.exe_dir.arena);
            break;
        case AST_PID:
        case AST_TIME_NS:
            break;
        case AST_ENUM_DECL:
            break;
        case AST_ENUM_VARIANT:
            if (node->as.enum_variant.payload)
                ast_free(node->as.enum_variant.payload);
            break;
        case AST_MATCH:
            ast_free(node->as.match_expr.scrutinee);
            for (size_t i = 0; i < node->as.match_expr.arm_count; i++)
                ast_free(node->as.match_expr.arms[i]);
            free(node->as.match_expr.arms);
            break;
        case AST_MATCH_ARM:
            match_arm_patterns_free(node->as.match_arm.patterns,
                                    node->as.match_arm.pattern_count);
            ast_free(node->as.match_arm.body);
            break;
        case AST_NATIVE_DECL:
            free(node->as.native_decl.params);
            break;
        case AST_PROGRAM:
            for (size_t i = 0; i < node->as.program.count; i++) {
                ast_free(node->as.program.statements[i]);
            }
            free(node->as.program.statements);
            break;
        default:
            break;
    }
    free(node);
}

/* ================================= Parser ================================= */

typedef struct {
    Lexer *lexer;
    Token current;
    Token previous;
} Parser;

static void parser_advance(Parser *parser) {
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);

    /* Handle lexer errors immediately */
    if (parser->current.kind == TOKEN_ERROR) {
        diagnostic(g_source_file, ERR_L001_INVALID_CHAR, parser->current.line,
                   parser->current.column, "%.*s",
                   (int)parser->current.length, parser->current.start);
    }
}

static void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->previous.kind = TOKEN_EOF;
    parser_advance(parser);
}

/* The absolute spelling of i64 min is valid only after unary minus applies. */
static bool token_is_i64_min_abs(const Token *token) {
    return token->kind == TOKEN_NUMBER &&
           token->length == 19 &&
           memcmp(token->start, "9223372036854775808", 19) == 0;
}

static int parser_check(Parser *parser, TokenKind kind) {
    return parser->current.kind == kind;
}

static int parser_match(Parser *parser, TokenKind kind) {
    if (!parser_check(parser, kind)) {
        return 0;
    }
    parser_advance(parser);
    return 1;
}

/* Try to parse ".Suffix" and look up "base.Suffix" in the value table.
 * On success, consumes dot and suffix tokens and returns the entry.
 * On failure, parser state is unchanged and returns NULL. */
static ValueTypeEntry *parser_try_dot_qualified(Parser *parser,
                                                 const char *base_start,
                                                 size_t base_len) {
    if (!parser_check(parser, TOKEN_DOT)) return NULL;
    Token saved_current = parser->current;
    Lexer saved_lexer = *parser->lexer;
    parser_advance(parser);  /* consume '.' */
    if (parser_check(parser, TOKEN_IDENTIFIER)) {
        char buf[128];
        size_t qual_len = base_len + 1 + parser->current.length;
        if (qual_len < sizeof(buf)) {
            memcpy(buf, base_start, base_len);
            buf[base_len] = '.';
            memcpy(buf + base_len + 1, parser->current.start,
                   parser->current.length);
            ValueTypeEntry *vt = value_table_lookup(g_value_table, buf, qual_len);
            if (vt) {
                parser_advance(parser);  /* consume suffix */
                return vt;
            }
        }
    }
    parser->current = saved_current;
    *parser->lexer = saved_lexer;
    return NULL;
}

/* Parse a type token. Returns TYPE_UNKNOWN if no valid type found.
 * If allow_void is true, void is accepted as valid type. */
static Type parser_parse_type(Parser *parser, bool allow_void) {
    if (parser_match(parser, TOKEN_I64)) return TYPE_I64;
    if (parser_match(parser, TOKEN_I32)) return TYPE_I32;
    if (parser_match(parser, TOKEN_U8))  return TYPE_U8;
    if (parser_match(parser, TOKEN_U32)) return TYPE_U32;
    if (parser_match(parser, TOKEN_F64)) return TYPE_F64;
    if (parser_match(parser, TOKEN_BOOL)) return TYPE_BOOL;
    if (parser_match(parser, TOKEN_ARENA)) return TYPE_ARENA;
    if (parser_match(parser, TOKEN_STR)) return slice_table_intern(TYPE_U8);
    if (allow_void && parser_match(parser, TOKEN_VOID)) return TYPE_VOID;
    /* Check for user-defined value types (including dot-qualified Name.Row) */
    if (parser_check(parser, TOKEN_IDENTIFIER)) {
        const char *base_start = parser->current.start;
        size_t base_len = parser->current.length;
        parser_advance(parser);  /* consume identifier */

        /* Check for module-qualified type: alias.TypeName */
        int type_mod = module_lookup_alias(base_start, base_len);
        if (type_mod >= 0 && parser_check(parser, TOKEN_DOT)) {
            parser_advance(parser);  /* consume '.' */
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                diagnostic(g_source_file, ERR_P015_EXPECTED_TYPE, parser->current.line,
                           parser->current.column,
                           "Expected type name after '%.*s.'",
                           (int)base_len, base_start);
                return TYPE_UNKNOWN;
            }
            const char *type_start = parser->current.start;
            size_t type_len = parser->current.length;
            parser_advance(parser);  /* consume type name */

            /* Try module.Table.Row (three-level) */
            ValueTypeEntry *vt = parser_try_dot_qualified(parser, type_start, type_len);
            if (!vt) vt = value_table_lookup_in_module(g_value_table, type_start, type_len,
                                                        (size_t)type_mod);
            if (vt) {
                if (!vt->is_public) {
                    diagnostic(g_source_file, ERR_S084_NOT_PUBLIC_TYPE,
                               parser->previous.line, parser->previous.column,
                               "Type '%.*s' is not public in module '%.*s'",
                               (int)type_len, type_start,
                               (int)base_len, base_start);
                }
                int idx = (int)(vt - g_value_table->types);
                return (Type)(TYPE_VALUE_BASE + idx);
            }
            EnumTypeEntry *et = enum_table_lookup_in_module(type_start, type_len,
                                                             (size_t)type_mod);
            if (et) {
                if (!et->is_public) {
                    diagnostic(g_source_file, ERR_S084_NOT_PUBLIC_TYPE,
                               parser->previous.line, parser->previous.column,
                               "Type '%.*s' is not public in module '%.*s'",
                               (int)type_len, type_start,
                               (int)base_len, base_start);
                }
                return enum_table_type_for(et);
            }
            diagnostic(g_source_file, ERR_P015_EXPECTED_TYPE, parser->previous.line,
                       parser->previous.column,
                       "Unknown type '%.*s' in module '%.*s'",
                       (int)type_len, type_start,
                       (int)base_len, base_start);
            return TYPE_UNKNOWN;
        }

        /* Non-module-qualified: local types only */
        ValueTypeEntry *vt = parser_try_dot_qualified(parser, base_start, base_len);
        if (!vt) vt = value_table_lookup_local(g_value_table, base_start, base_len,
                                                g_current_module_id);
        if (vt) {
            int idx = (int)(vt - g_value_table->types);
            return (Type)(TYPE_VALUE_BASE + idx);
        }
        EnumTypeEntry *et = enum_table_lookup_local(base_start, base_len,
                                                      g_current_module_id);
        if (et) return enum_table_type_for(et);
    }
    /* Array type: [T; N] or Slice type: [T] */
    if (parser_match(parser, TOKEN_LBRACKET)) {
        Type elem_type = parser_parse_type(parser, false);
        if (elem_type == TYPE_UNKNOWN) {
            diagnostic(g_source_file, ERR_P031_EXPECTED_ARRAY_ELEM_TYPE, parser->current.line,
                       parser->current.column,
                       "Expected element type in array/slice type");
            return TYPE_UNKNOWN;
        }
        /* [T] — slice type */
        if (parser_match(parser, TOKEN_RBRACKET)) {
            return slice_table_intern(elem_type);
        }
        if (!parser_match(parser, TOKEN_SEMICOLON)) {
            diagnostic(g_source_file, ERR_P032_EXPECTED_SEMICOLON_ARRAY, parser->current.line,
                       parser->current.column,
                       "Expected ';' or ']' in array/slice type");
            return TYPE_UNKNOWN;
        }
        if (!parser_check(parser, TOKEN_NUMBER)) {
            diagnostic(g_source_file, ERR_P033_EXPECTED_ARRAY_SIZE, parser->current.line,
                       parser->current.column,
                       "Expected array size");
            return TYPE_UNKNOWN;
        }
        /* Parse the size as integer literal */
        char buffer[32];
        size_t len = parser->current.length;
        if (len >= sizeof(buffer)) len = sizeof(buffer) - 1;
        memcpy(buffer, parser->current.start, len);
        buffer[len] = '\0';
        long size_val = strtol(buffer, NULL, 10);
        SourceLoc size_loc = token_loc(&parser->current);
        parser_advance(parser);  /* consume the number */
        if (size_val <= 0) {
            diagnostic(g_source_file, ERR_S031_ARRAY_SIZE_INVALID, size_loc.line,
                       size_loc.column,
                       "Array size must be a positive integer");
            return TYPE_UNKNOWN;
        }
        if (!parser_match(parser, TOKEN_RBRACKET)) {
            diagnostic(g_source_file, ERR_P034_EXPECTED_RBRACKET, parser->current.line,
                       parser->current.column,
                       "Expected ']' to close array type");
            return TYPE_UNKNOWN;
        }
        return array_table_intern(elem_type, (size_t)size_val);
    }
    return TYPE_UNKNOWN;
}

static TokenKind parser_peek_next(Parser *parser) {
    /* Save lexer state */
    Lexer saved = *parser->lexer;
    Token next = lexer_next_token(parser->lexer);
    /* Restore lexer state */
    *parser->lexer = saved;
    return next.kind;
}

/* Panic mode: skip tokens until we reach a synchronization point */
static void parser_synchronize(Parser *parser) {
    while (!parser_check(parser, TOKEN_EOF)) {
        switch (parser->current.kind) {
            case TOKEN_FUNC:
            case TOKEN_VAL:
            case TOKEN_MUT:
            case TOKEN_PUB:
            case TOKEN_RETURN:
            case TOKEN_ASSERT:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_FOR:
            case TOKEN_BREAK:
            case TOKEN_CONTINUE:
            case TOKEN_RBRACE:
                return;
            default:
                parser_advance(parser);
        }
    }
}

/* ============================ Operator Precedence ========================= */

static int parser_get_precedence(TokenKind kind) {
    switch (kind) {
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT:
            return 8;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 7;
        case TOKEN_LESS_LESS:
        case TOKEN_GREATER_GREATER:
            return 6;
        case TOKEN_AMPERSAND:
            return 5;
        case TOKEN_CARET:
            return 4;
        case TOKEN_PIPE:
            return 3;
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_BANG_EQUAL:
        case TOKEN_LESS:
        case TOKEN_GREATER:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER_EQUAL:
            return 2;
        case TOKEN_AND_AND:
            return 1;
        case TOKEN_OR_OR:
            return 0;
        default:
            return -1;
    }
}

static BinaryOp parser_token_to_binary_op(TokenKind kind) {
    switch (kind) {
        case TOKEN_PLUS:           return OP_ADD;
        case TOKEN_MINUS:          return OP_SUB;
        case TOKEN_STAR:           return OP_MUL;
        case TOKEN_SLASH:          return OP_DIV;
        case TOKEN_PERCENT:        return OP_MOD;
        case TOKEN_EQUAL_EQUAL:    return OP_EQ;
        case TOKEN_BANG_EQUAL:     return OP_NEQ;
        case TOKEN_LESS:           return OP_LT;
        case TOKEN_GREATER:        return OP_GT;
        case TOKEN_LESS_EQUAL:     return OP_LE;
        case TOKEN_GREATER_EQUAL:  return OP_GE;
        case TOKEN_AND_AND:        return OP_AND;
        case TOKEN_OR_OR:          return OP_OR;
        case TOKEN_AMPERSAND:      return OP_BITAND;
        case TOKEN_PIPE:           return OP_BITOR;
        case TOKEN_CARET:          return OP_BITXOR;
        case TOKEN_LESS_LESS:      return OP_SHL;
        case TOKEN_GREATER_GREATER: return OP_SHR;
        default:
            fprintf(stderr, "Internal error: not a binary operator\n");
            exit(1);
    }
}

static bool token_is_compound_assignment(TokenKind kind) {
    switch (kind) {
        case TOKEN_PLUS_EQUAL:
        case TOKEN_MINUS_EQUAL:
        case TOKEN_STAR_EQUAL:
        case TOKEN_SLASH_EQUAL:
        case TOKEN_PERCENT_EQUAL:
        case TOKEN_AMPERSAND_EQUAL:
        case TOKEN_PIPE_EQUAL:
        case TOKEN_CARET_EQUAL:
        case TOKEN_LESS_LESS_EQUAL:
        case TOKEN_GREATER_GREATER_EQUAL:
            return true;
        default:
            return false;
    }
}

static BinaryOp parser_token_to_compound_op(TokenKind kind) {
    switch (kind) {
        case TOKEN_PLUS_EQUAL:           return OP_ADD;
        case TOKEN_MINUS_EQUAL:          return OP_SUB;
        case TOKEN_STAR_EQUAL:           return OP_MUL;
        case TOKEN_SLASH_EQUAL:          return OP_DIV;
        case TOKEN_PERCENT_EQUAL:        return OP_MOD;
        case TOKEN_AMPERSAND_EQUAL:      return OP_BITAND;
        case TOKEN_PIPE_EQUAL:           return OP_BITOR;
        case TOKEN_CARET_EQUAL:          return OP_BITXOR;
        case TOKEN_LESS_LESS_EQUAL:      return OP_SHL;
        case TOKEN_GREATER_GREATER_EQUAL: return OP_SHR;
        default:
            fprintf(stderr, "Internal error: not a compound assignment operator\n");
            exit(1);
    }
}

static const char *binary_op_name(BinaryOp op) {
    switch (op) {
        case OP_ADD:    return "ADD";
        case OP_SUB:    return "SUB";
        case OP_MUL:    return "MUL";
        case OP_DIV:    return "DIV";
        case OP_MOD:    return "MOD";
        case OP_EQ:     return "EQ";
        case OP_NEQ:    return "NEQ";
        case OP_LT:     return "LT";
        case OP_GT:     return "GT";
        case OP_LE:     return "LE";
        case OP_GE:     return "GE";
        case OP_AND:    return "AND";
        case OP_OR:     return "OR";
        case OP_BITAND: return "BITAND";
        case OP_BITOR:  return "BITOR";
        case OP_BITXOR: return "BITXOR";
        case OP_SHL:    return "SHL";
        case OP_SHR:    return "SHR";
    }
    return "UNKNOWN";
}

/* ============================= Expression Parsing ========================= */

static Ast *parser_parse_expression(Parser *parser);
static Ast **parser_parse_arg_list(Parser *parser, size_t *count, bool *error,
                                    bool **out_is_ref, bool **out_is_mut_ref);
static Ast *parser_parse_block(Parser *parser);
static Ast *parser_parse_if(Parser *parser);
static Ast *parser_parse_match(Parser *parser);
static bool ast_if_is_value_expr(Ast *node);

static Type token_to_type(TokenKind kind) {
    switch (kind) {
        case TOKEN_I64: return TYPE_I64;
        case TOKEN_I32: return TYPE_I32;
        case TOKEN_U8:  return TYPE_U8;
        case TOKEN_U32: return TYPE_U32;
        case TOKEN_F64: return TYPE_F64;
        default:        return TYPE_UNKNOWN;
    }
}

/* Parse value constructor fields: { field: expr, ... }
 * Assumes '{' has already been consumed. Returns the AST node or NULL on error. */
static Ast *parser_parse_ctor_fields(Parser *parser, const char *type_name,
                                      size_t type_name_len, SourceLoc loc) {
    size_t capacity = 4;
    size_t field_count = 0;
    FieldInit *fields = malloc(capacity * sizeof(FieldInit));
    if (!fields) panic(ERR_I001_OUT_OF_MEMORY, "allocating constructor fields");

    while (!parser_check(parser, TOKEN_RBRACE) &&
           !parser_check(parser, TOKEN_EOF)) {
        if (field_count >= capacity) {
            capacity *= 2;
            fields = realloc(fields, capacity * sizeof(FieldInit));
            if (!fields) panic(ERR_I001_OUT_OF_MEMORY, "growing constructor fields");
        }
        if (!parser_match(parser, TOKEN_IDENTIFIER)) {
            diagnostic(g_source_file, ERR_P028_EXPECTED_FIELD_CTOR,
                       parser->current.line, parser->current.column,
                       "Expected field name in constructor");
            goto ctor_fields_cleanup;
        }
        fields[field_count].name_start = parser->previous.start;
        fields[field_count].name_length = parser->previous.length;
        if (!parser_match(parser, TOKEN_COLON)) {
            diagnostic(g_source_file, ERR_P029_EXPECTED_COLON_CTOR,
                       parser->current.line, parser->current.column,
                       "Expected ':' after field name in constructor");
            goto ctor_fields_cleanup;
        }
        Ast *fval = parser_parse_expression(parser);
        if (!fval) goto ctor_fields_cleanup;
        fields[field_count].value = fval;
        field_count++;
        if (!parser_match(parser, TOKEN_COMMA)) break;
    }
    if (!parser_match(parser, TOKEN_RBRACE)) {
        diagnostic(g_source_file, ERR_P030_EXPECTED_RBRACE_CTOR,
                   parser->current.line, parser->current.column,
                   "Expected '}' to close constructor");
        goto ctor_fields_cleanup;
    }
    return ast_make_value_constructor(type_name, type_name_len,
                                       fields, field_count, loc);

    ctor_fields_cleanup:
    for (size_t i = 0; i < field_count; i++) ast_free(fields[i].value);
    free(fields);
    return NULL;
}

static Ast *parser_parse_primary(Parser *parser) {
    /* Handle type casts: u8(expr), i32(expr), etc. */
    if (parser_match(parser, TOKEN_I64) || parser_match(parser, TOKEN_I32) ||
        parser_match(parser, TOKEN_U8)  || parser_match(parser, TOKEN_U32) ||
        parser_match(parser, TOKEN_F64)) {
        SourceLoc loc = token_loc(&parser->previous);
        Type target = token_to_type(parser->previous.kind);
        if (!parser_match(parser, TOKEN_LPAREN)) {
            diagnostic(g_source_file, ERR_P001_EXPECTED_EXPRESSION, loc.line, loc.column,
                       "Expected '(' after type name in cast expression");
            return NULL;
        }
        Ast *operand = parser_parse_expression(parser);
        if (!operand) return NULL;
        if (!parser_match(parser, TOKEN_RPAREN)) {
            diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                       parser->current.column,
                       "Expected ')' after cast expression");
            ast_free(operand);
            return NULL;
        }
        return ast_make_type_cast(target, operand, loc);
    }

    /* Handle unary minus */
    if (parser_match(parser, TOKEN_MINUS)) {
        SourceLoc loc = token_loc(&parser->previous);
        if (token_is_i64_min_abs(&parser->current)) {
            parser_advance(parser);
            return ast_make_number(LONG_MIN, loc);
        }
        Ast *operand = parser_parse_primary(parser);
        if (!operand) return NULL;
        return ast_make_unary(OP_NEG, operand, loc);
    }

    /* Handle unary NOT */
    if (parser_match(parser, TOKEN_BANG)) {
        SourceLoc loc = token_loc(&parser->previous);
        Ast *operand = parser_parse_primary(parser);
        if (!operand) return NULL;
        return ast_make_unary(OP_NOT, operand, loc);
    }

    /* Handle bitwise NOT */
    if (parser_match(parser, TOKEN_TILDE)) {
        SourceLoc loc = token_loc(&parser->previous);
        Ast *operand = parser_parse_primary(parser);
        if (!operand) return NULL;
        return ast_make_unary(OP_BITNOT, operand, loc);
    }

    if (parser_match(parser, TOKEN_NUMBER)) {
        SourceLoc loc = token_loc(&parser->previous);
        char buffer[32];
        size_t len = parser->previous.length;
        if (len >= sizeof(buffer)) {
            diagnostic(g_source_file, ERR_P006_NUMBER_TOO_LARGE, loc.line, loc.column,
                       "Number literal too large");
            return NULL;
        }
        memcpy(buffer, parser->previous.start, len);
        buffer[len] = '\0';
        errno = 0;
        long value = strtol(buffer, NULL, 10);
        if (errno == ERANGE) {
            diagnostic(g_source_file, ERR_P006_NUMBER_TOO_LARGE, loc.line, loc.column,
                       "Integer literal out of range");
            return ast_make_number(0, loc);
        }
        return ast_make_number(value, loc);
    }

    if (parser_match(parser, TOKEN_CHAR)) {
        SourceLoc loc = token_loc(&parser->previous);
        const char *content = parser->previous.start + 1;  /* skip opening quote */
        long byte_value;
        if (content[0] == '\\') {
            switch (content[1]) {
                case 'n':  byte_value = 10; break;
                case 't':  byte_value = 9;  break;
                case 'r':  byte_value = 13; break;
                case '\\': byte_value = 92; break;
                case '\'': byte_value = 39; break;
                case '0':  byte_value = 0;  break;
                default:   byte_value = (unsigned char)content[1]; break;
            }
        } else {
            byte_value = (unsigned char)content[0];
        }
        return ast_make_number(byte_value, loc);
    }

    if (parser_match(parser, TOKEN_FLOAT)) {
        SourceLoc loc = token_loc(&parser->previous);
        char buffer[64];
        size_t len = parser->previous.length;
        if (len >= sizeof(buffer)) {
            diagnostic(g_source_file, ERR_P006_NUMBER_TOO_LARGE, loc.line, loc.column,
                       "Float literal too large");
            return NULL;
        }
        memcpy(buffer, parser->previous.start, len);
        buffer[len] = '\0';
        double value = strtod(buffer, NULL);
        return ast_make_float(value, loc);
    }

    if (parser_match(parser, TOKEN_TRUE)) {
        SourceLoc loc = token_loc(&parser->previous);
        return ast_make_boolean(true, loc);
    }

    if (parser_match(parser, TOKEN_FALSE)) {
        SourceLoc loc = token_loc(&parser->previous);
        return ast_make_boolean(false, loc);
    }

    if (parser_match(parser, TOKEN_STRING)) {
        SourceLoc loc = token_loc(&parser->previous);
        /* Token spans from opening " to closing " inclusive */
        const char *content = parser->previous.start + 1;  /* skip opening quote */
        size_t raw_length = parser->previous.length - 2;   /* exclude both quotes */
        size_t byte_length = compute_string_byte_length(content, raw_length);
        return ast_make_string_literal(content, raw_length, byte_length, loc);
    }

    if (parser_match(parser, TOKEN_IDENTIFIER)) {
        SourceLoc loc = token_loc(&parser->previous);
        const char *name_start = parser->previous.start;
        size_t name_length = parser->previous.length;

        /* arena(capacity) — built-in arena constructor */
        if (name_length == 5 && memcmp(name_start, "arena", 5) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            Ast *capacity = parser_parse_expression(parser);
            if (!capacity) return NULL;
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after arena capacity");
                ast_free(capacity);
                return NULL;
            }
            return ast_make_arena_new(capacity, loc);
        }

        /* arena_alloc(mut ref arena, count) — built-in arena allocation */
        if (name_length == 11 && memcmp(name_start, "arena_alloc", 11) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            /* Expect 'mut ref' before arena argument */
            if (!parser_match(parser, TOKEN_MUT)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'mut ref' before arena argument in arena_alloc()");
                return NULL;
            }
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' after 'mut' in arena_alloc()");
                return NULL;
            }
            Ast *arena = parser_parse_expression(parser);
            if (!arena) return NULL;
            if (!parser_match(parser, TOKEN_COMMA)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column,
                           "Expected ',' after arena argument in arena_alloc()");
                ast_free(arena);
                return NULL;
            }
            Ast *count = parser_parse_expression(parser);
            if (!count) { ast_free(arena); return NULL; }
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after arena_alloc count");
                ast_free(arena);
                ast_free(count);
                return NULL;
            }
            return ast_make_arena_alloc(arena, count, loc);
        }

        /* arena_reset(mut ref arena) — built-in arena reset */
        if (name_length == 11 && memcmp(name_start, "arena_reset", 11) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            /* Expect 'mut ref' before arena argument */
            if (!parser_match(parser, TOKEN_MUT)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'mut ref' before arena argument in arena_reset()");
                return NULL;
            }
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' after 'mut' in arena_reset()");
                return NULL;
            }
            Ast *arena = parser_parse_expression(parser);
            if (!arena) return NULL;
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after arena_reset arena");
                ast_free(arena);
                return NULL;
            }
            return ast_make_arena_reset(arena, loc);
        }

        /* table_alloc(mut ref arena, count) — built-in table allocation */
        if (name_length == 11 && memcmp(name_start, "table_alloc", 11) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            if (!parser_match(parser, TOKEN_MUT)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'mut ref' before arena argument in table_alloc()");
                return NULL;
            }
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' after 'mut' in table_alloc()");
                return NULL;
            }
            Ast *arena = parser_parse_expression(parser);
            if (!arena) return NULL;
            if (!parser_match(parser, TOKEN_COMMA)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column,
                           "Expected ',' after arena argument in table_alloc()");
                ast_free(arena);
                return NULL;
            }
            Ast *count = parser_parse_expression(parser);
            if (!count) { ast_free(arena); return NULL; }
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after table_alloc count");
                ast_free(arena);
                ast_free(count);
                return NULL;
            }
            return ast_make_table_alloc(arena, count, loc);
        }

        /* table_len(ref table) — built-in table length */
        if (name_length == 9 && memcmp(name_start, "table_len", 9) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' before table argument in table_len()");
                return NULL;
            }
            Ast *table = parser_parse_expression(parser);
            if (!table) return NULL;
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after table_len argument");
                ast_free(table);
                return NULL;
            }
            return ast_make_table_len(table, loc);
        }

        /* table_get(ref table, index) — built-in table row access */
        if (name_length == 9 && memcmp(name_start, "table_get", 9) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' before table argument in table_get()");
                return NULL;
            }
            Ast *table = parser_parse_expression(parser);
            if (!table) return NULL;
            if (!parser_match(parser, TOKEN_COMMA)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column,
                           "Expected ',' after table argument in table_get()");
                ast_free(table);
                return NULL;
            }
            Ast *index = parser_parse_expression(parser);
            if (!index) { ast_free(table); return NULL; }
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after table_get index");
                ast_free(table);
                ast_free(index);
                return NULL;
            }
            return ast_make_table_get(table, index, loc);
        }

        /* table_insert(mut ref table, row) — built-in table row insert */
        if (name_length == 12 && memcmp(name_start, "table_insert", 12) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            if (!parser_match(parser, TOKEN_MUT)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'mut ref' before table argument in table_insert()");
                return NULL;
            }
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' after 'mut' in table_insert()");
                return NULL;
            }
            Ast *table = parser_parse_expression(parser);
            if (!table) return NULL;
            if (!parser_match(parser, TOKEN_COMMA)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column,
                           "Expected ',' after table argument in table_insert()");
                ast_free(table);
                return NULL;
            }
            Ast *row = parser_parse_expression(parser);
            if (!row) { ast_free(table); return NULL; }
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after table_insert row");
                ast_free(table);
                ast_free(row);
                return NULL;
            }
            return ast_make_table_insert(table, row, loc);
        }

        /* fd_write(fd, ref data) — built-in file descriptor write */
        if (name_length == 8 && memcmp(name_start, "fd_write", 8) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            Ast *fd = parser_parse_expression(parser);
            if (!fd) return NULL;
            if (!parser_match(parser, TOKEN_COMMA)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column,
                           "Expected ',' after fd argument in fd_write()");
                ast_free(fd);
                return NULL;
            }
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' before data argument in fd_write()");
                ast_free(fd);
                return NULL;
            }
            Ast *data = parser_parse_expression(parser);
            if (!data) { ast_free(fd); return NULL; }
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after fd_write data");
                ast_free(fd);
                ast_free(data);
                return NULL;
            }
            return ast_make_fd_write(fd, data, loc);
        }

        /* fd_read(fd, mut ref buf) — built-in file descriptor read */
        if (name_length == 7 && memcmp(name_start, "fd_read", 7) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            Ast *fd = parser_parse_expression(parser);
            if (!fd) return NULL;
            if (!parser_match(parser, TOKEN_COMMA)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column,
                           "Expected ',' after fd argument in fd_read()");
                ast_free(fd);
                return NULL;
            }
            if (!parser_match(parser, TOKEN_MUT)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'mut ref' before buffer argument in fd_read()");
                ast_free(fd);
                return NULL;
            }
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' after 'mut' in fd_read()");
                ast_free(fd);
                return NULL;
            }
            Ast *buf = parser_parse_expression(parser);
            if (!buf) { ast_free(fd); return NULL; }
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after fd_read buffer");
                ast_free(fd);
                ast_free(buf);
                return NULL;
            }
            return ast_make_fd_read(fd, buf, loc);
        }

        /* fd_open(ref path, flags) — built-in file open */
        if (name_length == 7 && memcmp(name_start, "fd_open", 7) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' before path argument in fd_open()");
                return NULL;
            }
            Ast *path = parser_parse_expression(parser);
            if (!path) return NULL;
            if (!parser_match(parser, TOKEN_COMMA)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column,
                           "Expected ',' after path argument in fd_open()");
                ast_free(path);
                return NULL;
            }
            Ast *flags = parser_parse_expression(parser);
            if (!flags) { ast_free(path); return NULL; }
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after fd_open flags");
                ast_free(path);
                ast_free(flags);
                return NULL;
            }
            return ast_make_fd_open(path, flags, loc);
        }

        /* fd_close(fd) — built-in file descriptor close */
        if (name_length == 8 && memcmp(name_start, "fd_close", 8) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            Ast *fd = parser_parse_expression(parser);
            if (!fd) return NULL;
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after fd_close fd");
                ast_free(fd);
                return NULL;
            }
            return ast_make_fd_close(fd, loc);
        }

        /* fd_seek(fd, offset, whence) — built-in file descriptor seek */
        if (name_length == 7 && memcmp(name_start, "fd_seek", 7) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            Ast *fd = parser_parse_expression(parser);
            if (!fd) return NULL;
            if (!parser_match(parser, TOKEN_COMMA)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ',' after fd in fd_seek()");
                ast_free(fd);
                return NULL;
            }
            Ast *offset = parser_parse_expression(parser);
            if (!offset) { ast_free(fd); return NULL; }
            if (!parser_match(parser, TOKEN_COMMA)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ',' after offset in fd_seek()");
                ast_free(fd);
                ast_free(offset);
                return NULL;
            }
            Ast *whence = parser_parse_expression(parser);
            if (!whence) { ast_free(fd); ast_free(offset); return NULL; }
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after fd_seek whence");
                ast_free(fd);
                ast_free(offset);
                ast_free(whence);
                return NULL;
            }
            return ast_make_fd_seek(fd, offset, whence, loc);
        }

        /* exit(code) — built-in process exit */
        if (name_length == 4 && memcmp(name_start, "exit", 4) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            Ast *code = parser_parse_expression(parser);
            if (!code) return NULL;
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after exit code");
                ast_free(code);
                return NULL;
            }
            return ast_make_exit(code, loc);
        }

        /* mem_copy(mut ref dst, ref src) — built-in memory copy */
        if (name_length == 8 && memcmp(name_start, "mem_copy", 8) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            if (!parser_match(parser, TOKEN_MUT)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'mut ref' before dst argument in mem_copy()");
                return NULL;
            }
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' after 'mut' in mem_copy()");
                return NULL;
            }
            Ast *dst = parser_parse_expression(parser);
            if (!dst) return NULL;
            if (!parser_match(parser, TOKEN_COMMA)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column,
                           "Expected ',' after dst argument in mem_copy()");
                ast_free(dst);
                return NULL;
            }
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' before src argument in mem_copy()");
                ast_free(dst);
                return NULL;
            }
            Ast *src = parser_parse_expression(parser);
            if (!src) { ast_free(dst); return NULL; }
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after mem_copy src");
                ast_free(dst);
                ast_free(src);
                return NULL;
            }
            return ast_make_mem_copy(dst, src, loc);
        }

        /* args(mut ref arena) — built-in command-line argument access */
        if (name_length == 4 && memcmp(name_start, "args", 4) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            if (!parser_match(parser, TOKEN_MUT)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'mut ref' before arena argument in args()");
                return NULL;
            }
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' after 'mut' in args()");
                return NULL;
            }
            Ast *arena = parser_parse_expression(parser);
            if (!arena) return NULL;
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after args arena");
                ast_free(arena);
                return NULL;
            }
            return ast_make_args(arena, loc);
        }

        /* shell_run(ref command) — built-in shell bridge for the self-hosted driver */
        if (name_length == 9 && memcmp(name_start, "shell_run", 9) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' before command argument in shell_run()");
                return NULL;
            }
            Ast *command = parser_parse_expression(parser);
            if (!command) return NULL;
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after shell_run command");
                ast_free(command);
                return NULL;
            }
            return ast_make_shell_run(command, loc);
        }

        /* exe_dir(mut ref arena) — built-in argv-based compiler-root recovery */
        if (name_length == 7 && memcmp(name_start, "exe_dir", 7) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            if (!parser_match(parser, TOKEN_MUT)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'mut ref' before arena argument in exe_dir()");
                return NULL;
            }
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' after 'mut' in exe_dir()");
                return NULL;
            }
            Ast *arena = parser_parse_expression(parser);
            if (!arena) return NULL;
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after exe_dir arena");
                ast_free(arena);
                return NULL;
            }
            return ast_make_exe_dir(arena, loc);
        }

        /* pid() — built-in per-process identifier for driver temp paths */
        if (name_length == 3 && memcmp(name_start, "pid", 3) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after pid()");
                return NULL;
            }
            return ast_make_pid(loc);
        }

        /* time_ns() — built-in wall-clock helper for benchmark instrumentation */
        if (name_length == 7 && memcmp(name_start, "time_ns", 7) == 0 &&
            parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after time_ns()");
                return NULL;
            }
            return ast_make_time_ns(loc);
        }

        /* Check for module-qualified access: alias.name */
        if (parser_check(parser, TOKEN_DOT)) {
            int mod_id = module_lookup_alias(name_start, name_length);
            if (mod_id >= 0) {
                parser_advance(parser);  /* consume '.' */
                if (!parser_match(parser, TOKEN_IDENTIFIER)) {
                    diagnostic(g_source_file, ERR_P003_EXPECTED_IDENTIFIER,
                               parser->current.line, parser->current.column,
                               "Expected name after '%.*s.'",
                               (int)name_length, name_start);
                    return NULL;
                }
                const char *qual_name = parser->previous.start;
                size_t qual_len = parser->previous.length;
                SourceLoc qual_loc = token_loc(&parser->previous);

                /* Check for module.EnumName.Variant (three-level) */
                if (parser_check(parser, TOKEN_DOT)) {
                    EnumTypeEntry *met = enum_table_lookup_in_module(
                        qual_name, qual_len, (size_t)mod_id);
                    if (met) {
                        if (!met->is_public) {
                            diagnostic(g_source_file, ERR_S084_NOT_PUBLIC_TYPE,
                                       qual_loc.line, qual_loc.column,
                                       "Enum '%.*s' is not public in module '%.*s'",
                                       (int)qual_len, qual_name,
                                       (int)name_length, name_start);
                        }
                        parser_advance(parser);  /* consume '.' */
                        if (!parser_match(parser, TOKEN_IDENTIFIER)) {
                            diagnostic(g_source_file, ERR_P044_EXPECTED_VARIANT_NAME,
                                       parser->current.line, parser->current.column,
                                       "Expected variant name after '%.*s.%.*s.'",
                                       (int)name_length, name_start,
                                       (int)qual_len, qual_name);
                            return NULL;
                        }
                        const char *var_start = parser->previous.start;
                        size_t var_len = parser->previous.length;
                        for (size_t i = 0; i < met->variant_count; i++) {
                            if (met->variants[i].name_length == var_len &&
                                memcmp(met->variants[i].name_start, var_start, var_len) == 0) {
                                Ast *payload = NULL;
                                if (met->variants[i].payload_type != TYPE_VOID) {
                                    if (!parser_match(parser, TOKEN_LPAREN)) {
                                        diagnostic(g_source_file, ERR_S079_MISSING_PAYLOAD,
                                                   parser->current.line, parser->current.column,
                                                   "Variant '%.*s' requires a payload",
                                                   (int)var_len, var_start);
                                        return NULL;
                                    }
                                    payload = parser_parse_expression(parser);
                                    if (!payload) return NULL;
                                    if (!parser_match(parser, TOKEN_RPAREN)) {
                                        diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN,
                                                   parser->current.line, parser->current.column,
                                                   "Expected ')' after variant payload");
                                        ast_free(payload);
                                        return NULL;
                                    }
                                } else if (met->has_data && parser_check(parser, TOKEN_LPAREN)) {
                                    parser_advance(parser);
                                    diagnostic(g_source_file, ERR_S078_UNEXPECTED_PAYLOAD,
                                               parser->previous.line, parser->previous.column,
                                               "Variant '%.*s' does not take a payload",
                                               (int)var_len, var_start);
                                    parser_synchronize(parser);
                                    return NULL;
                                }
                                return ast_make_enum_variant(enum_table_type_for(met),
                                                             met->variants[i].value,
                                                             payload, loc);
                            }
                        }
                        diagnostic(g_source_file, ERR_P045_UNKNOWN_ENUM_VARIANT,
                                   parser->previous.line, parser->previous.column,
                                   "Unknown variant '%.*s' in enum '%.*s.%.*s'",
                                   (int)var_len, var_start,
                                   (int)name_length, name_start,
                                   (int)qual_len, qual_name);
                        return NULL;
                    }
                }

                /* Check for module.func(...) */
                if (parser_check(parser, TOKEN_LPAREN)) {
                    parser_advance(parser);  /* consume '(' */
                    size_t arg_count = 0;
                    bool parse_error = false;
                    bool *arg_is_ref = NULL;
                    bool *arg_is_mut_ref = NULL;
                    Ast **arguments = parser_parse_arg_list(parser, &arg_count,
                                                             &parse_error,
                                                             &arg_is_ref,
                                                             &arg_is_mut_ref);
                    if (parse_error) return NULL;
                    if (!parser_match(parser, TOKEN_RPAREN)) {
                        diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN,
                                   parser->current.line, parser->current.column,
                                   "Expected ')' after arguments");
                        free(arguments); free(arg_is_ref); free(arg_is_mut_ref);
                        return NULL;
                    }
                    Ast *call = ast_make_func_call(qual_name, qual_len, arguments,
                                                    arg_is_ref, arg_is_mut_ref,
                                                    arg_count, loc);
                    call->as.func_call.module_id = mod_id;
                    return call;
                }

                /* Check for module.Type { ... } (value constructor) */
                if (parser_check(parser, TOKEN_LBRACE)) {
                    /* Try dot-qualified module.Table.Row first */
                    ValueTypeEntry *mvt = parser_try_dot_qualified(
                        parser, qual_name, qual_len);
                    if (!mvt) mvt = value_table_lookup_in_module(
                        g_value_table, qual_name, qual_len, (size_t)mod_id);
                    if (mvt) {
                        if (!mvt->is_public) {
                            diagnostic(g_source_file, ERR_S084_NOT_PUBLIC_TYPE,
                                       qual_loc.line, qual_loc.column,
                                       "Type '%.*s' is not public in module '%.*s'",
                                       (int)qual_len, qual_name,
                                       (int)name_length, name_start);
                        }
                        parser_advance(parser);  /* consume '{' */
                        return parser_parse_ctor_fields(parser, mvt->name_start,
                                                         mvt->name_length, loc);
                    }
                }

                /* Module-qualified constant: module.CONSTANT */
                Ast *ident = ast_make_identifier(qual_name, qual_len, loc);
                ident->as.identifier.module_id = mod_id;
                return ident;
            }
        }

        /* Check for enum variant: EnumName.Variant (local module only) */
        if (parser_check(parser, TOKEN_DOT)) {
            EnumTypeEntry *et = enum_table_lookup_local(name_start, name_length,
                                                         g_current_module_id);
            if (et) {
                parser_advance(parser);  /* consume '.' */
                if (!parser_match(parser, TOKEN_IDENTIFIER)) {
                    diagnostic(g_source_file, ERR_P044_EXPECTED_VARIANT_NAME, parser->current.line,
                               parser->current.column,
                               "Expected variant name after '%.*s.'",
                               (int)name_length, name_start);
                    return NULL;
                }
                const char *var_start = parser->previous.start;
                size_t var_len = parser->previous.length;
                for (size_t i = 0; i < et->variant_count; i++) {
                    if (et->variants[i].name_length == var_len &&
                        memcmp(et->variants[i].name_start, var_start, var_len) == 0) {
                        Ast *payload = NULL;
                        if (et->variants[i].payload_type != TYPE_VOID) {
                            /* Data variant: parse (expr) */
                            if (!parser_match(parser, TOKEN_LPAREN)) {
                                diagnostic(g_source_file, ERR_S079_MISSING_PAYLOAD,
                                           parser->current.line, parser->current.column,
                                           "Variant '%.*s' requires a payload of type %s",
                                           (int)var_len, var_start,
                                           type_name(et->variants[i].payload_type));
                                return NULL;
                            }
                            payload = parser_parse_expression(parser);
                            if (!payload) return NULL;
                            if (!parser_match(parser, TOKEN_RPAREN)) {
                                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN,
                                           parser->current.line, parser->current.column,
                                           "Expected ')' after variant payload");
                                ast_free(payload);
                                return NULL;
                            }
                        } else if (et->has_data && parser_check(parser, TOKEN_LPAREN)) {
                            /* Non-data variant in tagged enum: reject (expr) */
                            parser_advance(parser); /* consume '(' */
                            diagnostic(g_source_file, ERR_S078_UNEXPECTED_PAYLOAD,
                                       parser->previous.line, parser->previous.column,
                                       "Variant '%.*s' does not take a payload",
                                       (int)var_len, var_start);
                            parser_synchronize(parser);
                            return NULL;
                        }
                        return ast_make_enum_variant(enum_table_type_for(et),
                                                     et->variants[i].value,
                                                     payload, loc);
                    }
                }
                diagnostic(g_source_file, ERR_P045_UNKNOWN_ENUM_VARIANT, parser->previous.line,
                           parser->previous.column,
                           "Unknown variant '%.*s' in enum '%.*s'",
                           (int)var_len, var_start,
                           (int)name_length, name_start);
                return NULL;
            }
        }

        /* Check for function call: identifier followed by '(' */
        if (parser_check(parser, TOKEN_LPAREN)) {
            parser_advance(parser);  /* consume '(' */

            size_t arg_count = 0;
            bool parse_error = false;
            bool *arg_is_ref = NULL;
            bool *arg_is_mut_ref = NULL;
            Ast **arguments = parser_parse_arg_list(parser, &arg_count, &parse_error,
                                                     &arg_is_ref, &arg_is_mut_ref);

            if (parse_error) {
                return NULL;
            }

            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                           parser->current.column, "Expected ')' after arguments");
                free(arguments);
                free(arg_is_ref);
                free(arg_is_mut_ref);
                return NULL;
            }

            return ast_make_func_call(name_start, name_length, arguments,
                                       arg_is_ref, arg_is_mut_ref, arg_count, loc);
        }

        /* Check for dot-qualified Name.Row before constructor */
        {
            ValueTypeEntry *dot_vt = parser_try_dot_qualified(
                parser, name_start, name_length);
            if (dot_vt) {
                name_start = dot_vt->name_start;
                name_length = dot_vt->name_length;
            }
        }

        /* Check for value constructor: TypeName { ... } (local module only) */
        if (parser_check(parser, TOKEN_LBRACE)) {
            ValueTypeEntry *vt = value_table_lookup_in_module(g_value_table,
                                                               name_start, name_length,
                                                               g_current_module_id);
            if (vt) {
                parser_advance(parser);  /* consume '{' */
                return parser_parse_ctor_fields(parser, name_start, name_length, loc);
            }
        }

        return ast_make_identifier(name_start, name_length, loc);
    }

    /* Array literal: [expr, expr, ...] */
    if (parser_match(parser, TOKEN_LBRACKET)) {
        SourceLoc loc = token_loc(&parser->previous);

        size_t capacity = 4;
        size_t count = 0;
        Ast **elements = malloc(capacity * sizeof(Ast *));
        if (!elements) panic(ERR_I001_OUT_OF_MEMORY, "allocating array literal elements");

        if (!parser_check(parser, TOKEN_RBRACKET)) {
            do {
                Ast *elem = parser_parse_expression(parser);
                if (!elem) {
                    for (size_t i = 0; i < count; i++) ast_free(elements[i]);
                    free(elements);
                    return NULL;
                }
                if (count >= capacity) {
                    capacity *= 2;
                    elements = realloc(elements, capacity * sizeof(Ast *));
                    if (!elements) panic(ERR_I001_OUT_OF_MEMORY, "growing array literal");
                }
                elements[count++] = elem;
            } while (parser_match(parser, TOKEN_COMMA));
        }

        if (!parser_match(parser, TOKEN_RBRACKET)) {
            diagnostic(g_source_file, ERR_P035_EXPECTED_RBRACKET_LITERAL, parser->current.line,
                       parser->current.column,
                       "Expected ']' to close array literal");
            for (size_t i = 0; i < count; i++) ast_free(elements[i]);
            free(elements);
            return NULL;
        }

        return ast_make_array_literal(elements, count, loc);
    }

    if (parser_match(parser, TOKEN_LPAREN)) {
        Ast *expr = parser_parse_expression(parser);
        if (!expr) return NULL;
        if (!parser_match(parser, TOKEN_RPAREN)) {
            diagnostic(g_source_file, ERR_P002_EXPECTED_RPAREN, parser->current.line,
                       parser->current.column, "Expected ')' after expression");
            return NULL;
        }
        return expr;
    }

    /* Block expression */
    if (parser_match(parser, TOKEN_LBRACE)) {
        return parser_parse_block(parser);
    }

    /* If expression */
    if (parser_match(parser, TOKEN_IF)) {
        return parser_parse_if(parser);
    }

    /* Match expression */
    if (parser_match(parser, TOKEN_MATCH)) {
        return parser_parse_match(parser);
    }

    diagnostic(g_source_file, ERR_P001_EXPECTED_EXPRESSION, parser->current.line, parser->current.column,
               "Expected expression");
    return NULL;
}

static Ast *parser_parse_postfix(Parser *parser) {
    Ast *expr = parser_parse_primary(parser);
    if (!expr) return NULL;

    /* Chain field access and index access: expr.field, expr[i] */
    for (;;) {
        if (parser_match(parser, TOKEN_DOT)) {
            SourceLoc loc = token_loc(&parser->previous);
            if (!parser_match(parser, TOKEN_IDENTIFIER)) {
                diagnostic(g_source_file, ERR_P003_EXPECTED_IDENTIFIER, parser->current.line,
                           parser->current.column, "Expected field name after '.'");
                ast_free(expr);
                return NULL;
            }
            expr = ast_make_field_access(expr, parser->previous.start,
                                          parser->previous.length, loc);
        } else if (parser_match(parser, TOKEN_LBRACKET)) {
            SourceLoc loc = token_loc(&parser->previous);
            /* Check for [..end] or [..] (start omitted) */
            if (parser_match(parser, TOKEN_DOTDOT)) {
                Ast *end = NULL;
                if (!parser_check(parser, TOKEN_RBRACKET)) {
                    end = parser_parse_expression(parser);
                    if (!end) { ast_free(expr); return NULL; }
                }
                if (!parser_match(parser, TOKEN_RBRACKET)) {
                    diagnostic(g_source_file, ERR_P047_EXPECTED_RBRACKET_SLICE,
                               parser->current.line, parser->current.column,
                               "Expected ']' after slice range");
                    ast_free(expr); if (end) ast_free(end);
                    return NULL;
                }
                expr = ast_make_slice_access(expr, NULL, end, loc);
            } else {
                Ast *first = parser_parse_expression(parser);
                if (!first) { ast_free(expr); return NULL; }
                if (parser_match(parser, TOKEN_DOTDOT)) {
                    /* [start..end] or [start..] */
                    Ast *end = NULL;
                    if (!parser_check(parser, TOKEN_RBRACKET)) {
                        end = parser_parse_expression(parser);
                        if (!end) { ast_free(expr); ast_free(first); return NULL; }
                    }
                    if (!parser_match(parser, TOKEN_RBRACKET)) {
                        diagnostic(g_source_file, ERR_P047_EXPECTED_RBRACKET_SLICE,
                                   parser->current.line, parser->current.column,
                                   "Expected ']' after slice range");
                        ast_free(expr); ast_free(first); if (end) ast_free(end);
                        return NULL;
                    }
                    expr = ast_make_slice_access(expr, first, end, loc);
                } else {
                    /* [index] */
                    if (!parser_match(parser, TOKEN_RBRACKET)) {
                        diagnostic(g_source_file, ERR_P034_EXPECTED_RBRACKET,
                                   parser->current.line, parser->current.column,
                                   "Expected ']' after index expression");
                        ast_free(expr); ast_free(first);
                        return NULL;
                    }
                    expr = ast_make_index_access(expr, first, loc);
                }
            }
        } else {
            break;
        }
    }
    return expr;
}

static Ast *parser_parse_precedence(Parser *parser, int min_precedence) {
    /* Parse left operand */
    Ast *left = parser_parse_postfix(parser);
    if (!left) return NULL;

    /* Consume operators while precedence is high enough */
    while (parser_get_precedence(parser->current.kind) >= min_precedence) {
        TokenKind op_kind = parser->current.kind;
        SourceLoc op_loc = token_loc(&parser->current);
        int precedence = parser_get_precedence(op_kind);
        parser_advance(parser);

        /* Parse right operand with higher precedence for left associativity */
        Ast *right = parser_parse_precedence(parser, precedence + 1);
        if (!right) return NULL;

        /* Combine into binary operation */
        left = ast_make_binary(parser_token_to_binary_op(op_kind), left, right, op_loc);
    }

    return left;
}

static Ast *parser_parse_expression(Parser *parser) {
    return parser_parse_precedence(parser, 0);
}

/* ============================= Statement Parsing ========================== */

static Ast *parser_parse_var_declaration(Parser *parser, bool is_mutable,
                                         bool is_public) {
    /* Already consumed TOKEN_VAL or TOKEN_MUT */
    SourceLoc loc = token_loc(&parser->previous);
    const char *keyword = is_mutable ? "mut" : "val";

    /* Expect identifier */
    if (!parser_match(parser, TOKEN_IDENTIFIER)) {
        diagnostic(g_source_file, ERR_P003_EXPECTED_IDENTIFIER, parser->current.line,
                   parser->current.column, "Expected identifier after '%s'", keyword);
        parser_synchronize(parser);
        return NULL;
    }
    const char *name_start = parser->previous.start;
    size_t name_length = parser->previous.length;

    /* Parse type annotation (required for mut, optional for val) */
    Type type = TYPE_UNKNOWN;
    if (is_mutable) {
        if (!parser_match(parser, TOKEN_COLON)) {
            diagnostic(g_source_file, ERR_P014_EXPECTED_COLON, parser->current.line,
                       parser->current.column, "Expected ':' after identifier");
            parser_synchronize(parser);
            return NULL;
        }
        type = parser_parse_type(parser, false);
        if (type == TYPE_UNKNOWN) {
            diagnostic(g_source_file, ERR_P015_EXPECTED_TYPE, parser->current.line,
                       parser->current.column, "Expected type (i64, i32, u8, u32, f64, bool, str, or Arena)");
            parser_synchronize(parser);
            return NULL;
        }
    } else if (parser_match(parser, TOKEN_COLON)) {
        type = parser_parse_type(parser, false);
        if (type == TYPE_UNKNOWN) {
            diagnostic(g_source_file, ERR_P015_EXPECTED_TYPE, parser->current.line,
                       parser->current.column, "Expected type (i64, i32, u8, u32, f64, bool, str, or Arena)");
            parser_synchronize(parser);
            return NULL;
        }
    }

    /* Expect '=' */
    if (!parser_match(parser, TOKEN_EQUALS)) {
        diagnostic(g_source_file, ERR_P004_EXPECTED_EQUALS, parser->current.line,
                   parser->current.column, "Expected '=' in %s declaration", keyword);
        parser_synchronize(parser);
        return NULL;
    }

    /* Parse initializer expression */
    Ast *initializer = parser_parse_expression(parser);
    if (!initializer) {
        parser_synchronize(parser);
        return NULL;
    }

    if (is_mutable) {
        return ast_make_mut_decl(name_start, name_length, type, initializer,
                                 is_public, loc);
    }
    return ast_make_val_decl(name_start, name_length, type, initializer,
                             is_public, loc);
}

static Ast *parser_parse_return(Parser *parser) {
    /* Already consumed TOKEN_RETURN - capture its location */
    SourceLoc loc = token_loc(&parser->previous);

    /* Check for bare return (no expression).
     * TOKEN_IF is NOT listed here: `return if (c) { a } else { b }` is valid. */
    if (parser_check(parser, TOKEN_RBRACE) ||
        parser_check(parser, TOKEN_VAL) ||
        parser_check(parser, TOKEN_MUT) ||
        parser_check(parser, TOKEN_RETURN) ||
        parser_check(parser, TOKEN_ASSERT) ||
        parser_check(parser, TOKEN_WHILE) ||
        parser_check(parser, TOKEN_BREAK) ||
        parser_check(parser, TOKEN_CONTINUE)) {
        /* Bare return for void functions */
        return ast_make_return(NULL, loc);
    }

    /* Parse return value expression */
    Ast *value = parser_parse_expression(parser);
    if (!value) {
        parser_synchronize(parser);
        return NULL;
    }

    return ast_make_return(value, loc);
}

static Ast *parser_parse_assignment(Parser *parser) {
    /* Already consumed TOKEN_IDENTIFIER - capture location and name */
    SourceLoc loc = token_loc(&parser->previous);
    Ast *target = ast_make_identifier(parser->previous.start,
                                       parser->previous.length, loc);

    /* Consume '=' */
    parser_advance(parser);

    /* Parse value expression */
    Ast *value = parser_parse_expression(parser);
    if (!value) {
        ast_free(target);
        parser_synchronize(parser);
        return NULL;
    }

    return ast_make_assignment(target, value, loc);
}

static Ast *parser_finish_assignment(Parser *parser, Ast *target,
                                     TokenKind assign_kind, SourceLoc loc) {
    parser_advance(parser);

    Ast *value = parser_parse_expression(parser);
    if (!value) {
        ast_free(target);
        parser_synchronize(parser);
        return NULL;
    }

    if (assign_kind == TOKEN_EQUALS) {
        return ast_make_assignment(target, value, loc);
    }

    return ast_make_compound_assignment(target,
                                        parser_token_to_compound_op(assign_kind),
                                        value, loc);
}

static Ast *parser_parse_assert(Parser *parser) {
    /* Already consumed TOKEN_ASSERT - capture its location */
    SourceLoc loc = token_loc(&parser->previous);

    /* Parse condition expression */
    Ast *condition = parser_parse_expression(parser);
    if (!condition) {
        parser_synchronize(parser);
        return NULL;
    }

    return ast_make_assert(condition, loc);
}

/* Forward declaration for mutual recursion with parser_parse_block */
static Ast *parser_parse_statement(Parser *parser);

static Ast *parser_parse_block(Parser *parser) {
    /* LBRACE already consumed - capture its location */
    SourceLoc loc = token_loc(&parser->previous);

    Ast *block = ast_make_block(loc);

    while (!parser_check(parser, TOKEN_RBRACE) &&
           !parser_check(parser, TOKEN_EOF) &&
           !too_many_errors()) {

        /* Handle if/else: parse it, then decide if it's a value_expr or statement.
         * Only treat as value_expr if: followed by }, has else, and both branches
         * produce values (have value_expr set in their blocks). */
        if (parser_check(parser, TOKEN_IF)) {
            parser_advance(parser);
            Ast *if_node = parser_parse_if(parser);
            if (if_node) {
                if (parser_check(parser, TOKEN_RBRACE) &&
                    ast_if_is_value_expr(if_node)) {
                    block->as.block.value_expr = if_node;
                    break;
                } else {
                    ast_block_add_statement(block, if_node);
                }
            }
            continue;
        }

        /* Handle match: parse it, then decide if it's a value_expr or statement.
         * Only treat as value_expr if: followed by }, and all arm bodies
         * produce values (have value_expr set in their blocks). */
        if (parser_check(parser, TOKEN_MATCH)) {
            parser_advance(parser);
            Ast *match_node = parser_parse_match(parser);
            if (match_node) {
                bool all_arms_have_value = (match_node->as.match_expr.arm_count > 0);
                for (size_t i = 0; i < match_node->as.match_expr.arm_count; i++) {
                    Ast *arm = match_node->as.match_expr.arms[i];
                    if (!arm->as.match_arm.body->as.block.value_expr) {
                        all_arms_have_value = false;
                        break;
                    }
                }
                if (parser_check(parser, TOKEN_RBRACE) && all_arms_have_value) {
                    block->as.block.value_expr = match_node;
                    break;
                } else {
                    ast_block_add_statement(block, match_node);
                }
            }
            continue;
        }

        /* Check if current token starts a statement */
        bool is_statement_start =
            parser_check(parser, TOKEN_VAL) ||
            parser_check(parser, TOKEN_MUT) ||
            parser_check(parser, TOKEN_RETURN) ||
            parser_check(parser, TOKEN_ASSERT) ||
            parser_check(parser, TOKEN_WHILE) ||
            parser_check(parser, TOKEN_FOR) ||
            parser_check(parser, TOKEN_BREAK) ||
            parser_check(parser, TOKEN_CONTINUE) ||
            parser_check(parser, TOKEN_LBRACE) ||
            (parser_check(parser, TOKEN_IDENTIFIER) &&
             parser_peek_next(parser) == TOKEN_EQUALS);

        if (is_statement_start) {
            Ast *statement = parser_parse_statement(parser);
            if (statement) {
                ast_block_add_statement(block, statement);
            }
        } else {
            /* Try to parse as expression (potential block value or field assignment) */
            Ast *expr = parser_parse_expression(parser);
            if (expr) {
                if (parser_check(parser, TOKEN_RBRACE)) {
                    /* This is the block's value expression */
                    block->as.block.value_expr = expr;
                    break;
                } else if (parser_check(parser, TOKEN_EQUALS) ||
                           token_is_compound_assignment(parser->current.kind)) {
                    Ast *assign = parser_finish_assignment(parser, expr,
                                                           parser->current.kind,
                                                           expr->loc);
                    if (assign) {
                        ast_block_add_statement(block, assign);
                    }
                } else if (expr->kind == AST_FUNC_CALL ||
                           expr->kind == AST_ARENA_RESET ||
                           expr->kind == AST_TABLE_INSERT ||
                           expr->kind == AST_FD_WRITE ||
                           expr->kind == AST_FD_READ ||
                           expr->kind == AST_FD_CLOSE ||
                           expr->kind == AST_FD_SEEK ||
                           expr->kind == AST_MEM_COPY ||
                           expr->kind == AST_EXIT ||
                           expr->kind == AST_ARGS ||
                           expr->kind == AST_SHELL_RUN) {
                    /* Bare function call or built-in as statement */
                    ast_block_add_statement(block, expr);
                } else {
                    /* Expression not at end of block */
                    diagnostic(g_source_file, ERR_P005_EXPECTED_STATEMENT,
                               expr->loc.line, expr->loc.column,
                               "Bare expression not allowed; expected statement or '}'");
                    ast_free(expr);
                    parser_synchronize(parser);
                }
            }
        }
    }

    if (!parser_match(parser, TOKEN_RBRACE)) {
        diagnostic(g_source_file, ERR_P007_EXPECTED_RBRACE, parser->current.line,
                   parser->current.column, "Expected '}' to close block");
    }

    return block;
}

static Ast *parser_parse_if(Parser *parser) {
    /* TOKEN_IF already consumed - capture its location */
    SourceLoc loc = token_loc(&parser->previous);

    /* Expect '(' */
    if (!parser_match(parser, TOKEN_LPAREN)) {
        diagnostic(g_source_file, ERR_P008_EXPECTED_LPAREN_IF, parser->current.line,
                   parser->current.column, "Expected '(' after 'if'");
        parser_synchronize(parser);
        return NULL;
    }

    /* Parse condition */
    Ast *condition = parser_parse_expression(parser);
    if (!condition) {
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect ')' */
    if (!parser_match(parser, TOKEN_RPAREN)) {
        diagnostic(g_source_file, ERR_P009_EXPECTED_RPAREN_IF, parser->current.line,
                   parser->current.column, "Expected ')' after condition");
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect '{' and parse block */
    if (!parser_match(parser, TOKEN_LBRACE)) {
        diagnostic(g_source_file, ERR_P010_EXPECTED_LBRACE_IF, parser->current.line,
                   parser->current.column, "Expected '{' for if body");
        parser_synchronize(parser);
        return NULL;
    }
    Ast *then_block = parser_parse_block(parser);

    /* Optional else */
    Ast *else_block = NULL;
    if (parser_match(parser, TOKEN_ELSE)) {
        if (parser_match(parser, TOKEN_IF)) {
            Ast *else_if = parser_parse_if(parser);
            if (!else_if) {
                return NULL;
            }
            else_block = ast_make_block(else_if->loc);
            else_block->as.block.value_expr = else_if;
        } else {
            if (!parser_match(parser, TOKEN_LBRACE)) {
                diagnostic(g_source_file, ERR_P010_EXPECTED_LBRACE_IF, parser->current.line,
                           parser->current.column, "Expected '{' for else body");
                parser_synchronize(parser);
                return NULL;
            }
            else_block = parser_parse_block(parser);
        }
    }

    return ast_make_if(condition, then_block, else_block, loc);
}

static bool ast_if_is_value_expr(Ast *node) {
    if (!node || node->kind != AST_IF) return false;
    if (!node->as.if_stmt.else_block) return false;
    if (!node->as.if_stmt.then_block->as.block.value_expr) return false;
    if (!node->as.if_stmt.else_block->as.block.value_expr) return false;

    Ast *else_value = node->as.if_stmt.else_block->as.block.value_expr;
    if (else_value->kind == AST_IF) {
        return ast_if_is_value_expr(else_value);
    }

    return true;
}

/* Parse the limited literal forms allowed in scalar match arms. */
static Ast *parser_parse_match_literal_pattern(Parser *parser) {
    if (parser_check(parser, TOKEN_NUMBER) ||
        parser_check(parser, TOKEN_CHAR) ||
        parser_check(parser, TOKEN_TRUE) ||
        parser_check(parser, TOKEN_FALSE)) {
        return parser_parse_primary(parser);
    }

    if (parser_match(parser, TOKEN_MINUS)) {
        SourceLoc loc = token_loc(&parser->previous);
        if (!parser_match(parser, TOKEN_NUMBER)) {
            diagnostic(g_source_file, ERR_P052_EXPECTED_VARIANT_MATCH,
                       parser->current.line, parser->current.column,
                       "Expected integer literal after '-' in match arm");
            return NULL;
        }

        char buffer[32];
        size_t len = parser->previous.length;
        if (len >= sizeof(buffer)) {
            diagnostic(g_source_file, ERR_P006_NUMBER_TOO_LARGE, loc.line, loc.column,
                       "Number literal too large");
            return NULL;
        }
        memcpy(buffer, parser->previous.start, len);
        buffer[len] = '\0';
        errno = 0;
        long value = strtol(buffer, NULL, 10);
        if (errno == ERANGE) {
            diagnostic(g_source_file, ERR_P006_NUMBER_TOO_LARGE, loc.line, loc.column,
                       "Integer literal out of range");
            return ast_make_number(0, loc);
        }

        return ast_make_number(-value, loc);
    }

    return NULL;
}

/* Match patterns reuse the enum/literal syntax, plus reserved `else`. */
static bool parser_parse_match_arm_pattern(Parser *parser,
                                           MatchArmPattern *pattern) {
    memset(pattern, 0, sizeof(*pattern));
    pattern->loc = token_loc(&parser->current);

    if (parser_match(parser, TOKEN_ELSE)) {
        pattern->kind = MATCH_PATTERN_ELSE;
    } else if (parser_match(parser, TOKEN_IDENTIFIER)) {
        pattern->kind = MATCH_PATTERN_ENUM_VARIANT;
        pattern->variant_name = parser->previous.start;
        pattern->variant_length = parser->previous.length;
    } else if (parser_check(parser, TOKEN_NUMBER) ||
               parser_check(parser, TOKEN_CHAR) ||
               parser_check(parser, TOKEN_TRUE) ||
               parser_check(parser, TOKEN_FALSE) ||
               parser_check(parser, TOKEN_MINUS)) {
        pattern->kind = MATCH_PATTERN_LITERAL;
        pattern->pattern_expr = parser_parse_match_literal_pattern(parser);
        if (!pattern->pattern_expr) return false;
    } else {
        diagnostic(g_source_file, ERR_P052_EXPECTED_VARIANT_MATCH,
                   parser->current.line, parser->current.column,
                   "Expected match pattern in arm");
        return false;
    }

    if ((pattern->kind == MATCH_PATTERN_ENUM_VARIANT ||
         pattern->kind == MATCH_PATTERN_ELSE) &&
        parser_match(parser, TOKEN_LPAREN)) {
        pattern->has_binding = true;
        if (parser_match(parser, TOKEN_IDENTIFIER)) {
            if (!(parser->previous.length == 1 &&
                  parser->previous.start[0] == '_')) {
                pattern->binding_name = parser->previous.start;
                pattern->binding_length = parser->previous.length;
            }
        }
        if (!parser_match(parser, TOKEN_RPAREN)) {
            diagnostic(g_source_file, ERR_P055_EXPECTED_RPAREN_BINDING,
                       parser->current.line, parser->current.column,
                       "Expected ')' after match binding");
            return false;
        }
    }

    return true;
}

static Ast *parser_parse_match(Parser *parser) {
    /* TOKEN_MATCH already consumed */
    SourceLoc loc = token_loc(&parser->previous);

    if (!parser_match(parser, TOKEN_LPAREN)) {
        diagnostic(g_source_file, ERR_P049_EXPECTED_LPAREN_MATCH,
                   parser->current.line, parser->current.column,
                   "Expected '(' after 'match'");
        parser_synchronize(parser);
        return NULL;
    }

    Ast *scrutinee = parser_parse_expression(parser);
    if (!scrutinee) { parser_synchronize(parser); return NULL; }

    if (!parser_match(parser, TOKEN_RPAREN)) {
        diagnostic(g_source_file, ERR_P050_EXPECTED_RPAREN_MATCH,
                   parser->current.line, parser->current.column,
                   "Expected ')' after match scrutinee");
        ast_free(scrutinee);
        parser_synchronize(parser);
        return NULL;
    }

    if (!parser_match(parser, TOKEN_LBRACE)) {
        diagnostic(g_source_file, ERR_P051_EXPECTED_LBRACE_MATCH,
                   parser->current.line, parser->current.column,
                   "Expected '{' to open match body");
        ast_free(scrutinee);
        parser_synchronize(parser);
        return NULL;
    }

    /* Parse arms: Variant(binding) = { body }, literal = { body }, _ = { body }, else = { body }. */
    size_t arm_capacity = 8;
    size_t arm_count = 0;
    Ast **arms = malloc(arm_capacity * sizeof(Ast *));
    if (!arms) panic(ERR_I001_OUT_OF_MEMORY, "allocating match arms");

    while (!parser_check(parser, TOKEN_RBRACE) &&
           !parser_check(parser, TOKEN_EOF) &&
           !too_many_errors()) {
        SourceLoc arm_loc = token_loc(&parser->current);
        size_t pattern_capacity = 4;
        size_t pattern_count = 0;
        MatchArmPattern *patterns = malloc(pattern_capacity * sizeof(MatchArmPattern));
        if (!patterns) panic(ERR_I001_OUT_OF_MEMORY, "allocating match arm patterns");

        if (!parser_parse_match_arm_pattern(parser, &patterns[pattern_count++])) {
            match_arm_patterns_free(patterns, pattern_count);
            break;
        }

        while (parser_match(parser, TOKEN_PIPE)) {
            if (pattern_count >= pattern_capacity) {
                pattern_capacity *= 2;
                patterns = realloc(patterns,
                                   pattern_capacity * sizeof(MatchArmPattern));
                if (!patterns) {
                    panic(ERR_I001_OUT_OF_MEMORY, "growing match arm patterns");
                }
            }
            if (!parser_parse_match_arm_pattern(parser, &patterns[pattern_count])) {
                match_arm_patterns_free(patterns, pattern_count + 1);
                patterns = NULL;
                break;
            }
            pattern_count++;
        }

        if (!patterns) break;

        if (!parser_match(parser, TOKEN_EQUALS)) {
            diagnostic(g_source_file, ERR_P053_EXPECTED_EQUALS_MATCH,
                       parser->current.line, parser->current.column,
                       "Expected '=' after match pattern");
            match_arm_patterns_free(patterns, pattern_count);
            break;
        }

        if (!parser_match(parser, TOKEN_LBRACE)) {
            diagnostic(g_source_file, ERR_P051_EXPECTED_LBRACE_MATCH,
                       parser->current.line, parser->current.column,
                       "Expected '{' for match arm body");
            match_arm_patterns_free(patterns, pattern_count);
            break;
        }
        Ast *body = parser_parse_block(parser);
        if (!body) {
            match_arm_patterns_free(patterns, pattern_count);
            break;
        }

        if (arm_count >= arm_capacity) {
            arm_capacity *= 2;
            arms = realloc(arms, arm_capacity * sizeof(Ast *));
            if (!arms) panic(ERR_I001_OUT_OF_MEMORY, "growing match arms");
        }
        Ast *arm = ast_make_match_arm(patterns, pattern_count, body, arm_loc);
        arms[arm_count++] = arm;
    }

    if (!parser_match(parser, TOKEN_RBRACE)) {
        diagnostic(g_source_file, ERR_P054_EXPECTED_RBRACE_MATCH,
                   parser->current.line, parser->current.column,
                   "Expected '}' to close match");
        ast_free(scrutinee);
        for (size_t i = 0; i < arm_count; i++) ast_free(arms[i]);
        free(arms);
        parser_synchronize(parser);
        return NULL;
    }

    return ast_make_match(scrutinee, arms, arm_count, loc);
}

static Ast *parser_parse_while(Parser *parser) {
    /* TOKEN_WHILE already consumed - capture its location */
    SourceLoc loc = token_loc(&parser->previous);

    /* Expect '(' */
    if (!parser_match(parser, TOKEN_LPAREN)) {
        diagnostic(g_source_file, ERR_P011_EXPECTED_LPAREN_WHILE, parser->current.line,
                   parser->current.column, "Expected '(' after 'while'");
        parser_synchronize(parser);
        return NULL;
    }

    /* Parse condition */
    Ast *condition = parser_parse_expression(parser);
    if (!condition) {
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect ')' */
    if (!parser_match(parser, TOKEN_RPAREN)) {
        diagnostic(g_source_file, ERR_P012_EXPECTED_RPAREN_WHILE, parser->current.line,
                   parser->current.column, "Expected ')' after condition");
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect '{' and parse block */
    if (!parser_match(parser, TOKEN_LBRACE)) {
        diagnostic(g_source_file, ERR_P013_EXPECTED_LBRACE_WHILE, parser->current.line,
                   parser->current.column, "Expected '{' for while body");
        parser_synchronize(parser);
        return NULL;
    }
    Ast *body = parser_parse_block(parser);

    return ast_make_while(condition, body, loc);
}

static Ast *parser_parse_for(Parser *parser) {
    /* TOKEN_FOR already consumed - capture its location */
    SourceLoc loc = token_loc(&parser->previous);

    /* Expect loop variable identifier */
    if (!parser_match(parser, TOKEN_IDENTIFIER)) {
        diagnostic(g_source_file, ERR_P003_EXPECTED_IDENTIFIER, parser->current.line,
                   parser->current.column, "Expected loop variable after 'for'");
        parser_synchronize(parser);
        return NULL;
    }
    const char *var_start = parser->previous.start;
    size_t var_length = parser->previous.length;

    /* Expect 'in' */
    if (!parser_match(parser, TOKEN_IN)) {
        diagnostic(g_source_file, ERR_P038_EXPECTED_IN_FOR, parser->current.line,
                   parser->current.column, "Expected 'in' after loop variable");
        parser_synchronize(parser);
        return NULL;
    }

    /* Parse start expression */
    Ast *start = parser_parse_expression(parser);
    if (!start) {
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect '..' */
    if (!parser_match(parser, TOKEN_DOTDOT)) {
        diagnostic(g_source_file, ERR_P039_EXPECTED_DOTDOT, parser->current.line,
                   parser->current.column, "Expected '..' in range");
        parser_synchronize(parser);
        return NULL;
    }

    /* Parse end expression */
    Ast *end = parser_parse_expression(parser);
    if (!end) {
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect '{' and parse block */
    if (!parser_match(parser, TOKEN_LBRACE)) {
        diagnostic(g_source_file, ERR_P040_EXPECTED_LBRACE_FOR, parser->current.line,
                   parser->current.column, "Expected '{' for for-loop body");
        parser_synchronize(parser);
        return NULL;
    }
    Ast *body = parser_parse_block(parser);

    return ast_make_for(var_start, var_length, start, end, body, loc);
}

static Ast *parser_parse_break(Parser *parser) {
    /* TOKEN_BREAK already consumed - capture its location */
    SourceLoc loc = token_loc(&parser->previous);
    return ast_make_break(loc);
}

static Ast *parser_parse_continue(Parser *parser) {
    /* TOKEN_CONTINUE already consumed - capture its location */
    SourceLoc loc = token_loc(&parser->previous);
    return ast_make_continue(loc);
}

/* Parse a single parameter: [mut] [ref] name: type */
static bool parser_parse_parameter(Parser *parser, Parameter *param) {
    param->is_ref = false;
    param->is_mut_ref = false;

    /* Check for [mut] ref prefix */
    if (parser_match(parser, TOKEN_MUT)) {
        if (!parser_match(parser, TOKEN_REF)) {
            diagnostic(g_source_file, ERR_P036_EXPECTED_REF_PARAM, parser->current.line,
                       parser->current.column,
                       "Expected 'ref' after 'mut' in parameter");
            return false;
        }
        param->is_ref = true;
        param->is_mut_ref = true;
    } else if (parser_match(parser, TOKEN_REF)) {
        param->is_ref = true;
    }

    if (!parser_match(parser, TOKEN_IDENTIFIER)) {
        diagnostic(g_source_file, ERR_P003_EXPECTED_IDENTIFIER, parser->current.line,
                   parser->current.column, "Expected parameter name");
        return false;
    }
    param->name_start = parser->previous.start;
    param->name_length = parser->previous.length;

    if (!parser_match(parser, TOKEN_COLON)) {
        diagnostic(g_source_file, ERR_P014_EXPECTED_COLON, parser->current.line,
                   parser->current.column, "Expected ':' after parameter name");
        return false;
    }

    param->type = parser_parse_type(parser, false);
    if (param->type == TYPE_UNKNOWN) {
        if (parser_match(parser, TOKEN_VOID)) {
            diagnostic(g_source_file, ERR_S012_VOID_PARAM, parser->previous.line,
                       parser->previous.column, "void is not valid as parameter type");
        } else {
            diagnostic(g_source_file, ERR_P015_EXPECTED_TYPE, parser->current.line,
                       parser->current.column, "Expected type (i64, i32, u8, u32, f64, bool, str, or Arena)");
        }
        return false;
    }

    return true;
}

/* Parse argument list (without parens): [ref|mut ref] expr, ...
 * Sets *count to number of args. Sets *error to true on parse failure.
 * Returns NULL on empty list (not an error). Caller must free the returned arrays.
 * *out_is_ref and *out_is_mut_ref are parallel bool arrays (caller frees). */
static Ast **parser_parse_arg_list(Parser *parser, size_t *count, bool *error,
                                    bool **out_is_ref, bool **out_is_mut_ref) {
    *count = 0;
    *error = false;
    *out_is_ref = NULL;
    *out_is_mut_ref = NULL;

    /* Empty argument list */
    if (parser_check(parser, TOKEN_RPAREN)) {
        return NULL;
    }

    size_t capacity = 4;
    Ast **args = malloc(capacity * sizeof(Ast *));
    if (!args) panic(ERR_I001_OUT_OF_MEMORY, "allocating argument list");
    bool *is_ref = malloc(capacity * sizeof(bool));
    if (!is_ref) panic(ERR_I001_OUT_OF_MEMORY, "allocating ref flags");
    bool *is_mut_ref = malloc(capacity * sizeof(bool));
    if (!is_mut_ref) panic(ERR_I001_OUT_OF_MEMORY, "allocating mut ref flags");

    do {
        bool arg_ref = false;
        bool arg_mut_ref = false;

        if (parser_match(parser, TOKEN_MUT)) {
            if (!parser_match(parser, TOKEN_REF)) {
                diagnostic(g_source_file, ERR_P037_EXPECTED_REF_ARG, parser->current.line,
                           parser->current.column,
                           "Expected 'ref' after 'mut' in argument");
                goto fail;
            }
            arg_ref = true;
            arg_mut_ref = true;
        } else if (parser_match(parser, TOKEN_REF)) {
            arg_ref = true;
        }

        Ast *arg = parser_parse_expression(parser);
        if (!arg) goto fail;

        if (*count >= capacity) {
            capacity *= 2;
            args = realloc(args, capacity * sizeof(Ast *));
            if (!args) panic(ERR_I001_OUT_OF_MEMORY, "growing argument list");
            is_ref = realloc(is_ref, capacity * sizeof(bool));
            if (!is_ref) panic(ERR_I001_OUT_OF_MEMORY, "growing ref flags");
            is_mut_ref = realloc(is_mut_ref, capacity * sizeof(bool));
            if (!is_mut_ref) panic(ERR_I001_OUT_OF_MEMORY, "growing mut ref flags");
        }
        is_ref[*count] = arg_ref;
        is_mut_ref[*count] = arg_mut_ref;
        args[(*count)++] = arg;
    } while (parser_match(parser, TOKEN_COMMA));

    *out_is_ref = is_ref;
    *out_is_mut_ref = is_mut_ref;
    return args;

fail:
    free(args);
    free(is_ref);
    free(is_mut_ref);
    *count = 0;
    *error = true;
    return NULL;
}

/* Parse parameter list: name: type, name: type, ...
 * Stops before end_token. Returns NULL for empty list or on error (*count == 0). */
static Parameter *parser_parse_param_list(Parser *parser, size_t *count,
                                           TokenKind end_token) {
    *count = 0;

    /* Empty list */
    if (parser_check(parser, end_token)) {
        return NULL;
    }

    size_t capacity = 4;
    Parameter *params = malloc(capacity * sizeof(Parameter));
    if (!params) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating parameters");
    }

    /* Parse first parameter */
    if (!parser_parse_parameter(parser, &params[*count])) {
        free(params);
        return NULL;
    }
    (*count)++;

    /* Parse remaining parameters */
    while (parser_match(parser, TOKEN_COMMA)) {
        if (*count >= capacity) {
            capacity *= 2;
            params = realloc(params, capacity * sizeof(Parameter));
            if (!params) {
                panic(ERR_I001_OUT_OF_MEMORY, "growing parameters");
            }
        }

        if (!parser_parse_parameter(parser, &params[*count])) {
            free(params);
            return NULL;
        }
        (*count)++;
    }

    return params;
}

/* Parse function declaration: func name(params): returnType = { body } */
static Ast *parser_parse_function(Parser *parser, bool is_public) {
    /* TOKEN_FUNC already consumed */
    SourceLoc loc = token_loc(&parser->previous);

    /* Expect function name */
    if (!parser_match(parser, TOKEN_IDENTIFIER)) {
        diagnostic(g_source_file, ERR_P017_EXPECTED_FUNC_NAME, parser->current.line,
                   parser->current.column, "Expected function name after 'func'");
        parser_synchronize(parser);
        return NULL;
    }
    const char *name_start = parser->previous.start;
    size_t name_length = parser->previous.length;

    /* Expect '(' */
    if (!parser_match(parser, TOKEN_LPAREN)) {
        diagnostic(g_source_file, ERR_P018_EXPECTED_LPAREN_FUNC, parser->current.line,
                   parser->current.column, "Expected '(' after function name");
        parser_synchronize(parser);
        return NULL;
    }

    /* Parse parameter list */
    size_t param_count = 0;
    Parameter *params = parser_parse_param_list(parser, &param_count, TOKEN_RPAREN);

    /* Expect ')' */
    if (!parser_match(parser, TOKEN_RPAREN)) {
        diagnostic(g_source_file, ERR_P019_EXPECTED_RPAREN_FUNC, parser->current.line,
                   parser->current.column, "Expected ')' after parameters");
        if (params) free(params);
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect ':' */
    if (!parser_match(parser, TOKEN_COLON)) {
        diagnostic(g_source_file, ERR_P020_EXPECTED_COLON_FUNC, parser->current.line,
                   parser->current.column, "Expected ':' before return type");
        if (params) free(params);
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect return type */
    Type return_type = parser_parse_type(parser, true);
    if (return_type == TYPE_UNKNOWN) {
        diagnostic(g_source_file, ERR_P021_EXPECTED_RETURN_TYPE, parser->current.line,
                   parser->current.column, "Expected return type (i64, i32, u8, u32, f64, bool, str, Arena, or void)");
        if (params) free(params);
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect '=' */
    if (!parser_match(parser, TOKEN_EQUALS)) {
        diagnostic(g_source_file, ERR_P022_EXPECTED_EQUALS_FUNC, parser->current.line,
                   parser->current.column, "Expected '=' before function body");
        if (params) free(params);
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect '{' and parse block */
    if (!parser_match(parser, TOKEN_LBRACE)) {
        diagnostic(g_source_file, ERR_P023_EXPECTED_LBRACE_FUNC, parser->current.line,
                   parser->current.column, "Expected '{' for function body");
        if (params) free(params);
        parser_synchronize(parser);
        return NULL;
    }
    Ast *body = parser_parse_block(parser);

    return ast_make_func_decl(name_start, name_length, params, param_count,
                              return_type, body, is_public, loc);
}

static Ast *parser_parse_statement(Parser *parser) {
    if (too_many_errors()) return NULL;

    /* Block statement */
    if (parser_match(parser, TOKEN_LBRACE)) {
        return parser_parse_block(parser);
    }

    if (parser_match(parser, TOKEN_VAL)) {
        return parser_parse_var_declaration(parser, false, false);
    }

    if (parser_match(parser, TOKEN_MUT)) {
        return parser_parse_var_declaration(parser, true, false);
    }

    if (parser_match(parser, TOKEN_RETURN)) {
        return parser_parse_return(parser);
    }

    if (parser_match(parser, TOKEN_ASSERT)) {
        return parser_parse_assert(parser);
    }

    if (parser_match(parser, TOKEN_IF)) {
        return parser_parse_if(parser);
    }

    if (parser_match(parser, TOKEN_WHILE)) {
        return parser_parse_while(parser);
    }

    if (parser_match(parser, TOKEN_FOR)) {
        return parser_parse_for(parser);
    }

    if (parser_match(parser, TOKEN_MATCH)) {
        return parser_parse_match(parser);
    }

    if (parser_match(parser, TOKEN_BREAK)) {
        return parser_parse_break(parser);
    }

    if (parser_match(parser, TOKEN_CONTINUE)) {
        return parser_parse_continue(parser);
    }

    /* Check for assignment: identifier followed by '=' */
    if (parser_check(parser, TOKEN_IDENTIFIER) &&
        parser_peek_next(parser) == TOKEN_EQUALS) {
        parser_advance(parser);  /* consume identifier */
        return parser_parse_assignment(parser);
    }

    if (parser_check(parser, TOKEN_EOF)) {
        return NULL;
    }

    diagnostic(g_source_file, ERR_P005_EXPECTED_STATEMENT, parser->current.line,
               parser->current.column, "Expected statement");
    parser_synchronize(parser);
    return NULL;
}

/* Parse value/struct type declaration: value|struct Name { field: Type, ... } */
static Ast *parser_parse_value_decl(Parser *parser, bool is_struct,
                                    bool is_public) {
    /* TOKEN_VALUE or TOKEN_STRUCT already consumed */
    SourceLoc loc = token_loc(&parser->previous);
    const char *keyword = is_struct ? "struct" : "value";

    /* Expect type name */
    if (!parser_match(parser, TOKEN_IDENTIFIER)) {
        diagnostic(g_source_file, ERR_P024_EXPECTED_VALUE_NAME, parser->current.line,
                   parser->current.column,
                   "Expected %s type name after '%s'", keyword, keyword);
        parser_synchronize(parser);
        return NULL;
    }
    const char *name_start = parser->previous.start;
    size_t name_length = parser->previous.length;

    /* Expect '{' */
    if (!parser_match(parser, TOKEN_LBRACE)) {
        diagnostic(g_source_file, ERR_P025_EXPECTED_LBRACE_VALUE, parser->current.line,
                   parser->current.column,
                   "Expected '{' after %s type name", keyword);
        parser_synchronize(parser);
        return NULL;
    }

    /* Parse fields: name: Type, name: Type, ... */
    size_t field_count = 0;
    Parameter *fields = parser_parse_param_list(parser, &field_count, TOKEN_RBRACE);
    if (field_count == 0 && !parser_check(parser, TOKEN_RBRACE)) {
        /* Parse error in field list */
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect '}' */
    if (!parser_match(parser, TOKEN_RBRACE)) {
        diagnostic(g_source_file, ERR_P027_EXPECTED_RBRACE_VALUE, parser->current.line,
                   parser->current.column,
                   "Expected '}' to close %s type", keyword);
        free(fields);
        parser_synchronize(parser);
        return NULL;
    }

    value_table_add(name_start, name_length, fields, field_count, loc, is_struct,
                    is_public);

    return ast_make_value_decl(name_start, name_length, fields, field_count,
                               is_struct, is_public, loc);
}

/* Static string for the synthesized "_len" field name */
static const char *g_table_len_field = "_len";

/* Parse table declaration: table Name { field: Type, ... }
 * Generates a struct (columnar) and a value (row) in g_value_table,
 * plus a TableDeclEntry. Returns AST_VALUE_DECL for the struct. */
static Ast *parser_parse_enum_decl(Parser *parser, bool is_public) {
    /* TOKEN_ENUM already consumed */
    SourceLoc loc = token_loc(&parser->previous);

    if (!parser_match(parser, TOKEN_IDENTIFIER)) {
        diagnostic(g_source_file, ERR_P041_EXPECTED_ENUM_NAME, parser->current.line,
                   parser->current.column,
                   "Expected enum type name after 'enum'");
        parser_synchronize(parser);
        return NULL;
    }
    const char *name_start = parser->previous.start;
    size_t name_length = parser->previous.length;

    /* Check for duplicate type names */
    if (value_table_lookup(g_value_table, name_start, name_length) ||
        enum_table_lookup(name_start, name_length)) {
        diagnostic(g_source_file, ERR_S070_DUPLICATE_TYPE_NAME, loc.line, loc.column,
                   "Type name '%.*s' is already defined",
                   (int)name_length, name_start);
    }

    if (!parser_match(parser, TOKEN_LBRACE)) {
        diagnostic(g_source_file, ERR_P042_EXPECTED_LBRACE_ENUM, parser->current.line,
                   parser->current.column,
                   "Expected '{' after enum name");
        parser_synchronize(parser);
        return NULL;
    }

    /* Parse variants: Name, Name, ... */
    size_t capacity = 8;
    size_t variant_count = 0;
    EnumVariant *variants = malloc(capacity * sizeof(EnumVariant));
    if (!variants) panic(ERR_I001_OUT_OF_MEMORY, "allocating enum variants");

    while (!parser_check(parser, TOKEN_RBRACE) &&
           !parser_check(parser, TOKEN_EOF)) {
        if (!parser_match(parser, TOKEN_IDENTIFIER)) {
            diagnostic(g_source_file, ERR_P044_EXPECTED_VARIANT_NAME, parser->current.line,
                       parser->current.column,
                       "Expected variant name in enum");
            free(variants);
            parser_synchronize(parser);
            return NULL;
        }

        const char *variant_start = parser->previous.start;
        size_t variant_length = parser->previous.length;

        /* Check for duplicate variant names */
        for (size_t i = 0; i < variant_count; i++) {
            if (variants[i].name_length == variant_length &&
                memcmp(variants[i].name_start, variant_start,
                       variant_length) == 0) {
                diagnostic(g_source_file, ERR_S069_DUPLICATE_ENUM_VARIANT,
                           parser->previous.line, parser->previous.column,
                           "Duplicate variant '%.*s' in enum '%.*s'",
                           (int)variant_length, variant_start,
                           (int)name_length, name_start);
            }
        }

        /* Optional payload type: Variant(Type) */
        Type payload_type = TYPE_VOID;
        if (parser_match(parser, TOKEN_LPAREN)) {
            payload_type = parser_parse_type(parser, false);
            if (payload_type == TYPE_UNKNOWN) {
                diagnostic(g_source_file, ERR_P048_EXPECTED_RPAREN_PAYLOAD,
                           parser->current.line, parser->current.column,
                           "Expected type in variant payload");
                free(variants);
                parser_synchronize(parser);
                return NULL;
            }
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P048_EXPECTED_RPAREN_PAYLOAD,
                           parser->current.line, parser->current.column,
                           "Expected ')' after variant payload type");
                free(variants);
                parser_synchronize(parser);
                return NULL;
            }
        }

        if (variant_count >= capacity) {
            capacity *= 2;
            variants = realloc(variants, capacity * sizeof(EnumVariant));
            if (!variants) panic(ERR_I001_OUT_OF_MEMORY, "growing enum variants");
        }
        variants[variant_count].name_start = variant_start;
        variants[variant_count].name_length = variant_length;
        variants[variant_count].value = (long)variant_count;
        variants[variant_count].payload_type = payload_type;
        variant_count++;

        if (!parser_match(parser, TOKEN_COMMA)) break;
    }

    if (!parser_match(parser, TOKEN_RBRACE)) {
        diagnostic(g_source_file, ERR_P043_EXPECTED_RBRACE_ENUM, parser->current.line,
                   parser->current.column,
                   "Expected '}' to close enum");
        free(variants);
        parser_synchronize(parser);
        return NULL;
    }

    enum_table_add(name_start, name_length, variants, variant_count, loc,
                   is_public);
    return ast_make_enum_decl(name_start, name_length, variant_count, is_public,
                              loc);
}

static Ast *parser_parse_table_decl(Parser *parser, bool is_public) {
    /* TOKEN_TABLE already consumed */
    SourceLoc loc = token_loc(&parser->previous);

    /* Expect table name */
    if (!parser_match(parser, TOKEN_IDENTIFIER)) {
        diagnostic(g_source_file, ERR_P024_EXPECTED_VALUE_NAME, parser->current.line,
                   parser->current.column,
                   "Expected table type name after 'table'");
        parser_synchronize(parser);
        return NULL;
    }
    const char *name_start = parser->previous.start;
    size_t name_length = parser->previous.length;

    /* Expect '{' */
    if (!parser_match(parser, TOKEN_LBRACE)) {
        diagnostic(g_source_file, ERR_P025_EXPECTED_LBRACE_VALUE, parser->current.line,
                   parser->current.column,
                   "Expected '{' after table type name");
        parser_synchronize(parser);
        return NULL;
    }

    /* Parse original fields: name: Type, name: Type, ... */
    size_t field_count = 0;
    Parameter *orig_fields = parser_parse_param_list(parser, &field_count, TOKEN_RBRACE);
    if (field_count == 0 && !parser_check(parser, TOKEN_RBRACE)) {
        parser_synchronize(parser);
        return NULL;
    }

    /* Expect '}' */
    if (!parser_match(parser, TOKEN_RBRACE)) {
        diagnostic(g_source_file, ERR_P027_EXPECTED_RBRACE_VALUE, parser->current.line,
                   parser->current.column,
                   "Expected '}' to close table type");
        free(orig_fields);
        parser_synchronize(parser);
        return NULL;
    }

    /* Build struct fields: one slice column per original field + _len: i64 */
    size_t struct_field_count = field_count + 1;
    Parameter *struct_fields = malloc(struct_field_count * sizeof(Parameter));
    if (!struct_fields) panic(ERR_I001_OUT_OF_MEMORY, "allocating table struct fields");

    for (size_t i = 0; i < field_count; i++) {
        struct_fields[i].name_start = orig_fields[i].name_start;
        struct_fields[i].name_length = orig_fields[i].name_length;
        struct_fields[i].type = slice_table_intern(orig_fields[i].type);
        struct_fields[i].is_ref = false;
        struct_fields[i].is_mut_ref = false;
    }
    /* _len field */
    struct_fields[field_count].name_start = g_table_len_field;
    struct_fields[field_count].name_length = 4;
    struct_fields[field_count].type = TYPE_I64;
    struct_fields[field_count].is_ref = false;
    struct_fields[field_count].is_mut_ref = false;

    /* Register struct (columnar) type in g_value_table */
    Type struct_type = value_table_add(name_start, name_length,
                                       struct_fields, struct_field_count, loc,
                                       true, is_public);

    /* Synthesize row name: Name.Row */
    char *row_name = malloc(name_length + 4 + 1);
    if (!row_name) panic(ERR_I001_OUT_OF_MEMORY, "allocating row name");
    memcpy(row_name, name_start, name_length);
    memcpy(row_name + name_length, ".Row", 5);  /* includes null */

    /* Register row (value) type in g_value_table */
    Type row_type = value_table_add(row_name, name_length + 4,
                                     orig_fields, field_count, loc, false,
                                     is_public);

    /* Register in g_table_decls (orig_fields ownership transfers here) */
    table_decl_add(name_start, name_length, row_name,
                   struct_type, row_type, orig_fields, field_count,
                   is_public);

    return ast_make_value_decl(name_start, name_length, struct_fields,
                               struct_field_count, true, is_public, loc);
}

/* Resolve import path: std/ prefix -> compiler dir, else -> relative to current file */
static bool resolve_import_path(const char *import_path,
                                 const char *current_file,
                                 char *resolved, size_t resolved_size) {
    if (strncmp(import_path, "std/", 4) == 0) {
        snprintf(resolved, resolved_size, "%s/%s", g_compiler_dir, import_path);
    } else {
        /* Relative to current file's directory */
        char dir_buf[PATH_MAX];
        strncpy(dir_buf, current_file, sizeof(dir_buf) - 1);
        dir_buf[sizeof(dir_buf) - 1] = '\0';
        char *dir = dirname(dir_buf);
        snprintf(resolved, resolved_size, "%s/%s", dir, import_path);
    }
    /* Canonicalize */
    char canonical[PATH_MAX];
    if (realpath(resolved, canonical)) {
        strncpy(resolved, canonical, resolved_size - 1);
        resolved[resolved_size - 1] = '\0';
        return true;
    }
    return false;
}

/* Check if a file has already been imported */
static bool is_already_imported(const char *resolved_path) {
    for (size_t i = 0; i < g_imported_count; i++) {
        if (strcmp(g_imported_files[i], resolved_path) == 0) return true;
    }
    return false;
}

/* Forward declaration: parser_parse_program needs to call parser_parse_import
   and parser_parse_import needs parser_parse_program's declaration-parsing logic */
static void parser_parse_declarations(Parser *parser, Ast *program);

/* Parse an imported file's declarations into the program AST */
static void parser_parse_import(const char *resolved_path,
                                 const char *alias, size_t alias_length,
                                 Ast *program) {
    /* Read the imported file (reuse read_file which checks fread return) */
    size_t length;
    char *source = read_file(resolved_path, &length);

    /* Keep source alive (tokens point into it) */
    if (g_import_source_count >= MAX_IMPORTS) {
        panic(ERR_I002_INTERNAL_ERROR, "too many imported files");
    }
    g_import_sources[g_import_source_count++] = source;

    /* Register as a module */
    if (g_module_count >= MAX_IMPORTS) {
        panic(ERR_I002_INTERNAL_ERROR, "too many modules");
    }
    size_t module_id = g_module_count;
    g_modules[module_id].path = resolved_path;
    g_modules[module_id].alias = alias;
    g_modules[module_id].alias_length = alias_length;
    g_module_count++;

    /* Register alias for the importing module (before parsing, so alias is
     * available even if the imported file re-imports something) */
    if (g_module_alias_count < MAX_IMPORTS * 4) {
        g_module_aliases[g_module_alias_count++] = (ModuleAlias){
            alias, alias_length, module_id, g_current_module_id
        };
    }

    /* Save and set source file and module context */
    const char *saved_source_file = g_source_file;
    size_t saved_module_id = g_current_module_id;
    g_source_file = resolved_path;
    g_current_module_id = module_id;

    /* Lex and parse */
    Lexer lexer;
    lexer_init(&lexer, source);
    Parser parser;
    parser_init(&parser, &lexer);

    /* Parse all declarations from the imported file */
    parser_parse_declarations(&parser, program);

    /* Restore source file and module context */
    g_source_file = saved_source_file;
    g_current_module_id = saved_module_id;
}

/* Parse top-level declarations from the current parser into the program AST.
 * Used by both parser_parse_program (main file) and parser_parse_import. */
static void parser_parse_declarations(Parser *parser, Ast *program) {
    while (!parser_check(parser, TOKEN_EOF) && !too_many_errors()) {
        /* Check for 'pub' prefix on declarations */
        bool is_public = false;
        if (parser_match(parser, TOKEN_PUB)) {
            is_public = true;
            /* pub import is an error */
            if (parser_check(parser, TOKEN_IMPORT)) {
                diagnostic(g_source_file, ERR_P057_PUB_IMPORT,
                           parser->previous.line, parser->previous.column,
                           "'pub' cannot be used on imports");
                parser_synchronize(parser);
                continue;
            }
            /* pub native is an error (native declarations are always module-private) */
            if (parser_check(parser, TOKEN_NATIVE)) {
                diagnostic(g_source_file, ERR_P056_PUB_NOT_DECLARATION,
                           parser->previous.line, parser->previous.column,
                           "'pub' cannot be used on native declarations"
                           " (use a pub wrapper function instead)");
                parser_synchronize(parser);
                continue;
            }
            /* pub must be followed by a declaration keyword */
            if (!parser_check(parser, TOKEN_FUNC) &&
                !parser_check(parser, TOKEN_VALUE) &&
                !parser_check(parser, TOKEN_STRUCT) &&
                !parser_check(parser, TOKEN_TABLE) &&
                !parser_check(parser, TOKEN_ENUM) &&
                !parser_check(parser, TOKEN_VAL) &&
                !parser_check(parser, TOKEN_MUT)) {
                diagnostic(g_source_file, ERR_P056_PUB_NOT_DECLARATION,
                           parser->previous.line, parser->previous.column,
                           "'pub' must be followed by a declaration "
                           "(func, value, struct, table, enum, val, or mut)");
                parser_synchronize(parser);
                continue;
            }
        }

        if (parser_match(parser, TOKEN_IMPORT)) {
            /* import alias "path.nore" */
            if (!parser_match(parser, TOKEN_IDENTIFIER)) {
                diagnostic(g_source_file, ERR_P058_EXPECTED_MODULE_ALIAS,
                           parser->current.line, parser->current.column,
                           "Expected module alias after 'import' "
                           "(e.g., import io \"std/io.nore\")");
                parser_synchronize(parser);
                continue;
            }
            const char *alias_start = parser->previous.start;
            size_t alias_length = parser->previous.length;

            if (!parser_match(parser, TOKEN_STRING)) {
                diagnostic(g_source_file, ERR_P046_EXPECTED_IMPORT_PATH, parser->current.line,
                           parser->current.column,
                           "Expected import path after module alias");
                parser_synchronize(parser);
                continue;
            }
            /* String token includes quotes, strip them */
            const char *path_start = parser->previous.start + 1;
            size_t path_len = parser->previous.length - 2;
            char import_path[PATH_MAX];
            if (path_len >= sizeof(import_path)) {
                diagnostic(g_source_file, ERR_P046_EXPECTED_IMPORT_PATH, parser->previous.line,
                           parser->previous.column, "Import path too long");
                continue;
            }
            memcpy(import_path, path_start, path_len);
            import_path[path_len] = '\0';

            /* Resolve the path */
            char resolved[PATH_MAX];
            if (!resolve_import_path(import_path, g_source_file,
                                      resolved, sizeof(resolved))) {
                diagnostic(g_source_file, ERR_D007_IMPORT_NOT_FOUND, parser->previous.line,
                           parser->previous.column,
                           "Cannot find imported file '%s'", import_path);
                continue;
            }

            /* If already imported, just register the alias and skip re-parsing */
            if (is_already_imported(resolved)) {
                size_t target = 0;
                for (size_t m = 0; m < g_module_count; m++) {
                    if (strcmp(g_modules[m].path, resolved) == 0) {
                        target = m;
                        break;
                    }
                }
                if (g_module_alias_count < MAX_IMPORTS * 4) {
                    g_module_aliases[g_module_alias_count++] = (ModuleAlias){
                        alias_start, alias_length, target, g_current_module_id
                    };
                }
                continue;
            }

            /* Record as imported */
            if (g_imported_count >= MAX_IMPORTS) {
                panic(ERR_I002_INTERNAL_ERROR, "too many imported files");
            }
            g_imported_files[g_imported_count] = strdup(resolved);
            if (!g_imported_files[g_imported_count]) {
                panic(ERR_I001_OUT_OF_MEMORY, "recording imported file path");
            }
            g_imported_count++;

            /* Parse the imported file's declarations into this program.
             * Use the strdup'd path from g_imported_files (stable pointer). */
            parser_parse_import(g_imported_files[g_imported_count - 1],
                                alias_start, alias_length, program);

        } else if (parser_match(parser, TOKEN_NATIVE)) {
            /* native func name(params): return_type */
            SourceLoc native_loc = token_loc(&parser->previous);
            if (!parser_match(parser, TOKEN_FUNC)) {
                diagnostic(g_source_file, ERR_P059_NATIVE_NOT_FUNC,
                           parser->current.line, parser->current.column,
                           "'native' must be followed by 'func'");
                parser_synchronize(parser);
                continue;
            }
            /* Parse function signature (name, params, return type) */
            if (!parser_match(parser, TOKEN_IDENTIFIER)) {
                diagnostic(g_source_file, ERR_P017_EXPECTED_FUNC_NAME,
                           parser->current.line, parser->current.column,
                           "Expected function name after 'native func'");
                parser_synchronize(parser);
                continue;
            }
            const char *native_name = parser->previous.start;
            size_t native_name_len = parser->previous.length;
            if (!parser_match(parser, TOKEN_LPAREN)) {
                diagnostic(g_source_file, ERR_P018_EXPECTED_LPAREN_FUNC,
                           parser->current.line, parser->current.column,
                           "Expected '(' after function name");
                parser_synchronize(parser);
                continue;
            }
            /* Native declarations keep the parsed signature for sema validation. */
            size_t pcount = 0;
            Parameter *params = parser_parse_param_list(parser, &pcount, TOKEN_RPAREN);
            if (!parser_match(parser, TOKEN_RPAREN)) {
                diagnostic(g_source_file, ERR_P019_EXPECTED_RPAREN_FUNC,
                           parser->current.line, parser->current.column,
                           "Expected ')' after parameters");
                if (params) free(params);
                parser_synchronize(parser);
                continue;
            }
            if (!parser_match(parser, TOKEN_COLON)) {
                diagnostic(g_source_file, ERR_P020_EXPECTED_COLON_FUNC,
                           parser->current.line, parser->current.column,
                           "Expected ':' before return type");
                if (params) free(params);
                parser_synchronize(parser);
                continue;
            }
            Type rtype = parser_parse_type(parser, true);
            if (rtype == TYPE_UNKNOWN) {
                diagnostic(g_source_file, ERR_P021_EXPECTED_RETURN_TYPE,
                           parser->current.line, parser->current.column,
                           "Expected return type");
                if (params) free(params);
                parser_synchronize(parser);
                continue;
            }
            /* Native must NOT have a body */
            if (parser_check(parser, TOKEN_EQUALS)) {
                diagnostic(g_source_file, ERR_P060_NATIVE_HAS_BODY,
                           parser->current.line, parser->current.column,
                           "Native function must not have a body");
                free(params);
                parser_synchronize(parser);
                continue;
            }
            Ast *ndecl = ast_make_native_decl(native_name, native_name_len,
                                              params, pcount, rtype,
                                              native_loc);
            if (ndecl) {
                ast_program_add_statement(program, ndecl);
            }
        } else if (parser_match(parser, TOKEN_FUNC)) {
            Ast *func = parser_parse_function(parser, is_public);
            if (func) {
                ast_program_add_statement(program, func);
            }
        } else if (parser_match(parser, TOKEN_VALUE) ||
                   parser_match(parser, TOKEN_STRUCT)) {
            bool is_struct = parser->previous.kind == TOKEN_STRUCT;
            Ast *decl = parser_parse_value_decl(parser, is_struct, is_public);
            if (decl) {
                ast_program_add_statement(program, decl);
            }
        } else if (parser_match(parser, TOKEN_TABLE)) {
            Ast *decl = parser_parse_table_decl(parser, is_public);
            if (decl) {
                ast_program_add_statement(program, decl);
            }
        } else if (parser_match(parser, TOKEN_ENUM)) {
            Ast *decl = parser_parse_enum_decl(parser, is_public);
            if (decl) {
                ast_program_add_statement(program, decl);
            }
        } else if (parser_match(parser, TOKEN_VAL) ||
                   parser_match(parser, TOKEN_MUT)) {
            bool is_mutable = parser->previous.kind == TOKEN_MUT;
            Ast *var = parser_parse_var_declaration(parser, is_mutable,
                                                    is_public);
            if (var) {
                ast_program_add_statement(program, var);
            }
        } else {
            diagnostic(g_source_file, ERR_P016_EXPECTED_FUNC, parser->current.line,
                       parser->current.column,
                       "Expected declaration at top level");
            /* Skip tokens until we find a declaration or EOF */
            while (!parser_check(parser, TOKEN_FUNC) &&
                   !parser_check(parser, TOKEN_VALUE) &&
                   !parser_check(parser, TOKEN_STRUCT) &&
                   !parser_check(parser, TOKEN_TABLE) &&
                   !parser_check(parser, TOKEN_ENUM) &&
                   !parser_check(parser, TOKEN_IMPORT) &&
                   !parser_check(parser, TOKEN_VAL) &&
                   !parser_check(parser, TOKEN_MUT) &&
                   !parser_check(parser, TOKEN_PUB) &&
                   !parser_check(parser, TOKEN_NATIVE) &&
                   !parser_check(parser, TOKEN_EOF)) {
                parser_advance(parser);
            }
        }
    }
}

static Ast *parser_parse_program(Parser *parser) {
    /* Register the main file as module 0 */
    g_modules[0].path = g_source_file;
    g_modules[0].alias = NULL;
    g_modules[0].alias_length = 0;
    g_module_count = 1;
    g_current_module_id = 0;

    Ast *program = ast_make_program();
    parser_parse_declarations(parser, program);
    return program;
}

/* ============================== AST Printer =============================== */

static void parser_print_ast_step(Ast *node, int indent);

/* Keep debug dumps readable when one arm carries several pattern labels. */
static void parser_print_match_arm_pattern(const MatchArmPattern *pattern,
                                           int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }

    switch (pattern->kind) {
        case MATCH_PATTERN_ENUM_VARIANT:
            printf("PATTERN_VARIANT(%.*s)\n",
                   (int)pattern->variant_length,
                   pattern->variant_name);
            break;
        case MATCH_PATTERN_LITERAL:
            printf("PATTERN_LITERAL\n");
            parser_print_ast_step(pattern->pattern_expr, indent + 1);
            break;
        case MATCH_PATTERN_WILDCARD:
            printf("PATTERN_WILDCARD\n");
            break;
        case MATCH_PATTERN_ELSE:
            printf("PATTERN_ELSE\n");
            break;
    }
}

static void parser_print_ast_step(Ast *node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; i++) {
        printf("  ");
    }

    switch (node->kind) {
        case AST_NUMBER:
            printf("NUMBER(%ld)\n", node->as.number.value);
            break;

        case AST_FLOAT:
            printf("FLOAT(%g)\n", node->as.float_lit.value);
            break;

        case AST_BOOLEAN:
            printf("BOOLEAN(%s)\n", node->as.boolean.value ? "true" : "false");
            break;

        case AST_IDENTIFIER:
            printf("IDENTIFIER(%.*s)\n",
                   (int)node->as.identifier.length,
                   node->as.identifier.start);
            break;

        case AST_BINARY: {
            printf("BINARY(%s)\n", binary_op_name(node->as.binary.op));
            parser_print_ast_step(node->as.binary.left, indent + 1);
            parser_print_ast_step(node->as.binary.right, indent + 1);
            break;
        }

        case AST_UNARY: {
            const char *op_name;
            switch (node->as.unary.op) {
                case OP_NEG:    op_name = "NEG"; break;
                case OP_NOT:    op_name = "NOT"; break;
                case OP_BITNOT: op_name = "BITNOT"; break;
            }
            printf("UNARY(%s)\n", op_name);
            parser_print_ast_step(node->as.unary.operand, indent + 1);
            break;
        }

        case AST_FUNC_CALL:
            printf("FUNC_CALL(%.*s)\n",
                   (int)node->as.func_call.name_length,
                   node->as.func_call.name_start);
            for (size_t i = 0; i < node->as.func_call.arg_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                bool is_ref = node->as.func_call.arg_is_ref &&
                              node->as.func_call.arg_is_ref[i];
                bool is_mut = node->as.func_call.arg_is_mut_ref &&
                              node->as.func_call.arg_is_mut_ref[i];
                if (is_mut) printf("MUT_REF_ARG:\n");
                else if (is_ref) printf("REF_ARG:\n");
                else printf("ARG:\n");
                parser_print_ast_step(node->as.func_call.arguments[i], indent + 2);
            }
            break;

        case AST_VAL_DECL:
            printf("VAL_DECL(%.*s)\n",
                   (int)node->as.val_decl.name_length,
                   node->as.val_decl.name_start);
            parser_print_ast_step(node->as.val_decl.initializer, indent + 1);
            break;

        case AST_MUT_DECL:
            printf("MUT_DECL(%.*s)\n",
                   (int)node->as.mut_decl.name_length,
                   node->as.mut_decl.name_start);
            parser_print_ast_step(node->as.mut_decl.initializer, indent + 1);
            break;

        case AST_RETURN:
            printf("RETURN\n");
            if (node->as.return_stmt.value) {
                parser_print_ast_step(node->as.return_stmt.value, indent + 1);
            }
            break;

        case AST_ASSIGNMENT:
            printf("ASSIGNMENT\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("TARGET:\n");
            parser_print_ast_step(node->as.assignment.target, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("VALUE:\n");
            parser_print_ast_step(node->as.assignment.value, indent + 2);
            break;

        case AST_COMPOUND_ASSIGNMENT:
            printf("COMPOUND_ASSIGNMENT(%s)\n",
                   binary_op_name(node->as.compound_assignment.op));
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("TARGET:\n");
            parser_print_ast_step(node->as.compound_assignment.target, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("VALUE:\n");
            parser_print_ast_step(node->as.compound_assignment.value, indent + 2);
            break;

        case AST_ASSERT:
            printf("ASSERT\n");
            parser_print_ast_step(node->as.assert_stmt.condition, indent + 1);
            break;

        case AST_IF:
            printf("IF\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("COND:\n");
            parser_print_ast_step(node->as.if_stmt.condition, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("THEN:\n");
            parser_print_ast_step(node->as.if_stmt.then_block, indent + 2);
            if (node->as.if_stmt.else_block) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("ELSE:\n");
                parser_print_ast_step(node->as.if_stmt.else_block, indent + 2);
            }
            break;

        case AST_WHILE:
            printf("WHILE\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("COND:\n");
            parser_print_ast_step(node->as.while_stmt.condition, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("BODY:\n");
            parser_print_ast_step(node->as.while_stmt.body, indent + 2);
            break;

        case AST_FOR:
            printf("FOR %.*s\n", (int)node->as.for_stmt.var_length,
                   node->as.for_stmt.var_start);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("START:\n");
            parser_print_ast_step(node->as.for_stmt.start, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("END:\n");
            parser_print_ast_step(node->as.for_stmt.end, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("BODY:\n");
            parser_print_ast_step(node->as.for_stmt.body, indent + 2);
            break;

        case AST_BREAK:
            printf("BREAK\n");
            break;

        case AST_CONTINUE:
            printf("CONTINUE\n");
            break;

        case AST_BLOCK:
            printf("BLOCK\n");
            for (size_t i = 0; i < node->as.block.count; i++) {
                parser_print_ast_step(node->as.block.statements[i], indent + 1);
            }
            if (node->as.block.value_expr) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("VALUE:\n");
                parser_print_ast_step(node->as.block.value_expr, indent + 2);
            }
            break;

        case AST_FUNC_DECL:
            printf("FUNC_DECL(%.*s) -> %s\n",
                   (int)node->as.func_decl.name_length,
                   node->as.func_decl.name_start,
                   type_name(node->as.func_decl.return_type));
            for (size_t i = 0; i < node->as.func_decl.param_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                Parameter *p = &node->as.func_decl.params[i];
                const char *ref_prefix = "";
                if (p->is_mut_ref) ref_prefix = "mut ref ";
                else if (p->is_ref) ref_prefix = "ref ";
                printf("PARAM(%s%.*s: %s)\n", ref_prefix,
                       (int)p->name_length, p->name_start,
                       type_name(p->type));
            }
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("BODY:\n");
            parser_print_ast_step(node->as.func_decl.body, indent + 2);
            break;

        case AST_VALUE_DECL:
            printf("%s(%.*s)\n",
                   node->as.value_decl.is_struct ? "STRUCT_DECL" : "VALUE_DECL",
                   (int)node->as.value_decl.name_length,
                   node->as.value_decl.name_start);
            for (size_t i = 0; i < node->as.value_decl.field_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                Parameter *f = &node->as.value_decl.fields[i];
                printf("FIELD(%.*s: %s)\n",
                       (int)f->name_length, f->name_start,
                       type_name(f->type));
            }
            break;

        case AST_VALUE_CONSTRUCTOR:
            printf("VALUE_CONSTRUCTOR(%.*s)\n",
                   (int)node->as.value_constructor.type_name_length,
                   node->as.value_constructor.type_name_start);
            for (size_t i = 0; i < node->as.value_constructor.field_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                FieldInit *fi = &node->as.value_constructor.fields[i];
                printf("FIELD_INIT(%.*s):\n",
                       (int)fi->name_length, fi->name_start);
                parser_print_ast_step(fi->value, indent + 2);
            }
            break;

        case AST_FIELD_ACCESS:
            printf("FIELD_ACCESS(.%.*s)\n",
                   (int)node->as.field_access.field_length,
                   node->as.field_access.field_start);
            parser_print_ast_step(node->as.field_access.object, indent + 1);
            break;

        case AST_ARRAY_LITERAL:
            printf("ARRAY_LITERAL(%zu elements)\n",
                   node->as.array_literal.element_count);
            for (size_t i = 0; i < node->as.array_literal.element_count; i++) {
                parser_print_ast_step(node->as.array_literal.elements[i], indent + 1);
            }
            break;

        case AST_INDEX_ACCESS:
            printf("INDEX_ACCESS\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("OBJECT:\n");
            parser_print_ast_step(node->as.index_access.object, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("INDEX:\n");
            parser_print_ast_step(node->as.index_access.index, indent + 2);
            break;

        case AST_SLICE_ACCESS:
            printf("SLICE_ACCESS\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("OBJECT:\n");
            parser_print_ast_step(node->as.slice_access.object, indent + 2);
            if (node->as.slice_access.start) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("START:\n");
                parser_print_ast_step(node->as.slice_access.start, indent + 2);
            }
            if (node->as.slice_access.end) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("END:\n");
                parser_print_ast_step(node->as.slice_access.end, indent + 2);
            }
            break;

        case AST_STRING_LITERAL:
            printf("STRING_LITERAL(\"%.*s\")\n",
                   (int)node->as.string_literal.raw_length,
                   node->as.string_literal.start);
            break;

        case AST_ARENA_NEW:
            printf("ARENA_NEW\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("CAPACITY:\n");
            parser_print_ast_step(node->as.arena_new.capacity, indent + 2);
            break;

        case AST_ARENA_ALLOC:
            printf("ARENA_ALLOC\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("ARENA:\n");
            parser_print_ast_step(node->as.arena_alloc.arena, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("COUNT:\n");
            parser_print_ast_step(node->as.arena_alloc.count, indent + 2);
            break;

        case AST_ARENA_RESET:
            printf("ARENA_RESET\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("ARENA:\n");
            parser_print_ast_step(node->as.arena_reset.arena, indent + 2);
            break;

        case AST_TABLE_ALLOC:
            printf("TABLE_ALLOC\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("ARENA:\n");
            parser_print_ast_step(node->as.table_alloc.arena, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("COUNT:\n");
            parser_print_ast_step(node->as.table_alloc.count, indent + 2);
            break;

        case AST_TABLE_LEN:
            printf("TABLE_LEN\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("TABLE:\n");
            parser_print_ast_step(node->as.table_len.table, indent + 2);
            break;

        case AST_TABLE_GET:
            printf("TABLE_GET\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("TABLE:\n");
            parser_print_ast_step(node->as.table_get.table, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("INDEX:\n");
            parser_print_ast_step(node->as.table_get.index, indent + 2);
            break;

        case AST_TABLE_INSERT:
            printf("TABLE_INSERT\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("TABLE:\n");
            parser_print_ast_step(node->as.table_insert.table, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("ROW:\n");
            parser_print_ast_step(node->as.table_insert.row, indent + 2);
            break;

        case AST_TYPE_CAST:
            printf("CAST(%s)\n", type_name(node->as.type_cast.target_type));
            parser_print_ast_step(node->as.type_cast.operand, indent + 1);
            break;

        case AST_FD_WRITE:
            printf("FD_WRITE\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("FD:\n");
            parser_print_ast_step(node->as.fd_write.fd, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("DATA:\n");
            parser_print_ast_step(node->as.fd_write.data, indent + 2);
            break;

        case AST_FD_READ:
            printf("FD_READ\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("FD:\n");
            parser_print_ast_step(node->as.fd_read.fd, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("BUF:\n");
            parser_print_ast_step(node->as.fd_read.buf, indent + 2);
            break;

        case AST_FD_OPEN:
            printf("FD_OPEN\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("PATH:\n");
            parser_print_ast_step(node->as.fd_open.path, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("FLAGS:\n");
            parser_print_ast_step(node->as.fd_open.flags, indent + 2);
            break;

        case AST_FD_CLOSE:
            printf("FD_CLOSE\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("FD:\n");
            parser_print_ast_step(node->as.fd_close.fd, indent + 2);
            break;

        case AST_FD_SEEK:
            printf("FD_SEEK\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("FD:\n");
            parser_print_ast_step(node->as.fd_seek.fd, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("OFFSET:\n");
            parser_print_ast_step(node->as.fd_seek.offset, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("WHENCE:\n");
            parser_print_ast_step(node->as.fd_seek.whence, indent + 2);
            break;

        case AST_EXIT:
            printf("EXIT\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("CODE:\n");
            parser_print_ast_step(node->as.exit_call.code, indent + 2);
            break;

        case AST_MEM_COPY:
            printf("MEM_COPY\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("DST:\n");
            parser_print_ast_step(node->as.mem_copy.dst, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("SRC:\n");
            parser_print_ast_step(node->as.mem_copy.src, indent + 2);
            break;

        case AST_ARGS:
            printf("ARGS\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("ARENA:\n");
            parser_print_ast_step(node->as.args.arena, indent + 2);
            break;

        case AST_SHELL_RUN:
            printf("SHELL_RUN\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("COMMAND:\n");
            parser_print_ast_step(node->as.shell_run.command, indent + 2);
            break;

        case AST_EXE_DIR:
            printf("EXE_DIR\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("ARENA:\n");
            parser_print_ast_step(node->as.exe_dir.arena, indent + 2);
            break;

        case AST_PID:
            printf("PID\n");
            break;

        case AST_TIME_NS:
            printf("TIME_NS\n");
            break;

        case AST_ENUM_DECL:
            printf("ENUM_DECL(%.*s, %zu variants)\n",
                   (int)node->as.enum_decl.name_length,
                   node->as.enum_decl.name_start,
                   node->as.enum_decl.variant_count);
            break;

        case AST_ENUM_VARIANT: {
            EnumTypeEntry *et = enum_table_get(node->as.enum_variant.enum_type);
            printf("ENUM_VARIANT(%s.%ld)\n",
                   et ? type_name(node->as.enum_variant.enum_type) : "?",
                   node->as.enum_variant.value);
            if (node->as.enum_variant.payload) {
                parser_print_ast_step(node->as.enum_variant.payload, indent + 1);
            }
            break;
        }

        case AST_MATCH:
            printf("MATCH\n");
            parser_print_ast_step(node->as.match_expr.scrutinee, indent + 1);
            for (size_t i = 0; i < node->as.match_expr.arm_count; i++)
                parser_print_ast_step(node->as.match_expr.arms[i], indent + 1);
            break;

        case AST_MATCH_ARM:
            printf("ARM\n");
            for (size_t i = 0; i < node->as.match_arm.pattern_count; i++) {
                parser_print_match_arm_pattern(
                    &node->as.match_arm.patterns[i],
                    indent + 1);
            }
            parser_print_ast_step(node->as.match_arm.body, indent + 1);
            break;

        case AST_NATIVE_DECL:
            printf("NATIVE_DECL(%.*s) -> %s\n",
                   (int)node->as.native_decl.name_length,
                   node->as.native_decl.name_start,
                   type_name(node->as.native_decl.return_type));
            for (size_t i = 0; i < node->as.native_decl.param_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                Parameter *p = &node->as.native_decl.params[i];
                const char *ref_prefix = "";
                if (p->is_mut_ref) ref_prefix = "mut ref ";
                else if (p->is_ref) ref_prefix = "ref ";
                printf("PARAM(%s%.*s: %s)\n", ref_prefix,
                       (int)p->name_length, p->name_start,
                       type_name(p->type));
            }
            break;

        case AST_PROGRAM:
            printf("PROGRAM\n");
            for (size_t i = 0; i < node->as.program.count; i++) {
                parser_print_ast_step(node->as.program.statements[i], indent + 1);
            }
            break;
    }
}

static void parser_print_ast(Ast *ast) {
    printf("=== AST ===\n");
    parser_print_ast_step(ast, 0);
    printf("\n");
}

/* ============================= Scope Management =========================== */

/* Variable and Scope structs (defined here for use in folding) */
struct Variable {
    const char *name_start;
    size_t name_length;
    bool is_mutable;
    bool is_ref;
    Type type;
    bool is_comptime;
    union {
        long int_value;
        double float_value;
    } comptime_value;
    bool is_global;                   /* true for global variables */
    bool is_public;                   /* true if declared with pub */
    size_t module_id;
    bool arena_is_local;              /* for slices: true if arena is local (not ref param) */
    const char *arena_source_start;   /* for slices: name of source arena */
    size_t arena_source_length;
    bool is_invalidated;              /* for slices: true after source arena reset */
};
typedef struct Variable Variable;

struct Scope {
    Variable *vars;
    size_t count;
    size_t capacity;
    struct Scope *parent;
    int loop_depth;
};
typedef struct Scope Scope;

static Scope *scope_create(Scope *parent) {
    Scope *scope = malloc(sizeof(Scope));
    if (!scope) {
        panic(ERR_I001_OUT_OF_MEMORY, "allocating scope");
    }
    scope->vars = malloc(8 * sizeof(Variable));
    if (!scope->vars) {
        free(scope);
        panic(ERR_I001_OUT_OF_MEMORY, "allocating scope variables");
    }
    scope->count = 0;
    scope->capacity = 8;
    scope->parent = parent;
    scope->loop_depth = parent ? parent->loop_depth : 0;
    return scope;
}

static void scope_destroy(Scope *scope) {
    free(scope->vars);
    free(scope);
}

/* Lookup in current scope only (for duplicate detection) */
static Variable *scope_lookup_local(Scope *scope, const char *name_start,
                                     size_t name_length) {
    for (size_t i = 0; i < scope->count; i++) {
        if (scope->vars[i].name_length == name_length &&
            memcmp(scope->vars[i].name_start, name_start, name_length) == 0) {
            return &scope->vars[i];
        }
    }
    return NULL;
}

/* Lookup walking up the scope chain (for variable resolution) */
static Variable *scope_lookup(Scope *scope, const char *name_start,
                               size_t name_length) {
    while (scope != NULL) {
        Variable *v = scope_lookup_local(scope, name_start, name_length);
        if (v != NULL) {
            return v;
        }
        scope = scope->parent;
    }
    return NULL;
}

static Variable *scope_add(Scope *scope, const char *name_start,
                           size_t name_length, bool is_mutable, Type type,
                           SourceLoc loc) {
    /* Check for duplicates in current scope only */
    if (scope_lookup_local(scope, name_start, name_length) != NULL) {
        diagnostic(loc.file, ERR_S001_DUPLICATE_VARIABLE, loc.line, loc.column,
                   "Variable '%.*s' already declared in this scope",
                   (int)name_length, name_start);
        return NULL;
    }

    /* Grow if needed */
    if (scope->count >= scope->capacity) {
        scope->capacity *= 2;
        scope->vars = realloc(scope->vars, scope->capacity * sizeof(Variable));
        if (!scope->vars) {
            panic(ERR_I001_OUT_OF_MEMORY, "growing scope variables");
        }
    }

    /* Add variable */
    Variable *v = &scope->vars[scope->count];
    v->name_start = name_start;
    v->name_length = name_length;
    v->is_mutable = is_mutable;
    v->is_ref = false;
    v->type = type;
    v->is_comptime = false;
    v->is_global = false;
    v->is_public = false;
    v->module_id = g_current_module_id;
    v->arena_is_local = false;
    v->arena_source_start = NULL;
    v->arena_source_length = 0;
    v->is_invalidated = false;
    scope->count++;
    return v;
}

static void scope_add_comptime_int(Scope *scope, const char *name_start,
                                   size_t name_length, long value,
                                   SourceLoc loc) {
    Variable *v = scope_add(scope, name_start, name_length, false,
                            TYPE_COMPTIME_INT, loc);
    if (v != NULL) {
        v->is_comptime = true;
        v->comptime_value.int_value = value;
    }
}

static void scope_add_comptime_float(Scope *scope, const char *name_start,
                                     size_t name_length, double value,
                                     SourceLoc loc) {
    Variable *v = scope_add(scope, name_start, name_length, false,
                            TYPE_COMPTIME_FLOAT, loc);
    if (v != NULL) {
        v->is_comptime = true;
        v->comptime_value.float_value = value;
    }
}

static const SourceLoc BUILTIN_LOC = {0, 0, NULL};

static void scope_add_comptime_enum(Scope *scope, const char *name_start,
                                    size_t name_length, Type enum_type,
                                    long value, SourceLoc loc) {
    Variable *v = scope_add(scope, name_start, name_length, false,
                            enum_type, loc);
    if (v != NULL) {
        v->is_comptime = true;
        v->comptime_value.int_value = value;
    }
}

/* OS enum type, registered once before parsing */
static Type g_os_enum_type = TYPE_UNKNOWN;

static void platform_inject_os_enum(void) {
    EnumVariant *variants = malloc(2 * sizeof(EnumVariant));
    if (!variants) panic(ERR_I001_OUT_OF_MEMORY, "allocating OS enum variants");
    variants[0] = (EnumVariant){"Linux", 5, 0, TYPE_VOID};
    variants[1] = (EnumVariant){"MacOS", 5, 1, TYPE_VOID};
    g_os_enum_type = enum_table_add("OS", 2, variants, 2, BUILTIN_LOC, false);
    /* Mark as built-in so it's visible from all modules */
    g_enum_table[g_enum_count - 1].module_id = MODULE_BUILTIN;
}

static void scope_inject_platform_constants(Scope *scope) {
#ifdef __APPLE__
    scope_add_comptime_enum(scope, "TARGET_OS", 9, g_os_enum_type, 1, BUILTIN_LOC);
#else
    scope_add_comptime_enum(scope, "TARGET_OS", 9, g_os_enum_type, 0, BUILTIN_LOC);
#endif
    /* Mark built-in constants as visible from all modules */
    Variable *target_os = scope_lookup(scope, "TARGET_OS", 9);
    if (target_os) target_os->module_id = MODULE_BUILTIN;
}

/* ========================== Function Table ========================== */

typedef struct {
    const char *name_start;
    size_t name_length;
    Type return_type;
    Type *param_types;
    bool *param_is_ref;
    bool *param_is_mut_ref;
    size_t param_count;
    SourceLoc loc;
    bool returns_arena_slices;
    bool is_public;
    size_t module_id;
} FunctionEntry;

typedef struct {
    FunctionEntry *functions;
    size_t count;
    size_t capacity;
} FunctionTable;

static FunctionTable *func_table_create(void) {
    FunctionTable *table = malloc(sizeof(FunctionTable));
    if (!table) panic(ERR_I001_OUT_OF_MEMORY, "allocating function table");
    table->functions = malloc(8 * sizeof(FunctionEntry));
    if (!table->functions) {
        free(table);
        panic(ERR_I001_OUT_OF_MEMORY, "allocating function entries");
    }
    table->count = 0;
    table->capacity = 8;
    return table;
}

static void func_table_destroy(FunctionTable *table) {
    for (size_t i = 0; i < table->count; i++) {
        free(table->functions[i].param_types);
        free(table->functions[i].param_is_ref);
        free(table->functions[i].param_is_mut_ref);
    }
    free(table->functions);
    free(table);
}

static FunctionEntry *func_table_lookup(FunctionTable *table,
                                        const char *name_start, size_t name_length) {
    for (size_t i = 0; i < table->count; i++) {
        if (table->functions[i].name_length == name_length &&
            memcmp(table->functions[i].name_start, name_start, name_length) == 0) {
            return &table->functions[i];
        }
    }
    return NULL;
}

static FunctionEntry *func_table_lookup_in_module(FunctionTable *table,
                                                    const char *name_start,
                                                    size_t name_length,
                                                    size_t module_id) {
    for (size_t i = 0; i < table->count; i++) {
        if (table->functions[i].module_id == module_id &&
            table->functions[i].name_length == name_length &&
            memcmp(table->functions[i].name_start, name_start, name_length) == 0) {
            return &table->functions[i];
        }
    }
    return NULL;
}

static void func_table_add(FunctionTable *table, Ast *func_decl) {
    const char *name_start = func_decl->as.func_decl.name_start;
    size_t name_length = func_decl->as.func_decl.name_length;

    /* Derive module_id early for per-module duplicate check */
    size_t mod = module_for_file(func_decl->loc.file);

    /* Check for duplicates within the same module */
    if (func_table_lookup_in_module(table, name_start, name_length, mod) != NULL) {
        diagnostic(func_decl->loc.file, ERR_S008_DUPLICATE_FUNCTION, func_decl->loc.line,
                   func_decl->loc.column, "Duplicate function '%.*s'",
                   (int)name_length, name_start);
        return;
    }

    /* Grow if needed */
    if (table->count >= table->capacity) {
        table->capacity *= 2;
        table->functions = realloc(table->functions,
                                   table->capacity * sizeof(FunctionEntry));
        if (!table->functions) panic(ERR_I001_OUT_OF_MEMORY, "growing function table");
    }

    FunctionEntry *entry = &table->functions[table->count++];
    entry->name_start = name_start;
    entry->name_length = name_length;
    entry->return_type = func_decl->as.func_decl.return_type;
    entry->loc = func_decl->loc;
    entry->returns_arena_slices = false;
    entry->is_public = func_decl->as.func_decl.is_public;
    entry->module_id = mod;

    /* Store parameter types and ref flags */
    size_t param_count = func_decl->as.func_decl.param_count;
    entry->param_count = param_count;
    if (param_count > 0) {
        entry->param_types = malloc(param_count * sizeof(Type));
        if (!entry->param_types) panic(ERR_I001_OUT_OF_MEMORY, "allocating param types");
        entry->param_is_ref = malloc(param_count * sizeof(bool));
        if (!entry->param_is_ref) panic(ERR_I001_OUT_OF_MEMORY, "allocating param ref flags");
        entry->param_is_mut_ref = malloc(param_count * sizeof(bool));
        if (!entry->param_is_mut_ref) panic(ERR_I001_OUT_OF_MEMORY, "allocating param mut ref flags");
        for (size_t i = 0; i < param_count; i++) {
            entry->param_types[i] = func_decl->as.func_decl.params[i].type;
            entry->param_is_ref[i] = func_decl->as.func_decl.params[i].is_ref;
            entry->param_is_mut_ref[i] = func_decl->as.func_decl.params[i].is_mut_ref;
        }
    } else {
        entry->param_types = NULL;
        entry->param_is_ref = NULL;
        entry->param_is_mut_ref = NULL;
    }
}

/* Current function entry for escape analysis (set during typecheck_function) */
static FunctionEntry *g_current_func_entry = NULL;

/* Current module being typechecked (for module-aware lookups) */
static size_t g_typecheck_module_id = 0;

/* ============================= Constant Folding ============================ */

/* Returns true if node is a compile-time constant (literal or comptime variable) */
static bool is_comptime_constant(Ast *node, Scope *scope) {
    if (node->kind == AST_NUMBER || node->kind == AST_FLOAT ||
        node->kind == AST_BOOLEAN) {
        return true;
    }
    if (node->kind == AST_ENUM_VARIANT) {
        return node->as.enum_variant.payload == NULL;
    }
    if (node->kind == AST_IDENTIFIER && scope != NULL) {
        Variable *v = scope_lookup(scope, node->as.identifier.start,
                                   node->as.identifier.length);
        return v != NULL && v->is_comptime;
    }
    return false;
}

/* Get integer value from comptime node */
static long get_comptime_int(Ast *node, Scope *scope) {
    if (node->kind == AST_NUMBER) return node->as.number.value;
    if (node->kind == AST_BOOLEAN) return node->as.boolean.value ? 1L : 0L;
    if (node->kind == AST_FLOAT) return (long)node->as.float_lit.value;
    if (node->kind == AST_ENUM_VARIANT) return node->as.enum_variant.value;
    if (node->kind == AST_IDENTIFIER && scope != NULL) {
        Variable *v = scope_lookup(scope, node->as.identifier.start,
                                   node->as.identifier.length);
        if (v != NULL && v->is_comptime) {
            return v->comptime_value.int_value;
        }
    }
    return 0;
}

/* Get float value from comptime node */
static double get_comptime_float(Ast *node, Scope *scope) {
    if (node->kind == AST_FLOAT) return node->as.float_lit.value;
    if (node->kind == AST_NUMBER) return (double)node->as.number.value;
    if (node->kind == AST_IDENTIFIER && scope != NULL) {
        Variable *v = scope_lookup(scope, node->as.identifier.start,
                                   node->as.identifier.length);
        if (v != NULL && v->is_comptime) {
            if (v->type == TYPE_COMPTIME_FLOAT) {
                return v->comptime_value.float_value;
            } else {
                return (double)v->comptime_value.int_value;
            }
        }
    }
    return 0.0;
}

/* Check if node evaluates to int (for determining result type) */
static bool is_comptime_int_type(Ast *node, Scope *scope) {
    if (node->kind == AST_NUMBER) return true;
    if (node->kind == AST_FLOAT) return false;
    if (node->kind == AST_ENUM_VARIANT) return true;
    if (node->kind == AST_IDENTIFIER && scope != NULL) {
        Variable *v = scope_lookup(scope, node->as.identifier.start,
                                   node->as.identifier.length);
        if (v != NULL && v->is_comptime) {
            return v->type == TYPE_COMPTIME_INT || type_is_enum(v->type);
        }
    }
    return true;  /* default to int */
}

static bool int_add_overflows(long l, long r) {
    return (r > 0 && l > INT64_MAX - r) || (r < 0 && l < INT64_MIN - r);
}

static bool int_sub_overflows(long l, long r) {
    return (r < 0 && l > INT64_MAX + r) || (r > 0 && l < INT64_MIN + r);
}

static bool int_mul_overflows(long l, long r) {
    if (r == 0) return false;
    return (r == -1 && l == INT64_MIN) ||
           (l > 0 && r > 0 && l > INT64_MAX / r) ||
           (l < 0 && r < 0 && l < INT64_MAX / r) ||
           (l > 0 && r < 0 && r < INT64_MIN / l) ||
           (l < 0 && r > 0 && l < INT64_MIN / r);
}

/* Try to fold a binary arithmetic operation. Returns folded node or NULL.
 * Sets *div_by_zero to true if division by zero detected. */
static Ast *try_fold_binary(Ast *node, Scope *scope, bool *div_by_zero) {
    *div_by_zero = false;

    Ast *left = node->as.binary.left;
    Ast *right = node->as.binary.right;

    if (!is_comptime_constant(left, scope) || !is_comptime_constant(right, scope)) {
        return NULL;
    }

    BinaryOp op = node->as.binary.op;

    /* Only fold arithmetic and bitwise operations */
    if (op != OP_ADD && op != OP_SUB && op != OP_MUL && op != OP_DIV && op != OP_MOD &&
        op != OP_BITAND && op != OP_BITOR && op != OP_BITXOR && op != OP_SHL && op != OP_SHR) {
        return NULL;
    }

    /* Check for division by zero */
    if (op == OP_DIV || op == OP_MOD) {
        long r_int = get_comptime_int(right, scope);
        double r_float = get_comptime_float(right, scope);
        if (r_int == 0 && r_float == 0.0) {
            *div_by_zero = true;
            return NULL;
        }
    }

    /* Determine if result is int or float */
    bool result_is_float = !is_comptime_int_type(left, scope) ||
                           !is_comptime_int_type(right, scope);

    if (result_is_float) {
        double l = get_comptime_float(left, scope);
        double r = get_comptime_float(right, scope);
        double result;

        switch (op) {
            case OP_ADD: result = l + r; break;
            case OP_SUB: result = l - r; break;
            case OP_MUL: result = l * r; break;
            case OP_DIV: result = l / r; break;
            default: return NULL;
        }

        Ast *folded = ast_make_float(result, node->loc);
        folded->expr_type = TYPE_COMPTIME_FLOAT;
        return folded;
    } else {
        long l = get_comptime_int(left, scope);
        long r = get_comptime_int(right, scope);
        bool overflow = false;
        long result = 0;

        switch (op) {
            case OP_ADD: overflow = int_add_overflows(l, r); if (!overflow) result = l + r; break;
            case OP_SUB: overflow = int_sub_overflows(l, r); if (!overflow) result = l - r; break;
            case OP_MUL: overflow = int_mul_overflows(l, r); if (!overflow) result = l * r; break;
            case OP_DIV: result = l / r; break;
            case OP_MOD: result = l % r; break;
            case OP_BITAND: result = l & r; break;
            case OP_BITOR:  result = l | r; break;
            case OP_BITXOR: result = l ^ r; break;
            case OP_SHL:
            case OP_SHR:
                if (r < 0 || r >= 64) {
                    overflow = true;
                } else if (op == OP_SHL && l < 0) {
                    overflow = true;
                } else if (op == OP_SHL) {
                    result = l << r;
                } else {
                    result = l >> r;
                }
                break;
            default: return NULL;
        }
        if (overflow) {
            diagnostic(node->loc.file, ERR_S050_LITERAL_OUT_OF_RANGE, node->loc.line,
                       node->loc.column,
                       "Integer overflow in constant expression");
            return NULL;
        }

        Ast *folded = ast_make_number(result, node->loc);
        folded->expr_type = TYPE_COMPTIME_INT;
        return folded;
    }
}

/* Try to fold a comparison operation. Returns folded bool node or NULL. */
static Ast *try_fold_comparison(Ast *node, Scope *scope) {
    Ast *left = node->as.binary.left;
    Ast *right = node->as.binary.right;

    if (!is_comptime_constant(left, scope) || !is_comptime_constant(right, scope)) {
        return NULL;
    }

    BinaryOp op = node->as.binary.op;
    if (op < OP_EQ || op > OP_GE) {
        return NULL;
    }

    bool result_is_float = !is_comptime_int_type(left, scope) ||
                           !is_comptime_int_type(right, scope);
    bool result;

    if (result_is_float) {
        double l = get_comptime_float(left, scope);
        double r = get_comptime_float(right, scope);
        switch (op) {
            case OP_EQ:  result = l == r; break;
            case OP_NEQ: result = l != r; break;
            case OP_LT:  result = l < r; break;
            case OP_GT:  result = l > r; break;
            case OP_LE:  result = l <= r; break;
            case OP_GE:  result = l >= r; break;
            default: return NULL;
        }
    } else {
        long l = get_comptime_int(left, scope);
        long r = get_comptime_int(right, scope);
        switch (op) {
            case OP_EQ:  result = l == r; break;
            case OP_NEQ: result = l != r; break;
            case OP_LT:  result = l < r; break;
            case OP_GT:  result = l > r; break;
            case OP_LE:  result = l <= r; break;
            case OP_GE:  result = l >= r; break;
            default: return NULL;
        }
    }

    Ast *folded = ast_make_boolean(result, node->loc);
    folded->expr_type = TYPE_BOOL;
    return folded;
}

/* Try to fold a unary operation. Returns folded node or NULL. */
static Ast *try_fold_unary(Ast *node, Scope *scope) {
    Ast *operand = node->as.unary.operand;

    if (!is_comptime_constant(operand, scope)) return NULL;

    if (node->as.unary.op == OP_NEG) {
        if (is_comptime_int_type(operand, scope)) {
            Ast *folded = ast_make_number(-get_comptime_int(operand, scope), node->loc);
            folded->expr_type = TYPE_COMPTIME_INT;
            return folded;
        } else {
            Ast *folded = ast_make_float(-get_comptime_float(operand, scope), node->loc);
            folded->expr_type = TYPE_COMPTIME_FLOAT;
            return folded;
        }
    }

    if (node->as.unary.op == OP_NOT && operand->kind == AST_BOOLEAN) {
        Ast *folded = ast_make_boolean(!operand->as.boolean.value, node->loc);
        folded->expr_type = TYPE_BOOL;
        return folded;
    }

    if (node->as.unary.op == OP_BITNOT) {
        if (is_comptime_int_type(operand, scope)) {
            Ast *folded = ast_make_number(~get_comptime_int(operand, scope), node->loc);
            folded->expr_type = TYPE_COMPTIME_INT;
            return folded;
        }
    }

    return NULL;
}

/* Try to fold an if expression with comptime condition.
 * Returns the value expression from the chosen branch, or NULL. */
static Ast *try_fold_if_expr(Ast *node, Scope *scope) {
    /* Condition must be a boolean literal */
    if (node->as.if_stmt.condition->kind != AST_BOOLEAN) {
        return NULL;
    }

    /* Must have else branch for value */
    if (!node->as.if_stmt.else_block) {
        return NULL;
    }

    bool cond_value = node->as.if_stmt.condition->as.boolean.value;
    Ast *chosen_block = cond_value ? node->as.if_stmt.then_block
                                   : node->as.if_stmt.else_block;

    /* Chosen block must be a simple block with value expression */
    if (chosen_block->as.block.count > 0) {
        return NULL;  /* Has statements, can't fold */
    }
    if (!chosen_block->as.block.value_expr) {
        return NULL;  /* No value expression */
    }

    Ast *value_expr = chosen_block->as.block.value_expr;

    /* Value expression must be comptime */
    if (!is_comptime_constant(value_expr, scope)) {
        return NULL;
    }

    /* Return a copy of the value expression */
    /* For now, just return the expression directly - it will be type-checked */
    return value_expr;
}

/* ============================== Type Checking ============================== */

/* Check if a node is addressable (can take a reference to it).
 * True for identifiers and field-access chains rooted on an identifier. */
static bool is_addressable(Ast *node) {
    if (node->kind == AST_IDENTIFIER) return true;
    if (node->kind == AST_FIELD_ACCESS) return is_addressable(node->as.field_access.object);
    return false;
}

/* Check if a node can appear on the left-hand side of an assignment.
 * Valid targets are variables plus field/index chains rooted on a variable.
 * Slice .len is read-only and not assignable. */
static bool is_assignable_target(Ast *node) {
    if (node->kind == AST_IDENTIFIER) return true;
    if (node->kind == AST_INDEX_ACCESS) {
        return is_assignable_target(node->as.index_access.object);
    }
    if (node->kind == AST_FIELD_ACCESS) {
        if (node->as.field_access.field_length == 3 &&
            memcmp(node->as.field_access.field_start, "len", 3) == 0) {
            return false;
        }
        return is_assignable_target(node->as.field_access.object);
    }
    return false;
}

/* Check if a type is a scalar (integer, f64, bool, or comptime) */
static bool type_is_scalar(Type type) {
    return type_is_numeric(type) || type == TYPE_BOOL;
}

/* Walk through field/index chains to find the root AST node */
static Ast *ast_root_target(Ast *node) {
    while (node->kind == AST_FIELD_ACCESS || node->kind == AST_INDEX_ACCESS) {
        if (node->kind == AST_FIELD_ACCESS) node = node->as.field_access.object;
        else node = node->as.index_access.object;
    }
    return node;
}

static Type typecheck_expression(Ast *node, Scope *scope,
                                 FunctionTable *func_table);

static Type typecheck_assignment_target(Ast *target, Scope *scope,
                                        FunctionTable *func_table,
                                        SourceLoc loc) {
    Type target_type = typecheck_expression(target, scope, func_table);

    if (!is_assignable_target(target)) {
        diagnostic(loc.file, ERR_S088_INVALID_ASSIGN_TARGET, loc.line,
                   loc.column,
                   "Assignment target must be a variable, field, or index expression");
        return target_type;
    }

    Ast *root = ast_root_target(target);
    if (root->kind == AST_IDENTIFIER) {
        Variable *v = scope_lookup(scope, root->as.identifier.start,
                                   root->as.identifier.length);
        if (v == NULL) {
            return target_type;
        }
        if (!v->is_mutable) {
            if (target->kind == AST_FIELD_ACCESS) {
                diagnostic(loc.file, ERR_S030_FIELD_IMMUTABLE, loc.line,
                           loc.column,
                           "Cannot assign to field of immutable variable '%.*s'",
                           (int)root->as.identifier.length,
                           root->as.identifier.start);
            } else if (target->kind == AST_INDEX_ACCESS) {
                diagnostic(loc.file, ERR_S036_INDEX_IMMUTABLE, loc.line,
                           loc.column,
                           "Cannot assign to element of immutable variable '%.*s'",
                           (int)root->as.identifier.length,
                           root->as.identifier.start);
            } else {
                diagnostic(loc.file, ERR_S003_IMMUTABLE_ASSIGNMENT, loc.line,
                           loc.column,
                           "Cannot assign to immutable variable '%.*s'",
                           (int)root->as.identifier.length,
                           root->as.identifier.start);
            }
        }
    }

    if (target->kind == AST_IDENTIFIER && type_is_non_copyable(target_type)) {
        diagnostic(loc.file, ERR_S043_STRUCT_COPY, loc.line, loc.column,
                   "Cannot assign to %s variable (not copyable)",
                   type_name(target_type));
    }

    return target_type;
}

static Type typecheck_compound_assignment(BinaryOp op, Type target_type,
                                          Type value_type, SourceLoc loc) {
    Type result = target_type;

    switch (op) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
            if (type_is_enum(target_type) || type_is_enum(value_type)) {
                diagnostic(loc.file, ERR_S067_ENUM_ARITHMETIC, loc.line,
                           loc.column,
                           "Arithmetic is not allowed on enum type %s",
                           type_name(type_is_enum(target_type) ? target_type
                                                               : value_type));
                return result;
            }

            result = resolve_numeric_binary_type(target_type, value_type);
            if (result == TYPE_VOID) {
                diagnostic(loc.file, ERR_S006_TYPE_MISMATCH, loc.line,
                           loc.column,
                           "Cannot mix %s and %s in arithmetic operation",
                           type_name(target_type), type_name(value_type));
                return target_type;
            }

            if (op == OP_MOD &&
                (result == TYPE_F64 || result == TYPE_COMPTIME_FLOAT)) {
                diagnostic(loc.file, ERR_S061_MODULO_ON_FLOAT, loc.line,
                           loc.column,
                           "Modulo operator '%%' is not supported on floating-point types");
                return target_type;
            }
            return result;

        case OP_BITAND:
        case OP_BITOR:
        case OP_BITXOR:
        case OP_SHL:
        case OP_SHR:
            result = resolve_numeric_binary_type(target_type, value_type);
            if (result == TYPE_VOID) {
                diagnostic(loc.file, ERR_S006_TYPE_MISMATCH, loc.line,
                           loc.column,
                           "Cannot mix %s and %s in bitwise operation",
                           type_name(target_type), type_name(value_type));
                return target_type;
            }

            if (result == TYPE_F64 || result == TYPE_COMPTIME_FLOAT) {
                diagnostic(loc.file, ERR_S062_BITWISE_ON_FLOAT, loc.line,
                           loc.column,
                           "Bitwise operators are not supported on floating-point types");
                return target_type;
            }
            return result;

        default:
            panic(ERR_I002_INTERNAL_ERROR, "invalid compound assignment operator");
    }

    return target_type;
}

/* Return a human-readable label for ref/mut ref passing style */
static const char *ref_label(bool is_ref, bool is_mut_ref) {
    if (is_mut_ref) return "mut ref";
    if (is_ref) return "ref";
    return "(none)";
}

/* Validate a ref argument: addressability, scalar field, and mutability checks */
static void typecheck_ref_arg(Ast *arg, Type arg_type, bool is_mut,
                               Scope *scope) {
    if (!is_addressable(arg)) {
        if (arg->kind == AST_INDEX_ACCESS) {
            diagnostic(arg->loc.file, ERR_S042_REF_ARRAY_ELEMENT, arg->loc.line,
                       arg->loc.column,
                       "Cannot take reference of array element");
        } else {
            diagnostic(arg->loc.file, ERR_S039_REF_NOT_ADDRESSABLE, arg->loc.line,
                       arg->loc.column,
                       "Cannot take reference of non-addressable expression");
        }
        return;
    }

    if (arg->kind == AST_FIELD_ACCESS && type_is_scalar(arg_type)) {
        diagnostic(arg->loc.file, ERR_S041_REF_SCALAR_FIELD, arg->loc.line,
                   arg->loc.column,
                   "Cannot take reference of scalar field (just copy it)");
    }

    if (is_mut) {
        Ast *root = ast_root_target(arg);
        if (root->kind == AST_IDENTIFIER) {
            Variable *v = scope_lookup(scope, root->as.identifier.start,
                                        root->as.identifier.length);
            if (v && !v->is_mutable) {
                diagnostic(arg->loc.file, ERR_S040_MUT_REF_IMMUTABLE, arg->loc.line,
                           arg->loc.column,
                           "Cannot pass 'mut ref' to immutable variable '%.*s'",
                           (int)root->as.identifier.length,
                           root->as.identifier.start);
            }
        }
    }
}

static const char *check_integer_range(long value, Type target);
static void typecheck_comptime_range(Ast *init, Type target, Scope *scope);
static Type typecheck_expression(Ast *node, Scope *scope, FunctionTable *func_table);
static void typecheck_statement(Ast *node, Scope **scope, Type return_type, FunctionTable *func_table);
static void typecheck_invalidate_arena_slices(Scope *scope, const char *arena_start, size_t arena_length);

/* Match scalar support stays narrow: bool plus concrete integer types only. */
static bool type_is_match_scalar(Type type) {
    return type == TYPE_BOOL ||
           (type_is_integer(type) && type != TYPE_COMPTIME_INT);
}

/* Type-check one match arm body, including any enum payload binding. */
static Type typecheck_match_arm_body(Ast *arm, Scope *scope, Type return_type,
                                     FunctionTable *func_table,
                                     Variable *scrut_var) {
    Ast *body = arm->as.match_arm.body;
    Scope *arm_scope = scope_create(scope);
    if (arm->as.match_arm.binding_name &&
        arm->as.match_arm.binding_type != TYPE_VOID) {
        Variable *bv = scope_add(arm_scope,
                  arm->as.match_arm.binding_name,
                  arm->as.match_arm.binding_length,
                  false, arm->as.match_arm.binding_type,
                  arm->loc);
        /* Propagate arena tracking from scrutinee to slice binding. */
        if (bv && type_is_slice(arm->as.match_arm.binding_type) &&
            scrut_var && scrut_var->arena_source_start) {
            bv->arena_source_start = scrut_var->arena_source_start;
            bv->arena_source_length = scrut_var->arena_source_length;
            bv->arena_is_local = scrut_var->arena_is_local;
        }
    }

    for (size_t s = 0; s < body->as.block.count; s++) {
        typecheck_statement(body->as.block.statements[s],
                            &arm_scope, return_type, func_table);
    }

    Type arm_type = TYPE_VOID;
    if (body->as.block.value_expr) {
        arm_type = typecheck_expression(body->as.block.value_expr,
                                        arm_scope, func_table);
    }

    scope_destroy(arm_scope);
    return arm_type;
}

/* Keep match arms on a single compatible result type. */
static void typecheck_match_arm_result(Ast *arm, Type arm_type,
                                       Type *common_type, bool *first_arm) {
    if (*first_arm) {
        *common_type = arm_type;
        *first_arm = false;
        return;
    }

    Type resolved = resolve_branch_types(*common_type, arm_type);
    if (resolved == TYPE_VOID && *common_type != TYPE_VOID) {
        diagnostic(arm->loc.file, ERR_S076_MATCH_ARM_TYPE_MISMATCH,
                   arm->loc.line, arm->loc.column,
                   "Match arm type %s incompatible with previous arms (%s)",
                   type_name(arm_type), type_name(*common_type));
        return;
    }

    *common_type = resolved;
}

/* `_` stays a scalar-only wildcard spelling; parser still reads it as an identifier. */
static bool match_arm_pattern_is_scalar_wildcard(const MatchArmPattern *pattern) {
    return pattern->kind == MATCH_PATTERN_ENUM_VARIANT &&
           pattern->variant_length == 1 &&
           pattern->variant_name[0] == '_' &&
           !pattern->has_binding;
}

/* Catch-all patterns only count when they stand alone in their arm. */
static bool match_arm_pattern_is_catch_all(const MatchArmPattern *pattern) {
    return pattern->kind == MATCH_PATTERN_WILDCARD ||
           pattern->kind == MATCH_PATTERN_ELSE;
}

/* Scalar duplicate tracking needs room for every literal inside every arm. */
static size_t match_expr_pattern_count(const Ast *node) {
    size_t count = 0;

    for (size_t i = 0; i < node->as.match_expr.arm_count; i++) {
        count += node->as.match_expr.arms[i]->as.match_arm.pattern_count;
    }

    return count;
}

/* Type-check enum match arms for tagged and plain enums. */
static Type typecheck_enum_match_arms(Ast *node, Scope *scope, Type return_type,
                                      FunctionTable *func_table,
                                      Type scrut_type,
                                      EnumTypeEntry *et) {
    bool *covered = calloc(et->variant_count, sizeof(bool));
    if (!covered) panic(ERR_I001_OUT_OF_MEMORY, "allocating match coverage");
    Type common_type = TYPE_VOID;
    bool first_arm = true;
    bool has_else = false;
    bool saw_catch_all = false;

    /* Pre-lookup scrutinee variable for arena tracking propagation. */
    Variable *scrut_var = NULL;
    Ast *scrut = node->as.match_expr.scrutinee;
    if (scrut->kind == AST_IDENTIFIER) {
        scrut_var = scope_lookup(scope,
            scrut->as.identifier.start, scrut->as.identifier.length);
    }

    for (size_t i = 0; i < node->as.match_expr.arm_count; i++) {
        Ast *arm = node->as.match_expr.arms[i];
        size_t pattern_count = arm->as.match_arm.pattern_count;
        bool is_or_group = pattern_count > 1;
        bool arm_has_else = false;

        arm->as.match_arm.binding_name = NULL;
        arm->as.match_arm.binding_length = 0;
        arm->as.match_arm.binding_type = TYPE_VOID;

        if (saw_catch_all && pattern_count > 0) {
            diagnostic(arm->as.match_arm.patterns[0].loc.file,
                       ERR_S089_CATCH_ALL_NOT_LAST,
                       arm->as.match_arm.patterns[0].loc.line,
                       arm->as.match_arm.patterns[0].loc.column,
                       "Catch-all match arm must be last");
        }

        for (size_t p = 0; p < pattern_count; p++) {
            MatchArmPattern *pattern = &arm->as.match_arm.patterns[p];
            const char *vname = pattern->variant_name;
            size_t vlen = pattern->variant_length;
            bool found = false;

            if (pattern->kind == MATCH_PATTERN_ELSE) {
                if (pattern->has_binding) {
                    diagnostic(pattern->loc.file, ERR_S078_UNEXPECTED_PAYLOAD,
                               pattern->loc.line, pattern->loc.column,
                               "Catch-all match arm does not bind a payload");
                }
                if (is_or_group) {
                    diagnostic(pattern->loc.file, ERR_S006_TYPE_MISMATCH,
                               pattern->loc.line, pattern->loc.column,
                               "Catch-all match arm cannot be grouped with other patterns");
                } else {
                    arm_has_else = true;
                }
                continue;
            }

            if (pattern->kind != MATCH_PATTERN_ENUM_VARIANT) {
                diagnostic(pattern->loc.file, ERR_S006_TYPE_MISMATCH,
                           pattern->loc.line, pattern->loc.column,
                           "Match arms for %s must name enum variants or use 'else'",
                           type_name(scrut_type));
                continue;
            }

            for (size_t v = 0; v < et->variant_count; v++) {
                if (et->variants[v].name_length == vlen &&
                    memcmp(et->variants[v].name_start, vname, vlen) == 0) {
                    pattern->case_value = et->variants[v].value;
                    if (covered[v]) {
                        diagnostic(pattern->loc.file, ERR_S075_DUPLICATE_MATCH_ARM,
                                   pattern->loc.line, pattern->loc.column,
                                   "Duplicate match arm '%.*s'",
                                   (int)vlen, vname);
                    }
                    covered[v] = true;
                    found = true;

                    if (is_or_group) {
                        if (pattern->has_binding) {
                            diagnostic(pattern->loc.file, ERR_S078_UNEXPECTED_PAYLOAD,
                                       pattern->loc.line, pattern->loc.column,
                                       "Or-pattern enum variants do not bind payloads");
                        }
                    } else if (pattern->has_binding) {
                        if (et->variants[v].payload_type == TYPE_VOID) {
                            diagnostic(pattern->loc.file, ERR_S078_UNEXPECTED_PAYLOAD,
                                       pattern->loc.line, pattern->loc.column,
                                       "Variant '%.*s' in match does not bind a payload",
                                       (int)vlen, vname);
                        } else {
                            arm->as.match_arm.binding_name = pattern->binding_name;
                            arm->as.match_arm.binding_length = pattern->binding_length;
                            arm->as.match_arm.binding_type = et->variants[v].payload_type;
                        }
                    } else if (et->variants[v].payload_type != TYPE_VOID) {
                        diagnostic(pattern->loc.file, ERR_S079_MISSING_PAYLOAD,
                                   pattern->loc.line, pattern->loc.column,
                                   "Variant '%.*s' in match is missing its payload binding",
                                   (int)vlen, vname);
                    }

                    break;
                }
            }

            if (!found) {
                diagnostic(pattern->loc.file, ERR_P045_UNKNOWN_ENUM_VARIANT,
                           pattern->loc.line, pattern->loc.column,
                           "Unknown variant '%.*s' in match on %s",
                           (int)vlen, vname, type_name(scrut_type));
            }
        }

        if (arm_has_else) {
            if (has_else) {
                MatchArmPattern *pattern = &arm->as.match_arm.patterns[0];
                diagnostic(pattern->loc.file, ERR_S075_DUPLICATE_MATCH_ARM,
                           pattern->loc.line, pattern->loc.column,
                           "Duplicate match arm 'else'");
            }
            has_else = true;
            saw_catch_all = true;
        }

        typecheck_match_arm_result(
            arm,
            typecheck_match_arm_body(arm, scope, return_type,
                                     func_table, scrut_var),
            &common_type, &first_arm);
    }

    if (!has_else) {
        for (size_t v = 0; v < et->variant_count; v++) {
            if (!covered[v]) {
                diagnostic(node->loc.file, ERR_S074_NON_EXHAUSTIVE_MATCH,
                           node->loc.line, node->loc.column,
                           "Non-exhaustive match: missing variant '%.*s'",
                           (int)et->variants[v].name_length,
                           et->variants[v].name_start);
                break;
            }
        }
    }

    free(covered);
    return common_type;
}

/* Type-check scalar literal match arms and require a catch-all for integers. */
static Type typecheck_scalar_match_arms(Ast *node, Scope *scope,
                                        Type return_type,
                                        FunctionTable *func_table,
                                        Type scrut_type) {
    long *literal_values = NULL;
    size_t literal_capacity = match_expr_pattern_count(node);
    if (literal_capacity > 0) {
        literal_values = malloc(literal_capacity * sizeof(long));
        if (!literal_values) {
            panic(ERR_I001_OUT_OF_MEMORY, "allocating scalar match values");
        }
    }

    size_t literal_count = 0;
    bool has_catch_all = false;
    bool has_true = false;
    bool has_false = false;
    Type common_type = TYPE_VOID;
    bool first_arm = true;
    bool saw_catch_all = false;

    for (size_t i = 0; i < node->as.match_expr.arm_count; i++) {
        Ast *arm = node->as.match_expr.arms[i];
        size_t pattern_count = arm->as.match_arm.pattern_count;
        bool is_or_group = pattern_count > 1;
        bool arm_has_catch_all = false;

        arm->as.match_arm.binding_name = NULL;
        arm->as.match_arm.binding_length = 0;
        arm->as.match_arm.binding_type = TYPE_VOID;

        if (saw_catch_all && pattern_count > 0) {
            diagnostic(arm->as.match_arm.patterns[0].loc.file,
                       ERR_S089_CATCH_ALL_NOT_LAST,
                       arm->as.match_arm.patterns[0].loc.line,
                       arm->as.match_arm.patterns[0].loc.column,
                       "Catch-all match arm must be last");
        }

        for (size_t p = 0; p < pattern_count; p++) {
            MatchArmPattern *pattern = &arm->as.match_arm.patterns[p];

            if (match_arm_pattern_is_scalar_wildcard(pattern)) {
                pattern->kind = MATCH_PATTERN_WILDCARD;
            }

            if (match_arm_pattern_is_catch_all(pattern)) {
                if (pattern->kind == MATCH_PATTERN_ELSE && pattern->has_binding) {
                    diagnostic(pattern->loc.file, ERR_S078_UNEXPECTED_PAYLOAD,
                               pattern->loc.line, pattern->loc.column,
                               "Catch-all match arm does not bind a payload");
                }
                if (is_or_group) {
                    diagnostic(pattern->loc.file, ERR_S006_TYPE_MISMATCH,
                               pattern->loc.line, pattern->loc.column,
                               "Catch-all match arm cannot be grouped with other patterns");
                } else {
                    arm_has_catch_all = true;
                }
                continue;
            }

            if (pattern->kind == MATCH_PATTERN_ENUM_VARIANT) {
                diagnostic(pattern->loc.file, ERR_S006_TYPE_MISMATCH,
                           pattern->loc.line, pattern->loc.column,
                           "Match arms for %s must use literals, '_', or 'else'",
                           type_name(scrut_type));
                continue;
            }

            if (pattern->kind != MATCH_PATTERN_LITERAL) {
                continue;
            }

            Ast *pattern_expr = pattern->pattern_expr;
            Type pattern_type = typecheck_expression(pattern_expr, scope,
                                                     func_table);

            if (scrut_type == TYPE_BOOL) {
                if (pattern_type != TYPE_BOOL) {
                    diagnostic(pattern_expr->loc.file, ERR_S006_TYPE_MISMATCH,
                               pattern_expr->loc.line, pattern_expr->loc.column,
                               "Match literal type %s is not compatible with %s",
                               type_name(pattern_type), type_name(scrut_type));
                } else {
                    long value = get_comptime_int(pattern_expr, NULL);
                    pattern->case_value = value;
                    if (value != 0) {
                        if (has_true) {
                            diagnostic(pattern->loc.file, ERR_S075_DUPLICATE_MATCH_ARM,
                                       pattern->loc.line, pattern->loc.column,
                                       "Duplicate match arm 'true'");
                        }
                        has_true = true;
                    } else {
                        if (has_false) {
                            diagnostic(pattern->loc.file, ERR_S075_DUPLICATE_MATCH_ARM,
                                       pattern->loc.line, pattern->loc.column,
                                       "Duplicate match arm 'false'");
                        }
                        has_false = true;
                    }
                }
            } else {
                if (!type_can_coerce(pattern_type, scrut_type)) {
                    diagnostic(pattern_expr->loc.file, ERR_S006_TYPE_MISMATCH,
                               pattern_expr->loc.line, pattern_expr->loc.column,
                               "Match literal type %s is not compatible with %s",
                               type_name(pattern_type), type_name(scrut_type));
                } else {
                    if (pattern_type == TYPE_COMPTIME_INT) {
                        typecheck_comptime_range(pattern_expr, scrut_type, scope);
                    }
                    long value = get_comptime_int(pattern_expr, NULL);
                    pattern->case_value = value;
                    for (size_t j = 0; j < literal_count; j++) {
                        if (literal_values[j] == value) {
                            diagnostic(pattern->loc.file, ERR_S075_DUPLICATE_MATCH_ARM,
                                       pattern->loc.line, pattern->loc.column,
                                       "Duplicate match arm '%ld'",
                                       value);
                            break;
                        }
                    }
                    literal_values[literal_count++] = value;
                }
            }
        }

        if (arm_has_catch_all) {
            if (has_catch_all) {
                MatchArmPattern *pattern = &arm->as.match_arm.patterns[0];
                diagnostic(pattern->loc.file, ERR_S075_DUPLICATE_MATCH_ARM,
                           pattern->loc.line, pattern->loc.column,
                           "Duplicate match arm");
            }
            has_catch_all = true;
            saw_catch_all = true;
        }

        typecheck_match_arm_result(
            arm,
            typecheck_match_arm_body(arm, scope, return_type,
                                     func_table, NULL),
            &common_type, &first_arm);
    }

    if (scrut_type == TYPE_BOOL) {
        if (!has_catch_all && (!has_true || !has_false)) {
            diagnostic(node->loc.file, ERR_S074_NON_EXHAUSTIVE_MATCH,
                       node->loc.line, node->loc.column,
                       "Non-exhaustive match: missing pattern '%s'",
                       has_true ? "false" : "true");
        }
    } else if (!has_catch_all) {
        diagnostic(node->loc.file, ERR_S074_NON_EXHAUSTIVE_MATCH,
                   node->loc.line, node->loc.column,
                   "Non-exhaustive match: missing catch-all arm");
    }

    free(literal_values);
    return common_type;
}

/* Dispatch match type-checking by scrutinee category. */
static Type typecheck_match_arms(Ast *node, Scope *scope, Type return_type,
                                 FunctionTable *func_table) {
    Type scrut_type = typecheck_expression(
        node->as.match_expr.scrutinee, scope, func_table);
    EnumTypeEntry *et = enum_table_get(scrut_type);

    if (et) {
        return typecheck_enum_match_arms(node, scope, return_type,
                                         func_table, scrut_type, et);
    }

    if (type_is_match_scalar(scrut_type)) {
        return typecheck_scalar_match_arms(node, scope, return_type,
                                           func_table, scrut_type);
    }

    diagnostic(node->loc.file, ERR_S073_MATCH_NOT_ENUM,
               node->loc.line, node->loc.column,
               "Match scrutinee must be an enum, bool, or integer type, got %s",
               type_name(scrut_type));
    return TYPE_VOID;
}

/* Check that a native declaration exists for a built-in call */
static void require_native_decl(Ast *node, const char *name, size_t len,
                                const char *hint) {
    if (!native_table_lookup(name, len, g_typecheck_module_id)) {
        diagnostic(node->loc.file, ERR_S087_MISSING_NATIVE_DECL,
                   node->loc.line, node->loc.column,
                   "%s() requires: %s", name, hint);
    }
}

/* Match one registered native parameter against the compiler builtin contract. */
static bool native_entry_param_matches(const NativeEntry *entry, size_t index,
                                       Type wanted_type, bool wants_ref,
                                       bool wants_mut_ref) {
    if (index >= entry->param_count) return false;

    Parameter *param = &entry->params[index];
    return param->type == wanted_type &&
           param->is_ref == wants_ref &&
           param->is_mut_ref == wants_mut_ref;
}

/* Keep builtin native signatures aligned between the seed and self-hosted sema. */
static bool native_entry_has_builtin_signature(const NativeEntry *entry) {
    Type str_type = slice_table_intern(TYPE_U8);
    Type str_slice = slice_table_intern(str_type);

    if (entry->name_length == 8 &&
        memcmp(entry->name_start, "fd_write", 8) == 0) {
        return entry->param_count == 2 &&
               entry->return_type == TYPE_I64 &&
               native_entry_param_matches(entry, 0, TYPE_I32, false, false) &&
               native_entry_param_matches(entry, 1, str_type, true, false);
    }
    if (entry->name_length == 7 &&
        memcmp(entry->name_start, "fd_read", 7) == 0) {
        return entry->param_count == 2 &&
               entry->return_type == TYPE_I64 &&
               native_entry_param_matches(entry, 0, TYPE_I32, false, false) &&
               native_entry_param_matches(entry, 1, str_type, true, true);
    }
    if (entry->name_length == 7 &&
        memcmp(entry->name_start, "fd_open", 7) == 0) {
        return entry->param_count == 2 &&
               entry->return_type == TYPE_I32 &&
               native_entry_param_matches(entry, 0, str_type, true, false) &&
               native_entry_param_matches(entry, 1, TYPE_I32, false, false);
    }
    if (entry->name_length == 8 &&
        memcmp(entry->name_start, "fd_close", 8) == 0) {
        return entry->param_count == 1 &&
               entry->return_type == TYPE_VOID &&
               native_entry_param_matches(entry, 0, TYPE_I32, false, false);
    }
    if (entry->name_length == 7 &&
        memcmp(entry->name_start, "fd_seek", 7) == 0) {
        return entry->param_count == 3 &&
               entry->return_type == TYPE_I64 &&
               native_entry_param_matches(entry, 0, TYPE_I32, false, false) &&
               native_entry_param_matches(entry, 1, TYPE_I64, false, false) &&
               native_entry_param_matches(entry, 2, TYPE_I32, false, false);
    }
    if (entry->name_length == 8 &&
        memcmp(entry->name_start, "mem_copy", 8) == 0) {
        return entry->param_count == 2 &&
               entry->return_type == TYPE_I64 &&
               native_entry_param_matches(entry, 0, str_type, true, true) &&
               native_entry_param_matches(entry, 1, str_type, true, false);
    }
    if (entry->name_length == 4 &&
        memcmp(entry->name_start, "exit", 4) == 0) {
        return entry->param_count == 1 &&
               entry->return_type == TYPE_VOID &&
               native_entry_param_matches(entry, 0, TYPE_I32, false, false);
    }
    if (entry->name_length == 4 &&
        memcmp(entry->name_start, "args", 4) == 0) {
        return entry->param_count == 1 &&
               entry->return_type == str_slice &&
               native_entry_param_matches(entry, 0, TYPE_ARENA, true, true);
    }
    if (entry->name_length == 9 &&
        memcmp(entry->name_start, "shell_run", 9) == 0) {
        return entry->param_count == 1 &&
               entry->return_type == TYPE_I32 &&
               native_entry_param_matches(entry, 0, str_type, true, false);
    }
    if (entry->name_length == 7 &&
        memcmp(entry->name_start, "exe_dir", 7) == 0) {
        return entry->param_count == 1 &&
               entry->return_type == str_type &&
               native_entry_param_matches(entry, 0, TYPE_ARENA, true, true);
    }
    if (entry->name_length == 3 &&
        memcmp(entry->name_start, "pid", 3) == 0) {
        return entry->param_count == 0 &&
               entry->return_type == TYPE_I64;
    }
    if (entry->name_length == 7 &&
        memcmp(entry->name_start, "time_ns", 7) == 0) {
        return entry->param_count == 0 &&
               entry->return_type == TYPE_I64;
    }

    return true;
}

/* Diagnose builtin native declarations that drift from the compiler contract. */
static void validate_native_decl_signature(const NativeEntry *entry) {
    if (native_entry_has_builtin_signature(entry)) {
        return;
    }

    diagnostic(entry->loc.file, ERR_S090_INVALID_NATIVE_SIGNATURE,
               entry->loc.line, entry->loc.column,
               "Native function '%.*s' must use the compiler-provided signature",
               (int)entry->name_length, entry->name_start);
}

static Type typecheck_expression(Ast *node, Scope *scope, FunctionTable *func_table) {
    switch (node->kind) {
        case AST_NUMBER:
            node->expr_type = TYPE_COMPTIME_INT;
            return TYPE_COMPTIME_INT;

        case AST_FLOAT:
            node->expr_type = TYPE_COMPTIME_FLOAT;
            return TYPE_COMPTIME_FLOAT;

        case AST_BOOLEAN:
            node->expr_type = TYPE_BOOL;
            return TYPE_BOOL;

        case AST_ENUM_VARIANT: {
            if (node->as.enum_variant.payload) {
                Type payload_type = typecheck_expression(
                    node->as.enum_variant.payload, scope, func_table);
                EnumTypeEntry *et = enum_table_get(node->as.enum_variant.enum_type);
                if (et) {
                    long idx = node->as.enum_variant.value;
                    Type expected = et->variants[idx].payload_type;
                    if (payload_type != TYPE_UNKNOWN && expected != TYPE_VOID &&
                        !type_can_coerce(payload_type, expected)) {
                        diagnostic(node->loc.file, ERR_S077_PAYLOAD_TYPE_MISMATCH,
                                   node->loc.line, node->loc.column,
                                   "Payload type mismatch: expected %s, got %s",
                                   type_name(expected), type_name(payload_type));
                    }
                }
            }
            node->expr_type = node->as.enum_variant.enum_type;
            return node->as.enum_variant.enum_type;
        }

        case AST_IDENTIFIER: {
            const char *id_start = node->as.identifier.start;
            size_t id_len = node->as.identifier.length;
            Variable *v = NULL;

            if (node->as.identifier.module_id >= 0) {
                /* Qualified access: globals live in root scope only */
                size_t mod = (size_t)node->as.identifier.module_id;
                Scope *root = scope;
                while (root->parent != NULL) root = root->parent;
                for (size_t i = 0; i < root->count; i++) {
                    if (root->vars[i].module_id == mod &&
                        root->vars[i].name_length == id_len &&
                        memcmp(root->vars[i].name_start, id_start, id_len) == 0) {
                        v = &root->vars[i];
                        break;
                    }
                }
                if (v && !v->is_public) {
                    diagnostic(node->loc.file, ERR_S085_NOT_PUBLIC_VALUE,
                               node->loc.line, node->loc.column,
                               "Value '%.*s' is not public in module '%.*s'",
                               (int)id_len, id_start,
                               (int)g_modules[mod].alias_length,
                               g_modules[mod].alias);
                }
            } else {
                /* Unqualified access: try local scope first, then filter globals */
                v = scope_lookup(scope, id_start, id_len);
                if (v && v->is_global &&
                    v->module_id != g_typecheck_module_id &&
                    v->module_id != MODULE_BUILTIN) {
                    v = NULL;  /* not visible from this module */
                }
            }

            if (v == NULL) {
                diagnostic(node->loc.file, ERR_S002_UNDECLARED_VARIABLE, node->loc.line,
                           node->loc.column, "Undeclared variable '%.*s'",
                           (int)id_len, id_start);
                node->expr_type = TYPE_I64;
                return TYPE_I64;
            }
            if (v->is_invalidated) {
                diagnostic(node->loc.file, ERR_S056_SLICE_INVALIDATED, node->loc.line,
                           node->loc.column,
                           "Use of slice '%.*s' after arena reset",
                           (int)id_len, id_start);
            }
            node->expr_type = v->type;
            return v->type;
        }

        case AST_BINARY: {
            Type left_type = typecheck_expression(node->as.binary.left, scope, func_table);
            Type right_type = typecheck_expression(node->as.binary.right, scope, func_table);

            switch (node->as.binary.op) {
                /* Arithmetic: numeric x numeric -> numeric */
                case OP_ADD:
                case OP_SUB:
                case OP_MUL:
                case OP_DIV:
                case OP_MOD: {
                    if (type_is_enum(left_type) || type_is_enum(right_type)) {
                        diagnostic(node->loc.file, ERR_S067_ENUM_ARITHMETIC, node->loc.line, node->loc.column,
                                   "Arithmetic is not allowed on enum type %s",
                                   type_name(type_is_enum(left_type) ? left_type : right_type));
                        node->expr_type = TYPE_I64;
                        return TYPE_I64;
                    }
                    Type result = resolve_numeric_binary_type(left_type, right_type);
                    if (result == TYPE_VOID) {
                        diagnostic(node->loc.file, ERR_S006_TYPE_MISMATCH, node->loc.line, node->loc.column,
                                   "Cannot mix %s and %s in arithmetic operation",
                                   type_name(left_type), type_name(right_type));
                        node->expr_type = TYPE_I64;  /* default */
                        return TYPE_I64;
                    }

                    /* Modulo is integer-only */
                    if (node->as.binary.op == OP_MOD &&
                        (result == TYPE_F64 || result == TYPE_COMPTIME_FLOAT)) {
                        diagnostic(node->loc.file, ERR_S061_MODULO_ON_FLOAT, node->loc.line, node->loc.column,
                                   "Modulo operator '%%' is not supported on floating-point types");
                        node->expr_type = TYPE_I64;
                        return TYPE_I64;
                    }

                    /* Try constant folding */
                    bool div_by_zero = false;
                    Ast *folded = try_fold_binary(node, scope, &div_by_zero);
                    if (div_by_zero) {
                        diagnostic(node->loc.file, ERR_S019_DIVISION_BY_ZERO, node->loc.line, node->loc.column,
                                   "Division by zero in constant expression");
                        node->expr_type = result;
                        return result;
                    }
                    if (folded) {
                        /* Replace node with folded result */
                        ast_free(node->as.binary.left);
                        ast_free(node->as.binary.right);
                        *node = *folded;
                        free(folded);
                        return node->expr_type;
                    }

                    node->expr_type = result;
                    return result;
                }

                /* Comparison: numeric x numeric or bool equality -> bool. */
                case OP_EQ:
                case OP_NEQ:
                case OP_LT:
                case OP_GT:
                case OP_LE:
                case OP_GE: {
                    /* Enum comparison: == and != only, same enum type required */
                    if (type_is_enum(left_type) || type_is_enum(right_type)) {
                        /* Tagged enums cannot be compared at all */
                        EnumTypeEntry *cmp_et = enum_table_get(
                            type_is_enum(left_type) ? left_type : right_type);
                        if (cmp_et && cmp_et->has_data) {
                            diagnostic(node->loc.file, ERR_S080_TAGGED_ENUM_COMPARE,
                                       node->loc.line, node->loc.column,
                                       "Cannot compare tagged enum %.*s, use match instead",
                                       (int)cmp_et->name_length, cmp_et->name_start);
                        } else if (node->as.binary.op != OP_EQ && node->as.binary.op != OP_NEQ) {
                            diagnostic(node->loc.file, ERR_S067_ENUM_ARITHMETIC, node->loc.line, node->loc.column,
                                       "Only == and != are allowed on enum type %s",
                                       type_name(type_is_enum(left_type) ? left_type : right_type));
                        } else if (left_type != right_type) {
                            diagnostic(node->loc.file, ERR_S068_ENUM_COMPARE_MISMATCH, node->loc.line, node->loc.column,
                                       "Cannot compare different types %s and %s",
                                       type_name(left_type), type_name(right_type));
                        }

                        /* Try constant folding for enum comparisons */
                        Ast *folded = try_fold_comparison(node, scope);
                        if (folded) {
                            ast_free(node->as.binary.left);
                            ast_free(node->as.binary.right);
                            *node = *folded;
                            free(folded);
                            return TYPE_BOOL;
                        }

                        node->expr_type = TYPE_BOOL;
                        return TYPE_BOOL;
                    }

                    if (left_type == TYPE_BOOL || right_type == TYPE_BOOL) {
                        if ((node->as.binary.op != OP_EQ &&
                             node->as.binary.op != OP_NEQ) ||
                            left_type != TYPE_BOOL || right_type != TYPE_BOOL) {
                            diagnostic(node->loc.file, ERR_S006_TYPE_MISMATCH,
                                       node->loc.line, node->loc.column,
                                       "Cannot compare %s and %s",
                                       type_name(left_type), type_name(right_type));
                        }

                        Ast *folded = try_fold_comparison(node, scope);
                        if (folded) {
                            ast_free(node->as.binary.left);
                            ast_free(node->as.binary.right);
                            *node = *folded;
                            free(folded);
                            return TYPE_BOOL;
                        }

                        node->expr_type = TYPE_BOOL;
                        return TYPE_BOOL;
                    }

                    Type result = resolve_numeric_binary_type(left_type, right_type);
                    if (result == TYPE_VOID) {
                        diagnostic(node->loc.file, ERR_S006_TYPE_MISMATCH, node->loc.line, node->loc.column,
                                   "Cannot compare %s and %s",
                                   type_name(left_type), type_name(right_type));
                    }

                    /* Try constant folding */
                    Ast *folded = try_fold_comparison(node, scope);
                    if (folded) {
                        ast_free(node->as.binary.left);
                        ast_free(node->as.binary.right);
                        *node = *folded;
                        free(folded);
                        return TYPE_BOOL;
                    }

                    node->expr_type = TYPE_BOOL;
                    return TYPE_BOOL;
                }

                /* Logical: bool x bool -> bool */
                case OP_AND:
                case OP_OR:
                    if (left_type != TYPE_BOOL) {
                        diagnostic(node->as.binary.left->loc.file, ERR_S006_TYPE_MISMATCH, node->as.binary.left->loc.line,
                                   node->as.binary.left->loc.column,
                                   "Expected bool, got %s", type_name(left_type));
                    }
                    if (right_type != TYPE_BOOL) {
                        diagnostic(node->as.binary.right->loc.file, ERR_S006_TYPE_MISMATCH, node->as.binary.right->loc.line,
                                   node->as.binary.right->loc.column,
                                   "Expected bool, got %s", type_name(right_type));
                    }
                    node->expr_type = TYPE_BOOL;
                    return TYPE_BOOL;

                /* Bitwise: integer x integer -> integer */
                case OP_BITAND:
                case OP_BITOR:
                case OP_BITXOR:
                case OP_SHL:
                case OP_SHR: {
                    Type result = resolve_numeric_binary_type(left_type, right_type);
                    if (result == TYPE_VOID) {
                        diagnostic(node->loc.file, ERR_S006_TYPE_MISMATCH, node->loc.line, node->loc.column,
                                   "Cannot mix %s and %s in bitwise operation",
                                   type_name(left_type), type_name(right_type));
                        node->expr_type = TYPE_I64;
                        return TYPE_I64;
                    }

                    if (result == TYPE_F64 || result == TYPE_COMPTIME_FLOAT) {
                        diagnostic(node->loc.file, ERR_S062_BITWISE_ON_FLOAT, node->loc.line, node->loc.column,
                                   "Bitwise operators are not supported on floating-point types");
                        node->expr_type = TYPE_I64;
                        return TYPE_I64;
                    }

                    /* Try constant folding */
                    bool unused = false;
                    Ast *folded = try_fold_binary(node, scope, &unused);
                    if (folded) {
                        ast_free(node->as.binary.left);
                        ast_free(node->as.binary.right);
                        *node = *folded;
                        free(folded);
                        return node->expr_type;
                    }

                    node->expr_type = result;
                    return result;
                }
            }
            break;
        }

        case AST_UNARY: {
            Type operand_type = typecheck_expression(node->as.unary.operand, scope, func_table);

            /* Try constant folding first */
            Ast *folded = try_fold_unary(node, scope);
            if (folded) {
                ast_free(node->as.unary.operand);
                *node = *folded;
                free(folded);
                return node->expr_type;
            }

            switch (node->as.unary.op) {
                /* Negation: numeric -> numeric (preserves type) */
                case OP_NEG:
                    if (type_is_unsigned(operand_type)) {
                        diagnostic(node->as.unary.operand->loc.file, ERR_S006_TYPE_MISMATCH, node->as.unary.operand->loc.line,
                                   node->as.unary.operand->loc.column,
                                   "Cannot negate unsigned type %s",
                                   type_name(operand_type));
                        node->expr_type = operand_type;
                        return operand_type;
                    }
                    if (!type_is_numeric(operand_type)) {
                        diagnostic(node->as.unary.operand->loc.file, ERR_S006_TYPE_MISMATCH, node->as.unary.operand->loc.line,
                                   node->as.unary.operand->loc.column,
                                   "Expected numeric type for negation, got %s",
                                   type_name(operand_type));
                        node->expr_type = TYPE_I64;
                        return TYPE_I64;
                    }
                    node->expr_type = operand_type;
                    return operand_type;

                /* NOT: bool -> bool */
                case OP_NOT:
                    if (operand_type != TYPE_BOOL) {
                        diagnostic(node->as.unary.operand->loc.file, ERR_S006_TYPE_MISMATCH, node->as.unary.operand->loc.line,
                                   node->as.unary.operand->loc.column,
                                   "Expected bool for logical NOT, got %s",
                                   type_name(operand_type));
                    }
                    node->expr_type = TYPE_BOOL;
                    return TYPE_BOOL;

                /* Bitwise NOT: integer -> integer */
                case OP_BITNOT:
                    if (operand_type == TYPE_F64 || operand_type == TYPE_COMPTIME_FLOAT) {
                        diagnostic(node->as.unary.operand->loc.file, ERR_S062_BITWISE_ON_FLOAT, node->as.unary.operand->loc.line,
                                   node->as.unary.operand->loc.column,
                                   "Bitwise NOT is not supported on floating-point types");
                        node->expr_type = TYPE_I64;
                        return TYPE_I64;
                    }
                    if (!type_is_integer(operand_type)) {
                        diagnostic(node->as.unary.operand->loc.file, ERR_S006_TYPE_MISMATCH, node->as.unary.operand->loc.line,
                                   node->as.unary.operand->loc.column,
                                   "Expected integer type for bitwise NOT, got %s",
                                   type_name(operand_type));
                        node->expr_type = TYPE_I64;
                        return TYPE_I64;
                    }
                    node->expr_type = operand_type;
                    return operand_type;
            }
            break;
        }

        case AST_FUNC_CALL: {
            const char *name_start = node->as.func_call.name_start;
            size_t name_length = node->as.func_call.name_length;

            /* Module-aware function lookup */
            FunctionEntry *entry;
            if (node->as.func_call.module_id >= 0) {
                /* Qualified access: look up in specific module */
                size_t mod = (size_t)node->as.func_call.module_id;
                entry = func_table_lookup_in_module(func_table, name_start,
                                                     name_length, mod);
                if (entry == NULL) {
                    diagnostic(node->loc.file, ERR_S016_UNDEFINED_FUNCTION,
                               node->loc.line, node->loc.column,
                               "Function '%.*s' not found in module '%.*s'",
                               (int)name_length, name_start,
                               (int)g_modules[mod].alias_length,
                               g_modules[mod].alias);
                    node->expr_type = TYPE_I64;
                    return TYPE_I64;
                }
                if (!entry->is_public) {
                    diagnostic(node->loc.file, ERR_S083_NOT_PUBLIC_FUNC,
                               node->loc.line, node->loc.column,
                               "Function '%.*s' is not public in module '%.*s'",
                               (int)name_length, name_start,
                               (int)g_modules[mod].alias_length,
                               g_modules[mod].alias);
                }
            } else {
                /* Unqualified access: look up in current module only */
                entry = func_table_lookup_in_module(func_table, name_start,
                                                     name_length,
                                                     g_typecheck_module_id);
                if (entry == NULL) {
                    diagnostic(node->loc.file, ERR_S016_UNDEFINED_FUNCTION,
                               node->loc.line, node->loc.column,
                               "Undefined function '%.*s'",
                               (int)name_length, name_start);
                    node->expr_type = TYPE_I64;
                    return TYPE_I64;
                }
            }

            /* Check argument count */
            if (node->as.func_call.arg_count != entry->param_count) {
                diagnostic(node->loc.file, ERR_S017_WRONG_ARG_COUNT, node->loc.line,
                           node->loc.column,
                           "Function '%.*s' expects %zu arguments, got %zu",
                           (int)name_length, name_start,
                           entry->param_count, node->as.func_call.arg_count);
            }

            /* Type check each argument */
            size_t check_count = node->as.func_call.arg_count < entry->param_count
                                 ? node->as.func_call.arg_count : entry->param_count;
            for (size_t i = 0; i < check_count; i++) {
                Ast *arg = node->as.func_call.arguments[i];
                Type arg_type = typecheck_expression(arg, scope, func_table);

                /* Check ref/mut ref matching between call site and parameter */
                bool call_ref = node->as.func_call.arg_is_ref &&
                                node->as.func_call.arg_is_ref[i];
                bool call_mut = node->as.func_call.arg_is_mut_ref &&
                                node->as.func_call.arg_is_mut_ref[i];
                bool param_ref = entry->param_is_ref[i];
                bool param_mut = entry->param_is_mut_ref[i];

                if (call_ref != param_ref || call_mut != param_mut) {
                    diagnostic(arg->loc.file, ERR_S038_REF_MISMATCH, arg->loc.line,
                               arg->loc.column,
                               "Argument %zu: expected %s, got %s",
                               i + 1,
                               ref_label(param_ref, param_mut),
                               ref_label(call_ref, call_mut));
                }

                Type param_type = entry->param_types[i];

                if (call_ref && type_is_slice(param_type)) {
                    /* Slice ref: only check mutability (no addressability needed) */
                    if (call_mut) {
                        Ast *root = ast_root_target(arg);
                        if (root->kind == AST_IDENTIFIER) {
                            Variable *v = scope_lookup(scope, root->as.identifier.start,
                                                        root->as.identifier.length);
                            if (v && !v->is_mutable) {
                                diagnostic(arg->loc.file, ERR_S040_MUT_REF_IMMUTABLE, arg->loc.line,
                                           arg->loc.column,
                                           "Cannot pass 'mut ref' to immutable variable '%.*s'",
                                           (int)root->as.identifier.length,
                                           root->as.identifier.start);
                            }
                        }
                    }
                } else if (call_ref) {
                    typecheck_ref_arg(arg, arg_type, call_mut, scope);
                }
                if (!type_can_coerce(arg_type, param_type)) {
                    diagnostic(arg->loc.file, ERR_S018_ARG_TYPE_MISMATCH,
                               arg->loc.line, arg->loc.column,
                               "Argument %zu: expected %s, got %s",
                               i + 1, type_name(param_type),
                               type_name(arg_type));
                }
                if (arg_type == TYPE_COMPTIME_INT && type_is_integer(param_type)) {
                    typecheck_comptime_range(arg, param_type, scope);
                }
            }

            /* Also type-check remaining arguments (even if wrong count) */
            for (size_t i = check_count; i < node->as.func_call.arg_count; i++) {
                typecheck_expression(node->as.func_call.arguments[i], scope, func_table);
            }

            node->expr_type = entry->return_type;
            return entry->return_type;
        }

        case AST_BLOCK: {
            /* Create block scope */
            Scope *block_scope = scope_create(scope);

            /* Type-check all statements in block */
            for (size_t i = 0; i < node->as.block.count; i++) {
                typecheck_statement(node->as.block.statements[i], &block_scope,
                                   TYPE_VOID, func_table);
            }

            /* Type-check value expression if present */
            if (node->as.block.value_expr) {
                Type value_type = typecheck_expression(node->as.block.value_expr,
                                                       block_scope, func_table);
                scope_destroy(block_scope);
                node->expr_type = value_type;
                return value_type;
            }

            /* No value expression = void block */
            scope_destroy(block_scope);
            node->expr_type = TYPE_VOID;
            return TYPE_VOID;
        }

        case AST_IF: {
            /* Check condition */
            Type cond_type = typecheck_expression(node->as.if_stmt.condition,
                                                  scope, func_table);
            if (cond_type != TYPE_BOOL) {
                diagnostic(node->as.if_stmt.condition->loc.file, ERR_S007_CONDITION_NOT_BOOL, node->as.if_stmt.condition->loc.line,
                           node->as.if_stmt.condition->loc.column,
                           "Condition must be bool, got %s", type_name(cond_type));
            }

            /* If no else, result is void - but still type-check then block */
            if (!node->as.if_stmt.else_block) {
                typecheck_expression(node->as.if_stmt.then_block, scope, func_table);
                node->expr_type = TYPE_VOID;
                return TYPE_VOID;
            }

            /* Both branches exist - check types match */
            Type then_type = typecheck_expression(node->as.if_stmt.then_block,
                                                  scope, func_table);
            Type else_type = typecheck_expression(node->as.if_stmt.else_block,
                                                  scope, func_table);

            /* Resolve common type */
            Type result = resolve_branch_types(then_type, else_type);
            if (result == TYPE_VOID && then_type != TYPE_VOID) {
                diagnostic(node->loc.file, ERR_S021_BRANCH_TYPE_MISMATCH, node->loc.line, node->loc.column,
                           "If/else branches have incompatible types: %s and %s",
                           type_name(then_type), type_name(else_type));
                node->expr_type = then_type;  /* Use then type as fallback */
                return then_type;
            }

            /* Try to fold comptime if expression */
            Ast *folded = try_fold_if_expr(node, scope);
            if (folded) {
                /* Replace this node with the folded value */
                Ast *cond = node->as.if_stmt.condition;
                Ast *then_block = node->as.if_stmt.then_block;
                Ast *else_block = node->as.if_stmt.else_block;
                SourceLoc loc = node->loc;

                /* Set value_expr to NULL in blocks to avoid double-free */
                if (then_block->as.block.value_expr == folded) {
                    then_block->as.block.value_expr = NULL;
                }
                if (else_block->as.block.value_expr == folded) {
                    else_block->as.block.value_expr = NULL;
                }

                /* Copy the value expression into this node */
                *node = *folded;
                node->loc = loc;

                /* Free the old if structure */
                ast_free(cond);
                ast_free(then_block);
                ast_free(else_block);

                return node->expr_type;
            }

            node->expr_type = result;
            return result;
        }

        case AST_MATCH: {
            Type common_type = typecheck_match_arms(node, scope, TYPE_VOID, func_table);
            node->expr_type = common_type;
            return common_type;
        }

        case AST_VALUE_CONSTRUCTOR: {
            const char *tname = node->as.value_constructor.type_name_start;
            size_t tlen = node->as.value_constructor.type_name_length;
            ValueTypeEntry *vt = value_table_lookup(g_value_table, tname, tlen);
            if (!vt) {
                diagnostic(node->loc.file, ERR_S027_NOT_A_VALUE_TYPE, node->loc.line,
                           node->loc.column, "'%.*s' is not a value type",
                           (int)tlen, tname);
                node->expr_type = TYPE_I64;
                return TYPE_I64;
            }

            int idx = (int)(vt - g_value_table->types);
            Type vtype = (Type)(TYPE_VALUE_BASE + idx);

            /* Check for duplicate fields in constructor */
            FieldInit *fi = node->as.value_constructor.fields;
            size_t fc = node->as.value_constructor.field_count;
            for (size_t i = 0; i < fc; i++) {
                for (size_t j = 0; j < i; j++) {
                    if (fi[i].name_length == fi[j].name_length &&
                        memcmp(fi[i].name_start, fi[j].name_start,
                               fi[i].name_length) == 0) {
                        diagnostic(node->loc.file, ERR_S026_DUPLICATE_CTOR_FIELD, node->loc.line,
                                   node->loc.column,
                                   "Duplicate field '%.*s' in constructor",
                                   (int)fi[i].name_length, fi[i].name_start);
                        break;
                    }
                }
            }

            /* Check each constructor field exists and has correct type */
            for (size_t i = 0; i < fc; i++) {
                int fidx = value_table_find_field(vt, fi[i].name_start,
                                                   fi[i].name_length);
                if (fidx < 0) {
                    diagnostic(node->loc.file, ERR_S024_UNKNOWN_CTOR_FIELD, node->loc.line,
                               node->loc.column,
                               "Unknown field '%.*s' in '%.*s' constructor",
                               (int)fi[i].name_length, fi[i].name_start,
                               (int)tlen, tname);
                    typecheck_expression(fi[i].value, scope, func_table);
                    continue;
                }
                Type field_type = vt->fields[fidx].type;
                Type val_type = typecheck_expression(fi[i].value, scope, func_table);
                if (!type_can_coerce(val_type, field_type)) {
                    diagnostic(fi[i].value->loc.file, ERR_S006_TYPE_MISMATCH,
                               fi[i].value->loc.line,
                               fi[i].value->loc.column,
                               "Field '%.*s': expected %s, got %s",
                               (int)fi[i].name_length, fi[i].name_start,
                               type_name(field_type), type_name(val_type));
                }
            }

            /* Check all fields are present */
            for (size_t i = 0; i < vt->field_count; i++) {
                bool found = false;
                for (size_t j = 0; j < fc; j++) {
                    if (vt->fields[i].name_length == fi[j].name_length &&
                        memcmp(vt->fields[i].name_start, fi[j].name_start,
                               vt->fields[i].name_length) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    diagnostic(node->loc.file, ERR_S025_MISSING_CTOR_FIELD, node->loc.line,
                               node->loc.column,
                               "Missing field '%.*s' in '%.*s' constructor",
                               (int)vt->fields[i].name_length,
                               vt->fields[i].name_start,
                               (int)tlen, tname);
                }
            }

            node->expr_type = vtype;
            return vtype;
        }

        case AST_ARRAY_LITERAL: {
            size_t count = node->as.array_literal.element_count;
            if (count == 0) {
                /* Empty array literal - can't infer type */
                diagnostic(node->loc.file, ERR_S033_ARRAY_ELEM_TYPE, node->loc.line,
                           node->loc.column,
                           "Empty array literal requires type context");
                node->expr_type = TYPE_I64;
                return TYPE_I64;
            }
            /* Type-check all elements and resolve common type */
            Type common = typecheck_expression(node->as.array_literal.elements[0],
                                                scope, func_table);
            for (size_t i = 1; i < count; i++) {
                Type elem = typecheck_expression(node->as.array_literal.elements[i],
                                                  scope, func_table);
                Type resolved = resolve_numeric_binary_type(common, elem);
                if (resolved == TYPE_VOID) {
                    /* Try branch resolution for non-numeric types */
                    resolved = resolve_branch_types(common, elem);
                }
                if (resolved == TYPE_VOID) {
                    diagnostic(node->as.array_literal.elements[i]->loc.file, ERR_S033_ARRAY_ELEM_TYPE,
                               node->as.array_literal.elements[i]->loc.line,
                               node->as.array_literal.elements[i]->loc.column,
                               "Array element type mismatch: expected %s, got %s",
                               type_name(common), type_name(elem));
                } else {
                    common = resolved;
                }
            }
            Type arr_type = array_table_intern(common, count);
            node->expr_type = arr_type;
            return arr_type;
        }

        case AST_INDEX_ACCESS: {
            Type obj_type = typecheck_expression(node->as.index_access.object,
                                                  scope, func_table);
            Type idx_type = typecheck_expression(node->as.index_access.index,
                                                  scope, func_table);
            if (!type_is_array(obj_type) && !type_is_slice(obj_type)) {
                diagnostic(node->loc.file, ERR_S035_INDEX_ON_NON_ARRAY, node->loc.line,
                           node->loc.column,
                           "Cannot index non-array type %s",
                           type_name(obj_type));
                node->expr_type = TYPE_I64;
                return TYPE_I64;
            }
            if (!type_is_integer(idx_type)) {
                diagnostic(node->as.index_access.index->loc.file, ERR_S034_INDEX_NOT_INTEGER, node->as.index_access.index->loc.line,
                           node->as.index_access.index->loc.column,
                           "Index must be integer, got %s",
                           type_name(idx_type));
            }
            Type elem_type = type_element_type(obj_type);
            if (elem_type == TYPE_UNKNOWN) elem_type = TYPE_I64;
            node->expr_type = elem_type;
            return elem_type;
        }

        case AST_SLICE_ACCESS: {
            Type obj_type = typecheck_expression(node->as.slice_access.object,
                                                  scope, func_table);
            if (node->as.slice_access.start) {
                Type st = typecheck_expression(node->as.slice_access.start,
                                               scope, func_table);
                if (!type_is_integer(st)) {
                    diagnostic(node->as.slice_access.start->loc.file,
                               ERR_S071_SLICE_RANGE_NOT_INT,
                               node->as.slice_access.start->loc.line,
                               node->as.slice_access.start->loc.column,
                               "Slice range start must be integer, got %s",
                               type_name(st));
                }
            }
            if (node->as.slice_access.end) {
                Type et = typecheck_expression(node->as.slice_access.end,
                                               scope, func_table);
                if (!type_is_integer(et)) {
                    diagnostic(node->as.slice_access.end->loc.file,
                               ERR_S071_SLICE_RANGE_NOT_INT,
                               node->as.slice_access.end->loc.line,
                               node->as.slice_access.end->loc.column,
                               "Slice range end must be integer, got %s",
                               type_name(et));
                }
            }
            if (!type_is_array(obj_type) && !type_is_slice(obj_type)) {
                diagnostic(node->loc.file, ERR_S072_SLICE_ON_NON_ARRAY,
                           node->loc.line, node->loc.column,
                           "Cannot sub-slice non-array/slice type %s",
                           type_name(obj_type));
                node->expr_type = TYPE_I64;
                return TYPE_I64;
            }
            Type elem_type = type_element_type(obj_type);
            if (elem_type == TYPE_UNKNOWN) elem_type = TYPE_I64;
            Type result = slice_table_intern(elem_type);
            node->expr_type = result;
            return result;
        }

        case AST_FIELD_ACCESS: {
            Type obj_type = typecheck_expression(node->as.field_access.object,
                                                  scope, func_table);
            /* Slice .len field access */
            if (type_is_slice(obj_type)) {
                if (node->as.field_access.field_length == 3 &&
                    memcmp(node->as.field_access.field_start, "len", 3) == 0) {
                    node->expr_type = TYPE_I64;
                    return TYPE_I64;
                }
                diagnostic(node->loc.file, ERR_S029_UNKNOWN_FIELD_ACCESS, node->loc.line,
                           node->loc.column,
                           "No field '%.*s' on slice type '%s'",
                           (int)node->as.field_access.field_length,
                           node->as.field_access.field_start,
                           type_name(obj_type));
                node->expr_type = TYPE_I64;
                return TYPE_I64;
            }
            if (!type_is_value(obj_type)) {
                diagnostic(node->loc.file, ERR_S028_FIELD_ON_NON_VALUE, node->loc.line,
                           node->loc.column,
                           "Cannot access field on non-value type %s",
                           type_name(obj_type));
                node->expr_type = TYPE_I64;
                return TYPE_I64;
            }
            int idx = type_value_index(obj_type);
            ValueTypeEntry *vt = &g_value_table->types[idx];
            int fidx = value_table_find_field(vt,
                                               node->as.field_access.field_start,
                                               node->as.field_access.field_length);
            if (fidx < 0) {
                diagnostic(node->loc.file, ERR_S029_UNKNOWN_FIELD_ACCESS, node->loc.line,
                           node->loc.column,
                           "No field '%.*s' on type '%.*s'",
                           (int)node->as.field_access.field_length,
                           node->as.field_access.field_start,
                           (int)vt->name_length, vt->name_start);
                node->expr_type = TYPE_I64;
                return TYPE_I64;
            }
            Type field_type = vt->fields[fidx].type;
            node->expr_type = field_type;
            return field_type;
        }

        case AST_STRING_LITERAL:
            node->expr_type = slice_table_intern(TYPE_U8);
            return node->expr_type;

        case AST_ARENA_NEW: {
            Type cap_type = typecheck_expression(node->as.arena_new.capacity, scope, func_table);
            if (!type_is_integer(cap_type)) {
                diagnostic(node->as.arena_new.capacity->loc.file, ERR_S006_TYPE_MISMATCH, node->as.arena_new.capacity->loc.line,
                           node->as.arena_new.capacity->loc.column,
                           "Arena capacity must be integer, got %s",
                           type_name(cap_type));
            }
            g_has_arena = true;
            node->expr_type = TYPE_ARENA;
            return TYPE_ARENA;
        }

        case AST_ARENA_ALLOC: {
            Type arena_type = typecheck_expression(node->as.arena_alloc.arena, scope, func_table);
            if (!type_is_arena(arena_type)) {
                diagnostic(node->as.arena_alloc.arena->loc.file, ERR_S051_ARENA_ALLOC_TYPE, node->as.arena_alloc.arena->loc.line,
                           node->as.arena_alloc.arena->loc.column,
                           "arena_alloc() requires Arena, got %s",
                           type_name(arena_type));
            }
            /* Check arena is mutable */
            if (node->as.arena_alloc.arena->kind == AST_IDENTIFIER) {
                Variable *v = scope_lookup(scope,
                    node->as.arena_alloc.arena->as.identifier.start,
                    node->as.arena_alloc.arena->as.identifier.length);
                if (v && !v->is_mutable) {
                    diagnostic(node->as.arena_alloc.arena->loc.file, ERR_S052_ARENA_IMMUTABLE, node->as.arena_alloc.arena->loc.line,
                               node->as.arena_alloc.arena->loc.column,
                               "Cannot arena_alloc from immutable Arena, use 'mut'");
                }
            }
            Type count_type = typecheck_expression(node->as.arena_alloc.count, scope, func_table);
            if (!type_is_integer(count_type)) {
                diagnostic(node->as.arena_alloc.count->loc.file, ERR_S006_TYPE_MISMATCH, node->as.arena_alloc.count->loc.line,
                           node->as.arena_alloc.count->loc.column,
                           "arena_alloc() count must be integer, got %s",
                           type_name(count_type));
            }
            /* Ensure arena runtime is emitted (imported functions may use arena_alloc
               without an arena() constructor in the importing program) */
            g_has_arena = true;
            /* expr_type set to TYPE_UNKNOWN — propagated from declaration context */
            node->expr_type = TYPE_UNKNOWN;
            return TYPE_UNKNOWN;
        }

        case AST_ARENA_RESET: {
            Ast *arena_node = node->as.arena_reset.arena;
            Type arena_type = typecheck_expression(arena_node, scope, func_table);
            if (!type_is_arena(arena_type)) {
                diagnostic(arena_node->loc.file, ERR_S051_ARENA_ALLOC_TYPE, arena_node->loc.line,
                           arena_node->loc.column,
                           "arena_reset() requires Arena, got %s",
                           type_name(arena_type));
            }
            /* Check arena is mutable and invalidate its slices */
            if (arena_node->kind == AST_IDENTIFIER) {
                Variable *v = scope_lookup(scope,
                    arena_node->as.identifier.start,
                    arena_node->as.identifier.length);
                if (v && !v->is_mutable) {
                    diagnostic(arena_node->loc.file, ERR_S055_ARENA_RESET_IMMUTABLE,
                               arena_node->loc.line, arena_node->loc.column,
                               "Cannot arena_reset immutable Arena, use 'mut'");
                }
                if (v) {
                    typecheck_invalidate_arena_slices(scope,
                        arena_node->as.identifier.start,
                        arena_node->as.identifier.length);
                }
            }
            node->expr_type = TYPE_VOID;
            return TYPE_VOID;
        }

        case AST_TABLE_ALLOC: {
            Type arena_type = typecheck_expression(node->as.table_alloc.arena, scope, func_table);
            if (!type_is_arena(arena_type)) {
                diagnostic(node->as.table_alloc.arena->loc.file, ERR_S051_ARENA_ALLOC_TYPE, node->as.table_alloc.arena->loc.line,
                           node->as.table_alloc.arena->loc.column,
                           "table_alloc() requires Arena, got %s",
                           type_name(arena_type));
            }
            /* Check arena is mutable */
            if (node->as.table_alloc.arena->kind == AST_IDENTIFIER) {
                Variable *v = scope_lookup(scope,
                    node->as.table_alloc.arena->as.identifier.start,
                    node->as.table_alloc.arena->as.identifier.length);
                if (v && !v->is_mutable) {
                    diagnostic(node->as.table_alloc.arena->loc.file, ERR_S052_ARENA_IMMUTABLE, node->as.table_alloc.arena->loc.line,
                               node->as.table_alloc.arena->loc.column,
                               "Cannot table_alloc from immutable Arena, use 'mut'");
                }
            }
            Type count_type = typecheck_expression(node->as.table_alloc.count, scope, func_table);
            if (!type_is_integer(count_type)) {
                diagnostic(node->as.table_alloc.count->loc.file, ERR_S006_TYPE_MISMATCH, node->as.table_alloc.count->loc.line,
                           node->as.table_alloc.count->loc.column,
                           "table_alloc() count must be integer, got %s",
                           type_name(count_type));
            }
            g_has_arena = true;
            node->expr_type = TYPE_UNKNOWN;
            return TYPE_UNKNOWN;
        }

        case AST_TABLE_LEN: {
            Type table_type = typecheck_expression(node->as.table_len.table, scope, func_table);
            if (!is_table_type(table_type)) {
                diagnostic(node->as.table_len.table->loc.file, ERR_S060_NOT_TABLE_TYPE, node->as.table_len.table->loc.line,
                           node->as.table_len.table->loc.column,
                           "table_len() requires a table type, got %s",
                           type_name(table_type));
            }
            node->expr_type = TYPE_I64;
            return TYPE_I64;
        }

        case AST_TABLE_GET: {
            Type table_type = typecheck_expression(node->as.table_get.table, scope, func_table);
            if (!is_table_type(table_type)) {
                diagnostic(node->as.table_get.table->loc.file, ERR_S060_NOT_TABLE_TYPE, node->as.table_get.table->loc.line,
                           node->as.table_get.table->loc.column,
                           "table_get() requires a table type, got %s",
                           type_name(table_type));
                node->expr_type = TYPE_I64;
                return TYPE_I64;
            }
            Type index_type = typecheck_expression(node->as.table_get.index, scope, func_table);
            if (!type_is_integer(index_type)) {
                diagnostic(node->as.table_get.index->loc.file, ERR_S034_INDEX_NOT_INTEGER, node->as.table_get.index->loc.line,
                           node->as.table_get.index->loc.column,
                           "table_get() index must be integer, got %s",
                           type_name(index_type));
            }
            TableDeclEntry *te = table_decl_for_type(table_type);
            node->expr_type = te->row_type;
            return te->row_type;
        }

        case AST_TABLE_INSERT: {
            Type table_type = typecheck_expression(node->as.table_insert.table, scope, func_table);
            if (!is_table_type(table_type)) {
                diagnostic(node->as.table_insert.table->loc.file, ERR_S060_NOT_TABLE_TYPE, node->as.table_insert.table->loc.line,
                           node->as.table_insert.table->loc.column,
                           "table_insert() requires a table type, got %s",
                           type_name(table_type));
                node->expr_type = TYPE_VOID;
                return TYPE_VOID;
            }
            /* Check table is mutable */
            Ast *table_node = node->as.table_insert.table;
            if (table_node->kind == AST_IDENTIFIER) {
                Variable *v = scope_lookup(scope,
                    table_node->as.identifier.start,
                    table_node->as.identifier.length);
                if (v && !v->is_mutable) {
                    diagnostic(table_node->loc.file, ERR_S052_ARENA_IMMUTABLE, table_node->loc.line,
                               table_node->loc.column,
                               "Cannot table_insert into immutable table, use 'mut'");
                }
            }
            TableDeclEntry *te = table_decl_for_type(table_type);
            Type row_type = typecheck_expression(node->as.table_insert.row, scope, func_table);
            if (!type_can_coerce(row_type, te->row_type)) {
                diagnostic(node->as.table_insert.row->loc.file, ERR_S006_TYPE_MISMATCH, node->as.table_insert.row->loc.line,
                           node->as.table_insert.row->loc.column,
                           "table_insert() row must be %s, got %s",
                           type_name(te->row_type), type_name(row_type));
            }
            node->expr_type = TYPE_VOID;
            return TYPE_VOID;
        }

        case AST_TYPE_CAST: {
            Type operand_type = typecheck_expression(node->as.type_cast.operand, scope, func_table);
            Type target = node->as.type_cast.target_type;

            /* Enum to numeric: treat as integer cast (plain enums only) */
            if (type_is_enum(operand_type) && type_is_numeric(target)) {
                EnumTypeEntry *cast_et = enum_table_get(operand_type);
                if (cast_et && cast_et->has_data) {
                    diagnostic(node->loc.file, ERR_S081_TAGGED_ENUM_CAST,
                               node->loc.line, node->loc.column,
                               "Cannot cast tagged enum %.*s to %s",
                               (int)cast_et->name_length, cast_et->name_start,
                               type_name(target));
                    node->expr_type = target;
                    return target;
                }
                g_has_casts = true;
                node->expr_type = target;
                return target;
            }

            if (!type_is_numeric(operand_type)) {
                diagnostic(node->loc.file, ERR_S063_INVALID_CAST, node->loc.line, node->loc.column,
                           "Cannot cast %s to %s",
                           type_name(operand_type), type_name(target));
                node->expr_type = target;
                return target;
            }

            /* Comptime int → integer: compile-time range check */
            if (operand_type == TYPE_COMPTIME_INT && type_is_integer(target)) {
                typecheck_comptime_range(node->as.type_cast.operand, target, scope);
            }

            /* Comptime float → integer: compile-time truncation + range check */
            if (operand_type == TYPE_COMPTIME_FLOAT && type_is_integer(target)) {
                Ast *op = node->as.type_cast.operand;
                if (is_comptime_constant(op, scope)) {
                    double fval = get_comptime_float(op, scope);
                    const char *tname = check_integer_range((long)fval, target);
                    if (tname) {
                        diagnostic(op->loc.file, ERR_S050_LITERAL_OUT_OF_RANGE, op->loc.line, op->loc.column,
                                   "Value %g out of range for %s", fval, tname);
                    }
                }
            }

            g_has_casts = true;
            node->expr_type = target;
            return target;
        }

        case AST_FD_WRITE: {
            require_native_decl(node, "fd_write", 8,
                "native func fd_write(fd: i32, ref data: [u8]): i64");
            Type fd_type = typecheck_expression(node->as.fd_write.fd, scope, func_table);
            if (!type_is_integer(fd_type)) {
                diagnostic(node->as.fd_write.fd->loc.file, ERR_S064_IO_FD_TYPE, node->as.fd_write.fd->loc.line,
                           node->as.fd_write.fd->loc.column,
                           "fd_write() fd must be integer, got %s",
                           type_name(fd_type));
            }
            Type data_type = typecheck_expression(node->as.fd_write.data, scope, func_table);
            if (!type_is_byte_buffer(data_type)) {
                diagnostic(node->as.fd_write.data->loc.file, ERR_S065_IO_DATA_TYPE, node->as.fd_write.data->loc.line,
                           node->as.fd_write.data->loc.column,
                           "fd_write() data must be []u8, got %s",
                           type_name(data_type));
            }
            g_has_io = true;
            node->expr_type = TYPE_I64;
            return TYPE_I64;
        }

        case AST_FD_READ: {
            require_native_decl(node, "fd_read", 7,
                "native func fd_read(fd: i32, mut ref buf: [u8]): i64");
            Type fd_type = typecheck_expression(node->as.fd_read.fd, scope, func_table);
            if (!type_is_integer(fd_type)) {
                diagnostic(node->as.fd_read.fd->loc.file, ERR_S064_IO_FD_TYPE, node->as.fd_read.fd->loc.line,
                           node->as.fd_read.fd->loc.column,
                           "fd_read() fd must be integer, got %s",
                           type_name(fd_type));
            }
            Type buf_type = typecheck_expression(node->as.fd_read.buf, scope, func_table);
            if (!type_is_byte_buffer(buf_type)) {
                diagnostic(node->as.fd_read.buf->loc.file, ERR_S065_IO_DATA_TYPE, node->as.fd_read.buf->loc.line,
                           node->as.fd_read.buf->loc.column,
                           "fd_read() buffer must be []u8, got %s",
                           type_name(buf_type));
            }
            /* Check buffer is mutable */
            if (node->as.fd_read.buf->kind == AST_IDENTIFIER) {
                Variable *v = scope_lookup(scope,
                    node->as.fd_read.buf->as.identifier.start,
                    node->as.fd_read.buf->as.identifier.length);
                if (v && !v->is_mutable) {
                    diagnostic(node->as.fd_read.buf->loc.file, ERR_S066_IO_BUF_IMMUTABLE, node->as.fd_read.buf->loc.line,
                               node->as.fd_read.buf->loc.column,
                               "fd_read() buffer must be mutable, use 'mut'");
                }
            }
            g_has_io = true;
            node->expr_type = TYPE_I64;
            return TYPE_I64;
        }

        case AST_FD_OPEN: {
            require_native_decl(node, "fd_open", 7,
                "native func fd_open(ref path: str, flags: i32): i32");
            Type path_type = typecheck_expression(node->as.fd_open.path, scope, func_table);
            if (!type_is_byte_buffer(path_type)) {
                diagnostic(node->as.fd_open.path->loc.file, ERR_S065_IO_DATA_TYPE, node->as.fd_open.path->loc.line,
                           node->as.fd_open.path->loc.column,
                           "fd_open() path must be []u8, got %s",
                           type_name(path_type));
            }
            Type flags_type = typecheck_expression(node->as.fd_open.flags, scope, func_table);
            if (!type_is_integer(flags_type)) {
                diagnostic(node->as.fd_open.flags->loc.file, ERR_S064_IO_FD_TYPE, node->as.fd_open.flags->loc.line,
                           node->as.fd_open.flags->loc.column,
                           "fd_open() flags must be integer, got %s",
                           type_name(flags_type));
            }
            g_has_io = true;
            node->expr_type = TYPE_I32;
            return TYPE_I32;
        }

        case AST_FD_CLOSE: {
            require_native_decl(node, "fd_close", 8,
                "native func fd_close(fd: i32): void");
            Type fd_type = typecheck_expression(node->as.fd_close.fd, scope, func_table);
            if (!type_is_integer(fd_type)) {
                diagnostic(node->as.fd_close.fd->loc.file, ERR_S064_IO_FD_TYPE, node->as.fd_close.fd->loc.line,
                           node->as.fd_close.fd->loc.column,
                           "fd_close() fd must be integer, got %s",
                           type_name(fd_type));
            }
            g_has_io = true;
            node->expr_type = TYPE_VOID;
            return TYPE_VOID;
        }

        case AST_FD_SEEK: {
            require_native_decl(node, "fd_seek", 7,
                "native func fd_seek(fd: i32, offset: i64, whence: i32): i64");
            Type fd_type = typecheck_expression(node->as.fd_seek.fd, scope, func_table);
            if (!type_is_integer(fd_type)) {
                diagnostic(node->as.fd_seek.fd->loc.file, ERR_S064_IO_FD_TYPE, node->as.fd_seek.fd->loc.line,
                           node->as.fd_seek.fd->loc.column,
                           "fd_seek() fd must be integer, got %s",
                           type_name(fd_type));
            }
            Type offset_type = typecheck_expression(node->as.fd_seek.offset, scope, func_table);
            if (!type_is_integer(offset_type)) {
                diagnostic(node->as.fd_seek.offset->loc.file, ERR_S064_IO_FD_TYPE, node->as.fd_seek.offset->loc.line,
                           node->as.fd_seek.offset->loc.column,
                           "fd_seek() offset must be integer, got %s",
                           type_name(offset_type));
            }
            Type whence_type = typecheck_expression(node->as.fd_seek.whence, scope, func_table);
            if (!type_is_integer(whence_type)) {
                diagnostic(node->as.fd_seek.whence->loc.file, ERR_S064_IO_FD_TYPE, node->as.fd_seek.whence->loc.line,
                           node->as.fd_seek.whence->loc.column,
                           "fd_seek() whence must be integer, got %s",
                           type_name(whence_type));
            }
            g_has_io = true;
            node->expr_type = TYPE_I64;
            return TYPE_I64;
        }

        case AST_EXIT: {
            require_native_decl(node, "exit", 4,
                "native func exit(code: i32): void");
            Type code_type = typecheck_expression(node->as.exit_call.code, scope, func_table);
            if (!type_is_integer(code_type)) {
                diagnostic(node->as.exit_call.code->loc.file, ERR_S006_TYPE_MISMATCH, node->as.exit_call.code->loc.line,
                           node->as.exit_call.code->loc.column,
                           "exit() code must be integer, got %s",
                           type_name(code_type));
            }
            node->expr_type = TYPE_VOID;
            return TYPE_VOID;
        }

        case AST_MEM_COPY: {
            require_native_decl(node, "mem_copy", 8,
                "native func mem_copy(mut ref dst: [u8], ref src: [u8]): i64");
            Type dst_type = typecheck_expression(node->as.mem_copy.dst, scope, func_table);
            if (!type_is_byte_buffer(dst_type)) {
                diagnostic(node->as.mem_copy.dst->loc.file, ERR_S065_IO_DATA_TYPE, node->as.mem_copy.dst->loc.line,
                           node->as.mem_copy.dst->loc.column,
                           "mem_copy() dst must be []u8, got %s",
                           type_name(dst_type));
            }
            if (node->as.mem_copy.dst->kind == AST_IDENTIFIER) {
                Variable *v = scope_lookup(scope,
                    node->as.mem_copy.dst->as.identifier.start,
                    node->as.mem_copy.dst->as.identifier.length);
                if (v && !v->is_mutable) {
                    diagnostic(node->as.mem_copy.dst->loc.file, ERR_S066_IO_BUF_IMMUTABLE, node->as.mem_copy.dst->loc.line,
                               node->as.mem_copy.dst->loc.column,
                               "mem_copy() dst must be mutable, use 'mut'");
                }
            }
            Type src_type = typecheck_expression(node->as.mem_copy.src, scope, func_table);
            if (!type_is_byte_buffer(src_type)) {
                diagnostic(node->as.mem_copy.src->loc.file, ERR_S065_IO_DATA_TYPE, node->as.mem_copy.src->loc.line,
                           node->as.mem_copy.src->loc.column,
                           "mem_copy() src must be []u8, got %s",
                           type_name(src_type));
            }
            g_has_mem = true;
            node->expr_type = TYPE_I64;
            return TYPE_I64;
        }

        case AST_ARGS: {
            require_native_decl(node, "args", 4,
                "native func args(mut ref mem: Arena): [str]");
            Type arena_type = typecheck_expression(node->as.args.arena, scope, func_table);
            if (!type_is_arena(arena_type)) {
                diagnostic(node->as.args.arena->loc.file, ERR_S051_ARENA_ALLOC_TYPE, node->as.args.arena->loc.line,
                           node->as.args.arena->loc.column,
                           "args() requires Arena, got %s",
                           type_name(arena_type));
            }
            /* Check arena is mutable */
            if (node->as.args.arena->kind == AST_IDENTIFIER) {
                Variable *v = scope_lookup(scope,
                    node->as.args.arena->as.identifier.start,
                    node->as.args.arena->as.identifier.length);
                if (v && !v->is_mutable) {
                    diagnostic(node->as.args.arena->loc.file, ERR_S052_ARENA_IMMUTABLE, node->as.args.arena->loc.line,
                               node->as.args.arena->loc.column,
                               "Cannot args() from immutable Arena, use 'mut'");
                }
            }
            g_has_arena = true;
            g_has_args = true;
            Type str_slice = slice_table_intern(TYPE_U8);
            Type args_type = slice_table_intern(str_slice);
            node->expr_type = args_type;
            return args_type;
        }

        case AST_SHELL_RUN: {
            require_native_decl(node, "shell_run", 9,
                "native func shell_run(ref command: str): i32");
            Type command_type = typecheck_expression(node->as.shell_run.command, scope, func_table);
            if (!type_is_byte_buffer(command_type)) {
                diagnostic(node->as.shell_run.command->loc.file, ERR_S065_IO_DATA_TYPE,
                           node->as.shell_run.command->loc.line,
                           node->as.shell_run.command->loc.column,
                           "shell_run() command must be []u8, got %s",
                           type_name(command_type));
            }
            g_has_driver = true;
            node->expr_type = TYPE_I32;
            return TYPE_I32;
        }

        case AST_EXE_DIR: {
            require_native_decl(node, "exe_dir", 7,
                "native func exe_dir(mut ref mem: Arena): str");
            Type arena_type = typecheck_expression(node->as.exe_dir.arena, scope, func_table);
            if (!type_is_arena(arena_type)) {
                diagnostic(node->as.exe_dir.arena->loc.file, ERR_S051_ARENA_ALLOC_TYPE,
                           node->as.exe_dir.arena->loc.line,
                           node->as.exe_dir.arena->loc.column,
                           "exe_dir() requires Arena, got %s",
                           type_name(arena_type));
            }
            if (node->as.exe_dir.arena->kind == AST_IDENTIFIER) {
                Variable *v = scope_lookup(scope,
                    node->as.exe_dir.arena->as.identifier.start,
                    node->as.exe_dir.arena->as.identifier.length);
                if (v && !v->is_mutable) {
                    diagnostic(node->as.exe_dir.arena->loc.file, ERR_S052_ARENA_IMMUTABLE,
                               node->as.exe_dir.arena->loc.line,
                               node->as.exe_dir.arena->loc.column,
                               "Cannot exe_dir() from immutable Arena, use 'mut'");
                }
            }
            g_has_arena = true;
            g_has_args = true;
            g_has_driver = true;
            Type str_slice = slice_table_intern(TYPE_U8);
            node->expr_type = str_slice;
            return str_slice;
        }

        case AST_PID:
            require_native_decl(node, "pid", 3,
                "native func pid(): i64");
            g_has_driver = true;
            node->expr_type = TYPE_I64;
            return TYPE_I64;

        case AST_TIME_NS:
            require_native_decl(node, "time_ns", 7,
                "native func time_ns(): i64");
            g_has_time = true;
            node->expr_type = TYPE_I64;
            return TYPE_I64;

        default:
            break;
    }

    node->expr_type = TYPE_I64;  /* Default */
    return TYPE_I64;
}

/* Check if a long value fits in the target type's range.
 * Returns range description string on failure, NULL on success. */
static const char *check_integer_range(long value, Type target) {
    switch (target) {
        case TYPE_U8:
            if (value < 0 || value > UINT8_MAX) return "u8 (0..255)";
            break;
        case TYPE_I32:
            if (value < INT32_MIN || value > INT32_MAX) return "i32 (-2147483648..2147483647)";
            break;
        case TYPE_U32:
            if (value < 0 || value > (long)UINT32_MAX) return "u32 (0..4294967295)";
            break;
        default:
            break;
    }
    return NULL;
}

/* Check if a comptime_int value fits in the target type's range */
static void typecheck_comptime_range(Ast *init, Type target, Scope *scope) {
    if (!is_comptime_constant(init, scope)) return;
    long value = get_comptime_int(init, scope);
    const char *tname = check_integer_range(value, target);
    if (tname) {
        diagnostic(init->loc.file, ERR_S050_LITERAL_OUT_OF_RANGE, init->loc.line, init->loc.column,
                   "Value %ld out of range for %s", value, tname);
    }
}

/* Report a type coercion error, with a specific diagnostic for array length mismatch */
static void typecheck_coercion_error(Type init_type, Type declared, SourceLoc loc) {
    if (type_is_array(init_type) && type_is_array(declared)) {
        ArrayTypeEntry *fa = array_table_get(init_type);
        ArrayTypeEntry *ta = array_table_get(declared);
        if (fa && ta && fa->size != ta->size) {
            diagnostic(loc.file, ERR_S032_ARRAY_LENGTH_MISMATCH, loc.line, loc.column,
                       "Array literal has %zu elements, expected %zu",
                       fa->size, ta->size);
            return;
        }
    }
    diagnostic(loc.file, ERR_S006_TYPE_MISMATCH, loc.line, loc.column,
               "Cannot assign %s to variable of type %s",
               type_name(init_type), type_name(declared));
}

/* Emit S043 if a non-copyable type is initialized from a copy (not a constructor, call, or arena) */
static void typecheck_struct_copy(Type type, Ast *init) {
    if (type_is_non_copyable(type) &&
        init->kind != AST_VALUE_CONSTRUCTOR && init->kind != AST_FUNC_CALL &&
        init->kind != AST_ARENA_NEW && init->kind != AST_TABLE_ALLOC &&
        init->kind != AST_ENUM_VARIANT && init->kind != AST_MATCH) {
        diagnostic(init->loc.file, ERR_S043_STRUCT_COPY, init->loc.line, init->loc.column,
                   "Cannot copy %s (not copyable)", type_name(type));
    }
}

/* Invalidate all slices allocated from a given arena across the entire scope chain */
static void typecheck_invalidate_arena_slices(Scope *scope,
                                               const char *arena_start,
                                               size_t arena_length) {
    while (scope != NULL) {
        for (size_t i = 0; i < scope->count; i++) {
            Variable *v = &scope->vars[i];
            if ((type_is_slice(v->type) || is_table_type(v->type) || type_is_non_copyable(v->type)) &&
                v->arena_source_start != NULL &&
                v->arena_source_length == arena_length &&
                memcmp(v->arena_source_start, arena_start, arena_length) == 0) {
                v->is_invalidated = true;
            }
        }
        scope = scope->parent;
    }
}

/* Track whether a variable was allocated from a local (non-ref) arena */
static void typecheck_mark_arena_locality(Variable *sv, Ast *arena_node, Scope *scope) {
    if (!sv) return;
    if (arena_node->kind != AST_IDENTIFIER) return;
    Variable *av = scope_lookup(scope, arena_node->as.identifier.start,
                                arena_node->as.identifier.length);
    if (av) {
        sv->arena_is_local = !av->is_ref && !av->is_global;
        sv->arena_source_start = arena_node->as.identifier.start;
        sv->arena_source_length = arena_node->as.identifier.length;
    }
}

/* Check if a return type contains slices (bare slice, or struct with slice fields) */
static bool type_return_has_slices(Type type) {
    if (type_is_slice(type)) return true;
    if (enum_has_slice_payload(type)) return true;
    if (!type_is_struct(type) || type == TYPE_ARENA) return false;
    ValueTypeEntry *vt = value_table_get(type);
    if (!vt) return false;
    for (size_t i = 0; i < vt->field_count; i++) {
        if (type_is_slice(vt->fields[i].type)) return true;
    }
    return false;
}

/* Check if a return value contains slices that escape a local arena.
 * Handles three cases: bare slice, function call returning slices, struct constructor. */
static void typecheck_slice_escape(Ast *value, Type return_type, Scope *scope,
                                   FunctionTable *func_table) {
    /* Case 1: bare slice return */
    if (type_is_slice(return_type) && value->kind == AST_IDENTIFIER) {
        Variable *sv = scope_lookup(scope, value->as.identifier.start,
                                    value->as.identifier.length);
        if (sv && sv->arena_source_start) {
            if (sv->arena_is_local) {
                diagnostic(value->loc.file, ERR_S053_SLICE_ESCAPES_ARENA,
                           value->loc.line, value->loc.column,
                           "Slice '%.*s' escapes local arena",
                           (int)value->as.identifier.length,
                           value->as.identifier.start);
            } else if (g_current_func_entry) {
                g_current_func_entry->returns_arena_slices = true;
            }
        }
        return;
    }

    /* Case 2: function call returning slices — check arena args for escape/dependency */
    if (value->kind == AST_FUNC_CALL && type_return_has_slices(return_type)) {
        FunctionEntry *callee = func_table_lookup(func_table,
            value->as.func_call.name_start, value->as.func_call.name_length);
        if (!callee) return;
        for (size_t i = 0; i < callee->param_count && i < value->as.func_call.arg_count; i++) {
            if (callee->param_types[i] != TYPE_ARENA || !callee->param_is_ref[i])
                continue;
            Ast *root = ast_root_target(value->as.func_call.arguments[i]);
            if (root->kind != AST_IDENTIFIER) continue;
            Variable *av = scope_lookup(scope, root->as.identifier.start,
                                        root->as.identifier.length);
            if (!av) continue;
            if (!av->is_ref) {
                /* Local arena — defer check (callee may not be analyzed yet) */
                deferred_arena_check_add(value->loc,
                    callee->name_start, callee->name_length,
                    root->as.identifier.start, root->as.identifier.length);
            } else if (g_current_func_entry) {
                /* Ref-param arena — record transitive dependency */
                arena_dep_add(g_current_func_entry->name_start,
                              g_current_func_entry->name_length,
                              callee->name_start, callee->name_length);
            }
            break;
        }
        return;
    }

    /* Case 3: struct constructor — check slice fields */
    if (value->kind != AST_VALUE_CONSTRUCTOR) return;
    if (!type_is_struct(return_type) || return_type == TYPE_ARENA) return;
    ValueTypeEntry *vt = value_table_get(return_type);
    if (!vt) return;
    FieldInit *fi = value->as.value_constructor.fields;
    size_t fc = value->as.value_constructor.field_count;
    for (size_t i = 0; i < fc; i++) {
        int fidx = value_table_find_field(vt, fi[i].name_start, fi[i].name_length);
        if (fidx < 0 || !type_is_slice(vt->fields[fidx].type)) continue;
        if (fi[i].value->kind != AST_IDENTIFIER) continue;
        Variable *sv = scope_lookup(scope, fi[i].value->as.identifier.start,
                                    fi[i].value->as.identifier.length);
        if (sv && sv->arena_is_local) {
            diagnostic(fi[i].value->loc.file, ERR_S053_SLICE_ESCAPES_ARENA,
                       fi[i].value->loc.line, fi[i].value->loc.column,
                       "Slice '%.*s' in returned struct escapes local arena",
                       (int)fi[i].value->as.identifier.length,
                       fi[i].value->as.identifier.start);
        } else if (sv && sv->arena_source_start && g_current_func_entry) {
            g_current_func_entry->returns_arena_slices = true;
        }
    }
}

static void typecheck_statement_block(Ast *block, Scope **scope, Type return_type,
                                      FunctionTable *func_table) {
    for (size_t i = 0; i < block->as.block.count; i++) {
        typecheck_statement(block->as.block.statements[i], scope, return_type, func_table);
    }

    if (block->as.block.value_expr) {
        Ast *tail = block->as.block.value_expr;
        if (tail->kind == AST_IF || tail->kind == AST_MATCH) {
            typecheck_statement(tail, scope, return_type, func_table);
        } else {
            typecheck_expression(tail, *scope, func_table);
        }
    }
}

static void typecheck_statement(Ast *node, Scope **scope, Type return_type, FunctionTable *func_table) {
    switch (node->kind) {
        case AST_VAL_DECL: {
            Type init_type = typecheck_expression(node->as.val_decl.initializer, *scope, func_table);
            Type declared = node->as.val_decl.type;

            /* Table local via table_alloc — propagate declared type */
            if (is_table_type(declared) &&
                node->as.val_decl.initializer->kind == AST_TABLE_ALLOC) {
                node->as.val_decl.initializer->expr_type = declared;
                Variable *sv = scope_add(*scope, node->as.val_decl.name_start,
                          node->as.val_decl.name_length, false, declared, node->loc);
                typecheck_mark_arena_locality(sv,
                    node->as.val_decl.initializer->as.table_alloc.arena, *scope);
                break;
            }

            /* Slice local via arena alloc — propagate declared type */
            if (type_is_slice(declared) &&
                node->as.val_decl.initializer->kind == AST_ARENA_ALLOC) {
                node->as.val_decl.initializer->expr_type = declared;
                Variable *sv = scope_add(*scope, node->as.val_decl.name_start,
                          node->as.val_decl.name_length, false, declared, node->loc);
                typecheck_mark_arena_locality(sv,
                    node->as.val_decl.initializer->as.arena_alloc.arena, *scope);
                break;
            }

            /* Slice local via args() — returns [str] from arena */
            if (type_is_slice(declared) &&
                node->as.val_decl.initializer->kind == AST_ARGS) {
                Variable *sv = scope_add(*scope, node->as.val_decl.name_start,
                          node->as.val_decl.name_length, false, declared, node->loc);
                typecheck_mark_arena_locality(sv,
                    node->as.val_decl.initializer->as.args.arena, *scope);
                break;
            }

            /* Slice local via exe_dir() — returns str from arena */
            if (type_is_slice(declared) &&
                node->as.val_decl.initializer->kind == AST_EXE_DIR) {
                Variable *sv = scope_add(*scope, node->as.val_decl.name_start,
                          node->as.val_decl.name_length, false, declared, node->loc);
                typecheck_mark_arena_locality(sv,
                    node->as.val_decl.initializer->as.exe_dir.arena, *scope);
                break;
            }

            /* String literal exemption: val s: str = "..." */
            if (type_is_str(declared) &&
                node->as.val_decl.initializer->kind == AST_STRING_LITERAL) {
                node->as.val_decl.initializer->expr_type = declared;
                scope_add(*scope, node->as.val_decl.name_start,
                          node->as.val_decl.name_length, false, declared, node->loc);
                break;
            }

            /* Slice local via function call — allowed (escape checked at call site) */
            if (type_is_slice(declared) &&
                node->as.val_decl.initializer->kind == AST_FUNC_CALL) {
                scope_add(*scope, node->as.val_decl.name_start,
                          node->as.val_decl.name_length, false, declared, node->loc);
                break;
            }

            /* Slice local via sub-slicing — allowed (borrows from source) */
            if (type_is_slice(declared) &&
                node->as.val_decl.initializer->kind == AST_SLICE_ACCESS) {
                scope_add(*scope, node->as.val_decl.name_start,
                          node->as.val_decl.name_length, false, declared, node->loc);
                break;
            }

            /* Slice locals only allowed via alloc or function call */
            if (type_is_slice(declared)) {
                diagnostic(node->loc.file, ERR_S046_SLICE_LOCAL_VAR, node->loc.line,
                           node->loc.column,
                           "Slice type not allowed as local variable (use arena_alloc)");
            }

            /* Slice-bearing enum via function call — allowed (escape checked at call site) */
            if (enum_has_slice_payload(declared) &&
                node->as.val_decl.initializer->kind == AST_FUNC_CALL) {
                if (!type_can_coerce(init_type, declared)) {
                    typecheck_coercion_error(init_type, declared,
                                             node->as.val_decl.initializer->loc);
                }
                scope_add(*scope, node->as.val_decl.name_start,
                          node->as.val_decl.name_length, false, declared, node->loc);
                break;
            }

            if (declared == TYPE_UNKNOWN) {
                /* No explicit type — must be comptime */
                if (!type_is_comptime(init_type)) {
                    diagnostic(node->loc.file, ERR_S020_TYPE_REQUIRED, node->loc.line, node->loc.column,
                               "Type annotation required (expression is not compile-time constant)");
                    declared = TYPE_I64;  /* fallback */
                } else {
                    declared = init_type;  /* inherit comptime type */
                }
                node->as.val_decl.type = declared;  /* update AST */
            } else {
                /* Explicit type — check coercion */
                if (!type_can_coerce(init_type, declared)) {
                    typecheck_coercion_error(init_type, declared,
                                             node->as.val_decl.initializer->loc);
                }
                if (init_type == TYPE_COMPTIME_INT && type_is_integer(declared)) {
                    typecheck_comptime_range(node->as.val_decl.initializer, declared, *scope);
                }
                /* Propagate declared type into array literal for codegen */
                if (type_is_array(init_type) && type_is_array(declared) &&
                    init_type != declared) {
                    node->as.val_decl.initializer->expr_type = declared;
                }
            }

            typecheck_struct_copy(declared, node->as.val_decl.initializer);

            /* Add to scope with comptime info if applicable */
            if (type_is_comptime(declared) && is_comptime_constant(node->as.val_decl.initializer, *scope)) {
                if (declared == TYPE_COMPTIME_INT) {
                    scope_add_comptime_int(*scope, node->as.val_decl.name_start,
                                           node->as.val_decl.name_length,
                                           get_comptime_int(node->as.val_decl.initializer, *scope),
                                           node->loc);
                } else {
                    scope_add_comptime_float(*scope, node->as.val_decl.name_start,
                                             node->as.val_decl.name_length,
                                             get_comptime_float(node->as.val_decl.initializer, *scope),
                                             node->loc);
                }
            } else {
                scope_add(*scope, node->as.val_decl.name_start,
                          node->as.val_decl.name_length, false, declared, node->loc);
            }
            break;
        }

        case AST_MUT_DECL: {
            Type init_type = typecheck_expression(node->as.mut_decl.initializer, *scope, func_table);
            Type declared = node->as.mut_decl.type;

            /* Table local via table_alloc — propagate declared type */
            if (is_table_type(declared) &&
                node->as.mut_decl.initializer->kind == AST_TABLE_ALLOC) {
                node->as.mut_decl.initializer->expr_type = declared;
                Variable *sv = scope_add(*scope, node->as.mut_decl.name_start,
                          node->as.mut_decl.name_length, true, declared, node->loc);
                typecheck_mark_arena_locality(sv,
                    node->as.mut_decl.initializer->as.table_alloc.arena, *scope);
                break;
            }

            /* Slice local via arena alloc — propagate declared type */
            if (type_is_slice(declared) &&
                node->as.mut_decl.initializer->kind == AST_ARENA_ALLOC) {
                node->as.mut_decl.initializer->expr_type = declared;
                Variable *sv = scope_add(*scope, node->as.mut_decl.name_start,
                          node->as.mut_decl.name_length, true, declared, node->loc);
                typecheck_mark_arena_locality(sv,
                    node->as.mut_decl.initializer->as.arena_alloc.arena, *scope);
                break;
            }

            /* Slice local via args() — returns [str] from arena */
            if (type_is_slice(declared) &&
                node->as.mut_decl.initializer->kind == AST_ARGS) {
                Variable *sv = scope_add(*scope, node->as.mut_decl.name_start,
                          node->as.mut_decl.name_length, true, declared, node->loc);
                typecheck_mark_arena_locality(sv,
                    node->as.mut_decl.initializer->as.args.arena, *scope);
                break;
            }

            /* Slice local via exe_dir() — returns str from arena */
            if (type_is_slice(declared) &&
                node->as.mut_decl.initializer->kind == AST_EXE_DIR) {
                Variable *sv = scope_add(*scope, node->as.mut_decl.name_start,
                          node->as.mut_decl.name_length, true, declared, node->loc);
                typecheck_mark_arena_locality(sv,
                    node->as.mut_decl.initializer->as.exe_dir.arena, *scope);
                break;
            }

            /* String literal with mut is an error — mutating static memory is UB */
            if (type_is_str(declared) &&
                node->as.mut_decl.initializer->kind == AST_STRING_LITERAL) {
                diagnostic(node->loc.file, ERR_S054_STRING_LITERAL_MUT, node->loc.line,
                           node->loc.column,
                           "String literals cannot be mutable (use 'val')");
                /* Still add to scope for error recovery */
                scope_add(*scope, node->as.mut_decl.name_start,
                          node->as.mut_decl.name_length, true, declared, node->loc);
                break;
            }

            /* Slice local via function call — allowed (escape checked at call site) */
            if (type_is_slice(declared) &&
                node->as.mut_decl.initializer->kind == AST_FUNC_CALL) {
                scope_add(*scope, node->as.mut_decl.name_start,
                          node->as.mut_decl.name_length, true, declared, node->loc);
                break;
            }

            /* Slice local via sub-slicing — allowed (borrows from source) */
            if (type_is_slice(declared) &&
                node->as.mut_decl.initializer->kind == AST_SLICE_ACCESS) {
                scope_add(*scope, node->as.mut_decl.name_start,
                          node->as.mut_decl.name_length, true, declared, node->loc);
                break;
            }

            /* Slice locals only allowed via alloc or function call */
            if (type_is_slice(declared)) {
                diagnostic(node->loc.file, ERR_S046_SLICE_LOCAL_VAR, node->loc.line,
                           node->loc.column,
                           "Slice type not allowed as local variable (use arena_alloc)");
            }

            /* Slice-bearing enum via function call — allowed (escape checked at call site) */
            if (enum_has_slice_payload(declared) &&
                node->as.mut_decl.initializer->kind == AST_FUNC_CALL) {
                if (!type_can_coerce(init_type, declared)) {
                    typecheck_coercion_error(init_type, declared,
                                             node->as.mut_decl.initializer->loc);
                }
                scope_add(*scope, node->as.mut_decl.name_start,
                          node->as.mut_decl.name_length, true, declared, node->loc);
                break;
            }

            if (!type_can_coerce(init_type, declared)) {
                typecheck_coercion_error(init_type, declared,
                                         node->as.mut_decl.initializer->loc);
            }
            if (init_type == TYPE_COMPTIME_INT && type_is_integer(declared)) {
                typecheck_comptime_range(node->as.mut_decl.initializer, declared, *scope);
            }
            /* Propagate declared type into array literal for codegen */
            if (type_is_array(init_type) && type_is_array(declared) &&
                init_type != declared) {
                node->as.mut_decl.initializer->expr_type = declared;
            }

            typecheck_struct_copy(declared, node->as.mut_decl.initializer);

            scope_add(*scope, node->as.mut_decl.name_start,
                      node->as.mut_decl.name_length, true, declared, node->loc);
            break;
        }

        case AST_ASSIGNMENT: {
            Ast *target = node->as.assignment.target;
            Type target_type = typecheck_assignment_target(target, *scope,
                                                           func_table, node->loc);

            Type value_type = typecheck_expression(node->as.assignment.value, *scope, func_table);
            if (!type_can_coerce(value_type, target_type)) {
                diagnostic(node->as.assignment.value->loc.file, ERR_S006_TYPE_MISMATCH, node->as.assignment.value->loc.line,
                           node->as.assignment.value->loc.column,
                           "Cannot assign %s to %s",
                           type_name(value_type), type_name(target_type));
            }
            if (value_type == TYPE_COMPTIME_INT && type_is_integer(target_type)) {
                typecheck_comptime_range(node->as.assignment.value, target_type, *scope);
            }
            break;
        }

        case AST_COMPOUND_ASSIGNMENT: {
            Ast *target = node->as.compound_assignment.target;
            Type target_type = typecheck_assignment_target(target, *scope,
                                                           func_table, node->loc);
            Type value_type = typecheck_expression(
                node->as.compound_assignment.value, *scope, func_table);
            Type result_type = typecheck_compound_assignment(
                node->as.compound_assignment.op, target_type, value_type,
                node->loc);

            if (!type_can_coerce(result_type, target_type)) {
                diagnostic(node->as.compound_assignment.value->loc.file,
                           ERR_S006_TYPE_MISMATCH,
                           node->as.compound_assignment.value->loc.line,
                           node->as.compound_assignment.value->loc.column,
                           "Cannot assign %s to %s",
                           type_name(result_type), type_name(target_type));
            }
            break;
        }

        case AST_RETURN: {
            Ast *value = node->as.return_stmt.value;

            if (return_type == TYPE_VOID) {
                /* Void function should not return a value */
                if (value != NULL) {
                    diagnostic(node->loc.file, ERR_S014_VOID_RETURN_VALUE, node->loc.line,
                               node->loc.column,
                               "Void function cannot return a value");
                }
            } else {
                /* Non-void function must return a value */
                if (value == NULL) {
                    diagnostic(node->loc.file, ERR_S015_MISSING_RETURN_VALUE, node->loc.line,
                               node->loc.column,
                               "Function must return a value of type %s",
                               type_name(return_type));
                } else {
                    Type value_type = typecheck_expression(value, *scope, func_table);
                    if (!type_can_coerce(value_type, return_type)) {
                        diagnostic(node->loc.file, ERR_S013_RETURN_TYPE_MISMATCH, node->loc.line,
                                   node->loc.column,
                                   "Return type mismatch: expected %s, got %s",
                                   type_name(return_type), type_name(value_type));
                    }
                    if (value_type == TYPE_COMPTIME_INT && type_is_integer(return_type)) {
                        typecheck_comptime_range(value, return_type, *scope);
                    }
                    typecheck_slice_escape(value, return_type, *scope, func_table);
                }
            }
            break;
        }

        case AST_ASSERT: {
            Type cond_type = typecheck_expression(node->as.assert_stmt.condition, *scope, func_table);
            if (cond_type != TYPE_BOOL) {
                diagnostic(node->loc.file, ERR_S007_CONDITION_NOT_BOOL, node->loc.line,
                           node->loc.column,
                           "Assert condition must be bool, got %s",
                           type_name(cond_type));
            }
            break;
        }

        case AST_IF: {
            Type cond_type = typecheck_expression(node->as.if_stmt.condition, *scope, func_table);
            if (cond_type != TYPE_BOOL) {
                diagnostic(node->loc.file, ERR_S007_CONDITION_NOT_BOOL, node->loc.line,
                           node->loc.column,
                           "If condition must be bool, got %s",
                           type_name(cond_type));
            }

            /* Typecheck then block */
            Ast *then_block = node->as.if_stmt.then_block;
            Scope *then_scope = scope_create(*scope);
            typecheck_statement_block(then_block, &then_scope, return_type, func_table);
            scope_destroy(then_scope);

            /* Typecheck else block if present */
            if (node->as.if_stmt.else_block) {
                Ast *else_block = node->as.if_stmt.else_block;
                Scope *else_scope = scope_create(*scope);
                typecheck_statement_block(else_block, &else_scope, return_type, func_table);
                scope_destroy(else_scope);
            }
            break;
        }

        case AST_WHILE: {
            Type cond_type = typecheck_expression(node->as.while_stmt.condition, *scope, func_table);
            if (cond_type != TYPE_BOOL) {
                diagnostic(node->loc.file, ERR_S007_CONDITION_NOT_BOOL, node->loc.line,
                           node->loc.column,
                           "While condition must be bool, got %s",
                           type_name(cond_type));
            }

            /* Typecheck body */
            Ast *body = node->as.while_stmt.body;
            Scope *body_scope = scope_create(*scope);
            body_scope->loop_depth = (*scope)->loop_depth + 1;
            typecheck_statement_block(body, &body_scope, return_type, func_table);
            scope_destroy(body_scope);
            break;
        }

        case AST_FOR: {
            /* Typecheck start and end — both must be integer types */
            Type start_type = typecheck_expression(node->as.for_stmt.start, *scope, func_table);
            if (!type_is_integer(start_type)) {
                diagnostic(node->as.for_stmt.start->loc.file, ERR_S058_FOR_RANGE_NOT_INTEGER, node->as.for_stmt.start->loc.line,
                           node->as.for_stmt.start->loc.column,
                           "For-loop range bound must be integer, got %s",
                           type_name(start_type));
            }
            Type end_type = typecheck_expression(node->as.for_stmt.end, *scope, func_table);
            if (!type_is_integer(end_type)) {
                diagnostic(node->as.for_stmt.end->loc.file, ERR_S058_FOR_RANGE_NOT_INTEGER, node->as.for_stmt.end->loc.line,
                           node->as.for_stmt.end->loc.column,
                           "For-loop range bound must be integer, got %s",
                           type_name(end_type));
            }

            /* Typecheck body with loop variable in scope */
            Ast *body = node->as.for_stmt.body;
            Scope *body_scope = scope_create(*scope);
            body_scope->loop_depth = (*scope)->loop_depth + 1;
            scope_add(body_scope, node->as.for_stmt.var_start,
                      node->as.for_stmt.var_length, false, TYPE_I64, node->loc);
            typecheck_statement_block(body, &body_scope, return_type, func_table);
            scope_destroy(body_scope);
            break;
        }

        case AST_BREAK:
            if ((*scope)->loop_depth == 0) {
                diagnostic(node->loc.file, ERR_S004_BREAK_OUTSIDE_LOOP, node->loc.line,
                           node->loc.column, "'break' outside of loop");
            }
            break;

        case AST_CONTINUE:
            if ((*scope)->loop_depth == 0) {
                diagnostic(node->loc.file, ERR_S005_CONTINUE_OUTSIDE_LOOP, node->loc.line,
                           node->loc.column, "'continue' outside of loop");
            }
            break;

        case AST_BLOCK: {
            Scope *block_scope = scope_create(*scope);
            typecheck_statement_block(node, &block_scope, return_type, func_table);
            scope_destroy(block_scope);
            break;
        }

        case AST_FUNC_CALL:
        case AST_ARENA_RESET:
        case AST_TABLE_INSERT:
        case AST_FD_WRITE:
        case AST_FD_READ:
        case AST_FD_CLOSE:
        case AST_FD_SEEK:
        case AST_MEM_COPY:
        case AST_EXIT:
        case AST_ARGS:
        case AST_SHELL_RUN:
        case AST_EXE_DIR:
            typecheck_expression(node, *scope, func_table);
            break;

        case AST_MATCH:
            typecheck_match_arms(node, *scope, return_type, func_table);
            break;

        default:
            break;
    }
}

/* Check if an expression is valid as a global variable initializer.
 * Allowed: literals, comptime constants, value constructors with constant fields,
 * array literals with constant elements, arena() constructor, string literals.
 * NOT allowed: function calls, arena_alloc(), runtime expressions. */
static bool is_global_initializer(Ast *node, Scope *scope) {
    switch (node->kind) {
        case AST_NUMBER:
        case AST_FLOAT:
        case AST_BOOLEAN:
        case AST_STRING_LITERAL:
            return true;
        case AST_ENUM_VARIANT:
            if (node->as.enum_variant.payload)
                return is_global_initializer(node->as.enum_variant.payload, scope);
            return true;
        case AST_IDENTIFIER: {
            Variable *v = scope_lookup(scope, node->as.identifier.start,
                                       node->as.identifier.length);
            return v != NULL && v->is_comptime;
        }
        case AST_UNARY: {
            return is_global_initializer(node->as.unary.operand, scope);
        }
        case AST_BINARY: {
            return is_global_initializer(node->as.binary.left, scope) &&
                   is_global_initializer(node->as.binary.right, scope);
        }
        case AST_VALUE_CONSTRUCTOR: {
            for (size_t i = 0; i < node->as.value_constructor.field_count; i++) {
                if (!is_global_initializer(node->as.value_constructor.fields[i].value, scope)) {
                    return false;
                }
            }
            return true;
        }
        case AST_ARRAY_LITERAL: {
            for (size_t i = 0; i < node->as.array_literal.element_count; i++) {
                if (!is_global_initializer(node->as.array_literal.elements[i], scope)) {
                    return false;
                }
            }
            return true;
        }
        case AST_IF: {
            /* Comptime if/else: both branches must be value expressions */
            if (!node->as.if_stmt.else_block) return false;
            if (!node->as.if_stmt.then_block->as.block.value_expr) return false;
            if (!node->as.if_stmt.else_block->as.block.value_expr) return false;
            return is_global_initializer(node->as.if_stmt.condition, scope) &&
                   is_global_initializer(node->as.if_stmt.then_block->as.block.value_expr, scope) &&
                   is_global_initializer(node->as.if_stmt.else_block->as.block.value_expr, scope);
        }
        default:
            return false;
    }
}

/* Validate initializer coercion to declared type and propagate array types */
static void typecheck_global_coerce(Type init_type, Type declared,
                                    Ast *init, Scope *scope) {
    if (!type_can_coerce(init_type, declared)) {
        typecheck_coercion_error(init_type, declared, init->loc);
    }
    if (init_type == TYPE_COMPTIME_INT && type_is_integer(declared)) {
        typecheck_comptime_range(init, declared, scope);
    }
    if (type_is_array(init_type) && type_is_array(declared) &&
        init_type != declared) {
        init->expr_type = declared;
    }
}

/* Add a global variable to scope and mark it as global */
static Variable *scope_add_global(Scope *scope, const char *name_start,
                                  size_t name_length, bool is_mutable,
                                  Type type, SourceLoc loc) {
    Variable *v = scope_add(scope, name_start, name_length,
                            is_mutable, type, loc);
    if (v) {
        v->is_global = true;
        v->module_id = module_for_file(loc.file);
    }
    return v;
}

/* Type check a global variable declaration */
static void typecheck_global_decl(Ast *node, Scope *scope, FunctionTable *func_table) {
    if (node->kind == AST_VAL_DECL) {
        Type init_type = typecheck_expression(node->as.val_decl.initializer, scope, func_table);
        Type declared = node->as.val_decl.type;
        Ast *init = node->as.val_decl.initializer;

        /* String literal exemption: val s: str = "..." */
        if (type_is_str(declared) && init->kind == AST_STRING_LITERAL) {
            init->expr_type = declared;
            scope_add_global(scope, node->as.val_decl.name_start,
                             node->as.val_decl.name_length, false, declared,
                             node->loc);
            return;
        }

        /* Check initializer is a constant expression */
        if (!is_global_initializer(init, scope)) {
            diagnostic(node->loc.file, ERR_S057_GLOBAL_NOT_CONSTANT, node->loc.line, node->loc.column,
                       "Global variable initializer must be a constant expression");
        }

        /* Reject slice globals (except string literals handled above) */
        if (type_is_slice(declared)) {
            diagnostic(node->loc.file, ERR_S046_SLICE_LOCAL_VAR, node->loc.line, node->loc.column,
                       "Slice type not allowed as global variable");
        }

        if (declared == TYPE_UNKNOWN) {
            if (!type_is_comptime(init_type)) {
                diagnostic(node->loc.file, ERR_S020_TYPE_REQUIRED, node->loc.line, node->loc.column,
                           "Type annotation required (expression is not compile-time constant)");
                declared = TYPE_I64;
            } else {
                declared = init_type;
            }
            node->as.val_decl.type = declared;
        } else {
            typecheck_global_coerce(init_type, declared, init, scope);
        }

        typecheck_struct_copy(declared, init);

        if (type_is_comptime(declared) && is_comptime_constant(init, scope)) {
            if (declared == TYPE_COMPTIME_INT) {
                scope_add_comptime_int(scope, node->as.val_decl.name_start,
                                       node->as.val_decl.name_length,
                                       get_comptime_int(init, scope), node->loc);
            } else {
                scope_add_comptime_float(scope, node->as.val_decl.name_start,
                                         node->as.val_decl.name_length,
                                         get_comptime_float(init, scope), node->loc);
            }
            /* comptime globals don't need is_global since they are inlined */
        } else {
            scope_add_global(scope, node->as.val_decl.name_start,
                             node->as.val_decl.name_length, false, declared,
                             node->loc);
        }
    } else if (node->kind == AST_MUT_DECL) {
        Type init_type = typecheck_expression(node->as.mut_decl.initializer, scope, func_table);
        Type declared = node->as.mut_decl.type;
        Ast *init = node->as.mut_decl.initializer;

        /* Arena constructor: mut mem: Arena = arena(N) */
        if (declared == TYPE_ARENA && init->kind == AST_ARENA_NEW) {
            g_has_arena = true;
            scope_add_global(scope, node->as.mut_decl.name_start,
                             node->as.mut_decl.name_length, true, declared,
                             node->loc);
            return;
        }

        /* String literal with mut is an error */
        if (type_is_str(declared) && init->kind == AST_STRING_LITERAL) {
            diagnostic(node->loc.file, ERR_S054_STRING_LITERAL_MUT, node->loc.line,
                       node->loc.column,
                       "String literals cannot be mutable (use 'val')");
            scope_add_global(scope, node->as.mut_decl.name_start,
                             node->as.mut_decl.name_length, true, declared,
                             node->loc);
            return;
        }

        /* Check initializer is a constant expression (arena() handled above) */
        if (!is_global_initializer(init, scope)) {
            diagnostic(node->loc.file, ERR_S057_GLOBAL_NOT_CONSTANT, node->loc.line, node->loc.column,
                       "Global variable initializer must be a constant expression (or arena constructor)");
        }

        /* Reject slice globals */
        if (type_is_slice(declared)) {
            diagnostic(node->loc.file, ERR_S046_SLICE_LOCAL_VAR, node->loc.line, node->loc.column,
                       "Slice type not allowed as global variable");
        }

        typecheck_global_coerce(init_type, declared, init, scope);
        typecheck_struct_copy(declared, init);

        scope_add_global(scope, node->as.mut_decl.name_start,
                         node->as.mut_decl.name_length, true, declared,
                         node->loc);
    }

    /* Set is_public on the variable we just added */
    bool pub = (node->kind == AST_VAL_DECL) ? node->as.val_decl.is_public
                                             : node->as.mut_decl.is_public;
    if (pub) {
        const char *vname = (node->kind == AST_VAL_DECL)
                            ? node->as.val_decl.name_start
                            : node->as.mut_decl.name_start;
        size_t vlen = (node->kind == AST_VAL_DECL)
                      ? node->as.val_decl.name_length
                      : node->as.mut_decl.name_length;
        Variable *gv = scope_lookup_local(scope, vname, vlen);
        if (gv) gv->is_public = true;
    }
}

/* Type check a function declaration */
static void typecheck_function(Ast *func_decl, FunctionTable *func_table,
                               Scope *global_scope) {
    /* Set current module for module-aware lookups */
    g_typecheck_module_id = module_for_file(func_decl->loc.file);

    /* Set current function entry for escape analysis */
    g_current_func_entry = func_table_lookup_in_module(func_table,
        func_decl->as.func_decl.name_start,
        func_decl->as.func_decl.name_length, g_typecheck_module_id);

    /* Create scope with parameters (global scope as parent) */
    Scope *scope = scope_create(global_scope);

    /* Add parameters to scope, check for duplicates */
    for (size_t i = 0; i < func_decl->as.func_decl.param_count; i++) {
        Parameter *param = &func_decl->as.func_decl.params[i];

        /* Check for duplicate parameter names */
        if (scope_lookup_local(scope, param->name_start, param->name_length)) {
            diagnostic(func_decl->loc.file, ERR_S011_DUPLICATE_PARAM, func_decl->loc.line,
                       func_decl->loc.column, "Duplicate parameter '%.*s'",
                       (int)param->name_length, param->name_start);
        }

        /* Non-copyable parameters must use ref or mut ref */
        if (type_is_non_copyable(param->type) && !param->is_ref) {
            diagnostic(func_decl->loc.file, ERR_S044_STRUCT_BY_VALUE, func_decl->loc.line,
                       func_decl->loc.column,
                       "%s parameter '%.*s' must use 'ref' or 'mut ref'",
                       type_name(param->type),
                       (int)param->name_length, param->name_start);
        }

        /* Slice parameters must use ref or mut ref */
        if (type_is_slice(param->type) && !param->is_ref) {
            diagnostic(func_decl->loc.file, ERR_S049_SLICE_NO_REF, func_decl->loc.line,
                       func_decl->loc.column,
                       "Slice parameter '%.*s' must use 'ref' or 'mut ref'",
                       (int)param->name_length, param->name_start);
        }

        if (!scope_lookup_local(scope, param->name_start, param->name_length)) {
            /* ref → immutable, mut ref → mutable, normal → immutable */
            bool is_mutable = param->is_mut_ref;
            Variable *v = scope_add(scope, param->name_start, param->name_length,
                                    is_mutable, param->type, func_decl->loc);
            if (v) v->is_ref = param->is_ref;
        }
    }

    /* Type check body statements with the function's return type */
    Type return_type = func_decl->as.func_decl.return_type;
    Ast *body = func_decl->as.func_decl.body;
    for (size_t i = 0; i < body->as.block.count; i++) {
        typecheck_statement(body->as.block.statements[i], &scope, return_type, func_table);
    }
    /* Typecheck trailing value expression if present (e.g. bare call at end of body) */
    if (body->as.block.value_expr) {
        typecheck_expression(body->as.block.value_expr, scope, func_table);
    }

    scope_destroy(scope);
    g_current_func_entry = NULL;
}

/* Forward declaration: codegen needs access to the function table */
static FunctionTable *g_codegen_func_table = NULL;

static void typecheck_program(Ast *program) {
    FunctionTable *func_table = func_table_create();
    bool has_main = false;

    /* Create global scope for top-level variables */
    Scope *global_scope = scope_create(NULL);

    /* Inject predefined constants */
    scope_inject_platform_constants(global_scope);

    /* Validate value/struct type declarations */
    for (size_t i = 0; i < program->as.program.count; i++) {
        Ast *node = program->as.program.statements[i];
        if (node->kind == AST_VALUE_DECL) {
            Parameter *fields = node->as.value_decl.fields;
            size_t field_count = node->as.value_decl.field_count;
            const char *kind_label = node->as.value_decl.is_struct
                                     ? "struct" : "value";

            for (size_t f = 0; f < field_count; f++) {
                /* Check for duplicate field names */
                for (size_t g = 0; g < f; g++) {
                    if (fields[f].name_length == fields[g].name_length &&
                        memcmp(fields[f].name_start, fields[g].name_start,
                               fields[f].name_length) == 0) {
                        diagnostic(node->loc.file, ERR_S022_DUPLICATE_FIELD, node->loc.line,
                                   node->loc.column,
                                   "Duplicate field '%.*s' in %s type '%.*s'",
                                   (int)fields[f].name_length, fields[f].name_start,
                                   kind_label,
                                   (int)node->as.value_decl.name_length,
                                   node->as.value_decl.name_start);
                        break;
                    }
                }
                /* Reject ref on fields */
                if (fields[f].is_ref || fields[f].is_mut_ref) {
                    diagnostic(node->loc.file, ERR_S037_REF_ON_FIELD, node->loc.line,
                               node->loc.column,
                               "'ref' not allowed on %s type field '%.*s'",
                               kind_label,
                               (int)fields[f].name_length, fields[f].name_start);
                }
                /* Check field type is valid (not void, not unknown) */
                if (fields[f].type == TYPE_VOID || fields[f].type == TYPE_UNKNOWN) {
                    diagnostic(node->loc.file, ERR_S023_UNKNOWN_FIELD_TYPE, node->loc.line,
                               node->loc.column,
                               "Invalid type for field '%.*s' in %s type '%.*s'",
                               (int)fields[f].name_length, fields[f].name_start,
                               kind_label,
                               (int)node->as.value_decl.name_length,
                               node->as.value_decl.name_start);
                }
                /* Reject non-copyable types as fields */
                if (type_is_non_copyable(fields[f].type)) {
                    diagnostic(node->loc.file, ERR_S045_EMBED_STRUCT, node->loc.line,
                               node->loc.column,
                               "Cannot embed type '%s' as field in %s type '%.*s' (not copyable)",
                               type_name(fields[f].type), kind_label,
                               (int)node->as.value_decl.name_length,
                               node->as.value_decl.name_start);
                }
                /* Reject slice types as fields in value types (allowed in structs) */
                if (type_is_slice(fields[f].type) && !node->as.value_decl.is_struct) {
                    diagnostic(node->loc.file, ERR_S047_SLICE_AS_FIELD, node->loc.line,
                               node->loc.column,
                               "Slice type not allowed as field '%.*s' in %s type '%.*s'",
                               (int)fields[f].name_length, fields[f].name_start,
                               kind_label,
                               (int)node->as.value_decl.name_length,
                               node->as.value_decl.name_start);
                }
            }
        }
    }

    /* Validate table field types (original fields must be value-compatible) */
    for (size_t i = 0; i < g_table_decl_count; i++) {
        TableDeclEntry *te = &g_table_decls[i];
        for (size_t f = 0; f < te->field_count; f++) {
            Type ft = te->fields[f].type;
            if (type_is_slice(ft) || type_is_non_copyable(ft)) {
                diagnostic(g_source_file, ERR_S059_TABLE_FIELD_TYPE, 0, 0,
                           "Table field '%.*s' in table '%.*s' must be a value type (no slices, structs, or Arena)",
                           (int)te->fields[f].name_length, te->fields[f].name_start,
                           (int)te->name_length, te->name_start);
            }
        }
    }

    /* Validate tagged enum payload types (must be value-compatible) */
    for (size_t i = 0; i < g_enum_count; i++) {
        EnumTypeEntry *et = &g_enum_table[i];
        for (size_t v = 0; v < et->variant_count; v++) {
            Type pt = et->variants[v].payload_type;
            if (pt != TYPE_VOID && type_is_non_copyable(pt)) {
                diagnostic(et->loc.file, ERR_S082_PAYLOAD_NOT_VALUE,
                           et->loc.line, et->loc.column,
                           "Variant '%.*s' in enum '%.*s' has payload type '%s'"
                           " which is not value-compatible"
                           " (no structs or Arena)",
                           (int)et->variants[v].name_length,
                           et->variants[v].name_start,
                           (int)et->name_length, et->name_start,
                           type_name(pt));
            }
        }
    }

    /* First pass: register all functions and native declarations */
    for (size_t i = 0; i < program->as.program.count; i++) {
        Ast *node = program->as.program.statements[i];
        if (node->kind == AST_NATIVE_DECL) {
            const char *nname = node->as.native_decl.name_start;
            size_t nlen = node->as.native_decl.name_length;
            size_t mod = module_for_file(node->loc.file);

            /* Validate against known native names */
            if (!is_known_native(nname, nlen)) {
                diagnostic(node->loc.file, ERR_S086_UNKNOWN_NATIVE,
                           node->loc.line, node->loc.column,
                           "Unknown native function '%.*s'",
                           (int)nlen, nname);
                continue;
            }
            /* Check for duplicate native in same module */
            if (native_table_lookup(nname, nlen, mod)) {
                diagnostic(node->loc.file, ERR_S008_DUPLICATE_FUNCTION,
                           node->loc.line, node->loc.column,
                           "Duplicate native declaration '%.*s'",
                           (int)nlen, nname);
                continue;
            }
            if (g_native_count >= MAX_NATIVES) {
                panic(ERR_I002_INTERNAL_ERROR, "too many native declarations");
            }
            NativeEntry entry = {
                nname,
                nlen,
                mod,
                node->loc,
                node->as.native_decl.params,
                node->as.native_decl.param_count,
                node->as.native_decl.return_type
            };
            validate_native_decl_signature(&entry);
            g_native_table[g_native_count++] = entry;
        } else if (node->kind == AST_FUNC_DECL) {
            func_table_add(func_table, node);

            /* Check if this is main */
            if (node->as.func_decl.name_length == 4 &&
                memcmp(node->as.func_decl.name_start, "main", 4) == 0) {
                has_main = true;

                /* Verify main signature: no parameters, void return */
                if (node->as.func_decl.param_count != 0) {
                    diagnostic(node->loc.file, ERR_S010_INVALID_MAIN_SIG, node->loc.line,
                               node->loc.column, "main must have no parameters");
                }
                if (node->as.func_decl.return_type != TYPE_VOID) {
                    diagnostic(node->loc.file, ERR_S010_INVALID_MAIN_SIG, node->loc.line,
                               node->loc.column, "main must return void");
                }
            }
        }
    }

    /* Check for main function */
    if (g_require_main && !has_main) {
        diagnostic(g_source_file, ERR_S009_MISSING_MAIN, 1, 1, "No 'main' function defined");
    }

    /* Global variable pass: typecheck top-level val/mut declarations */
    for (size_t i = 0; i < program->as.program.count; i++) {
        Ast *node = program->as.program.statements[i];
        if (node->kind == AST_VAL_DECL || node->kind == AST_MUT_DECL) {
            g_typecheck_module_id = module_for_file(node->loc.file);
            typecheck_global_decl(node, global_scope, func_table);
        }
    }
    g_typecheck_module_id = 0;

    /* Second pass: type check each function */
    for (size_t i = 0; i < program->as.program.count; i++) {
        Ast *node = program->as.program.statements[i];
        if (node->kind == AST_FUNC_DECL) {
            typecheck_function(node, func_table, global_scope);
        }
    }

    /* Third pass: propagate returns_arena_slices transitively, then check deferred */
    {
        /* Fixpoint propagation */
        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t i = 0; i < g_arena_dep_count; i++) {
                ArenaDependency *d = &g_arena_deps[i];
                FunctionEntry *callee = func_table_lookup(func_table,
                    d->callee_name_start, d->callee_name_length);
                if (!callee || !callee->returns_arena_slices) continue;
                FunctionEntry *func = func_table_lookup(func_table,
                    d->func_name_start, d->func_name_length);
                if (func && !func->returns_arena_slices) {
                    func->returns_arena_slices = true;
                    changed = true;
                }
            }
        }

        /* Process deferred arena escape checks */
        for (size_t i = 0; i < g_deferred_arena_count; i++) {
            DeferredArenaCheck *c = &g_deferred_arena_checks[i];
            FunctionEntry *callee = func_table_lookup(func_table,
                c->callee_name_start, c->callee_name_length);
            if (callee && callee->returns_arena_slices) {
                diagnostic(c->loc.file, ERR_S053_SLICE_ESCAPES_ARENA,
                           c->loc.line, c->loc.column,
                           "Return value of '%.*s' contains slices that escape local arena '%.*s'",
                           (int)c->callee_name_length, c->callee_name_start,
                           (int)c->arena_name_length, c->arena_name_start);
            }
        }

        /* Cleanup */
        free(g_deferred_arena_checks);
        g_deferred_arena_checks = NULL;
        g_deferred_arena_count = 0;
        g_deferred_arena_capacity = 0;
        free(g_arena_deps);
        g_arena_deps = NULL;
        g_arena_dep_count = 0;
        g_arena_dep_capacity = 0;
    }

    /* Keep func_table alive for codegen (slice call-site needs param types) */
    g_codegen_func_table = func_table;
}

/* Conservative scan used by --strip-asserts before generated code drops assertions. */
static bool ast_may_have_side_effects(Ast *node);

static bool ast_list_may_have_side_effects(Ast **items, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (ast_may_have_side_effects(items[i])) {
            return true;
        }
    }
    return false;
}

static bool ast_block_may_have_side_effects(Ast *block) {
    if (!block || block->kind != AST_BLOCK) {
        return false;
    }
    if (ast_list_may_have_side_effects(block->as.block.statements,
                                       block->as.block.count)) {
        return true;
    }
    return ast_may_have_side_effects(block->as.block.value_expr);
}

static bool ast_may_have_side_effects(Ast *node) {
    if (!node) {
        return false;
    }

    switch (node->kind) {
        case AST_NUMBER:
        case AST_FLOAT:
        case AST_BOOLEAN:
        case AST_STRING_LITERAL:
        case AST_IDENTIFIER:
        case AST_NATIVE_DECL:
        case AST_FUNC_DECL:
        case AST_VALUE_DECL:
        case AST_ENUM_DECL:
        case AST_PROGRAM:
            return false;

        case AST_FUNC_CALL:
        case AST_ARENA_NEW:
        case AST_ARENA_ALLOC:
        case AST_ARENA_RESET:
        case AST_TABLE_ALLOC:
        case AST_TABLE_INSERT:
        case AST_FD_WRITE:
        case AST_FD_READ:
        case AST_FD_OPEN:
        case AST_FD_CLOSE:
        case AST_FD_SEEK:
        case AST_EXIT:
        case AST_MEM_COPY:
        case AST_ARGS:
        case AST_SHELL_RUN:
        case AST_EXE_DIR:
        case AST_PID:
        case AST_TIME_NS:
        case AST_ASSERT:
        case AST_ASSIGNMENT:
        case AST_COMPOUND_ASSIGNMENT:
        case AST_RETURN:
        case AST_WHILE:
        case AST_FOR:
        case AST_BREAK:
        case AST_CONTINUE:
            return true;

        case AST_BINARY:
            return ast_may_have_side_effects(node->as.binary.left) ||
                   ast_may_have_side_effects(node->as.binary.right);
        case AST_UNARY:
            return ast_may_have_side_effects(node->as.unary.operand);
        case AST_VAL_DECL:
            return ast_may_have_side_effects(node->as.val_decl.initializer);
        case AST_MUT_DECL:
            return ast_may_have_side_effects(node->as.mut_decl.initializer);
        case AST_IF:
            return ast_may_have_side_effects(node->as.if_stmt.condition) ||
                   ast_block_may_have_side_effects(node->as.if_stmt.then_block) ||
                   ast_block_may_have_side_effects(node->as.if_stmt.else_block);
        case AST_BLOCK:
            return ast_block_may_have_side_effects(node);
        case AST_VALUE_CONSTRUCTOR:
            for (size_t i = 0; i < node->as.value_constructor.field_count; i++) {
                if (ast_may_have_side_effects(node->as.value_constructor.fields[i].value)) {
                    return true;
                }
            }
            return false;
        case AST_FIELD_ACCESS:
            return ast_may_have_side_effects(node->as.field_access.object);
        case AST_ARRAY_LITERAL:
            return ast_list_may_have_side_effects(node->as.array_literal.elements,
                                                  node->as.array_literal.element_count);
        case AST_INDEX_ACCESS:
            return ast_may_have_side_effects(node->as.index_access.object) ||
                   ast_may_have_side_effects(node->as.index_access.index);
        case AST_SLICE_ACCESS:
            return ast_may_have_side_effects(node->as.slice_access.object) ||
                   ast_may_have_side_effects(node->as.slice_access.start) ||
                   ast_may_have_side_effects(node->as.slice_access.end);
        case AST_TABLE_LEN:
            return ast_may_have_side_effects(node->as.table_len.table);
        case AST_TABLE_GET:
            return ast_may_have_side_effects(node->as.table_get.table) ||
                   ast_may_have_side_effects(node->as.table_get.index);
        case AST_TYPE_CAST:
            return ast_may_have_side_effects(node->as.type_cast.operand);
        case AST_ENUM_VARIANT:
            return ast_may_have_side_effects(node->as.enum_variant.payload);
        case AST_MATCH:
            if (ast_may_have_side_effects(node->as.match_expr.scrutinee)) {
                return true;
            }
            return ast_list_may_have_side_effects(node->as.match_expr.arms,
                                                  node->as.match_expr.arm_count);
        case AST_MATCH_ARM:
            return ast_block_may_have_side_effects(node->as.match_arm.body);
    }

    return true;
}

static void validate_assert_stripping_node(Ast *node) {
    if (!node) {
        return;
    }

    if (node->kind == AST_ASSERT &&
        ast_may_have_side_effects(node->as.assert_stmt.condition)) {
        diagnostic(node->loc.file, ERR_S091_ASSERT_STRIP_EFFECT,
                   node->loc.line, node->loc.column,
                   "Cannot strip assert with side effects");
    }

    switch (node->kind) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->as.program.count; i++) {
                validate_assert_stripping_node(node->as.program.statements[i]);
            }
            break;
        case AST_FUNC_DECL:
            validate_assert_stripping_node(node->as.func_decl.body);
            break;
        case AST_FUNC_CALL:
            for (size_t i = 0; i < node->as.func_call.arg_count; i++) {
                validate_assert_stripping_node(node->as.func_call.arguments[i]);
            }
            break;
        case AST_BINARY:
            validate_assert_stripping_node(node->as.binary.left);
            validate_assert_stripping_node(node->as.binary.right);
            break;
        case AST_UNARY:
            validate_assert_stripping_node(node->as.unary.operand);
            break;
        case AST_VAL_DECL:
            validate_assert_stripping_node(node->as.val_decl.initializer);
            break;
        case AST_MUT_DECL:
            validate_assert_stripping_node(node->as.mut_decl.initializer);
            break;
        case AST_RETURN:
            validate_assert_stripping_node(node->as.return_stmt.value);
            break;
        case AST_ASSIGNMENT:
            validate_assert_stripping_node(node->as.assignment.target);
            validate_assert_stripping_node(node->as.assignment.value);
            break;
        case AST_COMPOUND_ASSIGNMENT:
            validate_assert_stripping_node(node->as.compound_assignment.target);
            validate_assert_stripping_node(node->as.compound_assignment.value);
            break;
        case AST_BLOCK:
            for (size_t i = 0; i < node->as.block.count; i++) {
                validate_assert_stripping_node(node->as.block.statements[i]);
            }
            validate_assert_stripping_node(node->as.block.value_expr);
            break;
        case AST_IF:
            validate_assert_stripping_node(node->as.if_stmt.condition);
            validate_assert_stripping_node(node->as.if_stmt.then_block);
            validate_assert_stripping_node(node->as.if_stmt.else_block);
            break;
        case AST_WHILE:
            validate_assert_stripping_node(node->as.while_stmt.condition);
            validate_assert_stripping_node(node->as.while_stmt.body);
            break;
        case AST_FOR:
            validate_assert_stripping_node(node->as.for_stmt.start);
            validate_assert_stripping_node(node->as.for_stmt.end);
            validate_assert_stripping_node(node->as.for_stmt.body);
            break;
        case AST_VALUE_CONSTRUCTOR:
            for (size_t i = 0; i < node->as.value_constructor.field_count; i++) {
                validate_assert_stripping_node(node->as.value_constructor.fields[i].value);
            }
            break;
        case AST_FIELD_ACCESS:
            validate_assert_stripping_node(node->as.field_access.object);
            break;
        case AST_ARRAY_LITERAL:
            for (size_t i = 0; i < node->as.array_literal.element_count; i++) {
                validate_assert_stripping_node(node->as.array_literal.elements[i]);
            }
            break;
        case AST_INDEX_ACCESS:
            validate_assert_stripping_node(node->as.index_access.object);
            validate_assert_stripping_node(node->as.index_access.index);
            break;
        case AST_SLICE_ACCESS:
            validate_assert_stripping_node(node->as.slice_access.object);
            validate_assert_stripping_node(node->as.slice_access.start);
            validate_assert_stripping_node(node->as.slice_access.end);
            break;
        case AST_ARENA_NEW:
            validate_assert_stripping_node(node->as.arena_new.capacity);
            break;
        case AST_ARENA_ALLOC:
            validate_assert_stripping_node(node->as.arena_alloc.arena);
            validate_assert_stripping_node(node->as.arena_alloc.count);
            break;
        case AST_ARENA_RESET:
            validate_assert_stripping_node(node->as.arena_reset.arena);
            break;
        case AST_TABLE_ALLOC:
            validate_assert_stripping_node(node->as.table_alloc.arena);
            validate_assert_stripping_node(node->as.table_alloc.count);
            break;
        case AST_TABLE_LEN:
            validate_assert_stripping_node(node->as.table_len.table);
            break;
        case AST_TABLE_GET:
            validate_assert_stripping_node(node->as.table_get.table);
            validate_assert_stripping_node(node->as.table_get.index);
            break;
        case AST_TABLE_INSERT:
            validate_assert_stripping_node(node->as.table_insert.table);
            validate_assert_stripping_node(node->as.table_insert.row);
            break;
        case AST_TYPE_CAST:
            validate_assert_stripping_node(node->as.type_cast.operand);
            break;
        case AST_FD_WRITE:
            validate_assert_stripping_node(node->as.fd_write.fd);
            validate_assert_stripping_node(node->as.fd_write.data);
            break;
        case AST_FD_READ:
            validate_assert_stripping_node(node->as.fd_read.fd);
            validate_assert_stripping_node(node->as.fd_read.buf);
            break;
        case AST_FD_OPEN:
            validate_assert_stripping_node(node->as.fd_open.path);
            validate_assert_stripping_node(node->as.fd_open.flags);
            break;
        case AST_FD_CLOSE:
            validate_assert_stripping_node(node->as.fd_close.fd);
            break;
        case AST_FD_SEEK:
            validate_assert_stripping_node(node->as.fd_seek.fd);
            validate_assert_stripping_node(node->as.fd_seek.offset);
            validate_assert_stripping_node(node->as.fd_seek.whence);
            break;
        case AST_EXIT:
            validate_assert_stripping_node(node->as.exit_call.code);
            break;
        case AST_MEM_COPY:
            validate_assert_stripping_node(node->as.mem_copy.dst);
            validate_assert_stripping_node(node->as.mem_copy.src);
            break;
        case AST_ARGS:
            validate_assert_stripping_node(node->as.args.arena);
            break;
        case AST_SHELL_RUN:
            validate_assert_stripping_node(node->as.shell_run.command);
            break;
        case AST_EXE_DIR:
            validate_assert_stripping_node(node->as.exe_dir.arena);
            break;
        case AST_ENUM_VARIANT:
            validate_assert_stripping_node(node->as.enum_variant.payload);
            break;
        case AST_MATCH:
            validate_assert_stripping_node(node->as.match_expr.scrutinee);
            for (size_t i = 0; i < node->as.match_expr.arm_count; i++) {
                validate_assert_stripping_node(node->as.match_expr.arms[i]);
            }
            break;
        case AST_MATCH_ARM:
            for (size_t i = 0; i < node->as.match_arm.pattern_count; i++) {
                validate_assert_stripping_node(node->as.match_arm.patterns[i].pattern_expr);
            }
            validate_assert_stripping_node(node->as.match_arm.body);
            break;
        default:
            break;
    }
}

/* ============================ Code Generation ============================= */

/* Emit a module-prefixed name: ni_<alias>__<name> for module > 0, ni_<name> for module 0 */
static void codegen_emit_mangled_name(FILE *out, const char *name, size_t name_len,
                                       size_t module_id) {
    if (module_id > 0 && module_id != MODULE_BUILTIN && g_modules[module_id].alias) {
        fprintf(out, "ni_%.*s__%.*s",
                (int)g_modules[module_id].alias_length, g_modules[module_id].alias,
                (int)name_len, name);
    } else {
        fprintf(out, "ni_%.*s", (int)name_len, name);
    }
}

/* Return a module-prefixed name as a static string for type names */
static const char *codegen_mangled_name_str(const char *name, size_t name_len,
                                              size_t module_id) {
    static char bufs[4][128];
    static int idx = 0;
    char *buf = bufs[idx++ % 4];
    if (module_id > 0 && module_id != MODULE_BUILTIN && g_modules[module_id].alias) {
        snprintf(buf, 128, "ni_%.*s__%.*s",
                 (int)g_modules[module_id].alias_length, g_modules[module_id].alias,
                 (int)name_len, name);
    } else {
        snprintf(buf, 128, "ni_%.*s", (int)name_len, name);
    }
    /* Replace dots with underscores (for Table.Row types) */
    for (char *p = buf; *p; p++) {
        if (*p == '.') *p = '_';
    }
    return buf;
}

static const char *codegen_type_to_c(Type type);  /* forward declaration */
static int codegen_temp_counter = 0;
static int codegen_match_counter = 0;
static Scope *g_codegen_scope = NULL;  /* current codegen scope for ref lookups */
static Scope *g_global_codegen_scope = NULL;  /* global scope for codegen */

typedef struct {
    Ast *index_node;
    int temp_idx;
} CodegenTargetIndexTemp;

/* Check if a block can be emitted as a simple expression (no hoisting needed) */
static bool codegen_can_emit_inline(Ast *node) {
    switch (node->kind) {
        case AST_BLOCK:
            return node->as.block.count == 0 &&
                   node->as.block.value_expr != NULL &&
                   codegen_can_emit_inline(node->as.block.value_expr);

        case AST_IF:
            return node->as.if_stmt.else_block != NULL &&
                   codegen_can_emit_inline(node->as.if_stmt.then_block) &&
                   codegen_can_emit_inline(node->as.if_stmt.else_block);

        case AST_MATCH:
            return false;

        default:
            return true;
    }
}

static bool codegen_is_simple_block(Ast *block) {
    return codegen_can_emit_inline(block);
}

/* Check if an if expression can be emitted as a simple ternary */
static bool codegen_is_simple_if(Ast *node) {
    return codegen_can_emit_inline(node);
}

/* Resolve comptime array types to their concrete C types for codegen.
 * e.g. [comptime_int; 3] -> [i64; 3], [comptime_float; 2] -> [f64; 2] */
static Type codegen_concrete_array_type(Type type) {
    if (!type_is_array(type)) return type;
    ArrayTypeEntry *at = array_table_get(type);
    if (!at) return type;
    Type elem = at->element_type;
    if (elem == TYPE_COMPTIME_INT) elem = TYPE_I64;
    else if (elem == TYPE_COMPTIME_FLOAT) elem = TYPE_F64;
    else if (type_is_array(elem)) elem = codegen_concrete_array_type(elem);
    if (elem == at->element_type) return type;  /* no change */
    return array_table_intern(elem, at->size);
}

/* Check if an identifier variable is passed by reference in the current codegen scope */
static bool codegen_is_ref(const char *name_start, size_t name_length) {
    if (!g_codegen_scope) return false;
    Variable *v = scope_lookup(g_codegen_scope, name_start, name_length);
    return v && v->is_ref;
}

/* Keep signed-min literals warning-free by avoiding an oversized token. */
static void codegen_emit_i64_literal(FILE *out, long value) {
    if (value == LONG_MIN) {
        fprintf(out, "((-9223372036854775807L) - 1L)");
    } else {
        fprintf(out, "%ldL", value);
    }
}

/* Emit round-trip double text while keeping integral values as C float literals. */
static void codegen_emit_f64_literal(FILE *out, double value) {
    char buf[64];
    bool has_float_marker = false;

    snprintf(buf, sizeof(buf), "%.17g", value);
    for (char *p = buf; *p; p++) {
        if (*p == '.' || *p == 'e' || *p == 'E') {
            has_float_marker = true;
            break;
        }
    }

    fputs(buf, out);
    if (!has_float_marker) {
        fputs(".0", out);
    }
}

/* Emit text as a C string literal for generated diagnostics. */
static void codegen_emit_c_string_literal(FILE *out, const char *text) {
    const unsigned char *p = (const unsigned char *)(text ? text : "");

    fputc('"', out);
    for (; *p; p++) {
        switch (*p) {
            case '\\': fputs("\\\\", out); break;
            case '"': fputs("\\\"", out); break;
            case '\n': fputs("\\n", out); break;
            case '\r': fputs("\\r", out); break;
            case '\t': fputs("\\t", out); break;
            default:
                if (*p < 32 || *p == 127)
                    fprintf(out, "\\%03o", (unsigned)*p);
                else
                    fputc((int)*p, out);
                break;
        }
    }
    fputc('"', out);
}

/* Emit file/line/column arguments for generated runtime checks. */
static void codegen_emit_runtime_location(FILE *out, Ast *node) {
    codegen_emit_c_string_literal(out, node->loc.file);
    fprintf(out, ", %zuUL, %zuUL", node->loc.line, node->loc.column);
}

/* Emit a ref-aware identifier: (*ni_X) for ref params, ni_X otherwise */
static void codegen_emit_identifier(FILE *out, const char *name_start, size_t name_length) {
    /* Inline comptime values (they have no C variable) */
    if (g_codegen_scope) {
        Variable *v = scope_lookup(g_codegen_scope, name_start, name_length);
        if (v && v->is_comptime) {
            if (v->type == TYPE_COMPTIME_INT || type_is_enum(v->type)) {
                codegen_emit_i64_literal(out, v->comptime_value.int_value);
            } else {
                codegen_emit_f64_literal(out, v->comptime_value.float_value);
            }
            return;
        }
        /* Use module-aware mangling for global variables */
        if (v && v->is_global && v->module_id > 0 && v->module_id != MODULE_BUILTIN) {
            if (codegen_is_ref(name_start, name_length)) {
                fprintf(out, "(*");
                codegen_emit_mangled_name(out, name_start, name_length, v->module_id);
                fprintf(out, ")");
            } else {
                codegen_emit_mangled_name(out, name_start, name_length, v->module_id);
            }
            return;
        }
    }
    if (codegen_is_ref(name_start, name_length)) {
        fprintf(out, "(*ni_%.*s)", (int)name_length, name_start);
    } else {
        fprintf(out, "ni_%.*s", (int)name_length, name_start);
    }
}

/* Emit field access on an object, using -> for ref identifiers, . otherwise.
 * emit_object_fn is used to recursively emit the object when not a ref identifier. */
/* Check if a field access node is slice.len */
static bool codegen_is_slice_len(Ast *node) {
    return type_is_slice(node->as.field_access.object->expr_type) &&
           node->as.field_access.field_length == 3 &&
           memcmp(node->as.field_access.field_start, "len", 3) == 0;
}

static void codegen_emit_field(FILE *out, Ast *node,
                                void (*emit_object_fn)(FILE *, Ast *)) {
    Ast *obj = node->as.field_access.object;
    if (obj->kind == AST_IDENTIFIER &&
        codegen_is_ref(obj->as.identifier.start, obj->as.identifier.length)) {
        fprintf(out, "ni_%.*s->ni_%.*s",
                (int)obj->as.identifier.length, obj->as.identifier.start,
                (int)node->as.field_access.field_length,
                node->as.field_access.field_start);
    } else {
        emit_object_fn(out, obj);
        fprintf(out, ".ni_%.*s", (int)node->as.field_access.field_length,
                node->as.field_access.field_start);
    }
}

static void codegen_emit_lvalue(FILE *out, Ast *node);       /* forward declaration */
static void codegen_emit_expression(FILE *out, Ast *node);  /* forward declaration */

static int codegen_find_target_index_temp(CodegenTargetIndexTemp *temps,
                                          size_t count, Ast *node) {
    for (size_t i = 0; i < count; i++) {
        if (temps[i].index_node == node) return temps[i].temp_idx;
    }
    return -1;
}

static void codegen_emit_target_index_name(FILE *out, int temp_idx) {
    fprintf(out, "__idx_%d", temp_idx);
}

static void codegen_emit_target_path(FILE *out, Ast *node,
                                     CodegenTargetIndexTemp *temps,
                                     size_t temp_count) {
    switch (node->kind) {
        case AST_IDENTIFIER:
            codegen_emit_identifier(out, node->as.identifier.start,
                                    node->as.identifier.length);
            break;

        case AST_FIELD_ACCESS: {
            Ast *obj = node->as.field_access.object;
            if (codegen_is_slice_len(node)) {
                codegen_emit_target_path(out, obj, temps, temp_count);
                fprintf(out, ".len");
            } else if (obj->kind == AST_IDENTIFIER &&
                       codegen_is_ref(obj->as.identifier.start,
                                      obj->as.identifier.length)) {
                fprintf(out, "ni_%.*s->ni_%.*s",
                        (int)obj->as.identifier.length, obj->as.identifier.start,
                        (int)node->as.field_access.field_length,
                        node->as.field_access.field_start);
            } else {
                codegen_emit_target_path(out, obj, temps, temp_count);
                fprintf(out, ".ni_%.*s",
                        (int)node->as.field_access.field_length,
                        node->as.field_access.field_start);
            }
            break;
        }

        case AST_INDEX_ACCESS: {
            int temp_idx = codegen_find_target_index_temp(temps, temp_count,
                                                          node);
            codegen_emit_target_path(out, node->as.index_access.object,
                                     temps, temp_count);
            fprintf(out, ".data[");
            if (temp_idx >= 0) {
                codegen_emit_target_index_name(out, temp_idx);
            } else {
                codegen_emit_expression(out, node->as.index_access.index);
            }
            fprintf(out, "]");
            break;
        }

        default:
            codegen_emit_expression(out, node);
            break;
    }
}

/* Emit a C binary operator token. */
static void codegen_emit_binary_op(FILE *out, BinaryOp op) {
    switch (op) {
        case OP_ADD: fprintf(out, " + "); break;
        case OP_SUB: fprintf(out, " - "); break;
        case OP_MUL: fprintf(out, " * "); break;
        case OP_DIV: fprintf(out, " / "); break;
        case OP_MOD: fprintf(out, " %% "); break;
        case OP_EQ:  fprintf(out, " == "); break;
        case OP_NEQ: fprintf(out, " != "); break;
        case OP_LT:  fprintf(out, " < "); break;
        case OP_GT:  fprintf(out, " > "); break;
        case OP_LE:  fprintf(out, " <= "); break;
        case OP_GE:  fprintf(out, " >= "); break;
        case OP_AND:    fprintf(out, " && "); break;
        case OP_OR:     fprintf(out, " || "); break;
        case OP_BITAND: fprintf(out, " & "); break;
        case OP_BITOR:  fprintf(out, " | "); break;
        case OP_BITXOR: fprintf(out, " ^ "); break;
        case OP_SHL:    fprintf(out, " << "); break;
        case OP_SHR:    fprintf(out, " >> "); break;
    }
}

static void codegen_emit_compound_assign_op(FILE *out, BinaryOp op) {
    switch (op) {
        case OP_ADD:    fprintf(out, " += "); break;
        case OP_SUB:    fprintf(out, " -= "); break;
        case OP_MUL:    fprintf(out, " *= "); break;
        case OP_DIV:    fprintf(out, " /= "); break;
        case OP_MOD:    fprintf(out, " %%= "); break;
        case OP_BITAND: fprintf(out, " &= "); break;
        case OP_BITOR:  fprintf(out, " |= "); break;
        case OP_BITXOR: fprintf(out, " ^= "); break;
        case OP_SHL:    fprintf(out, " <<= "); break;
        case OP_SHR:    fprintf(out, " >>= "); break;
        default:
            panic(ERR_I002_INTERNAL_ERROR, "invalid compound assignment op in codegen");
    }
}

/* Emit a condition for if/while. Skips outer parentheses on binary
 * expressions since the caller provides if(...)/while(...), avoiding
 * clang's -Wparentheses-equality warning on patterns like if ((a == b)). */
static void codegen_emit_condition(FILE *out, Ast *node) {
    if (node->kind == AST_BINARY) {
        codegen_emit_expression(out, node->as.binary.left);
        codegen_emit_binary_op(out, node->as.binary.op);
        codegen_emit_expression(out, node->as.binary.right);
    } else {
        codegen_emit_expression(out, node);
    }
}

/* Helpers for AST_SLICE_ACCESS codegen to avoid repetition */
static void codegen_emit_slice_start(FILE *out, Ast *node) {
    if (node->as.slice_access.start)
        codegen_emit_expression(out, node->as.slice_access.start);
    else
        fprintf(out, "0");
}

static void codegen_emit_obj_len(FILE *out, Ast *obj, Type obj_type) {
    if (type_is_slice(obj_type)) {
        codegen_emit_expression(out, obj);
        fprintf(out, ".len");
    } else {
        ArrayTypeEntry *at = array_table_get(obj_type);
        fprintf(out, "%zu", at ? at->size : (size_t)0);
    }
}

static void codegen_emit_slice_end(FILE *out, Ast *node, Ast *obj, Type obj_type) {
    if (node->as.slice_access.end)
        codegen_emit_expression(out, node->as.slice_access.end);
    else
        codegen_emit_obj_len(out, obj, obj_type);
}

/* Open-ended slice helpers keep the base expression single-evaluated. */
static void codegen_emit_slice_tail(FILE *out, Ast *node, Ast *obj, Type obj_type) {
    fprintf(out, "ni_slice_%d_tail(", type_slice_index(obj_type));
    codegen_emit_expression(out, obj);
    fprintf(out, ", ");
    codegen_emit_slice_start(out, node);
    fprintf(out, ", ");
    codegen_emit_runtime_location(out, node);
    fprintf(out, ")");
}

/* Emit a byte buffer argument, coercing [u8; N] arrays to ni_slice_0 */
static void codegen_emit_byte_buf(FILE *out, Ast *arg) {
    if (type_is_array(arg->expr_type)) {
        ArrayTypeEntry *ae = array_table_get(arg->expr_type);
        fprintf(out, "(ni_slice_0){.data = (uint8_t *)");
        codegen_emit_lvalue(out, arg);
        fprintf(out, ".data, .len = %zuL}", ae ? ae->size : 0);
    } else {
        codegen_emit_expression(out, arg);
    }
}

static void codegen_emit_expression(FILE *out, Ast *node) {
    switch (node->kind) {
        case AST_NUMBER:
            codegen_emit_i64_literal(out, node->as.number.value);
            break;

        case AST_FLOAT:
            codegen_emit_f64_literal(out, node->as.float_lit.value);
            break;

        case AST_BOOLEAN:
            fprintf(out, "%d", node->as.boolean.value ? 1 : 0);
            break;

        case AST_ENUM_VARIANT: {
            EnumTypeEntry *ev_et = enum_table_get(node->as.enum_variant.enum_type);
            if (ev_et && ev_et->has_data) {
                /* Tagged union: compound literal */
                long idx = node->as.enum_variant.value;
                fprintf(out, "(%s){.tag = ",
                        codegen_type_to_c(node->as.enum_variant.enum_type));
                codegen_emit_i64_literal(out, idx);
                if (node->as.enum_variant.payload) {
                    fprintf(out, ", .data = {.%.*s = ",
                            (int)ev_et->variants[idx].name_length,
                            ev_et->variants[idx].name_start);
                    codegen_emit_expression(out, node->as.enum_variant.payload);
                    fprintf(out, "}");
                }
                fprintf(out, "}");
            } else {
                /* Plain enum: integer literal */
                codegen_emit_i64_literal(out, node->as.enum_variant.value);
            }
            break;
        }

        case AST_IDENTIFIER:
            codegen_emit_identifier(out, node->as.identifier.start,
                                    node->as.identifier.length);
            break;

        case AST_BINARY: {
            fprintf(out, "(");
            codegen_emit_expression(out, node->as.binary.left);
            codegen_emit_binary_op(out, node->as.binary.op);
            codegen_emit_expression(out, node->as.binary.right);
            fprintf(out, ")");
            break;
        }

        case AST_UNARY: {
            fprintf(out, "(");
            switch (node->as.unary.op) {
                case OP_NEG:    fprintf(out, "-"); break;
                case OP_NOT:    fprintf(out, "!"); break;
                case OP_BITNOT: fprintf(out, "~"); break;
            }
            codegen_emit_expression(out, node->as.unary.operand);
            fprintf(out, ")");
            break;
        }

        case AST_FUNC_CALL: {
            /* Look up function entry for module_id and param type info */
            FunctionEntry *fentry = NULL;
            if (g_codegen_func_table) {
                int call_mod = node->as.func_call.module_id;
                if (call_mod >= 0) {
                    fentry = func_table_lookup_in_module(g_codegen_func_table,
                                node->as.func_call.name_start,
                                node->as.func_call.name_length, (size_t)call_mod);
                } else {
                    fentry = func_table_lookup_in_module(g_codegen_func_table,
                                node->as.func_call.name_start,
                                node->as.func_call.name_length,
                                module_for_file(node->loc.file));
                }
            }
            size_t func_mod = fentry ? fentry->module_id
                : (node->as.func_call.module_id >= 0
                   ? (size_t)node->as.func_call.module_id : 0);
            codegen_emit_mangled_name(out, node->as.func_call.name_start,
                                      node->as.func_call.name_length, func_mod);
            fprintf(out, "(");
            for (size_t i = 0; i < node->as.func_call.arg_count; i++) {
                if (i > 0) fprintf(out, ", ");
                Ast *arg = node->as.func_call.arguments[i];
                bool is_ref = node->as.func_call.arg_is_ref &&
                              node->as.func_call.arg_is_ref[i];
                Type param_type = (fentry && i < fentry->param_count)
                                  ? fentry->param_types[i] : TYPE_UNKNOWN;

                if (is_ref && type_is_slice(param_type)) {
                    Type arg_type = arg->expr_type;
                    if (type_is_array(arg_type)) {
                        /* Array→slice coercion: create fat pointer */
                        ArrayTypeEntry *ae = array_table_get(arg_type);
                        Type slice_elem = type_element_type(param_type);
                        /* If arg is an index access, emit bounds check inline */
                        if (arg->kind == AST_INDEX_ACCESS) {
                            Ast *idx_obj = arg->as.index_access.object;
                            Type idx_obj_type = idx_obj->expr_type;
                            fprintf(out, "(NI_BOUNDS_CHECK(");
                            codegen_emit_expression(out, arg->as.index_access.index);
                            if (type_is_slice(idx_obj_type)) {
                                fprintf(out, ", ");
                                codegen_emit_expression(out, idx_obj);
                                fprintf(out, ".len");
                            } else {
                                ArrayTypeEntry *oae = array_table_get(idx_obj_type);
                                fprintf(out, ", %zu", oae ? oae->size : 0);
                            }
                            fprintf(out, ", \"%s\", %zuUL, %zuUL), ",
                                    arg->loc.file, arg->loc.line, arg->loc.column);
                        }
                        fprintf(out, "(%s){.data = (%s *)",
                                codegen_type_to_c(param_type),
                                codegen_type_to_c(slice_elem));
                        codegen_emit_lvalue(out, arg);
                        fprintf(out, ".data, .len = %zuL}", ae ? ae->size : 0);
                        if (arg->kind == AST_INDEX_ACCESS) {
                            fprintf(out, ")");
                        }
                    } else {
                        /* Slice→slice passthrough: just pass through */
                        codegen_emit_expression(out, arg);
                    }
                } else if (is_ref) {
                    fprintf(out, "&");
                    codegen_emit_lvalue(out, arg);
                } else {
                    codegen_emit_expression(out, arg);
                }
            }
            fprintf(out, ")");
            break;
        }

        case AST_BLOCK:
            /* Only simple blocks (no statements) can be emitted inline */
            if (codegen_is_simple_block(node)) {
                codegen_emit_expression(out, node->as.block.value_expr);
            } else {
                panic(ERR_I002_INTERNAL_ERROR, "complex block in inline expression context");
            }
            break;

        case AST_IF:
            /* Only simple if (both branches are simple blocks) can be emitted inline */
            if (codegen_is_simple_if(node)) {
                fprintf(out, "(");
                codegen_emit_condition(out, node->as.if_stmt.condition);
                fprintf(out, " ? ");
                codegen_emit_expression(out, node->as.if_stmt.then_block->as.block.value_expr);
                fprintf(out, " : ");
                codegen_emit_expression(out, node->as.if_stmt.else_block->as.block.value_expr);
                fprintf(out, ")");
            } else {
                panic(ERR_I002_INTERNAL_ERROR, "complex if in inline expression context");
            }
            break;

        case AST_VALUE_CONSTRUCTOR: {
            fprintf(out, "(%s){", codegen_type_to_c(node->expr_type));
            for (size_t i = 0; i < node->as.value_constructor.field_count; i++) {
                if (i > 0) fprintf(out, ", ");
                FieldInit *fi = &node->as.value_constructor.fields[i];
                fprintf(out, ".ni_%.*s = ", (int)fi->name_length, fi->name_start);
                codegen_emit_expression(out, fi->value);
            }
            fprintf(out, "}");
            break;
        }

        case AST_FIELD_ACCESS:
            if (codegen_is_slice_len(node)) {
                codegen_emit_expression(out, node->as.field_access.object);
                fprintf(out, ".len");
            } else {
                codegen_emit_field(out, node, codegen_emit_expression);
            }
            break;

        case AST_ARRAY_LITERAL: {
            Type concrete = codegen_concrete_array_type(node->expr_type);
            fprintf(out, "(%s){{", codegen_type_to_c(concrete));
            for (size_t i = 0; i < node->as.array_literal.element_count; i++) {
                if (i > 0) fprintf(out, ", ");
                codegen_emit_expression(out, node->as.array_literal.elements[i]);
            }
            fprintf(out, "}}");
            break;
        }

        case AST_INDEX_ACCESS: {
            /* Emit bounds-checked index access as comma expression:
             * (NI_BOUNDS_CHECK(idx, size, file, line, col), obj.data[idx]) */
            Ast *obj = node->as.index_access.object;
            Type obj_type = obj->expr_type;

            if (type_is_slice(obj_type)) {
                /* Slice: runtime bounds check using .len */
                fprintf(out, "(NI_BOUNDS_CHECK(");
                codegen_emit_expression(out, node->as.index_access.index);
                fprintf(out, ", ");
                codegen_emit_expression(out, obj);
                fprintf(out, ".len, ");
                codegen_emit_runtime_location(out, node);
                fprintf(out, "), ");
                codegen_emit_expression(out, obj);
                fprintf(out, ".data[");
                codegen_emit_expression(out, node->as.index_access.index);
                fprintf(out, "])");
            } else {
                ArrayTypeEntry *at = array_table_get(obj_type);
                if (at) {
                    fprintf(out, "(NI_BOUNDS_CHECK(");
                    codegen_emit_expression(out, node->as.index_access.index);
                    fprintf(out, ", %zu, ", at->size);
                    codegen_emit_runtime_location(out, node);
                    fprintf(out, "), ");
                }
                codegen_emit_lvalue(out, obj);
                fprintf(out, ".data[");
                codegen_emit_expression(out, node->as.index_access.index);
                fprintf(out, "]");
                if (at) fprintf(out, ")");
            }
            break;
        }

        case AST_SLICE_ACCESS: {
            /* Emit bounds-checked sub-slice as comma expression:
             * (NI_SLICE_BOUNDS_CHECK(start, end, len, file, line, col),
             *  (ni_slice_X){.data = obj.data + start, .len = end - start}) */
            Ast *obj = node->as.slice_access.object;
            Type obj_type = obj->expr_type;

            if (!node->as.slice_access.end && type_is_slice(obj_type)) {
                codegen_emit_slice_tail(out, node, obj, obj_type);
                break;
            }

            fprintf(out, "(NI_SLICE_BOUNDS_CHECK(");
            codegen_emit_slice_start(out, node);
            fprintf(out, ", ");
            codegen_emit_slice_end(out, node, obj, obj_type);
            fprintf(out, ", ");
            codegen_emit_obj_len(out, obj, obj_type);
            fprintf(out, ", ");
            codegen_emit_runtime_location(out, node);
            fprintf(out, "), ");
            /* result slice literal */
            fprintf(out, "(%s){.data = (%s *)",
                    codegen_type_to_c(node->expr_type),
                    codegen_type_to_c(type_element_type(obj_type)));
            if (type_is_slice(obj_type))
                codegen_emit_expression(out, obj);
            else
                codegen_emit_lvalue(out, obj);
            fprintf(out, ".data + ");
            codegen_emit_slice_start(out, node);
            fprintf(out, ", .len = ");
            codegen_emit_slice_end(out, node, obj, obj_type);
            fprintf(out, " - ");
            codegen_emit_slice_start(out, node);
            fprintf(out, "})");
            break;
        }

        case AST_STRING_LITERAL:
            fprintf(out, "(%s){.data = (uint8_t *)\"%.*s\", .len = %zuL}",
                    codegen_type_to_c(node->expr_type),
                    (int)node->as.string_literal.raw_length,
                    node->as.string_literal.start,
                    node->as.string_literal.byte_length);
            break;

        case AST_ARENA_NEW:
            fprintf(out, "ni_arena_new(");
            codegen_emit_expression(out, node->as.arena_new.capacity);
            fprintf(out, ")");
            break;

        case AST_ARENA_ALLOC: {
            Type slice_type = node->expr_type;
            Type elem_type = type_element_type(slice_type);
            fprintf(out, "(%s){.data = (%s *)ni_arena_alloc(&",
                    codegen_type_to_c(slice_type), codegen_type_to_c(elem_type));
            codegen_emit_lvalue(out, node->as.arena_alloc.arena);
            fprintf(out, ", ");
            codegen_emit_expression(out, node->as.arena_alloc.count);
            fprintf(out, ", sizeof(%s), NI_ALIGNOF(%s)), .len = ",
                    codegen_type_to_c(elem_type), codegen_type_to_c(elem_type));
            codegen_emit_expression(out, node->as.arena_alloc.count);
            fprintf(out, "}");
            break;
        }

        case AST_ARENA_RESET:
            fprintf(out, "ni_arena_reset(&");
            codegen_emit_lvalue(out, node->as.arena_reset.arena);
            fprintf(out, ")");
            break;

        case AST_TABLE_ALLOC: {
            TableDeclEntry *te = table_decl_for_type(node->expr_type);
            if (!te) { fprintf(out, "0 /* error */"); break; }
            fprintf(out, "%s__alloc(&",
                    codegen_mangled_name_str(te->name_start, te->name_length,
                                             te->module_id));
            codegen_emit_lvalue(out, node->as.table_alloc.arena);
            fprintf(out, ", ");
            codegen_emit_expression(out, node->as.table_alloc.count);
            fprintf(out, ")");
            break;
        }

        case AST_TABLE_LEN:
            codegen_emit_lvalue(out, node->as.table_len.table);
            fprintf(out, ".ni__len");
            break;

        case AST_TABLE_GET: {
            Type table_type = node->as.table_get.table->expr_type;
            TableDeclEntry *te = table_decl_for_type(table_type);
            if (!te) { fprintf(out, "0 /* error */"); break; }
            fprintf(out, "%s__get(&",
                    codegen_mangled_name_str(te->name_start, te->name_length,
                                             te->module_id));
            codegen_emit_lvalue(out, node->as.table_get.table);
            fprintf(out, ", ");
            codegen_emit_expression(out, node->as.table_get.index);
            fprintf(out, ", ");
            codegen_emit_runtime_location(out, node);
            fprintf(out, ")");
            break;
        }

        case AST_TABLE_INSERT: {
            Type table_type = node->as.table_insert.table->expr_type;
            TableDeclEntry *te = table_decl_for_type(table_type);
            if (!te) { fprintf(out, "0 /* error */"); break; }
            fprintf(out, "%s__insert(&",
                    codegen_mangled_name_str(te->name_start, te->name_length,
                                             te->module_id));
            codegen_emit_lvalue(out, node->as.table_insert.table);
            fprintf(out, ", ");
            codegen_emit_expression(out, node->as.table_insert.row);
            fprintf(out, ")");
            break;
        }

        case AST_TYPE_CAST: {
            Type from = node->as.type_cast.operand->expr_type;
            Type to = node->as.type_cast.target_type;
            bool from_is_comptime = type_is_comptime(from);
            bool from_is_f64 = (from == TYPE_F64 || from == TYPE_COMPTIME_FLOAT);

            /* Enum → numeric: plain C cast (enum is backed by int64_t) */
            if (type_is_enum(from) && type_is_numeric(to)) {
                fprintf(out, "(%s)(", codegen_type_to_c(to));
                codegen_emit_expression(out, node->as.type_cast.operand);
                fprintf(out, ")");
                break;
            }

            /* Identity or comptime → target: just emit the operand (possibly with a plain cast) */
            if (from == to || (from_is_comptime && !from_is_f64)) {
                /* comptime_int → integer target: value already range-checked at compile time */
                if (to == TYPE_F64) {
                    fprintf(out, "(double)(");
                    codegen_emit_expression(out, node->as.type_cast.operand);
                    fprintf(out, ")");
                } else {
                    codegen_emit_expression(out, node->as.type_cast.operand);
                }
                break;
            }

            /* comptime_float → f64: identity */
            if (from == TYPE_COMPTIME_FLOAT && to == TYPE_F64) {
                codegen_emit_expression(out, node->as.type_cast.operand);
                break;
            }

            /* comptime_float → integer: compile-time truncation, emit as integer literal */
            if (from_is_comptime && from_is_f64 && type_is_integer(to)) {
                Ast *op = node->as.type_cast.operand;
                /* Safe: already range-checked in typecheck */
                fprintf(out, "(%s)(",  codegen_type_to_c(to));
                codegen_emit_expression(out, op);
                fprintf(out, ")");
                break;
            }

            /* f64 → integer: use checked cast functions */
            if (from_is_f64 && type_is_integer(to)) {
                const char *fn = NULL;
                switch (to) {
                    case TYPE_I64: fn = "ni_cast_i64_f64"; break;
                    case TYPE_I32: fn = "ni_cast_i32_f64"; break;
                    case TYPE_U8:  fn = "ni_cast_u8_f64"; break;
                    case TYPE_U32: fn = "ni_cast_u32_f64"; break;
                    default: break;
                }
                if (fn) {
                    fprintf(out, "%s(", fn);
                    codegen_emit_expression(out, node->as.type_cast.operand);
                    fprintf(out, ", \"%s\", ", type_name(from));
                    codegen_emit_runtime_location(out, node);
                    fprintf(out, ")");
                }
                break;
            }

            /* integer → f64: always safe, plain C cast */
            if (to == TYPE_F64) {
                fprintf(out, "(double)(");
                codegen_emit_expression(out, node->as.type_cast.operand);
                fprintf(out, ")");
                break;
            }

            /* integer → integer: check if narrowing/sign-change needed */
            bool needs_check = false;
            switch (to) {
                case TYPE_U8:
                    needs_check = (from != TYPE_U8);
                    break;
                case TYPE_I32:
                    needs_check = (from == TYPE_I64 || from == TYPE_U32);
                    break;
                case TYPE_U32:
                    needs_check = (from == TYPE_I64 || from == TYPE_I32);
                    break;
                case TYPE_I64:
                    /* u8 → i64, i32 → i64: safe; u32 → i64: safe (fits) */
                    needs_check = false;
                    break;
                default:
                    break;
            }

            if (needs_check) {
                const char *fn = NULL;
                switch (to) {
                    case TYPE_U8:  fn = "ni_cast_u8"; break;
                    case TYPE_I32: fn = "ni_cast_i32"; break;
                    case TYPE_U32: fn = "ni_cast_u32"; break;
                    default: break;
                }
                if (fn) {
                    fprintf(out, "%s(", fn);
                    codegen_emit_expression(out, node->as.type_cast.operand);
                    fprintf(out, ", \"%s\", ", type_name(from));
                    codegen_emit_runtime_location(out, node);
                    fprintf(out, ")");
                }
            } else {
                /* Widening/safe cast: plain C cast */
                fprintf(out, "(%s)(", codegen_type_to_c(to));
                codegen_emit_expression(out, node->as.type_cast.operand);
                fprintf(out, ")");
            }
            break;
        }

        case AST_FD_WRITE:
            fprintf(out, "ni_fd_write(");
            codegen_emit_expression(out, node->as.fd_write.fd);
            fprintf(out, ", ");
            codegen_emit_byte_buf(out, node->as.fd_write.data);
            fprintf(out, ")");
            break;

        case AST_FD_READ:
            fprintf(out, "ni_fd_read(");
            codegen_emit_expression(out, node->as.fd_read.fd);
            fprintf(out, ", ");
            codegen_emit_byte_buf(out, node->as.fd_read.buf);
            fprintf(out, ")");
            break;

        case AST_FD_OPEN:
            fprintf(out, "ni_fd_open(");
            codegen_emit_byte_buf(out, node->as.fd_open.path);
            fprintf(out, ", ");
            codegen_emit_expression(out, node->as.fd_open.flags);
            fprintf(out, ")");
            break;

        case AST_FD_CLOSE:
            fprintf(out, "ni_fd_close(");
            codegen_emit_expression(out, node->as.fd_close.fd);
            fprintf(out, ")");
            break;

        case AST_FD_SEEK:
            fprintf(out, "ni_fd_seek(");
            codegen_emit_expression(out, node->as.fd_seek.fd);
            fprintf(out, ", ");
            codegen_emit_expression(out, node->as.fd_seek.offset);
            fprintf(out, ", ");
            codegen_emit_expression(out, node->as.fd_seek.whence);
            fprintf(out, ")");
            break;

        case AST_EXIT:
            fprintf(out, "exit(");
            codegen_emit_expression(out, node->as.exit_call.code);
            fprintf(out, ")");
            break;

        case AST_MEM_COPY:
            fprintf(out, "ni_mem_copy(");
            codegen_emit_byte_buf(out, node->as.mem_copy.dst);
            fprintf(out, ", ");
            codegen_emit_byte_buf(out, node->as.mem_copy.src);
            fprintf(out, ")");
            break;

        case AST_ARGS:
            fprintf(out, "ni_args(&");
            codegen_emit_lvalue(out, node->as.args.arena);
            fprintf(out, ")");
            break;

        case AST_SHELL_RUN:
            fprintf(out, "ni_shell_run(");
            codegen_emit_byte_buf(out, node->as.shell_run.command);
            fprintf(out, ")");
            break;

        case AST_EXE_DIR:
            fprintf(out, "ni_exe_dir(&");
            codegen_emit_lvalue(out, node->as.exe_dir.arena);
            fprintf(out, ")");
            break;

        case AST_PID:
            fprintf(out, "ni_pid()");
            break;

        case AST_TIME_NS:
            fprintf(out, "ni_time_ns()");
            break;

        case AST_VAL_DECL:
        case AST_MUT_DECL:
        case AST_RETURN:
        case AST_ASSIGNMENT:
        case AST_COMPOUND_ASSIGNMENT:
        case AST_ASSERT:
        case AST_WHILE:
        case AST_FOR:
        case AST_BREAK:
        case AST_CONTINUE:
        case AST_FUNC_DECL:
        case AST_VALUE_DECL:
        case AST_ENUM_DECL:
        case AST_NATIVE_DECL:
        case AST_MATCH:
        case AST_MATCH_ARM:
        case AST_PROGRAM:
            /* These should never appear in expression context */
            panic(ERR_I002_INTERNAL_ERROR, "statement node in expression context");
            break;
    }
}

static void codegen_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; i++) {
        fprintf(out, "    ");
    }
}

static const char *codegen_type_to_c(Type type) {
    switch (type) {
        case TYPE_UNKNOWN:       return "int64_t";   /* should not happen */
        case TYPE_I64:           return "int64_t";
        case TYPE_I32:           return "int32_t";
        case TYPE_U8:            return "uint8_t";
        case TYPE_U32:           return "uint32_t";
        case TYPE_F64:           return "double";
        case TYPE_BOOL:          return "int";
        case TYPE_VOID:          return "void";
        case TYPE_COMPTIME_INT:  return "int64_t";   /* default */
        case TYPE_COMPTIME_FLOAT: return "double"; /* default */
        case TYPE_ARENA:         return "ni_Arena";
        default: {
            static char bufs[4][80];
            static int buf_idx = 0;
            ValueTypeEntry *vt = value_table_get(type);
            if (vt) {
                return codegen_mangled_name_str(vt->name_start, vt->name_length,
                                                vt->module_id);
            }
            if (type_is_array(type)) {
                char *buf = bufs[buf_idx++ % 4];
                snprintf(buf, 80, "ni_arr_%d", type_array_index(type));
                return buf;
            }
            if (type_is_slice(type)) {
                char *buf = bufs[buf_idx++ % 4];
                snprintf(buf, 80, "ni_slice_%d", type_slice_index(type));
                return buf;
            }
            EnumTypeEntry *et = enum_table_get(type);
            if (et) {
                return codegen_mangled_name_str(et->name_start, et->name_length,
                                                et->module_id);
            }
            return "int64_t";
        }
    }
}

static int g_strip_asserts = 0;

static void codegen_emit_statement(FILE *out, Ast *node, Scope **scope, int indent);

static void codegen_emit_lvalue(FILE *out, Ast *node) {
    codegen_emit_target_path(out, node, NULL, 0);
}

/* Max nesting depth for index accesses in assignment targets (e.g. a[i][j].f[k]) */
#define MAX_TARGET_INDEX_DEPTH 8

static void codegen_prepare_target_indices(FILE *out, Ast *node, int indent,
                                           CodegenTargetIndexTemp *temps,
                                           size_t *temp_count) {
    if (node->kind == AST_FIELD_ACCESS) {
        codegen_prepare_target_indices(out, node->as.field_access.object,
                                       indent, temps, temp_count);
        return;
    }

    if (node->kind != AST_INDEX_ACCESS) return;

    codegen_prepare_target_indices(out, node->as.index_access.object,
                                   indent, temps, temp_count);

    size_t slot = *temp_count;
    if (slot >= MAX_TARGET_INDEX_DEPTH) {
        panic(ERR_I002_INTERNAL_ERROR, "assignment target too deeply nested");
    }
    Ast *index_expr = node->as.index_access.index;
    temps[slot].index_node = node;
    temps[slot].temp_idx = codegen_temp_counter++;
    (*temp_count)++;

    codegen_indent(out, indent);
    fprintf(out, "%s ", codegen_type_to_c(index_expr->expr_type));
    codegen_emit_target_index_name(out, temps[slot].temp_idx);
    fprintf(out, " = ");
    codegen_emit_expression(out, index_expr);
    fprintf(out, ";\n");

    Type obj_type = node->as.index_access.object->expr_type;
    if (type_is_slice(obj_type)) {
        codegen_indent(out, indent);
        fprintf(out, "NI_BOUNDS_CHECK(");
        codegen_emit_target_index_name(out, temps[slot].temp_idx);
        fprintf(out, ", ");
        codegen_emit_target_path(out, node->as.index_access.object,
                                 temps, *temp_count);
        fprintf(out, ".len, ");
        codegen_emit_runtime_location(out, node);
        fprintf(out, ");\n");
    } else {
        ArrayTypeEntry *at = array_table_get(obj_type);
        if (at) {
            codegen_indent(out, indent);
            fprintf(out, "NI_BOUNDS_CHECK(");
            codegen_emit_target_index_name(out, temps[slot].temp_idx);
            fprintf(out, ", %zu, ", at->size);
            codegen_emit_runtime_location(out, node);
            fprintf(out, ");\n");
        }
    }
}

static size_t codegen_emit_target_index_setup(FILE *out, Ast *target, int indent,
                                               CodegenTargetIndexTemp *temps) {
    size_t count = 0;
    codegen_prepare_target_indices(out, target, indent, temps, &count);
    return count;
}

/* Emit free() for all local (non-ref) arenas in a scope */
static void codegen_emit_arena_frees(FILE *out, Scope *scope, int indent) {
    for (size_t i = 0; i < scope->count; i++) {
        if (scope->vars[i].type == TYPE_ARENA && !scope->vars[i].is_ref &&
            !scope->vars[i].is_global) {
            codegen_indent(out, indent);
            fprintf(out, "free(ni_%.*s.data);\n",
                    (int)scope->vars[i].name_length,
                    scope->vars[i].name_start);
        }
    }
}

/* Free local arenas in scopes up to and including the loop body scope */
static void codegen_emit_loop_arena_frees(FILE *out, Scope *scope, int indent) {
    Scope *s = scope;
    while (s) {
        codegen_emit_arena_frees(out, s, indent);
        if (s->parent && s->loop_depth > s->parent->loop_depth) break;
        s = s->parent;
    }
}

/* Emit block statements with scoped variable management.
 * Creates a child scope, emits statements, then restores parent scope. */
static void codegen_emit_block_statements(FILE *out, Ast *block, Scope **scope,
                                          int indent, bool is_loop) {
    *scope = scope_create(*scope);
    if (is_loop) (*scope)->loop_depth++;
    g_codegen_scope = *scope;

    for (size_t i = 0; i < block->as.block.count; i++) {
        codegen_emit_statement(out, block->as.block.statements[i], scope, indent);
    }

    /* Emit value_expr as a bare statement (e.g., trailing function call in if/while/for block) */
    if (block->as.block.value_expr) {
        if (block->as.block.value_expr->kind == AST_MATCH ||
            block->as.block.value_expr->kind == AST_IF) {
            codegen_emit_statement(out, block->as.block.value_expr, scope, indent);
        } else {
            codegen_indent(out, indent);
            codegen_emit_expression(out, block->as.block.value_expr);
            fprintf(out, ";\n");
        }
    }

    codegen_emit_arena_frees(out, *scope, indent);

    Scope *old = *scope;
    *scope = old->parent;
    g_codegen_scope = *scope;
    scope_destroy(old);
}

/* Check if an expression needs hoisting (complex block, complex if, or match) */
static bool codegen_needs_hoisting(Ast *node) {
    return !codegen_can_emit_inline(node);
}

/* Typed temps let branch values coerce to the assignment/result type. */
static bool codegen_needs_typed_temp(Ast *value, Type target_type) {
    return target_type != TYPE_UNKNOWN &&
           type_is_slice(target_type) &&
           type_is_array(value->expr_type) &&
           (value->kind == AST_BLOCK ||
            value->kind == AST_IF ||
            value->kind == AST_MATCH);
}

static void codegen_emit_match_switch(FILE *out, Ast *node, int assign_temp,
                                      Type temp_type, Scope **scope,
                                      int indent);
static void codegen_emit_if_assign(FILE *out, Ast *node, int assign_temp,
                                   Type temp_type, Scope **scope, int indent);
static void codegen_emit_block_body(FILE *out, Ast *block, int temp_idx,
                                    Type temp_type, Scope **scope,
                                    int indent);

/* Emit value with assignment-context coercions that C cannot infer. */
static void codegen_emit_expression_as_type(FILE *out, Ast *value,
                                            Type target_type) {
    if (target_type != TYPE_UNKNOWN &&
        type_is_slice(target_type) &&
        type_is_array(value->expr_type)) {
        ArrayTypeEntry *ae = array_table_get(value->expr_type);
        Type elem = type_element_type(target_type);
        fprintf(out, "(%s){.data = (%s *)",
                codegen_type_to_c(target_type), codegen_type_to_c(elem));
        codegen_emit_lvalue(out, value);
        fprintf(out, ".data, .len = %zuL}", ae ? ae->size : 0);
        return;
    }

    codegen_emit_expression(out, value);
}

/* Emit block contents and assign value_expr to temp variable if present */
static void codegen_emit_block_body(FILE *out, Ast *block, int temp_idx,
                                    Type temp_type, Scope **scope,
                                    int indent) {
    *scope = scope_create(*scope);
    g_codegen_scope = *scope;
    for (size_t i = 0; i < block->as.block.count; i++) {
        codegen_emit_statement(out, block->as.block.statements[i], scope, indent);
    }
    if (block->as.block.value_expr) {
        if (block->as.block.value_expr->kind == AST_MATCH) {
            codegen_emit_match_switch(out, block->as.block.value_expr,
                                      temp_idx, temp_type, scope, indent);
        } else if (block->as.block.value_expr->kind == AST_IF &&
                   codegen_needs_hoisting(block->as.block.value_expr)) {
            codegen_emit_if_assign(out, block->as.block.value_expr,
                                   temp_idx, temp_type, scope, indent);
        } else {
            codegen_indent(out, indent);
            fprintf(out, "__expr_%d = ", temp_idx);
            codegen_emit_expression_as_type(out, block->as.block.value_expr,
                                            temp_type);
            fprintf(out, ";\n");
        }
    }
    codegen_emit_arena_frees(out, *scope, indent);
    Scope *old = *scope;
    *scope = old->parent;
    g_codegen_scope = *scope;
    scope_destroy(old);
}

static void codegen_emit_if_assign(FILE *out, Ast *node, int assign_temp,
                                   Type temp_type, Scope **scope, int indent) {
    codegen_indent(out, indent);
    fprintf(out, "if (");
    codegen_emit_condition(out, node->as.if_stmt.condition);
    fprintf(out, ") {\n");
    codegen_emit_block_body(out, node->as.if_stmt.then_block, assign_temp,
                            temp_type, scope, indent + 1);
    codegen_indent(out, indent);
    fprintf(out, "}");

    if (node->as.if_stmt.else_block) {
        fprintf(out, " else {\n");
        codegen_emit_block_body(out, node->as.if_stmt.else_block, assign_temp,
                                temp_type, scope, indent + 1);
        codegen_indent(out, indent);
        fprintf(out, "}");
    }
    fprintf(out, "\n");
}

/* Emit a complex block expression to a temp variable.
 * Emits: TYPE __expr_N; { statements; __expr_N = value; }
 * Returns the temp index used. */
static int codegen_emit_block_to_temp(FILE *out, Ast *block, Type temp_type,
                                      Scope **scope, int indent) {
    int temp_idx = codegen_temp_counter++;
    if (temp_type == TYPE_UNKNOWN) temp_type = block->expr_type;

    codegen_indent(out, indent);
    fprintf(out, "%s __expr_%d;\n", codegen_type_to_c(temp_type), temp_idx);

    codegen_indent(out, indent);
    fprintf(out, "{\n");
    codegen_emit_block_body(out, block, temp_idx, temp_type, scope, indent + 1);
    codegen_indent(out, indent);
    fprintf(out, "}\n");

    return temp_idx;
}

/* Emit a complex if expression to a temp variable */
static int codegen_emit_if_to_temp(FILE *out, Ast *node, Type temp_type,
                                   Scope **scope, int indent) {
    int temp_idx = codegen_temp_counter++;
    if (temp_type == TYPE_UNKNOWN) temp_type = node->expr_type;

    codegen_indent(out, indent);
    fprintf(out, "%s __expr_%d;\n", codegen_type_to_c(temp_type), temp_idx);
    codegen_emit_if_assign(out, node, temp_idx, temp_type, scope, indent);
    return temp_idx;
}

/* Emit enum match lowering; tagged enums switch on .tag, plain enums on value. */
static void codegen_emit_match_enum_switch(FILE *out, Ast *node, int assign_temp,
                                           Type temp_type, Scope **scope, int indent,
                                           EnumTypeEntry *et) {
    int mid = codegen_match_counter++;
    Type scrut_type = node->as.match_expr.scrutinee->expr_type;

    codegen_indent(out, indent);
    fprintf(out, "%s ni_match_%d_ = ",
            codegen_type_to_c(scrut_type), mid);
    codegen_emit_expression(out, node->as.match_expr.scrutinee);
    fprintf(out, ";\n");

    codegen_indent(out, indent);
    if (et->has_data) {
        fprintf(out, "switch (ni_match_%d_.tag) {\n", mid);
    } else {
        fprintf(out, "switch (ni_match_%d_) {\n", mid);
    }

    for (size_t i = 0; i < node->as.match_expr.arm_count; i++) {
        Ast *arm = node->as.match_expr.arms[i];
        for (size_t p = 0; p < arm->as.match_arm.pattern_count; p++) {
            MatchArmPattern *pattern = &arm->as.match_arm.patterns[p];

            codegen_indent(out, indent + 1);
            if (pattern->kind == MATCH_PATTERN_ELSE) {
                fprintf(out, "default:\n");
            } else if (pattern->kind == MATCH_PATTERN_ENUM_VARIANT) {
                fprintf(out, "case ");
                codegen_emit_i64_literal(out, pattern->case_value);
                fprintf(out, ":\n");
            }
        }
        codegen_indent(out, indent + 1);
        fprintf(out, "{\n");

        if (arm->as.match_arm.binding_name &&
            arm->as.match_arm.binding_type != TYPE_VOID) {
            MatchArmPattern *pattern = &arm->as.match_arm.patterns[0];
            codegen_indent(out, indent + 2);
            fprintf(out, "%s ni_%.*s = ni_match_%d_.data.%.*s;\n",
                    codegen_type_to_c(arm->as.match_arm.binding_type),
                    (int)arm->as.match_arm.binding_length,
                    arm->as.match_arm.binding_name,
                    mid,
                    (int)pattern->variant_length,
                    pattern->variant_name);
        }

        if (assign_temp >= 0) {
            codegen_emit_block_body(out, arm->as.match_arm.body, assign_temp,
                                    temp_type, scope, indent + 2);
        } else {
            codegen_emit_block_statements(out, arm->as.match_arm.body,
                                           scope, indent + 2, false);
        }

        codegen_indent(out, indent + 2);
        fprintf(out, "break;\n");
        codegen_indent(out, indent + 1);
        fprintf(out, "}\n");
    }

    codegen_indent(out, indent);
    fprintf(out, "}\n");
}

/* Emit scalar match lowering as a direct C switch with literal cases. */
static void codegen_emit_match_scalar_switch(FILE *out, Ast *node,
                                             int assign_temp, Type temp_type,
                                             Scope **scope, int indent) {
    int mid = codegen_match_counter++;
    Type scrut_type = node->as.match_expr.scrutinee->expr_type;

    codegen_indent(out, indent);
    fprintf(out, "%s ni_match_%d_ = ",
            codegen_type_to_c(scrut_type), mid);
    codegen_emit_expression(out, node->as.match_expr.scrutinee);
    fprintf(out, ";\n");

    codegen_indent(out, indent);
    fprintf(out, "switch (ni_match_%d_) {\n", mid);

    for (size_t i = 0; i < node->as.match_expr.arm_count; i++) {
        Ast *arm = node->as.match_expr.arms[i];
        for (size_t p = 0; p < arm->as.match_arm.pattern_count; p++) {
            MatchArmPattern *pattern = &arm->as.match_arm.patterns[p];

            codegen_indent(out, indent + 1);
            if (pattern->kind == MATCH_PATTERN_WILDCARD ||
                pattern->kind == MATCH_PATTERN_ELSE) {
                fprintf(out, "default:\n");
            } else if (pattern->kind == MATCH_PATTERN_LITERAL) {
                fprintf(out, "case ");
                codegen_emit_i64_literal(out, pattern->case_value);
                fprintf(out, ":\n");
            }
        }
        codegen_indent(out, indent + 1);
        fprintf(out, "{\n");

        if (assign_temp >= 0) {
            codegen_emit_block_body(out, arm->as.match_arm.body, assign_temp,
                                    temp_type, scope, indent + 2);
        } else {
            codegen_emit_block_statements(out, arm->as.match_arm.body,
                                           scope, indent + 2, false);
        }

        codegen_indent(out, indent + 2);
        fprintf(out, "break;\n");
        codegen_indent(out, indent + 1);
        fprintf(out, "}\n");
    }

    codegen_indent(out, indent);
    fprintf(out, "}\n");
}

/* Emit match switch. If assign_temp >= 0, assign arm value_expr to __expr_<assign_temp>.
 * If assign_temp < 0, emit arm body as bare statements. */
static void codegen_emit_match_switch(FILE *out, Ast *node, int assign_temp,
                                      Type temp_type, Scope **scope, int indent) {
    Type scrut_type = node->as.match_expr.scrutinee->expr_type;
    EnumTypeEntry *et = enum_table_get(scrut_type);

    if (et) {
        codegen_emit_match_enum_switch(out, node, assign_temp, temp_type, scope,
                                       indent, et);
        return;
    }

    codegen_emit_match_scalar_switch(out, node, assign_temp, temp_type, scope,
                                     indent);
}

/* Emit a match expression to a temp variable */
static int codegen_emit_match_to_temp(FILE *out, Ast *node, Type temp_type,
                                      Scope **scope, int indent) {
    int temp_idx = codegen_temp_counter++;
    if (temp_type == TYPE_UNKNOWN) temp_type = node->expr_type;

    codegen_indent(out, indent);
    fprintf(out, "%s __expr_%d;\n", codegen_type_to_c(temp_type), temp_idx);

    codegen_emit_match_switch(out, node, temp_idx, temp_type, scope, indent);

    return temp_idx;
}

/* Emit a variable declaration (val or mut) */
static void codegen_emit_var_decl(FILE *out, const char *name_start, size_t name_length,
                                   Type type, Ast *init, bool is_const,
                                   Scope **scope, int indent) {
    const char *const_prefix = is_const ? "const " : "";
    const char *type_str = codegen_type_to_c(type);

    if (codegen_needs_hoisting(init) || codegen_needs_typed_temp(init, type)) {
        int temp_idx;
        if (init->kind == AST_BLOCK) {
            temp_idx = codegen_emit_block_to_temp(out, init, type, scope, indent);
        } else if (init->kind == AST_MATCH) {
            temp_idx = codegen_emit_match_to_temp(out, init, type, scope, indent);
        } else {
            temp_idx = codegen_emit_if_to_temp(out, init, type, scope, indent);
        }
        codegen_indent(out, indent);
        fprintf(out, "%s%s ni_%.*s = __expr_%d;\n",
                const_prefix, type_str, (int)name_length, name_start, temp_idx);
    } else {
        codegen_indent(out, indent);
        fprintf(out, "%s%s ni_%.*s = ",
                const_prefix, type_str, (int)name_length, name_start);
        codegen_emit_expression_as_type(out, init, type);
        fprintf(out, ";\n");
    }
}

/* Assignment RHS control-flow is lowered through a temp before the final store. */
static int codegen_emit_value_to_temp_if_needed(FILE *out, Ast *value,
                                                Type target_type,
                                                Scope **scope, int indent) {
    if (!codegen_needs_hoisting(value) &&
        !codegen_needs_typed_temp(value, target_type)) {
        return -1;
    }

    if (value->kind == AST_BLOCK) {
        return codegen_emit_block_to_temp(out, value, target_type, scope, indent);
    }
    if (value->kind == AST_MATCH) {
        return codegen_emit_match_to_temp(out, value, target_type, scope, indent);
    }
    return codegen_emit_if_to_temp(out, value, target_type, scope, indent);
}

/* Emit either the hoisted temp or the original inline expression. */
static void codegen_emit_assignment_value(FILE *out, Ast *value, int temp_idx,
                                          Type target_type) {
    if (temp_idx >= 0) {
        fprintf(out, "__expr_%d", temp_idx);
        return;
    }

    codegen_emit_expression_as_type(out, value, target_type);
}

static void codegen_emit_statement(FILE *out, Ast *node, Scope **scope, int indent) {
    switch (node->kind) {
        case AST_VAL_DECL:
            scope_add(*scope, node->as.val_decl.name_start,
                      node->as.val_decl.name_length, false,
                      node->as.val_decl.type, node->loc);
            codegen_emit_var_decl(out, node->as.val_decl.name_start,
                                  node->as.val_decl.name_length,
                                  node->as.val_decl.type,
                                  node->as.val_decl.initializer, true,
                                  scope, indent);
            break;

        case AST_MUT_DECL:
            scope_add(*scope, node->as.mut_decl.name_start,
                      node->as.mut_decl.name_length, true,
                      node->as.mut_decl.type, node->loc);
            codegen_emit_var_decl(out, node->as.mut_decl.name_start,
                                  node->as.mut_decl.name_length,
                                  node->as.mut_decl.type,
                                  node->as.mut_decl.initializer, false,
                                  scope, indent);
            break;

        case AST_RETURN: {
            /* Free all local arenas in the scope chain before returning */
            Scope *s = *scope;
            while (s) {
                codegen_emit_arena_frees(out, s, indent);
                s = s->parent;
            }
            codegen_indent(out, indent);
            if (node->as.return_stmt.value) {
                fprintf(out, "return ");
                codegen_emit_expression(out, node->as.return_stmt.value);
                fprintf(out, ";\n");
            } else {
                fprintf(out, "return;\n");
            }
            break;
        }

        case AST_ASSIGNMENT:
        {
            Ast *target = node->as.assignment.target;
            Ast *value = node->as.assignment.value;
            CodegenTargetIndexTemp temps[MAX_TARGET_INDEX_DEPTH];
            size_t temp_count = codegen_emit_target_index_setup(out, target,
                                                                 indent, temps);
            int value_temp = codegen_emit_value_to_temp_if_needed(out, value,
                                                                  target->expr_type,
                                                                  scope, indent);
            codegen_indent(out, indent);
            codegen_emit_target_path(out, target, temps, temp_count);
            fprintf(out, " = ");
            codegen_emit_assignment_value(out, value, value_temp,
                                          target->expr_type);
            fprintf(out, ";\n");
            break;
        }

        case AST_COMPOUND_ASSIGNMENT:
        {
            Ast *target = node->as.compound_assignment.target;
            Ast *value = node->as.compound_assignment.value;
            CodegenTargetIndexTemp temps[MAX_TARGET_INDEX_DEPTH];
            size_t temp_count = codegen_emit_target_index_setup(out, target,
                                                                 indent, temps);
            int value_temp = codegen_emit_value_to_temp_if_needed(out, value,
                                                                  target->expr_type,
                                                                  scope, indent);
            codegen_indent(out, indent);
            codegen_emit_target_path(out, target, temps, temp_count);
            codegen_emit_compound_assign_op(out,
                                            node->as.compound_assignment.op);
            codegen_emit_assignment_value(out, value, value_temp,
                                          target->expr_type);
            fprintf(out, ";\n");
            break;
        }

        case AST_ASSERT:
            if (g_strip_asserts) {
                break;
            }
            codegen_indent(out, indent);
            fprintf(out, "if (!(");
            codegen_emit_expression(out, node->as.assert_stmt.condition);
            fprintf(out, ")) {\n");
            codegen_indent(out, indent + 1);
            fprintf(out, "fprintf(stderr, ");
            codegen_emit_c_string_literal(out, node->loc.file);
            fprintf(out, " \":%zu:%zu: error[R001]: Assertion failed\\n\");\n",
                    node->loc.line, node->loc.column);
            codegen_indent(out, indent + 1);
            fprintf(out, "exit(%d);\n", EXIT_RUNTIME);
            codegen_indent(out, indent);
            fprintf(out, "}\n");
            break;

        case AST_IF: {
            Ast *then_block = node->as.if_stmt.then_block;
            Ast *else_block = node->as.if_stmt.else_block;

            codegen_indent(out, indent);
            fprintf(out, "if (");
            codegen_emit_condition(out, node->as.if_stmt.condition);
            fprintf(out, ") {\n");

            codegen_emit_block_statements(out, then_block, scope, indent + 1, false);

            codegen_indent(out, indent);
            fprintf(out, "}");

            if (else_block) {
                fprintf(out, " else {\n");
                codegen_emit_block_statements(out, else_block, scope, indent + 1, false);
                codegen_indent(out, indent);
                fprintf(out, "}");
            }
            fprintf(out, "\n");
            break;
        }

        case AST_WHILE: {
            Ast *body = node->as.while_stmt.body;

            codegen_indent(out, indent);
            fprintf(out, "while (");
            codegen_emit_condition(out, node->as.while_stmt.condition);
            fprintf(out, ") {\n");

            codegen_emit_block_statements(out, body, scope, indent + 1, true);

            codegen_indent(out, indent);
            fprintf(out, "}\n");
            break;
        }

        case AST_FOR: {
            Ast *body = node->as.for_stmt.body;
            const char *var = node->as.for_stmt.var_start;
            int var_len = (int)node->as.for_stmt.var_length;
            int end_temp = codegen_temp_counter++;

            /* Evaluate end once into a temp */
            codegen_indent(out, indent);
            fprintf(out, "const int64_t __for_end_%d = (int64_t)", end_temp);
            codegen_emit_expression(out, node->as.for_stmt.end);
            fprintf(out, ";\n");

            codegen_indent(out, indent);
            fprintf(out, "for (int64_t ni_%.*s = (int64_t)", var_len, var);
            codegen_emit_expression(out, node->as.for_stmt.start);
            fprintf(out, "; ni_%.*s < __for_end_%d; ni_%.*s++) {\n",
                    var_len, var, end_temp, var_len, var);

            /* Emit body with loop variable injected into scope */
            *scope = scope_create(*scope);
            (*scope)->loop_depth++;
            g_codegen_scope = *scope;
            scope_add(*scope, var, node->as.for_stmt.var_length,
                      false, TYPE_I64, node->loc);

            for (size_t i = 0; i < body->as.block.count; i++) {
                codegen_emit_statement(out, body->as.block.statements[i], scope, indent + 1);
            }

            /* Emit value_expr as a bare statement (trailing function call before }) */
            if (body->as.block.value_expr) {
                codegen_indent(out, indent + 1);
                codegen_emit_expression(out, body->as.block.value_expr);
                fprintf(out, ";\n");
            }

            codegen_emit_arena_frees(out, *scope, indent + 1);

            Scope *old = *scope;
            *scope = old->parent;
            g_codegen_scope = *scope;
            scope_destroy(old);

            codegen_indent(out, indent);
            fprintf(out, "}\n");
            break;
        }

        case AST_BREAK: {
            if ((*scope)->loop_depth == 0) {
                diagnostic(node->loc.file, ERR_S004_BREAK_OUTSIDE_LOOP, node->loc.line,
                           node->loc.column, "'break' outside of loop");
                break;  /* Skip code generation */
            }
            codegen_emit_loop_arena_frees(out, *scope, indent);
            codegen_indent(out, indent);
            fprintf(out, "break;\n");
            break;
        }

        case AST_CONTINUE: {
            if ((*scope)->loop_depth == 0) {
                diagnostic(node->loc.file, ERR_S005_CONTINUE_OUTSIDE_LOOP, node->loc.line,
                           node->loc.column, "'continue' outside of loop");
                break;  /* Skip code generation */
            }
            codegen_emit_loop_arena_frees(out, *scope, indent);
            codegen_indent(out, indent);
            fprintf(out, "continue;\n");
            break;
        }

        case AST_BLOCK:
            codegen_indent(out, indent);
            fprintf(out, "{\n");
            codegen_emit_block_statements(out, node, scope, indent + 1, false);
            codegen_indent(out, indent);
            fprintf(out, "}\n");
            break;

        case AST_VALUE_DECL:
        case AST_ENUM_DECL:
        case AST_NATIVE_DECL:
            /* Typedef already emitted at top level / no codegen needed */
            break;

        case AST_FUNC_CALL:
        case AST_ARENA_RESET:
        case AST_TABLE_INSERT:
        case AST_FD_WRITE:
        case AST_FD_READ:
        case AST_FD_CLOSE:
        case AST_FD_SEEK:
        case AST_MEM_COPY:
        case AST_EXIT:
        case AST_ARGS:
        case AST_SHELL_RUN:
            codegen_indent(out, indent);
            codegen_emit_expression(out, node);
            fprintf(out, ";\n");
            break;

        case AST_MATCH:
            codegen_emit_match_switch(out, node, -1, TYPE_UNKNOWN, scope, indent);
            break;

        default:
            panic(ERR_I002_INTERNAL_ERROR, "invalid statement type in code generation");
    }
}

static void codegen_emit_global_decl(FILE *out, Ast *node) {
    bool is_val = (node->kind == AST_VAL_DECL);
    Type type = is_val ? node->as.val_decl.type : node->as.mut_decl.type;
    const char *name_start = is_val ? node->as.val_decl.name_start
                                    : node->as.mut_decl.name_start;
    size_t name_length = is_val ? node->as.val_decl.name_length
                                : node->as.mut_decl.name_length;
    Ast *init = is_val ? node->as.val_decl.initializer
                       : node->as.mut_decl.initializer;
    const char *const_prefix = is_val ? "const " : "";
    size_t mod = module_for_file(node->loc.file);

    /* Comptime vals: skip C emission (inlined at use sites) */
    if (is_val && type_is_comptime(type)) return;

    /* String literal: use brace initializer (val only, mut string rejected by typecheck) */
    if (is_val && type_is_str(type)) {
        fprintf(out, "const %s ", codegen_type_to_c(type));
        codegen_emit_mangled_name(out, name_start, name_length, mod);
        fprintf(out, " = {.data = (uint8_t *)\"%.*s\", .len = %zuL};\n",
                (int)init->as.string_literal.raw_length,
                init->as.string_literal.start,
                init->as.string_literal.byte_length);
        return;
    }

    /* Arena: declaration only -- init deferred to main */
    if (!is_val && type == TYPE_ARENA) {
        fprintf(out, "ni_Arena ");
        codegen_emit_mangled_name(out, name_start, name_length, mod);
        fprintf(out, ";\nstatic const int64_t ");
        codegen_emit_mangled_name(out, name_start, name_length, mod);
        fprintf(out, "_cap = ");
        codegen_emit_expression(out, init->as.arena_new.capacity);
        fprintf(out, ";\n");
        return;
    }

    /* Value constructor: brace initializer */
    if (type_is_value(type) && init->kind == AST_VALUE_CONSTRUCTOR) {
        fprintf(out, "%s%s ", const_prefix, codegen_type_to_c(type));
        codegen_emit_mangled_name(out, name_start, name_length, mod);
        fprintf(out, " = {");
        for (size_t i = 0; i < init->as.value_constructor.field_count; i++) {
            if (i > 0) fprintf(out, ", ");
            FieldInit *fi = &init->as.value_constructor.fields[i];
            fprintf(out, ".ni_%.*s = ", (int)fi->name_length, fi->name_start);
            codegen_emit_expression(out, fi->value);
        }
        fprintf(out, "};\n");
        return;
    }

    /* Array literal: brace initializer */
    if (type_is_array(type) && init->kind == AST_ARRAY_LITERAL) {
        Type concrete = codegen_concrete_array_type(init->expr_type);
        fprintf(out, "%s%s ", const_prefix, codegen_type_to_c(concrete));
        codegen_emit_mangled_name(out, name_start, name_length, mod);
        fprintf(out, " = {{");
        for (size_t i = 0; i < init->as.array_literal.element_count; i++) {
            if (i > 0) fprintf(out, ", ");
            codegen_emit_expression(out, init->as.array_literal.elements[i]);
        }
        fprintf(out, "}};\n");
        return;
    }

    /* Scalar */
    fprintf(out, "%s%s ", const_prefix, codegen_type_to_c(type));
    codegen_emit_mangled_name(out, name_start, name_length, mod);
    fprintf(out, " = ");
    codegen_emit_expression(out, init);
    fprintf(out, ";\n");
}

/* Emit initialization for all global arenas (called at start of main) */
static void codegen_emit_global_arena_inits(FILE *out, int indent) {
    if (!g_global_codegen_scope) return;
    for (size_t i = 0; i < g_global_codegen_scope->count; i++) {
        Variable *v = &g_global_codegen_scope->vars[i];
        if (v->type == TYPE_ARENA && v->is_global) {
            codegen_indent(out, indent);
            codegen_emit_mangled_name(out, v->name_start, v->name_length, v->module_id);
            fprintf(out, " = ni_arena_new(");
            codegen_emit_mangled_name(out, v->name_start, v->name_length, v->module_id);
            fprintf(out, "_cap);\n");
        }
    }
}

/* Emit free() for all global arenas (called at end of main) */
static void codegen_emit_global_arena_frees(FILE *out, int indent) {
    if (!g_global_codegen_scope) return;
    for (size_t i = 0; i < g_global_codegen_scope->count; i++) {
        Variable *v = &g_global_codegen_scope->vars[i];
        if (v->type == TYPE_ARENA && v->is_global) {
            codegen_indent(out, indent);
            fprintf(out, "free(");
            codegen_emit_mangled_name(out, v->name_start, v->name_length, v->module_id);
            fprintf(out, ".data);\n");
        }
    }
}

static bool codegen_function_is_main(Ast *func_decl) {
    return func_decl->as.func_decl.name_length == 4 &&
           memcmp(func_decl->as.func_decl.name_start, "main", 4) == 0;
}

static void codegen_emit_function_params(FILE *out, Ast *func_decl, bool is_main) {
    if (is_main && g_has_args) {
        /* argc/argv already emitted by the signature helper */
        return;
    }

    if (func_decl->as.func_decl.param_count == 0) {
        fprintf(out, "void");
        return;
    }

    for (size_t i = 0; i < func_decl->as.func_decl.param_count; i++) {
        Parameter *param = &func_decl->as.func_decl.params[i];
        if (i > 0) fprintf(out, ", ");
        if (type_is_slice(param->type)) {
            /* Slice: passed by value as fat pointer struct */
            fprintf(out, "%s ni_%.*s", codegen_type_to_c(param->type),
                    (int)param->name_length, param->name_start);
        } else if (param->is_mut_ref) {
            fprintf(out, "%s *ni_%.*s", codegen_type_to_c(param->type),
                    (int)param->name_length, param->name_start);
        } else if (param->is_ref) {
            fprintf(out, "const %s *ni_%.*s", codegen_type_to_c(param->type),
                    (int)param->name_length, param->name_start);
        } else {
            fprintf(out, "%s ni_%.*s", codegen_type_to_c(param->type),
                    (int)param->name_length, param->name_start);
        }
    }
}

static void codegen_emit_function_signature(FILE *out, Ast *func_decl) {
    const char *name_start = func_decl->as.func_decl.name_start;
    size_t name_length = func_decl->as.func_decl.name_length;
    Type return_type = func_decl->as.func_decl.return_type;
    bool is_main = codegen_function_is_main(func_decl);

    size_t func_mod = module_for_file(func_decl->loc.file);

    if (is_main) {
        if (g_has_args) {
            fprintf(out, "int main(int argc, char **argv");
        } else {
            fprintf(out, "int main(");
        }
    } else {
        fprintf(out, "%s ", codegen_type_to_c(return_type));
        codegen_emit_mangled_name(out, name_start, name_length, func_mod);
        fprintf(out, "(");
    }

    codegen_emit_function_params(out, func_decl, is_main);
    fprintf(out, ")");
}

static void codegen_emit_function_prototype(FILE *out, Ast *func_decl) {
    codegen_emit_function_signature(out, func_decl);
    fprintf(out, ";\n");
}

static void codegen_emit_function(FILE *out, Ast *func_decl) {
    bool is_main = codegen_function_is_main(func_decl);

    codegen_emit_function_signature(out, func_decl);
    fprintf(out, " {\n");

    /* Create scope with parameters (global scope as parent for codegen) */
    Scope *scope = scope_create(g_global_codegen_scope);
    for (size_t i = 0; i < func_decl->as.func_decl.param_count; i++) {
        Parameter *param = &func_decl->as.func_decl.params[i];
        bool is_mutable = param->is_mut_ref;
        Variable *v = scope_add(scope, param->name_start, param->name_length,
                                is_mutable, param->type, func_decl->loc);
        if (v) {
            /* Slice params are passed by value as fat pointer struct, not as C pointer */
            v->is_ref = type_is_slice(param->type) ? false : param->is_ref;
        }
    }

    /* Set global codegen scope */
    g_codegen_scope = scope;

    /* For main: emit global arena initializations */
    if (is_main) codegen_emit_global_arena_inits(out, 1);

    /* For main: save argc/argv to static globals for ni_args() */
    if (is_main && g_has_args) {
        codegen_indent(out, 1);
        fprintf(out, "ni_argc = argc;\n");
        codegen_indent(out, 1);
        fprintf(out, "ni_argv = argv;\n");
    }

    /* Emit body */
    Ast *body = func_decl->as.func_decl.body;
    for (size_t i = 0; i < body->as.block.count; i++) {
        codegen_emit_statement(out, body->as.block.statements[i], &scope, 1);
    }
    /* Emit trailing value expression: return for non-void, bare statement for void */
    if (body->as.block.value_expr) {
        Ast *vexpr = body->as.block.value_expr;
        bool is_non_void = (func_decl->as.func_decl.return_type != TYPE_VOID);
        bool needs_temp = codegen_needs_hoisting(vexpr) ||
                          codegen_needs_typed_temp(vexpr,
                                                   func_decl->as.func_decl.return_type);
        if (needs_temp && is_non_void) {
            int temp_idx;
            if (vexpr->kind == AST_BLOCK) {
                temp_idx = codegen_emit_block_to_temp(out, vexpr,
                                                      func_decl->as.func_decl.return_type,
                                                      &scope, 1);
            } else if (vexpr->kind == AST_MATCH) {
                temp_idx = codegen_emit_match_to_temp(out, vexpr,
                                                      func_decl->as.func_decl.return_type,
                                                      &scope, 1);
            } else {
                temp_idx = codegen_emit_if_to_temp(out, vexpr,
                                                  func_decl->as.func_decl.return_type,
                                                  &scope, 1);
            }
            codegen_indent(out, 1);
            fprintf(out, "return __expr_%d;\n", temp_idx);
        } else if (codegen_needs_hoisting(vexpr)) {
            codegen_emit_statement(out, vexpr, &scope, 1);
        } else {
            codegen_indent(out, 1);
            if (is_non_void) {
                fprintf(out, "return ");
            }
            codegen_emit_expression_as_type(out, vexpr,
                                            func_decl->as.func_decl.return_type);
            fprintf(out, ";\n");
        }
    }

    /* Free local arenas at end of function (for void functions with no explicit return) */
    codegen_emit_arena_frees(out, scope, 1);

    /* For main: free global arenas */
    if (is_main) codegen_emit_global_arena_frees(out, 1);

    g_codegen_scope = NULL;
    scope_destroy(scope);

    fprintf(out, "}\n\n");
}

/* Check whether all custom-type dependencies among fields have been emitted.
 * emitted[] layout: [0..vcount) value types, [vcount..vcount+acount) arrays,
 *                   [vcount+acount..total) slices. */
static bool type_deps_emitted(Parameter *fields, size_t count,
                              const bool *emitted, size_t vcount, size_t acount) {
    for (size_t i = 0; i < count; i++) {
        Type t = fields[i].type;
        if (type_is_value(t) && !emitted[type_value_index(t)]) return false;
        if (type_is_array(t) && !emitted[vcount + type_array_index(t)]) return false;
        if (type_is_slice(t) && !emitted[vcount + acount + type_slice_index(t)]) return false;
    }
    return true;
}

/* Single-field dependency check helper for array/slice element types */
static bool type_elem_dep_emitted(Type elem_type, const bool *emitted,
                                   size_t vcount, size_t acount) {
    Parameter p = {.type = elem_type};
    return type_deps_emitted(&p, 1, emitted, vcount, acount);
}

static void codegen_emit_ir(FILE *out, Ast *ast) {
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <stdint.h>\n");
    if (g_has_arena) fprintf(out, "#include <stddef.h>\n");
    if (g_has_casts) fprintf(out, "#include <limits.h>\n");
    if (g_has_arena || g_has_io || g_has_mem || g_has_args || g_has_driver) fprintf(out, "#include <string.h>\n");
    if (g_has_io || g_has_driver) {
        fprintf(out, "#include <unistd.h>\n");
    }
    if (g_has_io) {
        fprintf(out, "#include <fcntl.h>\n");
    }
    if (g_has_time || g_has_driver) {
        fprintf(out, "#include <sys/time.h>\n");
    }
    if (g_has_driver) {
        fprintf(out, "#include <sys/wait.h>\n");
    }
    fprintf(out, "\n");

    if (ast->kind != AST_PROGRAM) {
        panic(ERR_I002_INTERNAL_ERROR, "codegen_emit_ir expects AST_PROGRAM");
    }

    /* Emit bounds check helper and macro if any array or slice types exist */
    if (g_array_table->count > 0 || g_slice_table->count > 0) {
        fprintf(out, "static void ni_bounds_fail(int64_t idx, size_t size,\n");
        fprintf(out, "                           const char *file, unsigned long line, unsigned long col) {\n");
        fprintf(out, "    fprintf(stderr, \"%%s:%%lu:%%lu: error[R002]: Array index out of bounds: %%ld not in [0, %%zu)\\n\",\n");
        fprintf(out, "            file, line, col, (long)idx, size);\n");
        fprintf(out, "    exit(2);\n");
        fprintf(out, "}\n");
        fprintf(out, "#define NI_BOUNDS_CHECK(idx, size, file, line, col) \\\n");
        fprintf(out, "    ((uint64_t)(idx) >= (uint64_t)(size) \\\n");
        fprintf(out, "     ? (ni_bounds_fail((int64_t)(idx), (size_t)(size), (file), (line), (col)), 0) : 0)\n");
        fprintf(out, "static void ni_slice_bounds_fail(int64_t s, int64_t e, int64_t len,\n");
        fprintf(out, "                                 const char *file, unsigned long line, unsigned long col) {\n");
        fprintf(out, "    fprintf(stderr, \"%%s:%%lu:%%lu: error[R004]: Slice bounds out of range: [%%ld..%%ld) not in [0, %%ld)\\n\",\n");
        fprintf(out, "            file, line, col, (long)s, (long)e, (long)len);\n");
        fprintf(out, "    exit(2);\n");
        fprintf(out, "}\n");
        fprintf(out, "#define NI_SLICE_BOUNDS_CHECK(s, e, len, file, line, col) \\\n");
        fprintf(out, "    ((s) < 0 || (e) < (s) || (e) > (len) \\\n");
        fprintf(out, "     ? (ni_slice_bounds_fail((int64_t)(s), (int64_t)(e), (int64_t)(len), (file), (line), (col)), 0) : 0)\n\n");
    }

    /* Emit cast runtime if any type casts exist */
    if (g_has_casts) {
        fprintf(out, "static void ni_cast_fail(const char *from, const char *to,\n");
        fprintf(out, "                         const char *file, unsigned long line, unsigned long col) {\n");
        fprintf(out, "    fprintf(stderr, \"%%s:%%lu:%%lu: error[R003]: Cast overflow: %%s to %%s\\n\",\n");
        fprintf(out, "            file, line, col, from, to);\n");
        fprintf(out, "    exit(2);\n");
        fprintf(out, "}\n");
        fprintf(out, "static uint8_t ni_cast_u8(int64_t v, const char *from,\n");
        fprintf(out, "        const char *file, unsigned long line, unsigned long col) {\n");
        fprintf(out, "    if (v < 0 || v > 255) ni_cast_fail(from, \"u8\", file, line, col);\n");
        fprintf(out, "    return (uint8_t)v;\n}\n");
        fprintf(out, "static int32_t ni_cast_i32(int64_t v, const char *from,\n");
        fprintf(out, "        const char *file, unsigned long line, unsigned long col) {\n");
        fprintf(out, "    if (v < INT32_MIN || v > INT32_MAX) ni_cast_fail(from, \"i32\", file, line, col);\n");
        fprintf(out, "    return (int32_t)v;\n}\n");
        fprintf(out, "static uint32_t ni_cast_u32(int64_t v, const char *from,\n");
        fprintf(out, "        const char *file, unsigned long line, unsigned long col) {\n");
        fprintf(out, "    if (v < 0 || v > (int64_t)UINT32_MAX) ni_cast_fail(from, \"u32\", file, line, col);\n");
        fprintf(out, "    return (uint32_t)v;\n}\n");
        fprintf(out, "static int64_t ni_cast_i64_f64(double v, const char *from,\n");
        fprintf(out, "        const char *file, unsigned long line, unsigned long col) {\n");
        fprintf(out, "    if (v != v || v < -9.2233720368547758e+18 || v > 9.2233720368547758e+18)\n");
        fprintf(out, "        ni_cast_fail(from, \"i64\", file, line, col);\n");
        fprintf(out, "    return (int64_t)v;\n}\n");
        fprintf(out, "static int32_t ni_cast_i32_f64(double v, const char *from,\n");
        fprintf(out, "        const char *file, unsigned long line, unsigned long col) {\n");
        fprintf(out, "    if (v != v || v < -2147483648.0 || v > 2147483647.0)\n");
        fprintf(out, "        ni_cast_fail(from, \"i32\", file, line, col);\n");
        fprintf(out, "    return (int32_t)v;\n}\n");
        fprintf(out, "static uint8_t ni_cast_u8_f64(double v, const char *from,\n");
        fprintf(out, "        const char *file, unsigned long line, unsigned long col) {\n");
        fprintf(out, "    if (v != v || v < 0.0 || v > 255.0)\n");
        fprintf(out, "        ni_cast_fail(from, \"u8\", file, line, col);\n");
        fprintf(out, "    return (uint8_t)v;\n}\n");
        fprintf(out, "static uint32_t ni_cast_u32_f64(double v, const char *from,\n");
        fprintf(out, "        const char *file, unsigned long line, unsigned long col) {\n");
        fprintf(out, "    if (v != v || v < 0.0 || v > 4294967295.0)\n");
        fprintf(out, "        ni_cast_fail(from, \"u32\", file, line, col);\n");
        fprintf(out, "    return (uint32_t)v;\n}\n");
        fprintf(out, "\n");
    }

    /* Emit Arena runtime if arena is used */
    if (g_has_arena) {
        fprintf(out, "#define NI_ALIGNOF(T) offsetof(struct { char c; T value; }, value)\n\n");
        fprintf(out, "typedef struct { uint8_t *data; size_t capacity; size_t offset; } ni_Arena;\n\n");
        fprintf(out, "static ni_Arena ni_arena_new(int64_t capacity) {\n");
        fprintf(out, "    ni_Arena a;\n");
        fprintf(out, "    a.data = (uint8_t *)malloc((size_t)capacity);\n");
        fprintf(out, "    if (!a.data) { fprintf(stderr, \"Arena allocation failed\\n\"); exit(1); }\n");
        fprintf(out, "    a.capacity = (size_t)capacity;\n");
        fprintf(out, "    a.offset = 0;\n");
        fprintf(out, "    return a;\n");
        fprintf(out, "}\n\n");
        fprintf(out, "static size_t ni_align_up(size_t offset, size_t align) {\n");
        fprintf(out, "    if (align <= 1) return offset;\n");
        fprintf(out, "    size_t rem = offset %% align;\n");
        fprintf(out, "    if (rem == 0) return offset;\n");
        fprintf(out, "    return offset + (align - rem);\n");
        fprintf(out, "}\n\n");
        fprintf(out, "static void *ni_arena_alloc(ni_Arena *a, int64_t count, size_t elem_size, size_t elem_align) {\n");
        fprintf(out, "    size_t bytes = (size_t)count * elem_size;\n");
        fprintf(out, "    size_t offset = ni_align_up(a->offset, elem_align);\n");
        fprintf(out, "    if (offset + bytes > a->capacity) {\n");
        fprintf(out, "        fprintf(stderr, \"Arena out of memory\\n\"); exit(1);\n");
        fprintf(out, "    }\n");
        fprintf(out, "    void *ptr = a->data + offset;\n");
        fprintf(out, "    memset(ptr, 0, bytes);\n");
        fprintf(out, "    a->offset = offset + bytes;\n");
        fprintf(out, "    return ptr;\n");
        fprintf(out, "}\n\n");
        fprintf(out, "static void ni_arena_reset(ni_Arena *a) { a->offset = 0; }\n\n");
    }

    /* Emit plain enum typedefs (no dependencies, emit first) */
    for (size_t i = 0; i < g_enum_count; i++) {
        EnumTypeEntry *et = &g_enum_table[i];
        if (!et->has_data) {
            fprintf(out, "typedef int64_t %s;\n",
                    codegen_mangled_name_str(et->name_start, et->name_length,
                                             et->module_id));
        }
    }
    if (g_enum_count > 0) fprintf(out, "\n");

    /* Emit type typedefs in dependency order (topological sort).
     * Types with no unresolved deps are emitted first. */
    size_t vcount = g_value_table->count;
    size_t acount = g_array_table->count;
    size_t scount = g_slice_table->count;
    size_t total = vcount + acount + scount;

    if (total > 0) {
        bool *emitted = calloc(total, sizeof(bool));
        size_t done = 0;

        while (done < total) {
            size_t progress = 0;

            for (size_t i = 0; i < vcount; i++) {
                if (emitted[i]) continue;
                ValueTypeEntry *vt = &g_value_table->types[i];
                if (!type_deps_emitted(vt->fields, vt->field_count, emitted, vcount, acount)) continue;
                fprintf(out, "typedef struct {");
                for (size_t f = 0; f < vt->field_count; f++) {
                    Parameter *field = &vt->fields[f];
                    fprintf(out, " %s ni_%.*s;", codegen_type_to_c(field->type),
                            (int)field->name_length, field->name_start);
                }
                fprintf(out, " } %s;\n",
                        codegen_type_to_c((Type)(TYPE_VALUE_BASE + i)));
                emitted[i] = true;
                done++; progress++;
            }

            for (size_t i = 0; i < acount; i++) {
                if (emitted[vcount + i]) continue;
                ArrayTypeEntry *at = &g_array_table->types[i];
                if (!type_elem_dep_emitted(at->element_type, emitted, vcount, acount)) continue;
                fprintf(out, "typedef struct { %s data[%zu]; } ni_arr_%zu;\n",
                        codegen_type_to_c(at->element_type), at->size, i);
                emitted[vcount + i] = true;
                done++; progress++;
            }

            for (size_t i = 0; i < scount; i++) {
                if (emitted[vcount + acount + i]) continue;
                SliceTypeEntry *se = &g_slice_table->types[i];
                if (!type_elem_dep_emitted(se->element_type, emitted, vcount, acount)) continue;
                fprintf(out, "typedef struct { %s *data; int64_t len; } ni_slice_%zu;\n",
                        codegen_type_to_c(se->element_type), i);
                emitted[vcount + acount + i] = true;
                done++; progress++;
            }

            if (progress == 0) break;
        }
        free(emitted);
        fprintf(out, "\n");
    }

    for (size_t i = 0; i < scount; i++) {
        fprintf(out, "static inline ni_slice_%zu ni_slice_%zu_tail(ni_slice_%zu ni_value, int64_t ni_start,\n",
                i, i, i);
        fprintf(out, "                                            const char *ni_file, unsigned long ni_line, unsigned long ni_col) {\n");
        fprintf(out, "    NI_SLICE_BOUNDS_CHECK(ni_start, ni_value.len, ni_value.len, ni_file, ni_line, ni_col);\n");
        fprintf(out, "    return (ni_slice_%zu){.data = ni_value.data + ni_start, .len = (ni_value.len - ni_start)};\n", i);
        fprintf(out, "}\n");
    }
    if (scount > 0) fprintf(out, "\n");

    /* Emit tagged union struct typedefs (needs slice types to be defined) */
    for (size_t i = 0; i < g_enum_count; i++) {
        EnumTypeEntry *et = &g_enum_table[i];
        if (!et->has_data) continue;
        fprintf(out, "typedef struct {\n    int64_t tag;\n    union {");
        for (size_t v = 0; v < et->variant_count; v++) {
            EnumVariant *ev = &et->variants[v];
            if (ev->payload_type == TYPE_VOID) continue;
            fprintf(out, "\n        %s %.*s;",
                    codegen_type_to_c(ev->payload_type),
                    (int)ev->name_length, ev->name_start);
        }
        fprintf(out, "\n    } data;\n} %s;\n\n",
                codegen_mangled_name_str(et->name_start, et->name_length,
                                         et->module_id));
    }

    /* Emit I/O runtime helpers if any I/O built-ins are used */
    if (g_has_io) {
        fprintf(out, "static int64_t ni_fd_write(int32_t fd, ni_slice_0 data) {\n");
        fprintf(out, "    return (int64_t)write(fd, data.data, (size_t)data.len);\n");
        fprintf(out, "}\n");
        fprintf(out, "static int64_t ni_fd_read(int32_t fd, ni_slice_0 buf) {\n");
        fprintf(out, "    return (int64_t)read(fd, buf.data, (size_t)buf.len);\n");
        fprintf(out, "}\n");
        fprintf(out, "static int32_t ni_fd_open(ni_slice_0 path, int32_t flags) {\n");
        fprintf(out, "    char tmp[4096];\n");
        fprintf(out, "    if (path.len >= 4096) { fprintf(stderr, \"fd_open: path too long\\n\"); exit(1); }\n");
        fprintf(out, "    memcpy(tmp, path.data, (size_t)path.len);\n");
        fprintf(out, "    tmp[path.len] = '\\0';\n");
        fprintf(out, "    return (int32_t)open(tmp, flags, 0644);\n");
        fprintf(out, "}\n");
        fprintf(out, "static void ni_fd_close(int32_t fd) { close(fd); }\n");
        fprintf(out, "static int64_t ni_fd_seek(int32_t fd, int64_t offset, int32_t whence) {\n");
        fprintf(out, "    return (int64_t)lseek(fd, (off_t)offset, whence);\n");
        fprintf(out, "}\n\n");
    }

    /* Emit mem_copy runtime helper if used */
    if (g_has_mem) {
        fprintf(out, "static int64_t ni_mem_copy(ni_slice_0 dst, ni_slice_0 src) {\n");
        fprintf(out, "    int64_t n = dst.len < src.len ? dst.len : src.len;\n");
        fprintf(out, "    memmove(dst.data, src.data, (size_t)n);\n");
        fprintf(out, "    return n;\n");
        fprintf(out, "}\n\n");
    }

    /* Emit args runtime helper if used */
    if (g_has_args) {
        Type str_type = slice_table_intern(TYPE_U8);
        Type args_type = slice_table_intern(str_type);
        const char *str_c = codegen_type_to_c(str_type);
        char str_buf[64];
        snprintf(str_buf, sizeof(str_buf), "%s", str_c);
        const char *args_c = codegen_type_to_c(args_type);
        char args_buf[64];
        snprintf(args_buf, sizeof(args_buf), "%s", args_c);
        fprintf(out, "static int ni_argc;\n");
        fprintf(out, "static char **ni_argv;\n");
        fprintf(out, "static %s ni_args(ni_Arena *mem) {\n", args_buf);
        fprintf(out, "    %s *arr = (%s *)ni_arena_alloc(mem, (int64_t)ni_argc, sizeof(%s), NI_ALIGNOF(%s));\n",
                str_buf, str_buf, str_buf, str_buf);
        fprintf(out, "    for (int i = 0; i < ni_argc; i++) {\n");
        fprintf(out, "        size_t len = strlen(ni_argv[i]);\n");
        fprintf(out, "        uint8_t *data = (uint8_t *)ni_arena_alloc(mem, (int64_t)len, 1, NI_ALIGNOF(uint8_t));\n");
        fprintf(out, "        memcpy(data, ni_argv[i], len);\n");
        fprintf(out, "        arr[i] = (%s){.data = data, .len = (int64_t)len};\n", str_buf);
        fprintf(out, "    }\n");
        fprintf(out, "    return (%s){.data = arr, .len = (int64_t)ni_argc};\n", args_buf);
        fprintf(out, "}\n\n");
    }

    if (g_has_time) {
        fprintf(out, "static int64_t ni_time_ns(void) {\n");
        fprintf(out, "    struct timeval tv;\n");
        fprintf(out, "    gettimeofday(&tv, NULL);\n");
        fprintf(out, "    return ((int64_t)tv.tv_sec * 1000000000LL) + ((int64_t)tv.tv_usec * 1000LL);\n");
        fprintf(out, "}\n\n");
    }

    if (g_has_driver) {
        fprintf(out, "static int32_t ni_shell_run(ni_slice_0 command) {\n");
        fprintf(out, "    char *buf = (char *)malloc((size_t)command.len + 1U);\n");
        fprintf(out, "    if (!buf) { fprintf(stderr, \"shell_run: out of memory\\n\"); exit(1); }\n");
        fprintf(out, "    memcpy(buf, command.data, (size_t)command.len);\n");
        fprintf(out, "    buf[command.len] = '\\0';\n");
        fprintf(out, "    int status = system(buf);\n");
        fprintf(out, "    free(buf);\n");
        fprintf(out, "    if (status < 0) return -1;\n");
        fprintf(out, "    if (WIFEXITED(status)) return (int32_t)WEXITSTATUS(status);\n");
        fprintf(out, "    return 1;\n");
        fprintf(out, "}\n");
        fprintf(out, "static int64_t ni_pid(void) {\n");
        fprintf(out, "    return (int64_t)getpid();\n");
        fprintf(out, "}\n");
        fprintf(out, "static ni_slice_0 ni_exe_dir(ni_Arena *mem) {\n");
        fprintf(out, "    const char *src = \".\";\n");
        fprintf(out, "    char real_buf[4096];\n");
        fprintf(out, "    char candidate[4096];\n");
        fprintf(out, "    if (ni_argc > 0 && ni_argv && ni_argv[0] && ni_argv[0][0] != '\\0') {\n");
        fprintf(out, "        src = ni_argv[0];\n");
        fprintf(out, "    }\n");
        fprintf(out, "    if (strchr(src, '/')) {\n");
        fprintf(out, "        if (realpath(src, real_buf)) src = real_buf;\n");
        fprintf(out, "    } else {\n");
        fprintf(out, "        const char *path_env = getenv(\"PATH\");\n");
        fprintf(out, "        size_t name_len = strlen(src);\n");
        fprintf(out, "        if (path_env) {\n");
        fprintf(out, "            const char *part = path_env;\n");
        fprintf(out, "            while (1) {\n");
        fprintf(out, "                const char *end = part;\n");
        fprintf(out, "                while (*end != '\\0' && *end != ':') end++;\n");
        fprintf(out, "                size_t dir_len = (size_t)(end - part);\n");
        fprintf(out, "                size_t copy_len = dir_len == 0 ? 1 : dir_len;\n");
        fprintf(out, "                if ((copy_len + 1 + name_len + 1) <= sizeof(candidate)) {\n");
        fprintf(out, "                    if (dir_len == 0) candidate[0] = '.';\n");
        fprintf(out, "                    else memcpy(candidate, part, dir_len);\n");
        fprintf(out, "                    candidate[copy_len] = '/';\n");
        fprintf(out, "                    memcpy(candidate + copy_len + 1, src, name_len);\n");
        fprintf(out, "                    candidate[copy_len + 1 + name_len] = '\\0';\n");
        fprintf(out, "                    if (access(candidate, X_OK) == 0) {\n");
        fprintf(out, "                        if (realpath(candidate, real_buf)) src = real_buf;\n");
        fprintf(out, "                        else src = candidate;\n");
        fprintf(out, "                        break;\n");
        fprintf(out, "                    }\n");
        fprintf(out, "                }\n");
        fprintf(out, "                if (*end == '\\0') break;\n");
        fprintf(out, "                part = end + 1;\n");
        fprintf(out, "            }\n");
        fprintf(out, "        }\n");
        fprintf(out, "    }\n");
        fprintf(out, "    size_t len = strlen(src);\n");
        fprintf(out, "    size_t slash = len;\n");
        fprintf(out, "    while (slash > 0) {\n");
        fprintf(out, "        if (src[slash - 1] == '/') { slash--; break; }\n");
        fprintf(out, "        slash--;\n");
        fprintf(out, "    }\n");
        fprintf(out, "    if (len > 0 && src[0] == '/' && slash == 0) {\n");
        fprintf(out, "        uint8_t *root = (uint8_t *)ni_arena_alloc(mem, 1, 1, NI_ALIGNOF(uint8_t));\n");
        fprintf(out, "        root[0] = '/';\n");
        fprintf(out, "        return (ni_slice_0){.data = root, .len = 1};\n");
        fprintf(out, "    }\n");
        fprintf(out, "    if (slash == 0) {\n");
        fprintf(out, "        uint8_t *dot = (uint8_t *)ni_arena_alloc(mem, 1, 1, NI_ALIGNOF(uint8_t));\n");
        fprintf(out, "        dot[0] = '.';\n");
        fprintf(out, "        return (ni_slice_0){.data = dot, .len = 1};\n");
        fprintf(out, "    }\n");
        fprintf(out, "    uint8_t *data = (uint8_t *)ni_arena_alloc(mem, (int64_t)slash, 1, NI_ALIGNOF(uint8_t));\n");
        fprintf(out, "    memcpy(data, src, slash);\n");
        fprintf(out, "    return (ni_slice_0){.data = data, .len = (int64_t)slash};\n");
        fprintf(out, "}\n\n");
    }

    /* Emit per-table helper functions */
    for (size_t ti = 0; ti < g_table_decl_count; ti++) {
        TableDeclEntry *te = &g_table_decls[ti];
        /* Copy mangled names to local buffers (rotating buffer in
         * codegen_mangled_name_str gets overwritten by codegen_type_to_c) */
        char tc_buf[128], mname_buf[128];
        snprintf(tc_buf, sizeof(tc_buf), "%s", codegen_type_to_c(te->struct_type));
        snprintf(mname_buf, sizeof(mname_buf), "%s",
                 codegen_mangled_name_str(te->name_start,
                                          te->name_length, te->module_id));
        const char *tc = tc_buf;
        const char *mname = mname_buf;
        ValueTypeEntry *struct_vt = value_table_get(te->struct_type);

        /* ni_table_alloc_Name */
        fprintf(out, "static %s %s__alloc(ni_Arena *a, int64_t cap) {\n", tc, mname);
        fprintf(out, "    return (%s){", tc);
        for (size_t f = 0; f < te->field_count; f++) {
            Type slice_type = struct_vt->fields[f].type;
            Type elem_type = type_element_type(slice_type);
            fprintf(out, "\n        .ni_%.*s = (%s){.data = (%s *)ni_arena_alloc(a, cap, sizeof(%s), NI_ALIGNOF(%s)), .len = cap},",
                    (int)te->fields[f].name_length, te->fields[f].name_start,
                    codegen_type_to_c(slice_type),
                    codegen_type_to_c(elem_type),
                    codegen_type_to_c(elem_type),
                    codegen_type_to_c(elem_type));
        }
        fprintf(out, "\n        .ni__len = 0\n    };\n}\n\n");

        /* ni_table_get_Name */
        char row_buf[128];
        snprintf(row_buf, sizeof(row_buf), "%s", codegen_type_to_c(te->row_type));
        const char *row_c = row_buf;
        fprintf(out, "static %s %s__get(const %s *t, int64_t idx, const char *file, unsigned long line, unsigned long col) {\n",
                row_c, mname, tc);
        fprintf(out, "    if ((uint64_t)idx >= (uint64_t)t->ni__len) {\n");
        fprintf(out, "        ni_bounds_fail(idx, (size_t)t->ni__len, file, line, col);\n");
        fprintf(out, "    }\n");
        fprintf(out, "    return (%s){", row_c);
        for (size_t f = 0; f < te->field_count; f++) {
            fprintf(out, ".ni_%.*s = t->ni_%.*s.data[idx]",
                    (int)te->fields[f].name_length, te->fields[f].name_start,
                    (int)te->fields[f].name_length, te->fields[f].name_start);
            if (f + 1 < te->field_count) fprintf(out, ", ");
        }
        fprintf(out, "};\n}\n\n");

        /* ni_table_insert_Name */
        fprintf(out, "static void %s__insert(%s *t, %s row) {\n",
                mname, tc, row_c);
        /* Use first column's .len as capacity */
        fprintf(out, "    if (t->ni__len >= t->ni_%.*s.len) {\n",
                (int)te->fields[0].name_length, te->fields[0].name_start);
        fprintf(out, "        fprintf(stderr, \"Table capacity exceeded\\n\"); exit(2);\n");
        fprintf(out, "    }\n");
        for (size_t f = 0; f < te->field_count; f++) {
            fprintf(out, "    t->ni_%.*s.data[t->ni__len] = row.ni_%.*s;\n",
                    (int)te->fields[f].name_length, te->fields[f].name_start,
                    (int)te->fields[f].name_length, te->fields[f].name_start);
        }
        fprintf(out, "    t->ni__len++;\n");
        fprintf(out, "}\n\n");
    }

    /* Create global codegen scope and populate it first (comptime values need
     * to be available before any global decl is emitted) */
    Scope *global_codegen_scope = scope_create(NULL);
    g_global_codegen_scope = global_codegen_scope;
    g_codegen_scope = global_codegen_scope;

    /* Inject predefined constants into codegen scope */
    scope_inject_platform_constants(global_codegen_scope);

    for (size_t i = 0; i < ast->as.program.count; i++) {
        Ast *node = ast->as.program.statements[i];
        if (node->kind == AST_VAL_DECL) {
            Type type = node->as.val_decl.type;
            Ast *init = node->as.val_decl.initializer;
            if (type_is_comptime(type) && is_comptime_constant(init, global_codegen_scope)) {
                if (type == TYPE_COMPTIME_INT) {
                    scope_add_comptime_int(global_codegen_scope,
                        node->as.val_decl.name_start, node->as.val_decl.name_length,
                        get_comptime_int(init, global_codegen_scope), node->loc);
                } else {
                    scope_add_comptime_float(global_codegen_scope,
                        node->as.val_decl.name_start, node->as.val_decl.name_length,
                        get_comptime_float(init, global_codegen_scope), node->loc);
                }
            } else {
                scope_add_global(global_codegen_scope,
                    node->as.val_decl.name_start, node->as.val_decl.name_length,
                    false, type, node->loc);
            }
        } else if (node->kind == AST_MUT_DECL) {
            scope_add_global(global_codegen_scope,
                node->as.mut_decl.name_start, node->as.mut_decl.name_length,
                true, node->as.mut_decl.type, node->loc);
        }
    }

    /* Emit global declarations */
    for (size_t i = 0; i < ast->as.program.count; i++) {
        Ast *node = ast->as.program.statements[i];
        if (node->kind == AST_VAL_DECL || node->kind == AST_MUT_DECL) {
            codegen_emit_global_decl(out, node);
        }
    }

    g_codegen_scope = NULL;
    fprintf(out, "\n");

    /* Emit user-function prototypes before any function body so forward calls
     * and mutual recursion work in the generated C. */
    bool emitted_prototypes = false;
    for (size_t i = 0; i < ast->as.program.count; i++) {
        Ast *node = ast->as.program.statements[i];
        if (node->kind == AST_FUNC_DECL) {
            codegen_emit_function_prototype(out, node);
            emitted_prototypes = true;
        }
    }
    if (emitted_prototypes) {
        fprintf(out, "\n");
    }

    /* Emit all functions */
    for (size_t i = 0; i < ast->as.program.count; i++) {
        Ast *node = ast->as.program.statements[i];
        if (node->kind == AST_FUNC_DECL) {
            codegen_emit_function(out, node);
        }
    }

    scope_destroy(global_codegen_scope);
    g_global_codegen_scope = NULL;
}

static void codegen_print_ir(Ast *ast) {
    printf("=== IR (C CODE) ===\n");
    codegen_emit_ir(stdout, ast);
    printf("\n");
}

typedef struct {
    char *data;
    size_t len;
} GeneratedC;

/* Benchmarked codegen needs the emitted C before any file I/O or Clang work. */
static GeneratedC codegen_build_c_memory(Ast *ast) {
    GeneratedC generated = {0};
    FILE *out = open_memstream(&generated.data, &generated.len);
    if (!out) {
        panic(ERR_I002_INTERNAL_ERROR,
              "failed to open generated C buffer: %s", strerror(errno));
    }

    codegen_emit_ir(out, ast);

    if (fclose(out) != 0) {
        free(generated.data);
        panic(ERR_I002_INTERNAL_ERROR,
              "failed to finalize generated C buffer: %s", strerror(errno));
    }

    return generated;
}

/* Generated C buffers are owned by the caller and freed after the tail work. */
static void generated_c_destroy(GeneratedC *generated) {
    free(generated->data);
    generated->data = NULL;
    generated->len = 0;
}

/* Emitting directly to a named file keeps the non-benchmark path simple. */
static void codegen_write_c_file(Ast *ast, const char *output_path) {
    FILE *out = fopen(output_path, "w");
    if (!out) {
        error(ERR_D006_CLANG_FAILED,
              "Could not write generated C file: %s", output_path);
    }

    codegen_emit_ir(out, ast);

    if (fclose(out) != 0) {
        unlink(output_path);
        error(ERR_D006_CLANG_FAILED,
              "Could not write generated C file: %s", output_path);
    }

    if (g_had_error) {
        unlink(output_path);
    }
}

static void codegen_compile_with_clang(const char *c_source_path, const char *binary_path) {
    char command[1024];
    const char *compiler = getenv("CC");
    if (!compiler || compiler[0] == '\0') compiler = "clang";
    int written = snprintf(command, sizeof(command),
                          "%s -std=c99 -O2 -fwrapv -o '%s' '%s' 2>&1",
                          compiler, binary_path, c_source_path);

    if (written < 0 || written >= (int)sizeof(command)) {
        panic(ERR_I002_INTERNAL_ERROR, "command buffer too small");
    }

    int status = system(command);

    if (status != 0) {
        error(ERR_D006_CLANG_FAILED,
                     "Clang compilation failed with exit code %d",
                     WEXITSTATUS(status));
    }
}

/* Writing a generated buffer keeps the benchmarked codegen/tail boundary explicit. */
static void generated_c_write_file(const GeneratedC *generated, const char *output_path) {
    FILE *out = fopen(output_path, "w");
    if (!out) {
        error(ERR_D006_CLANG_FAILED,
              "Could not write generated C file: %s", output_path);
    }

    if (generated->len > 0 &&
        fwrite(generated->data, 1, generated->len, out) != generated->len) {
        fclose(out);
        unlink(output_path);
        error(ERR_D006_CLANG_FAILED,
              "Could not write generated C file: %s", output_path);
    }

    if (fclose(out) != 0) {
        unlink(output_path);
        error(ERR_D006_CLANG_FAILED,
              "Could not write generated C file: %s", output_path);
    }
}

/* Temporary C files must stay bound to the original mkstemps descriptor. */
static void generated_c_write_temp(const GeneratedC *generated,
                                   int fd,
                                   const char *temp_path) {
    FILE *out = fdopen(fd, "w");
    if (!out) {
        close(fd);
        unlink(temp_path);
        panic(ERR_I002_INTERNAL_ERROR,
              "failed to open temporary file: %s", strerror(errno));
    }

    if (generated->len > 0 &&
        fwrite(generated->data, 1, generated->len, out) != generated->len) {
        fclose(out);
        unlink(temp_path);
        error(ERR_D006_CLANG_FAILED,
              "Could not write generated C file: %s", temp_path);
    }

    if (fclose(out) != 0) {
        unlink(temp_path);
        error(ERR_D006_CLANG_FAILED,
              "Could not write generated C file: %s", temp_path);
    }
}

static void codegen_compile(Ast *ast, const char *output_path) {
    /* Create temporary C file */
    char temp_path[] = "/tmp/nore_XXXXXX.c";
    int fd = mkstemps(temp_path, 2);
    if (fd == -1) {
        panic(ERR_I002_INTERNAL_ERROR,
              "failed to create temporary file: %s", strerror(errno));
    }

    GeneratedC generated = codegen_build_c_memory(ast);

    if (!g_had_error) {
        generated_c_write_temp(&generated, fd, temp_path);
    } else {
        close(fd);
        unlink(temp_path);
    }
    generated_c_destroy(&generated);

    /* Don't compile if there were errors */
    if (g_had_error) {
        unlink(temp_path);
        return;
    }

    /* Compile with Clang */
    codegen_compile_with_clang(temp_path, output_path);

    /* Cleanup temporary file */
    unlink(temp_path);
}

/* ================================ Compiler Flags ========================== */

#define NOREC_STAGE0_VERSION_TEXT "norec-stage0 v0.1.2"

typedef struct {
    int print_tokens;
    int print_ast;
    int print_ir;
    int benchmark;
    int run;
    int strip_asserts;
} CompilerFlags;

/* Coarse benchmark timing uses the same wall-clock source as the self-hosted compiler. */
static int64_t compiler_time_ns(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec * 1000000000LL) + ((int64_t)tv.tv_usec * 1000LL);
}

/* Keep the seed benchmark output compatible with the matrix script. */
static void bench_append_field(FILE *out, const char *name, int64_t value) {
    fprintf(out, " %s=%lld", name, (long long)value);
}

/* One compact line is enough for script parsing and hand inspection. */
static void emit_bench_report(const char *mode,
                              int64_t load_ns,
                              int64_t check_ns,
                              int64_t codegen_ns,
                              int64_t tail_ns) {
    int64_t total_ns = load_ns + check_ns + codegen_ns + tail_ns;
    fprintf(stderr, "bench mode=%s", mode);
    bench_append_field(stderr, "load_ns", load_ns);
    bench_append_field(stderr, "check_ns", check_ns);
    bench_append_field(stderr, "codegen_ns", codegen_ns);
    bench_append_field(stderr, "tail_ns", tail_ns);
    bench_append_field(stderr, "total_ns", total_ns);
    fputc('\n', stderr);
}

/* =================================== Main ================================= */

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        puts(NOREC_STAGE0_VERSION_TEXT);
        return 0;
    }

    if (argc < 2) {
        error(ERR_D001_NO_INPUT_FILE,
                     "Usage: %s <file.nore> [--run] [--lexer] [--parser] [--codegen] [--strip-asserts] [-o output]\n"
                     "       %s --emit-c <input.nore> <output.c> [compiler_root]\n"
                     "       %s --version",
                     argv[0],
                     argv[0],
                     argv[0]);
    }

    const char *input_path = NULL;
    const char *output_arg = NULL;
    const char *compiler_root_arg = NULL;
    int emit_c_mode = 0;
    CompilerFlags flags = {0};

    /* Extra arguments after -- (forwarded to program in --run mode) */
    int extra_argc = 0;
    char **extra_argv = NULL;

    if (strcmp(argv[1], "--emit-c") == 0) {
        emit_c_mode = 1;
        if (argc < 4) {
            error(ERR_D001_NO_INPUT_FILE,
                  "Expected input and output paths after --emit-c");
        }
        input_path = argv[2];
        output_arg = argv[3];
        for (int i = 4; i < argc; i++) {
            if (strcmp(argv[i], "--bench") == 0) {
                flags.benchmark = 1;
            } else if (strcmp(argv[i], "--strip-asserts") == 0) {
                flags.strip_asserts = 1;
            } else if (argv[i][0] == '-') {
                error(ERR_D002_UNKNOWN_FLAG, "Unknown flag: %s", argv[i]);
            } else if (!compiler_root_arg) {
                compiler_root_arg = argv[i];
            } else {
                error(ERR_D001_NO_INPUT_FILE,
                      "Expected input and output paths after --emit-c");
            }
        }
    } else {
        /* Parse command-line arguments */
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--") == 0) {
                extra_argc = argc - i - 1;
                extra_argv = &argv[i + 1];
                break;
            } else if (strcmp(argv[i], "--lexer") == 0) {
                flags.print_tokens = 1;
            } else if (strcmp(argv[i], "--parser") == 0) {
                flags.print_ast = 1;
            } else if (strcmp(argv[i], "--codegen") == 0) {
                flags.print_ir = 1;
            } else if (strcmp(argv[i], "--bench") == 0) {
                flags.benchmark = 1;
            } else if (strcmp(argv[i], "--strip-asserts") == 0) {
                flags.strip_asserts = 1;
            } else if (strcmp(argv[i], "--version") == 0) {
                puts(NOREC_STAGE0_VERSION_TEXT);
                return 0;
            } else if (strcmp(argv[i], "--run") == 0) {
                flags.run = 1;
            } else if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 >= argc) {
                    error(ERR_D004_MISSING_OUTPUT_PATH,
                                 "Expected output path after -o");
                }
                output_arg = argv[++i];
            } else if (argv[i][0] == '-') {
                error(ERR_D002_UNKNOWN_FLAG, "Unknown flag: %s", argv[i]);
            } else {
                if (input_path) {
                    error(ERR_D003_MULTIPLE_INPUTS,
                                 "Multiple input files specified");
                }
                input_path = argv[i];
            }
        }
    }

    if (!input_path) {
        error(ERR_D001_NO_INPUT_FILE, "No input file specified");
    }

    /* Set global source file for diagnostics */
    g_source_file = input_path;

    /* Determine compiler directory (for std/ imports) */
    {
        char real_buf[PATH_MAX];
        if (compiler_root_arg) {
            if (realpath(compiler_root_arg, real_buf)) {
                strncpy(g_compiler_dir, real_buf, sizeof(g_compiler_dir) - 1);
            } else {
                strncpy(g_compiler_dir, compiler_root_arg, sizeof(g_compiler_dir) - 1);
            }
            g_compiler_dir[sizeof(g_compiler_dir) - 1] = '\0';
        } else {
            char argv0_buf[PATH_MAX];
            strncpy(argv0_buf, argv[0], sizeof(argv0_buf) - 1);
            argv0_buf[sizeof(argv0_buf) - 1] = '\0';
            /* Try to resolve the real path of the compiler binary */
            if (realpath(argv[0], real_buf)) {
                strncpy(argv0_buf, real_buf, sizeof(argv0_buf) - 1);
                argv0_buf[sizeof(argv0_buf) - 1] = '\0';
            }
            char *dir = dirname(argv0_buf);
            strncpy(g_compiler_dir, dir, sizeof(g_compiler_dir) - 1);
            g_compiler_dir[sizeof(g_compiler_dir) - 1] = '\0';
        }
    }

    /* Add main file to import tracking (so it won't be imported again) */
    {
        char resolved_main[PATH_MAX];
        if (realpath(input_path, resolved_main)) {
            g_imported_files[0] = strdup(resolved_main);
            g_imported_count = 1;
        }
    }

    /* Determine output path */
    char output_path[256];
    if (output_arg) {
        strncpy(output_path, output_arg, sizeof(output_path) - 1);
        output_path[sizeof(output_path) - 1] = '\0';
    } else {
        /* Strip .nore extension or use a.out */
        const char *base = strrchr(input_path, '/');
        base = base ? base + 1 : input_path;
        const char *dot = strrchr(base, '.');
        if (dot && strcmp(dot, ".nore") == 0) {
            snprintf(output_path, sizeof(output_path), "%.*s",
                    (int)(dot - base), base);
        } else {
            snprintf(output_path, sizeof(output_path), "a.out");
        }
    }

    int64_t load_ns = 0;
    int64_t check_ns = 0;
    int64_t codegen_ns = 0;
    int64_t tail_ns = 0;
    int64_t phase_start = 0;
    GeneratedC generated = {0};
    bool generated_ready = false;

    /* Read source file */
    if (flags.benchmark) {
        phase_start = compiler_time_ns();
    }
    size_t length;
    char *source = read_file(input_path, &length);

    Lexer lexer;
    lexer_init(&lexer, source);

    /* --lexer: Print tokens */
    if (flags.print_tokens) {
        lexer_print_tokens(&lexer, source);
    }

    /* Initialize type tables */
    g_value_table = value_table_create();
    g_array_table = array_table_create();
    g_slice_table = slice_table_create();
    /* Register OS enum before parsing (so OS.Linux etc. are available) */
    platform_inject_os_enum();
    /* Parse program */
    Parser parser;
    parser_init(&parser, &lexer);
    Ast *ast = parser_parse_program(&parser);
    if (flags.benchmark) {
        load_ns = compiler_time_ns() - phase_start;
    }

    /* --parser: Print AST */
    if (flags.print_ast) {
        parser_print_ast(ast);
    }

    /* Type checking */
    g_require_main = !emit_c_mode;
    if (flags.benchmark) {
        phase_start = compiler_time_ns();
    }
    typecheck_program(ast);
    if (flags.strip_asserts) {
        validate_assert_stripping_node(ast);
    }
    g_strip_asserts = flags.strip_asserts;
    if (flags.benchmark) {
        check_ns = compiler_time_ns() - phase_start;
    }

    /* --codegen: Print intermediate C code (also runs semantic checks) */
    if (flags.print_ir) {
        codegen_print_ir(ast);
    }

    /* Skip compilation if any debug flag is set */
    int skip_compilation = flags.print_tokens || flags.print_ast || flags.print_ir;

    int run_exit_code = 0;

    if (!skip_compilation && flags.benchmark) {
        phase_start = compiler_time_ns();
        generated = codegen_build_c_memory(ast);
        generated_ready = true;
        codegen_ns = compiler_time_ns() - phase_start;
    }

    if (!skip_compilation) {
        if (emit_c_mode) {
            if (flags.benchmark) {
                if (!g_had_error) {
                    phase_start = compiler_time_ns();
                    generated_c_write_file(&generated, output_path);
                    tail_ns = compiler_time_ns() - phase_start;
                }
            } else {
                codegen_write_c_file(ast, output_path);
            }
        } else if (flags.run) {
            /* Compile to temp binary, run it, clean up */
            char temp_bin[] = "/tmp/nore_run_XXXXXX";
            int fd = mkstemp(temp_bin);
            if (fd == -1) {
                panic(ERR_I002_INTERNAL_ERROR,
                      "failed to create temporary file: %s", strerror(errno));
            }
            close(fd);

            if (flags.benchmark) {
                if (!g_had_error) {
                    char temp_path[] = "/tmp/nore_XXXXXX.c";
                    int cfd = mkstemps(temp_path, 2);
                    if (cfd == -1) {
                        panic(ERR_I002_INTERNAL_ERROR,
                              "failed to create temporary file: %s", strerror(errno));
                    }

                    phase_start = compiler_time_ns();
                    generated_c_write_temp(&generated, cfd, temp_path);
                    codegen_compile_with_clang(temp_path, temp_bin);
                    unlink(temp_path);

                    /* Build argv: [temp_bin, extra_argv..., NULL] */
                    char **run_argv = malloc((size_t)(1 + extra_argc + 1) * sizeof(char *));
                    if (!run_argv) panic(ERR_I001_OUT_OF_MEMORY, "allocating run argv");
                    run_argv[0] = temp_bin;
                    for (int i = 0; i < extra_argc; i++) run_argv[1 + i] = extra_argv[i];
                    run_argv[1 + extra_argc] = NULL;

                    pid_t pid = fork();
                    if (pid < 0) {
                        free(run_argv);
                        panic(ERR_I002_INTERNAL_ERROR, "fork failed: %s", strerror(errno));
                    } else if (pid == 0) {
                        execv(temp_bin, run_argv);
                        _exit(127);
                    }
                    int status;
                    waitpid(pid, &status, 0);
                    tail_ns = compiler_time_ns() - phase_start;
                    free(run_argv);
                    if (WIFEXITED(status)) {
                        run_exit_code = WEXITSTATUS(status);
                    } else {
                        run_exit_code = 1;
                    }
                }
            } else {
                codegen_compile(ast, temp_bin);
            }

            if (!g_had_error) {
                if (!flags.benchmark) {
                    /* Build argv: [temp_bin, extra_argv..., NULL] */
                    char **run_argv = malloc((size_t)(1 + extra_argc + 1) * sizeof(char *));
                    if (!run_argv) panic(ERR_I001_OUT_OF_MEMORY, "allocating run argv");
                    run_argv[0] = temp_bin;
                    for (int i = 0; i < extra_argc; i++) run_argv[1 + i] = extra_argv[i];
                    run_argv[1 + extra_argc] = NULL;

                    pid_t pid = fork();
                    if (pid < 0) {
                        free(run_argv);
                        panic(ERR_I002_INTERNAL_ERROR, "fork failed: %s", strerror(errno));
                    } else if (pid == 0) {
                        execv(temp_bin, run_argv);
                        _exit(127);
                    }
                    int status;
                    waitpid(pid, &status, 0);
                    free(run_argv);
                    if (WIFEXITED(status)) {
                        run_exit_code = WEXITSTATUS(status);
                    } else {
                        run_exit_code = 1;
                    }
                }
            }
            unlink(temp_bin);
        } else {
            /* Generate and compile */
            if (flags.benchmark) {
                if (!g_had_error) {
                    phase_start = compiler_time_ns();
                    char temp_path[] = "/tmp/nore_XXXXXX.c";
                    int fd = mkstemps(temp_path, 2);
                    if (fd == -1) {
                        panic(ERR_I002_INTERNAL_ERROR,
                              "failed to create temporary file: %s", strerror(errno));
                    }

                    generated_c_write_temp(&generated, fd, temp_path);
                    codegen_compile_with_clang(temp_path, output_path);
                    tail_ns = compiler_time_ns() - phase_start;
                    unlink(temp_path);
                }
            } else {
                codegen_compile(ast, output_path);
            }
        }
    }

    if (generated_ready) {
        generated_c_destroy(&generated);
    }

    /* Cleanup */
    ast_free(ast);
    value_table_destroy(g_value_table);
    array_table_destroy(g_array_table);
    slice_table_destroy(g_slice_table);
    for (size_t i = 0; i < g_enum_count; i++) free(g_enum_table[i].variants);
    free(g_enum_table);
    if (g_codegen_func_table) func_table_destroy(g_codegen_func_table);
    free(source);

    /* Report any collected errors (exits with non-zero).
     * Must happen before freeing import paths since errors reference them. */
    if (g_had_error) report_errors_and_exit();

    if (flags.benchmark && !skip_compilation) {
        emit_bench_report(emit_c_mode ? "emit-c" : "driver",
                          load_ns, check_ns, codegen_ns, tail_ns);
    }

    for (size_t i = 0; i < g_imported_count; i++) free(g_imported_files[i]);
    for (size_t i = 0; i < g_import_source_count; i++) free(g_import_sources[i]);

    return run_exit_code;
}
