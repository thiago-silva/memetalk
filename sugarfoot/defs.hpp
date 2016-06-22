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

#define JIT_HANDLER_RETURN_TOP 1
#define JIT_HANDLER_RETURN_THIS 2
#define JIT_HANDLER_SEND 3
#define JIT_HANDLER_SUPER_SEND 4
#define JIT_HANDLER_SUPER_CTOR_SEND 5
#define JIT_HANDLER_CALL 6
#define JIT_HANDLER_DBG 10
