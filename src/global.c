#include "./global.h"

void* _Allocate(String file, int line, size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "[%s:%d] Failed to allocate memory!!!\n", file, line);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* _Callocate(String file, int line, size_t count, size_t size) {
    void* ptr = calloc(count, size);
    if (ptr == NULL) {
        fprintf(stderr, "[%s:%d] Failed to allocate memory with Callocate!!!\n", file, line);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* _Reallocate(String file, int line, void* ptr, size_t size) {
    void* newPtr = realloc(ptr, size);
    if (newPtr == NULL) {
        fprintf(stderr, "[%s:%d] Failed to reallocate memory!!!\n", file, line);
        exit(EXIT_FAILURE);
    }
    return newPtr;
}

String AllocateString(String str) {
    String ptr = Allocate(strlen(str) + 1);
    strcpy(ptr, str);
    return ptr;
}

Rune* StringToRunes(String str) {
    if (str == NULL) {
        return NULL;
    }

    // Get the number of UTF-8 characters (runes) in the string
    size_t runeCount = utf_length(str);

    // Allocate array for runes + null terminator
    Rune* runes = Allocate(sizeof(Rune) * (runeCount + 1));

    // Convert each UTF-8 character to a rune
    for (size_t i = 0; i < runeCount; i++) {
        runes[i] = utf_char_code_at(str, i);
    }

    // Null-terminate the rune array
    runes[runeCount] = 0;

    return runes;
}

String RunesStrToString(Rune* runes) {
    if (runes == NULL) {
        return NULL;
    }

    // Count the number of runes
    size_t runeCount = 0;
    while (runes[runeCount] != 0) {
        runeCount++;
    }

    // Calculate total size needed for UTF-8 encoded string
    size_t totalSize = 0;
    for (size_t i = 0; i < runeCount; i++) {
        totalSize += utf_size_of_codepoint(runes[i]);
    }

    // Allocate string buffer
    String str = Allocate(totalSize + 1);

    // Convert each rune to UTF-8 and append to string
    size_t offset = 0;
    for (size_t i = 0; i < runeCount; i++) {
        String runeStr = utf_rune_to_string(runes[i]);
        size_t runeLen = strlen(runeStr);
        memcpy(str + offset, runeStr, runeLen);
        offset += runeLen;
        free(runeStr);
    }

    str[totalSize] = '\0';
    return str;
}

bool StringStartsWith(String str, String prefix) {
    if (str == NULL || prefix == NULL) {
        return false;
    }
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

int CoerceToI32(Value* value) {
    switch (value->Type) {
        case VLT_INT:
            return value->Value.I32;
        case VLT_NUM:
            return (int) value->Value.Num;
        case VLT_BINT:
        case VLT_BNUM:
            {
                bf_t* bf  = (bf_t*) value->Value.Opaque;
                int   num = 0;
                bf_get_int32(&num, bf, BF_RNDZ);
                return num;
            }
        default:
            return 0;
    }
}

long CoerceToI64(Value* value) {
    switch (value->Type) {
        case VLT_INT:
            return (long) value->Value.I32;
        case VLT_NUM:
            return (long) value->Value.Num;
        case VLT_BINT:
        case VLT_BNUM:
            {
                bf_t*   bf  = (bf_t*) value->Value.Opaque;
                int64_t num = 0;
                bf_get_int64(&num, bf, BF_RNDZ);
                return num;
            }
        default:
            return 0;
    }
}

double CoerceToNum(Value* value) {
    switch (value->Type) {
        case VLT_INT:
            return (double) value->Value.I32;
        case VLT_NUM:
            return (double) value->Value.Num;
        case VLT_BINT:
        case VLT_BNUM:
            {
                bf_t*  bf  = (bf_t*) value->Value.Opaque;
                double num = 0;
                bf_get_float64(bf, &num, BF_RNDNA);
                return num;
            }
        default:
            return 0.0;
    }
}

bf_t* CoerceToBitField(Interpreter* interp, Value* value) {
    switch (value->Type) {
        case VLT_INT:
            {
                bf_t* bf = Allocate(sizeof(bf_t));
                bf_init(&interp->BfContext, bf);
                bf_set_si(bf, (long) value->Value.I32);
                return bf;
            }
        case VLT_NUM:
            {
                bf_t* bf = Allocate(sizeof(bf_t));
                bf_init(&interp->BfContext, bf);
                bf_set_float64(bf, value->Value.Num);
                return bf;
            }
        case VLT_BINT:
        case VLT_BNUM:
            {
                return (bf_t*) value->Value.Opaque;
            }
        default:
            return NULL;
    }
}

limb_t BFPrecession(Value* value) {
    switch (value->Type) {
        case VLT_INT:
            return PREC_INT;
        case VLT_NUM:
            return PREC_DBL;
        case VLT_BINT:
            {
                bf_t* bf = (bf_t*) value->Value.Opaque;
                return bf->expn == 0 ? PREC_INT : PREC_NUM(bf->len);
            }
        case VLT_BNUM:
            {
                bf_t* bf = (bf_t*) value->Value.Opaque;
                return bf->expn == 0 ? PREC_INT : PREC_NUM(bf->len);
            }
        default:
            return 0;
    }
}

bool CoerceToBool(Value* value) {
    switch (value->Type) {
        case VLT_ERROR:
            return false;
        case VLT_INT:
            return value->Value.I32 != 0;
        case VLT_BINT:
            return !bf_is_zero((bf_t*) value->Value.Opaque);
        case VLT_BNUM:
            return !bf_is_zero((bf_t*) value->Value.Opaque);
        case VLT_NUM:
            return value->Value.Num != 0.0;
        case VLT_STR:
            return strlen(value->Value.Opaque) > 0;
        case VLT_BOOL:
            return !!(value->Value.I32);
        case VLT_NULL:
            return false;
        case VLT_USER_FUNCTION:
        case VLT_NATV_FUNCTION:
        case VLT_ENVIRONMENT:
            return true;
        case VLT_ARRAY:
            return (CoerceToArray(value)->Count > 0);
        case VLT_OBJECT:
            return (CoerceToHashMap(value)->Count > 0);
        case VLT_CLASS:
        case VLT_CLASS_INSTANCE:
            return true;
        default:
            printf("CoerceToBool: Unknown value type: %d\n", value->Type);
            return false;
    }
}

Environment* CoerceToEnvironment(Value* value) {
    if (value == NULL)
        return NULL;
    if (value->Type == VLT_ENVIRONMENT) {
        return (Environment*) value->Value.Opaque;
    }
    Panic("Value is not an Environment");
}

HashMap* CoerceToHashMap(Value* value) {
    if (value == NULL)
        return NULL;
    if (value->Type == VLT_OBJECT) {
        return (HashMap*) value->Value.Opaque;
    }
    Panic("Value is not a HashMap");
}

Array* CoerceToArray(Value* value) {
    if (value == NULL)
        return NULL;
    if (value->Type == VLT_ARRAY) {
        return (Array*) value->Value.Opaque;
    }
    Panic("Value is not an Array");
}

UserFunction* CoerceToUserFunction(Value* value) {
    if (value == NULL)
        return NULL;
    if (value->Type == VLT_USER_FUNCTION) {
        return (UserFunction*) value->Value.Opaque;
    }
    Panic("Value is not a UserFunction");
}

NativeFunction* CoerceToNativeFunction(Value* value) {
    if (value == NULL)
        return NULL;
    if (value->Type == VLT_NATV_FUNCTION) {
        return (NativeFunction*) value->Value.Opaque;
    }
    Panic("Value is not a NativeFunction");
}

Class* CoerceToUserClass(Value* value) {
    if (value == NULL)
        return NULL;
    if (value->Type == VLT_CLASS) {
        return (Class*) value->Value.Opaque;
    }
    Panic("Value is not a Class");
}

ClassInstance* CoerceToClassInstance(Value* value) {
    if (value == NULL)
        return NULL;
    if (value->Type == VLT_CLASS_INSTANCE) {
        return (ClassInstance*) value->Value.Opaque;
    }
    Panic("Value is not a ClassInstance");
}

StateMachine* CoerceToStateMachine(Value* value) {
    if (value == NULL)
        return NULL;
    if (value->Type == VLT_PROMISE) {
        return (StateMachine*) value->Value.Opaque;
    }
    Panic("Value is not a Promise/StateMachine");
}

String GetErrorLine(String path, Rune* runes, Position position, String message) {
    if (runes == NULL) {
        return NULL;
    }

#define PADDING        3
#define MAX_LINE_WIDTH 1000
#define LINE_NUM_WIDTH 4

    // Count total lines and find line start positions
    int  lineCount  = 1;
    int  maxLines   = 1000;  // Initial capacity
    int* lineStarts = Allocate(sizeof(int) * maxLines);
    lineStarts[0]   = 0;     // First line starts at index 0

    // Scan through runes to find newline positions
    for (int i = 0; runes[i] != 0; i++) {
        if (runes[i] == '\n') {
            if (lineCount >= maxLines) {
                maxLines           *= 2;
                int* newLineStarts  = Allocate(sizeof(int) * maxLines);
                memcpy(newLineStarts, lineStarts, sizeof(int) * lineCount);
                free(lineStarts);
                lineStarts = newLineStarts;
            }
            lineStarts[lineCount] = i + 1;
            lineCount++;
        }
    }

    // Calculate the range of lines to display (0-based internally)
    int startLine = (position.LineStart - PADDING > 1) ? position.LineStart - PADDING : 1;
    int endLine =
        (position.LineStart + PADDING < lineCount) ? position.LineStart + PADDING : lineCount;

    // Build the error message string
    size_t bufferSize = 4096;
    String result     = Allocate(bufferSize);
    size_t currentPos = 0;

    // Add error header
    currentPos += snprintf(result + currentPos,
                           bufferSize - currentPos,
                           "Error in [%s:%d:%d] %s\n\n",
                           path,
                           position.LineStart,
                           position.ColmStart,
                           message);

    // Display each line in the range
    for (int line = startLine; line <= endLine; line++) {
        int lineStartIdx = lineStarts[line - 1];
        int lineEndIdx   = (line < lineCount) ? lineStarts[line] - 1 : lineStartIdx;

        // Find actual end of line (excluding newline)
        while (lineEndIdx > lineStartIdx && runes[lineEndIdx] == '\n') {
            lineEndIdx--;
        }

        // Find the actual end of the line (scan until newline or null)
        int actualLineEnd = lineStartIdx;
        while (runes[actualLineEnd] != '\n' && runes[actualLineEnd] != 0) {
            actualLineEnd++;
        }
        actualLineEnd--;  // Back up to last character before newline/null

        // Convert runes to string for this line
        char lineBuffer[MAX_LINE_WIDTH];
        int  bufferIdx = 0;

        for (int i = lineStartIdx; i <= actualLineEnd && runes[i] != '\n' && runes[i] != 0; i++) {
            unsigned char utf8Buffer[5];
            int           utf8Size = utf_encode_char(runes[i], utf8Buffer);

            for (int j = 0; j < utf8Size && bufferIdx < MAX_LINE_WIDTH - 5; j++) {
                lineBuffer[bufferIdx++] = utf8Buffer[j];
            }
        }
        lineBuffer[bufferIdx] = '\0';

        // Ensure buffer is large enough
        size_t needed = currentPos + LINE_NUM_WIDTH + strlen(lineBuffer) + 200;
        if (needed > bufferSize) {
            bufferSize       = needed * 2;
            String newResult = Allocate(bufferSize);
            memcpy(newResult, result, currentPos);
            free(result);
            result = newResult;
        }

        // Print line number and content
        currentPos +=
            snprintf(result + currentPos, bufferSize - currentPos, "%4d | %s\n", line, lineBuffer);

        // Add error highlighting if this is the error line
        if (line == position.LineStart) {
            // Add error indicator line
            currentPos += snprintf(result + currentPos, bufferSize - currentPos, "%4s | ", "");

            // Convert to 0-based indexing
            int colStart = position.ColmStart - 1;
            int colEnd   = position.ColmEnded - 1;

            // Bounds checking
            if (colStart < 0)
                colStart = 0;
            if (colEnd < colStart)
                colEnd = colStart;

            int lineLength = strlen(lineBuffer);
            if (colEnd >= lineLength)
                colEnd = lineLength - 1;
            if (colEnd < 0)
                colEnd = 0;

            // Add spaces up to the error column
            for (int col = 0; col < colStart; col++) {
                result[currentPos++] = ' ';
            }

            // Add error carets
            int errorLength = colEnd - colStart + 1;
            if (errorLength < 1)
                errorLength = 1;

            for (int i = 0; i < errorLength; i++) {
                result[currentPos++] = '^';
            }

            result[currentPos++] = '\n';
            result[currentPos]   = '\0';
        }
    }

    free(lineStarts);

#undef PADDING
#undef MAX_LINE_WIDTH
#undef LINE_NUM_WIDTH

    return result;
}

void ThrowError(String path, Rune* runes, Position position, String message) {
    String errorLine = GetErrorLine(path, runes, position, message);
    fprintf(stderr, "%s", errorLine);
    free(errorLine);
    exit(EXIT_FAILURE);
}

String FormatString(String format, ...) {
    va_list args;
    va_start(args, format);

    // Determine required buffer size
    int size = vsnprintf(NULL, 0, format, args) + 1;  // +1 for null terminator
    va_end(args);

    // Allocate buffer
    String buffer = Allocate(size);

    // Write formatted string to buffer
    va_start(args, format);
    vsnprintf(buffer, size, format, args);
    va_end(args);

    return buffer;
}

String BFIntToString(bf_t* value) {
    String str =
        bf_ftoa(NULL, value, 10, PREC_INT, BF_RNDZ | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
    if (str == NULL) {
        Panic("Failed to convert big integer to string");
    }
    String formattedStr = FormatString("%sn", str);
    free(str);
    return formattedStr;
}

String BFNumToString(bf_t* value) {
    String str = bf_ftoa(NULL,
                         value,
                         10,
                         PREC_NUM(value->len),
                         BF_RNDZ | BF_FTOA_FORMAT_FREE_MIN | BF_FTOA_JS_QUIRKS);
    if (str == NULL) {
        Panic("Failed to convert big number to string");
    }
    String formattedStr = FormatString("%sn", str);
    free(str);
    return formattedStr;
}

#ifdef _WIN32
#    define getcwd         _getcwd
#    define PATH_SEPARATOR "\\"
#else
#    define PATH_SEPARATOR "/"
#endif

bool IsAbsolutePath(String path) {
    if (!path || path[0] == '\0')
        return false;

#ifdef _WIN32
    // Windows: "C:\" or "\\server"
    if (isalpha(path[0]) && path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
        return true;
    if ((path[0] == '\\' && path[1] == '\\') || (path[0] == '/' && path[1] == '/'))
        return true;
#else
    // POSIX: "/usr/bin"
    if (path[0] == '/')
        return true;
#endif

    return false;
}

String NormalizePath(String pathStr) {
    if (!pathStr || pathStr[0] == '\0') {
        return AllocateString(".");
    }

    bool is_abs = IsAbsolutePath(pathStr);

    // Create a mutable copy for tokenization
    String temp = Allocate(strlen(pathStr) + 1);
    strcpy(temp, pathStr);

    // Stack to hold the valid path components
    String stack[256];
    int    top = 0;

    // Tokenize the path by slashes
    String token = strtok(temp, "/\\");
    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            // Ignore "."
        } else if (strcmp(token, "..") == 0) {
            if (top > 0 && strcmp(stack[top - 1], "..") != 0) {
                // Pop the last directory off the stack
                top--;
            } else if (!is_abs) {
                // If it's a relative path and we are at the top, we must keep ".."
                stack[top++] = token;
            }
            // If it's absolute and top == 0, we can't go above root, so do nothing.
        } else {
            // Normal directory or file name
            stack[top++] = token;
        }
        token = strtok(NULL, "/\\");
    }

    // Allocate memory for the normalized result
    String result = Allocate(strlen(pathStr) + 2);  // +2 for potential root slash and null
    result[0]     = '\0';

    // Handle root for absolute paths
    if (is_abs) {
#ifdef _WIN32
        // If it was a drive letter (e.g., C:\), strtok leaves "C:" in stack[0]
        if (!(isalpha(pathStr[0]) && pathStr[1] == ':')) {
            strcpy(result, PATH_SEPARATOR);  // UNC or root slash
        }
#else
        strcpy(result, "/");
#endif
    }

    // Reconstruct the path from the stack
    for (int i = 0; i < top; i++) {
        strcat(result, stack[i]);
        if (i < top - 1) {
            strcat(result, PATH_SEPARATOR);
        }
    }

    // Edge cases (e.g., path was just "." or "/")
    if (result[0] == '\0') {
        strcpy(result, is_abs ? PATH_SEPARATOR : ".");
    }
#ifdef _WIN32
    // Ensure "C:" becomes "C:\" if it's the only thing left
    else if (isalpha(result[0]) && result[1] == ':' && result[2] == '\0') {
        strcat(result, PATH_SEPARATOR);
    }
#endif

    free(temp);
    return result;
}

String Basename(String pathStr) {
    if (!pathStr || pathStr[0] == '\0') {
        return AllocateString(".");
    }

    // Find the last separator
    String lastSep = NULL;
    for (int i = (int) strlen(pathStr) - 1; i >= 0; i--) {
        if (pathStr[i] == '/' || pathStr[i] == '\\') {
            lastSep = &pathStr[i];
            break;
        }
    }

    // Get the filename part (after last separator, or the whole string)
    String filename = lastSep ? lastSep + 1 : pathStr;

    // Find the last dot to strip the extension
    String lastDot = strrchr(filename, '.');
    if (lastDot && lastDot != filename) {
        size_t len    = lastDot - filename;
        String result = Allocate(len + 1);
        memcpy(result, filename, len);
        result[len] = '\0';
        return result;
    }

    return AllocateString(filename);
}

String Dirname(String pathStr) {
    // If path is empty or null, standard behavior is to return "." (current dir)
    if (!pathStr || pathStr[0] == '\0') {
        String dot = Allocate(2);
        strcpy(dot, ".");
        return dot;
    }

    // Work on a copy of the string so we don't mutate the user's input
    size_t len  = strlen(pathStr);
    String path = Allocate(len + 1);
    if (!path)
        return NULL;
    strcpy(path, pathStr);

    // 1. Strip any trailing separators (e.g., "src/folder/" -> "src/folder")
    // But don't strip if the string is just the root "/"
    while (len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
        path[len - 1] = '\0';
        len--;
    }

    // 2. Find the last separator
    String last_sep = NULL;
    for (int i = (int) len - 1; i >= 0; i--) {
        if (path[i] == '/' || path[i] == '\\') {
            last_sep = &path[i];
            break;
        }
    }

    // 3. No separator found? It's just a file name. Return "."
    if (!last_sep) {
        free(path);
        String dot = Allocate(2);
        strcpy(dot, ".");
        return dot;
    }

    // 4. If the separator is at the very beginning (e.g., "/file.txt")
    if (last_sep == path) {
        // Keep the root slash, terminate right after it
        path[1] = '\0';
    } else {
        // Separator is in the middle. Strip any sequence of duplicate slashes.
        while (last_sep > path && (*(last_sep - 1) == '/' || *(last_sep - 1) == '\\')) {
            last_sep--;
        }
        *last_sep = '\0';  // Terminate the string here to drop the basename
    }

    return path;
}

String AbsolutePathFromBase(String baseStr, String pathStr) {
    if (!pathStr || pathStr[0] == '\0') {
        return NULL;
    }

    // 1. If the path is already absolute, the base is irrelevant.
    if (IsAbsolutePath(pathStr)) {
        String abs_copy = Allocate(strlen(pathStr) + 1);
        strcpy(abs_copy, pathStr);
        String normalized_path = NormalizePath(abs_copy);
        free(abs_copy);
        return normalized_path;
    }

    // 2. If the base is empty, just return the path (or you could fallback to CWD)
    if (!baseStr || baseStr[0] == '\0') {
        String path_copy = Allocate(strlen(pathStr) + 1);
        strcpy(path_copy, pathStr);
        String normalized_path = NormalizePath(path_copy);
        free(path_copy);
        return normalized_path;
    }

    // 3. Prepare for concatenation
    size_t base_len        = strlen(baseStr);
    bool   needs_separator = true;

    // Check if base already ends with a separator
    char last_char = baseStr[base_len - 1];
    if (last_char == '/' || last_char == '\\') {
        needs_separator = false;
    }

    // Skip leading separators in the target path to avoid double slashes (e.g., "base//" "path")
    String p = pathStr;
    while (p[0] == '/' || p[0] == '\\') {
        p++;
    }

    // 4. Allocate and build the new path
    size_t total_len     = base_len + (needs_separator ? 1 : 0) + strlen(p) + 1;
    String resolved_path = Allocate(total_len);
    if (!resolved_path)
        return NULL;

    strcpy(resolved_path, baseStr);
    if (needs_separator) {
        strcat(resolved_path, PATH_SEPARATOR);
    }
    strcat(resolved_path, p);

    String normalized_path = NormalizePath(resolved_path);
    free(resolved_path);

    return normalized_path;
}

String AbsolutePath(String pathStr) {
    String path = pathStr;

    if (!path || path[0] == '\0') {
        return NULL;
    }

    // 1. Check if the path is already absolute
    if (IsAbsolutePath(path)) {
        return NormalizePath(pathStr);
    }

    // 2. Get the Current Working Directory
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return NULL;
    }

    // 3. Resolve the path with the CWD
    size_t cwd_len = strlen(cwd);

    bool needs_separator = true;
    if (cwd_len > 0) {
        char last_char = cwd[cwd_len - 1];
        if (last_char == '/' || last_char == '\\') {
            needs_separator = false;
        }
    }

    // Skip leading separators in the relative path to avoid double separators
    while (path[0] == '/' || path[0] == '\\') {
        path++;
    }

    // Allocate memory for: CWD + Separator + Path + Null Terminator
    size_t total_len         = cwd_len + (needs_separator ? 1 : 0) + strlen(path) + 1;
    String resolved_raw_path = Allocate(total_len);

    if (!resolved_raw_path) {
        return NULL;  // Memory allocation failed
    }

    // Assemble the final string
    strcpy(resolved_raw_path, cwd);
    if (needs_separator) {
        strcat(resolved_raw_path, PATH_SEPARATOR);
    }
    strcat(resolved_raw_path, path);

    // 4. Convert back to your custom String type
    String final_path = NormalizePath(resolved_raw_path);
    free(resolved_raw_path);
    return final_path;
}