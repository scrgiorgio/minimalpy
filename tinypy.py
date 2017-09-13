import sys

# see C++ addBuiltIn
if not "tinypy" in sys.version:

	# merge
	def merge(a,b):
		if isinstance(a,dict):
			for k in b: a[k] = b[k]
		else:
			for k in b: setattr(a,k,b[k])

	# number
	def number(v):
		if type(v) is str and v[0:2] == '0x':
			v = int(v[2:],16)
		return float(v)

	# istype
	def istype(v,t):
		if t == 'string': return isinstance(v,str)
		elif t == 'list': return (isinstance(v,list) or isinstance(v,tuple))
		elif t == 'dict': return isinstance(v,dict)
		elif t == 'number': return (isinstance(v,float) or isinstance(v,int))
		raise '?'

	# fpack
	def fpack(v):
		import struct
		return struct.pack('d',v)

	# loadFile
	def loadFile(filename):
		f = open(filename,'rb')
		r = f.read()
		f.close()
		return r

	# saveFile
	def saveFile(filename,v):
		f = open(filename,'wb')
		f.write(v)
		f.close()


OP_EOF=0
OP_ADD=1
OP_SUB=2
OP_MUL=3
OP_DIV=4
OP_POW=5
OP_BITAND=6
OP_BITOR=7
OP_CMP=8
OP_GET=9
OP_SET=10
OP_NUMBER=11
OP_STRING=12
OP_GGET=13
OP_GSET=14
OP_MOVE=15
OP_DEF=16
OP_PASS=17
OP_JUMP=18
OP_CALL=19
OP_RETURN=20
OP_IF=21
OP_DEBUG=22
OP_EQ=23
OP_LE=24
OP_LT=25
OP_DICT=26
OP_LIST=27
OP_NONE=28
OP_LEN=29
OP_POS=30
OP_PARAMS=31
OP_IGET=32
OP_FILE=33
OP_NAME=34
OP_NE=35
OP_HAS=36
OP_RAISE=37
OP_SETJMP=38
OP_MOD=39
OP_LSH=40
OP_RSH=41
OP_ITER=42
OP_DEL=43
OP_REGS=44
OP_BITXOR=45
OP_IFN=46
OP_NOT=47
OP_BITNOT = 48

# /////////////////////////////////////////////////////
def raiseError(explanation,s,i):
	y,x = i
	line = s.split('\n')[y-1]
	p = ''
	if y < 10: p += ' '
	if y < 100: p += '  '
	r = p + str(y) + ": " + line + "\n"
	r += "     "+" "*x+"^" +'\n'
	raise 'error raised: '+explanation+'\n'+r

# /////////////////////////////////////////////////////
class Token:
    def __init__(self,pos=(0,0),type='symbol',val=None,items=None):
        self.pos,self.type,self.val,self.items=pos,type,val,items

# /////////////////////////////////////////////////////
class Tokenizer:

	# __init__
	def __init__(self):

		self.ISYMBOLS = '`-=[];,./~!@$%^&*()+{}:<>?|'

		self.SYMBOLS = [
			'def','class','yield','return','pass','and','or','not','in','import',
			'is','while','break','for','continue','if','else','elif','try',
			'except','raise','True','False','None','global','del','from',
			'-','+','*','**','/','%','<<','>>',
			'-=','+=','*=','/=','=','==','!=','<','>', '|=', '&=', '^=',
			'<=','>=','[',']','{','}','(',')','.',':',',',';','&','|','!', '^']


	# clean
	def clean(self,s):
		s = s.replace('\r\n','\n')
		s = s.replace('\r','\n')
		return s

	# doTokenize
	def doTokenize(self,s):
		s = self.clean(s)

		class TData:

			# __init__
			def __init__(self):
				self.y,self.yi,self.nl = 1,0,True
				self.res,self.indent,self.braces = [],[0],0
			def add(self,t,v): self.res.append(Token(self.f,t,v))

		self.T,i,l = TData(),0,len(s)
		try: 
			return self.do_tokenize(s,i,l)
		except: 
			raiseError('Tokenizer.doTokenize',s,self.T.f)

	# do_tokenize
	def do_tokenize(self,s,i,l):
		self.T.f = (self.T.y,i-self.T.yi+1)
		while i < l:
			c = s[i]; self.T.f = (self.T.y,i-self.T.yi+1)
			if self.T.nl: self.T.nl = False; i = self.do_indent(s,i,l)
			elif c == '\n': i = self.do_nl(s,i,l)
			elif c in self.ISYMBOLS: i = self.do_symbol(s,i,l)
			elif c >= '0' and c <= '9': i = self.do_number(s,i,l)
			elif (c >= 'a' and c <= 'z') or \
				(c >= 'A' and c <= 'Z') or c == '_':  i = self.do_name(s,i,l)
			elif c=='"' or c=="'": i = self.do_string(s,i,l)
			elif c=='#': i = self.do_comment(s,i,l)
			elif c == '\\' and s[i+1] == '\n':
				i += 2; self.T.y,self.T.yi = self.T.y+1,i
			elif c == ' ' or c == '\t': i += 1
			else: 
				raiseError('Tokenizer.do_tokenize',s,self.T.f)
		self.indent(0)
		r = self.T.res; self.T = None
		#for t in r:
			#print (t.pos,t.type,t.val)
		return r

	# do_nl
	def do_nl(self,s,i,l):
		if not self.T.braces:
			self.T.add('nl',None)
		i,self.T.nl = i+1,True
		self.T.y,self.T.yi = self.T.y+1,i
		return i

	# do_indent
	def do_indent(self,s,i,l):
		v = 0
		while i<l:
			c = s[i]
			if c != ' ' and c != '\t': break
			i,v = i+1,v+1
		if c != '\n' and c != '#' and not self.T.braces: self.indent(v)
		return i

	# indent
	def indent(self,v):
		if v == self.T.indent[-1]: pass
		elif v > self.T.indent[-1]:
			self.T.indent.append(v)
			self.T.add('indent',v)
		elif v < self.T.indent[-1]:
			n = self.T.indent.index(v)
			while len(self.T.indent) > n+1:
				v = self.T.indent.pop()
				self.T.add('dedent',v)

	# do_symbol
	def do_symbol(self,s,i,l):
		symbols = []
		v,f,i = s[i],i,i+1
		if v in self.SYMBOLS: symbols.append(v)
		while i<l:
			c = s[i]
			if not c in self.ISYMBOLS: break
			v,i = v+c,i+1
			if v in self.SYMBOLS: symbols.append(v)
		v = symbols.pop(); n = len(v); i = f+n
		self.T.add('symbol',v)
		if v in ['[','(','{']: self.T.braces += 1
		if v in [']',')','}']: self.T.braces -= 1
		return i

	# do_number
	def do_number(self,s,i,l):
		v,i,c =s[i],i+1,s[i]
		while i<l:
			c = s[i]
			if (c < '0' or c > '9') and (c < 'a' or c > 'f') and c != 'x': break
			v,i = v+c,i+1
		if c == '.':
			v,i = v+c,i+1
			while i<l:
				c = s[i]
				if c < '0' or c > '9': break
				v,i = v+c,i+1
		self.T.add('number',v)
		return i

	# do_name
	def do_name(self,s,i,l):
		v,i =s[i],i+1
		while i<l:
			c = s[i]
			if (c < 'a' or c > 'z') and (c < 'A' or c > 'Z') and (c < '0' or c > '9') and c != '_': break
			v,i = v+c,i+1
		if v in self.SYMBOLS: self.T.add('symbol',v)
		else: self.T.add('name',v)
		return i

	# do_string
	def do_string(self,s,i,l):
		v,q,i = '',s[i],i+1
		if (l-i) >= 5 and s[i] == q and s[i+1] == q: # """
			i += 2
			while i<l-2:
				c = s[i]
				if c == q and s[i+1] == q and s[i+2] == q:
					i += 3
					self.T.add('string',v)
					break
				else:
					v,i = v+c,i+1
					if c == '\n': self.T.y,self.T.yi = self.T.y+1,i
		else:
			while i<l:
				c = s[i]
				if c == "\\":
					i = i+1; c = s[i]
					if c == "n": c = '\n'
					if c == "r": c = chr(13)
					if c == "t": c = "\t"
					if c == "0": c = "\0"
					v,i = v+c,i+1
				elif c == q:
					i += 1
					self.T.add('string',v)
					break
				else:
					v,i = v+c,i+1
		return i

	# do_comment
	def do_comment(self,s,i,l):
		i += 1
		while i<l:
			c = s[i]
			if c == '\n': break
			i += 1
		return i

# ///////////////////////////////////////////////
class Parser:

	# __init__
	def __init__(self):

		self.base_dmap = {
			',':{'lbp':20,'bp':20,'led':self.infix_tuple},
			'+':{'lbp':50,'bp':50,'led':self.infix_led},
			'-':{'lbp':50,'nud':self.prefix_neg,'bp':50,'led':self.infix_led},
			'not':{'lbp':35,'nud':self.prefix_nud,'bp':35,'bp':35,'led':self.infix_not },
			'%':{'lbp':60,'bp':60,'led':self.infix_led},
			'*':{'lbp':60,'nud':self.vargs_nud,'bp':60,'led':self.infix_led,},
			'**': {'lbp':65,'nud':self.nargs_nud,'bp':65,'led':self.infix_led,},
			'/':{'lbp':60,'bp':60,'led':self.infix_led},
			'(':{'lbp':70,'nud':self.paren_nud,'bp':80,'led':self.call_led,},
			'[':{'lbp':70,'nud':self.list_nud,'bp':80,'led':self.get_led,},
			'{':{'lbp':0,'nud':self.dict_nud,},
			'.':{'lbp':80,'bp':80,'led':self.dot_led,'type':'get',},
			'break':{'lbp':0,'nud':self.itself,'type':'break'},
			'pass':{'lbp':0,'nud':self.itself,'type':'pass'},
			'continue':{'lbp':0,'nud':self.itself,'type':'continue'},
			'eof':{'lbp':0,'type':'eof','val':'eof'},
			'def':{'lbp':0,'nud':self.def_nud,'type':'def',},
			'while':{'lbp':0,'nud':self.while_nud,'type':'while',},
			'for':{'lbp':0,'nud':self.for_nud,'type':'for',},
			'try':{'lbp':0,'nud':self.try_nud,'type':'try',},
			'if':{'lbp':0,'nud':self.if_nud,'type':'if',},
			'class':{'lbp':0,'nud':self.class_nud,'type':'class',},
			'raise':{'lbp':0,'nud':self.prefix_nud0,'type':'raise','bp':20,},
			'return':{'lbp':0,'nud':self.prefix_nud0,'type':'return','bp':10,},
			'import':{'lbp':0,'nud':self.prefix_nuds,'type':'import','bp':20,},
			'from':{'lbp':0,'nud':self.from_nud,'type':'from','bp':20,},
			'del':{'lbp':0,'nud':self.prefix_nuds,'type':'del','bp':10,},
			'global':{'lbp':0,'nud':self.prefix_nuds,'type':'globals','bp':20,},
			'=':{'lbp':10,'bp':9,'led':self.infix_led,},
		}

		self.i_infix(40,self.infix_led,'<','>','<=','>=','!=','==')
		self.i_infix(40,self.infix_is,'is','in')
		self.i_infix(10,self.infix_led,'+=','-=','*=','/=', '&=', '|=', '^=')
		self.i_infix(32,self.infix_led,'and','&')
		self.i_infix(31,self.infix_led,'^')
		self.i_infix(30,self.infix_led,'or','|')
		self.i_infix(36,self.infix_led,'<<','>>')

		self.i_terms(')','}',']',';',':','nl','elif','else','True','False','None','name','string','number','indent','dedent','except')
		self.base_dmap['nl']['val'] = 'nl'

	# check
	def check(self,t,*vs):
		if vs[0] == None: return True
		if t.type in vs: return True
		if t.type == 'symbol' and t.val in vs: return True
		return False

	# tweak
	def tweak(self,k,v):
		self.stack.append((k,self.dmap[k]))
		if v: 
			self.dmap[k] = self.omap[k]
		else: 
			self.dmap[k] = {'lbp':0,'nud':self.itself}

	# restore
	def restore(self):
		k,v = self.stack.pop()
		self.dmap[k] = v

	# raiseError
	def raiseError(self,ctx,t):
		raiseError("Parse.raiseError " + ctx,self.s,t.pos)

	# nud
	def nud(self,t):
		return t.nud(t)

	# led
	def led(self,t,left):
		return t.led(t,left)

	# get_lbp
	def get_lbp(self,t):
		return t.lbp

	# get_items
	def get_items(self,t):
		return t.items

	# terminal
	def terminal(self):
		if self._terminal > 1:
			self.raiseError('invalid statement',self.token)

	# self.expression
	def expression(self,rbp):
		t = self.token
		self.advance()
		left = self.nud(t)
		while rbp < self.get_lbp(self.token):
			t = self.token
			self.advance()
			left = self.led(t,left)
		return left

	# infix_led
	def infix_led(self,t,left):
		t.items = [left,self.expression(t.bp)]
		return t

	# infix_is
	def infix_is(self,t,left):
		if self.check(self.token,'not'):
			t.val = 'isnot'
			self.advance('not')
		t.items = [left,self.expression(t.bp)]
		return t

	# infix_not
	def infix_not(self,t,left):
		self.advance('in')
		t.val = 'notin'
		t.items = [left,self.expression(t.bp)]
		return t

	# infix_tuple
	def infix_tuple(self,t,left):
		r = self.expression(t.bp)
		if left.val == ',':
			left.items.append(r)
			return left
		t.items = [left,r]
		t.type = 'tuple'
		return t

	# lst
	def lst(self,t):
		if t == None: return []
		if self.check(t,',','tuple','statements'):
			return self.get_items(t)
		return [t]

	# ilst
	def ilst(self,typ,t):
		return Token(t.pos,typ,typ,self.lst(t))

	# call_led
	def call_led(self,t,left):
		r = Token(t.pos,'call','$',[left])
		while not self.check(self.token,')'):
			self.tweak(',',0)
			r.items.append(self.expression(0))
			if self.token.val == ',': self.advance(',')
			self.restore()
		self.advance(")")
		return r

	# get_led
	def get_led(self,t,left):
		r = Token(t.pos,'get','.',[left])
		items =  [left]
		more = False
		while not self.check(self.token,']'):
			more = False
			if self.check(self.token,':'):
				items.append(Token(self.token.pos,'symbol','None'))
			else:
				items.append(self.expression(0))
			if self.check(self.token,':'):
				self.advance(':')
				more = True
		if more:
			items.append(Token(self.token.pos,'symbol','None'))
		if len(items) > 2:
			items = [left,Token(t.pos,'slice',':',items[1:])]
		r.items = items
		self.advance("]")
		return r

	# dot_led
	def dot_led(self,t,left):
		r = self.expression(t.bp)
		r.type = 'string'
		t.items = [left,r]
		return t

	# itself
	def itself(self,t):
		return t

	# paren_nud
	def paren_nud(self,t):
		self.tweak(',',1)
		r = self.expression(0)
		self.restore()
		self.advance(')')
		return r

	# list_nud
	def list_nud(self,t):
		t.type = 'list'
		t.val = '[]'
		t.items = []
		next = self.token
		self.tweak(',',0)
		while not self.check(self.token,'for',']'):
			r = self.expression(0)
			t.items.append(r)
			if self.token.val == ',': self.advance(',')
		if self.check(self.token,'for'):
			t.type = 'comp'
			self.advance('for')
			self.tweak('in',0)
			t.items.append(self.expression(0))
			self.advance('in')
			t.items.append(self.expression(0))
			self.restore()
		self.restore()
		self.advance(']')
		return t

	# dict_nud
	def dict_nud(self,t):
		t.type='dict'
		t.val = '{}'
		t.items = []
		self.tweak(',',0)
		while not self.check(self.token,'}'):
			t.items.append(self.expression(0))
			if self.check(self.token,':',','): self.advance()
		self.restore()
		self.advance('}')
		return t

	# advance
	def advance(self,val=None):
		if not self.check(self.token,val):
			self.raiseError('expected '+val,self.token)
		if self.pos < len(self.tokens):
			t = self.tokens[self.pos]
			self.pos += 1
		else:
			t = Token((0,0),'eof','eof')
		self.token = self.do(t)
        
		self._terminal += 1
		if self.check(self.token,'nl','eof',';','dedent'):
			self._terminal = 0
		return t
        

	# iblock
	def iblock(self,items):
		while self.check(self.token,'nl',';'): self.advance()
		while True:
			items.append(self.expression(0))
			self.terminal()
			while self.check(self.token,'nl',';'): self.advance()
			if self.check(self.token,'dedent','eof'): break

	# block
	def block(self):
		items = []
		tok = self.token
    
		if self.check(self.token,'nl'):
			while self.check(self.token,'nl'): self.advance()
			self.advance('indent')
			self.iblock(items)
			self.advance('dedent')
		else:
			items.append(self.expression(0))
			while self.check(self.token,';'):
				self.advance(';')
				items.append(self.expression(0))
			self.terminal()
		while self.check(self.token,'nl'): self.advance()

		if len(items) > 1:
			return Token(tok.pos,'statements',';',items)
		return items.pop()

	# def_nud
	def def_nud(self,t):
		items = t.items = []
		items.append(self.token); self.advance()
		self.advance('(')
		r = Token(t.pos,'symbol','():',[])
		items.append(r)
		while not self.check(self.token,')'):
			self.tweak(',',0)
			r.items.append(self.expression(0))
			if self.check(self.token,','): self.advance(',')
			self.restore()
		self.advance(')')
		self.advance(':')
		items.append(self.block())
		return t

	# while_nud
	def while_nud(self,t):
		items = t.items = []
		items.append(self.expression(0))
		self.advance(':')
		items.append(self.block())
		return t

	# class_nud
	def class_nud(self,t):
		items = t.items = []
		items.append(self.expression(0))
		self.advance(':')
		items.append(self.ilst('methods',self.block()))
		return t

	# from_nud
	def from_nud(self,t):
		items = t.items = []
		items.append(self.expression(0))
		self.advance('import')
		items.append(self.expression(0))
		return t

	# for_nud
	def for_nud(self,t):
		items = t.items = []
		self.tweak('in',0)
		items.append(self.expression(0))
		self.advance('in')
		items.append(self.expression(0))
		self.restore()
		self.advance(':')
		items.append(self.block())
		return t

	# if_nud
	def if_nud(self,t):
		items = t.items = []
		a = self.expression(0)
		self.advance(':')
		b = self.block()
		items.append(Token(t.pos,'elif','elif',[a,b]))
		while self.check(self.token,'elif'):
			tok = self.token
			self.advance('elif')
			a = self.expression(0)
			self.advance(':')
			b = self.block()
			items.append(Token(tok.pos,'elif','elif',[a,b]))
		if self.check(self.token,'else'):
			tok = self.token
			self.advance('else')
			self.advance(':')
			b = self.block()
			items.append(Token(tok.pos,'else','else',[b]))
		return t

	# try_nud
	def try_nud(self,t):
		items = t.items = []
		self.advance(':')
		b = self.block()
		items.append(b)
		while self.check(self.token,'except'):
			tok = self.token
			self.advance('except')
			if not self.check(self.token,':'): a = self.expression(0)
			else: a = Token(tok.pos,'symbol','None')
			self.advance(':')
			b = self.block()
			items.append(Token(tok.pos,'except','except',[a,b]))
		return t

	# prefix_nud
	def prefix_nud(self,t):
		bp = t.bp
		t.items = [self.expression(bp)]
		return t

	# prefix_nud0
	def prefix_nud0(self,t):
		if self.check(self.token,'nl',';','eof','dedent'): return t
		return self.prefix_nud(t)

	# prefix_nuds
	def prefix_nuds(self,t):
		r = self.expression(0)
		return self.ilst(t.type,r)

	# prefix_neg
	def prefix_neg(self,t):
		r = self.expression(50)
		if r.type == 'number':
			r.val = str(-float(r.val))
			return r
		t.items = [Token(t.pos,'number','0'),r]
		return t

	# vargs_nud
	def vargs_nud(self,t):
		r = self.prefix_nud(t)
		t.type = 'args'
		t.val = '*'
		return t

	# nargs_nud
	def nargs_nud(self,t):
		r = self.prefix_nud(t)
		t.type = 'nargs'
		t.val = '**'
		return t

	# i_infix
	def i_infix(self,bp,led,*vs):
		for v in vs: 
			self.base_dmap[v] = {'lbp':bp,'bp':bp,'led':led}

	# i_terms
	def i_terms(self,*vs):
		for v in vs: 
			self.base_dmap[v] = {'lbp':0,'nud':self.itself}

	# gmap
	def gmap(self,t,v):
		if v not in self.dmap:
			self.raiseError('unknown "%s"'%v,t)
		return self.dmap[v]

	# do
	def do(self,t):
		if t.type == 'symbol': r = self.gmap(t,t.val)
		else: r = self.gmap(t,t.type)
		merge(t,r)
		return t

	# do_module
	def do_module(self):
		tok = self.token
		items = []
		self.iblock(items)
		self.advance('eof')
		if len(items) > 1:
			return Token(tok.pos,'statements',';',items)
		return items.pop()


	# cpy
	def cpy(self,d):
		r = {}
		for k in d: r[k] = d[k]
		return r

	# doParse
	def doParse(self,s,tokens):
		tokenizer=Tokenizer()
		s = tokenizer.clean(s)

		self.s = s
		self.tokens = tokens
		self.pos = 0
		self.token = None
		self.stack = []
		self._terminal = 0

		self.omap = self.cpy(self.base_dmap)
		self.dmap = self.cpy(self.base_dmap)
		self.advance()

		r = self.do_module()
		return r

# ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Encoder:

	# __init__
	def __init__(self):

		self.fmap = {
			'module':self.do_module,
			'statements':self.do_statements,
			'def':self.do_def,
			'return':self.do_return,
			'while':self.do_while,
			'if':self.do_if,
			'break':self.do_break,
			'pass':self.do_pass,
			'continue':self.do_continue,
			'for':self.do_for,
			'class':self.do_class,
			'raise':self.do_raise,
			'try':self.do_try,
			'import':self.do_import,
			'globals':self.do_globals,
			'del':self.do_del,
			'from':self.do_from,
		}

		self.rmap = {
			'list':self.do_list, 
			'tuple':self.do_list, 
			'dict':self.do_dict, 
			'slice':self.do_list,
			'comp':self.do_comp, 
			'name':self.do_name,
			'symbol':self.do_symbol,
			'number':self.do_number,
			'string':self.do_string,
			'get':self.do_get, 
			'call':self.do_call, 
			'reg':self.do_reg,
		}


	# insert
	def insert(self,v): 
		D.out.append(v)

	# begin
	def begin(self,gbl=False):
		if len(D.stack): 
			D.stack.append((D.vars,D.r2n,D.n2r,D._tmpi,D.mreg,D.snum,D._globals,D.lineno,D.globals,D.rglobals,D.cregs,D.tmpc))
		else: 
			D.stack.append(None)

		D.vars= []
		D.r2n= {}
		D.n2r= {}
		D._tmpi= 0
		D.mreg= 0
		D.snum= str(D._scopei)
		D._globals= gbl
		D.lineno= -1
		D.globals= []
		D.rglobals= []
		D.cregs = ['regs']
		D.tmpc = 0
		D._scopei += 1
		self.insert(D.cregs)

	# end
	def end(self):
		D.cregs.append(D.mreg)
		self.do_code(OP_EOF)

		if D.tmpc != 0:
			print("Warning:\nencode.py contains a register leak\n")
        
		if len(D.stack) > 1:
			D.vars,D.r2n,D.n2r,D._tmpi,D.mreg,D.snum,D._globals,D.lineno,D.globals,D.rglobals,D.cregs,D.tmpc = D.stack.pop()
		else: D.stack.pop()

	# write
	def write(self,v):
		if istype(v,'list'):
			self.insert(v)
			return
		for n in range(0,len(v),4):
			self.insert(('data',v[n:n+4]))

	# setpos
	def setpos(self,v):
		line,x = v
		if line == D.lineno: return
		text = D.lines[line-1]
		D.lineno = line
		val = text + "\0"*(4-len(text)%4)
		self.code_16(OP_POS,len(val)/4,line)
		self.write(val)

	# do_code
	def do_code(self,i,a=0,b=0,c=0):
		if not istype(i,'number'): raise
		if not istype(a,'number'): raise
		if not istype(b,'number'): raise
		if not istype(c,'number'): raise
		self.write(('code',i,a,b,c))

	# code_16
	def code_16(self,i,a,b):
		if b < 0: b += 0x8000
		self.do_code(i,a,(b&0xff00)>>8,(b&0xff)>>0)

	# get_code16
	def get_code16(self,i,a,b):
		return ('code',i,a,(b&0xff00)>>8,(b&0xff)>>0)

	# _do_string
	def _do_string(self,v,r=None):
		r = self.get_tmp(r)
		val = v + "\0"*(4-len(v)%4)
		self.code_16(OP_STRING,r,len(v))
		self.write(val)
		return r


	# do_import
	def do_import(self,t):
		for mod in t.items:
			mod.type = 'string'
			v = self.do_call(Token(t.pos,'call',None,[Token(t.pos,'name','import'),mod]))
			mod.type = 'name'
			self.do_set_ctx(mod,Token(t.pos,'reg',v))

	# do_from
	def do_from(self,t):
		mod = t.items[0]
		mod.type = 'string'
		v = self.do(Token(t.pos,'call',None,[
			Token(t.pos,'name','import'),
			mod]))
		item = t.items[1]
		if item.val == '*':
			self.free_tmp(self.do(Token(t.pos,'call',None,[
				Token(t.pos,'name','merge'),
				Token(t.pos,'name','__dict__'),
				Token(t.pos,'reg',v)]))) #REG
		else:
			item.type = 'string'
			self.free_tmp(self.do_set_ctx(
				Token(t.pos,'get',None,[ Token(t.pos,'name','__dict__'),item]),
				Token(t.pos,'get',None,[ Token(t.pos,'reg',v),item])
				)) #REG

	# do_globals 
	def do_globals(self,t):
		for t in t.items:
			if t.val not in D.globals:
				D.globals.append(t.val)

	# do_del
	def do_del(self,tt):
		for t in tt.items:
			r = self.do(t.items[0])
			r2 = self.do(t.items[1])
			self.do_code(OP_DEL,r,r2)
			self.free_tmp(r); self.free_tmp(r2) #REG

	# do_call
	def do_call(self,t,r=None):
		r = self.get_tmp(r)
		items = t.items
		fnc = self.do(items[0])
		a,b,c,d = self.p_filter(t.items[1:])
		e = None
		if len(b) != 0 or d != None:
			e = self.do(Token(t.pos,'dict',None,[])); self.un_tmp(e);
			for p in b:
				p.items[0].type = 'string'
				t1,t2 = self.do(p.items[0]),self.do(p.items[1])
				self.do_code(OP_SET,e,t1,t2)
				self.free_tmp(t1); self.free_tmp(t2) #REG
			if d: self.free_tmp(self.do(Token(t.pos,'call',None,[Token(t.pos,'name','merge'),Token(t.pos,'reg',e),d.items[0]]))) #REG
		self.manage_seq(OP_PARAMS,r,a)
		if c != None:
			t1,t2 = self._do_string('*'),self.do(c.items[0])
			self.do_code(OP_SET,r,t1,t2)
			self.free_tmp(t1); self.free_tmp(t2) #REG
		if e != None:
			t1 = self._do_none()
			self.do_code(OP_SET,r,t1,e)
			self.free_tmp(t1) #REG
		self.do_code(OP_CALL,r,fnc,r)
		self.free_tmp(fnc) #REG
		return r

	# do_name
	def do_name(self,t,r=None):
		if t.val in D.vars:
			return self.do_local(t,r)
		if t.val not in D.rglobals:
			D.rglobals.append(t.val)
		r = self.get_tmp(r)
		c = self.do_string(t)
		self.do_code(OP_GGET,r,c)
		self.free_tmp(c)
		return r

	# do_local
	def do_local(self,t,r=None):
		if t.val in D.rglobals:
			D.error = True
			raiseError('Encoder.do_local',D.source_code,t.pos)
		if t.val not in D.vars:
			D.vars.append(t.val)
		return self.get_reg(t.val)

	# do_def
	def do_def(self,tok,kls=None):
		items = tok.items

		t = self.get_tag()
		rf = self.do_function(t,'end')

		self.begin()
		self.setpos(tok.pos)
		r = self.do_local(Token(tok.pos,'name','__params'))
		self.do_info(items[0].val)
		a,b,c,d = self.p_filter(items[1].items)
		for p in a:
			v = self.do_local(p)
			tmp = self._do_none()
			self.do_code(OP_GET,v,r,tmp)
			self.free_tmp(tmp) #REG
		for p in b:
			v = self.do_local(p.items[0])
			self.do(p.items[1],v)
			tmp = self._do_none()
			self.do_code(OP_IGET,v,r,tmp)
			self.free_tmp(tmp) #REG
		if c != None:
			v = self.do_local(c.items[0])
			tmp = self._do_string('*')
			self.do_code(OP_GET,v,r,tmp)
			self.free_tmp(tmp) #REG
		if d != None:
			e = self.do_local(d.items[0])
			self.do_code(OP_DICT,e,0,0)
			tmp = self._do_none()
			self.do_code(OP_IGET,e,r,tmp)
			self.free_tmp(tmp) #REG
		self.free_tmp(self.do(items[2])) #REG
		self.end()

		self.tag(t,'end')

		if kls == None:
			if D._globals: self.do_globals(Token(tok.pos,0,0,[items[0]]))
			r = self.do_set_ctx(items[0],Token(tok.pos,'reg',rf))
		else:
			rn = self.do_string(items[0])
			self.do_code(OP_SET,kls,rn,rf)
			self.free_tmp(rn)

		self.free_tmp(rf)

	# do_class
	def do_class(self,t):
		tok = t
		items = t.items
		parent = None
		if items[0].type == 'name':
			name = items[0].val
			parent = Token(tok.pos,'name','object')
		else:
			name = items[0].items[0].val
			parent = items[0].items[1]

		kls = self.do(Token(t.pos,'dict',0,[]))
		self.un_tmp(kls)
		ts = self._do_string(name)
		self.do_code(OP_GSET,ts,kls)
		self.free_tmp(ts) #REG
    
		self.free_tmp(self.do(Token(tok.pos,'call',None,[
			Token(tok.pos,'name','setmeta'),
			Token(tok.pos,'reg',kls),
			parent])))
        
		for member in items[1].items:
			if member.type == 'def': self.do_def(member,kls)
			elif member.type == 'symbol' and member.val == '=': self.do_classvar(member,kls)
			else: continue
        
		self.free_reg(kls) #REG

	# do_classvar
	def do_classvar(self,t,r):
		var = self.do_string(t.items[0])
		val = self.do(t.items[1])
		self.do_code(OP_SET,r,var,val)
		self.free_reg(var)
		self.free_reg(val)
    
	# do_while
	def do_while(self,t):
		items = t.items
		t = self.stack_tag()
		self.tag(t,'begin')
		self.tag(t,'continue')
		r = self.do(items[0])
		self.do_code(OP_IF,r)
		self.free_tmp(r) #REG
		self.jump(t,'end')
		self.free_tmp(self.do(items[1])) #REG
		self.jump(t,'begin')
		self.tag(t,'break')
		self.tag(t,'end')
		self.pop_tag()

	# do_for
	def do_for(self,tok):
		items = tok.items

		reg = self.do_local(items[0])
		itr = self.do(items[1])
		i = self._do_number('0')

		t = self.stack_tag()
		self.tag(t,'loop')
		self.tag(t,'continue')
		self.do_code(OP_ITER,reg,itr,i)
		self.jump(t,'end')
		self.free_tmp(self.do(items[2])) #REG
		self.jump(t,'loop')
		self.tag(t,'break')
		self.tag(t,'end')
		self.pop_tag()

		self.free_tmp(itr) #REG
		self.free_tmp(i)

	# do_comp
	def do_comp(self,t,r=None):
		name = 'comp:'+self.get_tag()
		r = self.do_local(Token(t.pos,'name',name))
		self.do_code(OP_LIST,r,0,0)
		key = Token(t.pos,'get',None,[
				Token(t.pos,'reg',r),
				Token(t.pos,'symbol','None')])
		ap = Token(t.pos,'symbol','=',[key,t.items[0]])
		self.do(Token(t.pos,'for',None,[t.items[1],t.items[2],ap]))
		return r

	# do_if
	def do_if(self,t):
		items = t.items
		t = self.get_tag()
		n = 0
		for tt in items:
			self.tag(t,n)
			if tt.type == 'elif':
				a = self.do(tt.items[0]); self.do_code(OP_IF,a); self.free_tmp(a);
				self.jump(t,n+1)
				self.free_tmp(self.do(tt.items[1])) #REG
			elif tt.type == 'else':
				self.free_tmp(self.do(tt.items[0])) #REG
			else:
				raise
			self.jump(t,'end')
			n += 1
		self.tag(t,n)
		self.tag(t,'end')

	# do_try
	def do_try(self,t):
		items = t.items
		t = self.get_tag()
		self.setjmp(t,'except')
		self.free_tmp(self.do(items[0])) #REG
		self.do_code(OP_SETJMP,0)
		self.jump(t,'end')
		self.tag(t,'except')
		self.free_tmp(self.do(items[1].items[1])) #REG
		self.tag(t,'end')

	# do_return
	def do_return(self,t):
		if t.items: r = self.do(t.items[0])
		else: r = self._do_none()
		self.do_code(OP_RETURN,r)
		self.free_tmp(r)
		return

	# do_raise
	def do_raise(self,t):
		if t.items: r = self.do(t.items[0])
		else: r = self._do_none()
		self.do_code(OP_RAISE,r)
		self.free_tmp(r)
		return

	# do_statements
	def do_statements(self,t):
		for tt in t.items: self.free_tmp(self.do(tt))

	# do_list
	def do_list(self,t,r=None):
		r = self.get_tmp(r)
		self.manage_seq(OP_LIST,r,t.items)
		return r

	# do_dict
	def do_dict(self,t,r=None):
		r = self.get_tmp(r)
		self.manage_seq(OP_DICT,r,t.items)
		return r

	# do_get
	def do_get(self,t,r=None):
		items = t.items
		return self.infix(OP_GET,items[0],items[1],r)

	# do_break
	def do_break(self,t): 
		self.jump(D.tstack[-1],'break')

	# do_continue
	def do_continue(self,t): 
		self.jump(D.tstack[-1],'continue')

	# do_pass
	def do_pass(self,t): 
		self.do_code(OP_PASS)

	# do_info
	def do_info(self,name='?'):
		self.do_code(OP_FILE,self.free_tmp(self._do_string(D.filename)))
		self.do_code(OP_NAME,self.free_tmp(self._do_string(name)))

	# do_module
	def do_module(self,t):
		self.do_info()
		self.free_tmp(self.do(t.items[0])) #REG

	# do_reg
	def do_reg(self,t,r=None): 
		return t.val


	# logic_infix
	def logic_infix(self,op, tb, tc, _r=None):
		t = self.get_tag() 
		r = self.do(tb, _r)
		if _r != r: self.free_tmp(_r) #REG
		if op == 'and':   self.do_code(OP_IF, r)
		elif op == 'or':  self.do_code(OP_IFN, r)
		self.jump(t, 'end')
		_r = r
		r = self.do(tc, _r)
		if _r != r: self.free_tmp(_r) #REG
		self.tag(t, 'end')
		return r

	# _do_none
	def _do_none(self,r=None):
		r = self.get_tmp(r)
		self.do_code(OP_NONE,r)
		return r

	# do_symbol
	def do_symbol(self,t,r=None):
		sets = ['=']
		isets = ['+=','-=','*=','/=', '|=', '&=', '^=']
		cmps = ['<','>','<=','>=','==','!=']
		metas = {
			'+':OP_ADD,'*':OP_MUL,'/':OP_DIV,'**':OP_POW,
			'-':OP_SUB,
			'%':OP_MOD,'>>':OP_RSH,'<<':OP_LSH,
			'&':OP_BITAND,'|':OP_BITOR,'^':OP_BITXOR,
		}
		if t.val == 'None': return self._do_none(r)
		if t.val == 'True':
			return self._do_number('1',r)
		if t.val == 'False':
			return self._do_number('0',r)
		items = t.items

		if t.val in ['and','or']:
			return self.logic_infix(t.val, items[0], items[1], r)
		if t.val in isets:
			return self.imanage(t,self.do_symbol)
		if t.val == 'is':
			return self.infix(OP_EQ,items[0],items[1],r)
		if t.val == 'isnot':
			return self.infix(OP_CMP,items[0],items[1],r)
		if t.val == 'not':
			return self.unary(OP_NOT, items[0], r)
		if t.val == 'in':
			return self.infix(OP_HAS,items[1],items[0],r)
		if t.val == 'notin':
			r = self.infix(OP_HAS,items[1],items[0],r)
			zero = self._do_number('0')
			self.do_code(OP_EQ,r,r,self.free_tmp(zero))
			return r
		if t.val in sets:
			return self.do_set_ctx(items[0],items[1]);
		elif t.val in cmps:
			b,c = items[0],items[1]
			v = t.val
			if v[0] in ('>','>='):
				b,c,v = c,b,'<'+v[1:]
			cd = OP_EQ
			if v == '<': cd = OP_LT
			if v == '<=': cd = OP_LE
			if v == '!=': cd = OP_NE
			return self.infix(cd,b,c,r)
		else:
			return self.infix(metas[t.val],items[0],items[1],r)

	# do_set_ctx
	def do_set_ctx(self,k,v):
		if k.type == 'name':
			if (D._globals and k.val not in D.vars) or (k.val in D.globals):
				c = self.do_string(k)
				b = self.do(v)
				self.do_code(OP_GSET,c,b)
				self.free_tmp(c)
				self.free_tmp(b)
				return
			a = self.do_local(k)
			b = self.do(v)
			self.do_code(OP_MOVE,a,b)
			self.free_tmp(b)
			return a
		elif k.type in ('tuple','list'):
			if v.type in ('tuple','list'):
				n,tmps = 0,[]
				for kk in k.items:
					vv = v.items[n]
					tmp = self.get_tmp(); tmps.append(tmp)
					r = self.do(vv)
					self.do_code(OP_MOVE,tmp,r)
					self.free_tmp(r) #REG
					n+=1
				n = 0
				for kk in k.items:
					vv = v.items[n]
					tmp = tmps[n]
					self.free_tmp(self.do_set_ctx(kk,Token(vv.pos,'reg',tmp))) #REG
					n += 1
				return

			r = self.do(v); self.un_tmp(r)
			n, tmp = 0, Token(v.pos,'reg',r)
			for tt in k.items:
				self.free_tmp(self.do_set_ctx(tt,Token(tmp.pos,'get',None,[tmp,Token(tmp.pos,'number',str(n))]))) #REG
				n += 1
			self.free_reg(r)
			return
		r = self.do(k.items[0])
		rr = self.do(v)
		tmp = self.do(k.items[1])
		self.do_code(OP_SET,r,tmp,rr)
		self.free_tmp(r) #REG
		self.free_tmp(tmp) #REG
		return rr

	# manage_seq
	def manage_seq(self,i,a,items,sav=0):
		l = max(sav,len(items))
		n,tmps = 0,self.get_tmps(l)
		for tt in items:
			r = tmps[n]
			b = self.do(tt,r)
			if r != b:
				self.do_code(OP_MOVE,r,b)
				self.free_tmp(b)
			n +=1
		if not len(tmps):
			self.do_code(i,a,0,0)
			return 0
		self.do_code(i,a,tmps[0],len(items))
		self.free_tmps(tmps[sav:])
		return tmps[0]

	# p_filter
	def p_filter(self,items):
		a,b,c,d = [],[],None,None
		for t in items:
			if t.type == 'symbol' and t.val == '=': b.append(t)
			elif t.type == 'args': c = t
			elif t.type == 'nargs': d = t
			else: a.append(t)
		return a,b,c,d


	# is_tmp
	def is_tmp(self,r):
		if r is None: return False
		return (D.r2n[r][0] == '$')

	# un_tmp
	def un_tmp(self,r):
		n = D.r2n[r]
		self.free_reg(r)
		self.set_reg(r,'*'+n)

	# free_tmp
	def free_tmp(self,r):
		if self.is_tmp(r): self.free_reg(r)
		return r

	# free_tmps
	def free_tmps(self,r):
		for k in r: self.free_tmp(k)

	# get_reg
	def get_reg(self,n):
		if n not in D.n2r:
			self.set_reg(self.alloc(1),n)
		return D.n2r[n]

	# set_reg
	def set_reg(self,r,n):
		D.n2r[n] = r
		D.r2n[r] = n
		D.mreg = max(D.mreg,r+1)

	# free_reg
	def free_reg(self,r):
		if self.is_tmp(r): D.tmpc -= 1
		n = D.r2n[r]; del D.r2n[r]; del D.n2r[n]

	# imanage
	def imanage(self,orig,fnc):
		items = orig.items
		orig.val = orig.val[:-1]
		t = Token(orig.pos,'symbol','=',[items[0],orig])
		return fnc(t)

	# unary
	def unary(self,i,tb,r=None):
		r = self.get_tmp(r)
		b = self.do(tb)
		self.do_code(i,r,b)
		if r != b: self.free_tmp(b)
		return r

	# infix
	def infix(self,i,tb,tc,r=None):
		r = self.get_tmp(r)
		b,c = self.do(tb,r),self.do(tc)
		self.do_code(i,r,b,c)
		if r != b: self.free_tmp(b)
		self.free_tmp(c)
		return r


	# map_tags
	def map_tags(self):
		tags = {}
		out = []
		n = 0
		for item in D.out:
			if item[0] == 'tag':
				tags[item[1]] = n
				continue
			if item[0] == 'regs':
				out.append(self.get_code16(OP_REGS,item[1],0))
				n += 1
				continue
			out.append(item)
			n += 1
		for n in range(0,len(out)):
			item = out[n]
			if item[0] == 'jump':
				out[n] = self.get_code16(OP_JUMP,0,tags[item[1]]-n)
			elif item[0] == 'setjmp':
				out[n] = self.get_code16(OP_SETJMP,0,tags[item[1]]-n)
			elif item[0] == 'fnc':
				out[n] = self.get_code16(OP_DEF,item[1],tags[item[2]]-n)
		for n in range(0,len(out)):
			item = out[n]
			if item[0] == 'data':
				out[n] = item[1]
			elif item[0] == 'code':
				i,a,b,c = item[1:]
				out[n] = chr(i)+chr(a)+chr(b)+chr(c)
			else:
				raise str(('huh?',item))
			if len(out[n]) != 4:
				raise ('code '+str(n)+' is wrong length '+str(len(out[n])))
		D.out = out

	# get_tmp
	def get_tmp(self,r=None):
		if r != None: return r
		return self.get_tmps(1)[0]

	# get_tmps
	def get_tmps(self,t):
		rs = self.alloc(t)
		regs = range(rs,rs+t)
		for r in regs:
			self.set_reg(r,"$"+str(D._tmpi))
			D._tmpi += 1
		D.tmpc += t #REG
		return regs

	# alloc
	def alloc(self,t):
		s = ''.join(["01"[r in D.r2n] for r in range(0,min(256,D.mreg+t))])
		return s.index('0'*t)



	# do_string
	def do_string(self,t,r=None):
		return self._do_string(t.val,r)

	# _do_number
	def _do_number(self,v,r=None):
		r = self.get_tmp(r)
		self.do_code(OP_NUMBER,r,0,0)
		self.write(fpack(number(v)))
		return r

	# do_number
	def do_number(self,t,r=None):
		return self._do_number(t.val,r)

	# get_tag
	def get_tag(self):
		k = str(D._tagi)
		D._tagi += 1
		return k

	# stack_tag
	def stack_tag(self):
		k = self.get_tag()
		D.tstack.append(k)
		return k

	# pop_tag
	def pop_tag(self):
		D.tstack.pop()


	# do_function
	def do_function(self,*t):
		t = D.snum+':'+':'.join([str(v) for v in t])
		r = self.get_reg(t)
		self.insert(('fnc',r,t))
		return r

	# tag
	def tag(self,*t):
		t = D.snum+':'+':'.join([str(v) for v in t])
		self.insert(('tag',t))

	# jump
	def jump(self,*t):
		t = D.snum+':'+':'.join([str(v) for v in t])
		self.insert(('jump',t))

	# setjmp
	def setjmp(self,*t):
		t = D.snum+':'+':'.join([str(v) for v in t])
		self.insert(('setjmp',t))

	# do
	def do(self,t,r=None):
		if t.pos: self.setpos(t.pos)
		try:
			if t.type in self.rmap:
				return self.rmap[t.type](t,r)
			return self.fmap[t.type](t)
		except:
			if D.error: raise
			D.error = True
			raiseError('Encoder.do',D.source_code,t.pos)


	# doEncode
	def doEncode(self,filename,s,t):
		t = Token((1,1),'module','module',[t])
		global D
		tokenizer=Tokenizer()
		s = tokenizer.clean(s)

		class Status:
			pass

		D = Status()
		D.source_code = s
		D.filename = filename
		D.lines = D.source_code.split('\n')
		D.stack,D.out,D._scopei,D.tstack,D._tagi,D.data = [],[('tag','OP_EOF')],0,[],0,{}
		D.error = False	

		self.begin(True)
		self.do(t)
		self.end()
		self.map_tags()
		out = D.out; D = None
		return ''.join(out)

# compileFile
def compileFile(source_code,filename):
	tokens   = Tokenizer().doTokenize(source_code)
	tokens   = Parser().doParse(source_code,tokens)
	bytecode = Encoder().doEncode(filename,source_code,tokens)
	return bytecode

    
# importModule
def importModule(modulename):

	if modulename in MODULES:
		return MODULES[modulename]

	filename=modulename+'.py'
	source_code = loadFile(filename)
	bytecode = compileFile(source_code,filename)

	module = {}
	module['__name__'] = modulename
	module['__code__'] = bytecode
	module['__dict__'] = module

	MODULES[modulename] = module

	exec(bytecode,module)
	return module


# hexDump
def hexDump(data,buffer_name,cols = 16) :
	out = []
	out.append("unsigned char " + buffer_name +"[] = {")
	for n in range(0,len(data),cols):
		out.append(",".join([str(ord(v)) for v in data[n:n+cols]])+',')
	out.append("""};""")
	out.append("")
	return '\n'.join(out)

# generateByteCode
def generateByteCode(filename='tinypy.bytecode.h'):
	f = open('tinypy.bytecode.h','wb')
	f.write("/*This file is autogenerated by generate_bytecode.py*/\n\n")
	for modulefilename in ['tinypy']:
		src_filename=modulefilename+'.py'
		source_code = loadFile(src_filename)
		bytecode = compileFile(source_code,src_filename)
		f.write(hexDump(bytecode,'py_'+modulefilename)+"\n")
	f.close()

# main
if __name__ == '__main__' and not "tinypy" in sys.version:
	print("Generating bytecode!")
	generateByteCode()

