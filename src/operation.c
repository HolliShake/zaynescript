#include "./operation.h"


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
 * @brief Pushes a value onto the interpreter's stack.
 * @param interpreter The interpreter instance.
 * @param value The value to push.
 * @origin src/interpreter.c:103
 */
extern void Push(Interpreter* interpreter, Value* value);

/**
 * @brief Pops and returns the top value from the interpreter's
 * stack.
 * @param interpreter The interpreter instance.
 * @return The popped value.
 * @origin src/interpreter.c:107
 */
extern Value* Popp(Interpreter* interpreter);

/**
 * @brief Pops N values from the interpreter's stack.
 * @param interpreter The interpreter instance.
 * @param n The number of values to pop.
 * @origin src/interpreter.c:111
 */
extern void PopN(Interpreter* interpreter, int n);

/**
 * @brief Peeks at the top value on the interpreter's stack
 * without removing it.
 * @param interpreter The interpreter instance.
 * @return The top value on the stack.
 * @origin src/interpreter.c:115
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
 * @brief Runs the interpreter's main execution loop on a
 * function value.
 * @param interpreter The interpreter instance.
 * @param fnValue The compiled function value to execute.
 * @origin src/interpreter.c:390
 */
extern void Run(Interpreter* interpreter, Value* fnValue);

/**
 * @brief Array of core module mappers for built-in module
 * resolution.
 * @origin src/core/loader.c:4
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
			String errMsg =
				FormatString("%s: string indices must be integers, not %s",
							 TYPE_ERROR,
							 ValueTypeOf(index));
			Value* errVal = NewErrorValue(interpreter, errMsg);
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
 * @brief Creates a new lexer for tokenizing source code.
 * @param path The path of the source file.
 * @param data The source code as a Rune array.
 * @return A new Lexer instance.
 * @origin src/lexer.c:305
 */
extern Lexer* CreateLexer(String filePath, Rune* data);

/**
 * @brief Frees a lexer and its associated resources.
 * @param lexer The lexer to free.
 * @origin src/lexer.c:376
 */
extern void FreeLexer(Lexer* lexer);

/**
 * @brief Creates a new parser from a lexer.
 * @param lexer The lexer to read tokens from.
 * @return A new Parser instance.
 * @origin src/parser.c:3
 */
extern Parser* CreateParser(Lexer* lexer);

/**
 * @brief Parses the token stream into an AST.
 * @param parser The parser instance.
 * @return The root AST node of the parsed program.
 * @origin src/parser.c:1817
 */
extern Ast* Parse(Parser* parser);

/**
 * @brief Frees a parser and its associated resources.
 * @param parser The parser to free.
 * @origin src/parser.c:1822
 */
extern void FreeParser(Parser* parser);

/**
 * @brief Frees an AST and all its child nodes.
 * @param ast The AST to free.
 * @origin src/astnode.c:285
 */
extern void FreeAst(Ast* ast);

/**
 * @brief Creates a new compiler from an interpreter and parser.
 * @param interpreter The interpreter instance.
 * @param parser The parser to read AST from.
 * @return A new Compiler instance.
 * @origin src/compiler.c:14
 */
extern Compiler* CreateCompiler(Interpreter* interpreter, Parser* parser);

/**
 * @brief Compiles an AST into a callable function value
 * (bytecode).
 * @param compiler The compiler instance.
 * @param programAst The AST to compile.
 * @return The compiled function value.
 * @origin src/compiler.c:3060
 */
extern Value* CompileAst(Compiler* compiler, Ast* programAst);

/**
 * @brief Frees a compiler and its associated resources.
 * @param compiler The compiler to free.
 * @origin src/compiler.c:3064
 */
extern void FreeCompiler(Compiler* compiler);

/**
 * @brief Interprets a compiled function value.
 * @param interpreter The interpreter instance.
 * @param compiled The compiled function value to interpret.
 * @origin src/interpreter.c:1434
 */
extern void Interpret(Interpreter* interpreter, Value* compiled);

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
	Value* instanceValue =
		NewClassInstanceValue(interpreter, CreateClassInstance(clsValue));
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
		interpreter->ActiveTask = fn;
		sm						= CoerceToStateMachine(fn);
		sm->StckBot				= interpreter->StckC;
		sm->EnvrBot				= interpreter->EnvrC;

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

		Value** args = Allocate(sizeof(Value*) * argc);
		args[0]		 = NULL;

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

	// 1. Save

	Value* oldRoot = interpreter->RootEnv;
	if (uf->Scope == NULL) {
		interpreter->RootEnv = en;
	}

	// 2. Run the function
	Run(interpreter, fn);

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
		Rune*  lhsRunes	 = (Rune*) lhs->Value.Opaque;
		Rune*  rhsRunes	 = (Rune*) rhs->Value.Opaque;
		String lhsStr	 = RunesStrToString(lhsRunes);
		String rhsStr	 = RunesStrToString(rhsRunes);
		size_t lhsLen	 = strlen(lhsStr);
		size_t rhsLen	 = strlen(rhsStr);
		String resultStr = Allocate(lhsLen + rhsLen + 1);
		memcpy(resultStr, lhsStr, lhsLen);
		memcpy(resultStr + lhsLen, rhsStr, rhsLen);
		resultStr[lhsLen + rhsLen] = '\0';
		result					   = NewStrValue(interpreter, resultStr);
		free(lhsStr);
		free(rhsStr);
		free(resultStr);
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