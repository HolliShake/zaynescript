#include "./date.h"

// Helper to get timestamp from instance
static double _GetTimestamp(ClassInstance* instance) {
    Value* val = (Value*) HashMapGet(instance->Members, "_value");
    if (val && ValueIsNum(val)) {
        return val->Value.Num;
    }
    return 0.0;
}

// Helper to set timestamp on instance
static void _SetTimestamp(Interpreter* interpreter, ClassInstance* instance, double timestamp) {
    HashMapSet(instance->Members, "_value", NewNumValue(interpreter, timestamp));
}

// Very basic date parser
static double _ParseDateString(String dateStr) {
    int       y = 0, m = 0, d = 0, H = 0, M = 0, S = 0;
    struct tm timeinfo = { 0 };
    bool      found    = false;

    // YYYY-MM-DD or YYYY-MM-DDTHH:mm:ss or YYYY-MM-DD HH:mm:ss
    // We try multiple patterns. sscanf returns number of items assigned.
    if (sscanf(dateStr, "%d-%d-%d %d:%d:%d", &y, &m, &d, &H, &M, &S) >= 3) {
        found = true;
    } else if (sscanf(dateStr, "%d-%d-%dT%d:%d:%d", &y, &m, &d, &H, &M, &S) >= 3) {
        found = true;
    }
    // YYYY/MM/DD
    else if (sscanf(dateStr, "%d/%d/%d %d:%d:%d", &y, &m, &d, &H, &M, &S) >= 3) {
        found = true;
    }
    // MM/DD/YYYY
    else if (sscanf(dateStr, "%d/%d/%d %d:%d:%d", &m, &d, &y, &H, &M, &S) >= 3) {
        found = true;
    }

    if (found) {
        timeinfo.tm_year  = y - 1900;
        timeinfo.tm_mon   = m - 1;
        timeinfo.tm_mday  = d;
        timeinfo.tm_hour  = H;
        timeinfo.tm_min   = M;
        timeinfo.tm_sec   = S;
        timeinfo.tm_isdst = -1;  // Let mktime determine DST

        time_t raw = mktime(&timeinfo);
        if (raw != (time_t) -1) {
            return (double) raw * 1000.0;
        }
    }

    return 0.0;  // Invalid/Empty
}

static Value* _DateInit(Interpreter* interpreter, int argc, Value** arguments) {
    Value*         thisArg = arguments[0];
    ClassInstance* cls     = CoerceToClassInstance(thisArg);

    double timestamp = 0.0;

    if (argc == 1) {
        // new Date() -> current time
        timestamp = (double) time(NULL) * 1000.0;
    } else if (argc == 2) {
        // new Date(value) or new Date(dateString)
        Value* arg = arguments[1];
        if (ValueIsStr(arg)) {
            Rune*  runes = (Rune*) arg->Value.Opaque;
            String str   = RunesStrToString(runes);
            timestamp    = _ParseDateString(str);
            free(str);
        } else {
            timestamp = CoerceToNum(arg);
        }
    } else if (argc >= 3) {
        // new Date(year, month, [date, hours, minutes, seconds, ms])
        // Note: argc includes 'this', so arguments are at 1, 2, ...
        struct tm timeinfo = { 0 };

        // Year
        timeinfo.tm_year = (int) CoerceToNum(arguments[1]) - 1900;
        // Month (0-11)
        timeinfo.tm_mon = (int) CoerceToNum(arguments[2]);
        // Day (default 1)
        timeinfo.tm_mday = (argc > 3) ? (int) CoerceToNum(arguments[3]) : 1;
        // Hours
        timeinfo.tm_hour = (argc > 4) ? (int) CoerceToNum(arguments[4]) : 0;
        // Minutes
        timeinfo.tm_min = (argc > 5) ? (int) CoerceToNum(arguments[5]) : 0;
        // Seconds
        timeinfo.tm_sec = (argc > 6) ? (int) CoerceToNum(arguments[6]) : 0;

        // Make time returns seconds
        time_t rawtime = mktime(&timeinfo);
        timestamp      = (double) rawtime * 1000.0;

        // Add milliseconds if provided
        if (argc > 7) {
            timestamp += CoerceToNum(arguments[7]);
        }
    }

    _SetTimestamp(interpreter, cls, timestamp);
    return interpreter->Null;
}

static Value* _DateGetFullYear(Interpreter* interpreter, int argc, Value** arguments) {
    ClassInstance* cls      = CoerceToClassInstance(arguments[0]);
    double         ts       = _GetTimestamp(cls);
    time_t         rawtime  = (time_t) (ts / 1000.0);
    struct tm*     timeinfo = localtime(&rawtime);
    if (!timeinfo)
        return NewIntValue(interpreter, 0);
    return NewIntValue(interpreter, timeinfo->tm_year + 1900);
}

static Value* _DateGetMonth(Interpreter* interpreter, int argc, Value** arguments) {
    ClassInstance* cls      = CoerceToClassInstance(arguments[0]);
    double         ts       = _GetTimestamp(cls);
    time_t         rawtime  = (time_t) (ts / 1000.0);
    struct tm*     timeinfo = localtime(&rawtime);
    if (!timeinfo)
        return NewIntValue(interpreter, 0);
    return NewIntValue(interpreter, timeinfo->tm_mon);
}

static Value* _DateGetDate(Interpreter* interpreter, int argc, Value** arguments) {
    ClassInstance* cls      = CoerceToClassInstance(arguments[0]);
    double         ts       = _GetTimestamp(cls);
    time_t         rawtime  = (time_t) (ts / 1000.0);
    struct tm*     timeinfo = localtime(&rawtime);
    if (!timeinfo)
        return NewIntValue(interpreter, 0);
    return NewIntValue(interpreter, timeinfo->tm_mday);
}

static Value* _DateGetDay(Interpreter* interpreter, int argc, Value** arguments) {
    ClassInstance* cls      = CoerceToClassInstance(arguments[0]);
    double         ts       = _GetTimestamp(cls);
    time_t         rawtime  = (time_t) (ts / 1000.0);
    struct tm*     timeinfo = localtime(&rawtime);
    if (!timeinfo)
        return NewIntValue(interpreter, 0);
    return NewIntValue(interpreter, timeinfo->tm_wday);
}

static Value* _DateGetHours(Interpreter* interpreter, int argc, Value** arguments) {
    ClassInstance* cls      = CoerceToClassInstance(arguments[0]);
    double         ts       = _GetTimestamp(cls);
    time_t         rawtime  = (time_t) (ts / 1000.0);
    struct tm*     timeinfo = localtime(&rawtime);
    if (!timeinfo)
        return NewIntValue(interpreter, 0);
    return NewIntValue(interpreter, timeinfo->tm_hour);
}

static Value* _DateGetMinutes(Interpreter* interpreter, int argc, Value** arguments) {
    ClassInstance* cls      = CoerceToClassInstance(arguments[0]);
    double         ts       = _GetTimestamp(cls);
    time_t         rawtime  = (time_t) (ts / 1000.0);
    struct tm*     timeinfo = localtime(&rawtime);
    if (!timeinfo)
        return NewIntValue(interpreter, 0);
    return NewIntValue(interpreter, timeinfo->tm_min);
}

static Value* _DateGetSeconds(Interpreter* interpreter, int argc, Value** arguments) {
    ClassInstance* cls      = CoerceToClassInstance(arguments[0]);
    double         ts       = _GetTimestamp(cls);
    time_t         rawtime  = (time_t) (ts / 1000.0);
    struct tm*     timeinfo = localtime(&rawtime);
    if (!timeinfo)
        return NewIntValue(interpreter, 0);
    return NewIntValue(interpreter, timeinfo->tm_sec);
}

static Value* _DateGetTime(Interpreter* interpreter, int argc, Value** arguments) {
    ClassInstance* cls = CoerceToClassInstance(arguments[0]);
    double         ts  = _GetTimestamp(cls);
    return NewNumValue(interpreter, ts);
}

static Value* _DateToString(Interpreter* interpreter, int argc, Value** arguments) {
    ClassInstance* cls     = CoerceToClassInstance(arguments[0]);
    double         ts      = _GetTimestamp(cls);
    time_t         rawtime = (time_t) (ts / 1000.0);
    char           buffer[128];
    // Example: "Sat Jan 17 2026 12:34:56"
    // ctime includes newline, so we might want to manually format or strip it
    String tstr = ctime(&rawtime);
    if (tstr) {
        size_t len = strlen(tstr);
        if (len > 0 && tstr[len - 1] == '\n')
            tstr[len - 1] = '\0';

        return NewStrValue(interpreter, tstr);
    }

    return NewStrValue(interpreter, "Invalid Date");
}

static ModuleFunction _DateClassMethods[] = {
    // Date class
    { .Name      = CONSTRUCTOR_NAME,
      .Argc      = VARARG,
      .CFunction = (NativeFunctionCallback) _DateInit,
      .Value     = NULL },
    { .Name      = "getFullYear",
      .Argc      = 1,
      .CFunction = (NativeFunctionCallback) _DateGetFullYear,
      .Value     = NULL },
    { .Name      = "getMonth",
      .Argc      = 1,
      .CFunction = (NativeFunctionCallback) _DateGetMonth,
      .Value     = NULL },
    { .Name      = "getDate",
      .Argc      = 1,
      .CFunction = (NativeFunctionCallback) _DateGetDate,
      .Value     = NULL },
    { .Name      = "getDay",
      .Argc      = 1,
      .CFunction = (NativeFunctionCallback) _DateGetDay,
      .Value     = NULL },
    { .Name      = "getHours",
      .Argc      = 1,
      .CFunction = (NativeFunctionCallback) _DateGetHours,
      .Value     = NULL },
    { .Name      = "getMinutes",
      .Argc      = 1,
      .CFunction = (NativeFunctionCallback) _DateGetMinutes,
      .Value     = NULL },
    { .Name      = "getSeconds",
      .Argc      = 1,
      .CFunction = (NativeFunctionCallback) _DateGetSeconds,
      .Value     = NULL },
    { .Name      = "getTime",
      .Argc      = 1,
      .CFunction = (NativeFunctionCallback) _DateGetTime,
      .Value     = NULL },
    { .Name      = "toString",
      .Argc      = 1,
      .CFunction = (NativeFunctionCallback) _DateToString,
      .Value     = NULL },
    // end of module functions
    { .Name = NULL }
};

Value* CreateDateClass(Interpreter* interpreter) {
    Value* dateClass = NewClassValue(interpreter, CreateUserClass("Date", NULL));
    Class* cls       = CoerceToUserClass(dateClass);

    // Register methods
    for (int i = 0; _DateClassMethods[i].Name != NULL; i++) {
        ModuleFunction* func = &_DateClassMethods[i];
        String          hKey = func->Name;

        if (func->CFunction) {
            ClassDefineMemberByString(
                cls,
                hKey,
                NewNativeFunctionValue(
                    interpreter,
                    CreateNativeFunctionMeta((String) hKey, func->Argc, func->CFunction)),
                false);
        }
    }

    return dateClass;
}

Value* LoadCoreDate(Interpreter* interpreter) {
    Value* val = (interpreter->Date != NULL) ? interpreter->Date
                                             : (interpreter->Date = CreateDateClass(interpreter));

    Value*   module = NewObjectValue(interpreter);
    HashMap* map    = CoerceToHashMap(module);

    HashMapSet(map, "Date", val);
    return module;
}
