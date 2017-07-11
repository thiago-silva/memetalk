from pyparsers.parser import MemeParser
from pyparsers.memetr import MemeTr
from pyparsers.astbuilder import *
import pyutils
from pyutils import bits
from pyutils import entries
import traceback
import os
import sys
import ejson
from . import comp_vmemory

class MMC(object):
    MAGIC_NUMBER = 0x420

    def __init__(self, cmodule):
        self.cmodule = cmodule

    def name_ptr_for(self, name, mmc):
        acc = 0
        for entry_name_t, bsize in mmc['names']:
            if entry_name_t[0:-1] == name:
                return self.HEADER_SIZE + acc
            acc += bsize
        raise Exception('entry {} not found in NAMES'.format(name))

    def create_mmc_struct(self, vmem):
        mmc = {'header':
               {'magic_number': None,
                'ot_size': None,
                'er_size': None,
                'ir_size': None,
                'st_size': None,
                'names_size': None},
               'names': [],
               'object_table': [],
               'external_references': [],
               'internal_references': [],
               'symbols': [],
               'reloc_table': []
            }


        self.HEADER_SIZE = len(mmc['header']) * bits.WSIZE
        self.cmodule.fill(vmem)

        mmc['header']['magic_number'] = self.MAGIC_NUMBER

        # mmc['names'] = [(name_t, bits.string_block_size(name_t)) for name_t in [name + '\0' for name in vmem.external_names()]]

        # labels = self.cmodule.entry_labels()
        names_list = set([n + "\0" for n in vmem.external_names()])

        mmc['names'] = [(name_t, bits.string_block_size(name_t)) for name_t in names_list]
        mmc['header']['names_size'] = sum([x[1] for x in mmc['names']])

        base = self.HEADER_SIZE + mmc['header']['names_size']
        vmem.set_base(base)

        mmc['object_table'] = vmem.object_table()
        mmc['header']['ot_size'] = len(mmc['object_table'])

        for pair in vmem.external_references():
            mmc['external_references'].append(self.name_ptr_for(pair[0], mmc))
            mmc['external_references'].append(pair[1])

        mmc['header']['er_size'] = len(mmc['external_references']) * bits.WSIZE

        for pair in vmem.internal_references():
            mmc['internal_references'].append(self.name_ptr_for(pair[0], mmc))
            mmc['internal_references'].append(pair[1])

            mmc['header']['ir_size'] = len(mmc['internal_references']) * bits.WSIZE


        for pair in vmem.symbols_references():
            mmc['symbols'].append(self.name_ptr_for(pair[0], mmc))
            mmc['symbols'].append(pair[1])

        mmc['header']['st_size'] = len(mmc['symbols']) * bits.WSIZE

        mmc['reloc_table'] = vmem.reloc_table()

        return mmc

    def dump(self, filepath):
        vmem = comp_vmemory.CompVirtualMemory()
        mmc = self.create_mmc_struct(vmem)
        with open(filepath[:-2] + "mmc", "w") as fp:
            # header
            fp.write(bits.pack_word(mmc['header']['magic_number']))
            fp.write(bits.pack_word(mmc['header']['ot_size']))
            fp.write(bits.pack_word(mmc['header']['er_size']))
            fp.write(bits.pack_word(mmc['header']['ir_size']))
            fp.write(bits.pack_word(mmc['header']['st_size']))
            fp.write(bits.pack_word(mmc['header']['names_size']))

            # names
            for name, chunk_size in mmc['names']:
                text = name + ((chunk_size - len(name)) * '\0')
                fp.write(text)

            # object table
            for v in mmc['object_table']:
                fp.write(bits.pack_byte(v))

            # external references
            for v in mmc['external_references']:
                fp.write(bits.pack_word(v))

            # internal references
            for v in mmc['internal_references']:
                fp.write(bits.pack_word(v))

            # symbols
            for v in mmc['symbols']:
                fp.write(bits.pack_word(v))

            # reloc table
            for word in mmc['reloc_table']:
                fp.write(bits.pack_word(word))



class MMC_Fun(object):
    MAGIC_NUMBER = 0x420

    def __init__(self, cmod, cfun):
        self.cmod = cmod
        self.cfun = cfun

    def name_ptr_for(self, name, mmc):
        acc = 0
        for entry_name_t, bsize in mmc['names']:
            if entry_name_t[0:-1] == name:
                return self.HEADER_SIZE + acc
            acc += bsize
        raise Exception('entry {} not found in NAMES'.format(name))

    def create_mmc_struct(self, vmem):
        mmc = {'header':
               {'magic_number': None,
                'ot_size': None,
                'er_size': None,
                'st_size': None,
                'names_size': None,
                'cfun_addr': None},
               'names': [],
               'object_table': [],
               'external_references': [],
               'symbols': [],
               'reloc_table': []
            }


        self.HEADER_SIZE = len(mmc['header']) * bits.WSIZE
        self.cmod.fill(vmem)

        mmc['header']['magic_number'] = self.MAGIC_NUMBER

        names_list = set([n + "\0" for n in vmem.external_names()])

        mmc['names'] = [(name_t, bits.string_block_size(name_t)) for name_t in names_list]
        mmc['header']['names_size'] = sum([x[1] for x in mmc['names']])

        base = self.HEADER_SIZE + mmc['header']['names_size']
        vmem.set_base(base)

        mmc['object_table'] = vmem.object_table()
        mmc['header']['ot_size'] = len(mmc['object_table'])

        for pair in vmem.external_references():
            mmc['external_references'].append(self.name_ptr_for(pair[0], mmc))
            mmc['external_references'].append(pair[1])

        mmc['header']['er_size'] = len(mmc['external_references']) * bits.WSIZE

        for pair in vmem.symbols_references():
            mmc['symbols'].append(self.name_ptr_for(pair[0], mmc))
            mmc['symbols'].append(pair[1])

        mmc['header']['st_size'] = len(mmc['symbols']) * bits.WSIZE

        mmc['reloc_table'] = vmem.reloc_table()

        mmc['header']['cfun_addr'] = vmem.physical_address(self.cfun.oop)
        return mmc

    def dump(self):
        vmem = comp_vmemory.CompVirtualMemory()
        mmc = self.create_mmc_struct(vmem)

        fp = sys.stdout

        # header
        fp.write(bits.pack_word(mmc['header']['magic_number']))
        fp.write(bits.pack_word(mmc['header']['ot_size']))
        fp.write(bits.pack_word(mmc['header']['er_size']))
        fp.write(bits.pack_word(mmc['header']['st_size']))
        fp.write(bits.pack_word(mmc['header']['names_size']))
        fp.write(bits.pack_word(mmc['header']['cfun_addr']))

        # names
        for name, chunk_size in mmc['names']:
            text = name + ((chunk_size - len(name)) * '\0')
            fp.write(text)

        # object table
        for v in mmc['object_table']:
            fp.write(bits.pack_byte(v))

        # external references
        for v in mmc['external_references']:
            fp.write(bits.pack_word(v))

        # symbols
        for v in mmc['symbols']:
            fp.write(bits.pack_word(v))

        # reloc table
        for word in mmc['reloc_table']:
            fp.write(bits.pack_word(word))



class Compiler(ASTBuilder):
    def __init__(self):
        self.cmodule = None
        self.filepath = None

    def new_module(self):
        module_name =  os.path.splitext(os.path.basename(self.filepath))[0]
        self.cmodule = entries.CompiledModule(module_name)
        return self.cmodule

    def do_parse(self, parser, rule, *args):
        try:
            parser.has_fun_literal = pyutils.Flag() # pymeta uses eval() which disables assignment. This works around it
            return parser.apply(rule, *args)
        except Exception as err:
            if hasattr(err,'formatError'):
                print err.formatError(''.join(parser.input.data))
            else:
                print err, traceback.format_exc()
            sys.exit(1)

    def compile(self, filepath):
        self.filepath = filepath
        self.line_offset = 0
        self.parser = MemeParser(open(filepath, 'r').read())
        self.parser.i = self

        ast = self.do_parse(self.parser, 'start')[0]

        print ast

        self.parser = MemeTr([ast])
        self.parser.i = self

        self.do_parse(self.parser, "start")

        mmc = MMC(self.cmodule)
        mmc.dump(filepath)

    def compile_lambda(self, text, env_names):
        self.filepath = '<eval>'
        self.line_offset = 0

        self.parser = MemeParser(text)
        self.parser.i = self

        ast = self.do_parse(self.parser, 'cfunliteral_body')[0]
        # print ast

        cmod = entries.CompiledModule('dummy_module')
        cfun = cmod.new_function('<eval>', [])
        cfun.uses_env(True)
        cfun.declare_vars(env_names)

        self.parser = MemeTr([cfun, ast])
        self.parser.i = self

        self.do_parse(self.parser, 'cfunliteral')


        mmc = MMC_Fun(cmod, cfun)
        mmc.dump()

    def recompile_function(self, line, text):
        self.filepath = '<???>'
        self.line_offset = int(line)

        self.parser = MemeParser(text)
        self.parser.i = self

        ast = self.do_parse(self.parser, 'fun_rest', 'random_name')[0]
        # print ast

        # cmod = entries.CompiledModule('dummy_module')
        # cfun = cmod.new_function('<????????>', [])

        cmod = self.new_module()
        self.parser = MemeTr([cmod, ast])
        self.parser.i = self

        self.do_parse(self.parser, 'function_definition')

        cfun = cmod.functions['random_name']
        cfun.uses_env(False)
        mmc = MMC_Fun(cmod, cfun)
        mmc.dump()


def main(*paths):
    for path in paths:
        Compiler().compile(path)

def compile_lambda(js):
    # python -m pycompiler.compiler compile-lambda '{"text":"fun() {X.z}","env_names":""}'
    # python -m pycompiler.compiler compile-lambbd '{"text":"fun() {a}","env_names":["a"]}'
    js = ejson.loads(js)
    Compiler().compile_lambda(js['text'], js['env_names'])

def recompile_fun(js):
    # python -m pycompiler.compiler recompile-fun '{"text":"fun() {X.z}"}'
    js = ejson.loads(js)
    Compiler().recompile_function(js["start_line"], js['text'])

if __name__ == '__main__':
    from clime import Program
    prog = Program(debug=True, default='main')
    prog.main()
