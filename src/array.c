#include "./array.h"

Array* CreateArray() {
    Array* array    = Allocate(sizeof(Array));
    array->Capacity = 4;
    array->Count    = 0;
    array->Items    = Allocate(sizeof(void*) * array->Capacity);
    return array;
}

void ArrayPush(Array* array, void* value) {
    if (array->Count >= array->Capacity) {
        array->Capacity *= 2;
        array->Items     = Reallocate(array->Items, sizeof(void*) * array->Capacity);
    }
    array->Items[array->Count++] = value;
}

void* ArrayPop(Array* array) {
    if (array->Count == 0) {
        // error, out of bounds
        return NULL;
    }
    return array->Items[--array->Count];
}

void* ArrayPeek(Array* array) {
    if (array->Count == 0) {
        // error, out of bounds
        return NULL;
    }
    return array->Items[array->Count - 1];
}

void* ArraySet(Array* array, size_t index, void* value) {
    if (index >= array->Count) {
        // error, out of bounds
        return NULL;
    }
    return array->Items[index] = value;
}

void* ArrayGet(Array* array, size_t index) {
    if (index >= array->Count) {
        // error, out of bounds
        return NULL;
    }
    return array->Items[index];
}

size_t ArrayLength(Array* array) {
    return array->Count;
}

void ArrayExtend(Array* array, Array* other) {
    if (other->Count == 0)
        return;

    size_t required_capacity = array->Count + other->Count;
    if (required_capacity > array->Capacity) {
        while (array->Capacity < required_capacity) {
            array->Capacity *= 2;
        }
        array->Items = Reallocate(array->Items, sizeof(void*) * array->Capacity);
    }

    for (size_t i = 0; i < other->Count; i++) {
        array->Items[array->Count++] = other->Items[i];
    }
}

extern String ValueToString(Value* value);

String ArrayToString(Array* array) {
    if (array == NULL)
        return NULL;
    if (array->Count == 0) {
        String s = Allocate(3);
        if (s) {
            s[0] = '[';
            s[1] = ']';
            s[2] = '\0';
        }
        return s;
    }

    static const String SELF     = "[self]";
    static const size_t SELF_LEN = 6;

    // ----------------------------------------------------------------
    // PASS 1: resolve all elements, measure total length
    // ----------------------------------------------------------------
    String* parts = Allocate(array->Count * sizeof(String));
    if (!parts)
        return NULL;
    size_t* lens = Allocate(array->Count * sizeof(size_t));
    if (!lens) {
        free(parts);
        return NULL;
    }

    // '[' + (Count-1)*", " + ']' + '\0'
    size_t total = 1 + (array->Count - 1) * 2 + 1 + 1;

    for (size_t i = 0; i < array->Count; i++) {
        if (array->Items[i] == (Value*) array) {
            parts[i] = NULL;  // NULL sentinel = self-reference
            lens[i]  = SELF_LEN;
        } else {
            parts[i] = ValueToString(array->Items[i]);
            lens[i]  = parts[i] ? strlen(parts[i]) : 0;
        }
        total += lens[i];
    }

    // ----------------------------------------------------------------
    // PASS 2: single allocation, memcpy everything in
    // ----------------------------------------------------------------
    String buffer = Allocate(total);
    if (!buffer)
        goto cleanup;

    String p = buffer;
    *p++     = '[';

    for (size_t i = 0; i < array->Count; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }

        if (parts[i] == NULL) {
            memcpy(p, SELF, SELF_LEN);
            p += SELF_LEN;
        } else if (lens[i] > 0) {
            memcpy(p, parts[i], lens[i]);
            p += lens[i];
        }

        free(parts[i]);
        parts[i] = NULL;
    }

    *p++ = ']';
    *p   = '\0';

cleanup:
    for (size_t i = 0; i < array->Count; i++)
        free(parts[i]);  // NULL-safe, only hits on alloc failure path
    free(parts);
    free(lens);

    return buffer;
}