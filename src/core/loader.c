#include "./loader.h"


CoreMapper _CoreModuleMappers[] = {
    { .Name = "io"  , .Loader = LoadCoreIo   },
    { .Name = "math", .Loader = LoadCoreMath },
    { .Name = "Date", .Loader = LoadCoreDate },
    // End marker
    { .Name = NULL  , .Loader = NULL         }
};


Value* LoadCoreModule(Interpreter* interpreter, String moduleName) {
    for (int i = 0; _CoreModuleMappers[i].Name != NULL; i++) {
        if (strcmp(_CoreModuleMappers[i].Name, moduleName) == 0) {
            return _CoreModuleMappers[i].Loader(interpreter);
        }
    }
    return NULL;
}