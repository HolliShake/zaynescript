#include "../function.h"
#include "../hashmap.h"
#include "./math.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

// Helper macro for single argument math functions
#define MATH_FUNC_1(name, func) \
static Value* _Math##name(Interpreter* interpreter, int argc, Value** arguments) { \
    if (argc != 1 || !ValueIsNum(arguments[0])) return interpreter->Null; \
    double val = CoerceToNum(arguments[0]); \
    return NewNumValue(interpreter, func(val)); \
}

// Helper macro for two argument math functions
#define MATH_FUNC_2(name, func) \
static Value* _Math##name(Interpreter* interpreter, int argc, Value** arguments) { \
    if (argc != 2 || !ValueIsNum(arguments[0]) || !ValueIsNum(arguments[1])) return interpreter->Null; \
    double val1 = CoerceToNum(arguments[0]); \
    double val2 = CoerceToNum(arguments[1]); \
    return NewNumValue(interpreter, func(val1, val2)); \
}

MATH_FUNC_1(Abs, fabs)
MATH_FUNC_1(Acos, acos)
MATH_FUNC_1(Asin, asin)
MATH_FUNC_1(Atan, atan)
MATH_FUNC_1(Ceil, ceil)
MATH_FUNC_1(Cos, cos)
MATH_FUNC_1(Cosh, cosh)
MATH_FUNC_1(Exp, exp)
MATH_FUNC_1(Floor, floor)
MATH_FUNC_1(Log, log)
MATH_FUNC_1(Log10, log10)
MATH_FUNC_1(Round, round)
MATH_FUNC_1(Sin, sin)
MATH_FUNC_1(Sinh, sinh)
MATH_FUNC_1(Sqrt, sqrt)
MATH_FUNC_1(Tan, tan)
MATH_FUNC_1(Tanh, tanh)

MATH_FUNC_2(Atan2, atan2)
MATH_FUNC_2(Pow, pow)
MATH_FUNC_2(Max, fmax)
MATH_FUNC_2(Min, fmin)
MATH_FUNC_2(Hypot, hypot)

static ModuleFunction _MathModuleFunctions[] = {
    { .Name = "abs",   .Argc = 1, .CFunction = (NativeFunction) _MathAbs,   .Value = NULL },
    { .Name = "acos",  .Argc = 1, .CFunction = (NativeFunction) _MathAcos,  .Value = NULL },
    { .Name = "asin",  .Argc = 1, .CFunction = (NativeFunction) _MathAsin,  .Value = NULL },
    { .Name = "atan",  .Argc = 1, .CFunction = (NativeFunction) _MathAtan,  .Value = NULL },
    { .Name = "atan2", .Argc = 2, .CFunction = (NativeFunction) _MathAtan2, .Value = NULL },
    { .Name = "ceil",  .Argc = 1, .CFunction = (NativeFunction) _MathCeil,  .Value = NULL },
    { .Name = "cos",   .Argc = 1, .CFunction = (NativeFunction) _MathCos,   .Value = NULL },
    { .Name = "cosh",  .Argc = 1, .CFunction = (NativeFunction) _MathCosh,  .Value = NULL },
    { .Name = "exp",   .Argc = 1, .CFunction = (NativeFunction) _MathExp,   .Value = NULL },
    { .Name = "floor", .Argc = 1, .CFunction = (NativeFunction) _MathFloor, .Value = NULL },
    { .Name = "hypot", .Argc = 2, .CFunction = (NativeFunction) _MathHypot, .Value = NULL },
    { .Name = "log",   .Argc = 1, .CFunction = (NativeFunction) _MathLog,   .Value = NULL },
    { .Name = "log10", .Argc = 1, .CFunction = (NativeFunction) _MathLog10, .Value = NULL },
    { .Name = "max",   .Argc = 2, .CFunction = (NativeFunction) _MathMax,   .Value = NULL },
    { .Name = "min",   .Argc = 2, .CFunction = (NativeFunction) _MathMin,   .Value = NULL },
    { .Name = "pow",   .Argc = 2, .CFunction = (NativeFunction) _MathPow,   .Value = NULL },
    { .Name = "round", .Argc = 1, .CFunction = (NativeFunction) _MathRound, .Value = NULL },
    { .Name = "sin",   .Argc = 1, .CFunction = (NativeFunction) _MathSin,   .Value = NULL },
    { .Name = "sinh",  .Argc = 1, .CFunction = (NativeFunction) _MathSinh,  .Value = NULL },
    { .Name = "sqrt",  .Argc = 1, .CFunction = (NativeFunction) _MathSqrt,  .Value = NULL },
    { .Name = "tan",   .Argc = 1, .CFunction = (NativeFunction) _MathTan,   .Value = NULL },
    { .Name = "tanh",  .Argc = 1, .CFunction = (NativeFunction) _MathTanh,  .Value = NULL },
    { .Name = NULL }
};

Value* LoadCoreMath(Interpreter* interpreter) {
    Value* module = NewObjectValue(interpreter);
    HashMap* map  = CoerceToHashMap(module);

    for (int i = 0; _MathModuleFunctions[i].Name != NULL; i++) {
        ModuleFunction func = _MathModuleFunctions[i];
        String name = AllocateString((String)func.Name);
        
        if (func.Value != NULL) {
            HashMapSet(map, name, func.Value);
        } else {
            HashMapSet(map, name, NewNativeFunctionValue(interpreter, 
                CreateNativeFunctionMeta(
                    (const String) name,
                    func.Argc,
                    func.CFunction
                )
            ));
        }
    }
    
    // Constants
    HashMapSet(map, AllocateString("pi"), NewNumValue(interpreter, M_PI));
    HashMapSet(map, AllocateString("e"), NewNumValue(interpreter, M_E));

    return module;
}

