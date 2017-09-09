#define  WSIZE 8

#if WSIZE == 4
  typedef unsigned int word;
  typedef int number;
#elif WSIZE == 8
  typedef unsigned long word;
  typedef long number;
#endif

typedef char byte;
typedef word* oop;

typedef int bytecode;

class Process;
typedef int (*prim_function_t) (Process*);

#define DEFAULT_STACK_SIZE sizeof(word) * 1024 * 1024

#define EXCEPTION_FRAME_SIZE 3

#define INVALID_PAYLOAD 256

#define PRIM_RAISED 1
#define PRIM_HALTED 2

#define MM_TRUE (oop) 1
#define MM_FALSE (oop) 2
#define MM_NULL (oop) 0

#define MM_INT_MAX        0x3fffffffffffffff
#define MM_INT_MIN (long) 0xc000000000000000 // -4611686018427387904
// bytecodes

//#define PUSH_PARAM 1
#define PUSH_LOCAL 2
#define PUSH_LITERAL 3
#define PUSH_FIELD 4

#define PUSH_THIS 6
#define PUSH_MODULE 7
#define PUSH_BIN 8
#define PUSH_FP 9
#define PUSH_CONTEXT 10

#define POP_LOCAL 21
#define POP_FIELD 22
#define POP 24

#define RETURN_TOP 31
#define RETURN_THIS 30

#define SEND 40
#define CALL 41
#define SUPER_SEND 42
#define SUPER_CTOR_SEND 43

#define JZ 50
#define JMP 51
#define JMPB 52

//
#define EX_SEND_LIMIT  0

#define EX_EQUAL 53
#define EX_OBJECT_SEND 54
#define EX_DICTIONARY_INDEX 55
#define EX_DICTIONARY_SET 56
#define EX_OBJECT_NOT 57
#define EX_STRING_CONCAT 58
#define EX_DICTIONARY_HAS 59
#define EX_NUMERIC_AS_CHAR 60
#define EX_NUMERIC_SUM 61
#define EX_NUMERIC_MUL 62
#define EX_LIST_EACH 63
//
#define OO_OBJECT_LEN 2 //vt, delegate
#define OO_MODULE_LEN 4 //vt, delegate, dict, cmod
#define OO_LIST_LEN 4   //vt, delegate, size, elements frame
#define OO_DICT_LEN 4   //vt, delegate, size, frame
#define OO_FUN_LEN 4    //vt, delegate, cfun, module
#define OO_CLASS_BEHAVIOR_LEN 4
#define OO_CLASS_LEN 5
#define OO_CFUN_LEN 28
#define OO_SYMBOL_LEN 3
