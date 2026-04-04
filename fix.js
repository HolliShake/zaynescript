const fs = require('fs');
let content = fs.readFileSync('src/compiler.c', 'utf8');

// 1. _CompileIdentifier missing _ToFirstPosition
content = content.replace(
    /\/\/ Capture the variable\n\t\t_EmitLine\(compiler, uf, pos\);\n\t\t_EmitArg\(compiler, uf, OP_LOAD_CAPTURE, captureOffset\);\n\t\treturn;\n\t}\n\n\t_EmitLine\(compiler, uf, pos\);\n\t_EmitArg\(compiler, uf, OP_LOAD_LOCAL, symbol->Offset\);/,
    "// Capture the variable\n\t\t_EmitLine(compiler, uf, _ToFirstPosition(pos));\n\t\t_EmitArg(compiler, uf, OP_LOAD_CAPTURE, captureOffset);\n\t\treturn;\n\t}\n\n\t_EmitLine(compiler, uf, _ToFirstPosition(pos));\n\t_EmitArg(compiler, uf, OP_LOAD_LOCAL, symbol->Offset);"
);

// 2. OP_ARRAY_EXTEND
content = content.replace(
    /_CompileExpression\(compiler,\n\t\t\t\t\t\t\t\t\t\t\t\t  uf,\n\t\t\t\t\t\t\t\t\t\t\t\t  scope,\n\t\t\t\t\t\t\t\t\t\t\t\t  elements->A\);\n\t\t\t\t\t\t\t\t_EmitLine\(compiler, uf, _ToFirstPosition\(node->Position\)\);\n\t\t\t\t\t\t\t\t_Emit\(compiler, uf, OP_ARRAY_EXTEND\);/,
    "_CompileExpression(compiler,\n\t\t\t\t\t\t\t\t\t\t\t\t  uf,\n\t\t\t\t\t\t\t\t\t\t\t\t  scope,\n\t\t\t\t\t\t\t\t\t\t\t\t  elements->A);\n\t\t\t\t\t\t\t\t_EmitLine(compiler, uf, _ToLastPosition(elements->A->Position));\n\t\t\t\t\t\t\t\t_Emit(compiler, uf, OP_ARRAY_EXTEND);"
);

// 3. OP_OBJECT_EXTEND
content = content.replace(
    /_CompileExpression\(compiler,\n\t\t\t\t\t\t\t\t\t\t\t\t  uf,\n\t\t\t\t\t\t\t\t\t\t\t\t  scope,\n\t\t\t\t\t\t\t\t\t\t\t\t  properties->A\);\n\t\t\t\t\t\t\t\t_EmitLine\(compiler, uf, _ToFirstPosition\(node->Position\)\);\n\t\t\t\t\t\t\t\t_Emit\(compiler, uf, OP_OBJECT_EXTEND\);/,
    "_CompileExpression(compiler,\n\t\t\t\t\t\t\t\t\t\t\t\t  uf,\n\t\t\t\t\t\t\t\t\t\t\t\t  scope,\n\t\t\t\t\t\t\t\t\t\t\t\t  properties->A);\n\t\t\t\t\t\t\t\t_EmitLine(compiler, uf, _ToLastPosition(properties->A->Position));\n\t\t\t\t\t\t\t\t_Emit(compiler, uf, OP_OBJECT_EXTEND);"
);

// 4. TryCatch nextLine logic
content = content.replace(
    /nextLine = _ToLastPosition\(tryBlock->Position\);\n\n\tScope\* tryScope = CreateScope\(SCOPE_TRY_BLOCK, scope\);\n\twhile \(tryBlock != NULL\) {\n\t\t_CompileStatement\(compiler, uf, tryScope, tryBlock\);\n\t\ttryBlock = tryBlock->Next;\n\t}\n\n\t_EmitLine\(compiler, uf, nextLine\);\n\t_Emit\(compiler, uf, OP_POP_TRY\);\n\tFreeScope\(tryScope\);/,
    "Scope* tryScope = CreateScope(SCOPE_TRY_BLOCK, scope);\n\twhile (tryBlock != NULL) {\n\t\t_CompileStatement(compiler, uf, tryScope, tryBlock);\n\t\tnextLine = _ToLastPosition(tryBlock->Position);\n\t\ttryBlock = tryBlock->Next;\n\t}\n\n\t_EmitLine(compiler, uf, nextLine);\n\t_Emit(compiler, uf, OP_POP_TRY);\n\tFreeScope(tryScope);"
);

// 5. _CompileForStatement backwards jump line logic -- nextLine should be evaluated BEFORE OP_ABSOLUTE_JUMP
content = content.replace(
    /\t\t\/\/ goto: FORSTART\n\t\t_EmitLine\(compiler, uf, nextLine\);\n\t\t_JumpToAbsoluteLabel\(compiler,\n\t\t\t\t\t\t\t uf,\n\t\t\t\t\t\t\t _EmitJumpTo\(compiler, uf, OP_ABSOLUTE_JUMP\),\n\t\t\t\t\t\t\t labelFORSTART\);\n\n\t\t\/\/ ENDFOR:;/g,
    "\t\t// goto: FORSTART\n\t\tnextLine = _ToLastPosition(node->Position);\n\t\t_EmitLine(compiler, uf, nextLine);\n\t\t_JumpToAbsoluteLabel(compiler,\n\t\t\t\t\t\t\t uf,\n\t\t\t\t\t\t\t _EmitJumpTo(compiler, uf, OP_ABSOLUTE_JUMP),\n\t\t\t\t\t\t\t labelFORSTART);\n\n\t\t// ENDFOR:;"
);

// 6. _CompileWhileStatement
content = content.replace(
    /\t\t\/\/ goto: WHILESTART\n\t\t_EmitLine\(compiler, uf, nextLine\);\n\t\t_JumpToAbsoluteLabel\(compiler,\n\t\t\t\t\t\t\t uf,\n\t\t\t\t\t\t\t _EmitJumpTo\(compiler, uf, OP_ABSOLUTE_JUMP\),\n\t\t\t\t\t\t\t labelWHILESTART\);\n\n\t\t\/\/ ENDFOR:;/,
    "\t\t// goto: WHILESTART\n\t\tnextLine = _ToLastPosition(node->Position);\n\t\t_EmitLine(compiler, uf, nextLine);\n\t\t_JumpToAbsoluteLabel(compiler,\n\t\t\t\t\t\t\t uf,\n\t\t\t\t\t\t\t _EmitJumpTo(compiler, uf, OP_ABSOLUTE_JUMP),\n\t\t\t\t\t\t\t labelWHILESTART);\n\n\t\t// ENDFOR:;"
);

fs.writeFileSync('src/compiler.c', content);
console.log("Fix applied");
