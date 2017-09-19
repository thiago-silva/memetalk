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
#include <boost/filesystem.hpp>

using namespace std;

static MMLog _log(LOG_UTILS);

void create_cache_dir(std::string& path) {
  boost::filesystem::path dir(path);
  if (boost::filesystem::create_directories(dir)) {
    _log << "cache directory created: "<< path << std::endl;
  }
}

// char* read_first_line(char* filepath, int* file_size) {
//   fstream file;
//   file.open(filepath, fstream::in | fstream::binary);
//   return read_file(file, file_size);
// }

char* read_file(fstream& file, int* file_size) {
  string contents(static_cast<stringstream const&>(stringstream() << file.rdbuf()).str());
  *file_size = file.tellg();

  _log << "file size: "<< *file_size << endl;

  file.close();
  const char* str = contents.c_str();
  char* ret = (char*) GC_MALLOC((sizeof(char) * *file_size)+1);
  return (char*) memcpy(ret, str, (sizeof(char) * *file_size)+1);
}

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
using namespace std;
using namespace boost;
namespace fs = ::boost::filesystem;

// void open_file_in_meme_path(const std::string& filename, fstream & file) {
//   typedef split_iterator<string::iterator> string_split_iterator;
//   std::string mmpath = getenv("MEME_PATH");
//   for(string_split_iterator it = make_split_iterator(mmpath, first_finder(":", is_iequal()));
//       it != string_split_iterator(); ++it) {
//     std::string prefix = copy_range<std::string>(*it);
//     if (prefix.at(prefix.length() - 1) != '/') {
//       prefix += '/';
//     }
//     std::string filepath =  prefix + filename;
//     _log << "trying to open " << filepath << " " << std::endl;

//     file.open(filepath.c_str(), fstream::in | fstream::binary);
//     if (file.good()) {
//       break;
//     }
//   }
//   if (!file.good()) {
//     throw std::invalid_argument(string("file not found: ") + filename);
//   }
// }

char* read_mec_file(const std::string& file_path, int* file_size) {
  fstream file;
  file.open(file_path.c_str(), fstream::in | fstream::binary); //might be an absolute path

  if (!file.is_open()) {
    throw std::invalid_argument(string("could not open file: ") + file_path);
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

number extract_number(Process* proc, oop o) {
  if (is_small_int(o)) {
    return untag_small_int(o);
  } else if (proc->mmobj()->mm_object_vt(o) == proc->vm()->core()->get_prime("LongNum")) {
    return proc->mmobj()->mm_longnum_get(proc, o);
  } else {
    proc->raise("TypeError", "Expecting numeric value");
  }
  return 0; // unreachable
}

bool is_numeric(Process* proc, oop o) {
  return is_small_int(o) || proc->mmobj()->mm_object_vt(o) == proc->vm()->core()->get_prime("LongNum");
}
