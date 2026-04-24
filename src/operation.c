#include "./operation.h"

#include "global.h"

#define FreeTempBf(interpreter, bf, val)                                       \
	do {                                                                       \
		if ((val)->Type == VLT_INT || (val)->Type == VLT_NUM) {                \
			bf_delete(bf);                                                     \
			free(bf);                                                          \
		}                                                                      \
	} while (0)

#define PushArray(type, array, count, val, defaultValue)                        \
	do {                                                                        \
		(array)[(count)++] = val;                                               \
		(array)			   = Reallocate((array), sizeof(type) * ((count) + 1)); \
		(array)[(count)]   = (defaultValue);                                    \
	} while (0)

#define GetOffset() (interpreter->ConstantC)

/**
 * @brief Stores function in interpreter->ActiveFunction so diagnostics and
 * nested machinery know the current callee.
 * @param interpreter VM state field being updated.
 * @param function Value recorded as active; may be NULL when clearing.
 * @origin src/interpreter.c
 */
extern void SetActiveFunction(Interpreter* interpreter, Value* function);

/**
 * @brief Stores task in interpreter->ActiveTask while promise/async machinery
 * runs nested work.
 * @param interpreter VM state field being updated.
 * @param task Promise Value (or NULL) considered the currently executing async
 * task.
 * @origin src/interpreter.c
 */
extern void SetActiveTask(Interpreter* interpreter, Value* task);

/**
 * @brief Appends value at interpreter->Stacks[StckC++] so it becomes the new
 * operand-stack top.
 * @param interpreter VM whose StckC indexes the next free stack slot.
 * @param value Pointer stored on the stack; lifetime must cover the span it
 * remains reachable from the stack.
 * @origin src/interpreter.c
 */
extern void Push(Interpreter* interpreter, Value* value);

/**
 * @brief Returns interpreter->Stacks[--StckC], removing one slot from the
 * logical operand stack.
 * @param interpreter VM whose StckC must be > 0; otherwise behaviour is
 * undefined.
 * @return The Value* that was previously the stack top.
 * @origin src/interpreter.c
 */
extern Value* Popp(Interpreter* interpreter);

/**
 * @brief Lowers StckC by n without clearing slots, shrinking the logical stack
 * height after bulk operand drops.
 * @param interpreter VM whose stack depth must be at least n; otherwise
 * behaviour is undefined.
 * @param n Number of operand slots to discard from the top.
 * @origin src/interpreter.c
 */
extern void PopN(Interpreter* interpreter, int n);

/**
 * @brief Reads interpreter->Stacks[StckC - 1] without changing StckC
 * (non-destructive top-of-stack).
 * @param interpreter VM whose StckC must be > 0; otherwise behaviour is
 * undefined.
 * @return Current top operand without popping it.
 * @origin src/interpreter.c
 */
extern Value* Peek(Interpreter* interpreter);

static int _GetConstantOffset(Interpreter* interpreter, Value* value) {
	if (value == NULL) {
		goto BAD;
	}
	String valueStr = ValueToString(value), constantStr = NULL;
	for (int i = 0; i < interpreter->ConstantC; i++) {
		if (interpreter->Constants[i] != NULL) {
			constantStr = ValueToString(interpreter->Constants[i]);
			if (constantStr != NULL && strcmp(constantStr, valueStr) == 0) {
				free(constantStr);
				free(valueStr);
				return i;
			}
			if (constantStr != NULL) {
				free(constantStr);
			}
		}
	}
	free(valueStr);
BAD:;
	return -1;
}

static void _DupTop(Interpreter* interpreter) {
	Push(interpreter, Peek(interpreter));
}

/**
 * @brief Main opcode dispatch loop: executes a UserFunction bytecode stream or
 * resumes a StateMachine until return, error, or await boundary.
 * @param interpreter VM providing stacks, environments, and globals for opcode
 * handlers.
 * @param fnOrSm Either a VLT_USER_FUNCTION value or a promise StateMachine
 * wrapper around one.
 * @origin src/interpreter.c
 */
extern void Run(Interpreter* interpreter, Value* fnOrSm);

/**
 * @brief Maps canonical core module names (e.g. "io", "math") to their native
 * Loader entry points for import.
 * @origin src/core/loader.c
 */
extern CoreMapper _CoreModuleMappers[];

void SaveRootEnv(Interpreter* interpreter, Value* env) {
	interpreter->Envs[interpreter->EnvrC++] = interpreter->CallEnv;
	interpreter->RootEnv					= env;
	interpreter->CallEnv					= env;
}

void SaveEnv(Interpreter* interpreter, Value* env) {
	interpreter->Envs[interpreter->EnvrC++] = interpreter->CallEnv;
	interpreter->CallEnv					= env;
}

void RestoreEnv(Interpreter* interpreter) {
	Value* top = interpreter->Envs[interpreter->EnvrC - 1];
	interpreter->Envs[--interpreter->EnvrC] = NULL;
	interpreter->CallEnv					= top;
}

void RestoreNthEnvAndSync(Interpreter* interpreter, int n) {
	if (n < 0 || n >= interpreter->EnvrC) {
		// Invalid index, do nothing or handle error as needed
		return;
	}
	int			 start = interpreter->EnvrC - n;
	Value*		 top   = interpreter->Envs[start];
	Environment* dst   = CoerceToEnvironment(top);
	// Remove all environments above n
	for (int i = start + 1; i < n; i++) {
		Environment* current = CoerceToEnvironment(interpreter->Envs[i]);
		EnvironmentSync(current, dst);
		interpreter->Envs[i] = NULL;
	}
	interpreter->EnvrC	 -= n;
	interpreter->CallEnv  = top;
}

bool IsMethodOfObject(Interpreter* interpreter, Value* obj, Value* method) {
	String key = ValueToString(method);
	if (ValueIsPromise(obj)) {
		// Handle Promise methods or attributes
		Class* cls = CoerceToUserClass(interpreter->Promise);

		while (cls != NULL) {
			if (ClassHasMember(cls, key, false, true)) {
				free(key);
				return true;
			}
			if (cls->Base == NULL)
				break;
			cls = CoerceToUserClass(cls->Base);
		}

	} else if (ValueIsArray(obj)) {
		// Handle array methods or attributes
		Array* array = CoerceToArray(obj);

		// Check prototype chain
		Class* cls = CoerceToUserClass(interpreter->Array);

		while (cls != NULL) {
			if (ClassHasMember(cls, key, false, true)) {
				free(key);
				return true;
			}
			if (cls->Base == NULL)
				break;
			cls = CoerceToUserClass(cls->Base);
		}
	} else if (ValueIsObject(obj)) {
		// Handle object methods or attributes
		HashMap* map = CoerceToHashMap(obj);

		// Check prototype chain
		Class* cls = NULL;

		while (cls != NULL) {
			if (ClassHasMember(cls, key, false, true)) {
				free(key);
				return true;
			}
			if (cls->Base == NULL)
				break;
			cls = CoerceToUserClass(cls->Base);
		}
	} else if (ValueIsClass(obj)) {
		// Handle Class static functions or attributes
		Class* cls = CoerceToUserClass(obj);

		while (cls != NULL) {
			if (ClassHasMember(cls, key, false, true)) {
				free(key);
				return true;
			}
			if (cls->Base == NULL)
				break;
			cls = CoerceToUserClass(cls->Base);
		}
	} else if (ValueIsClassInstance(obj)) {
		// Handle class instance methods or attributes
		ClassInstance* instance = CoerceToClassInstance(obj);

		// Check prototype chain
		Class* cls = CoerceToUserClass(instance->Proto);

		while (cls != NULL) {
			if (ClassHasMember(cls, key, false, true)) {
				free(key);
				return true;
			}
			if (cls->Base == NULL)
				break;
			cls = CoerceToUserClass(cls->Base);
		}
	}

	free(key);
	return false;
}

Value* GenericGetAttribute(Interpreter* interpreter,
						   Value*		obj,
						   Value*		index,
						   bool			forMethodCall) {
	String key = ValueToString(index);
	if (ValueIsPromise(obj)) {
		// Handle Promise methods or attributes
		Class* cls = CoerceToUserClass(interpreter->Promise);

		while (cls != NULL) {
			if (ClassHasMember(cls, key, false, forMethodCall)) {
				Value* member = ClassGetMember(cls, key, false);
				free(key);
				return member;
			}
			if (cls->Base == NULL)
				break;
			cls = CoerceToUserClass(cls->Base);
		}

	} else if (ValueIsArray(obj)) {
		// Handle array methods or attributes
		Array* array = CoerceToArray(obj);

		if (ValueIsAnyNum(index)) {
			long idx = (long) CoerceToI64(index);
			if (idx < 0 || idx >= array->Count) {
				free(key);
				String errMsg =
					FormatString("%s: array index %ld out of bounds",
								 INDEX_ERROR,
								 idx);
				Value* errVal = NewErrorValue(interpreter, errMsg);
				free(errMsg);
				return errVal;
			}
			free(key);
			return array->Items[idx];
		}

		// Check prototype chain
		Class* cls = CoerceToUserClass(interpreter->Array);

		while (forMethodCall && cls != NULL) {
			if (ClassHasMember(cls, key, false, forMethodCall)) {
				Value* member = ClassGetMember(cls, key, false);
				free(key);
				return member;
			}
			if (cls->Base == NULL)
				break;
			cls = CoerceToUserClass(cls->Base);
		}

	} else if (ValueIsObject(obj)) {
		// Handle object methods or attributes
		HashMap* map = CoerceToHashMap(obj);

		if (HashMapContains(map, key)) {
			Value* val = HashMapGet(map, key);
			free(key);
			return val;
		}

		// Check prototype chain
		Class* cls = NULL;

		while (forMethodCall && cls != NULL) {
			if (ClassHasMember(cls, key, false, forMethodCall)) {
				Value* member = ClassGetMember(cls, key, false);
				free(key);
				return member;
			}
			if (cls->Base == NULL)
				break;
			cls = CoerceToUserClass(cls->Base);
		}

	} else if (ValueIsClass(obj)) {
		// Handle Class static functions or attributes
		Class* cls = CoerceToUserClass(obj);

		while (cls != NULL) {
			if (ClassHasMember(cls, key, true, forMethodCall)) {
				Value* member = ClassGetMember(cls, key, true);
				free(key);
				return member;
			}
			if (cls->Base == NULL)
				break;
			cls = CoerceToUserClass(cls->Base);
		}
	} else if (ValueIsClassInstance(obj)) {
		// Handle class instance methods or attributes
		ClassInstance* instance = CoerceToClassInstance(obj);

		// Check prototype chain
		Class* cls = CoerceToUserClass(instance->Proto);

		while (forMethodCall && cls != NULL) {
			if (ClassHasMember(cls, key, !forMethodCall, forMethodCall)) {
				Value* member = ClassGetMember(cls, key, !forMethodCall);
				free(key);
				return member;
			}
			if (cls->Base == NULL)
				break;
			cls = CoerceToUserClass(cls->Base);
		}

		// Check instance members
		Value* member = HashMapGet(instance->Members, key);

		if (member != NULL) {
			free(key);
			return member;
		}
	} else if (ValueIsStr(obj)) {
		Rune*  rne = (Rune*) obj->Value.Opaque;
		String st  = ValueToString(obj);
		size_t ln  = utf_length(st);
		free(st);
		if (!ValueIsAnyNum(index)) {
			free(key);
			String idxStr = ValueToString(index);
			String errMsg =
				FormatString("%s: string indices must be integers, not %s (%s)",
							 TYPE_ERROR,
							 ValueTypeOf(index),
							 idxStr);
			Value* errVal = NewErrorValue(interpreter, errMsg);
			free(idxStr);
			free(errMsg);
			return errVal;
		}
		long idx = CoerceToI64(index);
		if (idx < 0 || idx >= ln) {
			free(key);
			String errMsg = FormatString("%s: string index %ld out of bounds",
										 INDEX_ERROR,
										 idx);
			Value* errVal = NewErrorValue(interpreter, errMsg);
			free(errMsg);
			return errVal;
		}
		String str = utf_rune_to_string(rne[idx]);
		Value* val = NewStrValue(interpreter, str);
		free(key);
		free(str);
		return val;
	}
	free(key);
	return interpreter->Null;
}

Value* DoImportCore(Interpreter* interpreter, String moduleName) {
	Value* result = (Value*) HashMapGet(interpreter->Imports, moduleName);

	if (result != NULL) {
		return result;
	}

	result = LoadCoreModule(interpreter, moduleName);

	if (result == NULL) {
		String errMsg = FormatString("%s: core module '%s' not found",
									 IMPORT_ERROR,
									 moduleName);
		Value* errVal = NewErrorValue(interpreter, errMsg);
		free(errMsg);
		return errVal;
	}

	HashMapSet(interpreter->Imports, moduleName, result);

	return result;
}

/**
 * @brief Allocates a Lexer over UTF-32 runes with filename attribution for
 * diagnostics and token positions.
 * @param path Logical source path stored on the lexer (need not be a real
 * filesystem path).
 * @param data NUL-terminated wide-character buffer owned by caller until lexer
 * teardown.
 * @return Heap Lexer ready for CreateParser(), or NULL only if allocation fails
 * inside lexer.c.
 * @origin src/lexer.c
 */
extern Lexer* CreateLexer(String path, Rune* data);

/**
 * @brief Frees the Lexer wrapper struct; does not free the path string or the
 * Rune buffer passed to CreateLexer().
 * @param lexer Lexer previously returned by CreateLexer(); NULL is safe and is
 * a no-op.
 * @origin src/lexer.c
 */
extern void FreeLexer(Lexer* lexer);

/**
 * @brief Constructs parser state that pulls lookahead tokens from the given
 * lexer.
 * @param lexer Live lexer positioned before the first program token.
 * @return Parser object whose Parse() consumes lexer tokens until EOF or error.
 * @origin src/parser.c
 */
extern Parser* CreateParser(Lexer* lexer);

/**
 * @brief Consumes the lexer-driven token stream and builds the program AST
 * (statements, declarations, expressions).
 * @param parser Parser previously wired to a lexer; mutates parser error state
 * on syntax failure.
 * @return Root Ast node for the compilation unit, or partial tree with errors
 * recorded in parser.
 * @origin src/parser.c
 */
extern Ast* Parse(Parser* parser);

/**
 * @brief Destroys parser scratch state; does not free the lexer (caller frees
 * lexer separately).
 * @param parser Parser returned from CreateParser().
 * @origin src/parser.c
 */
extern void FreeParser(Parser* parser);

/**
 * @brief Recursively frees an Ast subtree and every owned string/child pointer
 * allocated during parsing.
 * @param ast Root node returned from Parse(); NULL is safe.
 * @origin src/astnode.c
 */
extern void FreeAst(Ast* ast);

/**
 * @brief Builds a Compiler bound to interpreter constants/tables while
 * consuming parser-produced AST metadata.
 * @param interpreter Supplies constant pools and runtime hooks referenced
 * during codegen.
 * @param parser Parser whose lexer positions feed line info into emitted
 * bytecode.
 * @return Compiler state used for a single CompileAst() invocation.
 * @origin src/compiler.c
 */
extern Compiler* CreateCompiler(Interpreter* interpreter, Parser* parser);

/**
 * @brief Lowers programAst to a VLT_USER_FUNCTION Value containing bytecode,
 * scope metadata, and debug tables.
 * @param compiler Active compiler previously created for this
 * interpreter/parser pair.
 * @param programAst Root AST for the module or snippet being compiled.
 * @return Callable Value* (user function) on success, or an Error Value when
 * compilation aborts.
 * @origin src/compiler.c
 */
extern Value* CompileAst(Compiler* compiler, Ast* programAst);

/**
 * @brief Tears down compiler-local registries and scratch buffers after
 * CompileAst() finishes.
 * @param compiler Compiler instance to dispose; does not free the interpreter
 * or parser.
 * @origin src/compiler.c
 */
extern void FreeCompiler(Compiler* compiler);

/**
 * @brief Runs _RunProgram(): seeds the module environment, executes fnValue via
 * Run(), then drives the combined task queue and mongoose poll loop until no
 * pending async work or open connections remain.
 * @param interpreter Fully initialized interpreter (built-ins, paths, GC roots,
 * MgMgr).
 * @param fnValue VLT_USER_FUNCTION entry compiled for this module; becomes the
 * first Run() target.
 * @origin src/interpreter.c
 */
extern void Interpret(Interpreter* interpreter, Value* fnValue);

static Value* DoImportFileOrLib(Interpreter* interpreter,
								String		 moduleNameOrPath,
								bool		 isLib) {
	String		currentModuleName = interpreter->ModulePath;
	ImportNode* currentModule =
		CreateOrGetImportNode(interpreter, currentModuleName);

	String filePath = NULL;
	FILE*  file		= NULL;

	// 1. Resolve Path and Open File based on Import Type
	if (isLib) {
		String basePath = interpreter->ExecPath;
#ifdef _WIN32
		filePath = FormatString("%slib\\%s.zs", basePath, moduleNameOrPath);
#else
		filePath =
			FormatString("/usr/local/lib/zscript/lib/%s.zs", moduleNameOrPath);
#endif
		file = fopen(filePath, "rb");

		// Search for relative lib fallback
		if (!file) {
			free(filePath);
			filePath = FormatString("%slib/%s.zs", basePath, moduleNameOrPath);
			file	 = fopen(filePath, "rb");
		}
	} else {
		filePath = FormatString("%s.zs", moduleNameOrPath);
		file	 = fopen(filePath, "rb");
	}

	// 2. Handle File Not Found
	if (!file) {
		String errMsg;
		if (isLib) {
			errMsg =
				FormatString("%s: lib module '%s' not found (searched '%s')",
							 IMPORT_ERROR,
							 moduleNameOrPath,
							 filePath);
		} else {
			errMsg =
				FormatString("%s: file '%s' not found", IMPORT_ERROR, filePath);
		}
		Value* errVal = NewErrorValue(interpreter, errMsg);
		free(errMsg);
		free(filePath);
		return errVal;
	}

	// 3. Cycle Detection
	ImportNode* newNode = CreateOrGetImportNode(interpreter, filePath);

	if (newNode->State == VISITING) {
		String errMsg = FormatString("%s: circular dependency detected when "
									 "importing '%s'",
									 IMPORT_ERROR,
									 filePath);
		Value* errVal = NewErrorValue(interpreter, errMsg);
		free(errMsg);
		free(filePath);
		fclose(file);
		return errVal;
	}

	ImportNodeAddDependency(currentModule, newNode);

	// 4. Cache Check
	if (HashMapContains(interpreter->Imports, filePath)) {
		Value* mod = (Value*) HashMapGet(interpreter->Imports, filePath);
		free(filePath);
		fclose(file);
		newNode->State = SAFE;
		return mod;
	}

	newNode->State = VISITING;

	// 5. Read File
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	String buffer = Allocate(size + 1);
	fread(buffer, 1, size, file);
	buffer[size] = '\0';
	fclose(file);

	Rune* data = StringToRunes(buffer);
	free(buffer);

	// 6. Lex, parse, compile, interpret
	Lexer*	  lexer		 = CreateLexer(filePath, data);
	Parser*	  parser	 = CreateParser(lexer);
	Ast*	  programAst = Parse(parser);
	Compiler* compiler	 = CreateCompiler(interpreter, parser);
	Value*	  compiled	 = CompileAst(compiler, programAst);

	UserFunction* uf		= CoerceToUserFunction(compiled);
	interpreter->ModulePath = uf->Name;

	DoCall(interpreter, compiled, 0, false);
	Value* result = Popp(interpreter);

	interpreter->ModulePath = currentModuleName;

	// 7. Cache & Cleanup
	HashMapSet(interpreter->Imports, filePath, result);
	newNode->State = SAFE;

	FreeLexer(lexer);
	FreeParser(parser);
	FreeAst(programAst);
	FreeCompiler(compiler);
	free(filePath);
	free(data);

	return result;
}

Value* DoImportLib(Interpreter* interpreter, String moduleName) {
	return DoImportFileOrLib(interpreter, moduleName, true);
}

Value* DoImportFile(Interpreter* interpreter, String filePathNoExt) {
	return DoImportFileOrLib(interpreter, filePathNoExt, false);
}

Value*
DoSetIndex(Interpreter* interpreter, Value* obj, Value* index, Value* val) {
	String hashKey = ValueToString(index);
	if (ValueIsArray(obj)) {
		free(hashKey);
		Array* array = CoerceToArray(obj);
		long   idx	 = (long) CoerceToI64(index);
		if (idx < 0 || idx >= array->Count) {
			String errMsg = FormatString("%s: array index %ld out of bounds",
										 INDEX_ERROR,
										 idx);
			Value* errVal = NewErrorValue(interpreter, errMsg);
			free(errMsg);
			return errVal;
		}
		array->Items[idx] = val;
	} else if (ValueIsObject(obj)) {
		HashMap* map = CoerceToHashMap(obj);
		if (map->ReadOnly) {
			free(hashKey);
			return NewErrorFValue(interpreter,
								  "%s: cannot set index on read-only object",
								  TYPE_ERROR);
		}
		HashMapSet(map, hashKey, val);
		free(hashKey);
	} else if (ValueIsClassInstance(obj)) {
		ClassInstance* instance = CoerceToClassInstance(obj);
		HashMapSet(instance->Members, hashKey, val);
		free(hashKey);
	} else if (ValueIsClass(obj)) {
		Class* cls = CoerceToUserClass(obj);
		HashMapSet(cls->StaticMembers, hashKey, val);
		free(hashKey);
	} else {
		free(hashKey);
		return NewErrorFValue(interpreter,
							  "%s: cannot set index on non-object",
							  TYPE_ERROR);
	}
	return interpreter->Null;
}

Value* DoGetIndex(Interpreter* interpreter, Value* obj, Value* index) {
	return GenericGetAttribute(interpreter, obj, index, false);
}

Value* DoCallCtor(Interpreter* interpreter, Value* clsValue, int argc) {
	if (clsValue == NULL)
		Panic("Attempted to call constructor on a null value\n");

	if (!ValueIsClass(clsValue)) {
		PopN(interpreter, argc);
		return NewErrorFValue(interpreter,
							  "%s: attempted to call "
							  "constructor on non-class value",
							  TYPE_ERROR);
	}

	Class* cls = CoerceToUserClass(clsValue);

	if (!ClassHasMember(cls, CONSTRUCTOR_NAME, false, true)) {
		if (argc != 0) {
			PopN(interpreter, argc);
			String errMsg =
				FormatString("%s: argument count mismatch, expected "
							 "0 arguments but got %d",
							 ARGUMENT_ERROR,
							 argc);
			Value* errVal = NewErrorValue(interpreter, errMsg);
			free(errMsg);
			return errVal;
		}
		// Push default instance, no constructor call
		ClassInstance* instance = CreateClassInstance(clsValue);
		Push(interpreter, NewClassInstanceValue(interpreter, instance));
		return interpreter->Null;
	}

	// Push thisArg
	Value* instanceValue = NULL;

	if (strcmp(cls->Name, "Object") == 0) {
		instanceValue = NewObjectValue(interpreter);
	} else if (strcmp(cls->Name, "Array") == 0) {
		instanceValue = NewArrayValue(interpreter);
	} else if (strcmp(cls->Name, "Blob") == 0) {
		instanceValue = NewBlobValue(interpreter, NULL, 0, NULL);
	} else {
		instanceValue =
			NewClassInstanceValue(interpreter, CreateClassInstance(clsValue));
	}

	Push(interpreter, instanceValue);

	Value* constructor = ClassGetMember(cls, CONSTRUCTOR_NAME, false);

	Value* result = DoCall(interpreter, constructor, ++argc, true);

	if (ValueIsNull(result)) {
		Popp(interpreter);	  // Pop constructor return value
		Push(interpreter,
			 instanceValue);  // Push instance as return value
	}

	return result;
}

Value* DoCallMethod(Interpreter* interpreter,
					Value*		 obj,
					Value*		 methodName,
					int			 argc) {
	bool withThis = IsMethodOfObject(interpreter, obj, methodName);
	if (!withThis) {
		argc--;
		Popp(interpreter);	// pop 'this'
	}

	Value* method = GenericGetAttribute(interpreter, obj, methodName, true);

	if (ValueIsNull(method)) {
		PopN(interpreter, argc);
		String method = ValueToString(methodName);
		String errMsg =
			FormatString("%s: method '%s' not found on object of type %s",
						 ATTRIBUTE_ERROR,
						 method,
						 ValueTypeOf(obj));
		Value* errVal = NewErrorValue(interpreter, errMsg);
		free(method);
		free(errMsg);
		return errVal;
	}

	return DoCall(interpreter, method, argc, withThis);
}

/**
 * @brief Records line/fn pair at CallStack[CallStackC++] so errors and stack
 * dumps can cite source locations.
 * @param interpreter VM whose call stack ring is bounded by STACK_SIZE
 * (unchecked here).
 * @param line Lexer/parser-derived source coordinates for the active opcode
 * span.
 * @param fn Callable Value associated with this activation (used only for
 * diagnostics).
 * @origin src/interpreter.c
 */
extern void PushTrace(Interpreter* interpreter, LineInfo line, Value* fn);

/**
 * @brief Decrements CallStackC after unwinding one nested call frame from the
 * diagnostic call stack.
 * @param interpreter VM whose CallStackC must be > 0; otherwise behaviour is
 * undefined.
 * @origin src/interpreter.c
 */
extern void PopTrace(Interpreter* interpreter);

Value* DoCall(Interpreter* interpreter, Value* fn, int argc, bool withThis) {
	Value* en = NULL;

	if (fn == NULL)
		Panic("Attempted to call a null value!");

	UserFunction* uf = NULL;
	StateMachine* sm = NULL;

	bool isfn = ValueIsUserFunction(fn);

	if (isfn) {
		uf = CoerceToUserFunction(fn);
		SaveEnv(interpreter,
				(en = NewEnvironmentValue(
					 interpreter,
					 CreateEnvironment(uf->Scope, uf->LocalC))));
	}

	if (ValueIsUserFunction(fn) && CoerceToUserFunction(fn)->Async) {
		uf			= CoerceToUserFunction(fn);
		sm			= CreateStateMachine(PENDING,
										 false,
										 0,
										 interpreter->CallEnv,
										 NULL,
										 fn);
		fn			= NewPromiseValue(interpreter, sm);
		sm->StckBot = interpreter->StckC;
		sm->EnvrBot = interpreter->EnvrC;
	}

	if (ValueIsPromise(fn)) {
		SetActiveTask(interpreter, fn);
		sm			= CoerceToStateMachine(fn);
		sm->StckBot = interpreter->StckC;
		sm->EnvrBot = interpreter->EnvrC;

		PushTrace(interpreter, sm->Line, fn);

		if (1) {
			// 2. ANCHOR: Set the new bottom to the CURRENT top
			// of the stack
			interpreter->EnvrC = sm->EnvrBot;
			interpreter->StckC = sm->StckBot;

			if (sm->EnvStack != NULL && sm->EnvrTop > 0) {
				memcpy(&interpreter->Envs[sm->EnvrBot],
					   sm->EnvStack,
					   sizeof(Value*) * sm->EnvrTop);

				// 3. Advance the global env stack pointer
				interpreter->EnvrC = sm->EnvrBot + sm->EnvrTop;
			}

			// 4. Restore to the OFFSET position
			// (interpreter->Stacks + sm->StackBot)
			if (sm->Stacks != NULL && sm->StckTop > 0) {
				memcpy(&interpreter->Stacks[sm->StckBot],
					   sm->Stacks,
					   sizeof(Value*) * sm->StckTop);

				// 5. Advance the global stack pointer
				interpreter->StckC = sm->StckBot + sm->StckTop;
			}
		}

		if (sm->Ip == 0) {
			Run(interpreter, fn);
		} else {
			interpreter->CallEnv = sm->CallEnv;
			Run(interpreter, fn);
		}

		SetActiveTask(interpreter, NULL);

		PopTrace(interpreter);

		if (isfn)
			RestoreEnv(interpreter);

		return interpreter->Null;
	}

	if (!ValueIsCallable(fn)) {
		PopN(interpreter, argc);
		return NewErrorFValue(interpreter,
							  "%s: invalid operation: attempted to call a "
							  "non-callable value (%s)",
							  TYPE_ERROR,
							  ValueTypeOf(fn));
	}

	if (ValueIsNativeFunction(fn)) {
		NativeFunction*		   nFMeta	  = CoerceToNativeFunction(fn);
		NativeFunctionCallback nativeFunc = nFMeta->FuncPtr;

		if (nFMeta->Argc != VARARG && argc != nFMeta->Argc) {
			PopN(interpreter, argc);
			String errMsg =
				FormatString("%s: argument count mismatch: expected "
							 "%d arguments but got %d",
							 ARGUMENT_ERROR,
							 nFMeta->Argc,
							 argc);

			Value* errVal = NewErrorValue(interpreter, errMsg);
			free(errMsg);

			return errVal;
		}

		Value** args = NULL;

		if (argc > 0) {
			args	= Allocate(sizeof(Value*) * argc);
			args[0] = NULL;
		}

		for (int i = 0; i < argc; i++) {
			args[i] = Popp(interpreter);
			// printf("ARG[%d]: %s\n", i,
			// ValueToString(args[i]));
		}

		Value* res = nativeFunc(interpreter, argc, args);

		Push(interpreter, res);
		free(args);

		return ValueIsError(res) ? res : interpreter->Null;
	}

	// Call
	if (argc != uf->Argc) {
		PopN(interpreter, argc);
		String errMsg = FormatString("%s: argument count mismatch: expected %d "
									 "arguments but got %d",
									 ARGUMENT_ERROR,
									 uf->Argc,
									 argc);

		Value* errVal = NewErrorValue(interpreter, errMsg);
		free(errMsg);

		if (isfn)
			RestoreEnv(interpreter);

		return errVal;
	}

	PushTrace(interpreter, uf->Lines[0], fn);

	// 1. Save
	Value* oldRoot = interpreter->RootEnv;
	if (uf->Scope == NULL) {
		interpreter->RootEnv = en;
	}

	SetActiveFunction(interpreter, fn);

	// 2. Run the function
	Run(interpreter, fn);

	SetActiveFunction(interpreter, NULL);

	PopTrace(interpreter);

	// 3. Restore
	if (isfn)
		RestoreEnv(interpreter);
	interpreter->RootEnv = oldRoot;

	return interpreter->Null;
}

Value* DoNot(Interpreter* interpreter, Value* val) {
	bool resultBool = !CoerceToBool(val);
	return resultBool ? interpreter->True : interpreter->False;
}

Value* DoPos(Interpreter* interpreter, Value* val) {
	if (ValueIsInt(val)) {
		return NewIntValue(interpreter, +CoerceToI32(val));
	} else if (ValueIsNum(val)) {
		return NewNumValue(interpreter, +CoerceToNum(val));
	} else if (ValueIsAnyNum(val)) {
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		bf_t* tmpBf = CoerceToBitField(interpreter, val);
		bf_set(resNum, tmpBf);
		FreeTempBf(interpreter, tmpBf, val);
		// unary + is a no-op, just copy
		int prec = BFPrecession(val);
		return prec == PREC_INT ? NewBigIntValue(interpreter, resNum)
								: NewBigNumValue(interpreter, resNum);
	} else {
		String errMsg = FormatString("%s: invalid operand for operator (+): %s",
									 TYPE_ERROR,
									 ValueTypeOf(val));
		Value* errVal = NewErrorValue(interpreter, errMsg);
		free(errMsg);
		return errVal;
	}
}

Value* DoNeg(Interpreter* interpreter, Value* val) {
	if (ValueIsInt(val)) {
		return NewIntValue(interpreter, -CoerceToI32(val));
	} else if (ValueIsNum(val)) {
		return NewNumValue(interpreter, -CoerceToNum(val));
	} else if (ValueIsAnyNum(val)) {
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		bf_t* tmpBf = CoerceToBitField(interpreter, val);
		bf_set(resNum, tmpBf);
		FreeTempBf(interpreter, tmpBf, val);
		bf_neg(resNum);	 // flip sign bit
		int prec = BFPrecession(val);
		return prec == PREC_INT ? NewBigIntValue(interpreter, resNum)
								: NewBigNumValue(interpreter, resNum);
	} else {
		String errMsg = FormatString("%s: invalid operand for operator (-): %s",
									 TYPE_ERROR,
									 ValueTypeOf(val));
		Value* errVal = NewErrorValue(interpreter, errMsg);
		free(errMsg);
		return errVal;
	}
}

Value* DoMul(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
		long resultNum = CoerceToI64(lhs) * CoerceToI64(rhs);
		result		   = (resultNum <= INT_MAX && resultNum >= INT_MIN)
							 ? NewIntValue(interpreter, (int) resultNum)
							 : NewNumValue(interpreter, (double) resultNum);
	} else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		double resultNum = CoerceToNum(lhs) * CoerceToNum(rhs);
		result			 = (resultNum == (int) resultNum && resultNum <= INT_MAX
							&& resultNum >= INT_MIN)
							   ? NewIntValue(interpreter, (int) resultNum)
							   : NewNumValue(interpreter, resultNum);
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum = CoerceToBitField(interpreter, rhs);
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		int prec = BFPrecession(lhs) | BFPrecession(rhs);
		bf_mul(resNum,
			   lhsNum,
			   rhsNum,
			   prec,
			   BF_RNDN | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = prec == PREC_INT ? NewBigIntValue(interpreter, resNum)
								  : NewBigNumValue(interpreter, resNum);
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (*): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoDiv(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	// Check for division by zero
	if ((ValueIsInt(rhs) && CoerceToI64(rhs) == 0)
		|| (ValueIsNum(rhs) && CoerceToNum(rhs) == 0.0)) {
		return NewErrorFValue(interpreter,
							  "%s: division by zero",
							  ZERO_DIVISION_ERROR);
	}

	if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
		long resultNum = CoerceToI64(lhs) / CoerceToI64(rhs);
		result		   = (resultNum <= INT_MAX && resultNum >= INT_MIN)
							 ? NewIntValue(interpreter, (int) resultNum)
							 : NewNumValue(interpreter, (double) resultNum);
	} else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		double resultNum = CoerceToNum(lhs) / CoerceToNum(rhs);
		result			 = (resultNum == (int) resultNum && resultNum <= INT_MAX
							&& resultNum >= INT_MIN)
							   ? NewIntValue(interpreter, (int) resultNum)
							   : NewNumValue(interpreter, resultNum);
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum = CoerceToBitField(interpreter, rhs);
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		int prec = BFPrecession(lhs) | BFPrecession(rhs);
		bf_div(resNum,
			   lhsNum,
			   rhsNum,
			   prec,
			   BF_RNDN | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = prec == PREC_INT ? NewBigIntValue(interpreter, resNum)
								  : NewBigNumValue(interpreter, resNum);
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (/): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoMod(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	// Check for modulo by zero
	if ((ValueIsInt(rhs) && CoerceToI64(rhs) == 0)
		|| (ValueIsNum(rhs) && CoerceToNum(rhs) == 0.0)) {
		return NewErrorFValue(interpreter,
							  "%s: modulo by zero",
							  ZERO_DIVISION_ERROR);
	}

	if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
		long resultNum = CoerceToI64(lhs) % CoerceToI64(rhs);
		result		   = (resultNum <= INT_MAX && resultNum >= INT_MIN)
							 ? NewIntValue(interpreter, (int) resultNum)
							 : NewNumValue(interpreter, (double) resultNum);
	} else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		double resultNum = fmod(CoerceToNum(lhs), CoerceToNum(rhs));
		result			 = (resultNum == (int) resultNum && resultNum <= INT_MAX
							&& resultNum >= INT_MIN)
							   ? NewIntValue(interpreter, (int) resultNum)
							   : NewNumValue(interpreter, resultNum);
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum = CoerceToBitField(interpreter, rhs);
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		int prec = BFPrecession(lhs) | BFPrecession(rhs);
		bf_rem(resNum,
			   lhsNum,
			   rhsNum,
			   prec,
			   BF_RNDN | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS,
			   BF_RNDZ);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = prec == PREC_INT ? NewBigIntValue(interpreter, resNum)
								  : NewBigNumValue(interpreter, resNum);
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (%%): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoInc(Interpreter* interpreter, Value* val) {
	Value* result = NULL;
	int	   offset = GetOffset();

	if (ValueIsInt(val)) {
		long resultNum = CoerceToI64(val) + 1;
		result		   = (resultNum <= INT_MAX && resultNum >= INT_MIN)
							 ? NewIntValue(interpreter, (int) resultNum)
							 : NewNumValue(interpreter, (double) resultNum);
	} else if (ValueIsNum(val)) {
		double resultNum = CoerceToNum(val) + 1.0;
		result			 = (resultNum == (int) resultNum && resultNum <= INT_MAX
							&& resultNum >= INT_MIN)
							   ? NewIntValue(interpreter, (int) resultNum)
							   : NewNumValue(interpreter, resultNum);
	} else if (ValueIsAnyNum(val)) {
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		bf_t* tmpBf = CoerceToBitField(interpreter, val);
		bf_set(resNum, tmpBf);
		FreeTempBf(interpreter, tmpBf, val);
		bf_add_si(resNum,
				  resNum,
				  1,
				  BF_PREC_INF,
				  BF_RNDZ | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
		int prec = BFPrecession(val);
		return prec == PREC_INT ? NewBigIntValue(interpreter, resNum)
								: NewBigNumValue(interpreter, resNum);
	} else {
		String errMsg =
			FormatString("%s: invalid operand for operator (++): %s",
						 TYPE_ERROR,
						 ValueTypeOf(val));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

static size_t RuneLength(Rune* rune) {
	if (rune == NULL)
		return 0;
	size_t i = 0;
	while (rune[i] != 0)
		i++;
	return i;
}

Value* DoAdd(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
		long resultNum = CoerceToI64(lhs) + CoerceToI64(rhs);
		result		   = (resultNum <= INT_MAX && resultNum >= INT_MIN)
							 ? NewIntValue(interpreter, (int) resultNum)
							 : NewNumValue(interpreter, (double) resultNum);
	} else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		double resultNum = CoerceToNum(lhs) + CoerceToNum(rhs);
		result			 = (resultNum == (int) resultNum && resultNum <= INT_MAX
							&& resultNum >= INT_MIN)
							   ? NewIntValue(interpreter, (int) resultNum)
							   : NewNumValue(interpreter, resultNum);
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum = CoerceToBitField(interpreter, rhs);
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		int prec = BFPrecession(lhs) | BFPrecession(rhs);
		bf_add(resNum,
			   lhsNum,
			   rhsNum,
			   prec,
			   BF_RNDN | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = prec == PREC_INT ? NewBigIntValue(interpreter, resNum)
								  : NewBigNumValue(interpreter, resNum);
	} else if (ValueIsStr(lhs) && ValueIsStr(rhs)) {
		Rune*  lhsRunes = (Rune*) lhs->Value.Opaque;
		Rune*  rhsRunes = (Rune*) rhs->Value.Opaque;
		size_t lhsLen	= RuneLength(lhsRunes);
		size_t rhsLen	= RuneLength(rhsRunes);
		Rune*  out		= Allocate(sizeof(Rune) * (lhsLen + rhsLen + 1));
		if (lhsLen > 0)
			memcpy(out, lhsRunes, sizeof(Rune) * lhsLen);
		if (rhsLen > 0)
			memcpy(out + lhsLen, rhsRunes, sizeof(Rune) * rhsLen);
		out[lhsLen + rhsLen] = 0;
		result				 = NewStrValueOwningRunes(interpreter, out);
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (+): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoDec(Interpreter* interpreter, Value* val) {
	Value* result = NULL;
	int	   offset = GetOffset();

	if (ValueIsInt(val)) {
		long resultNum = CoerceToI64(val) - 1;
		result		   = (resultNum <= INT_MAX && resultNum >= INT_MIN)
							 ? NewIntValue(interpreter, (int) resultNum)
							 : NewNumValue(interpreter, (double) resultNum);
	} else if (ValueIsNum(val)) {
		double resultNum = CoerceToNum(val) - 1.0;
		result			 = (resultNum == (int) resultNum && resultNum <= INT_MAX
							&& resultNum >= INT_MIN)
							   ? NewIntValue(interpreter, (int) resultNum)
							   : NewNumValue(interpreter, resultNum);
	} else if (ValueIsAnyNum(val)) {
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		bf_t* tmpBf = CoerceToBitField(interpreter, val);
		bf_set(resNum, tmpBf);
		FreeTempBf(interpreter, tmpBf, val);
		bf_add_si(resNum,
				  resNum,
				  -1,
				  BF_PREC_INF,
				  BF_RNDZ | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
		int prec = BFPrecession(val);
		return prec == PREC_INT ? NewBigIntValue(interpreter, resNum)
								: NewBigNumValue(interpreter, resNum);
	} else {
		String errMsg =
			FormatString("%s: invalid operand for operator (--): %s",
						 TYPE_ERROR,
						 ValueTypeOf(val));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoSub(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
		long resultNum = CoerceToI64(lhs) - CoerceToI64(rhs);
		result		   = (resultNum <= INT_MAX && resultNum >= INT_MIN)
							 ? NewIntValue(interpreter, (int) resultNum)
							 : NewNumValue(interpreter, (double) resultNum);
	} else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		double resultNum = CoerceToNum(lhs) - CoerceToNum(rhs);
		result			 = (resultNum == (int) resultNum && resultNum <= INT_MAX
							&& resultNum >= INT_MIN)
							   ? NewIntValue(interpreter, (int) resultNum)
							   : NewNumValue(interpreter, resultNum);
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum = CoerceToBitField(interpreter, rhs);
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		int prec = BFPrecession(lhs) | BFPrecession(rhs);
		bf_sub(resNum,
			   lhsNum,
			   rhsNum,
			   prec,
			   BF_RNDN | BF_FTOA_FORMAT_FRAC | BF_FTOA_JS_QUIRKS);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = prec == PREC_INT ? NewBigIntValue(interpreter, resNum)
								  : NewBigNumValue(interpreter, resNum);
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (-): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoLShift(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		long resultNum = CoerceToI64(lhs) << CoerceToI64(rhs);
		result		   = (resultNum == (int) resultNum && resultNum <= INT_MAX
						  && resultNum >= INT_MIN)
							 ? NewIntValue(interpreter, (int) resultNum)
							 : NewNumValue(interpreter, resultNum);
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum = CoerceToBitField(interpreter, rhs);
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);

		// Get shift amount from rhs
		slimb_t shiftAmount;
#if LIMB_BITS == 32
		bf_get_int32(&shiftAmount, rhsNum, 0);
		if (shiftAmount == INT32_MIN)
			shiftAmount = INT32_MIN + 1;
#else
		bf_get_int64(&shiftAmount, rhsNum, 0);
		if (shiftAmount == INT64_MIN)
			shiftAmount = INT64_MIN + 1;
#endif

		bf_set(resNum, lhsNum);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		bf_mul_2exp(resNum, shiftAmount, BF_PREC_INF, BF_RNDZ);
		// Left shift should never produce a fraction on
		// integers, but guard anyway in case lhs is a float
		if (shiftAmount < 0) {
			bf_rint(resNum, BF_RNDD);
		}

		int prec = BFPrecession(lhs) | BFPrecession(rhs);
		result	 = prec == PREC_INT ? NewBigIntValue(interpreter, resNum)
									: NewBigNumValue(interpreter, resNum);
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (<<): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoRShift(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		long resultNum = CoerceToI64(lhs) >> CoerceToI64(rhs);
		result		   = (resultNum == (int) resultNum && resultNum <= INT_MAX
						  && resultNum >= INT_MIN)
							 ? NewIntValue(interpreter, (int) resultNum)
							 : NewNumValue(interpreter, resultNum);
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum = CoerceToBitField(interpreter, rhs);
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);

		// Get shift amount from rhs and negate it (right shift =
		// left shift by -n)
		slimb_t shiftAmount;
#if LIMB_BITS == 32
		bf_get_int32(&shiftAmount, rhsNum, 0);
		if (shiftAmount == INT32_MIN)
			shiftAmount = INT32_MIN + 1;
#else
		bf_get_int64(&shiftAmount, rhsNum, 0);
		if (shiftAmount == INT64_MIN)
			shiftAmount = INT64_MIN + 1;
#endif
		// Negate to make it a right shift
		shiftAmount = -shiftAmount;

		bf_set(resNum, lhsNum);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		bf_mul_2exp(resNum, shiftAmount, BF_PREC_INF, BF_RNDZ);
		// Right shift can produce a fraction, floor it
		// (arithmetic shift behavior)
		bf_rint(resNum, BF_RNDD);

		int prec = BFPrecession(lhs) | BFPrecession(rhs);
		result	 = prec == PREC_INT ? NewBigIntValue(interpreter, resNum)
									: NewBigNumValue(interpreter, resNum);
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (>>): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoLT(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		int comparison = CoerceToNum(lhs) < CoerceToNum(rhs);
		result		   = comparison ? interpreter->True : interpreter->False;
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum	 = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum	 = CoerceToBitField(interpreter, rhs);
		int	  comparison = bf_cmp_lt(lhsNum, rhsNum);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = comparison ? interpreter->True : interpreter->False;
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (<): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoLTE(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		int comparison = CoerceToNum(lhs) <= CoerceToNum(rhs);
		result		   = comparison ? interpreter->True : interpreter->False;
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum	 = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum	 = CoerceToBitField(interpreter, rhs);
		int	  comparison = bf_cmp_le(lhsNum, rhsNum);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = comparison ? interpreter->True : interpreter->False;
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (<=): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoGT(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		int comparison = CoerceToNum(lhs) > CoerceToNum(rhs);
		result		   = comparison ? interpreter->True : interpreter->False;
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum	 = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum	 = CoerceToBitField(interpreter, rhs);
		int	  comparison = bf_cmp_lt(rhsNum, lhsNum);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = comparison ? interpreter->True : interpreter->False;
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (>): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoGTE(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		int comparison = CoerceToNum(lhs) >= CoerceToNum(rhs);
		result		   = comparison ? interpreter->True : interpreter->False;
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum	 = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum	 = CoerceToBitField(interpreter, rhs);
		int	  comparison = bf_cmp_le(rhsNum, lhsNum);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = comparison ? interpreter->True : interpreter->False;
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (>=): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoEQ(Interpreter* interpreter, Value* lhs, Value* rhs) {
	if (ValueIsEqual(lhs, rhs)) {
		return interpreter->True;
	}

	if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum	 = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum	 = CoerceToBitField(interpreter, rhs);
		int	  comparison = bf_cmp(lhsNum, rhsNum) == 0;
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		return comparison ? interpreter->True : interpreter->False;
	}

	return interpreter->False;
}

Value* DoNE(Interpreter* interpreter, Value* lhs, Value* rhs) {
	if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum	 = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum	 = CoerceToBitField(interpreter, rhs);
		int	  comparison = bf_cmp(lhsNum, rhsNum) != 0;
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		return comparison ? interpreter->True : interpreter->False;
	}
	return !ValueIsEqual(lhs, rhs) ? interpreter->True : interpreter->False;
}

Value* DoAnd(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	// Bitwise AND only works on integers
	if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
		int resultValue = lhs->Value.I32 & rhs->Value.I32;
		result			= NewIntValue(interpreter, resultValue);
	} else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		long long resultValue = (int) CoerceToI64(lhs) & (int) CoerceToI64(rhs);
		result				  = NewNumValue(interpreter, resultValue);
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum = CoerceToBitField(interpreter, rhs);
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		bf_logic_and(resNum, lhsNum, rhsNum);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = NewBigIntValue(interpreter, resNum);
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (&): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoOr(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	// Bitwise OR only works on integers
	if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
		int resultValue = lhs->Value.I32 | rhs->Value.I32;
		result			= NewIntValue(interpreter, resultValue);
	} else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		long long resultValue = (int) CoerceToI64(lhs) | (int) CoerceToI64(rhs);
		result				  = NewNumValue(interpreter, resultValue);
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum = CoerceToBitField(interpreter, rhs);
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		bf_logic_or(resNum, lhsNum, rhsNum);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = NewBigIntValue(interpreter, resNum);
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (|): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoXor(Interpreter* interpreter, Value* lhs, Value* rhs) {
	Value* result = NULL;

	// Bitwise XOR only works on integers
	if (ValueIsInt(lhs) && ValueIsInt(rhs)) {
		int resultValue = lhs->Value.I32 ^ rhs->Value.I32;
		result			= NewIntValue(interpreter, resultValue);
	} else if (ValueIsNum(lhs) && ValueIsNum(rhs)) {
		long long resultValue = (int) CoerceToI64(lhs) ^ (int) CoerceToI64(rhs);
		result				  = NewNumValue(interpreter, resultValue);
	} else if (ValueIsAnyNum(lhs) && ValueIsAnyNum(rhs)) {
		bf_t* lhsNum = CoerceToBitField(interpreter, lhs);
		bf_t* rhsNum = CoerceToBitField(interpreter, rhs);
		bf_t* resNum = Allocate(sizeof(bf_t));
		bf_init(&interpreter->BfContext, resNum);
		bf_logic_xor(resNum, lhsNum, rhsNum);
		FreeTempBf(interpreter, lhsNum, lhs);
		FreeTempBf(interpreter, rhsNum, rhs);
		result = NewBigIntValue(interpreter, resNum);
	} else {
		String errMsg =
			FormatString("%s: invalid operands for operator (^): %s and %s",
						 TYPE_ERROR,
						 ValueTypeOf(lhs),
						 ValueTypeOf(rhs));
		result = NewErrorValue(interpreter, errMsg);
		free(errMsg);
	}

	return result;
}

Value* DoLoadFunction(Interpreter* interpreter, int offset, bool closure) {
	// For closure, clone the function
	Value*		  fn = interpreter->Functions[offset];
	UserFunction* uf = CoerceToUserFunction(fn);
	uf->Scope		 = interpreter->CallEnv;

	if (closure) {
		fn		  = NewUserFunctionValue(interpreter, UserFunctionClone(uf));
		uf		  = CoerceToUserFunction(fn);
		uf->Scope = interpreter->CallEnv;
	}

	Environment* rootEnv = CoerceToEnvironment(interpreter->RootEnv);
	Environment* loclEnv = CoerceToEnvironment(interpreter->CallEnv);

	for (int i = 0; i < uf->CaptureC; i++) {
		CaptureMeta capture = uf->CaptureMetas[i];
		// Traverse up the environment chain to find the captured
		// variable through depth
		int			 depth		= 1;
		Environment* currentEnv = loclEnv;

		while (depth != capture.Depth && currentEnv != NULL) {
			currentEnv = CoerceToEnvironment(currentEnv->Parent);
			depth++;
		}

		uf->Captures[capture.Dst]			  = currentEnv->Locals[capture.Src];
		uf->Captures[capture.Dst]->IsCaptured = true;
		uf->Captures[capture.Dst]->RefCount++;
	}

	return fn;
}

#undef PushArray
#undef GetOffset