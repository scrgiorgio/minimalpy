
#ifndef TINYPY_H__
#define TINYPY_H__

#include <string>
#include <algorithm>
#include <vector>
#include <functional>
#include <map>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <stack>

typedef std::string String;

template <typename T> using SharedPtr= std::shared_ptr<T>;

class Object;
class NumberObject;
class StringObject;
class ListObject;
class DictObject;
class FunctionObject;
class PyEngine;

extern SharedPtr<Object> NoneObject;


//raiseException
inline void raiseException(String s) {
  throw s;
}

//raiseException
inline void raiseException(SharedPtr<Object> ex) {
  throw ex;
}

//////////////////////////////////////////////////////////////////
class StringUtils
{
public:

  //________________________________________________________
  class format
  {
  public:

    //constructor
    format() {
    }

    //constructor
    format(const format& other) {
      out << other.str();
    }

    //operator=
    format& operator=(const format& other) {
      out.clear(); out << other.str(); return *this;
    }

    //String()
    operator String() const {
      return out.str();
    }

    //str
    String str() const {
      return out.str();
    }

    //operator<<
    template <typename Type>
    format& operator<<(Type value) {
      out << value; return *this;
    }


  private:

    std::ostringstream out;

  };


  //substring
  static String substring(String s, int a, int b)
  {
    int len = (int)s.length();
    a = std::max( 0, (a < 0 ? len + a : a));
    b = std::min(len, (b < 0 ? len + b : b));
    return s.substr(a, b - a);
  }

  //replaceAll
  static String replaceAll(String str, const String& from, const String& to)
  {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != String::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }
    return str;
  };

  static String ltrim(String ret, String spaces = " \t\ret\n") {
    int i = (int)ret.find_first_not_of(spaces.c_str()); return i >= 0 ? ret.erase(0, i) : "";
  };

  static String rtrim(String ret, String spaces = " \t\ret\n") {
    int i = (int)ret.find_last_not_of(spaces.c_str()); return i >= 0 ? ret.erase(i + 1) : "";
  };

  static String trim(String s) {
    return ltrim(rtrim(s));
  }

  //split
  static std::vector<String> split(String source, String separator)
  {
    std::vector<String> ret;
    int m = (int)separator.size();
    for (int j; (j = (int)source.find(separator)) >= 0; source = source.substr(j + m))
      ret.push_back(source.substr(0, j));

    ret.push_back(source);
    return ret;
  };


  //join
  static String join(std::vector<String> v, String separator, String prefix="", String suffix = "")
  {
    int N = (int)v.size();
    std::ostringstream out;
    out << prefix;
    for (int I = 0; I<N; I++)
    {
      if (I) out << separator;
      out << v[I];
    }
    out << suffix;
    return out.str();
  }

};

//////////////////////////////////////////////////////////////
class Object : public std::enable_shared_from_this<Object>
{
public:

  enum
  {
    NoneType,
    NumberType,
    StringType,
    DictType,
    ListType,
    FunctionType
  };

  PyEngine* py;

  int type=NoneType;

  //todo: optimize!

  //constructor
  Object(PyEngine* py_, int type_) : py(py_), type(type_) {
  }

  //destructor
  virtual ~Object() {
  }

  //self
  SharedPtr<Object> self() const {
    return const_cast<Object*>(this)->shared_from_this();
  }

  //clone
  virtual SharedPtr<Object> clone() {
    assert(type == NoneType);
    return self();
  }

  //computeHash
  virtual int computeHash() {
    return (type==NoneType)? 0 : computeHash((void*)this, sizeof(void*));
  }

  //compare
  static int compare(SharedPtr<Object> a, SharedPtr<Object> b);

  //toString
  virtual String toString()  const
  {
    assert(type == NoneType);
    return "None";
  }

  //toBool
  virtual bool toBool() const {
    assert(type == NoneType);
    return false;
  }

  //len
  virtual int len() const {
    raiseException("(pyLen) TypeError: len() of unsized object");
    return 0;
  }

public:

  //hasAttr
  virtual bool hasAttr(SharedPtr<Object> k) {
    raiseException("(pyHas) TypeError: iterable argument required");
    return false;
  }

  //getAttr
  virtual SharedPtr<Object> getAttr(SharedPtr<Object> k) {
    raiseException("(pyGet) TypeError: ?");
    return NoneObject;
  }

  //setAttr
  virtual void setAttr(SharedPtr<Object> k, SharedPtr<Object> v) {
    raiseException("(pySet) TypeError: object does not support item assignment");
    return;
  }

  //delAttr
  virtual void delAttr(SharedPtr<Object> k) {
    raiseException("(pyDelete) TypeError: object does not support item deletion");
  }

  //call
  virtual SharedPtr<Object> call(SharedPtr<ListObject> args) {
    raiseException("(pyCall) TypeError: object is not callable" + this->toString());
    return NoneObject;
  }

public:

  //castToNumber
  SharedPtr<NumberObject> castToNumber()const {
    return std::dynamic_pointer_cast<NumberObject>(self());
  }

  //castToString
  SharedPtr<StringObject> castToString() const{
    return std::dynamic_pointer_cast<StringObject>(self());
  }

  //castToList
  SharedPtr<ListObject> castToList() const {
    return std::dynamic_pointer_cast<ListObject>(self());
  }

  //castToDict
  SharedPtr<DictObject> castToDict() const {
    return std::dynamic_pointer_cast<DictObject>(self());
  }

  //castToFunction
  SharedPtr<FunctionObject> castToFunction() const {
    return std::dynamic_pointer_cast<FunctionObject>(self());
  }

protected:

  //computeHash
  static int computeHash(void* v, int l) {
    int ret = l + (l >= 4 ? *(int*)v : 0);
    for (int i = l, step = (l >> 5) + 1; i >= step; i -= step)
      ret = ret ^ ((ret << 5) + (ret >> 2) + ((unsigned char *)v)[i - 1]);
    return ret;
  }

private:

  //non copyable
  Object(const Object&) = delete;
  const Object& operator=(const Object&) = delete;

};


typedef std::function<SharedPtr<Object>(PyEngine*, SharedPtr<ListObject>)> Function;

///////////////////////////////////////////////////////////
class NumberObject : public Object
{
public:

  double val;

  //constructor
  NumberObject(PyEngine* py,double val_) : Object(py,NumberType), val(val_) {
  }

  //destructor
  virtual ~NumberObject() {
  }


  //toString
  virtual String toString()  const override {
    return StringUtils::format()<<val;
  }

  //toBool
  virtual bool toBool() const override{
    return val!=0;
  }

  //clone
  virtual SharedPtr<Object> clone() override {
    return std::make_shared<NumberObject>(py, val);
  }

  //computeHash
  virtual int computeHash() override {
    return Object::computeHash((void*)&val, sizeof(double));
  }

};

///////////////////////////////////////////////////////////
class StringObject : public Object
{
public:

  String val;

  //constructor
  StringObject(PyEngine* py,String val_ = "") : Object(py,StringType), val(val_) {
  }

  //destructor
  virtual ~StringObject() {
  }

  //toString
  virtual String toString()  const override {
    return val;
  }

  //toBool
  virtual bool toBool() const override {
    return !val.empty();
  }

  //len
  virtual int len() const  override{
    return (int)val.length();
  }

  //hasAttr
  virtual bool hasAttr(SharedPtr<Object> k) override {

    if (k->type == Object::StringType) 
      return val.find(k->castToString()->val) != String::npos;
    else
      return false;
  }

  //getAttr
  virtual SharedPtr<Object> getAttr(SharedPtr<Object> k) override;

  //clone
  virtual SharedPtr<Object> clone() override {
    return std::make_shared<StringObject>(py, val);
  }

  //computeHash
  virtual int computeHash() override {
    return Object::computeHash((void*)val.c_str(), (int)val.length());
  }

};

///////////////////////////////////////////////////////////
class ListObject : public Object
{
public:

  typedef std::vector<SharedPtr<Object>> Vector;
  typedef typename Vector::iterator             iterator;
  typedef typename Vector::const_iterator const_iterator;

  Vector items;

  //constructor
  ListObject(PyEngine* py,Vector items_= Vector()) : Object(py,ListType),items(items_) {
  }

  //destructor
  virtual ~ListObject() {
  }

  //toString
  virtual String toString()  const override {
    std::ostringstream out;
    out << "[";
    int I=0;for (auto it : *this)
      out << (I++ ? "," : "") << it->toString();
    out << "]";
    return out.str();
  }

  //toBool
  virtual bool toBool() const override {
    return size()>0;
  }

  //len
  virtual int len() const  override {
    return size();
  }

  //hasAttr
  virtual bool hasAttr(SharedPtr<Object> k) override {
    return find(k) != -1;
  }

  //getAttr
  virtual SharedPtr<Object> getAttr(SharedPtr<Object> k) override;

  //setAttr
  virtual void setAttr(SharedPtr<Object> k, SharedPtr<Object> v) override;

  //clone
  virtual SharedPtr<Object> clone() override {
    return std::make_shared<ListObject>(py, items);
  }

  //computeHash
  virtual int computeHash() override {
    int ret = 0;
    for (auto it : *this)
        ret += it->computeHash();
    return ret;
  }

  //append
  void append(ListObject& other) {
    for (auto it : other)
      pushBack(it);
  }

  //find
  int find(SharedPtr<Object> what) {
    int n = 0; 
    for (auto it : *this)
    {
      if (Object::compare(what, it) == 0)
        return n;
      n++;
    }
    return -1;
  }

  //size
  int size() const {
    return (int)items.size();
  }

  //clear
  void clear() {
    this->items.clear();
  }

  //begin
  const_iterator begin() const {
    return items.begin();
  }

  //end
  const_iterator end() const {
    return items.end();
  }

  //begin
  iterator begin() {
    return items.begin();
  }

  //end
  iterator end() {
    return items.end();
  }

  //getAt
  SharedPtr<Object> getAt(int index) const {

    if (index < 0 || index >= size())
      raiseException("out of index");
    return items[index];
  }

  //getNumberAt
  double getNumberAt(int index) const {
    auto ret = this->getAt(index);
    if (ret->type != NumberType)
      raiseException("(popWithType) TypeError: unexpected type");
    return ret->castToNumber()->val;
  }

  //getStringAt
  String getStringAt(int index) const {
    auto ret = this->getAt(index);
    if (ret->type != StringType)
      raiseException("(popWithType) TypeError: unexpected type");
    return ret->castToString()->val;
  }

  //getListAt
  SharedPtr<ListObject> getListAt(int index) const {
    auto ret = this->getAt(index);
    if (ret->type != ListType)
      raiseException("(popWithType) TypeError: unexpected type");
    return ret->castToList();
  }

  //getDictAt
  SharedPtr<DictObject> getDictAt(int index) const {
    auto ret = this->getAt(index);
    if (ret->type != DictType)
      raiseException("(popWithType) TypeError: unexpected type");
    return ret->castToDict();
  }

  //popFunction
  SharedPtr<FunctionObject> getFunctionAt(int index) const {
    auto ret = this->getAt(index);
    if (ret->type != FunctionType)
      raiseException("(popWithType) TypeError: unexpected type");
    return ret->castToFunction();
  }

  //setAt
  void setAt(int index, SharedPtr<Object> value) {
    items[index] = value;
  }

  //eraseAt
  void eraseAt(iterator it) {
    items.erase(it);
  }

  //insertAt
  void insertAt(int index, SharedPtr<Object> value)
  {
    if (index >= this->size())
      items.push_back(value);
    else
      items.insert(items.begin() + index, value);
  }

  //pushBack
  void pushBack(SharedPtr<Object> value) {
    items.push_back(value);
  }


  //pushFront
  void pushFront(SharedPtr<Object> value) {
    insertAt(0,value);
  }

  //sort
  void sort() {
    std::sort(items.begin(), items.end(), [](const SharedPtr<Object>& a, const SharedPtr<Object>& b) {
      return Object::compare(a, b);
    });
  }

  //popAt
  SharedPtr<Object> popAt(int n) {
    auto ret = getAt(n);
    eraseAt(begin() + n);
    return ret;
  }

  //popBack
  SharedPtr<Object> popBack() {
    return popAt(size() - 1);
  }

  //popFront
  SharedPtr<Object> popFront() {
    return popAt(0);
  }

  //print
  void print() const
  {
    int n = 0;  for (auto it : *const_cast<ListObject*>(this)) {
      std::cout << (n ? " " : "") << it->toString();
      n++;
    }
    std::cout << std::endl;
  }

};


///////////////////////////////////////////////////////////
class DictObject : public Object
{
public:

  struct Less
  {
    bool operator() (const SharedPtr<Object>& lhs, const SharedPtr<Object>& rhs) const {
      return lhs->computeHash() < rhs->computeHash();
    }
  };

  typedef std::map<SharedPtr<Object>, SharedPtr<Object>, Less> Map;
  typedef typename Map::iterator iterator;

  Map map;

  SharedPtr<DictObject> meta;
  
  //constructor
  DictObject(PyEngine* py, bool bObject_ = false,Map map_=Map(),SharedPtr<DictObject> meta_= SharedPtr<DictObject>())
    : Object(py,DictType),bObject(bObject_),map(map_),meta(meta_) {
  }

  //destructor
  virtual ~DictObject() {
  }

  //toString
  virtual String toString()  const override {
    return StringUtils::format() << (bObject? "<object" : "<dict")<<" 0x" << std::hex << (size_t)this << ">";
  }

  //toBool
  virtual bool toBool() const override {
    return size()>0;
  }

  //len
  virtual int len() const  override {
    return size();
  }

  //hasAttr
  virtual bool hasAttr(SharedPtr<Object> k) override {
    return map.find(k) != end();
  }

  //getAttr
  virtual SharedPtr<Object> getAttr(SharedPtr<Object> k) override;

  //setAttr
  virtual void setAttr(SharedPtr<Object> k, SharedPtr<Object> v) override;

  //delAttr
  virtual void delAttr(SharedPtr<Object> k) override {
    auto it = map.find(k);
    if (it == map.end())
      raiseException(StringUtils::format() << "(py_dict_del) KeyError: " << k->toString());
    map.erase(it);
  }

  //lookup
  SharedPtr<Object> lookup(SharedPtr<Object> k, int depth = 8);

  //clone
  virtual SharedPtr<Object> clone() override {
    return std::make_shared<DictObject>(py, bObject,map,meta? std::dynamic_pointer_cast<DictObject>(meta->clone()) : SharedPtr<DictObject>());
  }

  //isDict
  bool isDict() const {
    return !bObject;
  }

  //isObject
  bool isObject() const {
    return bObject;
  }

  //call
  virtual SharedPtr<Object> call(SharedPtr<ListObject> args) override;


  //begin
  iterator begin() {
    return map.begin();
  }

  //end
  iterator end() {
    return map.end();
  }

  //empty
  bool empty() const {
    return map.empty();
  }

  //size
  int size() const {
    return (int)map.size();
  }

private:

  bool bObject = false;

  //computeHash
  static size_t computeHash(SharedPtr<Object> value);

};

///////////////////////////////////////////////////////////
class FunctionObject : public Object
{
public:
  SharedPtr<Object>     instance;
  SharedPtr<DictObject> globals;
  String                bytecode;
  Function              function;

  //constructor
  FunctionObject(PyEngine* py) : Object(py,FunctionType) {
  }

  //destructor
  virtual ~FunctionObject() {
  }

  //toString
  virtual String toString()  const {
    return StringUtils::format() << "<function 0x" << std::hex << (size_t)this << ">";
  }

  //toBool
  virtual bool toBool() const override {
    return true;
  }

  //call
  virtual SharedPtr<Object> call(SharedPtr<ListObject> args) override;

  //clone
  virtual SharedPtr<Object> clone() override {
    auto ret = std::make_shared<FunctionObject>(py);
    ret->instance = instance;
    ret->globals  = globals;
    ret->bytecode = bytecode;
    ret->function = function;
    return ret;
  }

  //computeHash
  virtual int computeHash() override {
    return Object::computeHash((void*)this,sizeof(void*));
  }

};

////////////////////////////////////////////////////////////////
class PyEngine 
{
public:

  //constructor
  PyEngine(int argc, char *argv[]);

  //destructor
  ~PyEngine();

  //addModule
  void addModule(String name, SharedPtr<DictObject> mod) {
    modules->setAttr(this->createString(name), mod);
  }

  //load
  String loadFile(String filename);

  //compileFile
  String compileFile(String sourcode, String filename);

  //callFunctionInModule
  SharedPtr<Object> callFunctionInModule(String modulename, String functionname, SharedPtr<ListObject> args) {
    auto module   = modules->getAttr(this->createString(modulename));
    auto function =  module->getAttr(this->createString(functionname));
    return function->call(args);
  }

  //runCode
  SharedPtr<Object> runCode(String sourcecode, SharedPtr<DictObject> globals, String filename = "<eval>");

public:

  //createNumber
  SharedPtr<NumberObject> createNumber(double val = 0) {
    return std::make_shared<NumberObject>(this, val);
  }

  //createString
  SharedPtr<StringObject> createString(String val = "") {
    return std::make_shared<StringObject>(this, val);
  }

  //createList
  SharedPtr<ListObject> createList(ListObject::Vector items = ListObject::Vector()) {
    return std::make_shared<ListObject>(this, items);
  }

  //createDict
  SharedPtr<DictObject> createDict(bool bObject = false, DictObject::Map map = DictObject::Map(), SharedPtr<DictObject> meta = SharedPtr<DictObject>()) {
    return std::make_shared<DictObject>(this, bObject, map, meta);
  }

  //createFunction
  SharedPtr<FunctionObject> createFunction(Function function, SharedPtr<Object> instance = SharedPtr<Object>(), String bytecode = "", SharedPtr<DictObject> globals = SharedPtr<DictObject>())
  {
    auto ret = std::make_shared<FunctionObject>(this);
    ret->bytecode = bytecode;
    ret->instance = instance;
    ret->globals  = globals;
    ret->function = function;
    return ret;
  }

  //createMethod
  SharedPtr<FunctionObject> createMethod(SharedPtr<Object> instance, Function function) {
    return createFunction(function, instance);
  }

private:

  friend class FunctionObject;

  //___________________________________________
  class Regs
  {
  public:

    std::vector< SharedPtr<SharedPtr<Object> > > v;

    //operator[]
    SharedPtr<Object>& operator[](int index) 
    {
      if (index >= v.size())
        v.resize(index + 1);
      if (!v[index])
        v[index] = std::make_shared<SharedPtr<Object>>();
      return *v[index];
    }
  };

  //__________________________________________
  class Frame
  {
  public:

    typedef std::function<void(SharedPtr<Object>)> OnPop;

    String                bytecode;
    int                   cursor = 0;
    int                   jump_to = 0;
    Regs                  regs;
    String                filename;
    String                name;
    String                line;
    SharedPtr<DictObject> globals;
    int                   lineno = 0;
    OnPop                 on_pop;
  };

  //A dictionary containing all builtin objects.
  SharedPtr<Object> builtins;

  //A dictionary with all loaded modules.
  SharedPtr<Object> modules;

  //A list of all call frames.
  std::stack< SharedPtr<Frame> > frames;

  //is_running
  int is_running = 0;

  //addBuiltIns
  void addBuiltIns();

  //pushFrame
  SharedPtr<Object> runFrame(String bytecode, std::vector< SharedPtr<Object> > regs, SharedPtr<DictObject> globals);

  //popFrame
  void popFrame(SharedPtr<Object> obj);

  //run
  void run();

  //runStep
  int runStep();

};


#endif //TINYPY_H__
