#ifndef _MATH_MODULE_H__
#define _MATH_MODULE_H__

#include "tinypy.h"


const double Exponential = 2.7182818284590452354;
const double PiGreco     = 3.14159265358979323846;

static SharedPtr<Object> math_frexp(PyEngine *py, SharedPtr<ListObject> args)
{
  double x = args->getNumberAt(0);
  int    y = 0;

  errno = 0;
  double result = frexp(x, &y);
  if (errno == EDOM || errno == ERANGE) {
    raiseException(StringUtils::format() << "frexp(x): x=" << x << ", ""out of range");
    return NoneObject;
  }

  auto ret = py->createList();
  ret->pushBack(py->createNumber(result));
  ret->pushBack(py->createNumber((double)y));
  return ret;
}


static SharedPtr<Object> math_log(PyEngine *py, SharedPtr<ListObject> args) {

  int nargs = args->size();

  double x = args->getNumberAt(0);
  double y = nargs >= 2 ? args->getNumberAt(1) : Exponential;


  errno = 0;
  double num = log10(x);
  if (errno == EDOM || errno == ERANGE) {
    raiseException(StringUtils::format() << "log(x, y): x=" << x << ",y=" << y << " out of range");
    return NoneObject;
  }
  errno = 0;
  double den = log10(y);
  if (errno == EDOM || errno == ERANGE) {
    raiseException(StringUtils::format() << "log(x, y): x=" << x << ",y=" << y << " out of range");
    return NoneObject;
  }
  double ret = num / den;
  return py->createNumber(ret);
}


static SharedPtr<Object> math_modf(PyEngine *py, SharedPtr<ListObject> args) {
  double x = args->getNumberAt(0);
  double y = 0.0;

  errno = 0;
  double result = modf(x, &y);
  if (errno == EDOM || errno == ERANGE) {
    raiseException(StringUtils::format() << __func__ << "(x): x=" << x << "out of range");
    return NoneObject;
  }

  auto ret = py->createList();
  ret->pushBack(py->createNumber(result));
  ret->pushBack(py->createNumber(y));
  return ret;
}


static SharedPtr<Object> math_fun1(PyEngine *py, SharedPtr<ListObject> args, String name, std::function<double(double)> fn) {
  double x = args->getNumberAt(0);
  errno = 0;
  double ret = fn(x);
  if (errno == EDOM || errno == ERANGE) {
    raiseException(StringUtils::format() << name << "(x): x=" << x << "out of range");
    return NoneObject;
  }
  return py->createNumber(ret);
}


static SharedPtr<Object> math_fun2(PyEngine *py, SharedPtr<ListObject> args, String name, std::function<double(double, double)> fn)
{
  double x = args->getNumberAt(0);
  double y = args->getNumberAt(1);
  errno = 0;
  double ret = fn(x, y);
  if (errno == EDOM || errno == ERANGE) {
    raiseException(StringUtils::format() << name << "(x, y): x=" << x << ",y=" << y << " out of range");
    return NoneObject;
  }
  return py->createNumber(ret);
}

static const double degToRad = 3.141592653589793238462643383 / 180.0;
static double degrees(double x) {
  return (x / degToRad);
}

static double radians(double x) {
  return (x * degToRad);
}

static SharedPtr<Object>   math_pi;
static SharedPtr<Object>   math_e;

void math_init(PyEngine *py)
{
  auto math_mod = py->createDict();

  math_pi = py->createNumber(PiGreco);
  math_e = py->createNumber(Exponential);

  math_mod->setAttr(py->createString("pi"), math_pi);
  math_mod->setAttr(py->createString("e"), math_e);
  math_mod->setAttr(py->createString("acos"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "acos", [](double x) {return acos(x); }); }));
  math_mod->setAttr(py->createString("asin"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "asin", [](double x) {return asin(x); }); }));
  math_mod->setAttr(py->createString("atan"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "atan", [](double x) {return atan(x); }); }));
  math_mod->setAttr(py->createString("atan2"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun2(py, args, "atan2", [](double x, double y) {return atan2(x, y); }); }));
  math_mod->setAttr(py->createString("ceil"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "ceil", [](double x) {return ceil(x); }); }));
  math_mod->setAttr(py->createString("cos"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "cos", [](double x) {return cos(x); }); }));
  math_mod->setAttr(py->createString("cosh"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "cosh", [](double x) {return cosh(x); }); }));
  math_mod->setAttr(py->createString("degrees"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "degrees", [](double x) {return degrees(x); }); }));
  math_mod->setAttr(py->createString("exp"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "exp", [](double x) {return exp(x); }); }));
  math_mod->setAttr(py->createString("fabs"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "fabs", [](double x) {return fabs(x); }); }));
  math_mod->setAttr(py->createString("floor"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "floor", [](double x) {return floor(x); }); }));
  math_mod->setAttr(py->createString("fmod"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun2(py, args, "fmod", [](double x, double y) {return fmod(x, y); }); }));
  math_mod->setAttr(py->createString("frexp"), py->createFunction(math_frexp));
  math_mod->setAttr(py->createString("hypot"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun2(py, args, "hypot", [](double x, double y) {return hypot(x, y); }); }));
  math_mod->setAttr(py->createString("ldexp"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun2(py, args, "ldexp", [](double x, double y) {return ldexp(x, y); }); }));
  math_mod->setAttr(py->createString("log"), py->createFunction(math_log));
  math_mod->setAttr(py->createString("log10"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "log10", [](double x) {return log10(x); }); }));
  math_mod->setAttr(py->createString("modf"), py->createFunction(math_modf));
  math_mod->setAttr(py->createString("pow"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun2(py, args, "pow", [](double x, double y) {return pow(x, y); }); }));
  math_mod->setAttr(py->createString("radians"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "radians", [](double x) {return radians(x); }); }));
  math_mod->setAttr(py->createString("sin"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "sin", [](double x) {return sin(x); }); }));
  math_mod->setAttr(py->createString("sinh"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "sinh", [](double x) {return sinh(x); }); }));
  math_mod->setAttr(py->createString("sqrt"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "sqrt", [](double x) {return sqrt(x); }); }));
  math_mod->setAttr(py->createString("tan"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "tan", [](double x) {return tan(x); }); }));
  math_mod->setAttr(py->createString("tanh"), py->createFunction([](PyEngine* py, SharedPtr<ListObject> args) {return math_fun1(py, args, "tanh", [](double x) {return tanh(x); }); }));

  math_mod->setAttr(py->createString("__doc__"), py->createString(
    "This module is always available.  It provides access to the\n"
    "mathematical functions defined by the regs[C] standard."));

  math_mod->setAttr(py->createString("__name__"), py->createString("math"));
  math_mod->setAttr(py->createString("__file__"), py->createString(__FILE__));
  py->addModule("math", math_mod);
}


#endif //_MATH_MODULE_H__

