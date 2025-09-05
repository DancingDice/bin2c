/*
** Copyright (c) Darren Moss.  All rights reserved.
*  Licensed under the MIT License.  See license.txt in the project root.
*/

#include <limits.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>



/*
** SIZE_MAX macro
*
*  This macro facilitates use of the "size_t" type by providing a "SIZE_MAX"
*  macro.  While the C language specification lacks some sort of "SIZE_MAX"
*  macro that the "limits.h" header file provides for the intrinsic types,
*  some library suppliers provide a "SIZE_MAX" macro.  Therefore, this macro
*  definition is conditional.
*
* Remarks
*
*  Per the C language specification, "size_t" is at least 16 bits in size,
*  which is similar to the C language's minimum size for a "short int".
*  Given the way the "typedef" keyword operates, defining the "size_t" type to
*  be smaller than an "unsigned short" is, seemingly, impossible.  Therefore,
*  the risk is that "SIZE_MAX" understates the size of "size_t".
*/

#if !defined ( SIZE_MAX )
#define SIZE_MAX  USHRT_MAX
#endif



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
** CHECK macro
*
*  This macro provides a build-time check of assumptions that must be true for
*  the source code, even when it successfully compiles, to operate as intended.
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
** bool type definition
*
*  This type definition provides a simply way to help make source code's use of
*  logic data types clear.  Given that the C language expresses a logic false as
*  zero and logic true as non-zero, a char is reasonably memory-efficient.
*
*  Remarks
*
*  The C language requires compilers to infer logic values have the signed int
*  type.  Therefore, this type depends on compilers properly performing an
*  implicit down-cast from signed int to signed char.  Given logic false is zero
*  and logic true is non-zero and compilers typically use the value one when
*  explicitly setting a logic true value, the implicit down-cast is safe.
*/

typedef signed char bool;



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
*  ==zero:  failure; at least one error occurred, likely presenting as
*           incomplete usage information in the standard error pipe
*  !=zero:  success; this function fulling outputed usage information
*
*  Remarks
*
*  Given this function describes the command line options and parameters, it
*  must remain coordinated with the parser function, "main_parseargs".
*/

static bool main_outputusage
(
    char const * program
)
{
    bool success;

    success = !0;

    /*
    ** The prologue to the usage information conveys two crucial pieces of
    *  information: the purpose of the program and its version number.
    */

    {
        int error;

        error =    fputs ( "\nBinary file to C header file converter (bin2h), version 1.0.\n\n",
                           stderr );
        success &= error >= 0;

        error =    fputs ( "This program extracts data in unaltered binary form from the given input file\n"  \
                           "and outputs that data as an array of unsigned characters (\"unsigned char\n"      \
                           "const\", specifically) into a C language header file.\n\n",
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
                          "%s <input> [-p <prefix>] [-s <suffix>]\n\n",
                          program );
        success &= error >= 0;

        error =    fputs ( "  input      Specifies the input file to use as the source of binary data.  The\n"    \
                           "             output header file will have the input file's path and name, but\n"      \
                           "             with the \".h\" extension.  The input file's name also serves as the\n"  \
                           "             name of the array, with any given prefix and suffix.\n",
                           stderr );
        success &= error >= 0;

        error =    fputs ( "  -p prefix  Prepends \"prefix\" to the name of the array.\n",
                           stderr );
        success &= error >= 0;

        error =    fputs ( "  -s suffix  Appends \"suffix\" to the name of the array.\n\n",
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
*  (at least 65534 characters); so, the limit is simiply a best-effort attempt
*  at gracefully exiting (instead of causing an access violation exception).
*/

static char const * main_findname
(
    char const * path
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
** main_runbin2h function
*
*  This is the core function of the program, which only the "main" function
*  should call, after parsing and validating the command-line arguments.
*
*  Parameter(s)
*
*  infile:   pointer to the "FILE" object for the input binary file; must be
*            be opened in "rb" or equivalent mode (but, idealy, not "r+b")
*  outfile:  pointer to the "FILE" object for the output header file; must be
*            be opened in "wt" or equivalent mode (but, idealy, not "w+t")
*  symbol:   pointer to the name of the array (usually the name of the input
*            binary file, without the leading file path and without the trailing
*            file extension)
*  prefix:   optional pointer to the prefix portion of the name of the array
*            (e.g.: "s_"); may be "NULL"
*  suffix:   optional pointer to the suffix portion of the name of the array
             (e.g.: "_data"); may be "NULL"
*
*  Return value(s)
*
*  ==zero:  failure; an error occurred and the output file likely is in an
*           incomplete form
*  !=zero:  success; the output file has the array of binary data
*
*  Remarks
*
*  This function assumes that the caller, which should be "main", has already
*  performed all necessary validation (e.g.: "infile" is readable in binary
*  form, "symbol" is a valid symbol name, etc.).  Deviations from this
*  function's assumptions can result in incomplete data and/or an uncompilable
*  header file.
*/

static bool main_runbin2h
(
    FILE *       infile,
    FILE *       outfile,
    char const * symbol,
    char const * prefix,
    char const * suffix
)
{
    bool success;

    success = !0;

    /*
    ** In the context of this converter, the purpose of naming the array is to
    *  avoid name collision.  Hence, the name has static scope (note: only one
    *  source file should include the header file, otherwise the program will
    *  have multiple copies of the same data), and, potentially, a prefix and a
    *  suffix.  That means names such as "s_filename_data" are possible, where
    *  the "s_" portion is the optional prefix ("s_" sometimes denotes static
    *  scope to prevent name collision with global and local names), the
    *  "filename" portion is the input binary file name (excluding the path and
    *  extension, which leverages any name anti-collision happening among the
    *  source files in a project), and the "_data" portion is the optional
    *  suffix ("_data" and "_length" or similar suffixes sometimes facilitates
    *  related objects, such as an array and its length, to share highly
    *  similar names and scope, differentiated in name by their suffixes).
    */

    if ( success )
    {
        int error;

        {
            error =    fputs ( "static unsigned char const ",
                               outfile );
            success &= error >= 0;
        }
        if ( prefix != NULL)
        {
            error =    fputs ( prefix,
                               outfile );
            success &= error >= 0;
        }
        {
            error =    fputs ( symbol,
                               outfile );
            success &= error >= 0;
        }
        if ( suffix != NULL)
        {
            error =    fputs ( suffix,
                               outfile );
            success &= error >= 0;
        }
        {
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
    *  write to the output header file in small chunks in an attempt to induce
    *  the file systems' caching to use buffered writes which, importantly,
    *  means flushing to stable media in the background.  (Therefore, using the
    *  "w+t" mode is not ideal as it may delay including pages in flushes.)
    */

    {
        char const *    format;
        unsigned char * buffer;

        CHECK ( ( SIZE_MAX / sizeof ( *buffer ) ) >= MAIN_CHUNKSIZE );

        format =  "0x%hXu";
        buffer =  malloc ( sizeof ( *buffer ) * MAIN_CHUNKSIZE );
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
                    success = 0;
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
                                      ( unsigned short ) *data );
                    success &= error >= 0;

                    format = ", 0x%hXu";

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

    if ( success )
    {
        int error;

        error =   fflush ( outfile );
        success = error >= 0;

    }

    if ( !success )
    {
        fputs ( "ERROR: failed to create an output header file from the input binary file.",
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

static char const * main_shortenname
(
    char * path
)
{
    unsigned short length;
    unsigned short terminus;
    unsigned short delimiter;

    /*
    ** Finding the file name and truncating its extension, if any is present,
    *  depends on finding the last slash character, if any is present.  This
    *  function assumes that "main_constructoutname" previously return
    *  successfully and, therefore, does not ensure that the path includes a
    *  terminating null character (and that there are no more full stops).
    */

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

    return (path);
}



/*
** main_constructoutpath function
*
*  This function copies the input pathname into a heap allocation and replaces
*  the input file's extension, if any is present, with the ".h" extension for
*  C language header files.  The caller must use the "free" function to release
*  the heap allocation.
*
*  Parameter(s)
*
*  inpath:  pointer to the input pathname, which may be just the file name
*
*  Return value(s)
*
*  ==NULL:  failure; an error occurred, such as "inpath" being too long or
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

static char * main_constructoutname
(
    char const * inpath
)
{
    char *         outpath;
    unsigned short length;
    unsigned short offset;

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

        length -= 1u;
        inpath += 1u;

    }

    /*
    ** Heap allocation only needs extra capacity of exactly three characters to
    *  accommodate the output pathname's ".h" extension and the null-terminating
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
            outpath = malloc ( sizeof ( *outpath ) * ( length + 2u + 1u ) );
        }

    }

    /*
    ** This is a specialized string copy algorithm due to the need to ignore
    *  the input pathname's extension, if any is present, and to copy ".h" into
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
        *outpath = 'h';
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
*  argc:     number of elements in "argv", excluding the terminating null element;
*            must be non-negative
*  argv:     pointer to an array of pointers to arguments
*  program:  pointer to "argv[0]"; "*program" may be "NULL" when this function
*            returns (which means "argv[0]" may be "NULL")
*  inpath:   pointer to "argv[1]"; "*inpath" will not be "NULL" when this function
*            returns success (which means "argv[1]" is a required argument)
*  prefix:   pointer to the prefix parameter (the "<prefix>" parameter in the
*            "[-p <prefix>]" option); "*prefix" may be "NULL" upon returning
*  suffix:   pointer to the suffix parameter (the "<suffix>" parameter in the
*            "[-s <suffix>]" option); "*suffix" may be "NULL" upon returning
*
*  Return value(s)
*
*  ==zero:  failure; an error was encountered, such as "argv[0]" missing, and
            the parameters are in an undefined state
*  !=zero:  success; the parameters are consistent with "argc" and "argv"
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
    int            argc,
    char * const * argv,
    char const * * program,
    char * *       inpath,
    char const * * prefix,
    char const * * suffix
)
{
    bool success;

    success = !0;

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

    {
        char const * * parameter;

        parameter = NULL;

        if ( argc > 0 )
        {

            *program =  *argv;
            parameter = ( char const * * ) inpath;

            argc -= 1;
            argv += 1u;

        }

        while ( argc > 0 )
        {

            if ( **argv == '-' )
            {
                char const * option;

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

                    default:
                    success = 0;
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
                    success = 0;
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
        #if !defined ( NDEBUG )
        abort ( );
        #else
        success = 0;
        #endif
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
*  arguments and call the "main_runbin2h" function with validated parameters.
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
*  processes arguments and calls the "main_runbin2h" function, which essentially
*  is the program.  Where to draw the line between processing arguments and
*  "being the program" is an art.  This function goes all the way to
*  constructing pathnames and creating "FILE" objects because this program only
*  needs two "FILE" objects and successfully creating "FILE" objects is strong
*  validation.  Conversely, it stops processing symbol names to simply forward
*  the prefix, the core name, and the suffix as individual parameters to
*  "main_runbin2h" given that avoids unnecessary heap allocations, keeps the
*  "main_runbin2h" function's parameters similar to the command-line arguments,
*  and facilitates future enhancements, such as adding a symbol with something
*  like a "_length" suffix.
*/

int main
(
    int            argc,
    char * const * argv
)
{
    bool         success;
    char const * program;
    char *       inpath;
    char const * prefix;
    char const * suffix;
    FILE *       infile;
    FILE *       outfile;

    success = !0;
    infile =  NULL;
    outfile = NULL;

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
                                    &suffix );
    }

    /*
    ** Given this program does not use environment variables and does not have
    *  inter-dependent options (e.g.: if one option is present, then another
    *  option must also be present), creating "FILE" objects for the mandatory
    *  input and output files is as much validation as is possible.  (The
    *  compiler is the real validator of the combination of the prefix, file
    *  name, and suffix parameters.)
    */

    if ( success )
    {
        infile =  fopen ( inpath,
                          "rb" );
        success = infile != NULL;
    }

    if ( success )
    {
        char * outpath;

        outpath = main_constructoutname ( inpath );

        if ( outpath != NULL )
        {

            outfile = fopen ( outpath,
                              "wt" );
            success = outfile != NULL;

            free ( outpath );

        }

        else
        {
            success = 0;
        }

    }

    /*
    ** At this point, argument validation is complete.  Failures until now are
    *  likely due to invalid arguments.  Therefore, outputing the usage
    *  information is a suitable reaction to a failure.  This also means that
    *  "main_runbin2h" must output its own failure information to the standard
    *  error pipe.
    */

    if ( !success )
    {
        if ( program != NULL )
        {
            main_outputusage ( main_findname ( program ) );
        }
        else
        {
            main_outputusage ( "bin2h" );
        }
    }

    else
    {
        success = main_runbin2h ( infile,
                                  outfile,
                                  main_shortenname ( inpath ),
                                  prefix,
                                  suffix );
    }

    /*
    ** Final clean-up actions typically always succeed.  However, closing a file
    *  can depend on a flush of at least file metadata to stable media and media
    *  can fail flushes.  Therefore, this function reports clean-up failures.
    */

    {
        bool clean;

        clean = !0;

        if ( outfile != NULL )
        {
            int error;

            error =  fclose ( outfile );
            clean &= error == 0;

        }

        if ( infile != NULL )
        {
            int error;

            error =  fclose ( infile );
            clean &= error == 0;

        }

        if ( !clean )
        {
            fputs ( "ERROR: failed to properly close files.",
                    stderr );
        }

        success &= clean;

    }

    return ( success ? EXIT_SUCCESS : EXIT_FAILURE ) ;
}
