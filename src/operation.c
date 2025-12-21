#include "./operation.h"

#define PushArray(type, array, count, val, defaultValue) do { \
    (array)[(count)++] = val; \
    (array) = Reallocate((array), sizeof(type) * ((count) + 1)); \
    (array)[(count)] = (defaultValue); \
} while(0)

#define GetOffset() (interp->ConstantC)

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