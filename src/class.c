#include "./class.h"
#include "global.h"

UserClass* CreateUserClass(String name, Value* base) {
    UserClass* cls = Allocate(sizeof(UserClass));
    cls->Name              = AllocateString(name);
    cls->Base              = base;
    cls->StaticMembers     = CreateHashMap(16);
    cls->InstanceMembers   = CreateHashMap(16);
    return cls;
}

void ClassExtend(UserClass* cls, Value* base) {
    cls->Base = base;
}

extern String ValueToString(Value*);

void ClassDefineMember(UserClass* cls, Value* key, Value* value, bool isStatic) {
    if (isStatic) {
        HashMapSet(cls->StaticMembers, ValueToString(key), value);
    } else {
        HashMapSet(cls->InstanceMembers, ValueToString(key), value);
    }
}

extern bool ValueIsCallable(Value*);

bool ClassHasMember(UserClass* cls, String key, bool isStatic, bool callable) {
    HashMap* members = isStatic ? cls->StaticMembers : cls->InstanceMembers;
    Value* member    = HashMapGet(members, key);
    if (member == NULL) {
        return false;
    }
    if (callable && callable != (bool) -1) {
        return ValueIsCallable(member);
    }
    return true;
}

Value* ClassGetMember(UserClass* cls, String key, bool isStatic) {
    HashMap* members = isStatic ? cls->StaticMembers : cls->InstanceMembers;
    return HashMapGet(members, key);
}

String ClassToString(UserClass* cls) {
    return cls->Name;
}

//

ClassInstance* CreateClassInstance(Value* proto) {
    ClassInstance* instance = Allocate(sizeof(ClassInstance));
    instance->Proto   = proto;
    instance->Members = CreateHashMap(16);
    return instance;
}

String ClassInstanceToString(ClassInstance* instance) {
    HashMap* members = instance->Members;
    String className = ClassToString(CoerceToUserClass(instance->Proto));
    
    // Start building the string
    size_t bufferSize = 1024;
    char* buffer = Allocate(bufferSize);
    size_t currentPos = 0;
    
    // Add class name and opening brace
    currentPos += snprintf(buffer + currentPos, bufferSize - currentPos, "%s { ", className);
    
    // Iterate through members
    bool first = true;
    for (size_t i = 0; i < members->Size; i++) {
        HashNode* node = &members->Buckets[i];
        while (node != NULL && node->Key != NULL) {
            // Ensure buffer is large enough
            size_t needed = currentPos + strlen(node->Key) + 100;
            if (needed > bufferSize) {
                bufferSize = needed * 2;
                char* newBuffer = Allocate(bufferSize);
                memcpy(newBuffer, buffer, currentPos);
                free(buffer);
                buffer = newBuffer;
            }
            
            if (!first) {
                currentPos += snprintf(buffer + currentPos, bufferSize - currentPos, ", ");
            }
            first = false;
            
            String valueStr = ValueToString((Value*)node->Val);
            currentPos += snprintf(buffer + currentPos, bufferSize - currentPos, 
                                  "%s: %s", node->Key, valueStr);
            
            node = node->Next;
        }
    }
    
    // Add closing brace
    currentPos += snprintf(buffer + currentPos, bufferSize - currentPos, " }");
    
    // Create final string
    String result = AllocateString(buffer);
    free(buffer);
    
    return result;
}