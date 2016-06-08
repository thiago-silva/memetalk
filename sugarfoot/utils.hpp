#ifndef UTILS_HPP
#define UTILS_HPP

#include "defs.hpp"
#include <string>

class VM;
class Process;
class CoreImage;

char* read_file(std::fstream& file, int* file_size);
char* read_mmc_file(const std::string& filepath, int* file_size);
word unpack_word(const char* data, int offset);
void relocate_addresses(char* data, int data_size, int start_reloc_table);
void link_symbols(char* data, int es_size, int start_external_symbols, VM* vm, CoreImage* core);

inline int decode_opcode(bytecode code) {
  return (0xFF000000 & code) >> 24;
}

inline int decode_args(bytecode code) {
  return 0xFFFFFF & code;
}

std::string bytecode_to_str(bytecode code);

inline bool is_small_int(oop num) {
#if WSIZE == 8
  return (bool) ((word) num & 0x8000000000000000);
#else
  return (bool) ((word) num & 0x80000000);
#endif
}

inline number untag_small_int(oop num) {
  word w = (word) num;
#if WSIZE == 8
  if (w & 0x4000000000000000) { //Negative
    return (w & 0x7FFFFFFFFFFFFFFF) | 0x8000000000000000;
  } else {
    return w & 0x7FFFFFFFFFFFFFFF;
  }
#else
  if (num & 0x40000000) { //Negative
    return (w & 0x7FFFFFFF) | 0x80000000;
  } else {
    return (w & 0x7FFFFFFF;
  }
#endif
}

inline oop tag_small_int(number num) {
#if WSIZE == 8
  return (oop) (num | 0x8000000000000000);
#else
  return (oop) (num | 0x80000000);
#endif
}

bool check_and_print_exception(Process* proc, int exc, oop ex);

#endif
