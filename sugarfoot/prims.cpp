#include "prims.hpp"
#include "vm.hpp"
#include "defs.hpp"
#include "process.hpp"
#include "utils.hpp"
#include "mmobj.hpp"
#include "mmc_image.hpp"
#include "qt_prims.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>

namespace fs = ::boost::filesystem;

#define DBG(...) if(_log._enabled) { _log << _log.yellow + _log.bold + "[prim|" << __FUNCTION__ << "] " << _log.normal << __VA_ARGS__; }

#define WARNING() MMLog::warning() << "[prim|" << __FUNCTION__ << "] " << _log.normal
#define ERROR() MMLog::error() << "[prim|" << __FUNCTION__ << "] " << _log.normal


static MMLog _log(LOG_PRIMS);

static int prim_remote_repl_compile_module(Process* proc) {
  oop module_name = proc->get_arg(0);
  char* mmpath = getenv("MEME_PATH");
  std::stringstream s;
  s << "python -m pycompiler.compiler  "<< mmpath << proc->mmobj()->mm_string_cstr(proc, module_name) << ".mm";
  DBG("Executing ... " << s.str() << std::endl);
  if (system(s.str().c_str()) == 0) {
    proc->stack_push(MM_TRUE);
  } else {
    proc->stack_push(MM_FALSE);
  }
  return 0;
}

static int prim_remote_repl_instantiate_module(Process* proc) {
  oop oop_module_name = proc->get_arg(0);
  char* module_name = proc->mmobj()->mm_string_cstr(proc, oop_module_name);
  try {
    DBG("instantiating module" << module_name << endl);

    oop imod = proc->vm()->instantiate_module(proc, module_name,
                                              proc->mmobj()->mm_list_new());
    proc->stack_push(imod);
    return 0;
  } catch(std::invalid_argument e) {
    DBG("instantiating module failed: " << e.what() << endl);
    oop ex = proc->mm_exception(
      "ImportError",
      (std::string("could not import module ") + e.what()).c_str());
    DBG("returning mm_exception : " << ex << endl);
    proc->stack_push(ex);
    return PRIM_RAISED;
  }
}


static int prim_io_print(Process* proc) {
  oop obj = proc->get_arg(0);

  DBG("---- prim_print" << endl);
  int exc;
  oop res = proc->send_0(obj, proc->vm()->new_symbol("toString"), &exc);
  if (exc != 0) {
    proc->stack_push(res);
    return PRIM_RAISED;
  }
  std::cout << proc->mmobj()->mm_string_cstr(proc, res) << endl;
  proc->stack_push(MM_NULL);
  return 0;
}

static int prim_io_read_file(Process* proc) {
  oop oop_path = proc->get_arg(0);
  char* path = proc->mmobj()->mm_string_cstr(proc, oop_path);

  std::fstream file;
  file.open(path, std::fstream::in | std::fstream::binary);
  int fsize;
  char* text = read_file(file, &fsize);
  proc->stack_push(proc->mmobj()->mm_string_new(text));
  return 0;
}

static int prim_io_write_file(Process* proc) {
  oop oop_path = proc->get_arg(0);
  oop oop_text = proc->get_arg(1);
  char* path = proc->mmobj()->mm_string_cstr(proc, oop_path);
  char* text = proc->mmobj()->mm_string_cstr(proc, oop_text);

  std::fstream file;
  file.open(path, std::fstream::out | std::fstream::binary);
  file << text;
  file.close();
  proc->stack_push(proc->mp());
  return 0;
}

static int prim_string_append(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  char* str_1 = proc->mmobj()->mm_string_cstr(proc, self);
  char* str_2 = proc->mmobj()->mm_string_cstr(proc, other);

  std::stringstream s;
  s << str_1 << str_2;
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_string_to_integer(Process* proc) {
  oop self =  proc->dp();

  char* str_1 = proc->mmobj()->mm_string_cstr(proc, self);
  number s = std::atol(str_1); //TODO: check bounds
  proc->stack_push(tag_small_int(s));
  return 0;
}

static int prim_string_equal(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  char* str_1 = proc->mmobj()->mm_string_cstr(proc, self);
  char* str_2 = proc->mmobj()->mm_string_cstr(proc, other);

  if (strcmp(str_1, str_2) == 0) {
    proc->stack_push(MM_TRUE);
  } else {
    proc->stack_push(MM_FALSE);
  }
  return 0;
}

static int prim_string_size(Process* proc) {
  oop self =  proc->dp();
  char* str_1 = proc->mmobj()->mm_string_cstr(proc, self);
  int len = strlen(str_1);
  proc->stack_push(tag_small_int(len));
  return 0;
}

static int prim_string_count(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  char* str_1 = proc->mmobj()->mm_string_cstr(proc, self);
  char* str_2 = proc->mmobj()->mm_string_cstr(proc, other);
  int count = 0;
  char* pos = str_1;
  while ((pos = strstr(pos, str_2)) != NULL) {
    count++;
    pos++;
  }
  proc->stack_push(tag_small_int(count));
  return 0;
}

static int prim_string_find(Process* proc) {
  oop self =  proc->dp();
  oop arg = proc->get_arg(0);
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);
  std::string str_arg = proc->mmobj()->mm_string_cstr(proc, arg);
  std::size_t pos = str.find(str_arg);
  if (pos == std::string::npos) {
    proc->stack_push(tag_small_int(-1));
  } else {
    proc->stack_push(tag_small_int(pos));
  }
  return 0;
}

static int prim_string_rindex(Process* proc) {
  oop self =  proc->dp();
  oop arg = proc->get_arg(0);
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);
  std::string str_arg = proc->mmobj()->mm_string_cstr(proc, arg);
  std::size_t pos = str.rfind(str_arg);
  if (pos == std::string::npos) {
    proc->stack_push(tag_small_int(-1));
  } else {
    proc->stack_push(tag_small_int(pos));
  }
  return 0;
}

static int prim_string_index(Process* proc) {
  oop self =  proc->dp();
  oop arg = proc->get_arg(0);
  number idx = untag_small_int(arg);

  std::string str = proc->mmobj()->mm_string_cstr(proc, self);
  char res[2];
  res[0] = str[idx];
  res[1] = '\0';
  proc->stack_push(proc->mmobj()->mm_string_new(res));
  return 0;
}

static int prim_string_from(Process* proc) {
  oop self =  proc->dp();
  oop idx = proc->get_arg(0);
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);
  std::string sub = str.substr(untag_small_int(idx));
  proc->stack_push(proc->mmobj()->mm_string_new(sub.c_str()));
  return 0;
}

static int prim_string_substr(Process* proc) {
  oop self =  proc->dp();
  oop from = proc->get_arg(0);
  oop max = proc->get_arg(1);
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);
  std::string sub = str.substr(untag_small_int(from), untag_small_int(max));
  proc->stack_push(proc->mmobj()->mm_string_new(sub.c_str()));
  return 0;
}


#include <boost/algorithm/string/replace.hpp>
static int prim_string_replace_all(Process* proc) {
  oop self =  proc->dp();
  oop what = proc->get_arg(0);
  oop val = proc->get_arg(1);

  std::string str = proc->mmobj()->mm_string_cstr(proc, self);
  std::string _what = proc->mmobj()->mm_string_cstr(proc, what);
  std::string _val = proc->mmobj()->mm_string_cstr(proc, val);

  std::string output = boost::replace_all_copy(str, _what, _val);

  proc->stack_push(proc->mmobj()->mm_string_new(output.c_str()));
  return 0;
}

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";
static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}


static std::string base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

static int prim_string_split(Process* proc) {
  oop self =  proc->dp();
  oop sep = proc->get_arg(0);

  std::string sep_str = proc->mmobj()->mm_string_cstr(proc, sep);
  std::string self_str = proc->mmobj()->mm_string_cstr(proc, self);

  oop ret = proc->mmobj()->mm_list_new();

  if (sep_str.size() == 0) {
    for (number i = 0; i < self_str.size(); i++) {
      std::stringstream s;
      s << self_str[i];
      oop chr = proc->mmobj()->mm_string_new(s.str().c_str());
      proc->mmobj()->mm_list_append(proc, ret, chr);
    }
  } else {
    std::size_t prev_pos = 0;
    std::size_t pos = self_str.find(sep_str);
    DBG("string: " << self_str << " split: " << sep_str << " pos: " << pos << endl);
    if (pos == std::string::npos) {
      proc->mmobj()->mm_list_append(proc, ret, self);
    } else {
      std::size_t pos, prev_pos = 0;
      while ((pos = self_str.find(sep_str, prev_pos)) != std::string::npos) {
        std::string str1 = self_str.substr(prev_pos, pos);
        DBG("string: " << self_str << " str1: '" << str1 << "'" << endl);
        oop oop_str1 = proc->mmobj()->mm_string_new(str1.c_str());
        proc->mmobj()->mm_list_append(proc, ret, oop_str1);

        prev_pos = pos + 1;
      }
      std::string str1 = self_str.substr(prev_pos);
      DBG("string: " << self_str << " str1: '" << str1 << "'" << endl);
      oop oop_str1 = proc->mmobj()->mm_string_new(str1.c_str());
      proc->mmobj()->mm_list_append(proc, ret, oop_str1);
    }
  }
  proc->stack_push(ret);
  return 0;
}

static int prim_string_to_symbol(Process* proc) {
  oop self =  proc->dp();
  proc->stack_push(proc->vm()->new_symbol(proc, self));
  return 0;
}

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

static int prim_string_trim(Process* proc) {
  oop self =  proc->dp();
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);
  std::string res = trim(str);
  oop oop_res = proc->mmobj()->mm_string_new(res.c_str());
  proc->stack_push(oop_res);
  return 0;
}

static int prim_string_each(Process* proc) {
  oop self =  proc->dp();
  oop fun = proc->get_arg(0);

  // DBG("prim_list_each: closure is: " << fun << endl);
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);

  for (int i = 0; i < str.size(); i++) {
    std::stringstream s;
    s << str[i];
    oop next = proc->mmobj()->mm_string_new(s.str().c_str());
    DBG("string each[" << i << "] = " << next << endl);
    proc->stack_push(tag_small_int(i));
    proc->stack_push(next);
    int exc;
    //TODO: we are not checking arity!
    oop val = proc->do_call(fun, &exc);
    if (exc != 0) {
      DBG("prim_string_each raised" << endl);
      proc->stack_push(val);
      return PRIM_RAISED;
    }
    DBG("string each[" << i << "] fun returned " << val << endl);
  }
  proc->stack_push(self);
  return 0;
}

static int prim_string_b64decode(Process* proc) {
  oop self =  proc->dp();
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);

  DBG("decode: [" << str << "] " << str.size() << endl);
  std::string dec = base64_decode(str);
  DBG("decoded: [" << dec << "]" << endl);

  oop oop_res = proc->mmobj()->mm_string_new(dec.c_str());
  proc->stack_push(oop_res);
  return 0;
}

static int prim_string_b64encode(Process* proc) {
  oop self =  proc->dp();
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);

  DBG("encode: [" << str << "] " << str.size() << endl);

  std::string enc = base64_encode(reinterpret_cast<const unsigned char*>(str.c_str()), str.length());
  DBG("encoded: [" << enc << "]" << endl);

  oop oop_res = proc->mmobj()->mm_string_new(enc.c_str());
  proc->stack_push(oop_res);
  return 0;
}

static int prim_string_only_spaces(Process* proc) {
  oop self =  proc->dp();
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);
  for (int i = 0; i < str.size(); i++) {
    if (str[i] != ' ' &&
        str[i] != '\t' &&
        str[i] != '\r' &&
        str[i] != '\n') {
      proc->stack_push(MM_FALSE);
      return 0;
    }
  }
  proc->stack_push(MM_TRUE);
  return 0;
}

static int prim_string_only_digits(Process* proc) {
  oop self =  proc->dp();
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);
  for (int i = 0; i < str.size(); i++) {
    if (str[i] < '0' || str[i] > '9') {
      proc->stack_push(MM_FALSE);
      return 0;
    }
  }
  proc->stack_push(MM_TRUE);
  return 0;
}

static int prim_string_is_lower(Process* proc) {
  oop self =  proc->dp();
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);
  for (int i = 0; i < str.size(); i++) {
    if (str[i] < 'a' || str[i] > 'z') {
      proc->stack_push(MM_FALSE);
      return 0;
    }
  }
  proc->stack_push(MM_TRUE);
  return 0;
}

static int prim_string_is_upper(Process* proc) {
  oop self =  proc->dp();
  std::string str = proc->mmobj()->mm_string_cstr(proc, self);
  for (int i = 0; i < str.size(); i++) {
    DBG((str[i] < 'A') << (str[i] > 'Z') << endl);
    if (str[i] < 'A' || str[i] > 'Z') {
      proc->stack_push(MM_FALSE);
      return 0;
    }
  }
  proc->stack_push(MM_TRUE);
  return 0;
}


static int prim_number_sum(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  if (!(is_small_int(self))) {
    proc->raise("TypeError", "Expectng small int");
  }

  if (!(is_small_int(other))) {
    proc->raise("TypeError", "Expectng small int");
  }

  number res = untag_small_int(self) + untag_small_int(other);
  proc->stack_push((oop) tag_small_int(res)); //TODO: check for overflow
  return 0;
}

static int prim_number_sub(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  if (!(is_small_int(self))) {
    proc->raise("TypeError", "Expectng small int");
  }

  if (!(is_small_int(other))) {
    proc->raise("TypeError", "Expectng small int");
  }

  number res =  untag_small_int(self) - untag_small_int(other);
  DBG(" SUB " << untag_small_int(self) << " - " << untag_small_int(other) << " = " << res << endl);
  proc->stack_push((oop) tag_small_int(res));
  return 0;
}

static int prim_number_mul(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  if (!(is_small_int(self))) {
    proc->raise("TypeError", "Expectng small int");
  }

  if (!(is_small_int(other))) {
    proc->raise("TypeError", "Expectng small int");
  }

  number res =  untag_small_int(self) * untag_small_int(other);
  proc->stack_push((oop) tag_small_int(res));  //TODO: check for overflow
  return 0;
}

static int prim_number_lt(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  if (!(is_small_int(self))) {
    proc->raise("TypeError", "Expectng small int");
  }

  if (!(is_small_int(other))) {
    proc->raise("TypeError", "Expectng small int");
  }

  oop res =  (oop) (untag_small_int(self) < untag_small_int(other));
  DBG(" PRIM< " << untag_small_int(self) << " < " << untag_small_int(other) << " = " << (untag_small_int(self) < untag_small_int(other)) << endl);
  proc->stack_push((oop)res);
  return 0;
}

static int prim_number_gt(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  if (!(is_small_int(self))) {
    proc->raise("TypeError", "Expectng small int");
  }

  if (!(is_small_int(other))) {
    proc->raise("TypeError", "Expectng small int");
  }

  oop res =  (oop) (untag_small_int(self) > untag_small_int(other));
  DBG(" PRIM< " << untag_small_int(self) << " > " << untag_small_int(other) << " = " << (untag_small_int(self) < untag_small_int(other)) << endl);
  proc->stack_push((oop)res);
  return 0;
}

static int prim_number_gteq(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  if (!(is_small_int(self))) {
    proc->raise("TypeError", "Expectng small int");
  }

  if (!(is_small_int(other))) {
    proc->raise("TypeError", "Expectng small int");
  }

  oop res =  (oop) (untag_small_int(self) >= untag_small_int(other));
  DBG(" PRIM< " << untag_small_int(self) << " >= " << untag_small_int(other) << " = " << (untag_small_int(self) < untag_small_int(other)) << endl);
  proc->stack_push((oop)res);
  return 0;
}

static int prim_number_to_string(Process* proc) {
  oop self =  proc->dp();
  std::stringstream s;
  s << untag_small_int(self);
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_number_to_source(Process* proc) {
  return prim_number_to_string(proc);
}

static int prim_exception_throw(Process* proc) {
  oop self =  proc->rp();
  proc->stack_push(self);
  return PRIM_RAISED;
}

static int prim_list_new(Process* proc) {
  oop self = proc->mmobj()->mm_list_new();

  DBG(self << endl);
  proc->stack_push(self);
  return 0;
}

static int prim_list_append(Process* proc) {
  oop self =  proc->dp();
  oop element = proc->get_arg(0);

  proc->mmobj()->mm_list_append(proc, self, element);

  proc->stack_push(self);
  return 0;
}

static int prim_list_prepend(Process* proc) {
  oop self =  proc->dp();
  oop element = proc->get_arg(0);

  // DBG("LIST: prepend " << element << endl);
  proc->mmobj()->mm_list_prepend(proc, self, element);
  proc->stack_push(self);
  return 0;
}

static int prim_list_index(Process* proc) {
  oop self =  proc->dp();
  oop index_oop = proc->get_arg(0);
  number index = untag_small_int(index_oop);

  oop val = proc->mmobj()->mm_list_entry(proc, self, index);
  DBG("list " << self << "[" << index << "] = " << val << endl);
  proc->stack_push(val);
  return 0;
}

static int prim_list_each(Process* proc) {
  oop self =  proc->dp();
  oop fun = proc->get_arg(0);

  // DBG("prim_list_each: closure is: " << fun << endl);
  number size = proc->mmobj()->mm_list_size(proc, self);

  for (int i = 0; i < size; i++) {
    oop next = proc->mmobj()->mm_list_entry(proc, self, i);
    DBG("list each[" << i << "] = " << next << endl);
    proc->stack_push(tag_small_int(i));
    proc->stack_push(next);
    int exc;
    //TODO: we are not checking arity!
    oop val = proc->do_call(fun, &exc);
    if (exc != 0) {
      DBG("prim_list_each raised" << endl);
      proc->stack_push(val);
      return PRIM_RAISED;
    }
    DBG("list each[" << i << "] fun returned " << val << endl);
  }
  proc->stack_push(self);
  return 0;
}

static int prim_list_map(Process* proc) {
  oop self =  proc->dp();
  oop fun = proc->get_arg(0);

  number size = proc->mmobj()->mm_list_size(proc, self);
  oop ret = proc->mmobj()->mm_list_new();
  for (int i = 0; i < size; i++) {
    oop next = proc->mmobj()->mm_list_entry(proc, self, i);
    DBG("map[" << i << "] = " << next << endl);
    proc->stack_push(next);
    int exc;
    //TODO: we are not checking arity!
    oop val = proc->do_call(fun, &exc);
    if (exc != 0) {
      DBG("raised" << endl);
      proc->stack_push(val);
      return PRIM_RAISED;
    }
    DBG("list map[" << i << "] fun returned " << val << endl);
    proc->mmobj()->mm_list_append(proc, ret, val);
  }
  proc->stack_push(ret);
  return 0;
}

static int prim_list_filter(Process* proc) {
  oop self =  proc->dp();
  oop fun = proc->get_arg(0);

  number size = proc->mmobj()->mm_list_size(proc, self);
  oop ret = proc->mmobj()->mm_list_new();
  for (int i = 0; i < size; i++) {
    oop next = proc->mmobj()->mm_list_entry(proc, self, i);
    DBG("filter[" << i << "] = " << next << endl);
    proc->stack_push(next);
    int exc;
    //TODO: we are not checking arity!
    oop val = proc->do_call(fun, &exc);
    if (exc != 0) {
      DBG("raised" << endl);
      proc->stack_push(val);
      return PRIM_RAISED;
    }
    DBG("filter[" << i << "] fun returned " << val << endl);
    if (val != MM_NULL && val != MM_FALSE) {
      proc->mmobj()->mm_list_append(proc, ret, next);
    }
  }
  proc->stack_push(ret);
  return 0;
}

static int prim_list_detect(Process* proc) {
  oop self =  proc->dp();
  oop fun = proc->get_arg(0);

  number size = proc->mmobj()->mm_list_size(proc, self);
  for (int i = 0; i < size; i++) {
    oop next = proc->mmobj()->mm_list_entry(proc, self, i);
    DBG("detect[" << i << "] = " << next << endl);
    proc->stack_push(next);
    int exc;
    //TODO: we are not checking arity!
    oop val = proc->do_call(fun, &exc);
    if (exc != 0) {
      DBG("raised" << endl);
      proc->stack_push(val);
      return PRIM_RAISED;
    }
    DBG("detect[" << i << "] fun returned " << val << endl);
    if (val != MM_NULL && val != MM_FALSE) {
      proc->stack_push(next);
      return 0;
    }
  }
  proc->stack_push(MM_NULL);
  return 0;
}



static int prim_list_size(Process* proc) {
  oop self =  proc->dp();
  proc->stack_push(tag_small_int(proc->mmobj()->mm_list_size(proc, self)));
  return 0;
}

static int prim_list_reverse(Process* proc) {
  oop self =  proc->dp();

  number size = proc->mmobj()->mm_list_size(proc, self);
  oop ret = proc->mmobj()->mm_list_new();
  for (int i = 0; i < size; i++) {
    oop next = proc->mmobj()->mm_list_entry(proc, self, i);
    proc->mmobj()->mm_list_prepend(proc, ret, next);
  }
  proc->stack_push(ret);
  return 0;
}

static int prim_list_join(Process* proc) {
  oop self =  proc->dp();
  oop sep = proc->get_arg(0);

  char* str_sep = "";

  number size = proc->mmobj()->mm_list_size(proc, self);

  std::stringstream s;
  for (int i = 0; i < size; i++) {
    oop next = proc->mmobj()->mm_list_entry(proc, self, i);
    s << str_sep << proc->mmobj()->mm_string_cstr(proc, next);
    str_sep = proc->mmobj()->mm_string_cstr(proc, sep);
  }
  oop res = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(res);
  return 0;
}

static int prim_list_plus(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  number this_size = proc->mmobj()->mm_list_size(proc, self);
  number other_size = proc->mmobj()->mm_list_size(proc, other);

  oop res = proc->mmobj()->mm_list_new();
  std::stringstream s;
  for (int i = 0; i < this_size; i++) {
    oop next = proc->mmobj()->mm_list_entry(proc, self, i);
    proc->mmobj()->mm_list_append(proc, res, next);
  }
  for (int i = 0; i < other_size; i++) {
    oop next = proc->mmobj()->mm_list_entry(proc, other, i);
    proc->mmobj()->mm_list_append(proc, res, next);
  }
  proc->stack_push(res);
  return 0;
}

static int prim_list_has(Process* proc) {
  oop self =  proc->dp();
  oop value = proc->get_arg(0);

  if (proc->mmobj()->mm_list_index_of(proc, self, value) != -1) {
    proc->stack_push(MM_TRUE);
    return 0;
  }

  for (number i = 0; i < proc->mmobj()->mm_list_size(proc, self); i++) {
    oop entry = proc->mmobj()->mm_list_entry(proc, self, i);
    int exc;
    oop res = proc->send_1(entry, proc->vm()->new_symbol("=="), value, &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return exc;
    }
    if (res == MM_TRUE) {
      proc->stack_push(MM_TRUE);
      return 0;
    }
  }

  proc->stack_push(MM_FALSE);
  return 0;
}

static int prim_list_last(Process* proc) {
  oop self =  proc->dp();

  number size = proc->mmobj()->mm_list_size(proc, self);
  DBG("size of list: " << size << endl);
  if (size > 0) {
    oop entry = proc->mmobj()->mm_list_entry(proc, self, size-1);
    proc->stack_push(entry);
  } else {
    proc->raise("IndexError", "List overflow");
    assert(0); //unreachable
  }
  return 0;
}

static int prim_list_from(Process* proc) {
  oop self =  proc->dp();
  oop idx = proc->get_arg(0);
  number index = untag_small_int(idx);

  number size = proc->mmobj()->mm_list_size(proc, self);
  oop ret = proc->mmobj()->mm_list_new();
  for (number i = index; i < size; i++) {
    oop next = proc->mmobj()->mm_list_entry(proc, self, i);
    proc->mmobj()->mm_list_append(proc, ret, next);
  }
  proc->stack_push(ret);
  return 0;
}


static int prim_list_to_string(Process* proc) {
  oop self =  proc->dp();
  std::stringstream s;
  s << "[";
  std::string comma = "";
  for (int i = 0; i < proc->mmobj()->mm_list_size(proc, self); i++) {
    int exc;
    oop res = proc->send_0(proc->mmobj()->mm_list_entry(proc, self, i),
                                proc->vm()->new_symbol("toString"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << comma << proc->mmobj()->mm_string_cstr(proc, res);
    comma = ", ";
  }
  s << "]";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_list_to_source(Process* proc) {
  oop self =  proc->dp();
  std::stringstream s;
  s << "[";
  std::string comma = "";
  for (int i = 0; i < proc->mmobj()->mm_list_size(proc, self); i++) {
    int exc;
    oop res = proc->send_0(proc->mmobj()->mm_list_entry(proc, self, i),
                                proc->vm()->new_symbol("toSource"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << comma << proc->mmobj()->mm_string_cstr(proc, res);
    comma = ", ";
  }
  s << "]";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}


static int prim_dictionary_new(Process* proc) {
  oop self = proc->mmobj()->mm_dictionary_new();
  DBG(self << endl);
  proc->stack_push(self);
  return 0;
}

static int prim_dictionary_set(Process* proc) {
  oop self =  proc->dp();
  oop key = proc->get_arg(0);
  oop val = proc->get_arg(1);

  proc->mmobj()->mm_dictionary_set(proc, self, key, val);
  proc->stack_push(self);
  return 0;
}

static int prim_dictionary_index(Process* proc) {
  oop self =  proc->dp();
  oop key = proc->get_arg(0);

  proc->stack_push(proc->mmobj()->mm_dictionary_get(proc, self, key));
  return 0;
}

static int prim_dictionary_to_string(Process* proc) {
  oop self =  proc->dp();
  std::stringstream s;
  s << "{";
  boost::unordered_map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(proc, self);
  boost::unordered_map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(proc, self);
  std::string comma = "";
  for ( ; it != end; it++) {
    int exc;
    oop res = proc->send_0(it->first,
                                proc->vm()->new_symbol("toString"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << comma << proc->mmobj()->mm_string_cstr(proc, res) << ": ";

    res = proc->send_0(it->second,
                                proc->vm()->new_symbol("toString"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << proc->mmobj()->mm_string_cstr(proc, res);
    comma = ", ";
  }
  s << "}";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_dictionary_to_source(Process* proc) {
  oop self =  proc->dp();
  std::stringstream s;
  s << "{";
  boost::unordered_map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(proc, self);
  boost::unordered_map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(proc, self);
  std::string comma = "";
  for ( ; it != end; it++) {
    int exc;
    oop res = proc->send_0(it->first,
                                proc->vm()->new_symbol("toSource"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << comma << proc->mmobj()->mm_string_cstr(proc, res) << ": ";

    res = proc->send_0(it->second,
                                proc->vm()->new_symbol("toSource"), &exc);
    if (exc != 0) {
      proc->stack_push(res);
      return PRIM_RAISED;
    }
    s << proc->mmobj()->mm_string_cstr(proc, res);
    comma = ", ";
  }
  s << "}";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_dictionary_plus(Process* proc) {
  oop self =  proc->dp();
  oop other = proc->get_arg(0);

  oop d = proc->mmobj()->mm_dictionary_new();

  boost::unordered_map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(proc, self);
  boost::unordered_map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(proc, self);
  for ( ; it != end; it++) {
    proc->mmobj()->mm_dictionary_set(proc, d, it->first, it->second);
  }

  it = proc->mmobj()->mm_dictionary_begin(proc, other);
  end = proc->mmobj()->mm_dictionary_end(proc, other);
  for ( ; it != end; it++) {
    proc->mmobj()->mm_dictionary_set(proc, d, it->first, it->second);
  }
  proc->stack_push(d);
  return 0;
}

static int prim_dictionary_has(Process* proc) {
  oop self =  proc->dp();
  oop key = proc->get_arg(0);
  if (proc->mmobj()->mm_dictionary_has_key(proc, self, key)) {
    proc->stack_push(MM_TRUE);
  } else {
    proc->stack_push(MM_FALSE);
  }
  return 0;
}

static int prim_dictionary_keys(Process* proc) {
  oop self =  proc->dp();
  proc->stack_push(proc->mmobj()->mm_dictionary_keys(proc, self));
  return 0;
}

static int prim_dictionary_values(Process* proc) {
  oop self =  proc->dp();
  proc->stack_push(proc->mmobj()->mm_dictionary_values(proc, self));
  return 0;
}

static int prim_dictionary_size(Process* proc) {
  oop self =  proc->dp();
  proc->stack_push(tag_small_int(proc->mmobj()->mm_dictionary_size(proc, self)));
  return 0;
}

static int prim_mirror_entries(Process* proc) {
  oop self =  proc->dp();
  oop mirrored = ((oop*)self)[2];


  if (is_small_int(mirrored)) {
    DBG("mirrored is number" << endl);
    oop lst = proc->mmobj()->mm_list_new();
    proc->stack_push(lst);
    return 0;
  } else if (proc->mmobj()->mm_is_list(mirrored)) {
      oop lst = proc->mmobj()->mm_list_new();
      for (number i = 0; i < proc->mmobj()->mm_list_size(proc, mirrored); i++) {
        proc->mmobj()->mm_list_append(proc, lst, tag_small_int(i));
      }
      proc->stack_push(lst);
      return 0;
  } else if (proc->mmobj()->mm_is_dictionary(mirrored)) {
      oop lst = proc->mmobj()->mm_list_new();

      boost::unordered_map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(proc, mirrored);
      boost::unordered_map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(proc, mirrored);
      for ( ; it != end; it++) {
        proc->mmobj()->mm_list_append(proc, lst, it->first);
      }
      proc->stack_push(lst);
      return 0;
  } else {
    DBG("mirrored is not [number|list|dict]" << endl);

    oop mirrored_class = proc->mmobj()->mm_object_vt(mirrored);

    if (proc->mmobj()->delegates_to(mirrored_class, proc->vm()->get_prime("Object"))) {
      // mirrored is a class instance
      DBG("mirrored is class instance" << endl);
      oop cclass = proc->mmobj()->mm_class_get_compiled_class(proc, mirrored_class);
      proc->stack_push(proc->mmobj()->mm_compiled_class_fields(proc, cclass));
      return 0;
    } else { //unknown structure
      DBG("mirrored has unknown structure" << endl);
      oop lst = proc->mmobj()->mm_list_new();
      proc->stack_push(lst);
      return 0;
    }
  }
}

static int prim_mirror_value_for(Process* proc) {
  oop self =  proc->dp();
  oop entry = proc->get_arg(0);

  oop mirrored = ((oop*)self)[2];

  if (proc->mmobj()->mm_is_list(mirrored)) {
    proc->stack_push(proc->mmobj()->mm_list_entry(proc, mirrored, untag_small_int(entry)));
    return 0;
  } else if (proc->mmobj()->mm_is_dictionary(mirrored)) {
    proc->stack_push(proc->mmobj()->mm_dictionary_get(proc, mirrored, entry));
    return 0;
  } else { //generic instance ?
    //assume mirrored is a class instance
    oop mirrored_class = proc->mmobj()->mm_object_vt(mirrored);
    if (!(proc->mmobj()->delegates_to(mirrored_class, proc->vm()->get_prime("Object")))) {
      proc->raise("TypeError", "Mirrored should be class instance");
    }

    oop cclass = proc->mmobj()->mm_class_get_compiled_class(proc, mirrored_class);
    oop fields = proc->mmobj()->mm_compiled_class_fields(proc, cclass);

    number idx = proc->mmobj()->mm_list_index_of(proc, fields, entry);
    if (!(idx >= 0)) {
      proc->raise("TypeError", "invalid index");
    }
    DBG("index of " << idx << endl);
    proc->stack_push(((oop*)mirrored)[2+idx]);
    return 0;
  }
}

static int prim_mirror_set_value_for(Process* proc) {
  oop self =  proc->dp();
  oop entry = proc->get_arg(0);
  oop value = proc->get_arg(1);

  oop mirrored = ((oop*)self)[2];

  if (proc->mmobj()->mm_is_list(mirrored)) {
    proc->mmobj()->mm_list_set(proc, mirrored, untag_small_int(entry), value);
    proc->stack_push(proc->rp());
    return 0;
  } else if (proc->mmobj()->mm_is_dictionary(mirrored)) {
    proc->mmobj()->mm_dictionary_set(proc, mirrored, entry, value);
    return 0;
  } else { //generic instance ?
    //assume mirrored is a class instance
    oop mirrored_class = proc->mmobj()->mm_object_vt(mirrored);
    if (!(proc->mmobj()->delegates_to(mirrored_class, proc->vm()->get_prime("Object")))) {
      proc->raise("TypeError", "Mirrored should be class instance");
    }

    oop cclass = proc->mmobj()->mm_class_get_compiled_class(proc, mirrored_class);
    oop fields = proc->mmobj()->mm_compiled_class_fields(proc, cclass);

    number idx = proc->mmobj()->mm_list_index_of(proc, fields, entry);
    if (!(idx >= 0)) {
      proc->raise("TypeError", "invalid index");
    }
    DBG("index of " << idx << endl);
    ((oop*)mirrored)[2+idx] = value;
    proc->stack_push(proc->rp());
    return 0;
  }
}

static int prim_mirror_vt_for(Process* proc) {
  oop obj = proc->get_arg(0);
  DBG("vt for " << obj << " = " << proc->mmobj()->mm_object_vt(obj) << endl);
  proc->stack_push(proc->mmobj()->mm_object_vt(obj));
  return 0;
}

static int prim_equal(Process* proc) {
  oop self =  proc->rp();
  oop other = proc->get_arg(0);
  DBG(self << " == " << other << "?" << (self == other ? MM_TRUE : MM_FALSE) << endl);
  proc->stack_push(self == other ? MM_TRUE : MM_FALSE);
  return 0;
}

// static int prim_id(Process* proc) {
//   oop self =  proc->rp();
//   std::stringstream s;
//   s << self;
//   proc->stack_push(proc->mmobj()->mm_string_new(s.str().c_str()));
//   return 0;
// }

static int prim_object_not(Process* proc) {
  oop self =  proc->dp();
  if ((self == MM_FALSE) || (self == MM_NULL)) {
    proc->stack_push(MM_TRUE);
  } else {
    proc->stack_push(MM_FALSE);
  }
  return 0;
}

static int prim_behavior_to_string(Process* proc) {
  oop klass =  proc->rp();

  oop cclass = proc->mmobj()->mm_class_get_compiled_class(proc, klass);
  oop class_name = proc->mmobj()->mm_compiled_class_name(proc, cclass);
  char* str_class_name = proc->mmobj()->mm_string_cstr(proc, class_name);
  std::stringstream s;
  s << "#<" << str_class_name << " class>";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_behavior_to_source(Process* proc) {
  return prim_behavior_to_string(proc);
}

static int prim_object_to_string(Process* proc) {
  oop self =  proc->rp();

  oop klass = proc->mmobj()->mm_object_vt(self);
  oop cclass = proc->mmobj()->mm_class_get_compiled_class(proc, klass);
  oop class_name = proc->mmobj()->mm_compiled_class_name(proc, cclass);
  char* str_class_name = proc->mmobj()->mm_string_cstr(proc, class_name);
  std::stringstream s;
  s << "#<" << str_class_name << " instance: " << self << ">";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_object_to_source(Process* proc) {
  return prim_object_to_string(proc);
}

static int prim_object_send(Process* proc) {
  oop self =  proc->rp();
  oop name = proc->get_arg(0);
  oop args_list = proc->get_arg(1);

  int exc;
  oop res = proc->send(self, name, args_list, &exc);
  if (exc != 0) {
    proc->stack_push(res);
    return PRIM_RAISED;
  }
  proc->stack_push(res);
  return 0;
}

static int prim_object_super_send(Process* proc) {
  oop self =  proc->rp();
  oop name = proc->get_arg(0);
  oop args_list = proc->get_arg(1);

  int exc;
  oop res = proc->super_send(self, name, args_list, &exc);
  if (exc != 0) {
    proc->stack_push(res);
    return PRIM_RAISED;
  }
  proc->stack_push(res);
  return 0;
}

static int prim_object_id(Process* proc) {
  oop self =  proc->rp();
  std::stringstream s;
  s << self;
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_symbol_to_string(Process* proc) {
  oop self =  proc->dp();
  char* str = proc->mmobj()->mm_symbol_cstr(proc, self);
  DBG(self << " str: " << str << endl);
  oop oop_str = proc->mmobj()->mm_string_new(str);
  proc->stack_push(oop_str);
  return 0;
}

static int prim_module_to_string(Process* proc) {
  oop self =  proc->rp();

  oop cmod = proc->mmobj()->mm_module_get_cmod(self);
  // DBG("prim_module_to_string imod: " << self << " cmod: " << cmod << endl);
  oop mod_name = proc->mmobj()->mm_compiled_module_name(proc, cmod);
  char* str_mod_name = proc->mmobj()->mm_string_cstr(proc, mod_name);
  std::stringstream s;
  s << "#<" << str_mod_name << " module instance: " << self << ">";
  oop oop_str = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->stack_push(oop_str);
  return 0;
}

static int prim_compiled_function_new_context(Process* proc) {
      //return Context.new(this, fp, module);
  oop args = proc->mmobj()->mm_list_new();
  proc->mmobj()->mm_list_append(proc, args, proc->rp());
  proc->mmobj()->mm_list_append(proc, args, proc->get_arg(0));
  proc->mmobj()->mm_list_append(proc, args, proc->get_arg(1));

  int exc;
  oop ctx = proc->send(proc->vm()->get_prime("Context"), proc->vm()->new_symbol("new"), args, &exc);
  if (exc != 0) {
    proc->stack_push(ctx);
    return PRIM_RAISED;
  }
  proc->stack_push(ctx);
  return 0;
}

static int prim_compiled_function_with_env(Process* proc) {
  // oop self =  proc->dp();
  oop text = proc->get_arg(0);
  oop scope_names = proc->get_arg(1);
  oop cmod = proc->get_arg(2);

  // DBG("prim_compiled_function_with_env " << proc->mmobj()->mm_string_cstr(text) << " -- " << cmod << " " << vars << endl);

  std::list<std::string> lst = proc->mmobj()->mm_sym_list_to_cstring_list(proc, scope_names);

  int exc;
  oop cfun = proc->vm()->compile_fun(proc, proc->mmobj()->mm_string_cstr(proc, text), lst, cmod, &exc);
  if (exc != 0) {
    proc->stack_push(cfun);
    return PRIM_RAISED;
  }

  // DBG("prim_compiled_function_with_env: GOT cfun: " << cfun << " " << *(oop*) cfun << endl);
  proc->stack_push(cfun);
  return 0;
}

static int prim_compiled_function_with_frame(Process* proc) {
  oop text = proc->get_arg(0);
  oop frame = proc->get_arg(1);
  oop cmod = proc->get_arg(2);

  oop fn = proc->mmobj()->mm_frame_get_cp(proc, frame);
  // DBG("prim_compiled_function_with_frame: associated to fun: " << fn << endl);
  oop env_table = proc->mmobj()->mm_function_env_table(proc, fn);
  // DBG("prim_compiled_function_with_frame: env_table: " << env_table << endl);

  std::list<std::string> lst = proc->mmobj()->mm_sym_list_to_cstring_list(proc, env_table);

  int exc;
  oop cfun = proc->vm()->compile_fun(proc, proc->mmobj()->mm_string_cstr(proc, text), lst, cmod, &exc);
  // DBG("prim_compiled_function_with_frame: cfun: " << cfun << " " << exc << endl);
  if (exc != 0) {
    proc->stack_push(cfun);
    return PRIM_RAISED;
  }

  // DBG("prim_compiled_function_with_env: GOT cfun: " << cfun << " " << *(oop*) cfun << endl);
  proc->stack_push(cfun);
  return 0;
}

static int prim_compiled_function_as_context_with_vars(Process* proc) {
  /* (1) allocate and assemble the ep/env from the vars with its values;
     (2) instantiate a Context with (self, env, imod)
  */

  oop self =  proc->dp();
  oop imod = proc->get_arg(0);
  oop vars = proc->get_arg(1);

  number env_size = proc->mmobj()->mm_compiled_function_get_num_locals_or_env(proc, self);
  oop env = (oop) calloc(sizeof(oop), env_size + 2); //+2: rp, dp

  if (vars != MM_NULL) {
    oop env_table = proc->mmobj()->mm_compiled_function_env_table(proc, self);
    boost::unordered_map<oop,oop>::iterator it = proc->mmobj()->mm_dictionary_begin(proc, vars);
    boost::unordered_map<oop,oop>::iterator end = proc->mmobj()->mm_dictionary_end(proc, vars);
    for ( ; it != end; it++) {
      std::string name = proc->mmobj()->mm_symbol_cstr(proc, it->first);
      if (name == "this") {
        ((oop*)env)[env_size] = it->second; //rp
        ((oop*)env)[env_size+1] = it->second; //ep
      } else {
        number idx = proc->mmobj()->mm_list_index_of(proc, env_table, it->first);
        ((oop*)env)[idx] = it->second;
      }
    }
  }

  oop args = proc->mmobj()->mm_list_new();
  proc->mmobj()->mm_list_append(proc, args, self);
  proc->mmobj()->mm_list_append(proc, args, env);
  proc->mmobj()->mm_list_append(proc, args, imod);

  DBG("Creating context..." << self << " " << vars << " " << imod << endl);

  int exc;
  oop ctx = proc->send(proc->vm()->get_prime("Context"), proc->vm()->new_symbol("new"), args, &exc);
  if (exc != 0) {
    proc->stack_push(ctx);
    return PRIM_RAISED;
  }

  // DBG("compiled_function_as_context_with_vars: " << ctx << endl);
  // DBG("ctx[cfun] " << proc->mmobj()->mm_function_get_cfun(ctx) << endl);
  // DBG("ctx[env] " << proc->mmobj()->mm_context_get_env(ctx) << endl);
  // DBG("ctx[imod] " << proc->mmobj()->mm_function_get_module(ctx) << endl);

  proc->stack_push(ctx);
  return 0;
}

static int prim_compiled_function_get_text(Process* proc) {
  oop self =  proc->dp();
  proc->stack_push(proc->mmobj()->mm_compiled_function_get_text(proc, self));
  return 0;
}

// static int prim_compiled_function_get_line_mapping(Process* proc) {
//   oop self =  proc->dp();
//   proc->stack_push(proc->mmobj()->mm_compiled_function_get_line_mapping(self));
//   return 0;
// }

// static int prim_compiled_function_get_loc_mapping(Process* proc) {
//   oop self =  proc->dp();
//   proc->stack_push(proc->mmobj()->mm_compiled_function_get_loc_mapping(self));
//   return 0;
// }

static int prim_compiled_function_loc_for(Process* proc) {
  oop self =  proc->dp();
  if (proc->mmobj()->mm_compiled_function_is_prim(proc, self)) {
    DBG("LOCINFO IS NULL" << endl);
    proc->stack_push(MM_NULL);
    return 0;
  }

  oop frame = proc->get_arg(0);
  bytecode* ip = proc->mmobj()->mm_frame_get_ip(proc, frame);
  DBG("loc_for_ip: " << ip << endl);
  bytecode* base_ip = proc->mmobj()->mm_compiled_function_get_code(proc, self);
  word idx = ip - base_ip;
  DBG("loc_for_ip base: " << base_ip << " ip: " << ip << " idx: " << idx << endl);

  oop mapping = proc->mmobj()->mm_compiled_function_get_loc_mapping(proc, self);
  boost::unordered_map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(proc, mapping);
  boost::unordered_map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(proc, mapping);
  oop the_lst = MM_NULL;
  for ( ; it != end; it++) {
    word b_offset = untag_small_int(it->first);
    DBG(" -- " << b_offset << " " <<  idx << std::endl);
    if (b_offset == idx) {
      the_lst = it->second;
      break;
    } else if(b_offset > idx) {
      break;
    } else {
      the_lst = it->second;
    }
  }
  proc->stack_push(the_lst);
  return 0;
}

static int prim_compiled_function_recompile(Process* proc) {
  oop text = proc->get_arg(0);
  oop self = proc->dp();
  int exc;
  oop cfun = proc->vm()->recompile_fun(proc, self, proc->mmobj()->mm_string_cstr(proc, text), &exc);
  DBG("cfun: " << cfun << " " << exc << endl);
  if (exc != 0) {
    proc->stack_push(cfun);
    return PRIM_RAISED;
  }

  // DBG() << "prim_compiled_function_with_env: GOT cfun: " << cfun << " " << *(oop*) cfun << endl;
  proc->stack_push(cfun);
  return 0;

}

static int prim_context_get_env(Process* proc) {
  oop self =  proc->dp();

  oop ctx_env = proc->mmobj()->mm_context_get_env(proc, self);

  oop env_table = proc->mmobj()->mm_function_env_table(proc, self);

  oop env_dict = proc->mmobj()->mm_dictionary_new();

  for (number i = 0; i < proc->mmobj()->mm_list_size(proc, env_table); i++) {
    oop key = proc->mmobj()->mm_list_entry(proc, env_table, i);
    oop val = ((oop*)ctx_env)[i];
    proc->mmobj()->mm_dictionary_set(proc, env_dict, key, val);
  }

  // std::map<oop, oop>::iterator it = proc->mmobj()->mm_dictionary_begin(env_table);
  // std::map<oop, oop>::iterator end = proc->mmobj()->mm_dictionary_end(env_table);
  // for ( ; it != end; it++) {
  //   number idx = untag_small_int(it->second);
  //   DBG("env idx " << idx << endl);
  //   proc->mmobj()->mm_dictionary_set(env_dict, it->first, ((oop*)ctx_env)[2+idx]); //2: rp, dp
  // }

  proc->stack_push(env_dict);
  return 0;
}

static int prim_context_with_frame(Process* proc) {
  oop code = proc->get_arg(0);
  oop frame = proc->get_arg(1);
  oop imod = proc->get_arg(2);

  oop cmod = proc->mmobj()->mm_module_get_cmod(imod);

  oop args = proc->mmobj()->mm_list_new();
  proc->mmobj()->mm_list_append(proc, args, code);
  proc->mmobj()->mm_list_append(proc, args, frame);
  proc->mmobj()->mm_list_append(proc, args, cmod);


  int exc;
  oop cfn = proc->send(proc->vm()->get_prime("CompiledFunction"), proc->vm()->new_symbol("withFrame"), args, &exc);
  if (exc != 0) {
    proc->stack_push(cfn);
    return PRIM_RAISED;
  }

  oop fp = proc->mmobj()->mm_frame_get_fp(proc, frame);

  args = proc->mmobj()->mm_list_new();
  proc->mmobj()->mm_list_append(proc, args, fp);
  proc->mmobj()->mm_list_append(proc, args, imod);
  oop ctx = proc->send(cfn, proc->vm()->new_symbol("new_context"), args, &exc);
  if (exc != 0) {
    proc->stack_push(ctx);
    return PRIM_RAISED;
  }
  proc->stack_push(ctx);
  return 0;
}

static int prim_function_get_env(Process* proc) {
  return prim_context_get_env(proc);
}



static int prim_context_new(Process* proc) {
  oop self = proc->dp();
  oop cfun = proc->get_arg(0);
  oop env = proc->get_arg(1);
  oop imod = proc->get_arg(2);

  proc->mmobj()->mm_context_set_cfun(proc, self, cfun);
  proc->mmobj()->mm_context_set_env(proc, self, env);
  proc->mmobj()->mm_context_set_module(proc, self, imod);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_get_compiled_module(Process* proc) {
  oop imod = proc->get_arg(0);
  proc->stack_push(proc->mmobj()->mm_module_get_cmod(imod));
  return 0;
}

static int prim_get_compiled_module_by_name(Process* proc) {
  oop name = proc->get_arg(0);
  char* str_name = proc->mmobj()->mm_string_cstr(proc, name);
  proc->stack_push(proc->vm()->get_compiled_module(proc, str_name));
  return 0;
}

static int prim_get_current_process(Process* proc) {
  proc->stack_push(proc->mmobj()->mm_process_new(proc, proc));
  return 0;
}

static int prim_get_current_frame(Process* proc) {
  proc->stack_push(proc->mmobj()->mm_frame_new(proc, proc->bp()));
  return 0;
}

static int prim_test_import(Process* proc) {
  oop filepath = proc->get_arg(0);
  oop args = proc->get_arg(1);

  char* str_filepath = proc->mmobj()->mm_string_cstr(proc, filepath);
  DBG(str_filepath << endl);
  MMCImage* mmc = new MMCImage(proc, proc->vm()->core(), str_filepath);
  mmc->load();
  proc->stack_push(mmc->instantiate_module(args));
  return 0;
}

// static int prim_test_get_module_function(Process* proc) {
//   oop name = proc->get_arg(0);
//   oop imod = proc->get_arg(1);

//   // char* str = proc->mmobj()->mm_symbol_cstr(name);
//   // DBG("XX: " << name << " str: " << str << endl);

//   oop dict = proc->mmobj()->mm_module_dictionary(imod);
//   oop fun = proc->mmobj()->mm_dictionary_get(dict, name);
//   DBG("prim_test_get_module_function: " << imod << " "
//           << name << " " << dict << " " << fun << endl);
//   proc->stack_push(fun);
//   return 0;
// }

static
void get_mm_files(const fs::path& root, std::vector<std::string>& ret) {
  if (!fs::exists(root)) return;

  if (fs::is_directory(root)) {
    fs::directory_iterator it(root);
    fs::directory_iterator endit;
    while(it != endit) {
      if (fs::is_regular_file(*it) and it->path().extension() == ".mmc")
      {
        ret.push_back(it->path().string());
      }
      ++it;
    }
  }
}

static int prim_test_files(Process* proc) {
  std::vector<std::string> ret;
  get_mm_files("./tests", ret);

  oop list = proc->mmobj()-> mm_list_new();

  for(std::vector<std::string>::iterator it = ret.begin(); it != ret.end(); it++) {
    oop str = proc->mmobj()->mm_string_new((*it).c_str());
    proc->mmobj()->mm_list_append(proc, list, str);
  }

  proc->stack_push(list);
  return 0;
}

static int prim_test_catch_exception(Process* proc) {
  int exc;
  oop ret = proc->send_0(proc->mp(), proc->vm()->new_symbol("bar"), &exc);
  proc->stack_push(ret);
  return 0;
}

static int prim_test_debug(Process* proc) {
  return PRIM_HALTED;
}
//   // DBG("prim_test_debug" << endl);
//   proc->pause();

//   // DBG("prim_test_debug: paused" << endl);
//   Process* dbg_proc = new Process(proc->vm());
//   // DBG("prim_test_debug: created new process" << endl);

//   oop oop_target_proc = proc->mmobj()->mm_process_new(proc);

//   int exc;
//   oop res = dbg_proc->send_1(proc->mp(), proc->vm()->new_symbol("dbg_main"), oop_target_proc, &exc);
//   if (exc != 0) {
//     proc->stack_push(res);
//     return PRIM_RAISED;
//   }

//   // DBG("prim_test_debug: called dbg_main" << endl);

//   proc->stack_push(proc->rp());
//   return 0;
// }

static int prim_process_step_into(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->step_into();

  proc->stack_push(proc->rp());
  return 0;
}

static int prim_process_step_over(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->step_over();

  proc->stack_push(proc->rp());
  return 0;
}

static int prim_process_step_over_line(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->step_over_line();

  proc->stack_push(proc->rp());
  return 0;
}

static int prim_process_step_out(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->step_out();

  proc->stack_push(proc->rp());
  return 0;
}

static int prim_process_resume(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->resume();

  proc->stack_push(proc->rp());
  return 0;
}

static int prim_process_reload_frame(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->reload_frame();
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_process_return_from_frame(Process* proc) {
  oop oop_target_proc = proc->rp();
  oop retval = proc->get_arg(0);

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->unload_fun_and_return(retval);
  target_proc->clear_exception_state();
  proc->stack_push(proc->rp());
  return 0;
}


static int prim_process_break_at_addr(Process* proc) {
  oop oop_target_proc = proc->rp();
  bytecode* addr = (bytecode*) proc->get_arg(0);

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->break_at_addr(addr);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_process_rewind_and_continue(Process* proc) {
  oop oop_target_proc = proc->rp();
  oop bp = proc->get_arg(0);

  DBG("rewiding to bp: " << bp << endl);
  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  target_proc->rewind_to_frame_and_continue(bp);
  proc->stack_push(proc->rp());
  return 0;
}

static int prim_process_cp(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  proc->stack_push(target_proc->cp());
  return 0;
}

static int prim_process_fp(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  proc->stack_push(target_proc->fp());
  return 0;
}

static int prim_process_mp(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
  proc->stack_push(target_proc->mp());
  return 0;
}


// static int prim_process_ip(Process* proc) {
//   oop oop_target_proc = proc->rp();

//   Process* target_proc = (Process*) (((oop*)oop_target_proc)[2]);
//   proc->stack_push((oop) target_proc->ip());
//   return 0;
// }

static int prim_process_frames(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = proc->mmobj()->mm_process_get_proc(proc, oop_target_proc);

  oop frames = proc->mmobj()->mm_list_new();
  // DBG("prim_process_frames: stack deph: " << target_proc->stack_depth() << endl);

  oop bp = target_proc->top_frame();
  DBG("cp from bp " << bp << ": " << target_proc->cp_from_base(bp) << endl);
  while(bp > (oop) 2) { //??
    DBG("loop on bp: " << bp << endl);
    oop cp = target_proc->cp_from_base(bp);
    DBG("cp: " << cp << endl);
    if (cp) {
      proc->mmobj()->mm_list_append(proc, frames, proc->mmobj()->mm_frame_new(proc, bp));
    } else {
      break;
    }
    bp = *(oop*) bp;
    DBG("new bp " << bp << endl);
  }
  proc->stack_push(frames);
  return 0;
}

static int prim_exception_constructor(Process* proc) {
  oop dself = proc->dp();
  // std::cerr << "prim exception" << " bp " << proc->bp() << endl;
  oop msg = proc->get_arg(0);
  proc->mmobj()->mm_exception_set_message(proc, dself, msg);

  oop bp = proc->bp();
  std::stringstream s;
  s << "Uncaugh exception: " << proc->mmobj()->mm_string_cstr(proc, msg) << endl;
  s << proc->dump_stack_trace(true);
  // while (bp) {
  //   oop cp = proc->cp_from_base(bp);
  //   if (!cp) break;
  //   s << proc->mmobj()->mm_string_cstr(proc,
  //                                 proc->mmobj()->mm_function_get_name(
  //                                   proc, cp), true);
  //   s << "():" << (proc->mmobj()->mm_function_get_line_for_instruction(
  //                    proc, cp, proc->ip_from_base(bp), true) + 1) << endl;
  //   bp = *(oop*)bp;
  // }

  oop st = proc->mmobj()->mm_string_new(s.str().c_str());
  proc->mmobj()->mm_exception_set_st(proc, dself, st);
  // std::cerr << "get bp " << proc->mmobj()->mm_exception_get_bp(proc, self);
  proc->stack_push(proc->rp());
  return 0;
}

// static int prim_exception_stack_trace(Process* proc) {
//   oop dself = proc->dp();
//   oop msg = proc->mmobj()->mm_exception_get_message(proc, dself, msg);

//   std::stringstream s;
//   s << "Uncaugh exception: " << proc->mmobj()->mm_string_cstr(proc, msg) << endl;

//   while (bp) {
//     oop cp = proc->cp_from_base(bp);
//     if (!cp) break;
//     s << proc->mmobj()->mm_string_cstr(proc,
//                                   proc->mmobj()->mm_function_get_name(
//                                     proc, cp), true);
//     s << "():" << (proc->mmobj()->mm_function_get_line_for_instruction(
//                      proc, cp, proc->ip_from_base(bp), true) + 1) << endl;
//     bp = *(oop*)bp;
//   }

//   oop ret = proc->mmobj()->mm_string_new(s.str().c_str());
//   proc->stack_push(ret);
//   return 0;
// }

static int prim_process_current_exception(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = proc->mmobj()->mm_process_get_proc(proc, oop_target_proc);
  proc->stack_push(target_proc->current_exception());
  return 0;
}

static int prim_process_run_until(Process* proc) {
  oop oop_target_proc = proc->rp();
  oop cfun = proc->get_arg(0);
  oop lineno = proc->get_arg(1);

  Process* target_proc = proc->mmobj()->mm_process_get_proc(proc, oop_target_proc);
  target_proc->run_until(cfun, untag_small_int(lineno));
  proc->stack_push(oop_target_proc);
  return 0;
}

static int prim_process_add_breakpoint(Process* proc) {
  oop oop_target_proc = proc->rp();
  oop cfun = proc->get_arg(0);
  oop lineno = proc->get_arg(1);

  Process* target_proc = proc->mmobj()->mm_process_get_proc(proc, oop_target_proc);
  target_proc->add_breakpoint(cfun, untag_small_int(lineno));
  proc->stack_push(oop_target_proc);
  return 0;
}

static int prim_process_toggle_module_break_mode(Process* proc) {
  oop oop_target_proc = proc->rp();

  Process* target_proc = proc->mmobj()->mm_process_get_proc(proc, oop_target_proc);
  if (target_proc->toggle_module_break_mode()) {
    proc->stack_push(MM_TRUE);
  } else {
    proc->stack_push(MM_FALSE);
  }
  return 0;
}

// static int prim_process_apply(Process* proc) {
//   oop oop_target_proc = proc->rp();
//   oop fn = proc->get_arg(0);

//   Process* target_proc = proc->mmobj()->mm_process_get_proc(oop_target_proc);
//   int exc;
    //TODO: we are not checking arity!
//   oop res = target_proc->do_call_protected(fn, &exc);
//   if (exc != 0) {
//     proc->stack_push(res);
//     return PRIM_RAISED;
//   }
//   proc->stack_push(res);
//   return 0;
// }

static int prim_frame_ip(Process* proc) {
  oop frame = proc->dp();
  oop ip = (oop) proc->mmobj()->mm_frame_get_ip(proc, frame);
  // oop bp =   proc->mmobj()->mm_frame_get_bp(frame);
  // DBG("prim_frame_ip bp: " << bp << endl);
  // oop ip = *((oop*)(bp - 2));
  // DBG("prim_frame_ip bp: " << bp << " has ip: " << ip << endl);
  proc->stack_push(ip);
  return 0;
}

static int prim_frame_cp(Process* proc) {
  oop frame = proc->dp();
  // oop bp =   proc->mmobj()->mm_frame_get_bp(frame);
  // DBG("prim_frame_cp bp: " << bp << endl);
  // oop cp = *((oop*)(bp - 5));
  // DBG("prim_frame_cp bp: " << bp << " has cp: " << cp << endl);
  proc->stack_push(proc->mmobj()->mm_frame_get_cp(proc, frame));
  return 0;
}

static int prim_frame_fp(Process* proc) {
  oop frame = proc->dp();
  proc->stack_push(proc->mmobj()->mm_frame_get_fp(proc, frame));
  return 0;
}

static int prim_frame_rp(Process* proc) {
  oop frame = proc->dp();
  proc->stack_push(proc->mmobj()->mm_frame_get_rp(proc, frame));
  return 0;
}

static int prim_frame_dp(Process* proc) {
  oop frame = proc->dp();
  proc->stack_push(proc->mmobj()->mm_frame_get_dp(proc, frame));
  return 0;
}

static int prim_frame_get_local_value(Process* proc) {
  oop frame = proc->dp();
  number idx = untag_small_int(proc->get_arg(0));
  oop fp = proc->mmobj()->mm_frame_get_fp(proc, frame);
  proc->stack_push(*((oop*)fp + idx));
  return 0;
}


static int prim_modules_path(Process* proc) {
  const char* mmpath = getenv("MEME_PATH");
  if (mmpath) {
    proc->stack_push(proc->mmobj()->mm_string_new(mmpath));
  } else {
    proc->stack_push(proc->mmobj()->mm_string_new("./mm"));
  }
  return 0;
}

void init_primitives(VM* vm) {
  vm->register_primitive("io_print", prim_io_print);
  vm->register_primitive("io_read_file", prim_io_read_file);
  vm->register_primitive("io_write_file", prim_io_write_file);

  vm->register_primitive("remote_repl_compile_module", prim_remote_repl_compile_module);
  vm->register_primitive("remote_repl_instantiate_module", prim_remote_repl_instantiate_module);

  vm->register_primitive("behavior_to_string", prim_behavior_to_string);
  vm->register_primitive("behavior_to_source", prim_behavior_to_source);

  vm->register_primitive("number_sum", prim_number_sum);
  vm->register_primitive("number_sub", prim_number_sub);
  vm->register_primitive("number_mul", prim_number_mul);
  vm->register_primitive("number_lt", prim_number_lt);
  vm->register_primitive("number_gt", prim_number_gt);
  vm->register_primitive("number_gteq", prim_number_gteq);
  vm->register_primitive("number_to_string", prim_number_to_string);
  vm->register_primitive("number_to_source", prim_number_to_source);

  vm->register_primitive("exception_throw", prim_exception_throw);

  vm->register_primitive("equal", prim_equal);
  // vm->register_primitive("id", prim_id);

  vm->register_primitive("object_not", prim_object_not);
  vm->register_primitive("object_to_string", prim_object_to_string);
  vm->register_primitive("object_to_source", prim_object_to_source);
  vm->register_primitive("object_send", prim_object_send);
  vm->register_primitive("object_super_send", prim_object_super_send);
  vm->register_primitive("object_id", prim_object_id);

  vm->register_primitive("symbol_to_string", prim_symbol_to_string);

  vm->register_primitive("module_to_string", prim_module_to_string);

  vm->register_primitive("list_new", prim_list_new);
  vm->register_primitive("list_append", prim_list_append);
  vm->register_primitive("list_prepend", prim_list_prepend);
  vm->register_primitive("list_index", prim_list_index);
  vm->register_primitive("list_each", prim_list_each);
  vm->register_primitive("list_map", prim_list_map);
  vm->register_primitive("list_filter", prim_list_filter);
  vm->register_primitive("list_detect", prim_list_detect);
  vm->register_primitive("list_has", prim_list_has);
  vm->register_primitive("list_last", prim_list_last);
  vm->register_primitive("list_from", prim_list_from);
  vm->register_primitive("list_to_string", prim_list_to_string);
  vm->register_primitive("list_to_source", prim_list_to_source);
  vm->register_primitive("list_size", prim_list_size);
  vm->register_primitive("list_reverse", prim_list_reverse);
  vm->register_primitive("list_join", prim_list_join);
  vm->register_primitive("list_plus", prim_list_plus);


  vm->register_primitive("dictionary_new", prim_dictionary_new);
  vm->register_primitive("dictionary_set", prim_dictionary_set);
  vm->register_primitive("dictionary_index", prim_dictionary_index);
  vm->register_primitive("dictionary_plus", prim_dictionary_plus);
  vm->register_primitive("dictionary_has", prim_dictionary_has);
  vm->register_primitive("dictionary_keys", prim_dictionary_keys);
  vm->register_primitive("dictionary_values", prim_dictionary_values);
  vm->register_primitive("dictionary_size", prim_dictionary_size);
  vm->register_primitive("dictionary_to_string", prim_dictionary_to_string);
  vm->register_primitive("dictionary_to_source", prim_dictionary_to_source);

  vm->register_primitive("string_to_integer", prim_string_to_integer);
  vm->register_primitive("string_append", prim_string_append);
  vm->register_primitive("string_equal", prim_string_equal);
  vm->register_primitive("string_count", prim_string_count);
  vm->register_primitive("string_size", prim_string_size);
  vm->register_primitive("string_find", prim_string_find);
  vm->register_primitive("string_rindex", prim_string_rindex);
  vm->register_primitive("string_index", prim_string_index);

  vm->register_primitive("string_from", prim_string_from);
  vm->register_primitive("string_substr", prim_string_substr);
  vm->register_primitive("string_replace_all", prim_string_replace_all);
  vm->register_primitive("string_b64decode", prim_string_b64decode);
  vm->register_primitive("string_b64encode", prim_string_b64encode);

  vm->register_primitive("string_only_spaces", prim_string_only_spaces);
  vm->register_primitive("string_only_digits", prim_string_only_digits);
  vm->register_primitive("string_is_lower", prim_string_is_lower);
  vm->register_primitive("string_is_upper", prim_string_is_upper);
  vm->register_primitive("string_split", prim_string_split);
  vm->register_primitive("string_to_symbol", prim_string_to_symbol);
  vm->register_primitive("string_trim", prim_string_trim);
  vm->register_primitive("string_each", prim_string_each);


  vm->register_primitive("mirror_entries", prim_mirror_entries);
  vm->register_primitive("mirror_value_for", prim_mirror_value_for);
  vm->register_primitive("mirror_set_value_for", prim_mirror_set_value_for);
  vm->register_primitive("mirror_vt_for", prim_mirror_vt_for);


  vm->register_primitive("compiled_function_new_context", prim_compiled_function_new_context);
  vm->register_primitive("compiled_function_with_env", prim_compiled_function_with_env);
  vm->register_primitive("compiled_function_with_frame", prim_compiled_function_with_frame);
  vm->register_primitive("compiled_function_as_context_with_vars", prim_compiled_function_as_context_with_vars);

  vm->register_primitive("compiled_function_get_text", prim_compiled_function_get_text);
  // vm->register_primitive("compiled_function_get_line_mapping", prim_compiled_function_get_line_mapping);
  // vm->register_primitive("compiled_function_get_loc_mapping", prim_compiled_function_get_loc_mapping);
  vm->register_primitive("compiled_function_loc_for", prim_compiled_function_loc_for);
  vm->register_primitive("compiled_function_recompile", prim_compiled_function_recompile);

  vm->register_primitive("context_new", prim_context_new);
  vm->register_primitive("context_get_env", prim_context_get_env);
  vm->register_primitive("context_with_frame", prim_context_with_frame);

  vm->register_primitive("function_get_env", prim_function_get_env);


  vm->register_primitive("test_import", prim_test_import);
  // vm->register_primitive("test_get_module_function", prim_test_get_module_function);
  vm->register_primitive("test_files", prim_test_files);

  vm->register_primitive("test_catch_exception", prim_test_catch_exception);
  vm->register_primitive("test_debug", prim_test_debug);

  vm->register_primitive("process_step_into", prim_process_step_into);
  vm->register_primitive("process_step_over", prim_process_step_over);
  vm->register_primitive("process_step_over_line", prim_process_step_over_line);
  vm->register_primitive("process_step_out", prim_process_step_out);
  vm->register_primitive("process_resume", prim_process_resume);
  vm->register_primitive("process_reload_frame", prim_process_reload_frame);
  vm->register_primitive("process_return_from_frame", prim_process_return_from_frame);
  vm->register_primitive("process_break_at_addr", prim_process_break_at_addr);
  vm->register_primitive("process_rewind_and_continue", prim_process_rewind_and_continue);
  vm->register_primitive("process_current_exception", prim_process_current_exception);
  vm->register_primitive("process_run_until", prim_process_run_until);
  vm->register_primitive("process_add_breakpoint", prim_process_add_breakpoint);
  vm->register_primitive("process_toggle_module_break_mode", prim_process_toggle_module_break_mode);

  vm->register_primitive("process_cp", prim_process_cp);
  vm->register_primitive("process_fp", prim_process_fp);
  vm->register_primitive("process_mp", prim_process_mp);
  // vm->register_primitive("process_ip", prim_process_ip);
  vm->register_primitive("process_frames", prim_process_frames);
  // vm->register_primitive("process_apply", prim_process_apply);
  // vm->register_primitive("process_eval_in_frame", prim_process_eval_in_frame);

  vm->register_primitive("exception_constructor", prim_exception_constructor);
  // vm->register_primitive("exception_stack_trace", prim_exception_stack_trace);

  vm->register_primitive("frame_ip", prim_frame_ip);
  vm->register_primitive("frame_cp", prim_frame_cp);
  vm->register_primitive("frame_fp", prim_frame_fp);
  vm->register_primitive("frame_rp", prim_frame_rp);
  vm->register_primitive("frame_dp", prim_frame_dp);
  vm->register_primitive("frame_get_local_value", prim_frame_get_local_value);

  vm->register_primitive("get_current_process", prim_get_current_process);
  vm->register_primitive("get_current_frame", prim_get_current_frame);
  vm->register_primitive("get_compiled_module", prim_get_compiled_module);
  vm->register_primitive("get_compiled_module_by_name", prim_get_compiled_module_by_name);

  vm->register_primitive("modules_path", prim_modules_path);

  qt_init_primitives(vm);
}
