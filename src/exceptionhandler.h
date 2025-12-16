#include "./global.h"

#ifndef EXCEPTIONHANDLER_H
#define EXCEPTIONHANDLER_H

/**
 * @brief Creates a new exception handler
 * 
 * @return Pointer to newly allocated ExceptionHandler structure
 */
ExceptionHandler* NewExceptionHandler();

/**
 * @brief Catches an exception
 * 
 * @param exceptionHandler Pointer to the exception handler
 */
void CatchException(ExceptionHandler* exceptionHandler);

/**
 * @brief Sends an exception
 * 
 * @param exceptionHandler Pointer to the exception handler
 * @param exception Pointer to the exception value
 */
void SendException(ExceptionHandler* exceptionHandler, Value* exception);

#endif