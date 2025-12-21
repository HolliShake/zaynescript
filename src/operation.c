#include "./operation.h"

#define PushArray(type, array, count, val, defaultValue) do { \
    (array)[(count)++] = val; \
    (array) = Reallocate((array), sizeof(type) * ((count) + 1)); \
    (array)[(count)] = (defaultValue); \
} while(0)

#define GetOffset() (interp->ConstantC)

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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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
        char*  lhsStr = (char*) lhs->Value.Opaque;
        char*  rhsStr = (char*) rhs->Value.Opaque;
        size_t lhsLen = strlen(lhsStr);
        size_t rhsLen = strlen(rhsStr);
        String resultStr = Allocate(lhsLen + rhsLen + 1);
        memcpy(resultStr, lhsStr, lhsLen);
        memcpy(resultStr + lhsLen, rhsStr, rhsLen + 1);
        result = NewStrValue(interp, resultStr);
    }

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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
    int offset = interp->ConstantC;
    Value* result = NULL;

    // Bitwise AND only works on integers
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        int resultValue = lhs->Value.I32 & rhs->Value.I32;
        result = NewIntValue(interp, resultValue);
    }

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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
    int offset = interp->ConstantC;
    Value* result = NULL;

    // Bitwise OR only works on integers
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        int resultValue = lhs->Value.I32 | rhs->Value.I32;
        result = NewIntValue(interp, resultValue);
    }

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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
    int offset = interp->ConstantC;
    Value* result = NULL;

    // Bitwise XOR only works on integers
    if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
        int resultValue = lhs->Value.I32 ^ rhs->Value.I32;
        result = NewIntValue(interp, resultValue);
    }

    if (result != NULL && out != NULL) {
        *out = result;
        return offset;
    } else if (result == NULL && out == NULL) {
        return FLG_INVALID_OPERATION;
    } else if (result == NULL) {
        return FLG_NOTFOUND;
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

#undef PushArray
#undef GetOffset