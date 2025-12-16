#include "./exceptionhandler.h"


ExceptionHandler* NewExceptionHandler() {
    ExceptionHandler* exceptionHandler = Allocate(sizeof(ExceptionHandler));
    exceptionHandler->Catched   = 0;
    exceptionHandler->Exception = NULL;
    return exceptionHandler;
}

void CatchException(ExceptionHandler* exceptionHandler) {
    exceptionHandler->Catched = 1;
}


void SendException(ExceptionHandler* exceptionHandler, Value* exception) {
    exceptionHandler->Exception = exception;
}