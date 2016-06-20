#ifndef MMJIT_HPP
#define MMJIT_HPP
#include "defs.hpp"

void* gnerate_code(bytecode* ip, number size, void* handler);
void free_jitcode(void *code);

#endif
