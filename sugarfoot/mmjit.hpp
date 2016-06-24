#ifndef MMJIT_HPP
#define MMJIT_HPP

#include "defs.hpp"
#include <boost/unordered_map.hpp>

void* generate_jit_code(bytecode* ip, number size, void* handler, boost::unordered_map<bytecode*, void*>&);
void free_jitcode(void *code);

#endif
