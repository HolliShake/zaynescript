#include "./operation.h"

#define PushArray(type, array, count, val, defaultValue) do { \
    (array)[(count)++] = val; \
    (array) = Reallocate((array), sizeof(type) * ((count) + 1)); \
    (array)[(count)] = (defaultValue); \
} while(0)

#define GetOffset() (interp->ConstantC)

#define _Push(value) (interp->Stacks[interp->StackC++] = (value))
#define _Popp()      (interp->Stacks[--interp->StackC])
#define _PopN(n)     (interp->Stacks[interp->StackC -= (n)])
#define _Peek()      (interp->Stacks[interp->StackC - 1])


static int _GetConstantOffset(Interpreter* interp, Value* value) {
    if (value == NULL) {
        goto BAD;
    }
    String valueStr = ValueToString(value), constantStr = NULL;
    for (int i = 0; i < interp->ConstantC; i++) {
        if (interp->Constants[i] != NULL) {
            constantStr = ValueToString(interp->Constants[i]);
            if (constantStr != NULL && strcmp(constantStr, valueStr) == 0) {
                free(constantStr);
                free(valueStr);
                return i;
            }
            if (constantStr != NULL) {
                free(constantStr);
            }
        }
    }
    free(valueStr);
    BAD:;
    return -1;
}

static void _DupTop(Interpreter* interp) {
    _Push(_Peek());
}

extern void Run(Interpreter* interp, Value* fnValue, Value* rootEnvObj, Value* envObj);

extern CoreMapper _CoreModuleMappers[];

bool IsMethodOfObject(Interpreter* interp, Value* obj, Value* method) {
    String key = ValueToString(method);
    if (ValueIsArray(obj)) {
        // Handle array methods or attributes
        Array* array = CoerceToArray(obj);

        // Check prototype chain
        Class* cls = CoerceToUserClass(interp->Array);

        while (cls != NULL) {
            if (ClassHasMember(cls, key, false, true)) {
                free(key);
                return true;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }
    } else if (ValueIsObject(obj)) {
        // Handle object methods or attributes
        HashMap* map = CoerceToHashMap(obj);

        // Check prototype chain
        Class* cls = NULL;

        while (cls != NULL) {
            if (ClassHasMember(cls, key, false, true)) {
                free(key);
                return true;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }
    } else if (ValueIsClass(obj)) {
        // Handle Class static functions or attributes
        Class* cls = CoerceToUserClass(obj);

        while (cls != NULL) {
            if (ClassHasMember(cls, key, false, true)) {
                free(key);
                return true;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }
    } else if (ValueIsClassInstance(obj)) {
        // Handle class instance methods or attributes
        ClassInstance* instance = CoerceToClassInstance(obj);

        // Check prototype chain
        Class* cls = CoerceToUserClass(instance->Proto);

        while (cls != NULL) {
            if (ClassHasMember(cls, key, false, true)) {
                free(key);
                return true;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }

    }

    free(key);
    return false;
}

Value* GenericGetAttribute(Interpreter* interp, Value* obj, Value* index, bool forMethodCall) {
    String key = ValueToString(index);
    if (ValueIsArray(obj)) {
        // Handle array methods or attributes
        Array* array = CoerceToArray(obj);

        if (ValueIsNum(index)) {
            long idx = (long) CoerceToI64(index);
            if (idx < 0 || idx >= array->Count) {
                free(key);
                String errMsg = FormatString("array index %ld out of bounds", idx);
                Value* errVal = NewErrorValue(interp, errMsg);
                free(errMsg);
                return errVal;
            }
            free(key);
            return array->Items[idx];
        }

        // Check prototype chain
        Class* cls = CoerceToUserClass(interp->Array);

        while (forMethodCall && cls != NULL) {
            if (ClassHasMember(cls, key, false, forMethodCall)) {
                Value* member = ClassGetMember(cls, key, false);
                free(key);
                return member;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }

    } else if (ValueIsObject(obj)) {
        // Handle object methods or attributes
        HashMap* map = CoerceToHashMap(obj);

        if (HashMapContains(map, key)) {
            Value* val = HashMapGet(map, key);
            free(key);
            return val;
        }

        // Check prototype chain
        Class* cls = NULL;

        while (forMethodCall && cls != NULL) {
            if (ClassHasMember(cls, key, false, forMethodCall)) {
                Value* member = ClassGetMember(cls, key, false);
                free(key);
                return member;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }

    } else if (ValueIsClass(obj)) {
        // Handle Class static functions or attributes
        Class* cls = CoerceToUserClass(obj);

        while (cls != NULL) {
            if (ClassHasMember(cls, key, true, forMethodCall)) {
                Value* member = ClassGetMember(cls, key, true);
                free(key);
                return member;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }
    } else if (ValueIsClassInstance(obj)) {
        // Handle class instance methods or attributes
        ClassInstance* instance = CoerceToClassInstance(obj);

        // Check prototype chain
        Class* cls = CoerceToUserClass(instance->Proto);

        while (forMethodCall && cls != NULL) {
            if (ClassHasMember(cls, key, !forMethodCall, forMethodCall)) {
                Value* member = ClassGetMember(cls, key, !forMethodCall);
                free(key);
                return member;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }

        // Check instance members
        Value* member = HashMapGet(instance->Members, key);

        if (member != NULL) {
            free(key);
            return member;
        }
    }
    free(key);
    return interp->Null;
}

Value* DoImportCore(Interpreter* interp, String moduleName) {
    Value* result = LoadCoreModule(interp, moduleName);
    if (result == NULL) {
        String errMsg = FormatString("core module '%s' not found", moduleName);
        Value* errVal = NewErrorValue(interp, errMsg);
        free(errMsg);
        return errVal;
    }
    return result;
}

Value* DoSetIndex(Interpreter* interp, Value* obj, Value* index, Value* val) {
    if (ValueIsArray(obj)) {
        Array* array = CoerceToArray(obj);
        long idx = (long) CoerceToI64(index);
        if (idx < 0 || idx >= array->Count) {
            String errMsg = FormatString("array index %ld out of bounds", idx);
            Value* errVal = NewErrorValue(interp, errMsg);
            free(errMsg);
            return errVal;
        }
        array->Items[idx] = val;
    } else if (ValueIsObject(obj)) {
        HashMap* map = CoerceToHashMap(obj);
        HashMapSet(map, ValueToString(index), val);
    } else if (ValueIsClassInstance(obj)) {
        ClassInstance* instance = CoerceToClassInstance(obj);
        HashMapSet(instance->Members, ValueToString(index), val);
    } else if (ValueIsClass(obj)) {
        Class* cls = CoerceToUserClass(obj);
        HashMapSet(cls->StaticMembers, ValueToString(index), val);
    } else {
        return NewErrorValue(interp, "invalid operation: cannot set index on non-object");
    }
    return interp->Null;
}

Value* DoGetIndex(Interpreter* interp, Value* obj, Value* index) {
    return GenericGetAttribute(interp, obj, index, false);
}

Value* DoCallCtor(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* clsValue, int argc) {
    if (clsValue == NULL) Panic("Attempted to call constructor on a null value\n");

    if (!ValueIsClass(clsValue)) {
        _PopN(argc);
        return NewErrorValue(interp, "invalid operation: attempted to call constructor on non-class value");
    }

    Class* cls = CoerceToUserClass(clsValue);

    if (!ClassHasMember(cls, CONSTRUCTOR_NAME, false, true)) {
        if (argc != 0) {
            _PopN(argc);
            String errMsg = FormatString("argument count mismatch: expected 0 arguments but got %d", argc);
            Value* errVal = NewErrorValue(interp, errMsg);
            free(errMsg);
            return errVal;
        }
        // Push default instance, no constructor call
        ClassInstance* instance = CreateClassInstance(clsValue);
        _Push(NewClassInstanceValue(interp, instance));
        return interp->Null;
    }

    // Push thisArg
    Value* instanceValue = NewClassInstanceValue(interp, CreateClassInstance(clsValue));
    _Push(instanceValue);

    Value* constructor = ClassGetMember(cls, CONSTRUCTOR_NAME, false);

    Value* result = DoCall(
        interp, 
        rootEnvObj, 
        envObj, 
        constructor, 
        ++argc
    );
    
    if (ValueIsNull(result)) {
        _Popp(); // Pop constructor return value
        _Push(instanceValue); // Push instance as return value
    }

    return result;
}

Value* DoCall(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* fn, int argc) {
    if (fn == NULL) Panic("Attempted to call a null value\n");

    if (!ValueIsCallable(fn)) {
        _PopN(argc);
        return NewErrorValue(interp, "invalid operation: attempted to call a non-callable value");
    }

    if (ValueIsNativeFunction(fn)) {
        NativeFunctionMeta* nFMeta = CoerceToNativeFunctionMeta(fn);
        NativeFunction nativeFunc  = nFMeta->FuncPtr;

        if (nFMeta->Argc != VARARG && argc != nFMeta->Argc) {
            _PopN(argc); 
            String errMsg = FormatString("argument count mismatch: expected %d arguments but got %d",  nFMeta->Argc, argc);
            Value* errVal = NewErrorValue(interp, errMsg);
            free(errMsg);
            return errVal;
        }

        Value** args = Allocate(sizeof(Value*) * argc);

        for (int i = 0; i < argc; i++) {
            args[i] = _Popp();
        }

        Value* res = nativeFunc(interp, argc, args);

        _Push(res);
        free(args);
        
        return ValueIsError(res) ? res : interp->Null;
    }

    if (!ValueIsCallable(fn)) {
        Panic("Attempted to call a non-callable value: %s\n", ValueToString(fn));
    }

    // Call
    UserFunction* uf = CoerceToUserFunction(fn);

    if (argc != uf->Argc) {
        _PopN(argc);
        String errMsg = FormatString("argument count mismatch: expected %d arguments but got %d",  uf->Argc, argc);
        Value* errVal = NewErrorValue(interp, errMsg);
        free(errMsg);
        return errVal;
    }

    if (uf->ParentEnv == NULL) {
        Panic("User function '%s' has null ParentEnv\n", uf->Name != NULL ? uf->Name : "<anonymous>");
    }
    
    Run(
        interp, 
        fn, 
        rootEnvObj, 
        NewEnvironmentValue(interp, CreateEnvironment(uf->ParentEnv, uf->LocalC))
    );
    return interp->Null;
}

Value* DoCallMethod(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* obj, Value* methodName, int argc) {
    if (IsMethodOfObject(interp, obj, methodName)) {
        ++argc; // add 1 for 'this'
    } else {
        _Popp(); // pop 'this'
    }
    
    Value* method = GenericGetAttribute(interp, obj, methodName, true);
    
    if (ValueIsNull(method)) {
        _PopN(argc); 
        String method = ValueToString(methodName);
        String errMsg = FormatString("method '%s' not found on object of type %s", method, ValueTypeOf(obj));
        Value* errVal = NewErrorValue(interp, errMsg);
        free(method);
        free(errMsg);
        return errVal;
    }

    return DoCall(
        interp, 
        rootEnvObj, 
        envObj, 
        method, 
        argc
    );
}

Value* DoNot(Interpreter* interp, Value* val) {
    bool resultBool = !CoerceToBool(val);
    return resultBool ? interp->True : interp->False;
}

Value* DoPos(Interpreter* interp, Value* val) {
    if (ValueIsInt(val)) {
        return NewIntValue(interp, +CoerceToI32(val));
    } else if (ValueIsNum(val)) {
        return NewNumValue(interp, +CoerceToNum(val));
    } else {
        String errMsg = FormatString("invalid operand for operator (+): %s", ValueTypeOf(val));
        Value* errVal = NewErrorValue(interp, errMsg);
        free(errMsg);
        return errVal;
    }
}

Value* DoNeg(Interpreter* interp, Value* val) {
    if (ValueIsInt(val)) {
        return NewIntValue(interp, -CoerceToI32(val));
    } else if (ValueIsNum(val)) {
        return NewNumValue(interp, -CoerceToNum(val));
    } else {
        String errMsg = FormatString("invalid operand for operator (-): %s", ValueTypeOf(val));
        Value* errVal = NewErrorValue(interp, errMsg);
        free(errMsg);
        return errVal;
    }
}

Value* DoMul(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;
    
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        long resultNum = CoerceToI64(lhs) * CoerceToI64(rhs);
        result = (resultNum <= INT_MAX && resultNum >= INT_MIN) 
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, (double) resultNum);
    } else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        double resultNum = CoerceToNum(lhs) * CoerceToNum(rhs);
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (*): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoDiv(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;
    
    // Check for division by zero
    if ((ValueIsInt(rhs) && CoerceToI64(rhs) == 0) || 
        (ValueIsNum(rhs) && CoerceToNum(rhs) == 0.0)) {
        return NewErrorValue(interp, "division by zero");
    }
    
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        long resultNum = CoerceToI64(lhs) / CoerceToI64(rhs);
        result = (resultNum <= INT_MAX && resultNum >= INT_MIN) 
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, (double) resultNum);
    } else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        double resultNum = CoerceToNum(lhs) / CoerceToNum(rhs);
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (/): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoMod(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;
    
    // Check for modulo by zero
    if ((ValueIsInt(rhs) && CoerceToI64(rhs) == 0) || 
        (ValueIsNum(rhs) && CoerceToNum(rhs) == 0.0)) {
        return NewErrorValue(interp, "modulo by zero");
    }
    
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        long resultNum = CoerceToI64(lhs) % CoerceToI64(rhs);
        result = (resultNum <= INT_MAX && resultNum >= INT_MIN) 
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, (double) resultNum);
    } else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        double resultNum = fmod(CoerceToNum(lhs), CoerceToNum(rhs));
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (%%): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoInc(Interpreter* interp, Value* val) {
    Value* result = NULL;
    int offset    = GetOffset();

    if (ValueIsInt(val)) {
        long resultNum = CoerceToI64(val) + 1;
        result = (resultNum <= INT_MAX && resultNum >= INT_MIN) 
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, (double) resultNum);
    } else if (ValueIsNum(val)) {
        double resultNum = CoerceToNum(val) + 1.0;
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    } else {
        String errMsg = FormatString(
            "invalid operand for operator (++): %s", ValueTypeOf(val)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoAdd(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;

    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        long resultNum = CoerceToI64(lhs) + CoerceToI64(rhs);
        result = (resultNum <= INT_MAX && resultNum >= INT_MIN) 
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, (double) resultNum);
    } else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        double resultNum = CoerceToNum(lhs) + CoerceToNum(rhs);
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    } else if (ValueIsStr(lhs) && ValueIsStr(rhs)) {
        Rune* lhsRunes = (Rune*) lhs->Value.Opaque;
        Rune* rhsRunes = (Rune*) rhs->Value.Opaque;
        String lhsStr = RunesStrToString(lhsRunes);
        String rhsStr = RunesStrToString(rhsRunes);
        size_t lhsLen = strlen(lhsStr);
        size_t rhsLen = strlen(rhsStr);
        String resultStr = Allocate(lhsLen + rhsLen + 1);
        memcpy(resultStr, lhsStr, lhsLen);
        memcpy(resultStr + lhsLen, rhsStr, rhsLen);
        resultStr[lhsLen + rhsLen] = '\0';
        result = NewStrValue(interp, resultStr);
        free(lhsStr);
        free(rhsStr);
        free(resultStr);
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (+): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoDec(Interpreter* interp, Value* val) {
    Value* result = NULL;
    int offset    = GetOffset();

    if (ValueIsInt(val)) {
        long resultNum = CoerceToI64(val) - 1;
        result = (resultNum <= INT_MAX && resultNum >= INT_MIN) 
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, (double) resultNum);
    } else if (ValueIsNum(val)) {
        double resultNum = CoerceToNum(val) - 1.0;
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    } else {
        String errMsg = FormatString(
            "invalid operand for operator (--): %s", ValueTypeOf(val)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoSub(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;

    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        long resultNum = CoerceToI64(lhs) - CoerceToI64(rhs);
        result = (resultNum <= INT_MAX && resultNum >= INT_MIN) 
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, (double) resultNum);
    } else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        double resultNum = CoerceToNum(lhs) - CoerceToNum(rhs);
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (-): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoLShift(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        long resultNum = CoerceToI64(lhs) << CoerceToI64(rhs);
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (<<): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoRShift(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;
    int offset    = GetOffset();

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        long resultNum = CoerceToI64(lhs) >> CoerceToI64(rhs);
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (>>): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoLT(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        int comparison = CoerceToNum(lhs) < CoerceToNum(rhs);
        result = comparison ? interp->True : interp->False;
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (<): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoLTE(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        int comparison = CoerceToNum(lhs) <= CoerceToNum(rhs);
        result = comparison ? interp->True : interp->False;
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (<=): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoGT(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        int comparison = CoerceToNum(lhs) > CoerceToNum(rhs);
        result = comparison ? interp->True : interp->False;
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (>): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoGTE(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        int comparison = CoerceToNum(lhs) >= CoerceToNum(rhs);
        result = comparison ? interp->True : interp->False;
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (>=): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoEQ(Interpreter* interp, Value* lhs, Value* rhs) {
    return ValueIsEqual(lhs, rhs) ? interp->True : interp->False;
}

Value* DoNE(Interpreter* interp, Value* lhs, Value* rhs) {
    return !ValueIsEqual(lhs, rhs) ? interp->True : interp->False;
}

Value* DoAnd(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;

    // Bitwise AND only works on integers
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        int resultValue = lhs->Value.I32 & rhs->Value.I32;
        result = NewIntValue(interp, resultValue);
    } else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        long long resultValue = (int)CoerceToI64(lhs) & (int)CoerceToI64(rhs);
        result = NewNumValue(interp, resultValue);
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (&): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoOr(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;

    // Bitwise OR only works on integers
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        int resultValue = lhs->Value.I32 | rhs->Value.I32;
        result = NewIntValue(interp, resultValue);
    } else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        long long resultValue = (int)CoerceToI64(lhs) | (int)CoerceToI64(rhs);
        result = NewNumValue(interp, resultValue);
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (|): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoXor(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;

    // Bitwise XOR only works on integers
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        int resultValue = lhs->Value.I32 ^ rhs->Value.I32;
        result = NewIntValue(interp, resultValue);
    } else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        long long resultValue = (int)CoerceToI64(lhs) ^ (int)CoerceToI64(rhs);
        result = NewNumValue(interp, resultValue);
    } else {
        String errMsg = FormatString(
            "invalid operands for operator (^): %s and %s", ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoLoadFunction(Interpreter* interp, Value* rootEnvObj, Value* envObj, int offset, bool closure) {
    // For closure, clone the function
    Value* fn = closure
        ? NewUserFunctionValue(interp, UserFunctionClone(CoerceToUserFunction(interp->Functions[offset])))
        : interp->Functions[offset];

    UserFunction* uf     = CoerceToUserFunction(fn);
    Environment* rootEnv = CoerceToEnvironment(rootEnvObj);
    Environment* loclEnv = CoerceToEnvironment(envObj);

    if (uf->ParentEnv == NULL) uf->ParentEnv = envObj;

    for (int i = 0; i < uf->CaptureC; i++) {
        CaptureMeta capture = uf->CaptureMetas[i];
        // Traverse up the environment chain to find the captured variable
        // through depth
        int depth = 1;
        Environment* currentEnv = loclEnv;
        
        while (depth != capture.Depth && currentEnv != NULL) {
            currentEnv = CoerceToEnvironment(currentEnv->Parent);    
            depth++;
        }
        currentEnv->Locals[capture.Src]->IsCaptured = true;
        uf->Captures[capture.Dst] = currentEnv->Locals[capture.Src];
    }

    return fn;
}

#undef PushArray
#undef GetOffset