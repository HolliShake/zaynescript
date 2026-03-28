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

extern void Run(Interpreter* interp, Value* fnValue);

extern CoreMapper _CoreModuleMappers[];

void SaveRootEnv(Interpreter* interp, Value* env) {
    interp->Envs[interp->EnvC++] = interp->CallEnv;
    interp->RootEnv = env;
    interp->CallEnv = env;
}

void SaveEnv(Interpreter* interp, Value* env) {
    interp->Envs[interp->EnvC++] = interp->CallEnv;
    interp->CallEnv = env;
}

void RestoreEnv(Interpreter* interp) {
    Value* top = interp->Envs[interp->EnvC - 1];
    interp->Envs[--interp->EnvC] = NULL;
    interp->CallEnv = top;
}

void RestoreNthEnvAndSync(Interpreter* interp, int n) {
    if (n < 0 || n >= interp->EnvC) {
        // Invalid index, do nothing or handle error as needed
        return;
    }
    Value* top = interp->Envs[n];
    // Remove all environments above n
    for (int i = interp->EnvC - 1; i > n; i--) {
        interp->Envs[i] = NULL;
    }
    interp->EnvC = n + 1;
    interp->CallEnv = top;
}

bool IsMethodOfObject(Interpreter* interp, Value* obj, Value* method) {
    String key = ValueToString(method);
    if (ValueIsPromise(obj)) {
        // Handle Promise methods or attributes
        Class* cls = CoerceToUserClass(interp->Promise);

        while (cls != NULL) {
            if (ClassHasMember(cls, key, false, true)) {
                free(key);
                return true;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }

    } else if (ValueIsArray(obj)) {
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
    if (ValueIsPromise(obj)) {
        // Handle Promise methods or attributes
        Class* cls = CoerceToUserClass(interp->Promise);

        while (cls != NULL) {
            if (ClassHasMember(cls, key, false, forMethodCall)) {
                Value* member = ClassGetMember(cls, key, false);
                free(key);
                return member;
            }
            if (cls->Base == NULL) break;
            cls = CoerceToUserClass(cls->Base);
        }

    } else if (ValueIsArray(obj)) {
        // Handle array methods or attributes
        Array* array = CoerceToArray(obj);

        if (ValueIsAnyNum(index)) {
            long idx = (long) CoerceToI64(index);
            if (idx < 0 || idx >= array->Count) {
                free(key);
                String errMsg = FormatString("%s: array index %ld out of bounds", INDEX_ERROR, idx);
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
    } else if (ValueIsStr(obj)) {
        Rune* rne = (Rune*) obj->Value.Opaque;
        String st = ValueToString(obj);
        size_t ln = utf_length(st);
        free(st);
        if (!ValueIsAnyNum(index)) {
            free(key);
            String errMsg = FormatString("%s: string indices must be integers, not %s", TYPE_ERROR, ValueTypeOf(index));
            Value* errVal = NewErrorValue(interp, errMsg);
            free(errMsg);
            return errVal;
        }
        long idx   = CoerceToI64(index);
        if (idx < 0 || idx >= ln) {
            free(key);
            String errMsg = FormatString("%s: string index %ld out of bounds", INDEX_ERROR, idx);
            Value* errVal = NewErrorValue(interp, errMsg);
            free(errMsg);
            return errVal;
        }
        String str = utf_rune_to_string(rne[idx]);
        Value* val = NewStrValue(interp, str);
        free(key);
        free(str);
        return val;
    }
    free(key);
    return interp->Null;
}

Value* DoImportCore(Interpreter* interp, String moduleName) {
    Value* result = (Value*) HashMapGet(interp->Imports, moduleName);

    if (result != NULL) {
        return result;
    }

    result = LoadCoreModule(interp, moduleName);

    if (result == NULL) {
        String errMsg = FormatString("%s: core module '%s' not found", IMPORT_ERROR, moduleName);
        Value* errVal = NewErrorValue(interp, errMsg);
        free(errMsg);
        return errVal;
    }

    HashMapSet(interp->Imports, moduleName, result);

    return result;
}

Value* DoSetIndex(Interpreter* interp, Value* obj, Value* index, Value* val) {
    //Note: memory leak (hashKey from ValueToString is not freed in the Array branch)
    String hashKey = ValueToString(index);
    if (ValueIsArray(obj)) {
        Array* array = CoerceToArray(obj);
        long idx = (long) CoerceToI64(index);
        if (idx < 0 || idx >= array->Count) {
            String errMsg = FormatString("%s: array index %ld out of bounds", INDEX_ERROR, idx);
            Value* errVal = NewErrorValue(interp, errMsg);
            free(errMsg);
            return errVal;
        }
        array->Items[idx] = val;
    } else if (ValueIsObject(obj)) {
        HashMap* map = CoerceToHashMap(obj);
        HashMapSet(map, hashKey, val);
        free(hashKey);
    } else if (ValueIsClassInstance(obj)) {
        ClassInstance* instance = CoerceToClassInstance(obj);
        HashMapSet(instance->Members, hashKey, val);
        free(hashKey);
    } else if (ValueIsClass(obj)) {
        Class* cls = CoerceToUserClass(obj);
        HashMapSet(cls->StaticMembers, hashKey, val);
        free(hashKey);
    } else {
        return NewErrorFValue(interp, "%s: cannot set index on non-object", TYPE_ERROR);
    }
    return interp->Null;
}

Value* DoGetIndex(Interpreter* interp, Value* obj, Value* index) {
    return GenericGetAttribute(interp, obj, index, false);
}

Value* DoCallCtor(Interpreter* interp, Value* clsValue, int argc) {
    if (clsValue == NULL) Panic("Attempted to call constructor on a null value\n");

    if (!ValueIsClass(clsValue)) {
        _PopN(argc);
        return NewErrorFValue(interp, "%s: attempted to call constructor on non-class value", TYPE_ERROR);
    }

    Class* cls = CoerceToUserClass(clsValue);

    if (!ClassHasMember(cls, CONSTRUCTOR_NAME, false, true)) {
        if (argc != 0) {
            _PopN(argc);
            String errMsg = FormatString("%s: argument count mismatch, expected 0 arguments but got %d", ARGUMENT_ERROR, argc);
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
        constructor, 
        ++argc,
        true
    );
    
    if (ValueIsNull(result)) {
        _Popp(); // Pop constructor return value
        _Push(instanceValue); // Push instance as return value
    }

    return result;
}

Value* DoCallMethod(Interpreter* interp, Value* obj, Value* methodName, int argc) {
    bool withThis = IsMethodOfObject(interp, obj, methodName);
    if (withThis) {
        ++argc; // add 1 for 'this'
    } else {
        _Popp(); // pop 'this'
    }
    
    Value* method = GenericGetAttribute(interp, obj, methodName, true);
    
    if (ValueIsNull(method)) {
        _PopN(argc); 
        String method = ValueToString(methodName);
        String errMsg = FormatString("%s: method '%s' not found on object of type %s", ATTRIBUTE_ERROR, method, ValueTypeOf(obj));
        Value* errVal = NewErrorValue(interp, errMsg);
        free(method);
        free(errMsg);
        return errVal;
    }

    return DoCall(
        interp, 
        method, 
        argc,
        withThis
    );
}

Value* DoCall(Interpreter* interp, Value* fn, int argc, bool withThis) {
    Value* env = NULL;

    if (fn == NULL) Panic("Attempted to call a null value!");

    if (ValueIsPromise(fn)) {
        // Resume only
        _PopN(argc);
        StateMachine* sm = CoerceToStateMachine(fn);
        if (sm->WaitFor == NULL) Panic("Attempted to resume a promise that is not waiting on any value!");
        if (sm->State == PENDING) {
            // 2. ANCHOR: Set the new bottom to the CURRENT top of the stack
            sm->StackBot = interp->StackC;

            // 3. Restore to the OFFSET position (interp->Stacks + sm->StackBot)
            if (sm->Stacks != NULL && sm->StackTop > 0) {
                memcpy(&interp->Stacks[sm->StackBot], sm->Stacks, sizeof(Value*) * sm->StackTop);
                
                // 4. Advance the global stack pointer
                interp->StackC = sm->StackBot + sm->StackTop;
            }
        } else {
            Panic("Attempted to resume a promise that is not pending (current state: %d)\n", sm->State);
        }

        // 1. Save
        interp->Envs[interp->EnvC++] = interp->CallEnv;
        interp->CallEnv = env = sm->CallEnv;
        
        // 2. Run
        Run(interp, fn);

        // 3. Restore
        env = interp->Envs[--interp->EnvC];
        interp->Envs[interp->EnvC] = NULL;
        interp->CallEnv = env;

        return interp->Null;
    }

    if (!ValueIsCallable(fn)) {
        _PopN(argc);
        //Note: memory leak (ValueToString(fn) allocates a string passed to NewErrorFValue but never freed)
        return NewErrorFValue(interp, "%s: invalid operation: attempted to call a non-callable value (%s)", TYPE_ERROR, ValueToString(fn));
    }

    if (ValueIsNativeFunction(fn)) {
        NativeFunction* nFMeta = CoerceToNativeFunction(fn);
        NativeFunctionCallback nativeFunc  = nFMeta->FuncPtr;

        if (nFMeta->Argc != VARARG && argc != nFMeta->Argc) {
            _PopN(argc); 
            String errMsg = FormatString("%s: argument count mismatch: expected %d arguments but got %d", ARGUMENT_ERROR, nFMeta->Argc, argc);
            Value* errVal = NewErrorValue(interp, errMsg);
            free(errMsg);
            return errVal;
        }

        Value** args = Allocate(sizeof(Value*) * argc); args[0] = NULL;

        int end = 0;
        if (withThis) {
            end = 1;
            args[0] = _Popp();
        }

        for (int i = argc - 1; i >= end; i--) {
            args[i] = _Popp();
            args[i];
        }

        Value* res = nativeFunc(interp, argc, args);

        _Push(res);
        free(args);
        
        return ValueIsError(res) ? res : interp->Null;
    }

    // Call
    UserFunction* uf = CoerceToUserFunction(fn);

    if (argc != uf->Argc) {
        _PopN(argc);
        String errMsg = FormatString("%s: argument count mismatch: expected %d arguments but got %d", ARGUMENT_ERROR, uf->Argc, argc);
        Value* errVal = NewErrorValue(interp, errMsg);
        free(errMsg);
        return errVal;
    }

    if (uf->Scope == NULL) {
        Panic("User function '%s' has null Scope\n", uf->Name != NULL ? uf->Name : "<anonymous>");
    }

    // 1. Save
    interp->Envs[interp->EnvC++] = interp->CallEnv;
    interp->CallEnv = env = NewEnvironmentValue(
        interp, 
        CreateEnvironment(
            uf->Scope, // Use the scope where the function is defined as parent env!
            uf->LocalC
        )
    );

    // 2. Run the function
    Run(interp, fn);

    // 3. Restore
    env = interp->Envs[--interp->EnvC];
    interp->Envs[interp->EnvC] = NULL;
    interp->CallEnv = env;

    return interp->Null;
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
    } else if (ValueIsAnyNum(val)) {
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        //Note: memory leak (CoerceToBitField allocates a new bf_t for VLT_INT/VLT_NUM types, but that temporary is never freed)
        bf_set(resNum, CoerceToBitField(interp, val));
        // unary + is a no-op, just copy
        int prec = BFPrecession(val);
        return prec == PREC_INT
            ? NewBigIntValue(interp, resNum)
            : NewBigNumValue(interp, resNum);
    } else {
        String errMsg = FormatString("%s: invalid operand for operator (+): %s", TYPE_ERROR, ValueTypeOf(val));
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
    } else if (ValueIsAnyNum(val)) {
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        //Note: memory leak (CoerceToBitField allocates a new bf_t for VLT_INT/VLT_NUM types, but that temporary is never freed)
        bf_set(resNum, CoerceToBitField(interp, val));
        bf_neg(resNum); // flip sign bit
        int prec = BFPrecession(val);
        return prec == PREC_INT
            ? NewBigIntValue(interp, resNum)
            : NewBigNumValue(interp, resNum);
    } else {
        String errMsg = FormatString("%s: invalid operand for operator (-): %s", TYPE_ERROR, ValueTypeOf(val));
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        int prec = BFPrecession(lhs) | BFPrecession(rhs);
        bf_mul(resNum, lhsNum, rhsNum, prec, BF_RNDN | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
        result = prec == PREC_INT
            ? NewBigIntValue(interp, resNum)
            : NewBigNumValue(interp, resNum);
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (*): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
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
        return NewErrorFValue(interp, "%s: division by zero", ZERO_DIVISION_ERROR);
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        int prec = BFPrecession(lhs) | BFPrecession(rhs);
        bf_div(resNum, lhsNum, rhsNum, prec, BF_RNDN | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
        result = prec == PREC_INT
            ? NewBigIntValue(interp, resNum)
            : NewBigNumValue(interp, resNum);
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (/): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
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
        return NewErrorFValue(interp, "%s: modulo by zero", ZERO_DIVISION_ERROR);
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        int prec = BFPrecession(lhs) | BFPrecession(rhs);
        bf_rem(resNum, lhsNum, rhsNum, prec, BF_RNDN | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS, BF_RNDZ);
        result = prec == PREC_INT
            ? NewBigIntValue(interp, resNum)
            : NewBigNumValue(interp, resNum);
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (%%): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
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
    } else if (ValueIsAnyNum(val)) {
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        //Note: memory leak (CoerceToBitField allocates a new bf_t for VLT_INT/VLT_NUM types, but that temporary is never freed)
        bf_set(resNum, CoerceToBitField(interp, val));
        bf_add_si(resNum, resNum, 1, BF_PREC_INF, BF_RNDZ | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
        int prec = BFPrecession(val);
        return prec == PREC_INT
            ? NewBigIntValue(interp, resNum)
            : NewBigNumValue(interp, resNum);
    } else {
        String errMsg = FormatString(
            "%s: invalid operand for operator (++): %s", TYPE_ERROR, ValueTypeOf(val)
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        int prec = BFPrecession(lhs) | BFPrecession(rhs);
        bf_add(resNum, lhsNum, rhsNum, prec, BF_RNDN | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
        result = prec == PREC_INT
            ? NewBigIntValue(interp, resNum)
            : NewBigNumValue(interp, resNum);
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
            "%s: invalid operands for operator (+): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
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
    } else if (ValueIsAnyNum(val)) {
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        //Note: memory leak (CoerceToBitField allocates a new bf_t for VLT_INT/VLT_NUM types, but that temporary is never freed)
        bf_set(resNum, CoerceToBitField(interp, val));
        bf_add_si(resNum, resNum, -1, BF_PREC_INF, BF_RNDZ | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
        int prec = BFPrecession(val);
        return prec == PREC_INT
            ? NewBigIntValue(interp, resNum)
            : NewBigNumValue(interp, resNum);
    } else {
        String errMsg = FormatString(
            "%s: invalid operand for operator (--): %s", TYPE_ERROR, ValueTypeOf(val)
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        int prec = BFPrecession(lhs) | BFPrecession(rhs);
        bf_sub(resNum, lhsNum, rhsNum, prec, BF_RNDN | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
        result = prec == PREC_INT
            ? NewBigIntValue(interp, resNum)
            : NewBigNumValue(interp, resNum);
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (-): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);

        // Get shift amount from rhs
        slimb_t shiftAmount;
#if LIMB_BITS == 32
        bf_get_int32(&shiftAmount, rhsNum, 0);
        if (shiftAmount == INT32_MIN)
            shiftAmount = INT32_MIN + 1;
#else
        bf_get_int64(&shiftAmount, rhsNum, 0);
        if (shiftAmount == INT64_MIN)
            shiftAmount = INT64_MIN + 1;
#endif

        bf_set(resNum, lhsNum);
        bf_mul_2exp(resNum, shiftAmount, BF_PREC_INF, BF_RNDZ);
        // Left shift should never produce a fraction on integers,
        // but guard anyway in case lhs is a float
        if (shiftAmount < 0) {
            bf_rint(resNum, BF_RNDD);
        }

        int prec = BFPrecession(lhs) | BFPrecession(rhs);
        result = prec == PREC_INT
            ? NewBigIntValue(interp, resNum)
            : NewBigNumValue(interp, resNum);
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (<<): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoRShift(Interpreter* interp, Value* lhs, Value* rhs) {
    Value* result = NULL;

    if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
        long resultNum = CoerceToI64(lhs) >> CoerceToI64(rhs);
        result = (resultNum == (int)resultNum && resultNum <= INT_MAX && resultNum >= INT_MIN)
            ? NewIntValue(interp, (int) resultNum)
            : NewNumValue(interp, resultNum);
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);

        // Get shift amount from rhs and negate it (right shift = left shift by -n)
        slimb_t shiftAmount;
#if LIMB_BITS == 32
        bf_get_int32(&shiftAmount, rhsNum, 0);
        if (shiftAmount == INT32_MIN)
            shiftAmount = INT32_MIN + 1;
#else
        bf_get_int64(&shiftAmount, rhsNum, 0);
        if (shiftAmount == INT64_MIN)
            shiftAmount = INT64_MIN + 1;
#endif
        // Negate to make it a right shift
        shiftAmount = -shiftAmount;

        bf_set(resNum, lhsNum);
        bf_mul_2exp(resNum, shiftAmount, BF_PREC_INF, BF_RNDZ);
        // Right shift can produce a fraction, floor it (arithmetic shift behavior)
        bf_rint(resNum, BF_RNDD);

        int prec = BFPrecession(lhs) | BFPrecession(rhs);
        result = prec == PREC_INT
            ? NewBigIntValue(interp, resNum)
            : NewBigNumValue(interp, resNum);
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (>>): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        int comparison = bf_cmp_lt(lhsNum, rhsNum);
        result = comparison ? interp->True : interp->False;
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (<): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        int comparison = bf_cmp_le(lhsNum, rhsNum);
        result = comparison ? interp->True : interp->False;
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (<=): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        int comparison = bf_cmp_lt(rhsNum, lhsNum);
        result = comparison ? interp->True : interp->False;
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (>): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        int comparison = bf_cmp_le(rhsNum, lhsNum);
        result = comparison ? interp->True : interp->False;
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (>=): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoEQ(Interpreter* interp, Value* lhs, Value* rhs) {
    if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        int comparison = bf_cmp(lhsNum, rhsNum) == 0;
        return comparison ? interp->True : interp->False;
    }
    return ValueIsEqual(lhs, rhs) ? interp->True : interp->False;
}

Value* DoNE(Interpreter* interp, Value* lhs, Value* rhs) {
    if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        int comparison = bf_cmp(lhsNum, rhsNum) != 0;
        return comparison ? interp->True : interp->False;
    }
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        bf_logic_and(resNum, lhsNum, rhsNum);
        result = NewBigIntValue(interp, resNum);
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (&): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        bf_logic_or(resNum, lhsNum, rhsNum);
        result = NewBigIntValue(interp, resNum);
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (|): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
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
    } else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
        //Note: memory leak (CoerceToBitField allocates a new bf_t when lhs or rhs is VLT_INT/VLT_NUM, but those temporaries are never freed)
        bf_t* lhsNum = CoerceToBitField(interp, lhs);
        bf_t* rhsNum = CoerceToBitField(interp, rhs);
        bf_t* resNum = Allocate(sizeof(bf_t));
        bf_init(&interp->BfContext, resNum);
        bf_logic_xor(resNum, lhsNum, rhsNum);
        result = NewBigIntValue(interp, resNum);
    } else {
        String errMsg = FormatString(
            "%s: invalid operands for operator (^): %s and %s", TYPE_ERROR, ValueTypeOf(lhs), ValueTypeOf(rhs)
        );
        result = NewErrorValue(interp, errMsg);
        free(errMsg);
    }

    return result;
}

Value* DoLoadFunction(Interpreter* interp, int offset, bool closure) {
    // For closure, clone the function
    Value*        fn = interp->Functions[offset];
    UserFunction* uf = CoerceToUserFunction(fn);
    uf->Scope = interp->CallEnv;
    
    if (closure) {
        fn = NewUserFunctionValue(interp, UserFunctionClone(uf));
        uf = CoerceToUserFunction(fn);
        uf->Scope = interp->CallEnv;
    }

    Environment* rootEnv = CoerceToEnvironment(interp->RootEnv);
    Environment* loclEnv = CoerceToEnvironment(interp->CallEnv);

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
        
        uf->Captures[capture.Dst] = currentEnv->Locals[capture.Src];
        uf->Captures[capture.Dst]->IsCaptured = true;
        uf->Captures[capture.Dst]->RefCount++;
    }

    return fn;
}

#undef PushArray
#undef GetOffset