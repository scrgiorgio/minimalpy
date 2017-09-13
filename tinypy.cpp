
#include "tinypy.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>

#include <math.h>
#include <errno.h>

#include "tinypy.bytecode.h"

#ifdef _WIN32
#pragma warning(disable:4267 4244 4996)
#endif

SharedPtr<Object> NoneObject = std::make_shared<Object>(nullptr, Object::NoneType);


enum {
  OP_EOF,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_POW,
  OP_BITAND,
  OP_BITOR,
  OP_CMP,
  OP_GET,
  OP_SET,
  OP_NUMBER,
  OP_STRING,
  OP_GGET,
  OP_GSET,
  OP_MOVE,
  OP_DEF,
  OP_PASS,
  OP_JUMP,
  OP_CALL,
  OP_RETURN,
  OP_IF,
  OP_DEBUG,
  OP_EQ,
  OP_LE,
  OP_LT,
  OP_DICT,
  OP_LIST,
  OP_NONE,
  OP_LEN,
  OP_LINE,
  OP_PARAMS,
  OP_IGET,
  OP_FILE,
  OP_NAME,
  OP_NE,
  OP_HAS,
  OP_RAISE,
  OP_SETJMP,
  OP_MOD,
  OP_LSH,
  OP_RSH,
  OP_ITER,
  OP_DEL,
  OP_REGS,
  OP_BITXOR,
  OP_IFN,
  OP_NOT,
  OP_BITNOT,
  OP_TOTAL
};



////////////////////////////////////////////////////////////////////////
int Object::compare(SharedPtr<Object> a, SharedPtr<Object> b)
{
  if (a->type != b->type)
    return a->type - b->type;

  switch (a->type) {

  case Object::NoneType:
    return 0;

  case Object::NumberType:
  {
    auto na = a->castToNumber()->val;
    auto nb = b->castToNumber()->val;
    auto v = na  - nb;
    return (v < 0 ? -1 : (v > 0 ? 1 : 0));
  }

  case Object::StringType:
  {
    auto sa = a->castToString()->val;
    auto sb = b->castToString()->val;
    int len = std::min(sa.length(), sb.length());
    int v = memcmp(sa.c_str(), sb.c_str(), len);
    return v? v : (sa.length() - sb.length());
  }

  case Object::ListType:
  {
    auto la = a->castToList();
    auto lb = b->castToList();

    int len = std::min(la->size(), lb->size());
    for (int n = 0; n < len ; n++)
    {
      auto aa = la->getAt(n);
      auto bb = lb->getAt(n);

      int v;
      if (aa->type == Object::ListType && bb->type == Object::ListType)
        v = aa->castToList().get() - bb->castToList().get();
      else
        v = compare(aa, bb);
      if (v)
        return v;
    }
    return la->size() - lb->size();
  }

  case Object::DictType: {
    return a.get() - b.get();
  }

  case Object::FunctionType:
    return a.get() - b.get();

  }

  raiseException("(py_cmp) TypeError: ?");
  return 0;
}


///////////////////////////////////////////////////////////////////////////////////
SharedPtr<Object> StringObject::getAttr(SharedPtr<Object> k)
{
  auto s = this->val;

  if (k->type == Object::NumberType)
  {
    int n = k->castToNumber()->val;
    int l = s.length();
    n = (n < 0 ? l + n : n);
    if (n >= 0 && n < l) {
      return py->createString(String(&s[n], 1));
    }
  }
  else if (k->type == Object::StringType)
  {
    auto ks = k->castToString()->val;

    //join
    if (ks == "join")
    {
      return py->createMethod(self(), [](PyEngine *py, SharedPtr<ListObject> args)
      {
        auto delim = args->getStringAt(0);
        auto list = args->getListAt(1);

        std::ostringstream out;
        for (int i = 0; i < list->size(); i++)
        {
          if (i) out << delim;
          out << list->getAt(i)->toString();
        }

        return py->createString(out.str());
      });
    }

    //split
    if (ks == "split")
    {
      return py->createMethod(self(), [](PyEngine *py, SharedPtr<ListObject> args)
      {
        String v = args->getStringAt(0);
        String d = args->getStringAt(1);
        auto ret = py->createList();
        for (auto it : StringUtils::split(v, d))
          ret->pushBack(py->createString(it.c_str()));
        return ret;
      });
    }

    //index
    if (ks == "index")
    {
      return py->createMethod(self(), [](PyEngine *py, SharedPtr<ListObject> args)
      {
        auto s = args->getStringAt(0);
        auto v = args->getStringAt(1);
        int n = (int)s.find(v);
        if (n<0)
          raiseException("(index) ValueError: substring not found");
        return py->createNumber(n);
      });
    }

    //strip
    if (ks == "strip")
    {
      return py->createMethod(self(), [](PyEngine *py, SharedPtr<ListObject> args)
      {
        auto s = args->getStringAt(0);
        s = StringUtils::trim(s);
        return py->createString(s);
      });
    }

    //replace
    if (ks == "replace")
    {
      return py->createMethod(self(), [](PyEngine *py, SharedPtr<ListObject> args)
      {
        auto s = args->getStringAt(0);
        auto k = args->getStringAt(1);
        auto v = args->getStringAt(2);
        return py->createString(StringUtils::replaceAll(s, k, v));
      });
    }
  }

  //extract substring
  if (k->type == Object::ListType)
  {
    auto extractNumber = [](SharedPtr<Object> value, int default_value) {

      if (value->type == Object::NumberType)
        return (int)value->castToNumber()->val;

      if (value->type == Object::NoneType)
        return default_value;

      raiseException("(pyGet) TypeError: indices must be numbers");
      return 0;
    };

    auto klist = k->castToList();

    int l = this->len();
    int a = extractNumber(klist->getAt(0), 0);
    int b = extractNumber(klist->getAt(1), l);

    a = std::max(0, (a < 0 ? l + a : a));
    b = std::min(l, (b < 0 ? l + b : b));

    return py->createString(StringUtils::substring(this->val, a, b));
  }

  return Object::getAttr(k);
}


///////////////////////////////////////////////////////////////////////////////////
SharedPtr<Object> ListObject::getAttr(SharedPtr<Object> k)
{
  if (k->type == Object::NumberType)
  {
    int n = k->castToNumber()->val;
    int len = this->size();
    if (n<0) n = len + n;

    if (n<0 || n >= len)
      raiseException("(py_list_get) KeyError");

    return this->getAt(n);
  }

  if (k->type == Object::StringType)
  {
    auto s = k->castToString()->val;

    if (s == "append")
    {
      return py->createMethod(self(), [](PyEngine *py, SharedPtr<ListObject> args) {
        auto list = args->getListAt(0);
        auto obj = args->getAt(1);
        list->pushBack(obj);
        return NoneObject;
      });
    }

    if (s == "pop")
    {
      return py->createMethod(self(), [](PyEngine *py, SharedPtr<ListObject> args) {
        return args->getListAt(0)->popBack();
      });
    }

    if (s == "index")
    {
      return py->createMethod(self(), [](PyEngine *py, SharedPtr<ListObject> args)
      {
        auto list = args->getListAt(0);
        auto what = args->getAt(1);

        int i = list->find(what);
        if (i < 0) {
          raiseException(StringUtils::format() 
            << "List " << list->toString() << " does not contain index " << what->toString());
        }

        return py->createNumber(i);
      });
    }

    if (s == "sort")
    {
      return py->createMethod(self(), [](PyEngine *py, SharedPtr<ListObject> args) {
        auto list = args->getListAt(0);
        list->sort();
        return NoneObject;
      });
    }

    if (s == "extend")
    {
      return py->createMethod(self(), [](PyEngine *py, SharedPtr<ListObject> args)
      {
        auto l1 = args->getListAt(0);
        auto l2 = args->getListAt(1);
        l1->append(*l2);
        return NoneObject;
      });
    }

    if (s == "*")
    {
      auto ret = self()->clone();
      this->clear(); //why clear here???
      return ret;
    }
  }

  else if (k->type == Object::NoneType) {
    return this->popFront();
  }

  //___________________________________________________
  if (k->type == Object::ListType)
  {
    auto extractNumber = [](SharedPtr<Object> value, int default_value) {

      if (value->type == Object::NumberType)
        return (int)value->castToNumber()->val;

      if (value->type == Object::NoneType)
        return default_value;

      raiseException("(pyGet) TypeError: indices must be numbers");
      return 0;
    };

    auto klist = k->castToList();

    int l = this->len();
    int a = extractNumber(klist->getAt(0),0);
    int b = extractNumber(klist->getAt(1),l);
    
    a = std::max(0, (a < 0 ? l + a : a));
    b = std::min(l, (b < 0 ? l + b : b));

    auto ret = py->createList();
    for (int i = 0; i < b - a; i++)
      ret->pushBack(this->getAt(a + i));
    return ret;
  }

  return Object::getAttr(k);
}



/////////////////////////////////////////////////////////////////////////////////////////
void ListObject::setAttr(SharedPtr<Object> k, SharedPtr<Object> v)
{
  if (k->type == Object::NumberType)
  {
    int index = k->castToNumber()->val;

    if (index >= this->size())
      raiseException("(py_list_set) KeyError");

    this->setAt(index, v);
    return;
  }

  if (k->type == Object::NoneType)
  {
    this->pushBack(v);
    return;
  }

  if (k->type == Object::StringType)
  {
    auto ks = k->castToString()->val;
    if (ks == "*")
    {
      if (auto other = v->castToList())
        this->append(*other);
      return;
    }
  }

  return Object::setAttr(k,v);
}

///////////////////////////////////////////////////////////////////////////////////
SharedPtr<Object> DictObject::getAttr(SharedPtr<Object> k)
{
  if (this->isObject())
  {
    if (auto get_function = this->lookup(py->createString("__get__")))
      return get_function->call(py->createList({ k }));
  }

  if (auto ret = this->lookup(k))
    return ret;

  auto it = this->map.find(k);
  if (it == this->end())
  {
    std::vector<String> keys;
    for (auto jt : *this)
      keys.push_back(jt.first->toString());

    auto skey = k->toString();
    raiseException(StringUtils::format() << "(py_dict_get) KeyError: " << skey << " not in " << StringUtils::join(keys, ",", "{", "}"));
  }

  return it->second;
}


/////////////////////////////////////////////////////////////////////////////////////////
void DictObject::setAttr(SharedPtr<Object> k, SharedPtr<Object> v)
{
  if (this->isObject())
  {
    if (auto set_function = this->lookup(py->createString("__set__"))) 
    {
      set_function->call(py->createList({ k, v }));
      return;
    }
  }

  this->map[k] = v;
}


//////////////////////////////////////////////////////////////////////
SharedPtr<Object> DictObject::lookup(SharedPtr<Object> k, int depth)
{
  auto it = this->map.find(k);
  if (it != this->end())
    return it->second;

  if (!--depth)
    raiseException("(pyLookUp) RuntimeError: maximum lookup depth exceeded");

  if (this->meta)
  {
    if (auto ret = this->meta->lookup(k, depth))
    {
      if (this->isObject() && ret->type == Object::FunctionType) 
      {
        auto fn = ret->castToFunction();
        ret = py->createFunction(fn->function, self(), fn->bytecode, fn->globals);
      }
      return ret;
    }
  }

  return SharedPtr<Object>();
}

//////////////////////////////////////////////////////////////////////
SharedPtr<Object> DictObject::call(SharedPtr<ListObject> args)
{
  if (this->isDict())
  {
    if (auto new_function = this->lookup(py->createString("__new__")))
    {
      args->pushFront(self());
      return new_function->call(args);
    }
  }
  else
  {
    assert(this->isObject());

    if (auto call_function = this->lookup(py->createString("__call__")))
      return call_function->call(args);
  }

  return Object::call(args);
}

//////////////////////////////////////////////////////////////////////
SharedPtr<Object> FunctionObject::call(SharedPtr<ListObject> args)
{
  if (this->function)
  {
    //if is method I need to insert the obj as first 
    if (this->instance)
      args->pushFront(this->instance);

    return this->function(py, args);
  }
  else
  {
    auto list = py->createList(args->items);

    //if is method I need to insert the obj as first 
    if (this->instance)
      list->pushFront(this->instance);

    return py->runFrame(this->bytecode, { list }, this->globals);
  }

  return Object::call(args);
}

////////////////////////////////////////////////////////////////////////////////
PyEngine::PyEngine(int argc, char *argv[])
{
  this->builtins = this->createDict();
  this->modules  = this->createDict();

  this->builtins->setAttr(this->createString("MODULES" ), this->modules);
  this->modules ->setAttr(this->createString("BUILTINS"), this->builtins);
  this->builtins->setAttr(this->createString("BUILTINS"), this->builtins);

  auto sys = this->createDict();
  sys->setAttr(this->createString("version"), this->createString("tinypy"));
  modules->setAttr(this->createString("sys"), sys);

  addBuiltIns();

  auto bytecode = String((char*)py_tinypy, sizeof(py_tinypy));
  auto module = this->createDict();
  module->setAttr(createString("__name__"), this->createString("tinypy"));
  module->setAttr(createString("__code__"), this->createString(bytecode));
  module->setAttr(createString("__dict__"), module);
  
  modules->setAttr(createString("tinypy"), module);

  runFrame(bytecode, {}, module);
}

////////////////////////////////////////////////////////////////////
PyEngine::~PyEngine()
{
}

///////////////////////////////////////////////////////////////////////////////
int PyEngine::runStep()
{
  auto bitwise_builtin_op=[&](String name, SharedPtr<Object> a, SharedPtr<Object> b, std::function<double(double, double)> fn)
  {
    if (a->type == Object::NumberType && b->type == Object::NumberType)
    {
      auto na = a->castToNumber()->val;
      auto nb = b->castToNumber()->val;
      return this->createNumber(fn(na, nb));
    }

    raiseException(StringUtils::format() << name << " TypeError: unsupported operand type(s)");
    return SharedPtr<NumberObject>();
  };


  while (1)
  {
    auto frame = frames.top();

    auto& regs = frame->regs;

    auto ptr = (unsigned char*)&frame->bytecode[frame->cursor];

    int op = ptr[0];
    int A = (int)ptr[1]; 
    int B = (int)ptr[2]; 
    int C = (int)ptr[3]; 

    frame->cursor += 4;
    auto Number = short((B<<8)+C);

    switch (op)
    {
    case OP_EOF:
      popFrame(NoneObject);
      return 0;

    case OP_ADD: {

      auto add=[&](SharedPtr<Object> a, SharedPtr<Object> b)->SharedPtr<Object>
      {
        if (a->type == Object::NumberType && a->type == b->type)
        {
          auto na = a->castToNumber()->val;
          auto nb = b->castToNumber()->val;
          return this->createNumber(na + nb);
        }

        if (a->type == Object::StringType && a->type == b->type)
        {
          auto as = a->castToString()->val;
          auto bs = b->castToString()->val;
          return this->createString(as + bs);
        }

        if (a->type == Object::ListType && a->type == b->type)
        {
          auto la = a->castToList();
          auto lb = b->castToList();
          auto ret = this->createList();
          ret->append(*la);
          ret->append(*lb);
          return ret;
        }
        raiseException("(pyAdd) TypeError: ?");
        return NoneObject;
      };

      regs[A] = add(regs[B], regs[C]);
      continue;
    }

    case OP_SUB: {

      auto sub=[&](SharedPtr<Object> a, SharedPtr<Object> b) {
        return bitwise_builtin_op("bitwise_sub", a, b, [](double a, double b) {return a - b; });
      };

      regs[A] = sub(regs[B], regs[C]);
      continue;
    }

    case OP_MUL: {

      auto mul=[&](SharedPtr<Object> a, SharedPtr<Object> b)->SharedPtr<Object>
      {
        if (a->type == Object::NumberType && a->type == b->type) {
          auto na = a->castToNumber()->val;
          auto nb = b->castToNumber()->val;
          return this->createNumber(na * nb);
        }

        if ((a->type == Object::StringType && b->type == Object::NumberType) ||
          (a->type == Object::NumberType && b->type == Object::StringType))
        {
          if (a->type == Object::NumberType)
            std::swap(a, b);

          auto as = a->castToString()->val;
          int  n = b->castToNumber()->val;

          std::ostringstream out;
          for (int I = 0; I < n; I++)
            out << as;
          return this->createString(out.str());
        }

        raiseException("(pyMul) TypeError: ?");
        return NoneObject;
      };

      regs[A] = mul(regs[B], regs[C]);
      continue;
    }

    case OP_DIV: {

      auto div=[&](SharedPtr<Object> a, SharedPtr<Object> b) {
        return bitwise_builtin_op("bitwise_div", a, b, [](double a, double b) {return a / b; });
      };

      regs[A] = div(regs[B], regs[C]);
      continue;
    }

    case OP_POW: {

      auto pow=[&](SharedPtr<Object> a, SharedPtr<Object> b) {
        return bitwise_builtin_op("bitwise_pow", a, b, [](double a, double b) {return ::pow(a, b); });
      };

      regs[A] = pow(regs[B], regs[C]);
      continue;
    }

    case OP_BITAND: {

      auto bitwiseAnd=[&](SharedPtr<Object> a, SharedPtr<Object> b) {
        return bitwise_builtin_op("bitwise_and", a, b, [](double a, double b) {return ((long)a) &  ((long)b); });
      };

      regs[A] = bitwiseAnd(regs[B], regs[C]);
      continue;
    }

    case OP_BITOR: {

      auto bitwiseOr=[&](SharedPtr<Object> a, SharedPtr<Object> b) {
        return bitwise_builtin_op("bitwise_or", a, b, [](double a, double b) {return ((long)a) | ((long)b); });
      };

      regs[A] = bitwiseOr(regs[B], regs[C]);
      continue;
    }

    case OP_BITXOR: {

      auto bitwiseXor=[&](SharedPtr<Object> a, SharedPtr<Object> b) {
        return bitwise_builtin_op("bitwise_xor", a, b, [](double a, double b) {return ((long)a) ^ ((long)b); });
      };

      regs[A] = bitwiseXor(regs[B], regs[C]);
      continue;
    }

    case OP_MOD: {

      auto mod=[&](SharedPtr<Object> a, SharedPtr<Object> b) {
        return bitwise_builtin_op("bitwise_mod", a, b, [](double a, double b) {return ((long)a) % ((long)b); });
      };

      regs[A] = mod(regs[B], regs[C]);
      continue;
    }

    case OP_LSH: {

      auto lsh=[&](SharedPtr<Object> a, SharedPtr<Object> b) {
        return bitwise_builtin_op("bitwise_lsh", a, b, [](double a, double b) {return ((long)a) << ((long)b); });
      };

      regs[A] = lsh(regs[B], regs[C]);
      continue;
    }

    case OP_RSH: {

      auto rsh=[&](SharedPtr<Object> a, SharedPtr<Object> b) {
        return bitwise_builtin_op("bitwise_rsh", a, b, [](double a, double b) {return ((long)a) >> ((long)b); });
      };

      regs[A] = rsh(regs[B], regs[C]);
      continue;
    }

    case OP_CMP:
      regs[A] = this->createNumber(Object::compare(regs[B], regs[C])); 
      continue;

    case OP_NE:
      regs[A] = this->createNumber(Object::compare(regs[B], regs[C]) != 0);
      continue;

    case OP_EQ:
      regs[A] = this->createNumber(Object::compare(regs[B], regs[C]) == 0);
      continue;

    case OP_LE:
      regs[A] = this->createNumber(Object::compare(regs[B], regs[C]) <= 0);
      continue;

    case OP_LT:
      regs[A] = this->createNumber(Object::compare(regs[B], regs[C]) < 0);
      continue;

    case OP_BITNOT: {

      auto bitwiseNot = [&](SharedPtr<Object> a)
      {
        if (a->type != Object::NumberType)
          raiseException("(py_bitwise_not) TypeError: unsupported operand type");
        return this->createNumber(~(long)a->castToNumber()->val);
      };

      regs[A] = bitwiseNot(regs[B]);
      continue;
    }

    case OP_NOT:
      regs[A] = this->createNumber(regs[B]->toBool()?0:1);
      continue;

    case OP_PASS:
      continue;

    case OP_IF: 
      if (regs[A]->toBool())
        frame->cursor += 4;
      continue;

    case OP_IFN:
      if (!regs[A]->toBool())
        frame->cursor += 4;
      continue;

    case OP_GET:
      regs[A] = regs[B]->getAttr(regs[C]);
      continue;

    case OP_ITER:
    {
      assert(regs[C]->type==Object::NumberType);
      auto& index = regs[C]->castToNumber()->val;

      if (index < regs[B]->len())
      {
        //regs[A]=regs[B][regs[C]]
        if (regs[B]->type == Object::ListType || regs[B]->type == Object::StringType)
        {
          regs[A] = regs[B]->getAttr(regs[C]);
        }
        else  if (regs[B]->type == Object::DictType && regs[C]->type == Object::NumberType) 
        {
          auto dictB = regs[B]->castToDict();
          int  offset = (int)regs[C]->castToNumber()->val;
          auto itB = dictB->begin();
          std::advance(itB, offset);
          regs[A] = itB->first;
        }

        else
          raiseException("(py_iter) TypeError: iteration over non-sequence");

        index += 1;
        frame->cursor += 4;
      }
      continue;
    }

    case OP_HAS:
      regs[A] = this->createNumber(regs[B]->hasAttr(regs[C])?1:0);
      continue;

    case OP_IGET: {

      auto igetAttr=[&](SharedPtr<Object> obj, SharedPtr<Object> k)
      {
        if (obj->type == Object::DictType)
        {
          auto dict = obj->castToDict();
          auto it = dict->map.find(k);
          if (it == dict->end())
            return SharedPtr<Object>();
        }

        if (obj->type == Object::ListType)
        {
          auto list = obj->castToList();
          if (!list->size())
            return SharedPtr<Object>();
        }

        auto ret = obj->getAttr(k);
        assert(ret);
        return ret;
      };

      if (auto found = igetAttr(regs[B], regs[C]))
        regs[A] = found;
      continue;
    }

    case OP_SET:
      regs[A]->setAttr(regs[B], regs[C]);
      continue;

    case OP_DEL:
      regs[A]->delAttr(regs[B]);
      continue;

    case OP_MOVE:
      regs[A] = regs[B]; 
      continue;

    case OP_NUMBER:
      regs[A] = this->createNumber(*(double*)&frame->bytecode[frame->cursor]);
      frame->cursor += sizeof(double);
      continue;

    case OP_STRING:
      regs[A] = this->createString(String((char*)&frame->bytecode[frame->cursor], Number));
      frame->cursor += ((Number / 4)+1)*4;
      continue;

    case OP_DICT:
      regs[A] = this->createDict();
      for (int i = 0; i < C; i+=2) 
        regs[A]->setAttr(regs[B+i+0], regs[B+i+1]);
      continue;

    case OP_LIST: 
    case OP_PARAMS: {
      auto listA = this->createList();
      regs[A] = listA;
      for (int i = 0; i < C; i++)
        listA->pushBack(regs[B + i]);
      continue;
    }

    case OP_LEN: 
      regs[A] = this->createNumber(regs[B]->len());
      continue;

    case OP_JUMP: 
      frame->cursor += (Number -1)*4;
      continue;

    case OP_SETJMP: 
      frame->jump_to = Number ? (frame->cursor + (Number -1)*4) : (-1);
      continue;

    case OP_CALL:
      regs[A] = regs[B]->call(regs[C]->castToList());
      return 0;

    case OP_GGET: {
      auto it=frame->globals->map.find(regs[B]);
      regs[A] = (it != frame->globals->map.end())? (it->second) : (builtins->getAttr(regs[B]));
      continue;
    }

    case OP_GSET:
      frame->globals->setAttr(regs[A], regs[B]);
      continue;

    case OP_DEF: {
      auto bytecode = String(&frame->bytecode[frame->cursor], (Number - 1) * 4);
      regs[A] = this->createFunction(nullptr, SharedPtr<Object>(), bytecode , frame->globals);
      frame->cursor += (Number - 1) * 4;
      continue;
    }

    case OP_RETURN: 
      frame->cursor -= 4;
      popFrame(regs[A]); 
      return 0;

    case OP_RAISE: 
      frame->cursor -= 4;
      raiseException(regs[A]); 
      return 0;

    case OP_DEBUG: 
      std::cout << "DEBUG: " << A << " " << regs[A]->toString();
      continue;

    case OP_NONE: 
      regs[A] = NoneObject; 
      continue;

    case OP_LINE:
      frame->line = String((char*)&frame->bytecode[frame->cursor], A * 4 - 1);;
      frame->lineno = Number;
      frame->cursor += A*4;
      continue;

    case OP_FILE: 
      frame->filename = regs[A]->toString();
      continue;

    case OP_NAME: 
      frame->name = regs[A]->toString();
      continue;

    case OP_REGS:  
      //don't care
      continue;

    default:
      raiseException("(py_step) RuntimeError: invalid instruction");
      return 0;
      continue;
    }
  }
  return 0;
}

//////////////////////////////////////////////////////////
void PyEngine::run()
{
  this->is_running++;

  bool bCatched = false;
  SharedPtr<Object> ex;
  try
  {
    int upto = this->frames.size();
    while (frames.size() >= upto)
    {
      //error happened?
      if (this->runStep() == -1)
        break;
    }
  }
  catch (String s) {
    ex = this->createString(s);
    bCatched = true;
  }
  catch (SharedPtr<Object> ex_) {
    ex = ex_;
    bCatched = true;
  }

  this->is_running--;

  if (!bCatched)
    return;


  std::ostringstream out;
  while (!frames.empty())
  {
    auto frame = frames.top();
    if (frame->jump_to >= 0)
    {
      frame->cursor = frame->jump_to;
      frame->jump_to = -1;
      return;
    }

    out<< "filename(" << frame->filename << ") line(" << frame->lineno << ") name(" << frame->name << ") "<<std::endl<< frame->line << std::endl;
    frames.pop();
  }

  std::cout << std::endl << "Exception:" << std::endl << out.str() << ex->toString()<<std::endl;;
  assert(false);
  exit(-1);
}

////////////////////////////////////////////////////////////////////////////////
void PyEngine::addBuiltIns()
{
  //addBuiltIn
  auto addBuiltIn = [&](String name, Function fn) {
    builtins->setAttr(this->createString(name), this->createFunction(fn));
  };

  addBuiltIn("print", [](PyEngine* py,SharedPtr<ListObject> args) {
    args->print();
    return NoneObject;
  });

  addBuiltIn("range", [](PyEngine* py, SharedPtr<ListObject> args)
  {
    int a = 0, b = 0, c = 1;

    if (args->size() == 1)
    {
      b = args->getNumberAt(0);
    }
    else if (args->size() == 2)
    {
      a = args->getNumberAt(0);
      b = args->getNumberAt(1);
    }
    else if (args->size() == 3)
    {
      a = args->getNumberAt(0);
      b = args->getNumberAt(1);
      c = args->getNumberAt(2);
    }
    else
    {
      raiseException("wrong arguments for range");
    }

    auto ret = py->createList();
    for (int I = a; (c > 0) ? I<b : I>b; I += c)
      ret->pushBack(py->createNumber(I));
    return ret;
  });

  addBuiltIn("min",[](PyEngine *py, SharedPtr<ListObject> args)
  {
    auto ret = args->getAt(0);
    for (auto arg : *args) {
      if (Object::compare(ret, arg) > 0)
        ret = arg;
    }
    return ret;
  });

  addBuiltIn("max",[](PyEngine *py, SharedPtr<ListObject> args)
  {
    auto ret = args->getAt(0);
    for (auto arg : *args) {
      if (Object::compare(ret, arg) < 0)
        ret = arg;
    }
    return ret;
  });

  addBuiltIn("bind",[](PyEngine *py, SharedPtr<ListObject> args) {
    auto ret  = args->getFunctionAt(0);
    auto obj  = args->getAt(1);
    return py->createFunction(ret->function, obj, ret->bytecode,ret->globals);
  });
  
  addBuiltIn("copy", [](PyEngine* py, SharedPtr<ListObject> args) {
    auto obj = args->getAt(0);
    return obj->clone();
  });

  addBuiltIn("import",[](PyEngine *py, SharedPtr<ListObject> args) {
    String modulename = args->getStringAt(0);

    if (py->modules->hasAttr(py->createString(modulename)))
      return py->modules->getAttr(py->createString(modulename));

    return py->callFunctionInModule("tinypy", "importModule", py->createList({py->createString(modulename) }));
  });

  addBuiltIn("len",[](PyEngine *py, SharedPtr<ListObject> args) {
    auto obj = args->getAt(0);
    return py->createNumber(obj->len());
  });

  addBuiltIn("assert",[](PyEngine *py, SharedPtr<ListObject> args) {
    int a = args->getNumberAt(0);
    if (a) { return NoneObject; }
    raiseException("(py_assert) AssertionError");
    return NoneObject;
  });

  addBuiltIn("str",[](PyEngine *py, SharedPtr<ListObject> args) {
    auto obj = args->getAt(0);
    return py->createString(obj->toString());
  });

  auto pyCreateFloat= [](PyEngine* py, SharedPtr<ListObject> args) {

    auto obj = args->getAt(0);

    if (obj->type == Object::NumberType)
      return obj->castToNumber();

    if (obj->type == Object::StringType)
      return py->createNumber(std::stod(obj->castToString()->val));

    raiseException("(pyCreateFloat) TypeError: ?");
    return SharedPtr<NumberObject>();
  };

  addBuiltIn("float", pyCreateFloat);

  //too dangerous
#if 0
  addBuiltIn("system", [](PyEngine *py, SharedPtr<ListObject> args)
  {
    auto s = args->getStringAt(0);
    int ret = system(s.c_str());
    return NumberObject::New(ret);
  });
#endif

  addBuiltIn("istype",[](PyEngine *py, SharedPtr<ListObject> args) {
    auto v = args->getAt(0);
    auto t = args->getStringAt(1);

    if (t == "string") return py->createNumber(v->type == Object::StringType);
    if (t == "list"  ) return py->createNumber(v->type == Object::ListType);
    if (t == "dict"  ) return py->createNumber(v->type == Object::DictType);
    if (t == "number") return py->createNumber(v->type == Object::NumberType);
    if (t == "fnc"   ) return py->createNumber(v->type == Object::FunctionType && !v->castToFunction()->instance);
    if (t == "method") return py->createNumber(v->type == Object::FunctionType &&  v->castToFunction()->instance);

    raiseException("(is_type) TypeError: ?");
    return SharedPtr<NumberObject>();
  });

  addBuiltIn("chr",[](PyEngine *py, SharedPtr<ListObject> args) {
    auto v = (unsigned char)args->getNumberAt(0);
    return py->createString(String((const char*)&v, 1));
  });

  addBuiltIn("saveFile",[](PyEngine *py, SharedPtr<ListObject> args)
  {
    String filename = args->getStringAt(0);
    String content = args->getStringAt(1);

    FILE * file = fopen(filename.c_str(), "wb");
    if (!file) raiseException("(py_save) IOError: ?");
    fwrite(content.c_str(), content.length(), 1, file);
    fclose(file);
    return NoneObject;
  });

  addBuiltIn("loadFile", [](PyEngine* py, SharedPtr<ListObject> args) {
    auto filename = args->getStringAt(0);
    auto content = py->loadFile(filename);
    return py->createString(content);
  });

  addBuiltIn("fpack",[](PyEngine *py, SharedPtr<ListObject> args)
  {
    double v = args->getNumberAt(0);
    auto ret = py->createString(String(sizeof(double), 0));
    *((double*)ret->val.c_str()) = v;
    return ret;
  });

  addBuiltIn("abs",[pyCreateFloat](PyEngine *py, SharedPtr<ListObject> args) {
    auto value = fabs(pyCreateFloat(py, args)->castToNumber()->val);
    return py->createNumber(value);
  });
  
  addBuiltIn("int", [pyCreateFloat](PyEngine *py, SharedPtr<ListObject> args) {
    auto value = (long)pyCreateFloat(py, args)->castToNumber()->val;
    return py->createNumber(value);
  });

  addBuiltIn("exec",[](PyEngine *py, SharedPtr<ListObject> args) {
    auto bytecode = args->getStringAt(0);
    auto globals  = args->getDictAt(1);
    return py->runFrame(bytecode, {}, globals);
  });

  addBuiltIn("exists",[](PyEngine *py, SharedPtr<ListObject> args) {
    auto filename = args->getStringAt(0);
    struct stat stbuf;
    auto ret = !stat(filename.c_str(), &stbuf);
    return py->createNumber(ret);
  });
  
  addBuiltIn("mtime",[](PyEngine *py, SharedPtr<ListObject> args) {
    auto filename = args->getStringAt(0);
    struct stat stbuf;
    if (stat(filename.c_str(), &stbuf))
      raiseException("(py_mtime) IOError: ?");

    return py->createNumber(stbuf.st_mtime);
  });

  addBuiltIn("number", pyCreateFloat);

  addBuiltIn("round",[pyCreateFloat](PyEngine *py, SharedPtr<ListObject> args)
  {
    auto num = pyCreateFloat(py,args)->castToNumber()->val;
    double av = fabs(num);
    double iv = (long)av;
    av = (av - iv < 0.5 ? iv : iv + 1);
    return py->createNumber(num < 0 ? -av : av);
  });

  addBuiltIn("ord",[](PyEngine *py, SharedPtr<ListObject> args) {
    auto s = args->getStringAt(0);
    if (s.length() != 1) 
      raiseException("(py_ord) TypeError: ord() expected a character");

    return py->createNumber((unsigned char)s.c_str()[0]);
  });

  addBuiltIn("merge",[](PyEngine *py, SharedPtr<ListObject> args)
  {
    auto d1 = args->getDictAt(0);
    auto d2 = args->getDictAt(1);
    for (auto it2 : *d2)
      d1->map[it2.first]=it2.second;

    return NoneObject;
  });


  addBuiltIn("setmeta",[](PyEngine *py, SharedPtr<ListObject> args)
  {
    auto dict = args->getDictAt(0);
    auto meta = args->getDictAt(1);
    dict->meta = meta;
    return NoneObject;
  });

  addBuiltIn("getmeta",[](PyEngine *py, SharedPtr<ListObject> args) {
    auto dict = args->getDictAt(0);
    return dict->meta;
  });

  addBuiltIn("bool", [](PyEngine *py, SharedPtr<ListObject> args) {
    auto obj = args->getAt(0);
    return py->createNumber(obj->toBool());
  });

  auto o = this->createDict( true);

  o->setAttr(this->createString("__call__"), this->createFunction([](PyEngine *py, SharedPtr<ListObject> args)
  {
    auto ret = py->createDict(true);

    if (args->size())
    {
      for (auto it : *args->getDictAt(0))
        ret->setAttr(it.first, it.second);
    }

    return ret;
  }));

  o->setAttr(this->createString("__new__" ), this->createFunction([](PyEngine *py, SharedPtr<ListObject> args)
  {
    auto klass = args->getDictAt(0); args->popFront();
    auto obj   = py->createDict(true);
    obj->meta = klass;

    if (auto init_function = obj->lookup(py->createString("__init__")))
      init_function->call(args);

    return obj;
  }));

  builtins->setAttr(createString("object"), o);
}

/////////////////////////////////////////////////////////////////
String PyEngine::compileFile(String sourcecode, String filename)
{
  return callFunctionInModule("tinypy", "compileFile", this->createList( { this->createString(sourcecode), this->createString(filename) }))->toString();
}

//////////////////////////////////////////////////////////////////////
String PyEngine::loadFile(String filename) {
  struct stat stbuf;
  stat(filename.c_str(), &stbuf);
  long len = stbuf.st_size;
  FILE * file = fopen(filename.c_str(), "rb");
  if (!file)
    raiseException("(py_load) IOError: ?");
  String content(len, 0);
  fread((char*)content.c_str(), 1, len, file);
  fclose(file);
  return content;
}


////////////////////////////////////////////////////////
SharedPtr<Object> PyEngine::runCode(String sourcecode, SharedPtr<DictObject> globals, String filename) 
{
  auto bytecode = compileFile(sourcecode, filename);
  return runFrame(bytecode, {}, globals);
}

///////////////////////////////////////////////////////////////////////////////////////////
SharedPtr<Object> PyEngine::runFrame(String bytecode, std::vector< SharedPtr<Object> > regs, SharedPtr<DictObject> globals)
{
  auto ret = NoneObject;

  auto frame=std::make_shared<Frame>();
  frame->globals = globals;
  frame->bytecode = bytecode;
  frame->cursor = 0;
  frame->jump_to = -1;
  frame->lineno = 0;
  frame->line = "";
  frame->name = "?";
  frame->filename = "?";
  frame->on_pop = [&](SharedPtr<Object> obj) {ret = obj; };

  int R = 0; for (auto it : regs)
    frame->regs[R++] = it;

  this->frames.push(frame);

  this->run();

  return ret;
}

/////////////////////////////////////////////////////////////////
void PyEngine::popFrame(SharedPtr<Object> return_value)
{
  auto frame = frames.top();

  frames.pop();

  if (auto fn = frame->on_pop)
    fn(return_value);
}

#include "math_module.h"

////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
  //NOTE: if you change tinypy.py run "python tinypy.py"
  //then you can try this by "tinypy.exe tests.py"

  auto py = new PyEngine(argc, argv);

  math_init(py);

  auto filename = argv[1];

  auto globals = py->createDict();
  globals->setAttr(py->createString("__name__"), py->createString("__main__"));

  auto sourcecode = py->loadFile(filename);
  py->runCode(sourcecode,globals,filename);

  delete py;

#ifdef  _DEBUG
  getchar();
#endif

  return 0;
}

