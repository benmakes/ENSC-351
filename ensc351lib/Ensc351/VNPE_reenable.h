/*
 * VNPE_reenable.h
 *
 *  Created on: 2015-02-16
 *      Author: wcs
 */

#undef PE_NULL
#undef PE2_NULL
#undef PE_EOF
#undef PE
#undef PE2
#undef PE_NOT
#undef PE_NEG
#undef PE_0
#undef PE_EOK

/*
 The PE_NULL macro is for wrapping functions to detect whether a function call
 returns NULL to indicate an error.  If NULL is returned, an error reporting
 function is called.
 */
#define PE_NULL(function) PE_NULL_P(function)

// The PE2_NULL macro is as above but also reports an additional piece of information
#define PE2_NULL(function, info) PE2_NULL_P(function, info)

/*
 The PE_EOF macro is for wrapping functions to detect whether a function call
 returns EOF to indicate an error.  If EOF is returned, an error reporting
 function is called.
*/
// NOTE -- EOF is often -1 so PE is often the same as PE_EOF
#define PE_EOF(function) PE_EOF_P(function)

/*
 The PE macro is for wrapping functions to detect whether a function call
 returns -1 to indicate an error.  If -1 is returned, an error reporting
 function is called.
*/
#define PE(function) PE_P(function)

// The PE2 macro is as above but also reports an additional piece of information
#define PE2(function, info) PE2_P(function, info)

/*
 The PE_NEG macro is for wrapping functions to detect whether a function call
 returns a negative number to indicate an error.  If a negative number is returned, an error reporting
 function is called.
*/
#define PE_NEG(function) PE_NEG_P(function)

// The PE_NOT macro is like PE but is intended for situations where a
// specific (desired) return value is required.
#define PE_NOT(function, desiredRv) PE_NOT_P(function, desiredRv)

// The PE_0 macro is for wrapping functions to detect whether a function call
//	did not return 0, indicating an error.  If 0 is not returned, an error reporting
//  function is called.
#define PE_0(function) PE_0_P(function)

// On Linux/Mac OS X systems, EOK may not exist
#ifdef EOK
// The PE_EOK macro is for wrapping functions to detect whether a function call
//	did not return 0, indicating an error.  If EOK is not returned, an error reporting
//  function is called.
#define PE_EOK(function) PE_EOK_P(function)
#endif
