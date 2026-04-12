#include "./compiler.h"

#define PushArray(type, array, count, val, defaultValue)                       \
	do {                                                                       \
		(array)[count++] = val;                                                \
		(array)			 = Reallocate((array), sizeof(type) * ((count) + 1));  \
		(array)[count]	 = (defaultValue);                                     \
	} while (0)

#define GetOffset() (compiler->Interpreter->ConstantC)

Compiler* CreateCompiler(Interpreter* interpreter, Parser* parser) {
	Compiler* compiler	  = Allocate(sizeof(Compiler));
	compiler->Interpreter = interpreter;
	compiler->Parser	  = parser;
	compiler->ModulePath  = NULL;
	return compiler;
}

static void _InitModule(Compiler* compiler) {
	compiler->ModulePath = AbsolutePath(compiler->Parser->Lexer->Path);
}

static String _GetModule(Compiler* compiler) {
	return compiler->ModulePath;
}

static Position _ToFirstPosition(Position pos) {
	return PositionFromLineAndColm(pos.LineStart, pos.ColmStart);
}

static Position _ToLastPosition(Position pos) {
	return PositionFromLineAndColm(pos.LineEnded, pos.ColmEnded);
}

static bool _IsAstConstant(Compiler* compiler, Ast* node) {
	switch (node->Type) {
		case AST_INT:
		case AST_NUM:
		case AST_STR:
			return 1;
		case AST_MUL:
		case AST_DIV:
		case AST_MOD:
		case AST_ADD:
		case AST_SUB:
			return _IsAstConstant(compiler, node->A)
				   && _IsAstConstant(compiler, node->B);
		default:
			return 0;
	}
}

typedef struct jit_compile_job_struct {
	Interpreter* interpreter;
	Value*		 fn;
} JitCompileJob;

static JitCompileJob* _CreateJitCompileJob(Interpreter* interpreter,
										   Value*		fn) {
	JitCompileJob* job = Allocate(sizeof(JitCompileJob));
	job->interpreter   = interpreter;
	job->fn			   = fn;
	return job;
}

static void* _CompileJob(void* arg) {
	JitCompileJob* job = (JitCompileJob*) arg;
	ZJitCompile(job->interpreter, job->fn);
	free(job);
	return NULL;
}

static void _StartJitCompileJob(Compiler* compiler, JitCompileJob* job) {
	// UserFunction* uf = CoerceToUserFunction(job->fn);
	// if (uf->JitMode == JIT_DISABLED) {
	// 	free(job);
	// 	return;
	// }
	// if (ThreadStart(&compiler->Interpreter->JitThread,
	// 				_CompileJob,
	// 				(Value*) (void*) job)
	// 	!= 0) {
	// 	Panic("Failed to start JIT compilation thread!\n");
	// }
	// compiler->Interpreter->JitThreadActive = true;
}

static int _SaveFunction(Compiler* compiler, Value* fn) {
	int offset = compiler->Interpreter->FunctionC;
	PushArray(Value*,
			  compiler->Interpreter->Functions,
			  compiler->Interpreter->FunctionC,
			  fn,
			  NULL);
	return offset;
}

static int _SaveInt(Compiler* compiler, int val) {
	int offset = GetOffset();

	for (int i = 0; i < offset; i++) {
		Value* constantRaw = compiler->Interpreter->Constants[i];
		if (ValueIsInt(constantRaw) && CoerceToI32(constantRaw) == val) {
			return i;
		}
	}

	Value* newValue = NewIntValue(compiler->Interpreter, val);

	PushArray(Value*,
			  compiler->Interpreter->Constants,
			  compiler->Interpreter->ConstantC,
			  newValue,
			  NULL);

	return offset;
}

static int _SaveNum(Compiler* compiler, double val) {
	int offset = GetOffset();

	for (int i = 0; i < offset; i++) {
		Value* constantRaw = compiler->Interpreter->Constants[i];
		if (ValueIsNum(constantRaw) && CoerceToNum(constantRaw) == val) {
			return i;
		}
	}

	Value* newValue = NewNumValue(compiler->Interpreter, val);

	PushArray(Value*,
			  compiler->Interpreter->Constants,
			  compiler->Interpreter->ConstantC,
			  newValue,
			  NULL);

	return offset;
}

static int _SaveBigNum(Compiler* compiler, bf_t* val, bool i64plus) {
	int offset = GetOffset();

	for (int i = 0; i < offset; i++) {
		Value* constantRaw = compiler->Interpreter->Constants[i];
		String constantStr = ValueToString(constantRaw);
		String valStr	   = i64plus ? BFIntToString(val) : BFNumToString(val);
		if (((i64plus && ValueIsBigInt(constantRaw))
			 || (!i64plus && ValueIsBigNum(constantRaw)))
			&& strcmp(constantStr, valStr) == 0) {
			free(constantStr);
			free(valStr);
			return i;
		}
		free(constantStr);
		free(valStr);
	}

	Value* newValue = i64plus ? NewBigIntValue(compiler->Interpreter, val)
							  : NewBigNumValue(compiler->Interpreter, val);

	PushArray(Value*,
			  compiler->Interpreter->Constants,
			  compiler->Interpreter->ConstantC,
			  newValue,
			  NULL);

	return offset;
}

static int _SaveStr(Compiler* compiler, String val) {
	int offset = GetOffset();
	for (int i = 0; i < offset; i++) {
		Value* constantRaw = compiler->Interpreter->Constants[i];
		if (!ValueIsStr(constantRaw)) {
			continue;
		}
		String constantStr = ValueToString(constantRaw);
		bool   isEqual	   = strcmp(constantStr, val) == 0;
		free(constantStr);
		if (isEqual) {
			return i;
		}
	}

	Value* newValue = NewStrValue(compiler->Interpreter, val);

	PushArray(Value*,
			  compiler->Interpreter->Constants,
			  compiler->Interpreter->ConstantC,
			  newValue,
			  NULL);

	return offset;
}

static int _SaveConstantValue(Compiler* compiler, Value* val) {
	int offset = GetOffset();

	// Search if the constant already exists
	for (int i = 0; i < offset; i++) {
		Value* constant = compiler->Interpreter->Constants[i];
		if (ValueIsEqual(constant, val)) {
			return i;
		}
	}

	PushArray(Value*,
			  compiler->Interpreter->Constants,
			  compiler->Interpreter->ConstantC,
			  val,
			  NULL);

	return offset;
}

static Value* _GetConstantValue(Compiler* compiler, int offset) {
	if (offset < 0 || offset >= compiler->Interpreter->ConstantC) {
		Panic("Constant offset out of bounds %d (max %d)\n",
			  offset,
			  compiler->Interpreter->ConstantC);
	}
	return compiler->Interpreter->Constants[offset];
}

static void _Emit(Compiler* compiler, UserFunction* uf, OpcodeEnum opcode) {
	PushArray(uint8_t, uf->Codes, uf->CodeC, opcode, 0);
}

static void _EmitConst(Compiler*	 compiler,
					   UserFunction* uf,
					   OpcodeEnum	 opcode,
					   int			 offset) {
	uint8_t b1, b2, b3, b4;
	uf->Codes[uf->CodeC++] = opcode;
	uf->Codes = Reallocate(uf->Codes, sizeof(uint8_t) * (uf->CodeC + 5));
	b1		  = (offset >> 24) & 0xFF;
	b2		  = (offset >> 16) & 0xFF;
	b3		  = (offset >> 8) & 0xFF;
	b4		  = (offset >> 0) & 0xFF;
	uf->Codes[uf->CodeC++] = b1;
	uf->Codes[uf->CodeC++] = b2;
	uf->Codes[uf->CodeC++] = b3;
	uf->Codes[uf->CodeC++] = b4;
}

static void _EmitString(Compiler*	  compiler,
						UserFunction* uf,
						OpcodeEnum	  opcode,
						String		  str) {
	int length = strlen(str);
	_Emit(compiler, uf, opcode);
	for (int i = 0; i < length; i++) {
		_Emit(compiler, uf, (OpcodeEnum) str[i]);
	}
	_Emit(compiler, uf, (OpcodeEnum) '\0');
}

static void
_EmitArg(Compiler* compiler, UserFunction* uf, OpcodeEnum opcode, int index) {
	_EmitConst(compiler, uf, opcode, index);
}

static void _EmitLine(Compiler* compiler, UserFunction* uf, Position pos) {
	PushArray(LineInfo,
			  uf->Lines,
			  uf->LineC,
			  ((LineInfo){ .Path = _GetModule(compiler),
						   .Pc	 = uf->CodeC,
						   .Line = pos.LineStart }),
			  ((LineInfo){}));
}

static int
_EmitJumpTo(Compiler* compiler, UserFunction* uf, OpcodeEnum opcode) {
	int offset = uf->CodeC;
	_EmitConst(compiler, uf, opcode, 0);
	return offset;
}

static void
_JumpToLabel(Compiler* compiler, UserFunction* uf, int sourceOffset) {
	uint8_t b1, b2, b3, b4;
	int		offset				= uf->CodeC;
	b1							= (offset >> 24) & 0xFF;
	b2							= (offset >> 16) & 0xFF;
	b3							= (offset >> 8) & 0xFF;
	b4							= (offset >> 0) & 0xFF;
	uf->Codes[sourceOffset + 1] = b1;
	uf->Codes[sourceOffset + 2] = b2;
	uf->Codes[sourceOffset + 3] = b3;
	uf->Codes[sourceOffset + 4] = b4;
}

static void _JumpToAbsoluteLabel(Compiler*	   compiler,
								 UserFunction* uf,
								 int		   sourceOffset,
								 int		   targetOffset) {
	uint8_t b1, b2, b3, b4;
	int		offset				= targetOffset;
	b1							= (offset >> 24) & 0xFF;
	b2							= (offset >> 16) & 0xFF;
	b3							= (offset >> 8) & 0xFF;
	b4							= (offset >> 0) & 0xFF;
	uf->Codes[sourceOffset + 1] = b1;
	uf->Codes[sourceOffset + 2] = b2;
	uf->Codes[sourceOffset + 3] = b3;
	uf->Codes[sourceOffset + 4] = b4;
}

#define _CompileExpression(compiler, uf, scope, node)                          \
	_CompileExpressionMain(compiler, uf, scope, node, false)

static void _CompileStatement(Compiler*		compiler,
							  UserFunction* uf,
							  Scope*		scope,
							  Ast*			node);

static void
_CompileAssignOp(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* exp);
static void _CompileAugmentedAssignOp(Compiler*		compiler,
									  UserFunction* uf,
									  Scope*		scope,
									  Ast*			exp,
									  OpcodeEnum	opcode);
static void _CompileAssignOpRhs(Compiler*	  compiler,
								UserFunction* uf,
								Scope*		  scope,
								Ast*		  lhs,
								bool		  postfix);
static void _CompileAssignOpLhs(Compiler*	  compiler,
								UserFunction* uf,
								Scope*		  scope,
								Ast*		  lhs,
								bool		  postfix);

static void _CompileIdentifier(Compiler*	 compiler,
							   UserFunction* uf,
							   Scope*		 scope,
							   String		 name,
							   Position		 pos) {
	if (!ScopeHasName(scope, name)) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   pos,
				   "variable not found");
	}

	Symbol* symbol = ScopeGetSymbol(scope, name, true);

	if (ScopeInside(scope, SCOPE_FUNCTION) && !ScopeIsLocalToFn(scope, name)) {
		int captureOffset = 0;
		if (!ScopeHasCapture(scope, name)) {
			captureOffset =
				UserFunctionAddCapture(uf,
									   ScopeGetDepthOfSymbol(scope, name),
									   symbol->Offset);
			ScopeSetCapture(scope, name, false, true, false, captureOffset);
		} else {
			captureOffset = ScopeGetCapture(scope, name, true)->Offset;
		}

		// Capture the variable
		_EmitLine(compiler, uf, _ToFirstPosition(pos));
		_EmitArg(compiler, uf, OP_LOAD_CAPTURE, captureOffset);
		return;
	}

	_EmitLine(compiler, uf, _ToFirstPosition(pos));
	_EmitArg(compiler, uf, OP_LOAD_LOCAL, symbol->Offset);
}

static Value* _CompileExpressionMain(Compiler*	   compiler,
									 UserFunction* uf,
									 Scope*		   scope,
									 Ast*		   node,
									 bool		   evalOnly) {
	Value *lhs = NULL, *rhs = NULL, *val = NULL;
	int	   offset = 0;
	switch (node->Type) {
		case AST_NAME:
			{
				_CompileIdentifier(compiler,
								   uf,
								   scope,
								   node->Value,
								   node->Position);
				break;
			}
		case AST_INT:
			{
				long long lld = strtoll(node->Value, NULL, 10);
				offset		  = (lld > INT_MAX || lld < INT_MIN)
									? _SaveNum(compiler, (double) lld)
									: _SaveInt(compiler, (int) lld);
				val			  = _GetConstantValue(compiler, offset);
				if (!evalOnly) {
					_EmitLine(compiler, uf, node->Position);
					_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
				}
				break;
			}
		case AST_BINT:
			{
				bf_t* num = Allocate(sizeof(bf_t));
				bf_init(&(compiler->Interpreter->BfContext), num);
				bf_atof(num, node->Value, NULL, 10, BF_PREC_INF, BF_RNDZ);
				offset = _SaveBigNum(compiler, num, true);
				_EmitLine(compiler, uf, node->Position);
				_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
				break;
			}
		case AST_NUM:
			{
				offset = _SaveNum(compiler, strtod(node->Value, NULL));
				val	   = _GetConstantValue(compiler, offset);
				if (!evalOnly) {
					_EmitLine(compiler, uf, node->Position);
					_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
				}
				break;
			}
		case AST_BNUM:
			{
				bf_t* num = Allocate(sizeof(bf_t));
				bf_init(&(compiler->Interpreter->BfContext), num);
				bf_atof(num, node->Value, NULL, 10, BF_PREC_INF, BF_RNDZ);
				offset = _SaveBigNum(compiler, num, false);
				_EmitLine(compiler, uf, node->Position);
				_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
				break;
			}
		case AST_STR:
			{
				offset = _SaveStr(compiler, node->Value);
				val	   = _GetConstantValue(compiler, offset);
				if (!evalOnly) {
					_EmitLine(compiler, uf, node->Position);
					_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
				}
				break;
			}
		case AST_BOOL:
			{
				// Do not save boolean constants
				if (strcmp(node->Value, "true") == 0) {
					val = compiler->Interpreter->True;
				} else {
					val = compiler->Interpreter->False;
				}

				if (!evalOnly) {
					_EmitLine(compiler, uf, node->Position);
					_EmitArg(compiler, uf, OP_LOAD_BOOL, CoerceToBool(val));
				}
				break;
			}
		case AST_NULL:
			{
				val = compiler->Interpreter->Null;

				if (!evalOnly) {
					_EmitLine(compiler, uf, node->Position);
					_Emit(compiler, uf, OP_LOAD_NULL);
				}
				break;
			}
		case AST_THIS:
			{
				if (!(ScopeInside(scope, SCOPE_CLASS)
					  || ScopeInside(scope, SCOPE_FUNCTION))) {
					ThrowError(compiler->Parser->Lexer->Path,
							   compiler->Parser->Lexer->Data,
							   node->Position,
							   "'this' can only be used inside "
							   "class methods");
				}
				_CompileIdentifier(compiler,
								   uf,
								   scope,
								   KEY_THIS,
								   node->Position);
				break;
			}
		case AST_LIST_LITERAL:
			{
				Ast* elements  = node->A;
				bool hasSpread = false;
				int	 count	   = 0;
				while (elements != NULL) {
					switch (elements->Type) {
						case AST_SPREAD:
							{
								if (!hasSpread) {
									// Emit array with
									// number if elements
									_EmitLine(compiler, uf, node->Position);
									_EmitArg(compiler,
											 uf,
											 OP_ARRAY_MAKE,
											 count);
								}
								count	  = 0;
								hasSpread = true;
								_CompileExpression(compiler,
												   uf,
												   scope,
												   elements->A);
								_EmitLine(compiler, uf, node->Position);
								_Emit(compiler, uf, OP_ARRAY_EXTEND);
								break;
							}
						default:
							{
								if (!hasSpread)
									++count;
								_CompileExpression(compiler,
												   uf,
												   scope,
												   elements);
								if (hasSpread) {
									_EmitLine(compiler, uf, node->Position);
									_Emit(compiler, uf, OP_ARRAY_PUSH);
								}
								break;
							}
					}
					elements = elements->Next;
				}
				if (!hasSpread) {
					_EmitLine(compiler, uf, node->Position);
					_EmitArg(compiler, uf, OP_ARRAY_MAKE, count);
				}
				break;
			}
		case AST_OBJECT_LITERAL:
			{
				Ast *properties = node->A, *k = NULL, *v = NULL;
				bool hasSpread = false;
				int	 count	   = 0;
				while (properties != NULL) {
					switch (properties->Type) {
						case AST_SPREAD:
							{
								if (!hasSpread) {
									// Emit object with
									// number if pairs
									_EmitLine(compiler, uf, node->Position);
									_EmitArg(compiler,
											 uf,
											 OP_OBJECT_MAKE,
											 count);
								}
								count	  = 0;
								hasSpread = true;
								_CompileExpression(compiler,
												   uf,
												   scope,
												   properties->A);
								_EmitLine(compiler, uf, node->Position);
								_Emit(compiler, uf, OP_OBJECT_EXTEND);
								break;
							}
						case AST_NAME:
							{
								if (!hasSpread)
									++count;
								k = properties;
								v = properties;
								_CompileExpression(compiler, uf, scope, v);
								_EmitLine(compiler, uf, node->Position);
								_EmitString(compiler,
											uf,
											OP_LOAD_STRING,
											k->Value);
								if (hasSpread) {
									_EmitLine(compiler, uf, node->Position);
									_Emit(compiler, uf, OP_SET_INDEX);
								}
								break;
							}
						case AST_OBJECT_KEY_VAL:
							{
								if (!hasSpread)
									++count;
								k = properties->A;
								v = k->B;
								_CompileExpression(compiler, uf, scope, v);
								_EmitLine(compiler, uf, node->Position);
								_EmitString(compiler,
											uf,
											OP_LOAD_STRING,
											k->Value);
								if (hasSpread) {
									_EmitLine(compiler, uf, node->Position);
									_Emit(compiler, uf, OP_ROT2);
									_EmitLine(compiler, uf, node->Position);
									_Emit(compiler, uf, OP_SET_INDEX);
								}
								break;
							}
						default:
							{
								ThrowError(compiler->Parser->Lexer->Path,
										   compiler->Parser->Lexer->Data,
										   properties->Position,
										   "invalid object "
										   "property");
							}
					}
					properties = properties->Next;
				}
				if (!hasSpread) {
					// Emit object with number if pairs
					_EmitLine(compiler, uf, node->Position);
					_EmitArg(compiler, uf, OP_OBJECT_MAKE, count);
				}
				break;
			}
		case AST_FUNCTION:
			{
				Scope*	 fnScope  = CreateScope(SCOPE_FUNCTION, scope);
				Ast*	 params	  = node->B;
				Ast*	 body	  = node->C;
				Position nextLine = _ToFirstPosition(node->Position),
						 lastLine = _ToLastPosition(node->Position);

				UserFunction* fn = CreateUserFunction(NULL, 0, node->Flag);

				int paramc = 0;

				// Create array to store parameters in
				// reverse
				Ast* param = params;

				while (param != NULL) {
					nextLine = _ToFirstPosition(param->Position);
					if (ScopeHasLocal(fnScope, param->Value)) {
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   nextLine,
								   "duplicate parameter name");
					}

					int offset = UserFunctionEmitLocal(fn);

					ScopeSetSymbol(fnScope,
								   param->Value,
								   false,
								   true,
								   false,
								   offset);

					_EmitLine(compiler, fn, nextLine);
					_EmitArg(compiler, fn, OP_STORE_LOCAL, offset);

					param = param->Next;
					++paramc;
				}

				fn->Argc = paramc;

				while (body != NULL) {
					_CompileStatement(compiler, fn, fnScope, body);
					body = body->Next;
				}

				_EmitLine(compiler, fn, lastLine);
				_Emit(compiler, fn, OP_LOAD_NULL);
				_EmitLine(compiler, fn, lastLine);
				_Emit(compiler, fn, OP_RETURN);

				// Create the function
				Value* fnValue =
					NewUserFunctionValue(compiler->Interpreter, fn);
				int funcOffset = _SaveFunction(compiler, fnValue);

				_EmitLine(compiler, uf, lastLine);
				_EmitArg(compiler, uf, OP_LOAD_FUNCTION_CLOSURE, funcOffset);
				FreeScope(fnScope);

				JitCompileJob* job =
					_CreateJitCompileJob(compiler->Interpreter, fnValue);
				_StartJitCompileJob(compiler, job);

				break;
			}
		case AST_ALLOCATION:
			{
				Ast* cls	   = node->A;
				Ast* arguments = node->B;

				int argc = 0;
				// Count arguments first
				Ast* argCount = arguments;
				while (argCount != NULL) {
					argc++;
					argCount = argCount->Next;
				}

				// Emit arguments in reverse order
				Ast** argArray = Allocate(sizeof(Ast*) * argc);
				int	  i		   = 0;
				Ast*  arg	   = arguments;
				while (arg != NULL) {
					argArray[i++] = arg;
					arg			  = arg->Next;
				}
				for (int j = argc - 1; j >= 0; j--) {
					_CompileExpression(compiler, uf, scope, argArray[j]);
				}
				free(argArray);

				_CompileExpression(compiler, uf, scope, cls);
				_EmitLine(compiler, uf, node->Position);
				_EmitArg(compiler, uf, OP_CALL_CTOR, argc);
				break;
			}
		case AST_MEMBER:
			{
				Ast* objc = node->A;
				Ast* attr = node->B;
				_CompileExpression(compiler, uf, scope, objc);
				_EmitLine(compiler, uf, node->Position);
				_EmitString(compiler, uf, OP_LOAD_STRING, attr->Value);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_GET_INDEX);
				break;
			}
		case AST_INDEX:
			{
				Ast* objc = node->A;
				Ast* indx = node->B;
				_CompileExpression(compiler, uf, scope, objc);
				_CompileExpression(compiler, uf, scope, indx);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_GET_INDEX);
				break;
			}
		case AST_CALL:
			{
				Ast* objc = node->A;
				Ast* args = node->B;

				int argc = 0;

				switch (objc->Type) {
					case AST_MEMBER:
					case AST_INDEX:
						{
							Ast* obj = objc->A;
							Ast* att = objc->B;

							// Count arguments first
							Ast * arg = args, *head = arg;
							Ast** argsReverse = Allocate(sizeof(Ast*));

							argsReverse[argc++] = obj;
							argsReverse = Reallocate(argsReverse,
													 sizeof(Ast*) * (argc + 1));

							while (head != NULL) {
								argsReverse[argc++] = head;
								argsReverse =
									Reallocate(argsReverse,
											   sizeof(Ast*) * (argc + 1));
								head = head->Next;
							}

							for (int i = argc - 1; i >= 0; i--) {
								_CompileExpression(compiler,
												   uf,
												   scope,
												   argsReverse[i]);
							}

							free(argsReverse);

							_EmitLine(compiler, uf, node->Position);
							_Emit(compiler,
								  uf,
								  OP_DUPTOP);  // duplicate
											   // for 'this'

							if (objc->Type == AST_MEMBER) {
								_EmitLine(compiler, uf, node->Position);
								_EmitString(compiler,
											uf,
											OP_LOAD_STRING,
											att->Value);
							} else {
								_CompileExpression(compiler, uf, scope, att);
							}

							_EmitLine(compiler, uf, node->Position);
							_EmitArg(compiler, uf, OP_CALL_METHOD, argc);
							break;
						}
					default:
						{
							// Count arguments first
							Ast * arg = args, *head = arg;
							Ast** argsReverse = Allocate(sizeof(Ast*));
							while (head != NULL) {
								argsReverse[argc++] = head;
								argsReverse =
									Reallocate(argsReverse,
											   sizeof(Ast*) * (argc + 1));
								head = head->Next;
							}

							for (int i = argc - 1; i >= 0; i--) {
								_CompileExpression(compiler,
												   uf,
												   scope,
												   argsReverse[i]);
							}

							free(argsReverse);

							_CompileExpression(compiler, uf, scope, objc);
							_EmitLine(compiler, uf, node->Position);
							_EmitArg(compiler, uf, OP_CALL, argc);
							break;
						}
				}
				break;
			}
		case AST_LOGICAL_NOT:
			{
				_CompileExpression(compiler, uf, scope, node->A);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_NOT);
				break;
			}
		case AST_POSITIVE:
			{
				_CompileExpression(compiler, uf, scope, node->A);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_POS);
				break;
			}
		case AST_NEGATIVE:
			{
				_CompileExpression(compiler, uf, scope, node->A);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_NEG);
				break;
			}
		case AST_POST_INC:
			{
				_CompileAssignOpRhs(compiler, uf, scope, node->A, true);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_POSTINC);
				_CompileAssignOpLhs(compiler, uf, scope, node->A, true);
				break;
			}
		case AST_POST_DEC:
			{
				_CompileAssignOpRhs(compiler, uf, scope, node->A, true);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_POSTDEC);
				_CompileAssignOpLhs(compiler, uf, scope, node->A, true);
				break;
			}
		case AST_PRE_INC:
			{
				_CompileAssignOpRhs(compiler, uf, scope, node->A, false);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_INC);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_DUPTOP);
				_CompileAssignOpLhs(compiler, uf, scope, node->A, false);
				break;
			}
		case AST_PRE_DEC:
			{
				_CompileAssignOpRhs(compiler, uf, scope, node->A, false);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_DEC);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_DUPTOP);
				_CompileAssignOpLhs(compiler, uf, scope, node->A, false);
				break;
			}
		case AST_AWAIT:
			{
				_CompileExpression(compiler, uf, scope, node->A);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_AWAIT);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_GET_AWAITED_VALUE);
				break;
			}
		case AST_MUL:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoMul(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}
				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_MUL);
				break;
			}
		case AST_DIV:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoDiv(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}

				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_DIV);
				break;
			}
		case AST_MOD:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoMod(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}

				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_MOD);
				break;
			}
		case AST_ADD:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoAdd(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}
				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_ADD);
				break;
			}
		case AST_SUB:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoSub(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}

				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_SUB);
				break;
			}
		case AST_LSHFT:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoLShift(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}

				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_LSHFT);
				break;
			}
		case AST_RSHFT:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoRShift(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}
				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_RSHFT);
				break;
			}
		case AST_LT:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoLT(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}
				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_LT);
				break;
			}
		case AST_LTE:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoLTE(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}
				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_LTE);
				break;
			}
		case AST_GT:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoGT(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}
				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_GT);
				break;
			}
		case AST_GTE:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoGTE(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}
				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_GTE);
				break;
			}
		case AST_EQ:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoEQ(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}
				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_EQ);
				break;
			}
		case AST_NE:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoNE(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}
				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_NE);
				break;
			}
		case AST_AND:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoAnd(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}
				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_AND);
				break;
			}
		case AST_OR:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoOr(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}

				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_OR);
				break;
			}
		case AST_XOR:
			{
				if (_IsAstConstant(compiler, node)) {
					lhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->A,
												 true);
					rhs = _CompileExpressionMain(compiler,
												 uf,
												 scope,
												 node->B,
												 true);
					val = DoXor(compiler->Interpreter, lhs, rhs);
					if (ValueIsError(val)) {
						// Note: memory leak
						// (ValueToString(val) allocates a
						// string passed to ThrowError which
						// calls exit() — the string is
						// never freed)
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   node->Position,
								   ValueToString(val));
					}
					offset = _SaveConstantValue(compiler, val);
					if (!evalOnly) {
						_EmitLine(compiler, uf, node->Position);
						_EmitConst(compiler, uf, OP_LOAD_CONST, offset);
					}
					break;
				}
				lhs = _CompileExpression(compiler, uf, scope, node->A);
				rhs = _CompileExpression(compiler, uf, scope, node->B);
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_XOR);
				break;
			}
		case AST_LAND:
			{
				_CompileExpression(compiler, uf, scope, node->A);
				_EmitLine(compiler, uf, node->Position);
				int jumpOffset =
					_EmitJumpTo(compiler, uf, OP_JUMP_IF_FALSE_OR_POP);
				_CompileExpression(compiler, uf, scope, node->B);
				_JumpToLabel(compiler, uf, jumpOffset);
				break;
			}
		case AST_LOR:
			{
				_CompileExpression(compiler, uf, scope, node->A);
				_EmitLine(compiler, uf, node->Position);
				int jumpOffset =
					_EmitJumpTo(compiler, uf, OP_JUMP_IF_TRUE_OR_POP);
				_CompileExpression(compiler, uf, scope, node->B);
				_JumpToLabel(compiler, uf, jumpOffset);
				break;
			}
		case AST_SWITCH:
			{
				Ast* expr		 = node->A;
				Ast* cases		 = node->B;
				Ast* defaultCase = node->C;

				int	 endSwitchCapacity = 8;
				int	 endSwitchC		   = 0;
				int* gotoEndSwitch = Allocate(sizeof(int) * endSwitchCapacity);

				_CompileExpression(compiler, uf, scope, expr);

				while (cases != NULL) {
					Ast* caseExpr = cases->A;
					Ast* caseBody = cases->B;

					int	 casesMatchCapacity = 8;
					int	 casesMatchC		= 0;
					int* casesMatch =
						Allocate(sizeof(int) * casesMatchCapacity);

					// CASE:;
					Ast* currentExpr = caseExpr;
					while (currentExpr != NULL) {
						_EmitLine(compiler, uf, currentExpr->Position);
						_Emit(compiler, uf, OP_DUPTOP);

						// COMPARE
						_CompileExpression(compiler, uf, scope, currentExpr);
						_EmitLine(compiler, uf, currentExpr->Position);
						_Emit(compiler, uf, OP_EQ);

						// GOTO EXECUTE:;
						_EmitLine(compiler, uf, currentExpr->Position);
						if (casesMatchC >= casesMatchCapacity) {
							casesMatchCapacity *= 2;
							casesMatch =
								Reallocate(casesMatch,
										   sizeof(int) * casesMatchCapacity);
						}
						casesMatch[casesMatchC++] =
							_EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_TRUE);

						currentExpr = currentExpr->Next;
					}

					// GOTO NEXTCASE;
					_EmitLine(compiler, uf, caseExpr->Position);
					int jumpToNextCase = _EmitJumpTo(compiler, uf, OP_JUMP);

					for (int i = 0; i < casesMatchC; i++) {
						_JumpToLabel(compiler, uf, casesMatch[i]);
					}

					free(casesMatch);

					// CLEANUP: Pop expression (Match found)
					_EmitLine(compiler, uf, caseBody->Position);
					_Emit(compiler, uf, OP_POPTOP);

					// EXECUTE:;
					_CompileExpression(compiler, uf, scope, caseBody);

					// GOTO ENDSWITCH;
					_EmitLine(compiler, uf, node->Position);
					if (endSwitchC >= endSwitchCapacity) {
						endSwitchCapacity *= 2;
						gotoEndSwitch =
							Reallocate(gotoEndSwitch,
									   sizeof(int) * endSwitchCapacity);
					}
					gotoEndSwitch[endSwitchC++] =
						_EmitJumpTo(compiler, uf, OP_JUMP);

					// NEXTCASE;
					_JumpToLabel(compiler, uf, jumpToNextCase);
					cases = cases->Next;
				}

				// CLEANUP: Pop expression (No cases
				// matched, entering default)
				_EmitLine(compiler, uf, node->Position);
				_Emit(compiler, uf, OP_POPTOP);

				if (defaultCase != NULL) {
					_CompileExpression(compiler, uf, scope, defaultCase);
				} else {
					_EmitLine(compiler, uf, node->Position);
					_Emit(compiler, uf, OP_LOAD_NULL);
				}

				// ENDSWITCH:;
				for (int i = 0; i < endSwitchC; i++) {
					_JumpToLabel(compiler, uf, gotoEndSwitch[i]);
				}

				free(gotoEndSwitch);
				break;
			}
		case AST_TERNARY:
			{
				_CompileExpression(compiler, uf, scope, node->A);
				_EmitLine(compiler, uf, node->Position);
				int jumpOffset =
					_EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);
				_CompileExpression(compiler, uf, scope, node->B);
				_JumpToLabel(compiler, uf, jumpOffset);
				int jumpToEnd = _EmitJumpTo(compiler, uf, OP_JUMP);
				_JumpToLabel(compiler, uf, jumpOffset);
				_CompileExpression(compiler, uf, scope, node->C);
				_JumpToLabel(compiler, uf, jumpToEnd);
				break;
			}
		case AST_ASSIGN:
			{
				_CompileAssignOp(compiler, uf, scope, node);
				break;
			}
		case AST_MUL_ASSIGN:
			{
				_CompileAugmentedAssignOp(compiler, uf, scope, node, OP_MUL);
				break;
			}
		case AST_DIV_ASSIGN:
			{
				_CompileAugmentedAssignOp(compiler, uf, scope, node, OP_DIV);
				break;
			}
		case AST_MOD_ASSIGN:
			{
				_CompileAugmentedAssignOp(compiler, uf, scope, node, OP_MOD);
				break;
			}
		case AST_ADD_ASSIGN:
			{
				_CompileAugmentedAssignOp(compiler, uf, scope, node, OP_ADD);
				break;
			}
		case AST_SUB_ASSIGN:
			{
				_CompileAugmentedAssignOp(compiler, uf, scope, node, OP_SUB);
				break;
			}
		case AST_LSHFT_ASSIGN:
			{
				_CompileAugmentedAssignOp(compiler, uf, scope, node, OP_LSHFT);
				break;
			}
		case AST_RSHFT_ASSIGN:
			{
				_CompileAugmentedAssignOp(compiler, uf, scope, node, OP_RSHFT);
				break;
			}
		case AST_AND_ASSIGN:
			{
				_CompileAugmentedAssignOp(compiler, uf, scope, node, OP_AND);
				break;
			}
		case AST_OR_ASSIGN:
			{
				_CompileAugmentedAssignOp(compiler, uf, scope, node, OP_OR);
				break;
			}
		case AST_XOR_ASSIGN:
			{
				_CompileAugmentedAssignOp(compiler, uf, scope, node, OP_XOR);
				break;
			}
		default:
			{
				ThrowError(compiler->Parser->Lexer->Path,
						   compiler->Parser->Lexer->Data,
						   node->Position,
						   "expected an expression");
				break;
			}
	}
	return val;
}

static void
_CompileAssignOp(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* exp) {
	Ast* lhs = exp->A;
	Ast* rhs = exp->B;
	switch (lhs->Type) {
		case AST_NAME:
			{
				_CompileExpression(compiler, uf, scope, rhs);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_DUPTOP);
				if (!ScopeHasName(scope, lhs->Value)) {
					ThrowError(compiler->Parser->Lexer->Path,
							   compiler->Parser->Lexer->Data,
							   lhs->Position,
							   "variable not found");
				}
				Symbol* symbol = ScopeGetSymbol(scope, lhs->Value, true);
				if (symbol->IsConstant) {
					ThrowError(compiler->Parser->Lexer->Path,
							   compiler->Parser->Lexer->Data,
							   lhs->Position,
							   "cannot reassign constant "
							   "variable");
				}
				if (ScopeInside(scope, SCOPE_FUNCTION)
					&& !ScopeIsLocalToFn(scope, lhs->Value)) {
					int captureOffset = 0;
					if (!ScopeHasCapture(scope, lhs->Value)) {
						captureOffset = UserFunctionAddCapture(
							uf,
							ScopeGetDepthOfSymbol(scope, lhs->Value),
							symbol->Offset);
						ScopeSetCapture(scope,
										lhs->Value,
										false,
										true,
										false,
										captureOffset);
					} else {
						captureOffset =
							ScopeGetCapture(scope, lhs->Value, true)->Offset;
					}

					// Capture the variable
					_EmitLine(compiler, uf, lhs->Position);
					_EmitArg(compiler, uf, OP_STORE_CAPTURE, captureOffset);
				} else {
					_EmitLine(compiler, uf, lhs->Position);
					_EmitArg(compiler, uf, OP_STORE_LOCAL, symbol->Offset);
				}
				break;
			}
		case AST_MEMBER:
			{
				// bot [obj, key, val] top
				Ast* obj = lhs->A;
				Ast* att = lhs->B;
				_CompileExpression(compiler, uf, scope, obj);
				_EmitLine(compiler, uf, lhs->Position);
				_EmitString(compiler, uf, OP_LOAD_STRING, att->Value);
				_CompileExpression(compiler, uf, scope, rhs);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_DUPTOP);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_ROT4);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_SET_INDEX);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf,
					  OP_POPTOP);  // Pops object
				break;
			}
		case AST_INDEX:
			{
				// bot [obj, key, val] top
				Ast* obj = lhs->A;
				Ast* idx = lhs->B;
				_CompileExpression(compiler, uf, scope, obj);
				_CompileExpression(compiler, uf, scope, idx);
				_CompileExpression(compiler, uf, scope, rhs);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_DUPTOP);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_ROT4);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_SET_INDEX);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf,
					  OP_POPTOP);  // Pops object
				break;
			}
		default:
			{
				ThrowError(compiler->Parser->Lexer->Path,
						   compiler->Parser->Lexer->Data,
						   exp->Position,
						   "invalid assignment operation");
			}
	}
}

static void _CompileAugmentedAssignOp(Compiler*		compiler,
									  UserFunction* uf,
									  Scope*		scope,
									  Ast*			exp,
									  OpcodeEnum	opcode) {
	Ast* lhs = exp->A;
	Ast* rhs = exp->B;
	switch (lhs->Type) {
		case AST_NAME:
			{
				_CompileIdentifier(compiler,
								   uf,
								   scope,
								   lhs->Value,
								   lhs->Position);
				_CompileExpression(compiler, uf, scope, rhs);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, opcode);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_DUPTOP);
				if (!ScopeHasName(scope, lhs->Value)) {
					ThrowError(compiler->Parser->Lexer->Path,
							   compiler->Parser->Lexer->Data,
							   lhs->Position,
							   "variable not found");
				}
				Symbol* symbol = ScopeGetSymbol(scope, lhs->Value, true);
				if (symbol->IsConstant) {
					ThrowError(compiler->Parser->Lexer->Path,
							   compiler->Parser->Lexer->Data,
							   lhs->Position,
							   "cannot reassign constant "
							   "variable");
				}
				if (ScopeInside(scope, SCOPE_FUNCTION)
					&& !ScopeIsLocalToFn(scope, lhs->Value)) {
					int captureOffset = 0;
					if (!ScopeHasCapture(scope, lhs->Value)) {
						captureOffset = UserFunctionAddCapture(
							uf,
							ScopeGetDepthOfSymbol(scope, lhs->Value),
							symbol->Offset);
						ScopeSetCapture(scope,
										lhs->Value,
										false,
										true,
										false,
										captureOffset);
					} else {
						captureOffset =
							ScopeGetCapture(scope, lhs->Value, true)->Offset;
					}

					// Capture the variable
					_EmitLine(compiler, uf, lhs->Position);
					_EmitArg(compiler, uf, OP_STORE_CAPTURE, captureOffset);
				} else {
					_EmitLine(compiler, uf, lhs->Position);
					_EmitArg(compiler, uf, OP_STORE_LOCAL, symbol->Offset);
				}
				break;
			}
		case AST_MEMBER:
			{
				// bot [obj, key, val] top
				Ast* obj = lhs->A;
				Ast* att = lhs->B;
				// bot [obj, key] top
				_CompileExpression(compiler, uf, scope, obj);
				_EmitLine(compiler, uf, lhs->Position);
				_EmitString(compiler, uf, OP_LOAD_STRING, att->Value);
				// bot [obj, key, obj] top
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_DUP2);
				// bot [obj, key, val] top
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_GET_INDEX);
				// bot [obj, key, val, rhs] top
				_CompileExpression(compiler, uf, scope, rhs);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf,
					  opcode);	// *=,/=,%=,+=,-=,<<=,>>=,&=,|=,^=
				// bot [obj, key, val, val] top
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_DUPTOP);
				// bot [obj, key, val, val] top
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_ROT4);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_SET_INDEX);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf,
					  OP_POPTOP);  // Pops object
				break;
			}
		case AST_INDEX:
			{
				// bot [obj, key, val] top
				Ast* obj = lhs->A;
				Ast* att = lhs->B;
				// bot [obj, key] top
				_CompileExpression(compiler, uf, scope, obj);
				_CompileExpression(compiler, uf, scope, att);
				// bot [obj, key, obj] top
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_DUP2);
				// bot [obj, key, val] top
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_GET_INDEX);
				// bot [obj, key, val, rhs] top
				_CompileExpression(compiler, uf, scope, rhs);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf,
					  opcode);	// *=,/=,%=,+=,-=,<<=,>>=,&=,|=,^=
				// bot [obj, key, val, val] top
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_DUPTOP);
				// bot [obj, key, val, val] top
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_ROT4);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_SET_INDEX);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf,
					  OP_POPTOP);  // Pops object
				break;
			}
		default:
			{
				ThrowError(compiler->Parser->Lexer->Path,
						   compiler->Parser->Lexer->Data,
						   exp->Position,
						   "invalid assignment operation");
			}
	}
}

static void _CompileAssignOpRhs(Compiler*	  compiler,
								UserFunction* uf,
								Scope*		  scope,
								Ast*		  rhs,
								bool		  postfix) {
	switch (rhs->Type) {
		case AST_NAME:
			{
				_CompileExpression(compiler, uf, scope, rhs);
				break;
			}
		case AST_INDEX:
			{
				// bot [obj, key, obj, key, val] top
				Ast* obj = rhs->A;
				Ast* att = rhs->B;
				_CompileExpression(compiler, uf, scope, obj);
				_CompileExpression(compiler, uf, scope, att);
				_EmitLine(compiler, uf, rhs->Position);
				_Emit(compiler, uf, OP_DUP2);
				_EmitLine(compiler, uf, rhs->Position);
				_Emit(compiler, uf, OP_GET_INDEX);
				break;
			}
		case AST_MEMBER:
			{
				// bot [obj, key, obj, key,val] top
				Ast* obj = rhs->A;
				Ast* att = rhs->B;
				_CompileExpression(compiler, uf, scope, obj);
				_EmitLine(compiler, uf, rhs->Position);
				_EmitString(compiler, uf, OP_LOAD_STRING, att->Value);
				_EmitLine(compiler, uf, rhs->Position);
				_Emit(compiler, uf, OP_DUP2);
				_EmitLine(compiler, uf, rhs->Position);
				_Emit(compiler, uf, OP_GET_INDEX);
				break;
			}
		default:
			{
				ThrowError(compiler->Parser->Lexer->Path,
						   compiler->Parser->Lexer->Data,
						   rhs->Position,
						   "invalid left operand");
			}
	}
}

static void _CompileAssignOpLhs(Compiler*	  compiler,
								UserFunction* uf,
								Scope*		  scope,
								Ast*		  lhs,
								bool		  postfix) {
	switch (lhs->Type) {
		case AST_NAME:
			{
				if (postfix) {
					_EmitLine(compiler, uf, lhs->Position);
					_Emit(compiler, uf, OP_ROT2);
				}

				if (!ScopeHasName(scope, lhs->Value)) {
					ThrowError(compiler->Parser->Lexer->Path,
							   compiler->Parser->Lexer->Data,
							   lhs->Position,
							   "variable not found");
				}

				Symbol* symbol = ScopeGetSymbol(scope, lhs->Value, true);

				if (symbol->IsConstant) {
					ThrowError(compiler->Parser->Lexer->Path,
							   compiler->Parser->Lexer->Data,
							   lhs->Position,
							   "cannot reassign constant "
							   "variable");
				}

				if (ScopeInside(scope, SCOPE_FUNCTION)
					&& !ScopeIsLocalToFn(scope, lhs->Value)) {
					int captureOffset = 0;
					if (!ScopeHasCapture(scope, lhs->Value)) {
						captureOffset = UserFunctionAddCapture(
							uf,
							ScopeGetDepthOfSymbol(scope, lhs->Value),
							symbol->Offset);
						ScopeSetCapture(scope,
										lhs->Value,
										false,
										true,
										false,
										captureOffset);
					} else {
						captureOffset =
							ScopeGetCapture(scope, lhs->Value, true)->Offset;
					}

					// Capture the variable
					_EmitLine(compiler, uf, lhs->Position);
					_EmitArg(compiler, uf, OP_STORE_CAPTURE, captureOffset);
				} else {
					_EmitLine(compiler, uf, lhs->Position);
					_EmitArg(compiler, uf, OP_STORE_LOCAL, symbol->Offset);
				}
				break;
			}
		case AST_INDEX:
		case AST_MEMBER:
			{
				// bot [obj, key, val, val] top
				// After rotate4:
				// bot [val, obj, key, val] top
				// postfix:
				// bot [obj, key, val, old] top
				// after rotate4:
				// bot [old, obj, key, val] top
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_ROT4);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf, OP_SET_INDEX);
				_EmitLine(compiler, uf, lhs->Position);
				_Emit(compiler, uf,
					  OP_POPTOP);  // Pops object
				break;
			}
		default:
			{
				ThrowError(compiler->Parser->Lexer->Path,
						   compiler->Parser->Lexer->Data,
						   lhs->Position,
						   "invalid left operand");
			}
	}
}

static void _CompileClassDeclaration(Compiler*	   compiler,
									 UserFunction* uf,
									 Scope*		   scope,
									 Ast*		   node) {
	if (!ScopeIs(scope, SCOPE_GLOBAL)) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   node->Position,
				   "classes can only be declared at the "
				   "global scope");
	}

	Scope* classScope = CreateScope(SCOPE_CLASS, scope);

	Ast* className = node->A;
	Ast* super	   = node->B;
	Ast* body	   = node->C;

	// Assume its already forwarded
	Symbol* symbol = ScopeGetSymbol(scope, className->Value, false);

	if (symbol == NULL) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   className->Position,
				   "class not found");
	}

	int nameOffset = symbol->Offset;

	_EmitLine(compiler, uf, className->Position);
	_EmitString(compiler, uf, OP_CLASS_MAKE, className->Value);

	if (super != NULL) {
		_CompileExpression(compiler, uf, scope, super);
		_EmitLine(compiler, uf, super->Position);
		_Emit(compiler, uf, OP_CLASS_EXTEND);
	}

	while (body != NULL) {
		bool isStatic =
			strcmp(body->Value, "static") == 0;	 // instance or static
		Ast* actualBody = body->A;
		switch (actualBody->Type) {
			case AST_ASSIGN:
				{
					Ast* propName = actualBody->A;
					Ast* propVal  = actualBody->B;
					_CompileExpression(compiler, uf, scope, propVal);
					_EmitLine(compiler, uf, propVal->Position);
					_EmitString(compiler, uf, OP_LOAD_STRING, propName->Value);

					if (ScopeHasLocal(classScope, propName->Value)) {
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   propName->Position,
								   "duplicate class member name");
					}
					ScopeSetSymbol(classScope,
								   propName->Value,
								   false,
								   true,
								   false,
								   -1);
					break;
				}
			case AST_FUNCTION:
				{
					Scope*	 fnScope  = CreateScope(SCOPE_FUNCTION, scope);
					Ast*	 fnName	  = actualBody->A;
					Ast*	 params	  = actualBody->B;
					Ast*	 body	  = actualBody->C;
					Position nextLine = _ToFirstPosition(actualBody->Position),
							 lastLine = _ToLastPosition(actualBody->Position);

					UserFunction* fn =
						CreateUserFunction(fnName->Value, 0, actualBody->Flag);

					Ast* param	= params;
					int	 paramc = 0;

					if (!isStatic) {
						// Emit 'this' as the first
						// parameter
						int offset = UserFunctionEmitLocal(fn);
						ScopeSetSymbol(fnScope,
									   KEY_THIS,
									   false,
									   true,
									   false,
									   offset);
						_EmitLine(compiler, fn, nextLine);
						_EmitArg(compiler, fn, OP_STORE_LOCAL, offset);
						paramc++;
					}

					while (param != NULL) {
						nextLine = _ToFirstPosition(param->Position);
						if (ScopeHasLocal(fnScope, param->Value)) {
							ThrowError(compiler->Parser->Lexer->Path,
									   compiler->Parser->Lexer->Data,
									   nextLine,
									   "duplicate parameter name");
						}

						int offset = UserFunctionEmitLocal(fn);

						ScopeSetSymbol(fnScope,
									   param->Value,
									   false,
									   true,
									   false,
									   offset);

						_EmitLine(compiler, fn, nextLine);
						_EmitArg(compiler, fn, OP_STORE_LOCAL, offset);
						param = param->Next;
						paramc++;
					}

					fn->Argc = paramc;

					while (body != NULL) {
						_CompileStatement(compiler, fn, fnScope, body);
						body = body->Next;
					}

					_EmitLine(compiler, fn, lastLine);
					_Emit(compiler, fn, OP_LOAD_NULL);
					_EmitLine(compiler, fn, lastLine);
					_Emit(compiler, fn, OP_RETURN);

					// Create the function
					Value* fnValue =
						NewUserFunctionValue(compiler->Interpreter, fn);
					int funcOffset = _SaveFunction(compiler, fnValue);

					_EmitLine(compiler, uf, lastLine);
					_EmitArg(compiler, uf, OP_LOAD_FUNCTION, funcOffset);
					_EmitLine(compiler, uf, lastLine);
					_EmitString(compiler, uf, OP_LOAD_STRING, fnName->Value);

					if (ScopeHasLocal(classScope, fnName->Value)) {
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   fnName->Position,
								   "duplicate class member name");
					}

					ScopeSetSymbol(classScope,
								   fnName->Value,
								   false,
								   true,
								   false,
								   -1);
					FreeScope(fnScope);


					JitCompileJob* job =
						_CreateJitCompileJob(compiler->Interpreter, fnValue);
					_StartJitCompileJob(compiler, job);

					break;
				}
			default:
				{
					ThrowError(compiler->Parser->Lexer->Path,
							   compiler->Parser->Lexer->Data,
							   body->Position,
							   "invalid class body element");
				}
		}

		_EmitLine(compiler, uf, _ToLastPosition(actualBody->Position));
		_Emit(compiler,
			  uf,
			  isStatic ? OP_CLASS_DEFINE_STATIC_MEMBER
					   : OP_CLASS_DEFINE_INSTANCE_MEMBER);
		body = body->Next;
	}

	// End class
	_EmitLine(compiler, uf, _ToLastPosition(node->Position));
	_EmitArg(compiler, uf, OP_STORE_NAME, nameOffset);
	FreeScope(classScope);
}

static void _CompileFunctionDeclaration(Compiler*	  compiler,
										UserFunction* uf,
										Scope*		  scope,
										Ast*		  node) {
	if (!ScopeIs(scope, SCOPE_GLOBAL)) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   node->Position,
				   "functions can only be declared at the "
				   "global scope");
	}

	Scope*	 fnScope  = CreateScope(SCOPE_FUNCTION, scope);
	Ast*	 fnName	  = node->A;
	Ast*	 params	  = node->B;
	Ast*	 body	  = node->C;
	Position nextLine = _ToFirstPosition(node->Position),
			 lastLine = _ToLastPosition(node->Position);

	// Assume its already forwarded
	Symbol* symbol = ScopeGetSymbol(scope, fnName->Value, false);

	if (symbol == NULL) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   fnName->Position,
				   "function not found");
	}

	int nameOffset = symbol->Offset;

	UserFunction* fn = CreateUserFunction(fnName->Value, 0, node->Flag);

	int paramc = 0;
	while (params != NULL) {
		nextLine = _ToFirstPosition(params->Position);
		if (ScopeHasLocal(fnScope, params->Value)) {
			ThrowError(compiler->Parser->Lexer->Path,
					   compiler->Parser->Lexer->Data,
					   nextLine,
					   "duplicate parameter name");
		}

		int offset = UserFunctionEmitLocal(fn);

		ScopeSetSymbol(fnScope, params->Value, false, true, false, offset);

		_EmitLine(compiler, fn, nextLine);
		_EmitArg(compiler, fn, OP_STORE_LOCAL, offset);
		paramc++;
		params = params->Next;
	}

	fn->Argc = paramc;

	while (body != NULL) {
		_CompileStatement(compiler, fn, fnScope, body);
		body = body->Next;
	}

	_EmitLine(compiler, fn, lastLine);
	_Emit(compiler, fn, OP_LOAD_NULL);
	_EmitLine(compiler, fn, lastLine);
	_Emit(compiler, fn, OP_RETURN);

	// Create the function
	Value* fnValue	  = NewUserFunctionValue(compiler->Interpreter, fn);
	int	   funcOffset = _SaveFunction(compiler, fnValue);

	_EmitLine(compiler, uf, lastLine);
	_EmitArg(compiler, uf, OP_LOAD_FUNCTION, funcOffset);
	_EmitLine(compiler, uf, lastLine);
	_EmitArg(compiler, uf, OP_STORE_NAME, nameOffset);
	FreeScope(fnScope);

	JitCompileJob* job = _CreateJitCompileJob(compiler->Interpreter, fnValue);
	_StartJitCompileJob(compiler, job);
}

static void _CompileImportStatement(Compiler*	  compiler,
									UserFunction* uf,
									Scope*		  scope,
									Ast*		  node) {
	if (!ScopeIs(scope, SCOPE_GLOBAL)) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   node->Position,
				   "imports can only be declared at the "
				   "global scope");
	}

	Ast* imports	= node->A;
	Ast* moduleName = node->B;

	bool isTryingRelative = (!StringStartsWith(moduleName->Value, "core:")
							 && !StringStartsWith(moduleName->Value, "lib:"))
							&& (StringStartsWith(moduleName->Value, "./")
								|| StringStartsWith(moduleName->Value, "../"));

	// Validate, should not contain spaces, dots, or special
	// characters
	if (strpbrk(moduleName->Value, " .><=!@#$%^&*()_+-=[]{}|\\;'\"`~") != NULL
		&& !isTryingRelative) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   moduleName->Position,
				   "invalid module name, should not "
				   "contain spaces, dots, or "
				   "special characters");
	}

	String modulePrefix = NULL;
	int	   prefixLen	= 0;
	int8_t type			= 0;

	if (StringStartsWith(moduleName->Value, "core:")) {
		type		 = OP_IMPORT_CORE;
		modulePrefix = moduleName->Value + 5;
		prefixLen	 = 5;
	} else if (StringStartsWith(moduleName->Value, "lib:")) {
		type		 = OP_IMPORT_LIB;
		modulePrefix = moduleName->Value + 4;
		prefixLen	 = 4;
	} else if (isTryingRelative) {
		type		   = OP_IMPORT_RELATIVE;
		String fileDir = Dirname(_GetModule(compiler));
		modulePrefix   = AbsolutePathFromBase(fileDir, moduleName->Value);
		free(fileDir);
	} else {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   moduleName->Position,
				   "invalid module name, expected 'core:' "
				   "or 'lib:' prefix");
	}

	_EmitLine(compiler, uf, moduleName->Position);
	_EmitString(compiler, uf, (OpcodeEnum) type, modulePrefix);

	if (imports == NULL) {
		// store as object
		_EmitLine(compiler, uf, moduleName->Position);
		_Emit(compiler, uf, OP_DUPTOP);

		if (ScopeHasLocal(scope, modulePrefix)) {
			ThrowError(compiler->Parser->Lexer->Path,
					   compiler->Parser->Lexer->Data,
					   moduleName->Position,
					   "duplicate import name");
		}

		String cleanName = Basename(modulePrefix);

		int offset = UserFunctionEmitLocal(uf);
		ScopeSetSymbol(scope, cleanName, true, true, true, offset);
		_EmitLine(compiler, uf, moduleName->Position);
		_EmitArg(compiler, uf, OP_STORE_NAME, offset);

		free(cleanName);
	}

	if (type == OP_IMPORT_RELATIVE) {
		free(modulePrefix);
	}

	while (imports != NULL) {
		String attributeName = imports->Value;

		_EmitLine(compiler, uf, imports->Position);
		_EmitString(compiler, uf, OP_OBJECT_PLUCK_ATTRIBUTE, attributeName);

		if (ScopeHasLocal(scope, attributeName)) {
			ThrowError(compiler->Parser->Lexer->Path,
					   compiler->Parser->Lexer->Data,
					   imports->Position,
					   "duplicate import name");
		}

		int offset = UserFunctionEmitLocal(uf);
		_EmitLine(compiler, uf, imports->Position);
		_EmitArg(compiler, uf, OP_STORE_NAME, offset);
		ScopeSetSymbol(scope, attributeName, true, true, false, offset);

		imports = imports->Next;
	}

	// Pop the module object
	_EmitLine(compiler, uf, _ToLastPosition(node->Position));
	_Emit(compiler, uf, OP_POPTOP);
}

static void _CompileVarDeclarationStatement(Compiler*	  compiler,
											UserFunction* uf,
											Scope*		  scope,
											Ast*		  node) {
	if (!ScopeIs(scope, SCOPE_GLOBAL)) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   node->Position,
				   "variables can only be declared at the "
				   "global scope");
	}
	Ast* declarations = node->A;
	while (declarations != NULL) {
		if (declarations->B != NULL) {
			_CompileExpression(compiler, uf, scope, declarations->B);
		} else {
			_EmitLine(compiler, uf, declarations->Position);
			_Emit(compiler, uf, OP_LOAD_NULL);
		}

		int offset = UserFunctionEmitLocal(uf);

		_EmitLine(compiler, uf, declarations->Position);
		_EmitArg(compiler, uf, OP_STORE_NAME, offset);

		if (ScopeHasLocal(scope, declarations->Value)) {
			ThrowError(compiler->Parser->Lexer->Path,
					   compiler->Parser->Lexer->Data,
					   declarations->Position,
					   "duplicate variable name");
		}

		ScopeSetSymbol(scope, declarations->Value, true, true, false, offset);

		declarations = declarations->Next;
	}
}

static void _CompileLocalDeclarationStatement(Compiler*		compiler,
											  UserFunction* uf,
											  Scope*		scope,
											  Ast*			node) {
	if (!(ScopeIs(scope, SCOPE_FUNCTION) || ScopeIs(scope, SCOPE_BLOCK)
		  || ScopeIs(scope, SCOPE_TRY_BLOCK))) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   node->Position,
				   "local declarations can only be used "
				   "inside a function or a block");
	}
	Ast* declarations = node->A;
	while (declarations != NULL) {
		if (declarations->B != NULL) {
			_CompileExpression(compiler, uf, scope, declarations->B);
		} else {
			_EmitLine(compiler, uf, declarations->Position);
			_Emit(compiler, uf, OP_LOAD_NULL);
		}

		int offset = UserFunctionEmitLocal(uf);
		_EmitLine(compiler, uf, declarations->Position);
		_EmitArg(compiler, uf, OP_STORE_LOCAL, offset);

		if (ScopeHasLocal(scope, declarations->Value)) {
			ThrowError(compiler->Parser->Lexer->Path,
					   compiler->Parser->Lexer->Data,
					   declarations->Position,
					   "duplicate variable name");
		}

		ScopeSetSymbol(scope, declarations->Value, false, true, false, offset);
		declarations = declarations->Next;
	}
}

static void _CompileConstDeclarationStatement(Compiler*		compiler,
											  UserFunction* uf,
											  Scope*		scope,
											  Ast*			node) {
	if (!(ScopeIs(scope, SCOPE_GLOBAL) || ScopeIs(scope, SCOPE_FUNCTION)
		  || ScopeIs(scope, SCOPE_BLOCK) || ScopeIs(scope, SCOPE_TRY_BLOCK))) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   node->Position,
				   "const declarations can only be used "
				   "inside a function or a block");
	}
	Ast* declarations = node->A;
	while (declarations != NULL) {
		if (declarations->B != NULL) {
			_CompileExpression(compiler, uf, scope, declarations->B);
		} else {
			_EmitLine(compiler, uf, declarations->Position);
			_Emit(compiler, uf, OP_LOAD_NULL);
		}

		int offset = UserFunctionEmitLocal(uf);
		_EmitLine(compiler, uf, declarations->Position);
		_EmitArg(compiler,
				 uf,
				 ScopeIs(scope, SCOPE_GLOBAL) ? OP_STORE_NAME : OP_STORE_LOCAL,
				 offset);

		if (ScopeHasLocal(scope, declarations->Value)) {
			ThrowError(compiler->Parser->Lexer->Path,
					   compiler->Parser->Lexer->Data,
					   declarations->Position,
					   "duplicate variable name");
		}

		ScopeSetSymbol(scope,
					   declarations->Value,
					   ScopeIs(scope, SCOPE_GLOBAL),
					   ScopeGetFirst(scope, SCOPE_FUNCTION) != NULL,
					   true,
					   offset);
		declarations = declarations->Next;
	}
}

static void _CompileInitializerConditionMutator(Compiler*	  compiler,
												UserFunction* uf,
												Scope*		  scope,
												Ast*		  node,
												int*		  outputVar) {
	Ast *lhs = node->A, *rhs = node->B;
	if (node->Type == AST_SHORT_ASSIGN) {
		_CompileExpression(compiler, uf, scope, rhs);
		if (ScopeHasLocal(scope, lhs->Value)) {
			ThrowError(compiler->Parser->Lexer->Path,
					   compiler->Parser->Lexer->Data,
					   lhs->Position,
					   "variable not found");
		}
		int offset = UserFunctionEmitLocal(uf);
		ScopeSetSymbol(scope, lhs->Value, false, true, false, offset);
		_EmitLine(compiler, uf, lhs->Position);
		_EmitArg(compiler, uf, OP_STORE_LOCAL, offset);
		*outputVar = offset;
		return;
	}
	_CompileExpression(compiler, uf, scope, node);
}

static void _CompileIfStatement(Compiler*	  compiler,
								UserFunction* uf,
								Scope*		  scope,
								Ast*		  node) {
	Ast*	 initializer = node->A;
	Ast*	 condition	 = initializer->Next;
	Ast*	 thenBranch	 = node->B;
	Ast*	 elseBranch	 = node->C;
	int		 ifVar		 = -1;
	Position nextLine	 = _ToFirstPosition(node->Position);

	// IFSTART:;
	if (initializer != NULL && condition != NULL) {
		nextLine = _ToFirstPosition(initializer->Position);
		_CompileInitializerConditionMutator(compiler,
											uf,
											scope,
											initializer,
											&ifVar);
		_CompileExpression(compiler, uf, scope, condition);
	} else if (initializer != NULL) {
		nextLine = _ToFirstPosition(initializer->Position);
		// use initializer as condition
		_CompileInitializerConditionMutator(compiler,
											uf,
											scope,
											initializer,
											&ifVar);
	} else {
		nextLine = _ToFirstPosition(condition->Position);
		_CompileExpression(compiler, uf, scope, condition);
	}

	// goto: ELSE
	_EmitLine(compiler, uf, nextLine);
	int labelELSE = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);

	// THEN:;
	_CompileStatement(compiler, uf, scope, thenBranch);

	nextLine = _ToLastPosition(thenBranch->Position);

	// goto: ENDIF
	_EmitLine(compiler, uf, nextLine);
	int labelENDIF = _EmitJumpTo(compiler, uf, OP_JUMP);

	// ELSE:;
	_JumpToLabel(compiler, uf, labelELSE);
	if (elseBranch != NULL) {
		// else branch is always in the outside scope
		_CompileStatement(compiler, uf, scope, elseBranch);
	}

	// ENDIF:;
	_JumpToLabel(compiler, uf, labelENDIF);

	// Close the iteration variable scope if it exists
	if (ifVar != -1) {
		_EmitLine(compiler, uf, nextLine);
		_EmitArg(compiler, uf, OP_LOCK_VAR, ifVar);
	}
}

static void _CompileSwitchStatement(Compiler*	  compiler,
									UserFunction* uf,
									Scope*		  scope,
									Ast*		  node) {
	Ast*	 expr		 = node->A;
	Ast*	 cases		 = node->B;
	Ast*	 defaultCase = node->C;
	Position nextLine	 = _ToFirstPosition(expr->Position);

	// Use a capacity-doubling strategy for better
	// performance
	int	 endSwitchCapacity = 8;
	int	 endSwitchC		   = 0;
	int* gotoEndSwitch	   = Allocate(sizeof(int) * endSwitchCapacity);

	// 1. Compile the main switch expression and leave it on
	// the stack
	_CompileExpression(compiler, uf, scope, expr);

	while (cases != NULL) {
		Ast* caseExpr = cases->A;
		Ast* caseBody = cases->B;

		int	 casesMatchCapacity = 8;
		int	 casesMatchC		= 0;
		int* casesMatch			= Allocate(sizeof(int) * casesMatchCapacity);

		// CASE:;
		Ast* currentExpr = caseExpr;
		while (currentExpr != NULL) {
			nextLine = _ToFirstPosition(currentExpr->Position);

			_EmitLine(compiler, uf, nextLine);
			_Emit(compiler,
				  uf,
				  OP_DUPTOP);  // Duplicate the switch
							   // expression

			// COMPARE
			_CompileExpression(compiler, uf, scope, currentExpr);
			_EmitLine(compiler, uf, nextLine);
			_Emit(compiler, uf, OP_EQ);

			// GOTO EXECUTE:;
			_EmitLine(compiler, uf, nextLine);

			// Check capacity before adding to avoid O(N^2)
			// reallocation overhead
			if (casesMatchC >= casesMatchCapacity) {
				casesMatchCapacity *= 2;
				casesMatch =
					Reallocate(casesMatch, sizeof(int) * casesMatchCapacity);
			}
			casesMatch[casesMatchC++] =
				_EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_TRUE);

			currentExpr = currentExpr->Next;
		}

		// GOTO NEXTCASE;
		_EmitLine(compiler, uf, nextLine);
		int jumpToNextCase = _EmitJumpTo(compiler, uf, OP_JUMP);

		// EXECUTE: Bind the match jumps to the start of
		// this case body
		for (int i = 0; i < casesMatchC; i++) {
			_JumpToLabel(compiler, uf, casesMatch[i]);
		}

		free(casesMatch);

		// CLEANUP
		// Pop expression
		_EmitLine(compiler, uf, nextLine);
		_Emit(compiler, uf, OP_POPTOP);

		// Compile the actual body of the case
		_CompileStatement(compiler, uf, scope, caseBody);

		nextLine = _ToLastPosition(caseBody->Position);

		// GOTO ENDSWITCH; (No fallthrough)
		_EmitLine(compiler, uf, nextLine);

		// Check capacity before adding
		if (endSwitchC >= endSwitchCapacity) {
			endSwitchCapacity *= 2;
			gotoEndSwitch =
				Reallocate(gotoEndSwitch, sizeof(int) * endSwitchCapacity);
		}
		gotoEndSwitch[endSwitchC++] = _EmitJumpTo(compiler, uf, OP_JUMP);

		// NEXTCASE; Bind the jump to the next iteration
		// here
		_JumpToLabel(compiler, uf, jumpToNextCase);

		cases = cases->Next;
	}

	// CLEANUP
	// Pop expression
	_EmitLine(compiler, uf, nextLine);
	_Emit(compiler, uf, OP_POPTOP);

	// Compile default case if it exists
	if (defaultCase != NULL) {
		_CompileStatement(compiler, uf, scope, defaultCase);
	}

	// ENDSWITCH:; Bind all successful case completions to
	// here
	for (int i = 0; i < endSwitchC; i++) {
		_JumpToLabel(compiler, uf, gotoEndSwitch[i]);
	}

	free(gotoEndSwitch);
}

static void _CompileForStatement(Compiler*	   compiler,
								 UserFunction* uf,
								 Scope*		   scope,
								 Ast*		   node) {
	Scope* loopScope   = CreateScope(SCOPE_LOOP, scope);
	Ast *  initializer = node->A,
		*condition	   = (initializer && initializer->Next) ? initializer->Next
															: initializer,
		*mutator	= (condition && condition->Next) ? condition->Next : NULL,
		*thenBranch = node->B;
	int	 loopVar	= -1;
	bool hasLocalInitializer =
		initializer != NULL && initializer->Type == AST_SHORT_ASSIGN;
	Position nextLine = _ToFirstPosition(node->Position);

	if (hasLocalInitializer) {
		nextLine = _ToFirstPosition(initializer->Position);
		_CompileInitializerConditionMutator(compiler,
											uf,
											loopScope,
											initializer,
											&loopVar);
	}

	// FORSTART:;
	int labelFORSTART = uf->CodeC;
	int labelENDFOR	  = -1;

	if (condition != NULL) {
		nextLine = _ToFirstPosition(condition->Position);
		_CompileExpression(compiler, uf, loopScope, condition);
		_EmitLine(compiler, uf, nextLine);
		labelENDFOR = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);
	}

	// THEN:;
	_CompileStatement(compiler, uf, loopScope, thenBranch);

	// Close the iteration variable scope if it exists
	if (loopVar != -1) {
		_EmitLine(compiler, uf, nextLine);
		_EmitArg(compiler, uf, OP_LOCK_VAR, loopVar);
	}

	nextLine = _ToLastPosition(thenBranch->Position);

	if (mutator != NULL) {
		// goto: MUTATOR
		for (int i = 0; i < loopScope->ContinueJumpC; i++) {
			_JumpToLabel(compiler, uf, loopScope->ContinueJumps[i]);
		}

		// MUTATOR:;
		_CompileExpression(compiler, uf, loopScope, mutator);
		_EmitLine(compiler, uf, nextLine);
		_Emit(compiler, uf, OP_POPTOP);
	} else {
		// goto: FORSTART
		for (int i = 0; i < loopScope->ContinueJumpC; i++) {
			_JumpToAbsoluteLabel(compiler,
								 uf,
								 loopScope->ContinueJumps[i],
								 labelFORSTART);
		}
	}

	// goto: FORSTART
	_EmitLine(compiler, uf, nextLine);
	_JumpToAbsoluteLabel(compiler,
						 uf,
						 _EmitJumpTo(compiler, uf, OP_ABSOLUTE_JUMP),
						 labelFORSTART);

	// ENDFOR:;
	for (int i = 0; i < loopScope->BreakJumpC; i++) {
		_JumpToLabel(compiler, uf, loopScope->BreakJumps[i]);
	}

	if (labelENDFOR != -1)
		_JumpToLabel(compiler, uf, labelENDFOR);

	FreeScope(loopScope);
}

static void _CompileWhileStatement(Compiler*	 compiler,
								   UserFunction* uf,
								   Scope*		 scope,
								   Ast*			 node) {
	Scope* loopScope   = CreateScope(SCOPE_LOOP, scope);
	Ast *  initializer = node->A,
		*condition	   = (initializer && initializer->Next) ? initializer->Next
															: initializer,
		*mutator	= (condition && condition->Next) ? condition->Next : NULL,
		*thenBranch = node->B;
	int	 loopVar	= -1;
	bool hasLocalInitializer =
		initializer != NULL && initializer->Type == AST_SHORT_ASSIGN;
	Position nextLine = _ToFirstPosition(node->Position);

	if (hasLocalInitializer) {
		nextLine = _ToFirstPosition(initializer->Position);
		_CompileInitializerConditionMutator(compiler,
											uf,
											loopScope,
											initializer,
											&loopVar);
	}

	// FORSTART:;
	int labelFORSTART = uf->CodeC;
	int labelENDFOR	  = -1;

	if (condition != NULL) {
		nextLine = _ToFirstPosition(condition->Position);
		_CompileExpression(compiler, uf, loopScope, condition);
		_EmitLine(compiler, uf, nextLine);
		labelENDFOR = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);
	}

	// THEN:;
	_CompileStatement(compiler, uf, loopScope, thenBranch);

	// Close the iteration variable scope if it exists
	if (loopVar != -1) {
		_EmitLine(compiler, uf, nextLine);
		_EmitArg(compiler, uf, OP_LOCK_VAR, loopVar);
	}

	nextLine = _ToLastPosition(thenBranch->Position);

	if (mutator != NULL) {
		// goto: MUTATOR
		for (int i = 0; i < loopScope->ContinueJumpC; i++) {
			_JumpToLabel(compiler, uf, loopScope->ContinueJumps[i]);
		}

		// MUTATOR:;
		_CompileExpression(compiler, uf, loopScope, mutator);
		_EmitLine(compiler, uf, nextLine);
		_Emit(compiler, uf, OP_POPTOP);
	} else {
		// goto: FORSTART
		for (int i = 0; i < loopScope->ContinueJumpC; i++) {
			_JumpToAbsoluteLabel(compiler,
								 uf,
								 loopScope->ContinueJumps[i],
								 labelFORSTART);
		}
	}

	// goto: FORSTART
	_EmitLine(compiler, uf, nextLine);
	_JumpToAbsoluteLabel(compiler,
						 uf,
						 _EmitJumpTo(compiler, uf, OP_ABSOLUTE_JUMP),
						 labelFORSTART);

	// ENDFOR:;
	for (int i = 0; i < loopScope->BreakJumpC; i++) {
		_JumpToLabel(compiler, uf, loopScope->BreakJumps[i]);
	}

	if (labelENDFOR != -1)
		_JumpToLabel(compiler, uf, labelENDFOR);

	FreeScope(loopScope);
}

static void _CompileDoWhileStatement(Compiler*	   compiler,
									 UserFunction* uf,
									 Scope*		   scope,
									 Ast*		   node) {
	Scope*	 loopScope	= CreateScope(SCOPE_LOOP, scope);
	Ast*	 condition	= node->A;
	Ast*	 thenBranch = node->B;
	Position nextLine	= _ToFirstPosition(node->Position);

	// DOSTART:;
	int doStart = uf->CodeC;

	// THEN:;
	_CompileStatement(compiler, uf, loopScope, thenBranch);

	nextLine = _ToLastPosition(thenBranch->Position);

	// continues
	for (int i = 0; i < loopScope->ContinueJumpC; i++) {
		_JumpToLabel(compiler, uf, loopScope->ContinueJumps[i]);
	}

	// CONDITION:;
	nextLine = _ToFirstPosition(condition->Position);
	_CompileExpression(compiler, uf, scope, condition);

	// goto: ENDDO
	_EmitLine(compiler, uf, nextLine);
	int labelENDDO = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);

	_EmitLine(compiler, uf, nextLine);
	_JumpToAbsoluteLabel(compiler,
						 uf,
						 _EmitJumpTo(compiler, uf, OP_ABSOLUTE_JUMP),
						 doStart);

	// ENDDDO:;
	// breaks
	for (int i = 0; i < loopScope->BreakJumpC; i++) {
		_JumpToLabel(compiler, uf, loopScope->BreakJumps[i]);
	}

	_JumpToLabel(compiler, uf, labelENDDO);

	FreeScope(loopScope);
}

static void _CompileTryCatch(Compiler*	   compiler,
							 UserFunction* uf,
							 Scope*		   scope,
							 Ast*		   node) {
	Ast*	 tryBlock	= node->A;
	Ast*	 catchParam = node->B;
	Ast*	 catchBlock = node->C;
	Position nextLine	= _ToFirstPosition(node->Position);
	int		 gotoCatch = -1, gotoEndCatch = -1;


	// BEGINTRY:;
	_EmitLine(compiler, uf, nextLine);
	gotoCatch = _EmitJumpTo(compiler, uf, OP_SETUP_TRY);

	Scope* tryScope = CreateScope(SCOPE_TRY_BLOCK, scope);
	while (tryBlock != NULL) {
		_CompileStatement(compiler, uf, tryScope, tryBlock);
		nextLine = _ToLastPosition(tryBlock->Position);
		tryBlock = tryBlock->Next;
	}

	_EmitLine(compiler, uf, nextLine);
	_Emit(compiler, uf, OP_POP_TRY);
	FreeScope(tryScope);

	// ENDTRY:;
	_EmitLine(compiler, uf, nextLine);
	gotoEndCatch = _EmitJumpTo(compiler, uf, OP_JUMP);

	// Catch begin, Jump here if encounters an error
	_JumpToLabel(compiler, uf, gotoCatch);

	// POPTRY:;
	_EmitLine(compiler, uf, nextLine);
	_Emit(compiler, uf, OP_POP_TRY);

	// CATCH:;
	Scope* catchScope = CreateScope(SCOPE_BLOCK, scope);
	int	   offset	  = UserFunctionEmitLocal(uf);
	ScopeSetSymbol(catchScope, catchParam->Value, false, true, false, offset);

	nextLine = _ToFirstPosition(catchParam->Position);

	_EmitLine(compiler, uf, nextLine);
	_EmitArg(compiler, uf, OP_STORE_LOCAL, offset);

	while (catchBlock != NULL) {
		_CompileStatement(compiler, uf, catchScope, catchBlock);
		catchBlock = catchBlock->Next;
	}

	FreeScope(catchScope);

	// ENDCATCH:;
	_JumpToLabel(compiler, uf, gotoEndCatch);
}

static void _CompileBlockStatement(Compiler*	 compiler,
								   UserFunction* uf,
								   Scope*		 scope,
								   Ast*			 node) {
	Scope* block   = CreateScope(SCOPE_BLOCK, scope);
	Ast*   current = node->A;
	while (current != NULL) {
		_CompileStatement(compiler, uf, block, current);
		current = current->Next;
	}
	FreeScope(block);
}

static void _CompileRaiseStatement(Compiler*	 compiler,
								   UserFunction* uf,
								   Scope*		 scope,
								   Ast*			 node) {
	_CompileExpression(compiler, uf, scope, node->A);
	_EmitLine(compiler, uf, node->Position);
	_Emit(compiler, uf, OP_RAISE);
}

static void _CompileAssertStatement(Compiler*	  compiler,
									UserFunction* uf,
									Scope*		  scope,
									Ast*		  node) {
	Ast* condition = node->A;
	Ast* fallback  = node->B;
	_CompileExpression(compiler, uf, scope, condition);
	_EmitLine(compiler, uf, condition->Position);
	int gotoEndAssert = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_TRUE);

	_CompileExpression(compiler, uf, scope, fallback);
	_EmitLine(compiler, uf, fallback->Position);
	_Emit(compiler, uf, OP_POPTOP);

	int jumpToEnd = _EmitJumpTo(compiler, uf, OP_JUMP);

	_EmitLine(compiler, uf, fallback->Position);
	_Emit(compiler, uf, OP_RAISE);

	// ENDASSERT:;
	_JumpToLabel(compiler, uf, gotoEndAssert);
	_JumpToLabel(compiler, uf, jumpToEnd);
}

static void _CompileContinueStatement(Compiler*		compiler,
									  UserFunction* uf,
									  Scope*		scope,
									  Ast*			node) {
	if (!ScopeInside(scope, SCOPE_LOOP)) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   node->Position,
				   "continue statement can only be used "
				   "inside a loop");
	}

	int tryToPop = ScopeCountNestedUntil(scope, SCOPE_TRY_BLOCK, SCOPE_LOOP);
	if (tryToPop == 1) {
		_EmitLine(compiler, uf, node->Position);
		_Emit(compiler, uf, OP_POP_TRY);
	} else if (tryToPop > 1) {
		_EmitLine(compiler, uf, node->Position);
		_EmitArg(compiler, uf, OP_POPN_TRY, tryToPop);
	}

	_EmitLine(compiler, uf, node->Position);
	int offset = _EmitJumpTo(compiler, uf, OP_JUMP);
	ScopeAddContinueJump(scope, offset);
}

static void _CompileBreakStatement(Compiler*	 compiler,
								   UserFunction* uf,
								   Scope*		 scope,
								   Ast*			 node) {
	if (!ScopeInside(scope, SCOPE_LOOP)) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   node->Position,
				   "break statement can only be used "
				   "inside a loop");
	}

	int tryToPop = ScopeCountNestedUntil(scope, SCOPE_TRY_BLOCK, SCOPE_LOOP);
	if (tryToPop == 1) {
		_EmitLine(compiler, uf, node->Position);
		_Emit(compiler, uf, OP_POP_TRY);
	} else if (tryToPop > 1) {
		_EmitLine(compiler, uf, node->Position);
		_EmitArg(compiler, uf, OP_POPN_TRY, tryToPop);
	}

	_EmitLine(compiler, uf, node->Position);
	int offset = _EmitJumpTo(compiler, uf, OP_JUMP);
	ScopeAddBreakJump(scope, offset);
}

static void _CompileReturnStatement(Compiler*	  compiler,
									UserFunction* uf,
									Scope*		  scope,
									Ast*		  node) {
	if (!ScopeInside(scope, SCOPE_FUNCTION)) {
		ThrowError(compiler->Parser->Lexer->Path,
				   compiler->Parser->Lexer->Data,
				   node->Position,
				   "return statement can only be used "
				   "inside a function");
	}

	Scope* fnScope = ScopeGetFirst(scope, SCOPE_FUNCTION);
	if (fnScope != NULL) {
		fnScope->Returned = true;
	}

	int tryToPop = ScopeCountNestedUntil(scope, SCOPE_TRY_BLOCK, SCOPE_LOOP);
	if (tryToPop == 1) {
		_EmitLine(compiler, uf, node->Position);
		_Emit(compiler, uf, OP_POP_TRY);
	} else if (tryToPop > 1) {
		_EmitLine(compiler, uf, node->Position);
		_EmitArg(compiler, uf, OP_POPN_TRY, tryToPop);
	}

	if (node->A != NULL)
		_CompileExpression(compiler, uf, scope, node->A);
	else {
		_EmitLine(compiler, uf, node->Position);
		_Emit(compiler, uf, OP_LOAD_NULL);
	}
	_EmitLine(compiler, uf, node->Position);
	_Emit(compiler, uf, OP_RETURN);
}

static void
_SetupJitMode(Compiler* compiler, UserFunction* uf, String directive) {
	if (strcmp(directive, DIR_JIT_AUTO) == 0) {
		uf->JitMode = JIT_AUTO;
	} else if (strcmp(directive, DIR_JIT_DISABLED) == 0) {
		uf->JitMode = JIT_DISABLED;
	} else if (strcmp(directive, DIR_JIT_ALWAYS) == 0) {
		uf->JitMode = JIT_ALWAYS;
	}
	// ignore, invalid directive for jit
}

static void _CompileExpressionStatement(Compiler*	  compiler,
										UserFunction* uf,
										Scope*		  scope,
										Ast*		  node) {
	if (node->A->Type == AST_STR)
		_SetupJitMode(compiler, uf, node->A->Value);
	_CompileExpression(compiler, uf, scope, node->A);
	_EmitLine(compiler, uf, node->Position);
	_Emit(compiler, uf, OP_POPTOP);
}

static void _ForwardDeclairations(Compiler*		compiler,
								  UserFunction* uf,
								  Scope*		scope,
								  Ast*			node) {
	while (node != NULL) {
		switch (node->Type) {
			case AST_CLASS:
				{
					Ast* className = node->A;
					if (ScopeHasLocal(scope, className->Value)) {
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   className->Position,
								   "duplicate class name");
					}
					int offset = UserFunctionEmitLocal(uf);
					ScopeSetSymbol(scope,
								   className->Value,
								   true,
								   true,
								   false,
								   offset);
					break;
				}
			case AST_FUNCTION:
				{
					Ast* fnName = node->A;
					if (ScopeHasLocal(scope, fnName->Value)) {
						ThrowError(compiler->Parser->Lexer->Path,
								   compiler->Parser->Lexer->Data,
								   fnName->Position,
								   "duplicate function name");
					}
					ScopeSetSymbol(scope,
								   fnName->Value,
								   true,
								   true,
								   false,
								   UserFunctionEmitLocal(uf));
					break;
				}
			default:
				break;
		}
		node = node->Next;
	}
}

static void _CompileStatement(Compiler*		compiler,
							  UserFunction* userFunction,
							  Scope*		scope,
							  Ast*			node) {
	switch (node->Type) {
		case AST_CLASS:
			_CompileClassDeclaration(compiler, userFunction, scope, node);
			break;
		case AST_FUNCTION:
			_CompileFunctionDeclaration(compiler, userFunction, scope, node);
			break;
		case AST_IMPORT:
			_CompileImportStatement(compiler, userFunction, scope, node);
			break;
		case AST_VAR_DECLARATION:
			_CompileVarDeclarationStatement(compiler,
											userFunction,
											scope,
											node);
			break;
		case AST_LOCAL_DECLARATION:
			_CompileLocalDeclarationStatement(compiler,
											  userFunction,
											  scope,
											  node);
			break;
		case AST_CONST_DECLARATION:
			_CompileConstDeclarationStatement(compiler,
											  userFunction,
											  scope,
											  node);
			break;
		case AST_IF:
			_CompileIfStatement(compiler, userFunction, scope, node);
			break;
		case AST_SWITCH:
			_CompileSwitchStatement(compiler, userFunction, scope, node);
			break;
		case AST_FOR:
			_CompileForStatement(compiler, userFunction, scope, node);
			break;
		case AST_WHILE:
			_CompileWhileStatement(compiler, userFunction, scope, node);
			break;
		case AST_DO_WHILE:
			_CompileDoWhileStatement(compiler, userFunction, scope, node);
			break;
		case AST_TRY_CATCH:
			_CompileTryCatch(compiler, userFunction, scope, node);
			break;
		case AST_BLOCK:
			_CompileBlockStatement(compiler, userFunction, scope, node);
			break;
		case AST_RAISE:
			_CompileRaiseStatement(compiler, userFunction, scope, node);
			break;
		case AST_ASSERT:
			_CompileAssertStatement(compiler, userFunction, scope, node);
			break;
		case AST_CONTINUE:
			_CompileContinueStatement(compiler, userFunction, scope, node);
			break;
		case AST_BREAK:
			_CompileBreakStatement(compiler, userFunction, scope, node);
			break;
		case AST_RETURN:
			_CompileReturnStatement(compiler, userFunction, scope, node);
			break;
		case AST_EXPRESSION_STATEMENT:
			_CompileExpressionStatement(compiler, userFunction, scope, node);
			break;
		case AST_EMPTY_STATEMENT:
			break;
		default:
			ThrowError(compiler->Parser->Lexer->Path,
					   compiler->Parser->Lexer->Data,
					   node->Position,
					   "expected a statement");
			break;
	}
}

static Value* _Program(Compiler* compiler, Ast* node, bool isModule) {
	_InitModule(compiler);

	Scope*		  scope = CreateScope(SCOPE_GLOBAL, NULL);
	UserFunction* uf	= CreateMainUserFunction(_GetModule(compiler), 0);

	Value* value = NewUserFunctionValue(compiler->Interpreter, uf);
	_SaveFunction(compiler, value);

	Ast* current = node->A;
	_ForwardDeclairations(compiler, uf, scope, current);
	while (current != NULL) {
		_CompileStatement(compiler, uf, scope, current);
		current = current->Next;
	}

	Position lastLine = _ToLastPosition(node->Position);
	int		 exports  = 0;

	if (isModule) {
		HashMap* names = scope->Symbols;
		for (int i = 0; i < names->Size; i++) {
			HashNode* node = &names->Buckets[i];
			while (node != NULL && node->Key != NULL) {
				String	name   = (String) node->Key;
				Symbol* symbol = (Symbol*) node->Val;
				_EmitLine(compiler, uf, lastLine);
				_EmitArg(compiler, uf, OP_LOAD_NAME, symbol->Offset);
				_EmitLine(compiler, uf, lastLine);
				_EmitString(compiler, uf, OP_LOAD_STRING, name);
				++exports;
				node = node->Next;
			}
		}
	}

	if (isModule) {
		_EmitLine(compiler, uf, lastLine);
		_EmitArg(compiler, uf, OP_OBJECT_MAKE, exports);
	} else {
		_EmitLine(compiler, uf, lastLine);
		_Emit(compiler, uf, OP_LOAD_NULL);
	}

	_EmitLine(compiler, uf, lastLine);
	_Emit(compiler, uf, OP_RETURN);

	FreeScope(scope);

	JitCompileJob* job = _CreateJitCompileJob(compiler->Interpreter, value);
	_StartJitCompileJob(compiler, job);

	return value;
}

Value* Compile(Compiler* compiler) {
	Ast*   program = Parse(compiler->Parser);
	Value* value   = _Program(compiler, program, false);
	FreeAst(program);
	return value;
}

Value* CompileAst(Compiler* compiler, Ast* programAst) {
	Value* value = _Program(compiler, programAst, true);
	return value;
}

void FreeCompiler(Compiler* compiler) {
	free(compiler->ModulePath);
	free(compiler);
}

#undef PushArray
#undef GetOffset