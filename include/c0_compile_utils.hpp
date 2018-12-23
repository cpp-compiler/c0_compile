#ifndef _C0_COMPILE_UTILS_H_
#define _C0_COMPILE_UTILS_H_

typedef int compile_errcode;

#define SYSTEM_ERROR 1000
#define COMPILE_ERROR 2000

#define COMPILE_OK 0
#define NOT_MATCH -1
#define FUNC_NOT_IMPLEMENTED -2
#define BAD_FILE_ERROR                  (-(SYSTEM_ERROR | 1))
#define INVALID_STRING_LENGTH_ERROR     (-(SYSTEM_ERROR | 2))
#define UNKNOWN_CHARACTER_ERROR         (-(COMPILE_ERROR | 1))
#define INVALID_DIGIT_ERROR             (-(COMPILE_ERROR | 2))
#define INVALID_DOUBLE_QUOTE_ERROR      (-(COMPILE_ERROR | 3))
#define INVALID_SINGLE_QUOTE_ERROR      (-(COMPILE_ERROR | 4))
#define INVALID_ARRAY_SIZE_ERROR        (-(COMPILE_ERROR | 5))
#define REPEAT_DEFINITION_IDENTIFIER    (-(COMPILE_ERROR | 6))
#define UNDEFINED_IDENTIFIER_ERROR      (-(COMPILE_ERROR | 7))
#define INVALID_TYPE_ERROR              (-(COMPILE_ERROR | 8))
#define INVALID_ASSIGN_MATCH_ERROR      (-(COMPILE_ERROR | 9))
#define UNDEFINED_SYMBOL_TERM           (-(COMPILE_ERROR | 10))
#define SEMANTIC_ERROR                  (-(COMPILE_ERROR | 11))
#endif // _C0_COMPILE_UTILS_H_
