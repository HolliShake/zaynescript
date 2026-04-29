#ifndef INTERPRETER_H
#define INTERPRETER_H

/**
 * @file interpreter.h
 * @brief Declares the runtime entry points and frame helpers used to execute
 *        compiled LanguageX bytecode.
 *
 * The interpreter owns the VM-wide singletons, frame stack, async task queue,
 * and event-loop integration that drive both synchronous code and promise-
 * based coroutines.
 */

#include "./array.h"
#include "./class.h"
#include "./decompiler.h"
#include "./environment.h"
#include "./error.h"
#include "./function.h"
#include "./gc.h"
#include "./global.h"
#include "./operation.h"
#include "./parser.h"
#include "./statemachine.h"
#include "./value.h"

/**
 * @brief Allocates a fresh interpreter and installs all runtime singletons.
 *
 * Initializes the libbf context, core classes, scalar singletons, import
 * tracking, and the Mongoose event manager used by async-native modules.
 *
 * @param execPath Executable path used as the base for resolving relative
 *                 imports and locating the running binary.
 * @return Fully initialized interpreter instance owned by the caller.
 */
Interpreter* CreateInterpreter(String execPath);

/**
 * @brief Switches the interpreter's ambient async task pointer.
 *
 * Async error propagation consults this field to decide which promise should
 * be rejected when execution raises out of a coroutine or queued callback.
 *
 * @param interpreter Interpreter whose current task is being updated.
 * @param task Promise Value currently executing, or `NULL` outside task
 *             dispatch.
 */
void SetActiveTask(Interpreter* interpreter, Value* task);

/**
 * @brief Updates the frame pointer used by native helpers that need stack
 *        access outside the main dispatch loop.
 *
 * Event-loop callbacks and certain runtime helpers borrow this pointer to push
 * temporary arguments onto the correct frame.
 *
 * @param interpreter Interpreter whose current frame should be exposed.
 * @param frame Active call frame, or `NULL` when no bytecode frame is running.
 */
void SetCurrentFrame(Interpreter* interpreter, CallFrame* frame);

/**
 * @brief Executes a compiled top-level function and drains the async task
 *        queue until the program is idle.
 *
 * This is the public runtime entry point used by `main.c` after compilation.
 * It runs the program, polls native I/O integrations, and reports unhandled
 * promise rejections before returning.
 *
 * @param interpreter Interpreter that owns the program state and event loop.
 * @param fnValue Value wrapping the compiled top-level `UserFunction`.
 */
void Interpret(Interpreter* interpreter, Value* fnValue /*UserFunction*/);

/**
 * @brief Performs panic cleanup and terminates the process with failure.
 *
 * Used after fatal runtime errors once the caller has already emitted the
 * relevant diagnostics.
 *
 * @param interpreter Interpreter whose owned resources should be released
 *                    before exiting.
 */
void InterpreterPanicExit(Interpreter* interpreter);

/**
 * @brief Releases interpreter-owned runtime infrastructure.
 *
 * Frees import tracking, singleton arrays, executable path strings, and the
 * Mongoose/libbf contexts. GC-managed Values should already have been cleaned
 * up before this is called.
 *
 * @param interpreter Interpreter instance to destroy.
 */
void FreeInterpreter(Interpreter* interpreter);


#endif /* INTERPRETER_H */