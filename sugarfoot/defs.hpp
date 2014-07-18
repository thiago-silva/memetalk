#define  WSIZE 8

#if WSIZE == 4
  typedef int word;
  typedef int number;
#elif WSIZE == 8
  typedef long word;
  typedef long number;
#endif

typedef char byte;
typedef word* oop;

typedef int bytecode;

class Process;
typedef int (*prim_function_t) (Process*);

#define DEFAULT_STACK_SIZE sizeof(word) * 1024 * 10

#define INVALID_PAYLOAD 256

#define PRIM_RAISED 1

// bytecodes

#define PUSH_PARAM 1
#define PUSH_LOCAL 2
#define PUSH_LITERAL 3
#define PUSH_FIELD 4
#define PUSH_THIS 6
#define PUSH_MODULE 7

#define POP_LOCAL 21
#define POP_FIELD 22
#define POP 24

#define RETURN_TOP 31
#define RETURN_THIS 30

#define SEND 40
#define SUPER_SEND 42
#define SUPER_CTOR_SEND 43

#define JZ 50
#define JMP 51
