from pyparsers.parser import MemeParser
from pyparsers.memetr import MemeTr
from pyparsers.astbuilder import *
import pyutils
from pyutils import bits
from . import core_vmem
from pprint import pprint as P
from pdb import set_trace as br
import os
from pyutils import entries

class Compiler(ASTBuilder):
    def __init__(self):
        self.vmem = core_vmem.CoreVirtualMemory()
        self.HEADER_SIZE = 4 * bits.WSIZE # bytes. 3 = entries, names_size, es_size, ot_size

        self.core_compiled_module = entries.CoreCompiledModule()

    def name_ptr_for_name(self, name, core):
        acc = 0
        for entry_name_t, bsize in core['names']:
            if entry_name_t[0:-1] == name:
                return self.HEADER_SIZE + acc
            acc += bsize
        raise Exception('entry {} not found in NAMES'.format(name))

    def compile(self):
        self.line_offset = 0
        self.parser = MemeParser(open(os.path.join(os.path.dirname(__file__), '../mm/core.mm'), 'r').read())
        self.parser.has_fun_literal = pyutils.Flag() # pymeta uses eval() which disables assignment. This works around it
        self.parser.i = self
        ast = self.parser.apply("start")[0]
        print ast
        self.parser = MemeTr([ast])
        self.parser.i = self
        self.parser.apply('start')

        core = self.build_core()
        self.dump(core)

    def build_core(self):
        core = {
            'header': {
                'entries': None,             # number of labeled objects (top-levels, behaviors, cclasses)
                'names_size': None,          # size in bytes of NAMES section
                'st_size': None,             # size of Symbols table
                'ot_size': None},            # size of OBJECT TABLE in bytes
            'names': [],
            'index': [],                     # index of top level entities
            'object_table': [],
            'symbols': [],                   # Symbol references
            'reloc_table': []}

        self.core_compiled_module.fill(self.vmem)

        # -- HEADER and NAMES section
        labels = self.core_compiled_module.entry_labels()
        core['header']['entries'] = len(labels)

        # names :: [(string, alloc-size in bytes, self-ptr)]
        names_list = set([l + "\0" for l in labels] + [n + "\0" for n in self.vmem.external_names()])

        core['names'] = [(name_t, bits.string_block_size(name_t)) for name_t in names_list]

        core['header']['names_size'] = sum([x[1] for x in core['names']])

        index_size = core['header']['entries'] * 2 * bits.WSIZE # *2: pair (name, entry),
        base = self.HEADER_SIZE + core['header']['names_size'] + index_size
        self.vmem.set_base(base)

        # - OBJECT TABLE
        core['object_table'] = self.vmem.object_table()

        # - HEADER ot_size
        core['header']['ot_size'] = len(core['object_table'])

        # symbols
        for pair in self.vmem.symbols_references():
            core['symbols'].append(self.name_ptr_for_name(pair[0], core))
            core['symbols'].append(pair[1])

        core['header']['st_size'] = len(core['symbols']) * bits.WSIZE

        # reloc
        core['reloc_table'] = self.vmem.reloc_table()

        # - INDEX section
        for name in labels:
            core['index'].append(self.name_ptr_for_name(name, core))  # ptr to string
            core['index'].append(self.vmem.index_for(name))           # ptr to object

        return core

    def dump(self, core):
        with open("core.img", "w") as fp:
            # header

            fp.write(bits.pack_word(core['header']['entries']))
            fp.write(bits.pack_word(core['header']['names_size']))
            fp.write(bits.pack_word(core['header']['st_size']))
            fp.write(bits.pack_word(core['header']['ot_size']))

            # names
            for name, chunk_size in core['names']:
                text = name + ((chunk_size - len(name)) * '\0')
                fp.write(text)

            # index
            for ptr in core['index']:
                fp.write(bits.pack_word(ptr))

            # object table
            for v in core['object_table']:
                fp.write(bits.pack_byte(v))

            # symbols
            for v in core['symbols']:
                fp.write(bits.pack_word(v))

            # reloc table
            for word in core['reloc_table']:
                fp.write(bits.pack_word(word))

    ######################

    def new_module(self):
        return self.core_compiled_module


Compiler().compile()
