# -*- coding: utf-8 -*-
# Copyright (c) 2012-2013 Thiago B. L. Silva <thiago@metareload.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

from PyQt4 import QtGui, QtCore, QtWebKit
from PyQt4.QtWebKit import *
from PyQt4.QtGui import *
from PyQt4.QtCore import *

import sys
import re
from os import listdir
from os.path import isfile, join
from pdb import set_trace as br
import scintilla_editor
from config import MODULES_PATH
import traceback
from mmpprint import P
from mobject import obj_eq, id_eq
import logging

logger = logging.getLogger("i")

_app = None
_qt_imodule = None

def prim_modules_path(proc):
    return MODULES_PATH

def prim_available_modules(proc):
    return [f[:-3] for f in listdir(MODULES_PATH) if isfile(join(MODULES_PATH,f)) and f != "core.md"]

def _get_module(proc, name):
    return proc.interpreter.compiled_module_by_filename(proc, name + ".mm")

def prim_get_module(proc):
    return _get_module(proc,  proc.locals()['name'])

def prim_save_module(proc):
    return proc.interpreter.save_module(proc,proc.locals()['name'])

def prim_import(proc):
    compiled_module = proc.interpreter.compiled_module_by_filename(proc.locals()["mname"])
    args = dict(zip(compiled_module["params"],proc.locals()["margs"]).items())
    imodule = proc.interpreter.instantiate_module(compiled_module, args, proc.interpreter.kernel_module_instance())
    return imodule

def prim_exception_throw(proc):
    proc.throw(proc.reg('r_rp'))

def prim_exception_cl_throw(proc):
    ExceptionClass = proc.reg('r_rp')
    msg = proc.locals()['message']
    ex = proc.do_send(ExceptionClass, 'new', [msg])
    proc.throw(ex)

def prim_exception_type(proc):
    return proc.reg('r_rp')['_vt']


#### VMProcess
def prim_vmprocess_spawn(proc):
    logger.debug('prim_vmprocess_spawn')
    procid = proc.call_interpreter(True, 'spawn')
    logger.debug('prim_vmprocess_spawn got id:' + str(procid))
    proc.reg('r_rdp')['self'] = proc.interpreter.processes[str(procid)]
    logger.debug('prim_vmprocess_spawn, proc::' + P(proc.reg('r_rdp')['self'],1,True))
    return proc.reg('r_rp')

def prim_vmprocess_from_procid(proc):
    raise Exception("TODO")
    logger.debug('prim_vmprocess_from_procid')
    proc.reg('r_drp')['self'] = proc.interpreter.process_for(proc.locals['procid'])

def _update_frames(proc, mproc):
    logger.debug('_update_frames')
    raw_frames = mproc['self'].all_stack_frames()
    VMStackFrameClass = proc.interpreter.get_core_class('VMStackFrame')
    frames = [proc.interpreter.alloc_object(VMStackFrameClass,{'self':x}) for x in raw_frames]
    mproc['frames'] = frames

def prim_vmprocess_handshake_target(proc):
    logger.debug('prim_vmprocess_handshake_target')
    proc.init_debugging_session()
    _update_frames(proc, proc.reg('r_rdp'))

def prim_vmprocess_step_into(proc):
    logger.debug('prim_vmprocess_step_into')
    proc.call_target_process(True, proc.reg('r_rdp')['self'].procid, 'step_into')
    _update_frames(proc, proc.reg('r_rdp'))

def prim_vmprocess_step_over(proc):
    logger.debug('prim_vmprocess_step_over')
    proc.call_target_process(True, proc.reg('r_rdp')['self'].procid, 'step_over')
    _update_frames(proc, proc.reg('r_rdp'))

def prim_vmprocess_step_line(proc):
    logger.debug('prim_vmprocess_step_line')
    proc.call_target_process(True, proc.reg('r_rdp')['self'].procid, 'step_line')
    _update_frames(proc, proc.reg('r_rdp'))

def prim_vmprocess_continue(proc):
    logger.debug('prim_vmprocess_continue')
    _proc = proc.reg('r_rdp')['self']
    proc.call_target_process(True, _proc.procid, 'continue')
    _update_frames(proc, proc.reg('r_rdp'))

def prim_vmprocess_terminate(proc):
    logger.debug('prim_vmprocess_terminate')
    _proc = proc.reg('r_rdp')['self']
    _proc.terminate()

def prim_vmprocess_stack_frames(proc):
    logger.debug('prim_vmprocess_stack_frames')
    return proc.reg('r_rdp')['frames']

def prim_vmprocess_break_at(proc):
    logger.debug('prim_vmprocess_break_at')
    _proc = proc.reg('r_rdp')['self']
    cfun = proc.locals()['cfun']
    line = proc.locals()['line']
    _proc.break_at(cfun, line)
    return proc.reg('r_rdp')

def prim_vmprocess_eval_in_frame(proc):
    logger.debug('prim_vmprocess_eval_in_frame')
    text = proc.locals()['text']
    frame_idx = proc.locals()['frame_index']
    _proc = proc.reg('r_rdp')['self']
    logger.debug('sending text to be evaluated at procid: ' + str(_proc.procid))
    proc.call_target_process(True, _proc.procid, 'eval_in_frame', text, frame_idx)
    ex = _proc.shared['eval.exception']
    res = _proc.shared['eval.result']
    logger.debug('prim_vmprocess_eval_in_frame result: ' + P((ex,res),1,True))
    if ex:
        proc.throw(ex)
    return res

def prim_vmprocess_continue_to_line(proc):
    logger.debug('prim_vmprocess_continue_to_line')
    _proc = proc.reg('r_rdp')['self']
    line = proc.locals()['line']
    logger.debug('going to line:' + str(line))
    proc.call_target_process(True, _proc.procid, 'continue_to_line', line)
    _update_frames(proc, proc.reg('r_rdp'))

def prim_vmprocess_reload_frame(proc):
    logger.debug('prim_vmprocess_reload_frame')
    _proc = proc.reg('r_rdp')['self']
    proc.call_target_process(True, _proc.procid, 'reload_frame')
    _update_frames(proc, proc.reg('r_rdp'))

def prim_vmprocess_rewind_and_break(proc):
    logger.debug('prim_vmprocess_rewind_and_break')
    frames_count = proc.locals()['frames_count']
    to_line = proc.locals()['to_line']
    _proc = proc.reg('r_rdp')['self']
    proc.call_target_process(True, _proc.procid, 'rewind_and_break', frames_count, to_line)
    _update_frames(proc, proc.reg('r_rdp'))

def prim_vmprocess_current(proc):
    mproc = proc.mm_self()
    _update_frames(proc, mproc)
    return mproc

def prim_vmprocess_detach(proc):
    logger.debug('prim_vmprocess_detach')
    _proc = proc.reg('r_rdp')['self']
    proc.call_target_process(False, _proc.procid, 'detach')


def prim_vmprocess_run(proc):
    logger.debug("prim_vmprocess_run");
    fn = proc.locals()['fn']
    args = proc.locals()['args']
    _proc = proc.reg('r_rdp')['self']
    _proc.setup('exec_fun', fn, args)
    logger.debug("prim_vmprocess_run sending proc_start");
    proc.call_interpreter(True, 'proc_start', _proc.procid)

def prim_vmprocess_run_and_halt(proc):
    logger.debug("prim_vmprocess_run_and_halt");
    fn = proc.locals()['fn']
    args = proc.locals()['args']
    _proc = proc.reg('r_rdp')['self']
    _proc.setup('exec_fun', fn, args, 'paused')
    logger.debug("prim_vmprocess_run_and_halt sending proc_start");
    proc.call_interpreter(True, 'proc_start', _proc.procid)

def prim_vmprocess_is_running(proc):
    raise Exception("TODO")
    return proc.is_alive()

def prim_vmprocess_halt(proc):
    logger.debug("prim_vmprocess_halt")
    proc.debug_me()

def prim_vmprocess_halt_fn(proc):
    logger.debug("prim_vmprocess_halt_fn")
    proc.debug_me()
    logger.debug("prim_vmprocess_halt_fn: calling fn...")
    fn = proc.locals()['fn']
    args = proc.locals()['args']
    return proc.setup_and_run_unprotected(None, None, fn['compiled_function']['name'], fn, args, True)

def prim_vmprocess_last_exception(proc):
    _proc = proc.reg('r_rdp')['self']
    return _proc.shared['last_exception']

def prim_vmprocess_debug_on_exception(proc):
    logger.debug("prim_vmprocess_debug_on_exception")
    _proc = proc.reg('r_rdp')['self']
    _proc.shared['flag_debug_on_exception'] = True

#### VMStackFrame

def _create_location_info(proc, frame):
    ast = frame['r_ip']
    outer_cfun = proc.interpreter.shitty_get_module_from_cfunction(frame['r_cp']['compiled_function'])
    start_line = ast.start_line - outer_cfun['line']
    start_col = ast.start_col
    end_line = ast.end_line - outer_cfun['line']
    end_col = ast.end_col
    res = {"start_line":start_line, "start_col": start_col, "end_line": end_line, "end_col":end_col}
    return res

def prim_vmstackframe_instruction_pointer(proc):
    frame = _lookup_field(proc, proc.reg('r_rp'), 'self')
    ast = frame['r_ip']
    if ast == None: #first frame is none
        return None

    # ast has the line numbering relative to the entire module filre.
    # we need to make it relative to the toplevel function
    return _create_location_info(proc, frame)



def prim_vmstackframe_module_pointer(proc):
    frame = _lookup_field(proc, proc.reg('r_rp'), 'self')
    if frame['r_cp'] != None: #first frame is none
        return frame['r_cp']['module']
    else:
        return None

def prim_vmstackframe_context_pointer(proc):
    frame = proc.reg('r_rdp')['self']
    return frame['r_cp']

def prim_vmstackframe_receiver_pointer(proc):
    frame = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return frame['r_rp']

def prim_vmstackframe_receiver_data_pointer(proc):
    frame = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return frame['r_rdp']

def prim_vmstackframe_environment_pointer(proc):
    frame = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return frame['r_ep']

def prim_vmstackframe_locals(proc):
    frame = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return frame['locals']

def prim_io_print(proc):
    arg = proc.locals()['arg']
    if isinstance(arg, dict):
        P(arg,4)
    else:
        print arg

def prim_ast_line(proc):
    br()

def prim_object_to_string(proc):
    obj = proc.reg('r_rp')
    if obj == None:
        return "null"
    elif isinstance(obj, dict) and '_vt' in obj:
        return P(obj,1, True) #dirt and limited, str does inifinite loop
    else:
        return str(obj)

def _to_source(obj):
    if obj == None:
        return 'null'
    elif isinstance(obj, dict) and '@tag' in obj:
        return  '<' + obj['@tag'] + '>'
    else:
        return P(obj,1,True)

def prim_object_to_source(proc):
    obj = proc.reg('r_rp')
    return _to_source(obj)

def prim_object_send(proc):
    selector_str = proc.locals()['selector']['self'] # should be a symbol
    args = proc.locals()['args']
    receiver = proc.reg('r_rp')
    return proc.do_send(receiver, selector_str, args)

def prim_object_equal(proc):
    return obj_eq(proc.reg('r_rp'), proc.locals()['other'])

def prim_object_not_equal(proc):
    return not obj_eq(proc.reg('r_rp'), proc.locals()['other'])

# def prim_string_replace(i):
#     return re.sub(i.locals()['what'],i.locals()['for'],i.reg('r_rp'))

def prim_symbol_to_string(proc):
    return proc.reg('r_rdp')['self']

def prim_string_size(proc):
    return len(proc.reg('r_rp'))

def prim_string_concat(proc):
    return proc.reg('r_rp') + proc.locals()['arg']

def prim_string_from(proc):
    return proc.reg('r_rp')[proc.locals()['idx']:]

def prim_string_rindex(proc):
    try:
        return proc.reg('r_rdp').rindex(proc.locals()['arg'])
    except ValueError:
        return -1

def prim_string_leq(proc):
    return proc.reg('r_rp') <= proc.locals()['other']

def prim_string_geq(proc):
    return proc.reg('r_rp') >= proc.locals()['other']

def prim_string_count(proc):
    return proc.reg('r_rp').count(proc.locals()['sub'])

def prim_string_substring(proc):
    return proc.reg('r_rp')[proc.locals()['from']:proc.locals()['from']+proc.locals()['count']]

def prim_string_split(proc):
    return proc.reg('r_rp').split(proc.locals()['sep'])

def prim_string_to_symbol(proc):
    return proc.interpreter.interned_symbol_for(proc.reg('r_rdp'))

def prim_string_char_code(proc):
    return ord(proc.reg('r_rdp'))

def prim_string_each(proc):
    for x in proc.reg('r_rdp'):
        proc.setup_and_run_fun(None, None, 'fn', proc.locals()['fn'], [x], True)

def prim_module_instance_compiled_module(proc):
    return proc.reg('r_rdp')['_vt']['compiled_module']

def prim_compiled_function_new_top_level(proc):
    name = proc.locals()['name']
    text = proc.locals()['text']
    owner = proc.locals()['owner']
    flag = proc.locals()['flag']
    cfun = proc.interpreter.compile_top_level(proc, name,text,owner, flag)
    cfun['is_top_level'] = True
    return cfun

def _create_closure(proc, text, outer, is_embedded):
    cfun = proc.interpreter.compile_closure(proc, text, outer)
    cfun['is_embedded'] = is_embedded #text is embeded in outter cfun?
    cfun['is_top_level'] = False
    return cfun

def prim_compiled_function_new_closure(proc):
    text = proc.locals()['text']
    outer = proc.locals()['outer_cfun']
    is_embedded = proc.locals()['is_embedded']
    return _create_closure(proc, text, outer, is_embedded)

def prim_compiled_function_set_code(proc):
    if proc.reg('r_rdp')['is_top_level']:
        return proc.interpreter.recompile_top_level(proc, proc.reg('r_rdp'), proc.locals()['code'])
    else:
        # NOTE: if a Function of this cfun already exists, its env should
        # be updated, otherwise calling it will screw up var access.
        return proc.interpreter.recompile_closure(proc, proc.reg('r_rdp'), proc.locals()['code'])



# hackutility for as_context..
class ProxyEnv():
    # dshared requires this hack
    def __getattr__(self, name):
        return self.__dict__[name]

    def __eq__(self, other):
        return id(other) == id(self)

    def __ne__(self, other):
        return id(other) != id(self)

    def __init__(self, frame, cp_env_table):
        self.frame = frame
        self.table = dict(cp_env_table.items())

        self.i = len(self.table)
        for name in frame['locals'].keys():
            self.table[str(self.i)] = name
            self.i = self.i + 1

    def __getitem__(self, key):
        if isinstance(key, (int, long)) or (isinstance(key,str) and key.isdigit()):
            return self.frame['locals'][self.table[str(key)]]
        else:
            #r_rdp,r_rp
            return self.frame[key]

    def __setitem__(self, key, val):
        if isinstance(key, (int, long)) or (isinstance(key,str) and key.isdigit()):
            self.frame['locals'][self.table[str(key)]] = val
        else:
            #r_rdp,r_rp
            self.frame[key] = val
    def __contains__(self, item):
        return item in ['r_rdp', 'r_rp'] + map(str,range(0,self.i))

    def __iter__(self):
        ret = {}
        for key, name in self.table.iteritems():
            ret[key] = self.frame['locals'][name]
        return ret.iteritems()

def _compiled_function_as_context_with_frame(cfun, proc, imod, frame):
    # If the frame passed has an r_ep, just use it as our env.
    # else, we need to construct a proxy env capable of changing
    # frame.locals(). We also need to patch r_rp function's env_table
    # so that all frame.locals()/this are mapped there (so env_lookup works).
    if frame['r_ep'] != None:
        env = frame['r_ep']
        # Though this cfun was already constructed with an outer_cfun,
        # now we have a frame, so it's cp should be our outer_cfun! :(
        cfun['outer_cfun'] = frame['r_cp']['compiled_function']
        # patching our env_table's indexes
        begin = max(map(int,cfun['outer_cfun']['env_table'].keys())) + 1
        cfun['env_table'] = dict([(str(int(x)+begin),y) for x,y in cfun['env_table'].iteritems()])
    else:
        env = ProxyEnv(frame, cfun['env_table'])
        cfun['env_table'] = env.table # patching the env_table
    ret = proc.interpreter.compiled_function_to_context(cfun, env, imod)
    return ret

def prim_compiled_function_as_context_with_frame(proc):
    frame = proc.locals()['frame']['self']
    imod = frame['r_cp']['module']
    return _compiled_function_as_context_with_frame(proc.reg('r_rp'), proc, imod, frame)

def prim_compiled_function_as_context_with_vars(proc):
    var_dict = proc.locals()['vars']
    env = dict(var_dict.items()) # we do 'del' below, lets no fuck with the parameter
    if 'this' in env: #another hack for binding this
        this = env['this']
        del env['this']
    else:
        this = None
    begin = len(proc.reg('r_rp')['env_table'])
    proc.reg('r_rp')['env_table'].update(dict(zip(map(str,range(begin,begin+len(env.keys()))), env.keys())))
    env = dict([(key,env[name]) for key,name in proc.reg('r_rp')['env_table'].iteritems() if name in env])
    env['r_rdp'] = env['r_rp'] = this
    ret = proc.interpreter.compiled_function_to_context(proc.reg('r_rp'), env, proc.locals()['imodule'])
    return ret

def prim_compiled_function_instantiate(proc):
    return proc.interpreter.function_from_cfunction(proc.reg('r_rp'), proc.locals()['imodule'])

def prim_context_apply(proc):
    # fn.apply([...args...])
    args = proc.locals()['args']
    return proc.setup_and_run_fun(None, None, proc.reg('r_rdp')['compiled_function']['name'], proc.reg('r_rdp'), args, True)

def prim_context_get_env(proc):
    #warning 1: no checking if the env idexes are ordered. we assume so.
    #warning 2: only the outer env is returned (closures declaring variables
    #                                            are not contemplated. its
    #                                            a TODO).
    env = dict(proc.reg('r_rdp')['env'].items())
    env_table = dict(proc.reg('r_rdp')['compiled_function']['env_table'].items())
    ret = {}
    for k,v in env_table.items():
        if k in env:
            ret[v] = env[k]
    return ret

def _context_with_frame(proc, text, frame):
    code = code = "fun() {" + text + "}"
    imod = frame['r_cp']['module']
    outer = proc.reg('r_cp')['compiled_function']
    cfn = _create_closure(proc, code, outer, False)
    return _compiled_function_as_context_with_frame(cfn, proc, imod, frame)

def prim_context_with_frame(proc):
    text = proc.locals()['text']
    frame = proc.locals()['frame']
    return _context_with_frame(proc, text, frame)

def prim_function_apply(proc):
    # fn.apply([...args...])
    args = proc.locals()['args']
    return proc.setup_and_run_fun(None, None, proc.reg('r_rdp')['compiled_function']['name'], proc.reg('r_rdp'), args, True)

def prim_number_plus(proc):
    return proc.reg('r_rp') + proc.locals()['arg']

def prim_number_minus(proc):
    return proc.reg('r_rp') - proc.locals()['arg']

def prim_number_lst(proc):
    return proc.reg('r_rp') < proc.locals()['arg']

def prim_number_lsteq(proc):
    return proc.reg('r_rp') <= proc.locals()['arg']

def prim_number_grteq(proc):
    return proc.reg('r_rp') >= proc.locals()['arg']

def prim_dictionary_plus(proc):
    return dict(proc.reg('r_rp').items() + proc.locals()['arg'].items())

def prim_dictionary_size(proc):
    return len(proc.reg('r_rp'))

def prim_dictionary_sorted_each(proc):
    d = proc.reg('r_rdp')
    dsorted = [(key,d[key]) for key in sorted(d.keys())]
    for t in dsorted:
        proc.setup_and_run_fun(None, None, 'fn', proc.locals()['fn'], [t[0],t[1]], True)

def prim_dictionary_each(proc):
    for key,val in proc.reg('r_rdp').iteritems():
        proc.setup_and_run_fun(None, None, 'fn', proc.locals()['fn'], [key,val], True)

def prim_dictionary_has(proc):
    return proc.locals()['key'] in proc.reg('r_rdp')

def prim_dictionary_set(proc):
    proc.reg('r_rdp')['key'] = proc.locals()["value"]

def prim_dictionary_remove(proc):
    del proc.reg('r_rdp')[proc.locals()['key']]

def prim_dictionary_map(proc):
    ret = []
    for key,val in proc.reg('r_rdp').iteritems():
        ret.append(proc.setup_and_run_fun(None, None, 'fn', proc.locals()['fn'], [key,val], True))
    return ret

def prim_get_compiled_module(proc):
    return proc.interpreter.get_vt(proc.locals()['module'])['compiled_module']

def prim_get_compiled_class(proc):
    return proc.locals()['klass']['compiled_class']

def _compiled_class_constructors(cclass):
    return dict([(name, cfun) for name,cfun in cclass['own_methods'].iteritems() if cfun['is_ctor']])

def prim_compiled_class_constructors(proc):
    return _compiled_class_constructors(proc.reg('r_rdp'))

def prim_compiled_class_rename(proc):
    klass = proc.reg('r_rdp')
    new_name = proc.locals()['name']
    old_name = klass['name']
    cmod = klass['module']

    klass['name'] = new_name

    del cmod['compiled_classes'][old_name]
    cmod['compiled_classes'][new_name] = klass

    for imod in proc.interpreter.imodules():
        if id_eq(imod['_vt']['compiled_module'], cmod):
            iklass = imod[old_name]
            del imod[old_name]
            imod[new_name] = iklass

            del imod['_vt']['dict'][old_name] # del accessor
            imod['_vt']['dict'][new_name] = proc.interpreter.create_accessor_method(imod, new_name)

    return proc.reg('r_rp')

def prim_compiled_class_set_fields(proc):
    klass = proc.reg('r_rdp')
    fields = proc.locals()['fields']

    def diff(a, b):
        b = set(b)
        return [aa for aa in a if aa not in b]

    rm =  diff(klass['fields'], fields)
    add = diff(fields, klass['fields'])

    class_id = klass['@id']
    if class_id in proc.interpreter.shared['instances_by_class']:
        for _, obj in proc.interpreter.shared['instances_by_class'][class_id].iteritems():
            for f in rm: del obj[f]
            for f in add: obj[f] = None

    klass['fields'] = fields

def _compiled_class_class_methods(cclass):
    return dict([(name, cfun) for name,cfun in cclass['own_methods'].iteritems() if not cfun['is_ctor']])

def prim_compiled_class_class_methods(proc):
    return _compiled_class_class_methods(proc.reg('r_rdp')['own_methods'])

def prim_compiled_class_add_method(proc):
    cfun = proc.locals()['cfun']
    flag = proc.locals()['flag']

    # add the function to the compiled class
    if flag['self'] == 'instance_method':
        proc.reg('r_rdp')['methods'][cfun['name']] = cfun
    elif flag['self'] == 'class_method' or flag['self'] == 'constructor':
        proc.reg('r_rdp')['own_methods'][cfun['name']] = cfun
    else:
        proc.throw_with_message("Unknown flag: " + P(flag,1,True))


    # add the function to the instantiated classes:
    for imod in proc.interpreter.imodules():
        a = imod['_vt']['compiled_module']
        b = proc.reg('r_rdp')['module']
        if id_eq(a, b):
            fun = proc.interpreter.function_from_cfunction(cfun, imod)

            if flag['self'] == 'instance_method':
                imod[proc.reg('r_rdp')['name']]['dict'][cfun['name']] = fun
            else:
                imod[proc.reg('r_rdp')['name']]['_vt']['dict'][cfun['name']] = fun

    return proc.reg('r_rp')

def prim_compiled_class_remove_method(proc):
    name = proc.locals()['name']
    flag = proc.locals()['flag']
    cclass = proc.reg('r_rdp')

    # removing function from compiled class
    if flag['self'] == 'instance_method':
        del proc.reg('r_rdp')['methods'][name]
    else:
        del proc.reg('r_rdp')['own_methods'][name]

    # removing function from instances:
    for imod in proc.interpreter.imodules():
        if id_eq(imod['_vt']['compiled_module'], proc.reg('r_rdp')['module']):
            if flag['self'] == 'instance_method':
                del imod[proc.reg('r_rdp')['name']]['dict'][name]
            else:
                del imod[proc.reg('r_rdp')['name']]['_vt']['dict'][name]

def prim_compiled_class_method_flag(proc):
    if proc.locals()['cfun'] in proc.reg('r_rdp')['methods'].values():
        return proc.interpreter.interned_symbol_for("instance_method")
    elif proc.locals()['cfun'] in proc.reg('r_rdp')['own_methods'].values():
        return proc.interpreter.interned_symbol_for("class_method")
    return None

def prim_mirror_fields(proc):
    mirrored = proc.reg('r_rdp')['mirrored']
    if hasattr(mirrored, 'keys'):
        return mirrored.keys()
    elif isinstance(mirrored, list):
        return [str(x) for x in range(0,len(mirrored))]
    else:
        return []

def prim_mirror_value_for(proc):
    if isinstance(proc.reg('r_rdp')['mirrored'], list):
        return proc.reg('r_rdp')['mirrored'][int(proc.locals()['name'])]
    else:
        return proc.reg('r_rdp')['mirrored'][proc.locals()['name']]

def prim_mirror_set_value_for(proc):
    if isinstance(proc.reg('r_rdp')['mirrored'], list):
        proc.reg('r_rdp')['mirrored'][int(proc.locals()['name'])] = proc.locals()['value']
    else:
        proc.reg('r_rdp')['mirrored'][proc.locals()['name']] = proc.locals()['value']
    return proc.reg('r_rp')

def prim_mirror_vt(proc):
    return proc.interpreter.get_vt(proc.locals()['obj'])

def prim_list_ctor(proc):
    for x in proc.locals()['lst']:
         proc.reg('r_rdp').append(x)
    return proc.reg('r_rp')

def prim_list_each(proc):
    for x in proc.reg('r_rdp'):
        proc.setup_and_run_fun(None, None, 'fn', proc.locals()['fn'], [x], True)

def prim_list_first(proc):
    return proc.reg('r_rdp')[0]

def prim_list_rest(proc):
    return proc.reg('r_rdp')[1:]

def prim_list_reversed(proc):
    return list(reversed(proc.reg('r_rdp')))

def prim_list_prepend(proc):
    proc.reg('r_rdp').insert(0, proc.locals()['arg'])
    return proc.reg('r_rdp')

def prim_list_append(proc):
    proc.reg('r_rdp').append(proc.locals()['arg'])
    return proc.reg('r_rdp')

def prim_list_get(proc):
    return proc.reg('r_rdp')[proc.locals()['n']]

def prim_list_size(proc):
    return len(proc.reg('r_rdp'))

def prim_list_map(proc):
    return [proc.setup_and_run_fun(None, None, 'fn', proc.locals()['fn'], [x], True) for x in proc.reg('r_rdp')]

def prim_list_plus(proc):
    return list(proc.reg('r_rdp') + proc.locals()['arg'])

def prim_list_has(proc):
    return proc.locals()['value'] in proc.reg('r_rdp')

def prim_list_join(proc):
    return proc.locals()['sep'].join(proc.reg('r_rdp'))

def prim_list_to_string(proc):
    ret = []
    for x in proc.reg('r_rdp'):
        if hasattr(x,'__str__'):
            ret.append(str(x))
        else:
            ret.append(P(x,1,True))
    return ', '.join(ret)

def prim_io_file_contents(proc):
    return open(proc.locals()['path']).read()

def prim_compiled_module_remove_function(proc):
    name = proc.locals()['name']
    cmod = proc.reg('r_rdp')

    # removing function from compiled module
    del cmod['compiled_functions'][name]

    # removing function from instances:
    for imod in proc.interpreter.imodules():
        if id_eq(imod['_vt']['compiled_module'], cmod):
            # remove the getter...
            del imod['_vt']['dict'][name]
            # ...and the python dict field
            del imod[name]

def prim_compiled_module_add_function(proc):
    cfun = proc.locals()['cfun']
    cmod = proc.reg('r_rdp')

    # add the function to the compiled module
    cmod['compiled_functions'][cfun['name']] = cfun

    # add the function to the module instances:
    for imod in proc.interpreter.imodules():
        if id_eq(imod['_vt']['compiled_module'], cmod):
            fun = proc.interpreter.function_from_cfunction(cfun, imod)

            # add the getter
            imod['_vt']['dict'][cfun['name']] = fun

            # add the function to the dict:
            imod[cfun['name']] = fun

def prim_compiled_module_new_class(proc):
    name = proc.locals()['name']
    super_name = proc.locals()['super_name']
    cmod = proc.reg('r_rdp')
    klass = proc.interpreter.create_compiled_class({"name": name,
                                                    "super_class_name":super_name,
                                                    "module": cmod})

    cmod['compiled_classes'][name] = klass
    for imod in proc.interpreter.imodules():
        if id_eq(imod['_vt']['compiled_module'], cmod):
            cb = proc.interpreter.new_object({"_vt": proc.interpreter.get_core_class("Behavior"),
                                              "parent": proc.interpreter.get_core_class("ObjectBehavior")['_vt'],
                                              "dict": {},
                                              "@tag": name + "Behavior"})

            imod['_vt']['dict'][name] = proc.interpreter.create_accessor_method(imod, name)
            imod[name] = proc.interpreter.create_class({"_vt": cb,
                                                        "parent": proc.interpreter.get_core_class("Object"),
                                                        "dict": {},
                                                        "compiled_class": klass,
                                                        "@tag": name + " Class"})

    return klass

def prim_compiled_module_add_class(proc):
    klass = proc.locals()['klass']
    name = klass['name']
    cmod = proc.reg('r_rdp')
    klass['module'] = cmod

    cmod['compiled_classes'][name] = klass

    for imod in proc.interpreter.imodules():
        if id_eq(imod['_vt']['compiled_module'], cmod):
            cb = proc.interpreter.new_object({"_vt": proc.interpreter.get_core_class("Behavior"),
                                              "parent": proc.lookup_in_modules(klass["super_class_name"], imod)['_vt'],
                                              "dict": proc.interpreter.compiled_functions_to_functions(klass["own_methods"], imod),
                                              "@tag": name + "Behavior"})


            imod['_vt']['dict'][name] = proc.interpreter.create_accessor_method(imod, name)
            imod[name] = proc.interpreter.create_class({"_vt": cb,
                                                        "parent": proc.interpreter.get_core_class("Object"),
                                                        "dict": {},
                                                        "compiled_class": klass,
                                                        "@tag": name + " Class"})

    return klass


def prim_compiled_module_remove_class(proc):
    name = proc.locals()['name']
    cmod = proc.reg('r_rdp')

    del cmod['compiled_classes'][name]

    # removing function from instances:
    for imod in proc.interpreter.imodules():
        if id_eq(imod['_vt']['compiled_module'], cmod):
            del imod[name]
            del imod['_vt']['dict'][name]

    return proc.reg('r_rp')

def prim_compiled_module_default_parameter_for(proc):
    # really simplorious, for the moment
    if proc.locals()['name'] in proc.reg('r_rdp')['default_params']:
        return proc.reg('r_rdp')['default_params'][proc.locals()['name']]['value']
    else:
        return None

def prim_compiled_module_set_default_parameter(proc):
    # really simplorious, for the moment
    proc.reg('r_rdp')['default_params'][proc.locals()['name']] = \
        {'name': proc.locals()['name'], 'type':'lib','value':proc.locals()['m']}

    return proc.reg('r_rp')

def prim_compiled_module_instantiate(proc):
    core = proc.interpreter.get_core_module()
    return proc.interpreter.instantiate_module(proc, proc.reg('r_rdp'), proc.locals()['args'], core)

# lookup the inner handle of the actual binding object
# which is set -- this is because a hierarchy of delegates
# will have the same field (say, QMainWindow < QWidget both
# have 'self', but QMainWindow.new() sets 'self' only to
# its leaf object, not to the delegated QWidget
def _lookup_field(proc, mobj, name):
    if mobj == None:
        proc.throw_with_message('field not found: ' + name)
    if name in mobj and mobj[name] != None:
        return mobj[name]
    else:
        return _lookup_field(proc, mobj['_delegate'], name)

# def _set_field(mobj, name, value):
#     if mobj == None:
#         raise Exception('field not found: ' + name)
#     if name in mobj:
#         mobj[name] = value
#     else:
#         _set_field(mobj['_delegate'], name, value)
### Qt

## return a meme instance given something from pyqt
def _meme_instance(proc, obj):
    global _qt_imodule
    mapping = {
        QtGui.QListWidgetItem: _qt_imodule["QListWidgetItem"],
        QtWebKit.QWebView: _qt_imodule['QWebView'],
        QtWebKit.QWebFrame: _qt_imodule['QWebFrame'],
        QtWebKit.QWebPage: _qt_imodule['QWebPage'],
        QtWebKit.QWebElement: _qt_imodule['QWebElement'],
        QtGui.QAction: _qt_imodule['QAction'],
        QtCore.QUrl: _qt_imodule['QUrl'],
        QtGui.QMenuBar: _qt_imodule['QMenuBar'],
        _QTableWidgetItem: _qt_imodule['QTableWidgetItem'],
        scintilla_editor.MemeQsciScintilla: _qt_imodule['QsciScintilla']}

    if obj == None:
        return obj
    elif isinstance(obj, basestring):
        return obj# {"_vt":i.get_core_class("String"), 'self':obj}
    elif isinstance(obj, dict) and '_vt' not in obj:
        return obj#{"_vt":i.get_core_class("Dictionary"), 'self':obj}
    elif isinstance(obj, int) or isinstance(obj, long):
        return obj#{"_vt":i.get_core_class("Number"), 'self':obj}
    elif isinstance(obj, list):
        return obj#{"_vt":i.get_core_class("List"), 'self':obj}
    # elif isinstance(obj, QtCore.QUrl):
    #     return qstring_to_str(obj.toString()) # TODO: currently ignoring QUrl objects
    elif obj.__class__ in mapping: # NOTE: should be a qt instance
        if hasattr(obj, 'meme_instance'): # all qt objetcs should have this one day
            logger.debug("Returning meme_instance for " + str(obj))
            return obj.meme_instance
        else:
            return {"_vt":mapping[obj.__class__], 'self':obj}
    else:
        logger.warning("*** WARNING: object has no memetalk mapping specified:")
        logger.warning(P(obj,1,True))
        return None

def qstring_to_str(qstring):
    #return str(qstring.toUtf8()).decode("utf-8")
    return unicode(qstring.toUtf8(), "utf-8")


def prim_exit(proc):
    proc.exit(proc.locals()['code'])

#################### Qt bindings ####################


# QApplication

def qt_identity(x):
    return str(x) + "::" + str(id(x))

# dshared requires this hack
def qt__getattr__(self, name):
    return self.__dict__[name]

QApplication.__getattr__ = qt__getattr__
QWidget.__getattr__ = qt__getattr__
#_QMainWindow
QMainWindow.__getattr__ = qt__getattr__
QPlainTextEdit.__getattr__ = qt__getattr__
QMenu.__getattr__ = qt__getattr__
QMenuBar.__getattr__ = qt__getattr__
QAction.__getattr__ = qt__getattr__
QShortcut.__getattr__ = qt__getattr__
QTextCursor.__getattr__ = qt__getattr__
QLayout.__getattr__ = qt__getattr__
QVBoxLayout.__getattr__ = qt__getattr__
QHBoxLayout.__getattr__ = qt__getattr__
QListWidget.__getattr__ = qt__getattr__
QListWidgetItem.__getattr__ = qt__getattr__
QLineEdit.__getattr__ = qt__getattr__
QLabel.__getattr__ = qt__getattr__
QHeaderView.__getattr__ = qt__getattr__
QComboBox.__getattr__ = qt__getattr__
QTableWidget.__getattr__ = qt__getattr__
QTableWidgetItem.__getattr__ = qt__getattr__
QWebView.__getattr__ = qt__getattr__
QWebPage.__getattr__ = qt__getattr__
QWebFrame.__getattr__ = qt__getattr__
QWebElement.__getattr__ = qt__getattr__
QUrl.__getattr__ = qt__getattr__
scintilla_editor.__getattr__ = qt__getattr__

_refs = [] # global referenfes to qt widgets so
           # the GC doesn't wip them out

def prim_qt_qapplication_new(proc):
    global _qt_imodule
    _qt_imodule = proc.reg('r_mp')
    proc.reg('r_rdp')['self'] = QApplication(sys.argv)
    return proc.reg('r_rdp')

def prim_qt_qapplication_focus_widget(proc):
    w = QApplication.focusWidget()
    return _meme_instance(proc,w)

def prim_qt_qapplication_exit(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.exit(proc.locals()['code'])

def prim_qt_qapplication_exec(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qtobj.exec_()


def prim_qt_qeventloop_new(proc):
    qev = QEventLoop()
    proc.reg('r_rdp')['self'] = qev
    return proc.reg('r_rp')

def prim_qt_qeventloop_exec(proc):
    return proc.reg('r_rdp')['self'].exec_()

def prim_qt_qeventloop_exit(proc):
    return proc.reg('r_rdp')['self'].exit(proc.locals()['code'])

# QWidget

def prim_qt_qwidget_new(proc):
    parent = proc.locals()['parent']
    if parent != None:
        qt_parent = _lookup_field(proc, parent, 'self')
        proc.reg('r_rdp')["self"] = QtGui.QWidget(qt_parent)
    else:
        proc.reg('r_rdp')['self'] = QtGui.QWidget()
    global _refs
    _refs.append(proc.reg('r_rdp')['self'])
    return proc.reg('r_rp')


def prim_qt_qwidget_set_focus(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setFocus()
    return proc.reg('r_rp')


def prim_qt_qwidget_set_maximum_height(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setMaximumHeight(proc.locals()['h'])
    return proc.reg('r_rp')


def prim_qt_qwidget_set_minimum_size(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setMinimumSize(proc.locals()['w'],proc.locals()['h'])
    return proc.reg('r_rp')


def prim_qt_qwidget_set_minimum_width(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setMinimumWidth(proc.locals()['w'])
    return proc.reg('r_rp')



def prim_qt_qwidget_show(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.show()
    return proc.reg('r_rp')


def prim_qt_qwidget_hide(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.hide()
    return proc.reg('r_rp')


def prim_qt_qwidget_set_window_title(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setWindowTitle(proc.locals()['title'])
    return proc.reg('r_rp')


def prim_qt_qwidget_resize(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    w = proc.locals()["w"]
    h = proc.locals()["h"]
    qtobj.resize(w,h)
    return proc.reg('r_rp')


def prim_qt_qwidget_add_action(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qt_action = _lookup_field(proc, proc.locals()["action"], 'self')
    qtobj.addAction(qt_action)
    return proc.reg('r_rp')


def prim_qt_qwidget_set_maximum_width(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setMaximumWidth(proc.locals()['w'])
    return proc.reg('r_rp')


def prim_qt_qwidget_connect(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    signal = proc.locals()['signal']
    slot = proc.locals()['slot']
    def callback(*rest):
        args = []
        for arg in rest:
            args.append(_meme_instance(proc,arg))
        try:
            proc.setup_and_run_fun(None, None, '<?>', slot, args, True)
        except proc.interpreter.py_memetalk_exception() as e:
            logger.debug("prim_qt_qwidget_connect: Exception raised: " + e.mmobj()['message'])
            logger.debug("Python trace:")
            logger.debug(e.mmobj()['py_trace'])
            logger.debug("Memetalk trace:")
            logger.debug(e.mmobj()['mtrace'])

    getattr(qtobj,signal).connect(callback)
    return proc.reg('r_rp')


def prim_qt_qwidget_actions(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    r = [_meme_instance(proc, x) for x in qtobj.actions()]
    return r


def prim_qt_qwidget_set_stylesheet(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setStyleSheet(proc.locals()['s'])
    return proc.reg('r_rp')


def prim_qt_qwidget_is_visible(proc):
    return _lookup_field(proc, proc.reg('r_rp'), 'self').isVisible()


def prim_qt_qwidget_close(proc):
    return _lookup_field(proc, proc.reg('r_rp'), 'self').close()


def prim_qt_qwidget_has_focus(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qtobj.hasFocus()

# QMainWindow

class _QMainWindow(QtGui.QMainWindow):
    def __init__(self, get_proc, meme_obj, parent=None):
        super(QMainWindow, self).__init__(parent)
        self.meme_obj = meme_obj
        self.get_proc = get_proc

    def closeEvent(self, ev):
        if 'closeEvent' in self.meme_obj['_vt']['dict']:
            fn = self.meme_obj['_vt']['dict']['closeEvent']
            self.get_proc().setup_and_run_fun(self.meme_obj, self.meme_obj, 'closeEvent', fn, [], True)

    # dshared requires this hack
    def __getattr__(self, name):
        return self.__dict__[name]


def prim_qt_qmainwindow_new(proc):
    #dshared is unable to store Process object. Indirection to the rescue
    def get_proc():
        return proc
    proc.reg('r_rdp')['self'] = _QMainWindow(get_proc, proc.reg('r_rp'))
    global _refs
    _refs.append(proc.reg('r_rdp')['self'])
    return proc.reg('r_rp')


def prim_qt_qmainwindow_set_central_widget(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setCentralWidget(_lookup_field(proc, proc.locals()['widget'],'self'))
    return proc.reg('r_rp')

# Warning! QMenuBar inherits QWidget!
# however we did not create the QWidget instance delegate
# here.

def prim_qt_qmainwindow_menu_bar(proc):
    QMenuBarClass = proc.reg('r_mp')["QMenuBar"]
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qt_bar_instance = qtobj.menuBar()
    return proc.interpreter.alloc_object(QMenuBarClass, {'self':qt_bar_instance})


def prim_qt_qmainwindow_status_bar(proc):
    QWidgetClass = proc.reg('r_mp')["QWidget"]
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qt_bar_instance = qtobj.statusBar()
    return proc.interpreter.alloc_object(QWidgetClass, {'self':qt_bar_instance})

# QPlainTextEdit

def prim_qt_qplaintextedit_new(proc):
    # new(QWidget parent)
    parent = proc.locals()['parent']
    if parent != None:
        qt_parent = _lookup_field(proc, parent, 'self')
    else:
        qt_parent = None
    proc.reg('r_rdp')["self"] = QtGui.QPlainTextEdit(qt_parent)
    return proc.reg('r_rp')


def prim_qt_qplaintextedit_set_tabstop_width(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setTabStopWidth(proc.locals()["val"])
    return proc.reg('r_rp')


def prim_qt_qplaintextedit_text_cursor(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qt_cursor = qtobj.textCursor()
    QTextCursorClass = proc.reg('r_mp')["QTextCursor"]
    return proc.interpreter.alloc_object(QTextCursorClass, {'self':qt_cursor})


def prim_qt_qplaintextedit_set_text_cursor(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    cursor = _lookup_field(proc, proc.locals()['cursor'], 'self')
    qtobj.setTextCursor(cursor)
    return proc.reg('r_rp')


def prim_qt_qplaintextedit_set_plain_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setPlainText(proc.locals()['text'])
    return proc.reg('r_rp')



def prim_qt_qplaintextedit_to_plain_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qstring_to_str(qtobj.toPlainText())



# QMenuBar
# Warning! QMenuBar inherits QWidget!
# however we did not create the QWidget instance delegate
# here.

def prim_qt_qmenubar_add_menu(proc):
    label = proc.locals()["str"]
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qt_menu_instance = qtobj.addMenu(label)
    QMenuClass = proc.reg('r_mp')["QMenu"]
    return proc.interpreter.alloc_object(QMenuClass, {'self':qt_menu_instance})

# QAction

def prim_qt_qaction_new(proc):
    label = proc.locals()['label']
    parent = proc.locals()['parent']
    if parent != None:
        qt_parent = _lookup_field(proc, parent, 'self')
    else:
        qt_parent = None
    proc.reg('r_rdp')["self"] = QtGui.QAction(label,qt_parent)
    return proc.reg('r_rp')


def prim_qt_qaction_connect(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    signal = proc.locals()["signal"]
    slot = proc.locals()["slot"]
    def callback(*rest):
        try:
            proc.setup_and_run_fun(None, None, '<?>', slot, [], True)
        except proc.interpreter.py_memetalk_exception() as e:
            logger.debug("Exception raised: " + e.mmobj()['message'])
            logger.debug("Python trace:")
            logger.debug(e.mmobj()['py_trace'])
            logger.debug("Memetalk trace:")
            logger.debug(e.mmobj()['mtrace'])

    getattr(qtobj,signal).connect(callback)
    return proc.reg('r_rp')


def prim_qt_qaction_set_shortcut(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setShortcut(proc.locals()["shortcut"]);
    return proc.reg('r_rp')


def prim_qt_qaction_set_shortcut_context(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setShortcutContext(proc.locals()["context"]);
    return proc.reg('r_rp')


def prim_qt_qaction_set_enabled(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setEnabled(proc.locals()["val"]);
    return proc.reg('r_rp')


def prim_qt_qshortcut_set_context(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setContext(proc.locals()["context"]);
    return proc.reg('r_rp')


def prim_qt_qshortcut_new(proc):
    keys = proc.locals()['sc']
    parent = proc.locals()['parent']
    slot = proc.locals()['slot']

    if parent != None:
        qt_parent = _lookup_field(proc, parent, 'self')
    else:
        qt_parent = None

    proc.reg('r_rdp')["self"] = QtGui.QShortcut(keys,qt_parent)
    proc.reg('r_rdp')["self"].setKey(keys)

    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    def callback(*rest):
        proc.setup_and_run_fun(None, None, '<?>', slot, [], True)
        logger.debug('callback slot: ' + str(slot['compiled_function']['body']))
        logger.debug('callback: eventloop proc equal? ' + str(proc == eventloop_processes[-1]['proc']))

    qtobj.activated.connect(callback)
    return proc.reg('r_rp')

#     def callback():
#         proc.setup_and_run_fun(None, None, '<?>', fn, [], True)

#     shortcut = QtGui.QShortcut(QtGui.QKeySequence(keys), qtobj, callback)
#     shortcut.setContext(QtCore.Qt.WidgetShortcut)
#     return proc.reg('r_rp')

# QTextCursor


def prim_qt_qtextcursor_selected_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')

    # Qt docs say \u2029 is a paragraph, and should be replaced with \n
    # TODO: make this replace works written in memetalk instead of hardcoded
    # here.
    return qstring_to_str(qtobj.selectedText()).replace(u'\u2029', '\n')


def prim_qt_qtextcursor_selection_end(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qtobj.selectionEnd()


def prim_qt_qtextcursor_set_position(proc):
    pos = proc.locals()['pos']
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setPosition(pos)
    return proc.reg('r_rp')


def prim_qt_qtextcursor_insert_text(proc):
    text = proc.locals()['text']
    string = text.decode('string_escape')
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.insertText(string)
    return proc.reg('r_rp')


def prim_qt_qtextcursor_drag_right(proc):
    length = proc.locals()['len']
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    res = qtobj.movePosition(
        QtGui.QTextCursor.Left, QtGui.QTextCursor.KeepAnchor, length)
    return True if res else False

#QLayout


def prim_qt_qlayout_add_widget(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    parent = _lookup_field(proc, proc.locals()['widget'], 'self')
    qtobj.addWidget(parent)
    return proc.reg('r_rp')

# QVBoxLayout

def prim_qt_qvboxlayout_new(proc):
    parent = proc.locals()['parent']
    if parent != None:
        qtobj = _lookup_field(proc, parent, 'self')
        proc.reg('r_rdp')['self'] = QtGui.QVBoxLayout(qtobj)
    else:
        proc.reg('r_rdp')['self'] = QtGui.QVBoxLayout()
    return proc.reg('r_rp')


def prim_qt_qvboxlayout_add_layout(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    parent = _lookup_field(proc, proc.locals()['layout'], 'self')
    qtobj.addLayout(parent)
    return proc.reg('r_rp')


def prim_qt_qvboxlayout_add_widget(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    w = _lookup_field(proc, proc.locals()['widget'], 'self')
    qtobj.addWidget(w)
    return proc.reg('r_rp')



# QHBoxLayout

def prim_qt_qhboxlayout_new(proc):
    parent = proc.locals()['parent']
    if parent != None:
        qtobj = _lookup_field(proc, parent, 'self')
        proc.reg('r_rdp')['self'] = QtGui.QHBoxLayout(qtobj)
    else:
        proc.reg('r_rdp')['self'] = QtGui.QHBoxLayout()
    return proc.reg('r_rp')


def prim_qt_qhboxlayout_add_widget(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    w = _lookup_field(proc, proc.locals()['widget'], 'self')
    qtobj.addWidget(w)
    return proc.reg('r_rp')


def prim_qt_qhboxlayout_set_contents_margins(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setContentsMargins(proc.locals()['l'],proc.locals()['t'],proc.locals()['r'],proc.locals()['b'])
    return proc.reg('r_rp')

# QListWidget

def prim_qt_qlistwidget_new(proc):
    parent = proc.locals()['parent']
    if parent != None:
        qtobj = _lookup_field(proc, parent, 'self')
        proc.reg('r_rdp')['self'] = QtGui.QListWidget(qtobj)
    else:
        proc.reg('r_rdp')['self'] = QtGui.QListWidget()
    return proc.reg('r_rp')


def prim_qt_qlistwidget_current_item(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return _meme_instance(proc,qtobj.currentItem())

# QListWidgetItem

def prim_qt_qlistwidgetitem_new(proc):
    txt = proc.locals()['text']
    parent = proc.locals()['parent']
    qtparent = None if parent == None else _lookup_field(proc, parent, 'self')
    proc.reg('r_rdp')['self'] = QtGui.QListWidgetItem(txt,qtparent)
    return proc.reg('r_rp')


def prim_qt_qlistwidgetitem_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qstring_to_str(qtobj.text())


#QLineEdit

def prim_qt_qlineedit_new(proc):
    parent = proc.locals()['parent']
    if parent != None:
        qtobj = _lookup_field(proc, parent, 'self')
        proc.reg('r_rdp')['self'] = QtGui.QLineEdit(qtobj)
    else:
        proc.reg('r_rdp')['self'] = QtGui.QLineEdit()
    return proc.reg('r_rp')


def prim_qt_qlineedit_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qstring_to_str(qtobj.text())


def prim_qt_qlineedit_set_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setText(proc.locals()['text'])
    return proc.reg('r_rp')


def prim_qt_qlineedit_selected_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qstring_to_str(qtobj.selectedText())


def prim_qt_qlineedit_select_all(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.selectAll()
    return proc.reg('r_rp')


def prim_qt_qlineedit_set_selection(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setSelection(proc.locals()['start'], proc.locals()['length'])
    return proc.reg('r_rp')



def prim_qt_qlabel_new(proc):
    parent = proc.locals()['parent']
    if parent != None:
        qtobj = _lookup_field(proc, parent, 'self')
        proc.reg('r_rdp')['self'] = QtGui.QLabel(qtobj)
    else:
        proc.reg('r_rdp')['self'] = QtGui.QLabel()
    return proc.reg('r_rp')


def prim_qt_qlabel_set_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setText(proc.locals()['text'])
    return proc.reg('r_rp')


def prim_qt_qheaderview_hide(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.hide()
    return proc.reg('r_rp')


def prim_qt_qheaderview_set_stretch_last_section(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setStretchLastSection(proc.locals()['val'])
    return proc.reg('r_rp')



def prim_qt_qcombobox_new(proc):
    parent = proc.locals()['parent']
    if parent != None:
        qtobj = _lookup_field(proc, parent, 'self')
        proc.reg('r_rdp')['self'] = QtGui.QComboBox(qtobj)
    else:
        proc.reg('r_rdp')['self'] = QtGui.QComboBox()
    return proc.reg('r_rp')


def prim_qt_qcombobox_add_item(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.addItem(proc.locals()['item'])
    return proc.reg('r_rp')


def prim_qt_qcombobox_set_current_index(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setCurrentIndex(proc.locals()['i'])
    return proc.reg('r_rp')


def prim_qt_qcombobox_clear(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.clear()
    return proc.reg('r_rp')

def prim_qt_qcombobox_count(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qtobj.count()

## QTableWidget


def prim_qt_qtablewidget_new(proc):
    parent = proc.locals()['parent']
    #rows = proc.locals()['rows']
    #cols = proc.locals()['cols']
    if parent != None:
        qtobj = _lookup_field(proc, parent, 'self')
        proc.reg('r_rdp')['self'] = QtGui.QTableWidget(qtobj)
    else:
        proc.reg('r_rdp')['self'] = QtGui.QTableWidget()
    return proc.reg('r_rp')


def prim_qt_qtablewidget_set_horizontal_header_labels(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setHorizontalHeaderLabels(proc.locals()['labels'])
    return proc.reg('r_rp')


def prim_qt_qtablewidget_vertical_header(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    header = qtobj.verticalHeader()
    QHeaderViewClass = proc.reg('r_mp')['QHeaderView']
    return proc.interpreter.alloc_object(QHeaderViewClass, {'self':header})


def prim_qt_qtablewidget_set_selection_mode(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setSelectionMode(proc.locals()['mode'])
    return proc.reg('r_rp')


def prim_qt_qtablewidget_horizontal_header(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    header = qtobj.horizontalHeader()
    QHeaderViewClass = proc.reg('r_mp')['QHeaderView']
    return proc.interpreter.alloc_object(QHeaderViewClass, {'self':header})


def prim_qt_qtablewidget_set_item(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    item = _lookup_field(proc, proc.locals()['item'], 'self')
    qtobj.setItem(proc.locals()['line'], proc.locals()['col'], item)
    return proc.reg('r_rp')


def prim_qt_qtablewidget_set_sorting_enabled(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setSortingEnabled(proc.locals()['val'])
    return proc.reg('r_rp')


def prim_qt_qtablewidget_clear(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.clear()
    return proc.reg('r_rp')


def prim_qt_qtablewidget_set_row_count(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setRowCount(proc.locals()['count'])
    return proc.reg('r_rp')


def prim_qt_qtablewidget_set_column_count(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setColumnCount(proc.locals()['count'])
    return proc.reg('r_rp')


# QTableWidgetItem
class _QTableWidgetItem(QTableWidgetItem):
    def __init__(self, meme, label):
        super(QTableWidgetItem, self).__init__(label)
        self.meme_instance = meme


def prim_qt_qtablewidgetitem_new(proc):
    proc.reg('r_rdp')['self'] = _QTableWidgetItem(proc.reg('r_rp'), proc.locals()['label'])
    return proc.reg('r_rp')


def prim_qt_qtablewidgetitem_set_flags(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setFlags(Qt.ItemFlag(proc.locals()['flags']))
    return proc.reg('r_rp')



def prim_qt_qwebview_new(proc):
    parent = proc.locals()['parent']
    if parent != None:
        qtobj = _lookup_field(proc, parent, 'self')
        proc.reg('r_rdp')['self'] = QtWebKit.QWebView(qtobj)
    else:
        proc.reg('r_rdp')['self'] = QtWebKit.QWebView()
    return proc.reg('r_rp')


def prim_qt_qwebview_set_url(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setUrl(QtCore.QUrl(proc.locals()['url']))
    return proc.reg('r_rp')

# note: this is not from Qt
# def prim_qt_qwebview_load_url(proc):
#     qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
#     qtobj.setHtml(open(proc.locals()['url']).read())
#     return proc.reg('r_rp')


def prim_qt_qwebview_set_html(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setHtml(proc.locals()['html'])
    return proc.reg('r_rp')


def prim_qt_qwebview_page(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return _meme_instance(proc,qtobj.page())


def prim_qt_qwebpage_set_link_delegation_policy(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setLinkDelegationPolicy(proc.locals()['policy'])
    return proc.reg('r_rp')


def prim_qt_qwebpage_main_frame(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return _meme_instance(proc,qtobj.mainFrame())


def prim_qt_qwebframe_document_element(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    x = _meme_instance(proc,qtobj.documentElement())
    return x


def prim_qt_qwebframe_scroll_to_anchor(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qtobj.scrollToAnchor(proc.locals()['anchor'])


def prim_qt_qwebelement_find_first(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return _meme_instance(proc,qtobj.findFirst(proc.locals()['str']))


def prim_qt_qwebelement_append_outside(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    if isinstance(proc.locals()['val'], basestring):
        qtobj.appendOutside(proc.locals()['val'])
    else:
        qtobj.appendOutside(_lookup_field(proc, proc.locals()['val'], 'self'))
    return proc.reg('r_rp')


def prim_qt_qwebelement_append_inside(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    if isinstance(proc.locals()['val'], basestring):
        qtobj.appendInside(proc.locals()['val'])
    else:
        qtobj.appendInside(_lookup_field(proc, proc.locals()['val'], 'self'))
    return proc.reg('r_rp')


def prim_qt_qwebelement_set_plain_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setPlainText(proc.locals()['str'])
    return proc.reg('r_rp')


def prim_qt_qwebelement_clone(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return _meme_instance(proc,qtobj.clone())


def prim_qt_qwebelement_set_style_property(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setStyleProperty(proc.locals()['name'], proc.locals()['val'])
    return proc.reg('r_rp')


def prim_qt_qwebelement_set_attribute(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setAttribute(proc.locals()['name'], proc.locals()['val'])
    return proc.reg('r_rp')



def prim_qt_qwebelement_to_outer_xml(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qstring_to_str(qtobj.toOuterXml())


def prim_qt_qwebelement_set_inner_xml(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setInnerXml(proc.locals()['xml'])
    return proc.reg('r_rp')


def prim_qt_qwebelement_take_from_document(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qtobj.takeFromDocument()


def prim_qt_qurl_has_fragment(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qtobj.hasFragment()


def prim_qt_qurl_fragment(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qstring_to_str(qtobj.fragment())


def prim_qt_qurl_query_item_value(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qstring_to_str(qtobj.queryItemValue(proc.locals()['name']))


def prim_qt_qurl_has_query_item(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qtobj.hasQueryItem(proc.locals()['name'])


def prim_qt_qurl_path(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qstring_to_str(qtobj.path())


def prim_qt_qurl_to_string(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qstring_to_str(qtobj.toString())



# scintilla

def prim_qt_scintilla_editor_new(proc):
    parent = proc.locals()['parent']
    if parent != None:
        qtobj = _lookup_field(proc, parent, 'self')
        proc.reg('r_rdp')['self'] = scintilla_editor.MemeQsciScintilla(proc.reg('r_rp'), qtobj)
    else:
        proc.reg('r_rdp')['self'] = scintilla_editor.MemeQsciScintilla(proc.reg('r_rp'))
    return proc.reg('r_rp')


def prim_qt_scintilla_editor_set_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setText(proc.locals()['text'])
    return proc.reg('r_rp')


def prim_qt_scintilla_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qstring_to_str(qtobj.text())


def prim_qt_scintilla_cut(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.cut()
    return proc.reg('r_rp')


def prim_qt_scintilla_copy(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.copy()
    return proc.reg('r_rp')


def prim_qt_scintilla_paste(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.paste()
    return proc.reg('r_rp')


def prim_qt_scintilla_redo(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.redo()
    return proc.reg('r_rp')

def prim_qt_scintilla_undo(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.undo()
    return proc.reg('r_rp')


def prim_qt_scintilla_set_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.set_text(proc.locals()['text'])
    return proc.reg('r_rp')


def prim_qt_scintilla_saved(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.saved()
    return proc.reg('r_rp')


def prim_qt_scintilla_paused_at_line(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.paused_at_line(proc.locals()['start_line'], proc.locals()['start_col'], proc.locals()['end_line'], proc.locals()['end_col'])
    return proc.reg('r_rp')


def prim_qt_scintilla_selected_text(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qstring_to_str(qtobj.selectedText())


def prim_qt_scintilla_get_cursor_position(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    line, idx = qtobj.getCursorPosition()
    return {"line": line, "index":idx}


def prim_qt_scintilla_insert_at(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.insertAt(proc.locals()['text'], proc.locals()['line'], proc.locals()['index'])
    return proc.reg('r_rp')

def prim_qt_scintilla_append(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.append(proc.locals()['text'])
    return proc.reg('r_rp')

def prim_qt_scintilla_lines(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    return qtobj.lines()

def prim_qt_scintilla_get_selection(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    start_line, start_index, end_line, end_index = qtobj.getSelection()
    return {"start_line":start_line, "start_index": start_index, 'end_line': end_line, 'end_index': end_index}


def prim_qt_scintilla_set_selection(proc):
    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.setSelection(proc.locals()['start_line'], proc.locals()['start_index'], proc.locals()['end_line'], proc.locals()['end_index'])
    return proc.reg('r_rp')



_factories = []

def prim_qt_extra_qwebpage_enable_plugins(proc):
    name = proc.locals()['name']
    fn = proc.locals()['fn']
    class WebPluginFactory(QWebPluginFactory):
        def __init__(self, parent = None):
            QWebPluginFactory.__init__(self, parent)
        def create(self, mimeType, url, _names, _values):
            names = map(str, _names)
            values =  map(str, _values)
            if mimeType == "x-pyqt/" + name:
                try:
                    mobj = proc.setup_and_run_fun(None, None, '<?>', fn, [dict(zip(names,values))], True)
                    return _lookup_field(proc, mobj, 'self')
                except proc.interpreter.py_memetalk_exception() as e:
                    logger.debug("Exception raised: " + e.mmobj()['message'])
                    logger.debug("Python trace:")
                    logger.debug(e.mmobj()['py_trace'])
                    logger.debug("Memetalk trace:")
                    logger.debug(e.mmobj()['mtrace'])

        def plugins(self):
            plugin = QWebPluginFactory.Plugin()
            plugin.name = "PyQt Widget"
            plugin.description = "An example Web plugin written with PyQt."
            mimeType = QWebPluginFactory.MimeType()
            mimeType.name = "x-pyqt/widget"
            mimeType.description = "PyQt widget"
            mimeType.fileExtensions = []
            plugin.mimeTypes = [mimeType]
            return [plugin]

    global _factories # If this is gc'd, it segfaults
    factory = WebPluginFactory()
    _factories.append(factory) #we may have many instances of qwebpage

    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    QWebSettings.globalSettings().setAttribute(QWebSettings.PluginsEnabled, True)
    qtobj.setPluginFactory(factory)
    return proc.reg('r_rp')

def prim_test_files(proc):
    path = MODULES_PATH + "/../../tests"
    return [path + "/" + f for f in listdir(path) if isfile(join(path,f))]

def prim_exception_unprotected(proc):
    return proc.setup_and_run_unprotected(None, None, 'fn', proc.locals()['fn'], [], True)

def prim_test_import(proc):
    cmod = proc.interpreter.compiled_module_by_filepath(proc, proc.locals()['filepath'])
    return proc.interpreter.instantiate_module(proc, cmod, {}, proc.reg('r_cp')['module'])


def prim_http_get(proc):
    import httplib2
    resp, content = httplib2.Http().request(proc.locals()['url'])
    return content

def prim_parse_json(proc):
    import json
    return json.loads(proc.locals['str'])


######################### optimizing #############################


def _top_level_compiled_function(cfun):
      # if (this.isTopLevel()) {
      #   return null;
      # } else {
      #   if (@outer_cfun.isTopLevel()) {
      #     return @outer_cfun;
      #   } else {
      #     return @outer_cfun.topLevelCompiledFunction();
      #   }
      # }
    if cfun['is_top_level']:
        return None
    elif cfun['outer_cfun']['is_top_level']:
        return cfun['outer_cfun']
    else:
        return _top_level_compiled_function(cfun['outer_cfun'])

def prim_idez_debugger_ui_update_info(proc):
  # // if (0 <= @frame_index) {
  # //   @editor.setText(@execFrames.codeFor(@frame_index));
  # //   @editor.setFrameIndex(@frame_index);
  # //   var locInfo = @execFrames.locationInfoFor(@frame_index);
  # //   @editor.pausedAtLine(locInfo["start_line"], locInfo["start_col"], locInfo["end_line"], locInfo["end_col"]);
  # //   @localVarList.clear();
  # //   @fieldVarList.clear();
  # //   if (@shouldUpdateVars) {
  # //     @localVarList.loadFrame(@execFrames.frame(@frame_index));
  # //     @fieldVarList.loadReceiver(@execFrames.frame(@frame_index));
  # //   }
  # // }
    logger.debug('prim_idez_debugger_ui_update_info')
    frame_index = proc.reg("r_rp")['frame_index']
    if frame_index < 0:
        return proc.reg('r_rp')

    editor = proc.reg("r_rp")['editor']
    qteditor = _lookup_field(proc, editor, 'self')
    _proc = proc.reg("r_rp")['process']['self']
    raw_frames = _proc.all_stack_frames()

    # ExecutionFrames.codeFor:
    cp = raw_frames[frame_index]['r_cp']['compiled_function']
    if cp['is_top_level']:
        code_for = cp['text']
    elif  cp['is_embedded']:
        code_for = _top_level_compiled_function(cp)['text']
    else:
        code_for = cp['text']

    qteditor.setText(code_for)

    # DebuggerEditor.setFrameIndex
    editor['frame_index'] = frame_index

    # ExecutionFrame.locationInfoFor
    locInfo = _create_location_info(_proc, raw_frames[frame_index])
    qteditor.paused_at_line(locInfo["start_line"], locInfo["start_col"], locInfo["end_line"], locInfo["end_col"])

    localVarList = proc.reg("r_rp")['localVarList']
    fieldVarList = proc.reg("r_rp")['fieldVarList']

    _lookup_field(proc, localVarList, 'self').clear()
    _lookup_field(proc, fieldVarList, 'self').clear()

    VMStackFrameClass = proc.interpreter.get_core_class('VMStackFrame')
    mm_frame = proc.interpreter.alloc_object(VMStackFrameClass,{'self':raw_frames[frame_index]})

    if proc.reg('r_rdp')['shouldUpdateVars']:
        proc.do_send(localVarList, 'loadFrame', [mm_frame])
        proc.do_send(fieldVarList, 'loadReceiver', [mm_frame])

def _owner_full_name(owner):
    if 'module' in owner: #compiled class
        return owner['module']['name'] + "/" + owner['name']
    else:
        return owner['name']

def _cfun_full_name(cfun):
      # if (this.isTopLevel()) {
      #   if (@owner) {
      #     return @owner.fullName() + "::" + @name;
      #   } else {
      #     return "<deep-sea>::" + @name; //We have no class [eg. Behavior function]
      #   }
      # } else {
      #   return this.topLevelCompiledFunction().fullName() + "[" + @name + "]";
      # }

    if cfun['is_top_level']:
        if cfun['owner']:
            return _owner_full_name(cfun['owner']) + "::" + cfun['name']
        else:
            return "<deep-sea>::" + cfun['name']
    else:
        return _cfun_full_name(_top_level_compiled_function(cfun)) + "[" + cfun['name'] + "]"

def prim_idez_stack_combo_update_info(proc):
#   // this.clear();
#   // @frames.names().each(fun(name) {
#   //   this.addItem(name);
#   // });
#   // this.setCurrentIndex(@frames.size() - 1);
# }
    logger.debug('prim_idez_stack_combo_update_info')
    mself = proc.reg("r_rp")
    this = _lookup_field(proc, proc.reg("r_rp"), 'self')

    mself['updating'] = True

    this.clear()

    execFrames = proc.reg("r_rdp")['frames']
    _proc = execFrames['vmproc']['self']
    raw_frames = _proc.all_stack_frames()
    names = [_cfun_full_name(frame['r_cp']['compiled_function']) for frame in raw_frames]
    for name in names:
        this.addItem(name) # this shit triggers it

    idx = len(raw_frames)-1
    this.setCurrentIndex(idx)
    mself['updating'] = False
    fn = mself['on_update']
    proc.setup_and_run_fun(None, None, fn['compiled_function']['name'], fn, [idx], True)

def prim_idez_variablelistwidget_load_frame(proc):
  # this.clear();
  # this.setHorizontalHeaderLabels(['Name', 'Value']);
  # if (frame.environmentPointer) {
  #   var env = frame.environmentPointer;
  #   var table = frame.contextPointer.compiledFunction.env_table;
  #   this.setRowCount(env.size);
  #   var _this = env['r_rp'];
  #   this.setItem(0, 0, VariableItem.new("this", _this));
  #   this.setItem(0, 1, VariableItem.new(_this.toSource, _this));
  #   var _dthis = env['r_rdp'];
  #   this.setItem(1, 0, VariableItem.new("@this", _dthis));
  #   this.setItem(1, 1, VariableItem.new(_dthis.toSource, _dthis));
  #   var i = 2;
  #   table.each(fun(idx, varname) {
  #     var entry = env[idx];
  #     this.setItem(i, 0, VariableItem.new(varname, entry));
  #     this.setItem(i, 1, VariableItem.new(entry.toSource, entry));
  #     i = i + 1;
  #   });
  # } else {
  #   this.setRowCount(2 + frame.locals.size);
  #   var _this = frame.receiverPointer;
  #   this.setItem(0, 0, VariableItem.new("this", _this));
  #   this.setItem(0, 1, VariableItem.new(_this.toSource, _this));
  #   var _dthis = frame.receiverDataPointer;
  #   this.setItem(1, 0, VariableItem.new("@this", _dthis));
  #   this.setItem(1, 1, VariableItem.new(_dthis.toSource, _dthis));
  #   var i = 2;
  #   frame.locals.each(fun(name,val) {
  #     this.setItem(i, 0, VariableItem.new(name, frame.locals[name]));
  #     this.setItem(i, 1, VariableItem.new(val.toSource, frame.locals[name]));
  #     i = i + 1;
  #   });
  # }

    logger.debug('prim_idez_variablelistwidget_load_frame')
    global _qt_imodule
    VariableItem = proc.reg('r_mp')['VariableItem']

    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.clear()
    qtobj.setHorizontalHeaderLabels(['Name', 'Value']);
    frame = proc.locals()['frame']['self']
    if frame['r_ep']:
        env = frame['r_ep']
        table = frame['r_cp']['compiled_function']['env_table']
        qtobj.setRowCount(len(env))
        _this = env['r_rp']

        varitem = proc.do_send(VariableItem, 'new', ['this', _this])
        qtobj.setItem(0,0, _lookup_field(proc, varitem, 'self'))
        varitem = proc.do_send(VariableItem, 'new', [_to_source(_this), _this])
        qtobj.setItem(0,1, _lookup_field(proc, varitem, 'self'))

        _dthis = env['r_rdp']
        varitem = proc.do_send(VariableItem, 'new', ['@this', _dthis])
        qtobj.setItem(1,0, _lookup_field(proc, varitem, 'self'))
        varitem = proc.do_send(VariableItem, 'new', [_to_source(_dthis), _dthis])
        qtobj.setItem(1,1, _lookup_field(proc, varitem, 'self'))

        i = 2
        for idx, varname in table.iteritems():
            entry = env[idx]
            varitem = proc.do_send(VariableItem, 'new', [varname, entry])
            qtobj.setItem(i,0, _lookup_field(proc, varitem, 'self'))
            varitem = proc.do_send(VariableItem, 'new', [_to_source(entry), entry])
            qtobj.setItem(i,1, _lookup_field(proc, varitem, 'self'))
            i = i + 1
    else:
        qtobj.setRowCount(2 + len(frame['locals']))
        _this = frame['r_rp']

        varitem = proc.do_send(VariableItem, 'new', ['this', _this])
        qtobj.setItem(0,0, _lookup_field(proc, varitem, 'self'))
        varitem = proc.do_send(VariableItem, 'new', [_to_source(_this), _this])
        qtobj.setItem(0,1, _lookup_field(proc, varitem, 'self'))

        _dthis = frame['r_rdp']
        varitem = proc.do_send(VariableItem, 'new', ['@this', _dthis])
        qtobj.setItem(1,0, _lookup_field(proc, varitem, 'self'))
        varitem = proc.do_send(VariableItem, 'new', [_to_source(_dthis), _dthis])
        qtobj.setItem(1,1, _lookup_field(proc, varitem, 'self'))

        i = 2
        for name, val in frame['locals'].iteritems():
            varitem = proc.do_send(VariableItem, 'new', [name, frame['locals'][name]])
            qtobj.setItem(i,0, _lookup_field(proc, varitem, 'self'))
            varitem = proc.do_send(VariableItem, 'new', [_to_source(val), val])
            qtobj.setItem(i,1, _lookup_field(proc, varitem, 'self'))
            i = i + 1

def prim_idez_variablelistwidget_load_receiver(proc):
  # this.clear();
  # this.setHorizontalHeaderLabels(['Name', 'Value']);
  # var _dthis = null;
  # if (frame.environmentPointer) {
  #   _dthis = frame.environmentPointer['r_rdp'];
  # } else {
  #   _dthis = frame.receiverDataPointer;
  # }
  # var mirror = Mirror.new(_dthis);
  # var fields = mirror.fields();
  # this.setRowCount(fields.size);
  # var i = 0;
  # fields.each(fun(name) {
  #   this.setItem(i, 0, VariableItem.new(name, mirror.valueFor(name)));
  #   this.setItem(i, 1, VariableItem.new(mirror.valueFor(name).toString, mirror.valueFor(name)));
  #   i = i + 1;
  # });

    logger.debug('prim_idez_variablelistwidget_load_frame')

    global _qt_imodule
    VariableItem = proc.reg('r_mp')['VariableItem']

    qtobj = _lookup_field(proc, proc.reg('r_rp'), 'self')
    qtobj.clear()
    qtobj.setHorizontalHeaderLabels(['Name', 'Value']);
    frame = proc.locals()['frame']['self']

    _dthis = None
    if frame['r_ep']:
        _dthis = frame['r_ep']['r_rdp']
    else:
        _dthis = frame['r_rdp']

    #Mirror.fields
    if hasattr(_dthis, 'keys'):
        fields = _dthis.keys()
    elif isinstance(_dthis, list):
        fields = [str(x) for x in range(0,len(_dthis))]
    else:
        fields = []

    qtobj.setRowCount(len(fields))
    i = 0
    for name in fields:
        varitem = proc.do_send(VariableItem, 'new', [name, _dthis[name]])
        qtobj.setItem(i, 0, _lookup_field(proc, varitem, 'self'))
        varitem = proc.do_send(VariableItem, 'new', [_to_source(_dthis[name]), _dthis[name]])
        qtobj.setItem(i, 1, _lookup_field(proc, varitem, 'self'))
        i = i + 1




def prim_idez_module_explorer_show_class(proc):
# fun(module_name, class_name) {
#   this.show_module_basic(module_name);
#   var doc = @webview.page().mainFrame().documentElement();
#   var mlist = doc.findFirst("#menu-listing .link-list");
#   var klass = @current_cmodule.compiled_classes[class_name];
#   var div = doc.findFirst("div[id=templates] .class_template").clone();
#   doc.findFirst(".module-classes").appendInside(div);
#   div.setStyleProperty("display","block");
#   div.setAttribute("id", klass.fullName);
#   div.findFirst(".class_name").setPlainText(klass.name);
#   div.findFirst(".super_class").setPlainText(klass.super_class_name);
#   div.findFirst(".fields_list").setPlainText(klass.fields.toString());
#   var ctors = div.findFirst(".constructors");
#   ctors.setAttribute("id", "ctors_" + klass.name);
#   klass.constructors.sortedEach(fun(name, cfn) {
#     mlist.appendInside("<li><a href='#" + cfn.fullName + "'>*" + cfn.fullName + "</a></li>");
#     this.show_function(cfn, "ctor_method", "div[id='ctors_" + klass.name + "']");
#   });
#   var ims = div.findFirst(".instance_methods");
#   ims.setAttribute("id", "imethods_" + klass.name);
#   klass.instanceMethods.sortedEach(fun(name, cfn) {
#    mlist.appendInside("<li><a href='#" + cfn.fullName + "'>" + cfn.fullName + "</a></li>");
#    this.show_function(cfn, "instance_method", "div[id='imethods_" + klass.name + "']");
#   });
#   var cms = div.findFirst(".class_methods");
#   cms.setAttribute("id", "cmethods_" + klass.name);
#   klass.classMethods.sortedEach(fun(name, cfn) {
#     mlist.appendInside("<li><a href='#" + cfn.fullName + "'>[" + cfn.fullName + "]</a></li>");
#     this.show_function(cfn, "class_method", "div[id='cmethods_" + klass.name + "']");
#   });
# }
    module_name = proc.locals()['module_name']
    class_name = proc.locals()['class_name']
    qt_webview = _lookup_field(proc, proc.reg('r_rdp')['webview'], 'self')
    current_cmodule = proc.reg('r_rdp')['current_cmodule']
    proc.do_send(proc.reg('r_rp'), 'show_module_basic', [module_name])
    doc = qt_webview.page().mainFrame().documentElement()
    mlist = doc.findFirst("#menu-listing .link-list")
    klass = current_cmodule['compiled_classes'][class_name]
    div = doc.findFirst("div[id=templates] .class_template").clone()
    doc.findFirst(".module-classes").appendInside(div)
    div.setStyleProperty("display", "block")
    div.setAttribute("id", _owner_full_name(klass))
    div.findFirst(".class_name").setPlainText(klass['name'])
    div.findFirst(".super_class").setPlainText(klass['super_class_name'])
    div.findFirst(".fields_list").setPlainText(str(klass['fields']))
    ctors = div.findFirst(".constructors")
    ctors.setAttribute("id", "ctors_" + klass['name'])

    #sortedEach
    d = _compiled_class_constructors(klass)
    dsorted = [(key,d[key]) for key in sorted(d.keys())]
    for t in dsorted:
        name = t[0]
        cfn = t[1]
        mlist.appendInside("<li><a href='#" + _cfun_full_name(cfn) + "'>*" + _cfun_full_name(cfn) + "</a></li>");
        proc.do_send(proc.reg('r_rp'), 'show_function', [cfn, "ctor_method", "div[id='ctors_" + klass['name'] + "']"])

    ims = div.findFirst(".instance_methods")
    ims.setAttribute("id", "imethods_" + klass['name'])

    #sortedEach
    d = klass['methods']
    dsorted = [(key,d[key]) for key in sorted(d.keys())]
    for t in dsorted:
        name = t[0]
        cfn = t[1]
        mlist.appendInside("<li><a href='#" + _cfun_full_name(cfn) + "'>" + _cfun_full_name(cfn) + "</a></li>");
        proc.do_send(proc.reg('r_rp'), 'show_function', [cfn, "instance_method", "div[id='imethods_" + klass['name'] + "']"])


    cms = div.findFirst(".class_methods")
    cms.setAttribute("id", "cmethods_" + klass['name'])

    #sortedEach
    d = _compiled_class_class_methods(klass)
    dsorted = [(key,d[key]) for key in sorted(d.keys())]
    for t in dsorted:
        name = t[0]
        cfn = t[1]
        mlist.appendInside("<li><a href='#" + _cfun_full_name(cfn) + "'>[" + _cfun_full_name(cfn) + "]</a></li>");
        proc.do_send(proc.reg('r_rp'), 'show_function', [cfn, "class_method", "div[id='cmethods_" + klass['name'] + "']"])


def prim_idez_module_explorer_show_function(proc):
# show_function: fun(cfn, function_type, parent_sel) {
#   var doc = @webview.page().mainFrame().documentElement();
#   var parent = doc.findFirst(parent_sel);
#   var div = doc.findFirst("div[id=templates] .function_template").clone();
#   div.setAttribute("id", cfn.fullName);
#   div.setStyleProperty("display","block");
#   div.findFirst(".function_name").setPlainText(cfn.fullName);
#   div.findFirst(".function_name").setAttribute("name",cfn.fullName);
#   if (Mirror.vtFor(cfn.owner) == CompiledClass) {
#     div.findFirst(".method-click-advice").setAttribute("href","/show-editor?class=" + cfn.owner.name + "&name=" + cfn.name + "&type=" + function_type + "&id=" + cfn.fullName);
#   } else {
#     div.findFirst(".method-click-advice").setAttribute("href","/show-editor?name=" + cfn.name  + "&id=" + cfn.fullName + "&type=" + function_type);
#   }
#   parent.appendInside(div);
# }
    cfn = proc.locals()['cfn']
    function_type = proc.locals()['function_type']
    parent_sel = proc.locals()['parent_sel']
    qt_webview = _lookup_field(proc, proc.reg('r_rdp')['webview'], 'self')

    doc = qt_webview.page().mainFrame().documentElement()
    parent = doc.findFirst(parent_sel)

    div = doc.findFirst("div[id=templates] .function_template").clone()
    div.setAttribute("id", _cfun_full_name(cfn))
    div.setStyleProperty("display", "block")
    div.findFirst(".function_name").setPlainText(_cfun_full_name(cfn))
    div.findFirst(".function_name").setAttribute("name", _cfun_full_name(cfn))

    if 'module' in cfn['owner']: #compiled class
        div.findFirst(".method-click-advice").setAttribute("href","/show-editor?class=" + cfn['owner']['name'] +\
                                                               "&name=" + cfn['name'] + "&type=" + function_type + "&id=" + _cfun_full_name(cfn))
    else:
        div.findFirst(".method-click-advice").setAttribute("href","/show-editor?name=" + cfn['name'] +\
                                                               "&id=" + _cfun_full_name(cfn) + "&type=" + function_type);

    parent.appendInside(div);


def prim_idez_module_explorer_show_module(proc):
# instance_method show_module: fun(name) {
#   this.show_module_basic(name);
#   var doc = @webview.page().mainFrame().documentElement();
#   var mlist = doc.findFirst("#menu-listing .link-list");
#   var fns = @current_cmodule.compiled_functions();
#   fns.sortedEach(fun(name,cfn) {
#     mlist.appendInside("<li><a href='#" + cfn.fullName + "'>" + cfn.fullName + "</a></li>");
#     this.show_function(cfn, "module", ".module-functions");
#   });
#   doc.findFirst(".module-functions").setAttribute("style","display:block");
#   @current_cmodule.compiled_classes().sortedEach(fun(name, klass) {
#     mlist.appendInside("<li><a href='/class?module=" + klass.module.name + "&class=" + klass.name + "'>" + klass.fullName + "</a></li>");
#   });

    name = proc.locals()['name']
    proc.do_send(proc.reg('r_rp'), 'show_module_basic', [name])
    qt_webview = _lookup_field(proc, proc.reg('r_rdp')['webview'], 'self')
    doc = qt_webview.page().mainFrame().documentElement()
    current_cmodule = proc.reg('r_rdp')['current_cmodule']

    mlist = doc.findFirst("#menu-listing .link-list")
    fns = current_cmodule['compiled_functions']

    #sortedEach
    d = fns
    dsorted = [(key,d[key]) for key in sorted(d.keys())]
    for t in dsorted:
        name = t[0]
        cfn = t[1]
        mlist.appendInside("<li><a href='#" + _cfun_full_name(cfn) + "'>" + _cfun_full_name(cfn) + "</a></li>");
        proc.do_send(proc.reg('r_rp'), 'show_function', [cfn, "module", ".module-functions"])

    doc.findFirst(".module-functions").setAttribute("style", "diplay:block")


    #sortedEach
    d = current_cmodule['compiled_classes']
    dsorted = [(key,d[key]) for key in sorted(d.keys())]
    for t in dsorted:
        name = t[0]
        klass = t[1]
        mlist.appendInside("<li><a href='/class?module=" + klass['module']['name'] + "&class=" + klass['name'] + "'>" + _owner_full_name(klass) + "</a></li>");


def prim_idez_module_explorer_show_editor(proc):
# instance_method show_editor: fun(id, name, type, class_name) {
#   var doc = @webview.page().mainFrame().documentElement();
#   var div = doc.findFirst("div[id='" + id + "']");
#   var cfn = null;
#   div.findFirst(".method-click-advice").takeFromDocument();
#   var divcode = div.findFirst(".function-source-code");
#   divcode.appendInside("<object width=800></object>");
#   var obj = divcode.findFirst("object");
#   obj.takeFromDocument;
#   obj.setInnerXml("<param name='class_name'/><param name='function_type'/><param name='module_name'/><param name='function_name'/><param name='code'/>");
#   if (type == "module") {
#     cfn = @current_cmodule.compiled_functions[name];
#   }
#   if (type == "ctor_method") {
#     cfn = @current_cmodule.compiled_classes[class_name].constructors[name];
#     obj.findFirst("param[name='class_name']").setAttribute("value",cfn.owner.name);
#   }
#   if (type == "class_method") {
#     cfn = @current_cmodule.compiled_classes[class_name].classMethods[name];
#     obj.findFirst("param[name='class_name']").setAttribute("value",cfn.owner.name);
#   }
#   if (type == "instance_method") {
#     cfn = @current_cmodule.compiled_classes[class_name].instanceMethods[name];
#     obj.findFirst("param[name='class_name']").setAttribute("value",cfn.owner.name);
#   }
#   obj.findFirst("param[name='function_type']").setAttribute("value",type);
#   obj.findFirst("param[name='module_name']").setAttribute("value",@current_cmodule.name);
#   obj.findFirst("param[name='function_name']").setAttribute("value",cfn.name);
#   obj.findFirst("param[name='code']").setAttribute("value",cfn.text);
#   obj.setAttribute("type","x-pyqt/editor");
#   divcode.appendInside(obj);
# }
    id = proc.locals()['id']
    name = proc.locals()['name']
    type = proc.locals()['type']
    class_name = proc.locals()['class_name']

    current_cmodule = proc.reg('r_rdp')['current_cmodule']

    qt_webview = _lookup_field(proc, proc.reg('r_rdp')['webview'], 'self')
    doc = qt_webview.page().mainFrame().documentElement()

    div = doc.findFirst("div[id='" + id + "']")
    cfn = None
    div.findFirst(".method-click-advice").takeFromDocument()
    divcode = div.findFirst(".function-source-code")
    divcode.appendInside("<object width=800></object>")
    obj = divcode.findFirst("object")
    obj.takeFromDocument()
    obj.setInnerXml("<param name='class_name'/><param name='function_type'/><param name='module_name'/><param name='function_name'/><param name='code'/>");
    if type == "module":
        cfn = current_cmodule['compiled_functions'][name]
    elif type == "ctor_method":
        cfn = _compiled_class_constructors(current_cmodule['compiled_classes'][class_name])[name]
        obj.findFirst("param[name='class_name']").setAttribute("value",cfn['owner']['name'])
    elif type == "class_method":
        cfn = _compiled_class_class_methods(current_cmodule['compiled_classes'][class_name])[name]
        obj.findFirst("param[name='class_name']").setAttribute("value",cfn['owner']['name']);
    elif type == "instance_method":
        cfn = current_cmodule['compiled_classes'][class_name]['methods'][name];
        obj.findFirst("param[name='class_name']").setAttribute("value",cfn['owner']['name']);
    else:
        proc.throw_with_message('unexpected method type')

    obj.findFirst("param[name='function_type']").setAttribute("value",type);
    obj.findFirst("param[name='module_name']").setAttribute("value",current_cmodule['name']);
    obj.findFirst("param[name='function_name']").setAttribute("value",cfn['name']);
    obj.findFirst("param[name='code']").setAttribute("value",cfn['text']);
    obj.setAttribute("type","x-pyqt/editor");
    divcode.appendInside(obj);


def prim_idez_module_explorer_editor_for_plugin(proc):
# instance_method editor_for_plugin: fun(params) {
#   var cfn = null;
#   var e = null;
#   if (params.has("function_type")) {
#     if (params["function_type"] == "module") {
#       cfn = get_module(params["module_name"]).compiled_functions()[params["function_name"]];
#     }
#     if (params["function_type"] == "ctor_method") {
#       cfn = get_module(params["module_name"]).compiled_classes()[params["class_name"]].constructors()[params["function_name"]];
#     }
#     if (params["function_type"] == "class_method") {
#       cfn = get_module(params["module_name"]).compiled_classes()[params["class_name"]].classMethods()[params["function_name"]];
#     }
#     if (params["function_type"] == "instance_method") {
#       cfn = get_module(params["module_name"]).compiled_classes()[params["class_name"]].instanceMethods()[params["function_name"]];
#     }
#     if (cfn) {
#       e =ExplorerEditor.new(null, cfn);
#       e.withVariables(fun() { @variables });
#       e.withIModule(fun() { this.currentIModule() });
#       e.onFinish(fun(env) { @variables = env + @variables;});
#     }
#   } else {
#     if (params.has("code")) {
#       e = ExplorerEditor.new(null, null);
#       e.withVariables(fun() { @variables });
#       e.withIModule(fun() { this.currentIModule() });
#       e.onFinish(fun(env) { @variables = env + @variables;});
#       e.setText(params["code"]);
#     }
#   }
#   return e;
# }
    params = proc.locals()['params']
    cfn = None
    if 'function_type' in params:
        if params['function_type'] == 'module':
            cfn = _get_module(proc, params["module_name"])['compiled_functions'][params["function_name"]]
        if params["function_type"] == "ctor_method":
            mod = _get_module(proc, params["module_name"])
            cfn = _compiled_class_constructors(mod['compiled_classes'][params["class_name"]])[params["function_name"]]
        if params["function_type"] == "class_method":
            mod = _get_module(proc, params["module_name"])
            cfn = _compiled_class_class_methods(mod['compiled_classes'][params["class_name"]])[params["function_name"]]
        if params["function_type"] == "instance_method":
            mod = _get_module(proc, params["module_name"])
            cfn = mod['compiled_classes'][params["class_name"]]['methods'][params["function_name"]]
        if cfn:
            e = proc.do_send(proc.reg('r_rp'), 'editor_for_cfun', [cfn])
    else:
        if 'code' in params:
            e = proc.do_send(proc.reg('r_rp'), 'editor_for_code', [params["code"]])
    if e == None:
        logger.warning("returning editor as None!")
    return e;