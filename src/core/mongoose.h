/**
 * @file mongoose.h
 * @brief Core Mongoose HTTP server module interface
 *
 * Provides an Express.js-style HTTP server binding powered by
 * the Mongoose embedded networking library.  Usage:
 *
 *   import mongoose from "mongoose"
 *
 *   const app = mongoose.createServer()
 *
 *   app.get("/", (req, res) => {
 *     res.send("Hello, World!")
 *   })
 *
 *   app.listen(8000)
 */

#include "../../mongoose/mongoose.h"
#include "../function.h"
#include "../global.h"
#include "../hashmap.h"
#include "../value.h"


#ifndef CORE_MONGOOSE_H
#	define CORE_MONGOOSE_H

/**
 * @brief Loads the core Mongoose HTTP module
 *
 * @param  interpreter The interpreter instance
 * @return Value* Pointer to the loaded module object, or NULL
 * on failure
 */
Value* LoadCoreMongoose(Interpreter* interpreter);

#endif /* CORE_MONGOOSE_H */
