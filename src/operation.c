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
    return FLG_NOTFOUND;
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

        if (ValueIsNum(method)) {
            long idx = (long) CoerceToI64(method);
            if (idx < 0 || idx >= array->Count) {
                free(key);
                return interp->Null;
            }
            free(key);
            return array->Items[idx];
        }

        // Check prototype chain
        UserClass* cls = CoerceToUserClass(interp->Array);

        while (cls != NULL) {
            if (ClassHasMember(cls, key, false, true)) {
                Value* member = ClassGetMember(
                    cls, 
                    key, 
                    false
                );
                free(key);
                return true;
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
        UserClass* cls = NULL;

        while (cls != NULL) {
            if (ClassHasMember(cls, key, false, true)) {
                Value* member = ClassGetMember(
                    cls, 
                    key, 
                    false
                );
                free(key);
                return true;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }
    } else if (ValueIsClass(obj)) {
        // Handle Class static functions or attributes
        UserClass* cls = CoerceToUserClass(obj);

        while (cls != NULL) {
            if (ClassHasMember(cls, key, false, true)) {
                Value* member = ClassGetMember(
                    cls, 
                    key, 
                    true
                );
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
        UserClass* cls = CoerceToUserClass(instance->Proto);

        while (cls != NULL) {
            if (ClassHasMember(cls, key, false, true)) {
                Value* member = ClassGetMember(
                    cls, 
                    key, 
                    false
                );
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

Value* GenericGetAttribute(Interpreter* interp, Value* obj, Value* attr, bool forMethodCall) {
    String key = ValueToString(attr);
    if (ValueIsArray(obj)) {
        // Handle array methods or attributes
        Array* array = CoerceToArray(obj);

        if (ValueIsNum(attr)) {
            long idx = (long) CoerceToI64(attr);
            if (idx < 0 || idx >= array->Count) {
                free(key);
                return interp->Null;
            }
            free(key);
            return array->Items[idx];
        }

        // Check prototype chain
        UserClass* cls = CoerceToUserClass(interp->Array);

        while (forMethodCall && cls != NULL) {
            if (ClassHasMember(cls, key, false, forMethodCall)) {
                Value* member = ClassGetMember(
                    cls, 
                    key, 
                    false
                );
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
        UserClass* cls = NULL;

        while (forMethodCall && cls != NULL) {
            if (ClassHasMember(cls, key, false, forMethodCall)) {
                Value* member = ClassGetMember(
                    cls, 
                    key, 
                    false
                );
                free(key);
                return member;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }

    } else if (ValueIsClass(obj)) {
        // Handle Class static functions or attributes
        UserClass* cls = CoerceToUserClass(obj);

        while (cls != NULL) {
            if (ClassHasMember(cls, key, true, forMethodCall)) {
                Value* member = ClassGetMember(
                    cls, 
                    key, 
                    true
                );
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
        UserClass* cls = CoerceToUserClass(instance->Proto);

        while (forMethodCall && cls != NULL) {
            if (ClassHasMember(cls, key, !forMethodCall, forMethodCall)) {
                Value* member = ClassGetMember(
                    cls, 
                    key, 
                    !forMethodCall
                );
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

int DoImportCore(Interpreter* interp, String moduleName, Value** out) {
    *out = LoadCoreModule(interp, moduleName);
    if (*out != NULL) {
        return FLG_SUCCESS;
    }
    return FLG_NOTFOUND;
}

int DoSetIndex(Interpreter* interp, Value* obj, Value* index, Value* val) {
    if (ValueIsArray(obj)) {
        Array* array = CoerceToArray(obj);
        long idx = (long) CoerceToI64(index);
        if (idx < 0 || idx >= array->Count) {
            return FLG_OUT_OF_BOUNDS;
        }
        array->Items[idx] = val;
        return FLG_SUCCESS;
    } else if (ValueIsObject(obj)) {
        HashMap* map = CoerceToHashMap(obj);
        HashMapSet(map, ValueToString(index), val);
        return FLG_SUCCESS;
    } else if (ValueIsClassInstance(obj)) {
        ClassInstance* instance = CoerceToClassInstance(obj);
        HashMapSet(instance->Members, ValueToString(index), val);
        return FLG_SUCCESS;
    } else if (ValueIsClass(obj)) {
        UserClass* cls = CoerceToUserClass(obj);
        HashMapSet(cls->StaticMembers, ValueToString(index), val);
        return FLG_SUCCESS;
    }
    return FLG_INVALID_OPERATION;
}

int DoGetIndex(Interpreter* interp, Value* obj, Value* index, Value** out) {
    *out = GenericGetAttribute(interp, obj, index, false);
    if (ValueIsNull(*out)) {
        return FLG_NOTFOUND;
    }
    return FLG_SUCCESS;
}

int DoCallCtor(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* clsValue, int argc) {
    if (clsValue == NULL) Panic("Attempted to call constructor on a null value\n");

    if (!ValueIsClass(clsValue)) {
        _PopN(argc); return FLG_INVALID_OPERATION;
    }

    UserClass* cls = CoerceToUserClass(clsValue);

    if (!ClassHasMember(cls, CONSTRUCTOR_NAME, false, true)) {
        if (argc != 0) {
            return FLG_ARG_MISMATCH;
        }
        // Push default instance, no constructor call
        ClassInstance* instance = CreateClassInstance(clsValue);
        _Push(NewClassInstanceValue(interp, instance));
        return FLG_SUCCESS;
    }

    // Push thisArg
    ClassInstance* instance = CreateClassInstance(clsValue);
    Value* instanceValue = NewClassInstanceValue(interp, instance);
    _Push(instanceValue);

    Value* constructor = ClassGetMember(cls, CONSTRUCTOR_NAME, false);

    int flg = DoCall(
        interp, 
        rootEnvObj, 
        envObj, 
        constructor, 
        ++argc
    );
    
    if (flg == FLG_SUCCESS) {
        _Popp(); // Pop constructor return value
        _Push(instanceValue); // Push instance as return value
    }

    return flg;
}

int DoCall(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* fn, int argc) {
    if (fn == NULL) Panic("Attempted to call a null value\n");

    if (!ValueIsCallable(fn)) {
        _PopN(argc); return FLG_INVALID_OPERATION;
    }

    if (ValueIsNativeFunction(fn)) {
        NativeFunctionMeta* nFMeta = CoerceToNativeFunctionMeta(fn);
        NativeFunction nf          = nFMeta->FuncPtr;

        if (nFMeta->Argc != VARARG && argc != nFMeta->Argc) {
            _PopN(argc); return FLG_ARG_MISMATCH;
        }

        Value** args = Allocate(sizeof(Value*) * argc);

        for (int i = 0; i < argc; i++) {
            args[i] = _Popp();
        }

        Value* res = nf(interp, argc, args);

        if (ValueIsError(res)) {
            return FLG_ERROR;
        }

        _Push(res);
        free(args);
        
        return FLG_SUCCESS;
    }

    // Call
    UserFunction* uf = CoerceToUserFunction(fn);
    Environment* env = CreateEnvironment(envObj, uf->LocalC);

    if (!ValueIsCallable(fn)) {
        Panic("Attempted to call a non-callable value: %s\n", ValueToString(fn));
    }

    if (argc != uf->Argc) {
        _PopN(argc); return FLG_ARG_MISMATCH;
    }
    
    Run(
        interp, 
        fn, 
        (uf->ParentEnv != NULL ? uf->ParentEnv : rootEnvObj), 
        NewEnvironmentValue(interp, env)
    );
    return FLG_SUCCESS;
}

int DoCallMethod(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* obj, Value* methodName, int argc) {
    if (IsMethodOfObject(interp, obj, methodName)) {
        ++argc; // add 1 for 'this'
    } else {
        _Popp(); // pop 'this'
    }
    
    Value* method = GenericGetAttribute(interp, obj, methodName, true);
    
    if (ValueIsNull(method)) {
        _PopN(argc); return FLG_NOTFOUND;
    }

    return DoCall(
        interp, 
        rootEnvObj, 
        envObj, 
        method, 
        argc
    );
}

int DoNot(Interpreter* interp, Value* val, Value** out) {
    bool resultBool = !CoerceToBool(val);
    *out = resultBool ? interp->True : interp->False;
    return FLG_SUCCESS;
}

int DoPos(Interpreter* interp, Value* val, Value** out) {
    *out = val;
    return FLG_SUCCESS;
}

int DoNeg(Interpreter* interp, Value* val, Value** out) {
    *out = val;
    return FLG_SUCCESS;
}

int DoMul(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();
    
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
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );
    return offset;
}

int DoDiv(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();
    
    // Check for division by zero
    if ((ValueIsInt(rhs) && CoerceToI64(rhs) == 0) || 
        (ValueIsNum(rhs) && CoerceToNum(rhs) == 0.0)) {
        return FLG_ZERO_DIV;
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
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );
    return offset;
}

int DoMod(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();
    
    // Check for modulo by zero
    if ((ValueIsInt(rhs) && CoerceToI64(rhs) == 0) || 
        (ValueIsNum(rhs) && CoerceToNum(rhs) == 0.0)) {
        return FLG_ZERO_DIV;
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
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );
    return offset;
}

int DoInc(Interpreter* interp, Value* val, Value** out) {
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
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants,
        interp->ConstantC, 
        result, 
        NULL
    );
    return offset;
}

int DoAdd(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();

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
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoDec(Interpreter* interp, Value* val, Value** out) {
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
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants,
        interp->ConstantC, 
        result, 
        NULL
    );
    return offset;
}

int DoSub(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();

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
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoLShift(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        long resultNum = CoerceToI64(lhs) << CoerceToI64(rhs);
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoRShift(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        long resultNum = CoerceToI64(lhs) >> CoerceToI64(rhs);
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoLT(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        int comparison = CoerceToNum(lhs) < CoerceToNum(rhs);
        result = comparison ? interp->True : interp->False;
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoLTE(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        int comparison = CoerceToNum(lhs) <= CoerceToNum(rhs);
        result = comparison ? interp->True : interp->False;
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoGT(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        int comparison = CoerceToNum(lhs) > CoerceToNum(rhs);
        result = comparison ? interp->True : interp->False;
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoGTE(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        int comparison = CoerceToNum(lhs) >= CoerceToNum(rhs);
        result = comparison ? interp->True : interp->False;
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoEQ(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();

    // Check if both are numbers (int or num)
    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        int comparison = CoerceToNum(lhs) == CoerceToNum(rhs);
        result = comparison ? interp->True : interp->False;
    }
    // Check if both are booleans
    else if (ValueIsBool(lhs) && ValueIsBool(rhs)) {
        int comparison = lhs->Value.I32 == rhs->Value.I32;
        result = comparison ? interp->True : interp->False;
    }
    // Check if both are null
    else if (ValueIsNull(lhs) && ValueIsNull(rhs)) {
        result = interp->True;
    }
    // Check if both are strings
    else if (ValueIsStr(lhs) && ValueIsStr(rhs)) {
        Rune* lhsRunes = (Rune*) lhs->Value.Opaque;
        Rune* rhsRunes = (Rune*) rhs->Value.Opaque;
        
        // Compare rune by rune
        int i = 0;
        int equal = 1;
        while (lhsRunes[i] != 0 || rhsRunes[i] != 0) {
            if (lhsRunes[i] != rhsRunes[i]) {
                equal = 0;
                break;
            }
            i++;
        }
        result = equal ? interp->True : interp->False;
    }
    // Different types are not equal
    else {
        result = interp->False;
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoNE(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    Value* result = NULL;
    int offset    = GetOffset();

    // Check if both are numbers (int or num)
    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        int comparison = CoerceToNum(lhs) != CoerceToNum(rhs);
        result = comparison ? interp->True : interp->False;
    }
    // Check if both are booleans
    else if (ValueIsBool(lhs) && ValueIsBool(rhs)) {
        int comparison = lhs->Value.I32 != rhs->Value.I32;
        result = comparison ? interp->True : interp->False;
    }
    // Check if both are null
    else if (ValueIsNull(lhs) && ValueIsNull(rhs)) {
        result = interp->False;
    }
    // Check if both are strings
    else if (ValueIsStr(lhs) && ValueIsStr(rhs)) {
        Rune* lhsRunes = (Rune*) lhs->Value.Opaque;
        Rune* rhsRunes = (Rune*) rhs->Value.Opaque;
        
        // Compare rune by rune
        int i = 0;
        int equal = 1;
        while (lhsRunes[i] != 0 || rhsRunes[i] != 0) {
            if (lhsRunes[i] != rhsRunes[i]) {
                equal = 0;
                break;
            }
            i++;
        }
        result = equal ? interp->False : interp->True;
    }
    // Different types are not equal
    else {
        result = interp->True;
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoAnd(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    int offset    = interp->ConstantC;
    Value* result = NULL;

    // Bitwise AND only works on integers
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        int resultValue = lhs->Value.I32 & rhs->Value.I32;
        result = NewIntValue(interp, resultValue);
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoOr(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    int offset    = interp->ConstantC;
    Value* result = NULL;

    // Bitwise OR only works on integers
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        int resultValue = lhs->Value.I32 | rhs->Value.I32;
        result = NewIntValue(interp, resultValue);
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoXor(Interpreter* interp, Value* lhs, Value* rhs, Value** out) {
    int offset    = interp->ConstantC;
    Value* result = NULL;

    // Bitwise XOR only works on integers
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        int resultValue = lhs->Value.I32 ^ rhs->Value.I32;
        result = NewIntValue(interp, resultValue);
    }

    if (result == NULL) {
        return FLG_INVALID_OPERATION;
    }
    
    if (out != NULL) {
        *out = result;
        return offset;
    }

    if (_GetConstantOffset(interp, result) != FLG_NOTFOUND) {
        return _GetConstantOffset(interp, result);
    }

    PushArray(
        Value*,
        interp->Constants, 
        interp->ConstantC, 
        result, 
        NULL
    );

    return offset;
}

int DoLoadFunction(Interpreter* interp, Value* rootEnvObj, Value* envObj, int offset, bool closure, Value** out) {
    // For closure, clone the function
    Value* fn = closure
        ? NewUserFunctionValue(interp, UserFunctionClone(CoerceToUserFunction(interp->Functions[offset])))
        : interp->Functions[offset];

    if (out != NULL) {
        *out = fn;
    }

    UserFunction* uf     = CoerceToUserFunction(fn);
    Environment* rootEnv = CoerceToEnvironment(rootEnvObj);
    Environment* loclEnv = CoerceToEnvironment(envObj);
    for (int i = 0; i < uf->CaptureC; i++) {
        CaptureMeta capture = uf->CaptureMetas[i];
        if (capture.IsGlobal) {
            rootEnv->Locals[capture.Src]->IsCaptured = true;
            uf->Captures[capture.Dst] = rootEnv->Locals[capture.Src];
        } else {
            loclEnv->Locals[capture.Src]->IsCaptured = true;
            uf->Captures[capture.Dst] = loclEnv->Locals[capture.Src];
        }
    }

    uf->ParentEnv = rootEnvObj;
    return FLG_SUCCESS;
}

#undef PushArray
#undef GetOffset