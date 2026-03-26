#include "../class.h"
#include "../function.h"
#include "../global.h"
#include "../value.h"
#include "../statemachine.h"


#ifndef CORE_PROMISE_H
#define CORE_PROMISE_H

Value* CreatePromiseClass(Interpreter* interpreter);

Value* LoadCorePromise(Interpreter* interpreter);

#endif