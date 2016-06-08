#include "utils.hpp"
#include <fstream>
#include <sstream>
#include "vm.hpp"
#include "process.hpp"
#include "mmobj.hpp"
#include "core_image.hpp"
#include "log.hpp"
#include <stdexcept>
#include <sstream>
#include <stdlib.h>
#include <string.h>

using namespace std;

static MMLog _log(LOG_UTILS);

char* read_file(fstream& file, int* file_size) {
  string contents(static_cast<stringstream const&>(stringstream() << file.rdbuf()).str());
  *file_size = file.tellg();

  _log << "file size: "<< *file_size << endl;

  file.close();
  const char* str = contents.c_str();
  char* ret = (char*) malloc((sizeof(char) * *file_size)+1);
  return (char*) memcpy(ret, str, (sizeof(char) * *file_size)+1);
}

char* read_mmc_file(const std::string& name_or_path, int* file_size) {
  fstream file;

  std::string filepath = name_or_path;

  _log << "read_mmc_file: " << filepath << endl;
  if (filepath[0] == '.' || filepath[0] == '/') { //relative or absolute path
    file.open(filepath.c_str(), fstream::in | fstream::binary);
  } else { //filename
    if (filepath.substr(filepath.find_last_of(".") + 1) != "img" &&
        filepath.substr(filepath.find_last_of(".") + 1) != "mmc") {
      filepath = filepath + ".mmc";
    }
    file.open(filepath.c_str(), fstream::in | fstream::binary);
    if (!file.good()) {
      char* mmpath = getenv("MEME_PATH");
      std::string base = mmpath == NULL ? "" : mmpath;
      _log << "failed to open " << name_or_path <<
        " trying " << (base + filepath) << endl;
      file.open((base + filepath).c_str(), fstream::in | fstream::binary);
      if (!file.good()) {
        throw std::invalid_argument(string("Unable to open file: ") + filepath);
      }
    }
  }

  if (!file.is_open()) {
    throw std::invalid_argument(string("file not found: ") + filepath);
  }
  return read_file(file, file_size);
}


word unpack_word(const char* data, int offset) {
  // assert((offset+WSIZE-1) < _data_size);
  return *((word*) &(data[offset]));
}

void relocate_addresses(char* data, int data_size, int start_reloc_table) {
  const char* base = data;

  for (int i = start_reloc_table; i < data_size; i += WSIZE) {
    word target = unpack_word(data,  i);
    word local_ptr = unpack_word(data,  target);
    // _log << (oop) target << "->" << (oop) (base + local_ptr) << endl;
    // write_word(data, target, (word) (base + local_ptr));
    * (word*) &(data[target]) = (word) (base + local_ptr);
  }
}


void link_symbols(char* data, int es_size, int start_external_symbols, VM* vm, CoreImage*) {
  const char* base = data;

  for (int i = 0; i < es_size; i += (2 * WSIZE)) {
    word name_offset = unpack_word(data, start_external_symbols + i);
    char* name = (char*) (base + name_offset);
    word obj_offset = unpack_word(data, start_external_symbols + i + WSIZE);
    word* obj = (word*) (base + obj_offset);
    _log << "Symbol: " << (oop) obj_offset << " - " << (oop) *obj << " [" << name << "] " << endl;
    * obj = (word) vm->new_symbol(name);
    // _log << "offset: " << (oop) obj_offset << " - obj: " << (oop) *obj
    //         << " [" << name << "] -> " << " vt: " << * (oop*) *obj << " == " << core->get_prime("Symbol") << endl;
  }
}




bool check_and_print_exception(Process* proc, int exc, oop ex) {
  if (exc != 0) {
    oop oo_exc = proc->send_0(ex, proc->vm()->new_symbol("toString"), &exc);
    std::cerr << "Exception raised: " << proc->mmobj()->mm_string_cstr(proc, oo_exc) << endl;
    return true;
  }
  return false;
}


std::string bytecode_to_str(bytecode code) {
  std::stringstream s;
  switch(decode_opcode(code)) {
    case PUSH_LOCAL:
      s <<"PUSH_LOCAL" << decode_args(code);
      break;
    case PUSH_LITERAL:
      s <<"PUSH_LITERAL"<< decode_args(code);
      break;
    case PUSH_FIELD:
      s <<"PUSH_FIELD"<< decode_args(code);
      break;
    case PUSH_THIS:
      s <<"PUSH_THIS"<< decode_args(code);
      break;
    case PUSH_MODULE:
      s <<"PUSH_MODULE"<< decode_args(code);
      break;
    case PUSH_BIN:
      s <<"PUSH_BIN"<< decode_args(code);
      break;
    case PUSH_FP:
      s <<"PUSH_FP"<< decode_args(code);
      break;
    case PUSH_CONTEXT:
      s <<"PUSH_CONTEXT"<< decode_args(code);
      break;
    case POP_LOCAL:
      s <<"POP_LOCAL"<< decode_args(code);
      break;
    case POP_FIELD:
      s <<"POP_FIELD"<< decode_args(code);
      break;
    case POP:
      s <<"POP"<< decode_args(code);
      break;
    case RETURN_TOP:
      s <<"RETURN_TOP"<< decode_args(code);
      break;
    case RETURN_THIS:
      s <<"RETURN_THIS"<< decode_args(code);
      break;
    case SEND:
      s <<"SEND"<< decode_args(code);
      break;
    case CALL:
      s <<"CALL"<< decode_args(code);
      break;
    case SUPER_SEND:
      s <<"SUPER_SEND"<< decode_args(code);
      break;
    case SUPER_CTOR_SEND:
      s <<"SUPER_CTOR_SEND"<< decode_args(code);
      break;
    case JZ:
      s <<"JZ"<< decode_args(code);
      break;
    case JMP:
      s <<"JMP"<< decode_args(code);
      break;
    case JMPB:
      s <<"JMPB"<< decode_args(code);
      break;
    default:
      std::cerr << "unknown code " << code << endl;
  }
  return s.str();
}

