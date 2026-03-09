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
    
    // Start with reasonable initial capacity
    size_t capacity = 32 + array->Count * 16;
    size_t length = 0;
    String buffer = Allocate(capacity);
    
    if (buffer == NULL) return NULL;
    
    buffer[length++] = '[';
    buffer[length] = '\0';
    
    for (size_t i = 0; i < array->Count; i++) {
        if (i > 0) {
            // Ensure space for ", "
            if (length + 2 >= capacity) {
                capacity *= 2;
                buffer = Reallocate(buffer, capacity);
            }
            buffer[length++] = ',';
            buffer[length++] = ' ';
            buffer[length] = '\0';
        }
        
        String valStr;
        if (array->Items[i] == array) {
            valStr = "[self]";
            size_t len = 6;
            if (length + len + 1 >= capacity) {
                capacity = capacity * 2 > length + len + 32 ? capacity * 2 : length + len + 32;
                buffer = Reallocate(buffer, capacity);
            }
            strcpy(buffer + length, valStr);
            length += len;
        } else {
            valStr = ValueToString(array->Items[i]);
            if (valStr != NULL) {
                size_t len = strlen(valStr);
                if (length + len + 1 >= capacity) {
                    capacity = capacity * 2 > length + len + 32 ? capacity * 2 : length + len + 32;
                    buffer = Reallocate(buffer, capacity);
                }
                strcpy(buffer + length, valStr);
                length += len;
                free(valStr);
            }
        }
    }
    
    // Ensure space for "]"
    if (length + 2 >= capacity) {
        buffer = Reallocate(buffer, capacity + 2);
    }
    buffer[length++] = ']';
    buffer[length] = '\0';
    
    return buffer;
}