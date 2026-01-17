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
        array->Items = Reallocate(array->Items, sizeof(void*) * array->Capacity);
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

void ArrayExtend(Array *array, Array *other) {
    if (other->Count == 0) return;
    
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
    if (array == NULL) {
        return NULL;
    }
    
    // Calculate approximate size needed for string representation
    size_t bufferSize = 100; // Initial size for "[ ... ]"
    for (size_t i = 0; i < array->Count; i++) {
        bufferSize += 50; // Value + formatting
    }
    
    String result = Allocate(bufferSize);
    if (result == NULL) {
        return NULL;
    }
    
    strcpy(result, "[");
    bool first = true;
    
    for (size_t i = 0; i < array->Count; i++) {
        if (!first) {
            strcat(result, ", ");
        }
        
        if (array->Items[i] == array) {
            strcat(result, "[self]");
        } else {
            //NOTE: memory leak (ValueToString returns a new string that is not freed)
            strcat(result, ValueToString(array->Items[i]));
        }
        
        first = false;
    }
    
    strcat(result, "]");
    return result;
}