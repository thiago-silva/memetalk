#ifndef MMJIT_HPP
#define MMJIT_HPP
#include "defs.hpp"

class Process;
class MMObj;

void* gnerate_code_for(bytecode* ip, number size, Process* proc, MMObj* mmobj);

#endif
