
#include "./loader.h"

CoreMapper _CoreModuleMappers[] = {
	{ .Name = "io", .Loader = LoadCoreIo },
	{ .Name = "os", .Loader = LoadCoreOs },
	{ .Name = "math", .Loader = LoadCoreMath },
	{ .Name = "date", .Loader = LoadCoreDate },
	{ .Name = "object", .Loader = LoadCoreObject },
	{ .Name = "array", .Loader = LoadCoreArray },
	{ .Name = "promise", .Loader = LoadCorePromise },
	{ .Name = "mongoose", .Loader = LoadCoreMongoose },
// Do not map these library on minimal builds to save space, and because they
// are not commonly
#ifndef ZSMINIMAL
	{ .Name = "sqlite", .Loader = LoadCoreSqlite },
#endif
	// End marker
	{ .Name = NULL, .Loader = NULL }
};

Value* LoadCoreModule(Interpreter* interpreter, String moduleName) {
	for (int i = 0; _CoreModuleMappers[i].Name != NULL; i++) {
		if (strcmp(_CoreModuleMappers[i].Name, moduleName) == 0) {
			return _CoreModuleMappers[i].Loader(interpreter);
		}
	}
	return NULL;
}