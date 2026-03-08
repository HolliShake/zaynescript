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

// StringBuffer implementation for efficient string concatenation
typedef struct {
    String Data;
    size_t Capacity;
    size_t Length;
} StringBuffer;

static void StringBuffer_Init(StringBuffer* sb, size_t initialCapacity) {
    sb->Capacity = initialCapacity > 0 ? initialCapacity : 32;
    sb->Data = Allocate(sb->Capacity);
    sb->Length = 0;
    if (sb->Data) sb->Data[0] = '\0';
}

static void StringBuffer_Append(StringBuffer* sb, String str) {
    if (str == NULL || sb->Data == NULL) return;
    size_t len = strlen(str);
    if (sb->Length + len + 1 > sb->Capacity) {
        size_t newCapacity = sb->Capacity * 2;
        if (newCapacity < sb->Length + len + 1) {
            newCapacity = sb->Length + len + 1 + 32;
        }
        sb->Data = Reallocate(sb->Data, newCapacity);
        sb->Capacity = newCapacity;
    }
    strcpy(sb->Data + sb->Length, str);
    sb->Length += len;
}

String ArrayToString(Array* array) {
    if (array == NULL) {
        return NULL;
    }
    
    // Calculate approximate initial size
    size_t initialSize = 32;
    if (array->Count > 0) {
        initialSize += array->Count * 16; // Estimate 16 chars per item on average
    }
    
    StringBuffer sb;
    StringBuffer_Init(&sb, initialSize);
    
    if (sb.Data == NULL) return NULL;
    
    StringBuffer_Append(&sb, "[");
    
    for (size_t i = 0; i < array->Count; i++) {
        if (i > 0) {
            StringBuffer_Append(&sb, ", ");
        }
        
        if (array->Items[i] == array) {
            StringBuffer_Append(&sb, "[self]");
        } else {
            String valStr = ValueToString(array->Items[i]);
            if (valStr != NULL) {
                StringBuffer_Append(&sb, valStr);
                free(valStr); // Fix memory leak
            }
        }
    }
    
    StringBuffer_Append(&sb, "]");
    return sb.Data;
}