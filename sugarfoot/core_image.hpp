#ifndef CORE_LOADER_HPP
#define CORE_LOADER_HPP

#include <list>
#include <string>
#include <boost/unordered_map.hpp>
#include "defs.hpp"
#include "log.hpp"

class VM;

class CoreImage {
  static word HEADER_SIZE;
  static const char* PRIMES_NAMES[];
  static int TOTAL_PRIMES;

public:
  CoreImage(VM*, const char*);
  void load();
  inline oop get_prime(const char* name) { return _primes.at(name); }
  bool has_class(const char*);
  oop get_module_instance();
  boost::unordered_map<std::string,oop>& get_primes() { return _primes; }
private:
  void load_header();
  void load_prime_objects_table();
  bool is_prime(const char*);
  bool is_core_instance(const char*);

  MMLog _log;
  VM* _vm;
  const char* _filepath;
  int _data_size;
  char* _data;

  //header
  word _num_entries;
  word _names_size;
  word _es_size;
  word _ot_size;

  oop _core_imod;

  boost::unordered_map<std::string, oop> _primes;
};

#endif
