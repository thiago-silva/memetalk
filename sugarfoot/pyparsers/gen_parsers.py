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

import os, sys
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from pymeta import builder, grammar

fp = open(os.path.join(os.path.dirname(__file__), 'parser.py'), 'wb')
ometa_grammar = grammar.OMetaGrammar(open(os.path.join(os.path.dirname(__file__),"meme.g")).read())
tree = ometa_grammar.parseGrammar('MemeParser', builder.TreeBuilder)
fp.write(builder.writeBoot(tree))

# fp = open(os.path.join(os.path.dirname(__file__), 'coretr.py'), 'wb')
# ometa_grammar = grammar.OMetaGrammar(open(os.path.join(os.path.dirname(__file__),"core_tr.g")).read())
# tree = ometa_grammar.parseGrammar('CoreTr', builder.TreeBuilder)
# fp.write(builder.writeBoot(tree))

fp = open(os.path.join(os.path.dirname(__file__), 'memetr.py'), 'wb')
ometa_grammar = grammar.OMetaGrammar(open(os.path.join(os.path.dirname(__file__),"meme_tr.g")).read())
tree = ometa_grammar.parseGrammar('MemeTr', builder.TreeBuilder)
fp.write(builder.writeBoot(tree))

fp = open(os.path.join(os.path.dirname(__file__), 'opt.py'), 'wb')
ometa_grammar = grammar.OMetaGrammar(open(os.path.join(os.path.dirname(__file__),"opt.g")).read())
tree = ometa_grammar.parseGrammar('Opt', builder.TreeBuilder)
fp.write(builder.writeBoot(tree))

# fp = open(os.path.dirname(__file__) + '/../loader.py', 'wb')
# ometa_grammar = grammar.OMetaGrammar(open(os.path.dirname(__file__)+"/load_module.ast.g").read())
# tree = ometa_grammar.parseGrammar('Loader', builder.TreeBuilder)
# fp.write(builder.writeBoot(tree))
