
#include "./loader.h"

CoreMapper _CoreModuleMappers[] = {
	{ .Name = "array", .Loader = LoadCoreArray },
	{ .Name = "blob", .Loader = LoadCoreBlob },
	{ .Name = "crypto", .Loader = LoadCoreCrypto },
	{ .Name = "date", .Loader = LoadCoreDate },
	{ .Name = "io", .Loader = LoadCoreIo },
	{ .Name = "json", .Loader = LoadCoreJson },
	{ .Name = "math", .Loader = LoadCoreMath },
	{ .Name = "mysql", .Loader = LoadCoreMysql },
	{ .Name = "mongoose", .Loader = LoadCoreMongoose },
	{ .Name = "object", .Loader = LoadCoreObject },
	{ .Name = "os", .Loader = LoadCoreOs },
	{ .Name = "path", .Loader = LoadCorePath },
	{ .Name = "promise", .Loader = LoadCorePromise },
	{ .Name = "regex", .Loader = LoadCoreRegex },
	{ .Name = "sqlite", .Loader = LoadCoreSqlite },
	{ .Name = "string", .Loader = LoadCoreString },
	{ .Name = "utf8", .Loader = LoadCoreUtf8 },
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