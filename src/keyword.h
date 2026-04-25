
/**
 * @file keyword.h
 * @brief Language keyword string constants.
 *
 * Defines string constants for all reserved keywords in the
 * language. These are used by the lexer to identify keyword
 * tokens during tokenization.
 */

#ifndef KEYWORD_H
#define KEYWORD_H

#define KEY_CLASS  "class"	/**< Class declaration keyword */
#define KEY_ENUM   "enum"	/**< Enumeration declaration keyword */
#define KEY_STATIC "static" /**< Static member modifier keyword */
#define KEY_IMPORT "import" /**< Module import keyword */
#define KEY_FROM   "from"	/**< Import source specifier keyword */
#define KEY_CONST  "const"	/**< Immutable variable declaration keyword */
#define KEY_VAR	   "var"	/**< Mutable variable declaration keyword */
#define KEY_LOCAL                                                              \
	"local"					/**< Local (function-scoped) variable declaration  \
							   keyword */
#define KEY_ASYNC  "async"	/**< Asynchronous function modifier keyword */
#define KEY_FN	   "fn"		/**< Function declaration keyword */
#define KEY_IF	   "if"		/**< Conditional branch keyword */
#define KEY_ELSE   "else"	/**< Alternative branch keyword */
#define KEY_SWITCH "switch" /**< Multi-way branch keyword */
#define KEY_CASE   "case"	/**< Case label keyword in a switch statement */
#define KEY_DEFAULT                                                            \
	"default"				/**< Default label keyword in a switch statement   \
							 */
#define KEY_WHILE "while"	/**< Pre-conditioned loop keyword */
#define KEY_DO                                                                 \
	"do"			  /**< Post-conditioned loop keyword (used with while)     \
					   */
#define KEY_FOR "for" /**< C-style for-loop keyword */
#define KEY_TRY "try" /**< Exception-guarded block keyword */
#define KEY_CATCH                                                              \
	"catch"			  /**< Exception handler block keyword                     \
					   */
#define KEY_RETURN	 "return"	/**< Function return keyword */
#define KEY_BREAK	 "break"	/**< Loop/switch exit keyword */
#define KEY_CONTINUE "continue" /**< Loop iteration restart keyword */
#define KEY_RAISE	 "raise"	/**< Exception throwing keyword */
#define KEY_ASSERT	 "assert"	/**< Assertion keyword for runtime checks */
#define KEY_NULL	 "null"		/**< Null literal keyword */
#define KEY_TRUE	 "true"		/**< Boolean true literal keyword */
#define KEY_FALSE	 "false"	/**< Boolean false literal keyword */
#define KEY_NEW		 "new"		/**< Object instantiation keyword */
#define KEY_AWAIT	 "await"	/**< Asynchronous operation awaiting keyword */
#define KEY_TYPEOF   "typeof"	/**< Typeof keyword */
#define KEY_THIS	 "this"		/**< Current instance reference keyword */

#endif							/* KEYWORD_H */
