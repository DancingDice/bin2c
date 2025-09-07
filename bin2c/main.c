/*
** Copyright (c) Darren Moss.  All rights reserved.
*  Licensed under the MIT License.  See license.txt in the project root.
*/

#include <limits.h>
#include <ctype.h>
#include <string.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>



/*
** restrict keyword-like macro
*
* The C99 specification includes the "restrict" keyword, which is a useful
* pointer aliasing (or, more accurately, anti-aliasing, given "restrict"
* clarifies that the compiler can employ optimizations that depend on pointers'
* ranges not overlapping).  Given that usefulness, this macro provides a way to
* use C99's restrict keyword without breaking C89 compatibility.
*
* Remarks
*
* A C89 compiler will likely generate functionally different instructions than a
* C99 compiler will, given the C99 compiler may optimize pointer accesses.
* If those functional differences are meaningful, then they reveal a source
* defect (either in overuse of "restrict" or overlapping pointers).
*/

#if !defined ( __STDC_VERSION__ ) || ( defined ( __STDC_VERSION__ ) && ( __STDC_VERSION__ < 199901l ) )
#define restrict
#endif



/*
** bool type definition
*
*  The C99 specification includes a "_Bool" intrinsic type and C99's "stdbool.h"
*  header file provides the corresponding "bool" type and its "true" and "false"
*  values.  Therefore, this "bool" type definition leverages C99's
*  implementation, when it is available, and falls back to manually defining the
*  "bool" type, where C99's implementation is not available.
*
*  Remarks
*
*  Ultimately, the purpose of the "bool" type is to provide a simple way to help
*  make source code's use of logic data types clear.  If manual implementation
*  is necessary, then an enumeration is suitable due to having clearly defined
*  false and true value, although an enumeration has the trade-off of being
*  excessively large, potentially as large as an "int".
*/

#if defined ( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199901l )

#include <stdbool.h>

#elif !defined ( __cplusplus )

typedef enum
{
    false = 0,
    true =  !0
} bool;

#endif



/*
** SIZE_MAX enumeration-like macro
*
*  The C99 specification includes a "SIZE_MAX" macro via C99's "stdint.h" header
*  file.  Therefore, this "SIZE_MAX" type definition leverages C99's macro, when
*  it is available, and falls back to manually defining the "SIZE_MAX", when
*  C99's implementation is not available.  (Note: some library suppliers provide
*  a "SIZE_MAX" macro, regardless of the C specification they implement.)
*
* Remarks
*
*  Per the C language specification, "size_t" is at least 16 bits in size,
*  which is similar to the C language's minimum size for a "short int".
*  Given the way the "typedef" keyword operates, defining the "size_t" type to
*  be smaller than an "unsigned short" is, seemingly, impossible.  Therefore,
*  when manual definition is necessary, this macro uses "USHRT_MAX", given that
*  the risk is that "SIZE_MAX" understates the size of "size_t" (which is safe).
*/

#if defined ( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199901l )
#include <stdint.h>
#elif !defined ( SIZE_MAX )
#define SIZE_MAX  USHRT_MAX
#endif



/*
** CHECK function-like macro
*
*  This macro provides a build-time check of assumptions that must be true for
*  the source code, even when it successfully compiles, to operate as intended.
*
*  Parameter(s)
*
*  condition:  the build-time condition that must be true for the build process
*              to continue and, ultimately, successfully complete
*
*  Remarks
*
*  Given that array indexing must be non-negative, this macro repurposes the
*  compiler's "sizeof" keyword to cause the compiler to evaluate the expression
*  in the array index.  The result will be either a positive or negative
*  integer, corresponding to whether "condition" is true or false.  A false
*  condition means that the compilation attempt will fail due to the negative
*  array index value in this macro.
*/

#define CHECK(condition)  ( ( void ) sizeof ( char[1-2*!(condition)] ) )



/*
** MAIN_PATHDELIMITER macro
*
*  This macro is a best-effort attempt to use either the forward-slash or the
*  back-slash character for delimiting directory names in a file path.
*
*  Remarks
*
*  Most operating systems typically use the forward-slash character.  Yet, most
*  of Microsoft's operating systems typically use the back-slash character and
*  Windows is their most popular series of operating systems.  Therefore, this
*  best-effort attempt focuses on Windows variants.
*/

#if defined ( _WIN16 ) || defined ( _WIN32 ) || defined ( _WIN64 )
#define MAIN_PATHDELIMITER  '\\'
#else
#define MAIN_PATHDELIMITER  '/'
#endif



/*
** MAIN_CHUNKSIZE macro
*
*  This macro is a best-effort attempt to make reading the input binary file
*  efficient by reading in page-sized chunks.
*
*  Remarks
*
*  Most Intel-style processors have 4096 bytes per page; therefore, the best-
*  effort attempt uses a small multiple of the page size for Intel-style
*  processors.  Most storage devices have 512 bytes per sector; therefore, the
*  fallback uses a small multiple of the sector size as the default.
*/

#if defined ( __i386__ ) || defined ( __x86_64__ ) || defined ( _M_IX86 ) || defined ( _M_AMD64 )
#define MAIN_CHUNKSIZE  ( 4096u * 4u )
#else
#define MAIN_CHUNKSIZE  ( 512u * 8u )
#endif



/*
** main_outputusage function
*
*  This function makes a best-effort attempt to output usage information to the
*  standard error pipe.
*
*  Parameter(s)
*
*  program:  pointer to the name this program in whatever form it is present in
*            the command line
*
*  Return value(s)
*
*  ==false:  failure; at least one error occurred, likely presenting as
*            incomplete usage information in the standard error pipe
*  !=false:  success; this function fulling outputed usage information
*
*  Remarks
*
*  Given this function describes the command line options and parameters, it
*  must remain coordinated with the parser function, "main_parseargs".
*/

static bool main_outputusage
(
    char const * restrict program
)
{
    bool success;

    success = true;

    /*
    ** The prologue to the usage information conveys two crucial pieces of
    *  information: the purpose of the program and its version number.
    */

    {
        int error;

        error =    fputs ( "\nBinary file to C language file converter (bin2c), version 2.0.\n\n",
                           stderr );
        success &= error >= 0;

        error =    fputs ( "This program extracts data in unaltered binary form from the given input file and outputs that data as an array of\n"  \
                           "unsigned characters (\"unsigned char const\", specifically) into C language file(s).\n\n",
                           stderr );
        success &= error >= 0;

    }


    /*
    ** The usage information conforms to IBM-style conventions (verbatim parts
    *  are undecorated, mandatory parts are enclosed in chevrons, and optional
    *  parts are enclosed in brackets.
    */

    {
        int error;

        error = fprintf ( stderr,
                          "%s <input_file> [-p <array_prefix>] [-s <array_suffix>] [-g <length_suffix>]\n\n",
                          program );
        success &= error >= 0;

        error =    fputs ( "  input_file        Specifies the input file to use as the source of binary data.  The output file(s) will have the\n"    \
                           "                    input file's path and name, but with the \".h\" extension and, when the \"-g\" option is present,\n"  \
                           "                    the \".c\" extension.  The input file's name also serves as the core of the name of the array.\n",
                           stderr );
        success &= error >= 0;

        error =    fputs ( "  -p array_prefix   Prepends \"array_prefix\" to the name of the array.\n",
                           stderr );
        success &= error >= 0;

        error =    fputs ( "  -s array_suffix   Appends \"array_suffix\" to the name of the array.\n",
                           stderr );
        success &= error >= 0;

        error =    fputs ( "  -g length_suffix  Gives the name global scope and creates both header and source files.  If this option is not present,\n"    \
                           "                    then the name has static scope and only a header file is created.  If this option is present, then\n"       \
                           "                    \"length_suffix\" is appended to the input file's name, \"array_prefix\", when present, is prepended to\n"  \
                           "                    the input file's name, and are  capitalized to form a macro for the number of elements.\n",
                           stderr );
        success &= error >= 0;

    }

    return ( success );
}



/*
** main_findname function
*
*  This function simply provides a pointer to the substring portion of a
*  pathname that contains the file name.  This function assumes a prior call of
*  the "main_constructoutname" function returned successfully.
*
*  Parameter(s)
*
*  path:  pointer to the pathname, which may be just the file name
*
*  Return value(s)
*
*  !=NULL:  pointer to the file name, which may or may not include the file
*           extension, depending on the "path" parameter
*
*  Remarks
*
*  This function limits the "path" parameter length, including the null-
*  terminating character, to "USHRT_MAX".  That is rather long for a file path
*  (at least 65535 characters); so, the limit is simiply a best-effort attempt
*  at gracefully exiting (instead of causing an access violation exception).
*/

static char const * restrict main_findname
(
    char const * restrict path
)
{
    unsigned short length;
    unsigned short delimiter;

    /*
    ** Finding the file name depends on finding the last slash character, if any
    *  is present.  This function assumes that "main_constructoutname"
    *  previously return successfully and, therefore, does not ensure that the
    *  path includes a terminating null character.
    */

    CHECK ( sizeof ( length ) <= sizeof ( path ) );

    length =    USHRT_MAX;
    delimiter = 0;

    while ( length > 0 )
    {

        if ( *path == MAIN_PATHDELIMITER )
        {
            delimiter = ( USHRT_MAX - length ) + 1u;
        }
        else if ( *path == '\0' )
        {
            break;
        }

        length -= 1u;
        path +=   1u;

    }

    path -= ( USHRT_MAX - length ) - delimiter;

    return ( path );
}



/*
** main_runbin2c_constructmacro_upperizemacro function
*
*  This function merely converts the characters in the given "part" parameter
*  to upper-case as it copies the characters into the "macro" parameter.
*
*  Parameter(s)
*
*  part:   pointer to the text to capitalize and copy into the "macro" parameter
*  macro:  pointer to the buffer that receives the capitalized characters
*
*  Return value(s)
*
*  !=NULL:  pointer into the "macro" parameter where the caller must either
*           place a null-terminating character or initiate another copy
*
*  Remarks
*
*  This function does not copy a null-terminating character into the "macro"
*  parameter.  Rather, by returning a pointer to where the null-terminating
*  character should be, this function enables the caller to either concatenate
*  more text into the "macro" parameter or to place the null-terminating
*  character itself.
*/

static char * restrict main_runbin2c_constructmacro_upperizemacro
(
    char const * restrict part,
    char * restrict       macro
)
{

    while ( *part != '\0' )
    {

        *macro = ( char ) toupper ( *part );

        part +=  1u;
        macro += 1u;

    }

    return ( macro );
}


/*
** main_runbin2c_constructmacro function
*
*  This function concatenates and capitalizes the supplied strings in a new heap
*  allocation.  This facilitates creation of macros, such as the header guard
*  and the array length macros.
*
*  Parameter(s)
*
*  prefix:  pointer to the prefix to concatenate and capitalize; may be "NULL"
*  core:    pointer to the core to concatenate and capitalize
*  suffix:  pointer to the suffix to concatenate and capitalize; may be "NULL"
*
*  Return value(s)
*
*  ==NULL:  failure; an error occurred, such as the heap allocation failing or
*           the concatenated text string being too long
*  !=NULL:  success; the three input text strings were capitalized and
*           concatenated into a heap allocation
*
*  Remarks
*
*  The functions in this program all use "USHRT_MAX" for the maximum string
*  length, including for this function's returned string.  Therefore, the
*  combined length of the three input strings and the terminating null
*  character must be "<=USHRT_MAX".
*/

static char * restrict main_runbin2c_constructmacro
(
    char const * restrict prefix,
    char const * restrict core,
    char const * restrict suffix
)
{
    char * restrict macro;

    /*
    ** Since all three strings need to fit within "USHRT_MAX", this functions
    *  simply measures all of them and, given they likely all fit within
    *  "USHRT_MAX", unifies failing due to excessive length with failing due to
    *  insufficient spare heap capacity.
    */

    {
        bool           success;
        unsigned short length;

        success = true;
        length =  USHRT_MAX;

        {
            size_t fragment;

            if ( prefix != NULL )
            {
                fragment = strlen ( prefix );
                success &= fragment < length;
                length -=  ( unsigned short ) fragment;
            }
            {
                fragment = strlen ( core );
                success &= fragment < length;
                length -=  ( unsigned short ) fragment;
            }
            if ( suffix != NULL )
            {
                fragment = strlen ( suffix );
                success &= fragment < length;
                length -=  ( unsigned short ) fragment;
            }

        }

        success &= length >= 1u;
        length =   USHRT_MAX - length;

        CHECK ( ( SIZE_MAX / sizeof ( *macro ) ) >= USHRT_MAX );

        if ( success )
        {
            macro = ( char * ) malloc ( sizeof ( *macro ) * ( length + 1u ) );
        }
        else
        {
            macro = NULL;
        }

    }

    /*
    ** Since the C standard library lacks a string-wide "toupper" type function,
    *  this function must manually traverse the strings and capitalize each
    *  character individually.  Therefore, it also performs the concatenation
    *  (instead of using "strcat").
    */

    if ( macro != NULL )
    {
        char * restrict cursor;

        cursor = macro;

        if ( prefix != NULL )
        {
            cursor = main_runbin2c_constructmacro_upperizemacro ( prefix,
                                                                  cursor );
        }
        {
            cursor = main_runbin2c_constructmacro_upperizemacro ( core,
                                                                  cursor );
        }
        if ( suffix != NULL )
        {
            cursor = main_runbin2c_constructmacro_upperizemacro ( suffix,
                                                                  cursor );
        }

        *cursor = '\0';

    }

    return ( macro );
}



/*
** main_runbin2c_outputsymbol function
*
*  This function outputs the three components of the full symbolic name to the
*  given "FILE" object, concatenating the components in the process.
*
*  Parameter(s)
*
*  prefix:   optional pointer to the prefix portion of the name of the array
*            (e.g.: "s_"); may be "NULL"
*  symbol:   pointer to the name of the array (usually the name of the input
*            binary file, without the leading file path and without the trailing
*            file extension)
*  suffix:   optional pointer to the suffix portion of the name of the array
*            (e.g.: "_data"); may be "NULL"
*  outfile:  pointer to the "FILE" object for the output C file; must be
*            be opened in "wb" or equivalent mode (but, idealy, not "w+b")
*
*  Return value(s)
*
*  ==false:  failure; an error occurred and the output file likely is in an
*            incomplete form
*  !=false:  success; the output file has the full symbolic name
*
*  Remarks
*
*  This function only outputs the symbolic name.  The caller must prepend
*  storage-class specifiers, such as "extern" or "static", as well as the array
*  brackes, "[]", as is necessary.
*/

static bool main_runbin2c_outputsymbol
(
    char const * restrict prefix,
    char const * restrict symbol,
    char const * restrict suffix,
    FILE * restrict       outfile
)
{
    bool success;

    success = true;

    if ( prefix != NULL )
    {
        int error;

        error =    fputs ( prefix,
                           outfile );
        success &= error >= 0;

    }

    {
        int error;

        error =    fputs ( symbol,
                           outfile );
        success &= error >= 0;

    }

    if ( suffix != NULL )
    {
        int error;

        error =    fputs ( suffix,
                           outfile );
        success &= error >= 0;

    }

    return ( success );
}



/*
** main_runbin2c function
*
*  This is the core function of the program, which only the "main" function
*  should call, after parsing and validating the command-line arguments.
*
*  Parameter(s)
*
*  infile:   pointer to the "FILE" object for the input binary file; must be
*            be opened in "rb" or equivalent mode (but, idealy, not "r+b")
*  symbol:   pointer to the name of the array (usually the name of the input
*            binary file, without the leading file path and without the trailing
*            file extension)
*  prefix:   optional pointer to the prefix portion of the name of the array
*            (e.g.: "s_"); may be "NULL"
*  suffix:   optional pointer to the suffix portion of the name of the array
*            (e.g.: "_data"); may be "NULL"
*  global:   optional pointer to the suffix portion of the name of the length
*            of the array (e.g.: "_length"); may be "NULL"
*  outpath:  pointer to the pathname for the output files; a single-character
*            extension must be present, which this function will replace with
*            "h" and, potentially, "c", in the process of creating the array
*
*  Return value(s)
*
*  ==false:  failure; an error occurred and the output file likely is in an
*            incomplete form
*  !=false:  success; the output file has the array of binary data
*
*  Remarks
*
*  This function assumes that the caller, which should be "main", has already
*  performed all necessary validation (e.g.: "binfile" is readable in binary
*  form, "symbol" is a valid symbol name, etc.).  Deviations from this
*  function's assumptions can result in incomplete data and/or an uncompilable
*  header and/or source file.
*/

static bool main_runbin2c
(
    FILE * restrict       infile,
    char const * restrict symbol,
    char const * restrict prefix,
    char const * restrict suffix,
    char const * restrict global,
    char * restrict       outpath
)
{
    bool   success;
    size_t offset;
    long   length;

    success = true;

    /*
    ** The last character of "outpath" is the whitespace that this function
    *  replaces with "h" and, potentially, "c" to create the C output files.
    */

    if ( success )
    {
        offset =  strlen ( outpath );
        success = offset >= 1u;
        offset -= 1u;
    }

    /*
    ** In the context of this converter, the purpose of naming the array is to
    *  avoid name collision.  Hence, the name has the option to have a prefix
    *  and suffix.  That means names such as "g_filename_data" are possible.
    *  (The "filename" portion is the input binary file name, excluding the path
    *  and extension, which leverages any name anti-collision happening among
    *  the source files in a project).
    */

    {
        FILE * restrict outfile;

        outfile = NULL;

        if ( success )
        {

            outpath[offset] = ( global != NULL ) ? 'c' : 'h';

            outfile = fopen ( outpath,
                              "wt" );
            success = outfile != NULL;

        }

        /*
        ** If the name has global scope, then the definition of the array must
        *  reside in a source file.  Otherwise, the definition of the array must
        *  reside in a header file and the name must have static scope.
        *  Therefore, this function creates either a source or header file for
        *  the array definition, based on the name's scope.
        */

        if ( success )
        {
            int error;

            if ( global != NULL )
            {
                error =    fprintf ( outfile,
                                     "#include \"%s.h\"\n\n",
                                     symbol );
                success &= error >= 0;
            }
            else
            {
                error =   fputs ( "static ",
                                  outfile );
                success &= error >= 0;
            }
            {
                error =  fputs ( "unsigned char const ",
                                 outfile );
                success &= error >= 0;
            }

        }

        if ( success )
        {

            {
                int error;

                error =    main_runbin2c_outputsymbol ( prefix,
                                                        symbol,
                                                        suffix,
                                                        outfile );
                success &= error >= 0;

            }

            {
                int error;

                error =    fputs ( "[] = { ",
                                   outfile );
                success &= error >= 0;

            }

        }

        /*
        ** This converter attempts to efficiently read from the input binary file in
        *  large chunks that are potentially multiples of the page size.  This
        *  approach facilitates some file systems' caching behavior that auto-
        *  selects unbuffered reads, which, importantly, amortizes time lost to
        *  media access and can eliminate in-memory copying. (Therefore, using the
        *  "r+b" mode is not ideal as the potential for writing may prevent
        *  unbuffered reading.)   Conversely, this converter attempts to efficiently
        *  write to the output C file in small chunks in an attempt to induce the
        *  file systems' caching to use buffered writes which, importantly, means
        *  flushing to stable media in the background.  (Therefore, using the "w+t"
        *  mode is not ideal as it may delay including pages in flushes.)
        */

        {
            char const * restrict    format;
            unsigned char * restrict buffer;

            CHECK ( ( SIZE_MAX / sizeof ( *buffer ) ) >= MAIN_CHUNKSIZE );

            length = 0;

            #if defined ( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199901l )
            format =  "0x%hhXu";
            #else
            format =  "0x%hXu";
            #endif

            buffer =  ( unsigned char * ) malloc ( sizeof ( *buffer ) * MAIN_CHUNKSIZE );
            success = buffer != NULL;

            while ( success )
            {
                size_t count;

                count = fread ( buffer,
                                sizeof ( *buffer ),
                                MAIN_CHUNKSIZE,
                                infile );
                if ( count < 1u )
                {
                    if ( ferror ( infile ) )
                    {
                        success = false;
                    }
                }

                if ( success )
                {
                    unsigned char const * data;

                    data = buffer;

                    while ( count > 0 )
                    {
                        int error;

                        error = fprintf ( outfile,
                                          format,
                                          #if defined ( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199901l )
                                          ( unsigned char ) *data );
                                          #else
                                          ( unsigned short ) *data );
                                          #endif
                        success &= error >= 0;

                        success &= length < LONG_MAX;
                        length +=  1ul;

                        #if defined ( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199901l )
                        format =  ", 0x%hhXu";
                        #else
                        format =  ", 0x%hXu";
                        #endif

                        count -= 1u;
                        data +=  1u;

                    }

                }

                if ( feof ( infile ) )
                {
                    break;
                }

            }

            if  ( buffer != NULL )
            {
                free ( buffer );
            }

        }

        if ( success )
        {
            int error;

            error =   fputs ( " };\n",
                              outfile );
            success = error >= 0;

        }

        if ( outfile != NULL )
        {
            int error;

            error =    fflush ( outfile );
            success &= error >= 0;

            error =    fclose ( outfile );
            success &= error >= 0;

        }

    }

    /*
    ** If the name has global scope, then its definition resides in a source
    *  file and its declaration resides in a header file.  (For static scope,
    *  the definition resides in a header file and also serves as the name
    *  declaration.)  Additionally, global scope obscures the array's size.
    *  Therefore, the header file also has a macro that expresses the number of
    *  elements in the array.
    */

    if ( global )
    {
        FILE * restrict outfile;

        outfile = NULL;

        if ( success )
        {

            outpath[offset] = 'h';

            outfile = fopen ( outpath,
                              "wt" );
            success = outfile != NULL;

        }

        if ( success )
        {
            char * restrict macro;

            macro =   main_runbin2c_constructmacro ( NULL,
                                                     symbol,
                                                     NULL );
            success = macro != NULL;

            if ( success )
            {
                int error;

                error =   fprintf ( outfile,
                                    "#if !defined ( __%s_H__ )\n\n#define __%s_H__\n\nextern unsigned char const ",
                                    macro,
                                    macro );
                success = error >= 0;

                free ( macro );

            }

        }

        if ( success )
        {

            {
                int error;

                error =    main_runbin2c_outputsymbol ( prefix,
                                                        symbol,
                                                        suffix,
                                                        outfile );
                success &= error >= 0;

            }

            {
                int error;

                error =    fputs ( "[];\n\n#define ",
                                   outfile );
                success &= error >= 0;

            }

        }

        if ( success )
        {

            {
                char * restrict macro;

                macro =   main_runbin2c_constructmacro ( prefix,
                                                         symbol,
                                                         global );
                success = macro != NULL;

                if ( success )
                {
                    int error;

                    error =   fputs ( macro,
                                      outfile );
                    success = error >= 0;

                    free ( macro );

                }

            }

            if ( success )
            {
                int error;

                #if defined ( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199901l )
                if ( length <= SCHAR_MAX )
                {
                    error = fprintf ( outfile,
                                      "  %hhd",
                                      ( signed char ) length );
                }
                else if ( length <= SHRT_MAX )
                #else
                if ( length <= SHRT_MAX )
                #endif
                {
                    error = fprintf ( outfile,
                                      "  %hd",
                                      ( short ) length );
                }
                else if ( length <= INT_MAX )
                {
                    error = fprintf ( outfile,
                                      "  %d",
                                      ( int ) length );
                }
                else
                {
                    error = fprintf ( outfile,
                                      "  %ldl",
                                      length );
                }

                success &= error >= 0;

                error =    fputs ( "\n\n#endif\n",
                                   outfile );
                success &= error >= 0;

            }

        }

        if ( outfile != NULL )
        {
            int error;

            error =    fflush ( outfile );
            success &= error >= 0;
            error =    fclose ( outfile );
            success &= error >= 0;

        }

    }

    if ( !success )
    {
        fputs ( "ERROR: failed to create output C file(s) from the input binary file.",
                stderr );
    }

    return ( success );
}



/*
** main_shortenname function
*
*  This function a truncates a pathname to remove its file extension.  It also
*  finds the portion that contains the file name and returns a pointer to that
*  substring.  This function assumes a prior call of the "main_constructoutname"
*  function returned successfully.
*
*  Parameter(s)
*
*  path:  pointer to the pathname, which may be just the file name
*
*  Return value(s)
*
*  !=NULL:  pointer to the file name, which does not include the extension
*
*  Remarks
*
*  This function limits the "path" parameter length, including the null-
*  terminating character, to "USHRT_MAX".  That is rather long for a file path
*  (at least 65535 characters); so, the limit is simiply a best-effort attempt
*  at gracefully exiting (instead of causing an access violation exception).
*/

static char const * restrict main_shortenname
(
    char * restrict path
)
{
    unsigned short terminus;
    unsigned short delimiter;

    /*
    ** Finding the file name and truncating its extension, if any is present,
    *  depends on finding the last slash character, if any is present.  This
    *  function assumes that "main_constructoutname" previously return
    *  successfully and, therefore, does not ensure that the path includes a
    *  terminating null character (and that there are no more full stops).
    */

    {
        unsigned short length;

        CHECK ( sizeof ( length ) <= sizeof ( path ) );

        length =    USHRT_MAX;
        terminus =  0;
        delimiter = 0;

        while ( length > 0 )
        {

            if ( *path == '.' )
            {
                terminus = length;
            }
            else if ( *path == MAIN_PATHDELIMITER )
            {
                terminus =  0;
                delimiter = ( USHRT_MAX - length ) + 1u;
            }
            else if ( *path == '\0' )
            {
                break;
            }

            length -= 1u;
            path +=   1u;

        }

        path -= ( USHRT_MAX - length ) - delimiter;

    }

    /*
    ** Truncating the pathname to remove the file extension, if any is present,
    *  is just a simple over-writing of the full stop with a terminating null
    *  character.  (The file extension remains untouched, albeit invisible.)
    */

    if ( terminus > 0 )
    {
        terminus =       ( USHRT_MAX - terminus ) - delimiter;
        path[terminus] = '\0';
    }

    return ( path );
}



/*
** main_constructoutpath function
*
*  This function copies the input pathname into a heap allocation and replaces
*  the input file's extension, if any is present, with a ". " extension so that
*  the caller can replace the whitespace with C language file extensions.  The
*  caller must use the "free" function to release the heap allocation.
*
*  Parameter(s)
*
*  inpath:  pointer to the input pathname, which may be just the file name
*
*  Return value(s)
*
*  ==NULL:  failure; an error occurred, such as "binpath" being too long or
*           the heap allocation failing
*  !=NULL:  success; pointer to the output path (the caller must release this
*           heap allocation)
*
*  Remarks
*
*  This function limits the "inpath" parameter length, including the null-
*  terminating character, to "USHRT_MAX".  That is rather long for a file path
*  (at least 65535 characters); so, the limit is simiply a best-effort attempt
*  at gracefully exiting (instead of causing an access violation exception).
*/

static char * restrict main_constructoutpath
(
    char const * restrict inpath
)
{
    char * restrict outpath;
    unsigned short  length;
    unsigned short  offset;

    outpath = NULL;

    /*
    ** Finding the file extension depends on finding the last full stop, if any
    *  is present, after finding the last slash delimiter, if any is present.
    */

    CHECK ( sizeof ( length ) <= sizeof ( inpath ) );

    length = USHRT_MAX;
    offset = USHRT_MAX;

    while ( length > 0 )
    {

        if ( *inpath == '.' )
        {
            offset = length;
        }
        else if ( *inpath == MAIN_PATHDELIMITER )
        {
            offset = USHRT_MAX;
        }
        else if ( *inpath == '\0' )
        {
            break;
        }

        length -=  1u;
        inpath += 1u;

    }

    /*
    ** Heap allocation only needs extra capacity of exactly three characters to
    *  accommodate the output pathname's ". " extension and the null-terminating
    *  character.
    */

    if ( *inpath == '\0' )
    {

        inpath -= ( USHRT_MAX - length );

        if ( offset != USHRT_MAX )
        {
            length = offset;
        }

        CHECK ( ( SIZE_MAX / sizeof ( *outpath ) ) >= USHRT_MAX );

        if ( length >= ( 2u + 1u ) )
        {
            length =  USHRT_MAX - length;
            outpath = ( char * ) malloc ( sizeof ( *outpath ) * ( length + 2u + 1u ) );
        }

    }

    /*
    ** This is a specialized string copy algorithm due to the need to ignore
    *  the input pathname's extension, if any is present, and to copy ". " into
    *  the output pathname instead.
    */

    if ( outpath != NULL )
    {

        offset = length;

        while ( length > 0 )
        {

            *outpath = *inpath;

            length -=  1u;
            inpath +=  1u;
            outpath += 1u;

        }

        *outpath = '.';
        outpath += 1u;
        *outpath = ' ';
        outpath += 1u;

        *outpath = '\0';
        outpath -= 2u;

        outpath -= offset;

    }

    return ( outpath );
}



/*
** main_parseargs function
*
*  This function parses the command-line arguments.  It applies a typical
*  "program <object> [-o <parameter>]" pattern to the arguments, but is case
*  insensitive and does not support long option variants.
*
*  Parameter(s)
*
*  argc:        number of elements in "argv", excluding the terminating null
*               element; must be non-negative
*  argv:        pointer to an array of pointers to arguments
*  program:     pointer to "argv[0]"; "*program" may be "NULL" when this
*               function returns (which means "argv[0]" may be "NULL")
*  inpath:      pointer to "argv[1]"; "*inpath" will not be "NULL" when this
*               function returns success ("argv[1]" is a required argument)
*  prefix:      pointer to the prefix parameter (the "<array_prefix>" parameter
*               in the "[-p <array_prefix>]" option); "*prefix" may be "NULL"
*               upon returning
*  suffix:      pointer to the suffix parameter (the "<array_suffix>" parameter
*               in the "[-s <array_suffix>]" option); "*suffix" may be "NULL"
*               upon returning
*  global:      pointer to the global parameter (the "<length_suffix>" parameter
*               in the "[-g <length_suffix>]" option); "*global" may be "NULL"
*               upon returning
*
*  Return value(s)
*
*  ==false:  failure; an error was encountered, such as "argv[0]" missing, and
*            the parameters are in an undefined state
*  !=false:  success; the parameters are consistent with "argc" and "argv"
*
*  Remarks
*
*  The purpose of the first pass of parsing command line arguments is largely to
*  organize the arguments into an expected syntax.  This does not mean that the
*  parameters are valid (e.g.: pathnames may be invalid, options' parameters may
*  be invalid).  It only means mandatory parameters are present, no unknown
*  options, no duplicate options, no spurious parameters, etc.
*/

static bool main_parseargs
(
    int                              argc,
    char * const restrict * restrict argv,
    char const * restrict * restrict program,
    char * restrict * restrict       inpath,
    char const * restrict * restrict prefix,
    char const * restrict * restrict suffix,
    char const * restrict * restrict global
)
{
    bool success;

    success = true;

    /*
    ** Given that options can be in any order, every option must be compared
    *  with all possible values.  Given that the parameter for an option follows
    *  the option as the subsequent parameter, the loop needs another iteration
    *  to retrieve the parameter.  Finally, given that duplicate instances of an
    *  option are invalid and that no instance of an option is valid, the
    *  options start as "NULL", must be "NULL" when being set, and might still
    *  be "NULL" when this loop successfully completes.
    */

    *prefix = NULL;
    *suffix = NULL;
    *global = NULL;

    {
        char const * restrict * restrict parameter;

        parameter = NULL;

        if ( argc > 0 )
        {

            *program =  *argv;
            parameter = ( char const * restrict * restrict ) inpath;

            argc -= 1;
            argv += 1u;

        }

        while ( argc > 0 )
        {

            if ( **argv == '-' )
            {
                char const * restrict option;

                success &= parameter == NULL;

                option = *argv + 1u;

                switch ( *option )
                {

                    case 'p':
                    case 'P':
                    parameter = prefix;
                    break;

                    case 's':
                    case 'S':
                    parameter = suffix;
                    break;

                    case 'g':
                    case 'G':
                    parameter = global;
                    break;

                    default:
                    success = false;
                    break;

                }

                option +=  1u;
                success &= *option == '\0';

            }

            else
            {
                if ( parameter != NULL )
                {
                    *parameter = *argv;
                    success &=   *parameter != NULL;
                    parameter =  NULL;
                }
                else
                {
                    success = false;
                }
            }

            argc -= 1;
            argv += 1u;

        }

        success &= parameter == NULL;

    }

    /*
    ** The "argc" parameter must be non-negative.  At this point in the
    *  function, if "argc" were non-negative, then it would be zero now.
    *  Additionally, the "argv" parameter is null-terminated.  Therefore, this
    *  final check catches both forms of invalid parameters.
    */

    if ( argc < 0 )
    {
        success = false;
    }
    else
    {
        success &= *argv == NULL;
    }

    return ( success );
}



/*
** main function
*
*  This is the program's main function.  It's core purpose is to process
*  arguments and call the "main_runbin2c" function with validated parameters.
*
*  Parameter(s)
*
*  argc:  number of elements in "argv", excluding the terminating null element;
*         must be non-negative
*  argv:  pointer to an array of pointers to arguments
*
*  Return value(s):
*
*  EXIT_SUCCESS:  success; the output file exists and is in a valid state
*  EXIT_FAILURE:  failure; an error occurred and usage information was outputed
*                 to the standard error pipe (and the output file may or may not
*                 exist; if it exists, then it may or may not be valid)
*
*  Remarks
*
*  While this function is the entry point to the program, it largely just
*  processes arguments and calls the "main_runbin2c" function, which essentially
*  is the program.  Where to draw the line between processing arguments and
*  "being the program" is an art.  This function goes all the way to
*  constructing pathnames and creating a "FILE" object for the input file
*  because successfully creating a "FILE" object is strong validation of the
*  pathname.  Conversely, it stops processing symbol names to simply forward
*  the prefix, the core name, and the suffix as individual parameters to
*  "main_runbin2c" given that avoids unnecessary heap allocations and keeps the
*  "main_runbin2c" function's parameters similar to the command-line arguments.
*/

int main
(
    int                     argc,
    char * const * restrict argv
)
{
    bool            success;
    FILE * restrict infile;
    char * restrict outpath;

    success = true;
    infile =  NULL;
    outpath = NULL;

    /*
    ** This function manages failures in three sections.  This first section
    *  effectively considers all failures as argument validation failures.  The
    *  second section exists inside the "main_runbin2c" function.  The final
    *  section is resource clean-up which, since it includes system storage,
    *  must report clean-up failures.
    */

    {
        char const * restrict program;
        char *       restrict inpath;
        char const * restrict prefix;
        char const * restrict suffix;
        char const * restrict global;

        /*
        ** The first pass of parsing command-line arguments is simply validating
        *  syntax.  Success at this stage merely means mandatory parameters are
        *  present, options are not duplicated, and present options are valid.
        */

        if ( success )
        {
             success = main_parseargs ( argc,
                                        argv,
                                        &program,
                                        &inpath,
                                        &prefix,
                                        &suffix,
                                        &global );
        }

        /*
        ** Given this program does not use environment variables and does not
        *  have inter-dependent options (e.g.: if one option is present, then
        *  another option must also be present), creating "FILE" objects for the
        *  mandatory input file is as much validation as is possible.  (The
        *  compiler, ultimately, will be the real validator of the combination
        *  of the prefix, file name, and suffix parameters.)
        */

        if ( success )
        {
            infile =  fopen ( inpath,
                              "rb" );
            success = infile != NULL;
        }

        if ( success )
        {
            outpath =  main_constructoutpath ( inpath );
            success &= outpath != NULL;
        }

        /*
        ** At this point, argument validation is complete.  Failures until now
        *  are likely due to invalid arguments.  Therefore, outputing the usage
        *  information is a suitable reaction to a failure.  This also means
        *  that "main_runbin2c" must output its own failure information to the
        *  standard error pipe.
        */

        if ( !success )
        {
            if ( program != NULL )
            {
                main_outputusage ( main_findname ( program ) );
            }
            else
            {
                main_outputusage ( "bin2c" );
            }
        }

        else
        {
            success = main_runbin2c ( infile,
                                      main_shortenname ( inpath ),
                                      prefix,
                                      suffix,
                                      global,
                                      outpath );
        }

    }

    /*
    ** Final clean-up actions typically always succeed.  However, closing a file
    *  can depend on a flush of at least file metadata to stable media and media
    *  can fail flushes.  Failed flushes of metadata for the input file can
    *  render the input file inaccessible (although most file systems try to
    *  prevent that outcome from a failed flush).
    */

    {
        bool clean;

        clean = true;

        if ( outpath != NULL )
        {
            free ( outpath );
        }

        if ( infile != NULL )
        {
            int error;

            error =  fclose ( infile );
            clean &= error == 0;

        }

        if ( !clean )
        {
            fputs ( "ERROR: failed to properly close the input file.",
                    stderr );
        }

        success &= clean;

    }

    return ( success ? EXIT_SUCCESS : EXIT_FAILURE ) ;
}
