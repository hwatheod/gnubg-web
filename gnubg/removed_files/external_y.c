/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 1 "external_y.y"

/*
 * external_y.y -- command parser for external interface
 *
 * by Michael Petch <mpetch@gnubg.org>, 2014.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 3 or later of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: external_y.y,v 1.32 2015/02/08 17:23:29 plm Exp $
 */

#ifndef EXTERNAL_Y_H
#define EXTERNAL_Y_H

#define MERGE_(a,b)  a##b
#define LABEL_(a,b) MERGE_(a, b)

#define YY_PREFIX(variable) MERGE_(ext_,variable)

#define yymaxdepth YY_PREFIX(maxdepth)
#define yyparse YY_PREFIX(parse)
#define yylex   YY_PREFIX(lex)
#define yyerror YY_PREFIX(error)
#define yylval  YY_PREFIX(lval)
#define yychar  YY_PREFIX(char)
#define yydebug YY_PREFIX(debug)
#define yypact  YY_PREFIX(pact)
#define yyr1    YY_PREFIX(r1)
#define yyr2    YY_PREFIX(r2)
#define yydef   YY_PREFIX(def)
#define yychk   YY_PREFIX(chk)
#define yypgo   YY_PREFIX(pgo)
#define yyact   YY_PREFIX(act)
#define yyexca  YY_PREFIX(exca)
#define yyerrflag YY_PREFIX(errflag)
#define yynerrs YY_PREFIX(nerrs)
#define yyps    YY_PREFIX(ps)
#define yypv    YY_PREFIX(pv)
#define yys     YY_PREFIX(s)
#define yy_yys  YY_PREFIX(yys)
#define yystate YY_PREFIX(state)
#define yytmp   YY_PREFIX(tmp)
#define yyv     YY_PREFIX(v)
#define yy_yyv  YY_PREFIX(yyv)
#define yyval   YY_PREFIX(val)
#define yylloc  YY_PREFIX(lloc)
#define yyreds  YY_PREFIX(reds)
#define yytoks  YY_PREFIX(toks)
#define yylhs   YY_PREFIX(yylhs)
#define yylen   YY_PREFIX(yylen)
#define yydefred YY_PREFIX(yydefred)
#define yydgoto  YY_PREFIX(yydgoto)
#define yysindex YY_PREFIX(yysindex)
#define yyrindex YY_PREFIX(yyrindex)
#define yygindex YY_PREFIX(yygindex)
#define yytable  YY_PREFIX(yytable)
#define yycheck  YY_PREFIX(yycheck)
#define yyname   YY_PREFIX(yyname)
#define yyrule   YY_PREFIX(yyrule)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "glib-ext.h"
#include "external.h"
#include "backgammon.h"
#include "external_y.h"

/* Resolve a warning on older GLIBC/GNU systems that have stpcpy */
#if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
extern char *stpcpy(char *s1, const char *s2);
#endif

#define extcmd ext_get_extra(scanner)  

int YY_PREFIX(get_column)  (void * yyscanner );
void YY_PREFIX(set_column) (int column_no, void * yyscanner );
extern int YY_PREFIX(lex) (YYSTYPE * yylval_param, scancontext *scanner);         
extern scancontext *YY_PREFIX(get_extra) (void *yyscanner );
extern void StartParse(void *scancontext);
extern void yyerror(scancontext *scanner, const char *str);

void yyerror(scancontext *scanner, const char *str)
{
    if (extcmd->ExtErrorHandler)
        extcmd->ExtErrorHandler(extcmd, str);
    else
        fprintf(stderr,"Error: %s\n",str);
}

#endif



/* Line 268 of yacc.c  */
#line 179 "external_y.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 1
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     EOL = 258,
     EXIT = 259,
     DISABLED = 260,
     INTERFACEVERSION = 261,
     DEBUG = 262,
     SET = 263,
     NEW = 264,
     OLD = 265,
     OUTPUT = 266,
     E_INTERFACE = 267,
     HELP = 268,
     PROMPT = 269,
     E_STRING = 270,
     E_CHARACTER = 271,
     E_INTEGER = 272,
     E_FLOAT = 273,
     E_BOOLEAN = 274,
     FIBSBOARD = 275,
     FIBSBOARDEND = 276,
     EVALUATION = 277,
     CRAWFORDRULE = 278,
     JACOBYRULE = 279,
     RESIGNATION = 280,
     BEAVERS = 281,
     CUBE = 282,
     CUBEFUL = 283,
     CUBELESS = 284,
     DETERMINISTIC = 285,
     NOISE = 286,
     PLIES = 287,
     PRUNE = 288
   };
#endif
/* Tokens.  */
#define EOL 258
#define EXIT 259
#define DISABLED 260
#define INTERFACEVERSION 261
#define DEBUG 262
#define SET 263
#define NEW 264
#define OLD 265
#define OUTPUT 266
#define E_INTERFACE 267
#define HELP 268
#define PROMPT 269
#define E_STRING 270
#define E_CHARACTER 271
#define E_INTEGER 272
#define E_FLOAT 273
#define E_BOOLEAN 274
#define FIBSBOARD 275
#define FIBSBOARDEND 276
#define EVALUATION 277
#define CRAWFORDRULE 278
#define JACOBYRULE 279
#define RESIGNATION 280
#define BEAVERS 281
#define CUBE 282
#define CUBEFUL 283
#define CUBELESS 284
#define DETERMINISTIC 285
#define NOISE 286
#define PLIES 287
#define PRUNE 288




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 116 "external_y.y"

    gboolean bool;
    gchar character;
    gfloat floatnum;
    gint intnum;
    GString *str;
    GValue *gv;
    GList *list;
    commandinfo *cmd;



/* Line 293 of yacc.c  */
#line 294 "external_y.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */

/* Line 343 of yacc.c  */
#line 127 "external_y.y"



/* Line 343 of yacc.c  */
#line 310 "external_y.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  25
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   74

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  38
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  23
/* YYNRULES -- Number of rules.  */
#define YYNRULES  55
/* YYNRULES -- Number of states.  */
#define YYNSTATES  85

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   288

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      35,    36,     2,     2,    37,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    34,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     9,    12,    15,    18,    21,    24,
      27,    30,    33,    35,    37,    40,    42,    44,    48,    50,
      53,    56,    59,    62,    65,    68,    71,    73,    76,    78,
      81,    84,    86,    88,    89,    92,    93,    96,    99,   102,
     107,   115,   117,   119,   121,   123,   125,   127,   129,   131,
     133,   137,   139,   141,   142,   144
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      39,     0,    -1,     3,    -1,     8,    40,     3,    -1,     6,
       3,    -1,    13,     3,    -1,     4,     3,    -1,    41,     3,
      -1,     7,    55,    -1,    12,     9,    -1,    12,    10,    -1,
      14,    53,    -1,    49,    -1,    50,    -1,     5,    58,    -1,
      54,    -1,    42,    -1,    43,    34,    42,    -1,    21,    -1,
      24,    55,    -1,    23,    55,    -1,    25,    54,    -1,    26,
      55,    -1,    32,    54,    -1,    31,    52,    -1,    31,    54,
      -1,    33,    -1,    33,    55,    -1,    30,    -1,    30,    55,
      -1,    27,    55,    -1,    28,    -1,    29,    -1,    -1,    47,
      45,    -1,    -1,    48,    46,    -1,    48,    45,    -1,    51,
      47,    -1,    22,    20,    51,    48,    -1,    20,    15,    34,
      15,    34,    43,    44,    -1,    18,    -1,    15,    -1,    17,
      -1,    19,    -1,    58,    -1,    55,    -1,    53,    -1,    52,
      -1,    54,    -1,    35,    60,    36,    -1,    57,    -1,    56,
      -1,    -1,    59,    -1,    60,    37,    59,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   175,   175,   181,   188,   194,   200,   206,   260,   265,
     271,   277,   284,   292,   300,   312,   316,   321,   328,   332,
     337,   342,   347,   354,   359,   364,   372,   377,   382,   387,
     392,   397,   402,   410,   427,   435,   452,   457,   464,   478,
     493,   505,   513,   522,   530,   538,   548,   548,   548,   548,
     553,   560,   560,   565,   569,   574
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "EOL", "EXIT", "DISABLED",
  "INTERFACEVERSION", "DEBUG", "SET", "NEW", "OLD", "OUTPUT",
  "E_INTERFACE", "HELP", "PROMPT", "E_STRING", "E_CHARACTER", "E_INTEGER",
  "E_FLOAT", "E_BOOLEAN", "FIBSBOARD", "FIBSBOARDEND", "EVALUATION",
  "CRAWFORDRULE", "JACOBYRULE", "RESIGNATION", "BEAVERS", "CUBE",
  "CUBEFUL", "CUBELESS", "DETERMINISTIC", "NOISE", "PLIES", "PRUNE", "':'",
  "'('", "')'", "','", "$accept", "commands", "setcommand", "command",
  "board_element", "board_elements", "endboard", "sessionoption",
  "evaloption", "sessionoptions", "evaloptions", "boardcommand",
  "evalcommand", "board", "float_type", "string_type", "integer_type",
  "boolean_type", "list_type", "basic_types", "list", "list_element",
  "list_elements", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,    58,    40,    41,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    38,    39,    39,    39,    39,    39,    39,    40,    40,
      40,    40,    41,    41,    41,    42,    43,    43,    44,    45,
      45,    45,    45,    46,    46,    46,    46,    46,    46,    46,
      46,    46,    46,    47,    47,    48,    48,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    57,    57,    57,
      58,    59,    59,    60,    60,    60
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     2,     2,     2,     2,     2,     2,
       2,     2,     1,     1,     2,     1,     1,     3,     1,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     1,     2,
       2,     1,     1,     0,     2,     0,     2,     2,     2,     4,
       7,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     1,     0,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     2,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    12,    13,    33,     6,    53,    14,     4,     0,     0,
       0,     0,     5,     0,     0,     1,     7,    38,    42,    43,
      41,    44,    48,    47,    49,    46,    52,    51,    45,    54,
       0,     8,     9,    10,    11,     3,     0,    35,     0,     0,
       0,     0,    34,    50,     0,     0,    39,    20,    19,    21,
      22,    55,     0,     0,    31,    32,    28,     0,     0,    26,
      37,    36,    16,     0,    15,    30,    29,    24,    25,    23,
      27,    18,     0,    40,    17
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     9,    21,    10,    72,    73,    83,    52,    71,    27,
      56,    11,    12,    13,    32,    33,    34,    35,    36,    37,
      38,    39,    40
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -49
static const yytype_int8 yypact[] =
{
       3,   -49,     7,   -23,    15,    10,    26,     0,    19,    32,
      41,   -49,   -49,   -49,   -49,   -14,   -49,   -49,    27,    18,
      34,    44,   -49,    16,    43,   -49,   -49,    12,   -49,   -49,
     -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,
       4,   -49,   -49,   -49,   -49,   -49,    49,   -49,    27,    27,
      48,    27,   -49,   -49,   -14,    33,    29,   -49,   -49,   -49,
     -49,   -49,    48,    27,   -49,   -49,    27,    25,    48,    27,
     -49,   -49,   -49,    -8,   -49,   -49,   -49,   -49,   -49,   -49,
     -49,   -49,    48,   -49,   -49
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -49,   -49,   -49,   -49,   -16,   -49,   -49,    13,   -49,   -49,
     -49,   -49,   -49,    46,     1,    51,   -48,   -18,   -49,   -49,
      69,    20,   -49
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      41,    28,    59,    29,    30,    31,     1,     2,     3,     4,
      14,     5,    15,    81,    74,    23,     6,    18,    17,    78,
      79,    15,    19,     7,    20,     8,    82,    42,    43,    22,
      57,    58,    25,    60,    74,    48,    49,    50,    51,    24,
      53,    54,    29,    30,    26,    75,    31,    45,    76,    28,
      46,    80,    48,    49,    50,    51,    63,    64,    65,    66,
      67,    68,    69,     7,    55,    29,    84,    62,    77,    70,
      47,    44,    16,     0,    61
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-49))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int8 yycheck[] =
{
      18,    15,    50,    17,    18,    19,     3,     4,     5,     6,
       3,     8,    35,    21,    62,    15,    13,     7,     3,    67,
      68,    35,    12,    20,    14,    22,    34,     9,    10,     3,
      48,    49,     0,    51,    82,    23,    24,    25,    26,    20,
      36,    37,    17,    18,     3,    63,    19,     3,    66,    15,
      34,    69,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    20,    15,    17,    82,    34,    67,    56,
      24,    20,     3,    -1,    54
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     8,    13,    20,    22,    39,
      41,    49,    50,    51,     3,    35,    58,     3,     7,    12,
      14,    40,     3,    15,    20,     0,     3,    47,    15,    17,
      18,    19,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    55,     9,    10,    53,     3,    34,    51,    23,    24,
      25,    26,    45,    36,    37,    15,    48,    55,    55,    54,
      55,    59,    34,    27,    28,    29,    30,    31,    32,    33,
      45,    46,    42,    43,    54,    55,    55,    52,    54,    54,
      55,    21,    34,    44,    42
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (scanner, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, scanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, scanner); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, scancontext *scanner)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, scanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    scancontext *scanner;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (scanner);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, scancontext *scanner)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, scanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    scancontext *scanner;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, scanner);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, scancontext *scanner)
#else
static void
yy_reduce_print (yyvsp, yyrule, scanner)
    YYSTYPE *yyvsp;
    int yyrule;
    scancontext *scanner;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, scanner); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, scancontext *scanner)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, scanner)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    scancontext *scanner;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (scanner);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {
      case 15: /* "E_STRING" */

/* Line 1391 of yacc.c  */
#line 168 "external_y.y"
	{ if ((yyvaluep->str)) g_string_free((yyvaluep->str), TRUE); };

/* Line 1391 of yacc.c  */
#line 1323 "external_y.c"
	break;
      case 40: /* "setcommand" */

/* Line 1391 of yacc.c  */
#line 169 "external_y.y"
	{ if ((yyvaluep->list)) g_list_free((yyvaluep->list)); };

/* Line 1391 of yacc.c  */
#line 1332 "external_y.c"
	break;
      case 41: /* "command" */

/* Line 1391 of yacc.c  */
#line 171 "external_y.y"
	{ if ((yyvaluep->cmd)) { g_free((yyvaluep->cmd)); }};

/* Line 1391 of yacc.c  */
#line 1341 "external_y.c"
	break;
      case 42: /* "board_element" */

/* Line 1391 of yacc.c  */
#line 170 "external_y.y"
	{ if ((yyvaluep->gv)) { g_value_unsetfree((yyvaluep->gv)); }};

/* Line 1391 of yacc.c  */
#line 1350 "external_y.c"
	break;
      case 43: /* "board_elements" */

/* Line 1391 of yacc.c  */
#line 169 "external_y.y"
	{ if ((yyvaluep->list)) g_list_free((yyvaluep->list)); };

/* Line 1391 of yacc.c  */
#line 1359 "external_y.c"
	break;
      case 45: /* "sessionoption" */

/* Line 1391 of yacc.c  */
#line 169 "external_y.y"
	{ if ((yyvaluep->list)) g_list_free((yyvaluep->list)); };

/* Line 1391 of yacc.c  */
#line 1368 "external_y.c"
	break;
      case 46: /* "evaloption" */

/* Line 1391 of yacc.c  */
#line 169 "external_y.y"
	{ if ((yyvaluep->list)) g_list_free((yyvaluep->list)); };

/* Line 1391 of yacc.c  */
#line 1377 "external_y.c"
	break;
      case 47: /* "sessionoptions" */

/* Line 1391 of yacc.c  */
#line 169 "external_y.y"
	{ if ((yyvaluep->list)) g_list_free((yyvaluep->list)); };

/* Line 1391 of yacc.c  */
#line 1386 "external_y.c"
	break;
      case 48: /* "evaloptions" */

/* Line 1391 of yacc.c  */
#line 169 "external_y.y"
	{ if ((yyvaluep->list)) g_list_free((yyvaluep->list)); };

/* Line 1391 of yacc.c  */
#line 1395 "external_y.c"
	break;
      case 49: /* "boardcommand" */

/* Line 1391 of yacc.c  */
#line 170 "external_y.y"
	{ if ((yyvaluep->gv)) { g_value_unsetfree((yyvaluep->gv)); }};

/* Line 1391 of yacc.c  */
#line 1404 "external_y.c"
	break;
      case 50: /* "evalcommand" */

/* Line 1391 of yacc.c  */
#line 170 "external_y.y"
	{ if ((yyvaluep->gv)) { g_value_unsetfree((yyvaluep->gv)); }};

/* Line 1391 of yacc.c  */
#line 1413 "external_y.c"
	break;
      case 51: /* "board" */

/* Line 1391 of yacc.c  */
#line 169 "external_y.y"
	{ if ((yyvaluep->list)) g_list_free((yyvaluep->list)); };

/* Line 1391 of yacc.c  */
#line 1422 "external_y.c"
	break;
      case 52: /* "float_type" */

/* Line 1391 of yacc.c  */
#line 170 "external_y.y"
	{ if ((yyvaluep->gv)) { g_value_unsetfree((yyvaluep->gv)); }};

/* Line 1391 of yacc.c  */
#line 1431 "external_y.c"
	break;
      case 53: /* "string_type" */

/* Line 1391 of yacc.c  */
#line 170 "external_y.y"
	{ if ((yyvaluep->gv)) { g_value_unsetfree((yyvaluep->gv)); }};

/* Line 1391 of yacc.c  */
#line 1440 "external_y.c"
	break;
      case 54: /* "integer_type" */

/* Line 1391 of yacc.c  */
#line 170 "external_y.y"
	{ if ((yyvaluep->gv)) { g_value_unsetfree((yyvaluep->gv)); }};

/* Line 1391 of yacc.c  */
#line 1449 "external_y.c"
	break;
      case 55: /* "boolean_type" */

/* Line 1391 of yacc.c  */
#line 170 "external_y.y"
	{ if ((yyvaluep->gv)) { g_value_unsetfree((yyvaluep->gv)); }};

/* Line 1391 of yacc.c  */
#line 1458 "external_y.c"
	break;
      case 56: /* "list_type" */

/* Line 1391 of yacc.c  */
#line 170 "external_y.y"
	{ if ((yyvaluep->gv)) { g_value_unsetfree((yyvaluep->gv)); }};

/* Line 1391 of yacc.c  */
#line 1467 "external_y.c"
	break;
      case 57: /* "basic_types" */

/* Line 1391 of yacc.c  */
#line 170 "external_y.y"
	{ if ((yyvaluep->gv)) { g_value_unsetfree((yyvaluep->gv)); }};

/* Line 1391 of yacc.c  */
#line 1476 "external_y.c"
	break;
      case 58: /* "list" */

/* Line 1391 of yacc.c  */
#line 169 "external_y.y"
	{ if ((yyvaluep->list)) g_list_free((yyvaluep->list)); };

/* Line 1391 of yacc.c  */
#line 1485 "external_y.c"
	break;
      case 59: /* "list_element" */

/* Line 1391 of yacc.c  */
#line 170 "external_y.y"
	{ if ((yyvaluep->gv)) { g_value_unsetfree((yyvaluep->gv)); }};

/* Line 1391 of yacc.c  */
#line 1494 "external_y.c"
	break;
      case 60: /* "list_elements" */

/* Line 1391 of yacc.c  */
#line 169 "external_y.y"
	{ if ((yyvaluep->list)) g_list_free((yyvaluep->list)); };

/* Line 1391 of yacc.c  */
#line 1503 "external_y.c"
	break;

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (scancontext *scanner);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (scancontext *scanner)
#else
int
yyparse (scanner)
    scancontext *scanner;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1806 of yacc.c  */
#line 176 "external_y.y"
    {
            extcmd->ct = COMMAND_NONE;
            YYACCEPT;
        }
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 182 "external_y.y"
    {
            extcmd->pCmdData = (yyvsp[(2) - (3)].list);
            extcmd->ct = COMMAND_SET;
            YYACCEPT;
        }
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 189 "external_y.y"
    {
            extcmd->ct = COMMAND_VERSION;
            YYACCEPT;
        }
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 195 "external_y.y"
    {
            extcmd->ct = COMMAND_HELP;
            YYACCEPT;
        }
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 201 "external_y.y"
    {
            extcmd->ct = COMMAND_EXIT;
            YYACCEPT;
        }
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 207 "external_y.y"
    {
            if ((yyvsp[(1) - (2)].cmd)->cmdType == COMMAND_LIST) {
                g_value_unsetfree((yyvsp[(1) - (2)].cmd)->pvData);
                extcmd->ct = (yyvsp[(1) - (2)].cmd)->cmdType;
                YYACCEPT;
            } else {
                GMap *optionsmap = (GMap *)g_value_get_boxed((GValue *)g_list_nth_data(g_value_get_boxed((yyvsp[(1) - (2)].cmd)->pvData), 1));
                GList *boarddata = (GList *)g_value_get_boxed((GValue *)g_list_nth_data(g_value_get_boxed((yyvsp[(1) - (2)].cmd)->pvData), 0));
                extcmd->ct = (yyvsp[(1) - (2)].cmd)->cmdType;
                extcmd->pCmdData = (yyvsp[(1) - (2)].cmd)->pvData;

                if (g_list_length(boarddata) < MAX_RFBF_ELEMENTS) {
                    GVALUE_CREATE(G_TYPE_INT, int, 0, gvfalse); 
                    GVALUE_CREATE(G_TYPE_INT, int, 1, gvtrue); 
                    GVALUE_CREATE(G_TYPE_FLOAT, float, 0.0, gvfloatzero); 

                    extcmd->bi.gsName = g_string_new(g_value_get_gstring_gchar(boarddata->data));
                    extcmd->bi.gsOpp = g_string_new(g_value_get_gstring_gchar(g_list_nth_data(boarddata, 1)));

                    GList *curbrdpos = g_list_nth(boarddata, 2);
                    int *curarraypos = extcmd->anList;
                    while (curbrdpos != NULL) {
                        *curarraypos++ = g_value_get_int(curbrdpos->data);                
                        curbrdpos = g_list_next(curbrdpos);
                    }

                    extcmd->nPlies = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_PLIES, gvfalse));
                    extcmd->fCrawfordRule = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_CRAWFORDRULE, gvfalse));
                    extcmd->fJacobyRule = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_JACOBYRULE, gvfalse));
                    extcmd->fUsePrune = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_PRUNE, gvfalse));
                    extcmd->fCubeful =  g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_CUBEFUL, gvfalse));
                    extcmd->rNoise = g_value_get_float(str2gv_map_get_key_value(optionsmap, KEY_STR_NOISE, gvfloatzero));
                    extcmd->fDeterministic = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_DETERMINISTIC, gvtrue));
                    extcmd->nResignation = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_RESIGNATION, gvfalse));
                    extcmd->fBeavers = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_BEAVERS, gvtrue));

                    g_value_unsetfree(gvtrue);
                    g_value_unsetfree(gvfalse);
                    g_value_unsetfree(gvfloatzero);
                    g_free((yyvsp[(1) - (2)].cmd));
                    
                    YYACCEPT;
                } else {
                    yyerror(scanner, "Invalid board. Maximum number of elements is 52");
                    g_value_unsetfree((yyvsp[(1) - (2)].cmd)->pvData);                
                    g_free((yyvsp[(1) - (2)].cmd));
                    YYERROR;
                }
            }
        }
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 261 "external_y.y"
    {
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_DEBUG, (yyvsp[(2) - (2)].gv));
        }
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 266 "external_y.y"
    {
            GVALUE_CREATE(G_TYPE_INT, int, 1, gvint); 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_NEWINTERFACE, gvint);
        }
    break;

  case 10:

/* Line 1806 of yacc.c  */
#line 272 "external_y.y"
    {
            GVALUE_CREATE(G_TYPE_INT, int, 0, gvint); 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_NEWINTERFACE, gvint);
        }
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 278 "external_y.y"
    {
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_PROMPT, (yyvsp[(2) - (2)].gv));
        }
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 285 "external_y.y"
    {
            commandinfo *cmdInfo = g_malloc0(sizeof(commandinfo));
            cmdInfo->pvData = (yyvsp[(1) - (1)].gv);
            cmdInfo->cmdType = COMMAND_FIBSBOARD;
            (yyval.cmd) = cmdInfo;
        }
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 293 "external_y.y"
    {
            commandinfo *cmdInfo = g_malloc0(sizeof(commandinfo));
            cmdInfo->pvData = (yyvsp[(1) - (1)].gv);
            cmdInfo->cmdType = COMMAND_EVALUATION;
            (yyval.cmd) = cmdInfo;
        }
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 301 "external_y.y"
    { 
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, (yyvsp[(2) - (2)].list), gvptr);
            g_list_free((yyvsp[(2) - (2)].list));
            commandinfo *cmdInfo = g_malloc0(sizeof(commandinfo));
            cmdInfo->pvData = gvptr;
            cmdInfo->cmdType = COMMAND_LIST;
            (yyval.cmd) = cmdInfo;
        }
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 317 "external_y.y"
    { 
            (yyval.list) = g_list_prepend(NULL, (yyvsp[(1) - (1)].gv)); 
        }
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 322 "external_y.y"
    { 
            (yyval.list) = g_list_prepend((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].gv)); 
        }
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 333 "external_y.y"
    { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_JACOBYRULE, (yyvsp[(2) - (2)].gv)); 
        }
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 338 "external_y.y"
    { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_CRAWFORDRULE, (yyvsp[(2) - (2)].gv));
        }
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 343 "external_y.y"
    { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_RESIGNATION, (yyvsp[(2) - (2)].gv));
        }
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 348 "external_y.y"
    { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_BEAVERS, (yyvsp[(2) - (2)].gv));
        }
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 355 "external_y.y"
    { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_PLIES, (yyvsp[(2) - (2)].gv)); 
        }
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 360 "external_y.y"
    {
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_NOISE, (yyvsp[(2) - (2)].gv)); 
        }
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 365 "external_y.y"
    {
            float floatval = (float) g_value_get_int((yyvsp[(2) - (2)].gv)) / 10000.0f;
            GVALUE_CREATE(G_TYPE_FLOAT, float, floatval, gvfloat); 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_NOISE, gvfloat); 
            g_value_unsetfree((yyvsp[(2) - (2)].gv));
        }
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 373 "external_y.y"
    { 
            (yyval.list) = create_str2int_tuple (KEY_STR_PRUNE, TRUE);
        }
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 378 "external_y.y"
    { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_PRUNE, (yyvsp[(2) - (2)].gv));
        }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 383 "external_y.y"
    { 
            (yyval.list) = create_str2int_tuple (KEY_STR_DETERMINISTIC, TRUE);
        }
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 388 "external_y.y"
    { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_DETERMINISTIC, (yyvsp[(2) - (2)].gv));
        }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 393 "external_y.y"
    { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_CUBEFUL, (yyvsp[(2) - (2)].gv));
        }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 398 "external_y.y"
    { 
            (yyval.list) = create_str2int_tuple (KEY_STR_CUBEFUL, TRUE); 
        }
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 403 "external_y.y"
    { 
            (yyval.list) = create_str2int_tuple (KEY_STR_CUBEFUL, FALSE); 
        }
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 410 "external_y.y"
    { 
            /* Setup the defaults */
            STR2GV_MAPENTRY_CREATE(KEY_STR_JACOBYRULE, fJacoby, G_TYPE_INT, 
                                    int, jacobyentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_CRAWFORDRULE, TRUE, G_TYPE_INT, 
                                    int, crawfordentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_RESIGNATION, FALSE, G_TYPE_INT, 
                                    int, resignentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_BEAVERS, TRUE, G_TYPE_INT, 
                                    int, beaversentry);

            GList *defaults = 
                g_list_prepend(g_list_prepend(g_list_prepend(g_list_prepend(NULL, jacobyentry), crawfordentry), \
                               resignentry), beaversentry);
            (yyval.list) = defaults;
        }
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 428 "external_y.y"
    { 
            STR2GV_MAP_ADD_ENTRY((yyvsp[(1) - (2)].list), (yyvsp[(2) - (2)].list), (yyval.list)); 
        }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 435 "external_y.y"
    { 
            /* Setup the defaults */
            STR2GV_MAPENTRY_CREATE(KEY_STR_JACOBYRULE, fJacoby, G_TYPE_INT, 
                                    int, jacobyentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_CRAWFORDRULE, TRUE, G_TYPE_INT, 
                                    int, crawfordentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_RESIGNATION, FALSE, G_TYPE_INT, 
                                    int, resignentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_BEAVERS, TRUE, G_TYPE_INT, 
                                    int, beaversentry);

            GList *defaults = 
                g_list_prepend(g_list_prepend(g_list_prepend(g_list_prepend(NULL, jacobyentry), crawfordentry), \
                               resignentry), beaversentry);
            (yyval.list) = defaults;
        }
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 453 "external_y.y"
    { 
            STR2GV_MAP_ADD_ENTRY((yyvsp[(1) - (2)].list), (yyvsp[(2) - (2)].list), (yyval.list)); 
        }
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 458 "external_y.y"
    { 
            STR2GV_MAP_ADD_ENTRY((yyvsp[(1) - (2)].list), (yyvsp[(2) - (2)].list), (yyval.list)); 
        }
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 465 "external_y.y"
    {
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, (yyvsp[(1) - (2)].list), gvptr1);
            GVALUE_CREATE(G_TYPE_BOXED_MAP_GV, boxed, (yyvsp[(2) - (2)].list), gvptr2);
            GList *newList = g_list_prepend(g_list_prepend(NULL, gvptr2), gvptr1);  
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, newList, gvnewlist);
            (yyval.gv) = gvnewlist;
            g_list_free(newList);
            g_list_free((yyvsp[(1) - (2)].list));
            g_list_free((yyvsp[(2) - (2)].list));
        }
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 479 "external_y.y"
    {
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, (yyvsp[(3) - (4)].list), gvptr1);
            GVALUE_CREATE(G_TYPE_BOXED_MAP_GV, boxed, (yyvsp[(4) - (4)].list), gvptr2);
            
            GList *newList = g_list_prepend(g_list_prepend(NULL, gvptr2), gvptr1);  
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, newList, gvnewlist);
            (yyval.gv) = gvnewlist;
            g_list_free(newList);
            g_list_free((yyvsp[(3) - (4)].list));
            g_list_free((yyvsp[(4) - (4)].list));
        }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 494 "external_y.y"
    {
            GVALUE_CREATE(G_TYPE_GSTRING, boxed, (yyvsp[(4) - (7)].str), gvstr1); 
            GVALUE_CREATE(G_TYPE_GSTRING, boxed, (yyvsp[(2) - (7)].str), gvstr2); 
            (yyvsp[(6) - (7)].list) = g_list_reverse((yyvsp[(6) - (7)].list));
            (yyval.list) = g_list_prepend(g_list_prepend((yyvsp[(6) - (7)].list), gvstr1), gvstr2); 
            g_string_free((yyvsp[(4) - (7)].str), TRUE);
            g_string_free((yyvsp[(2) - (7)].str), TRUE);
        }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 506 "external_y.y"
    { 
            GVALUE_CREATE(G_TYPE_FLOAT, float, (yyvsp[(1) - (1)].floatnum), gvfloat); 
            (yyval.gv) = gvfloat; 
        }
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 514 "external_y.y"
    { 
            GVALUE_CREATE(G_TYPE_GSTRING, boxed, (yyvsp[(1) - (1)].str), gvstr); 
            g_string_free ((yyvsp[(1) - (1)].str), TRUE); 
            (yyval.gv) = gvstr; 
        }
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 523 "external_y.y"
    { 
            GVALUE_CREATE(G_TYPE_INT, int, (yyvsp[(1) - (1)].intnum), gvint); 
            (yyval.gv) = gvint; 
        }
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 531 "external_y.y"
    { 
            GVALUE_CREATE(G_TYPE_INT, int, (yyvsp[(1) - (1)].bool), gvint); 
            (yyval.gv) = gvint; 
        }
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 539 "external_y.y"
    { 
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, (yyvsp[(1) - (1)].list), gvptr);
            g_list_free((yyvsp[(1) - (1)].list));
            (yyval.gv) = gvptr;
        }
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 554 "external_y.y"
    { 
            (yyval.list) = g_list_reverse((yyvsp[(2) - (3)].list));
        }
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 565 "external_y.y"
    { 
            (yyval.list) = NULL; 
        }
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 570 "external_y.y"
    { 
            (yyval.list) = g_list_prepend(NULL, (yyvsp[(1) - (1)].gv));
        }
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 575 "external_y.y"
    { 
            (yyval.list) = g_list_prepend((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].gv)); 
        }
    break;



/* Line 1806 of yacc.c  */
#line 2341 "external_y.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (scanner, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (scanner, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, scanner);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (scanner, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, scanner);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 2067 of yacc.c  */
#line 579 "external_y.y"


#ifdef EXTERNAL_TEST

/* 
 * Test code can be built by configuring GNUBG with --without-gtk option and doing the following:
 *
 * ./ylwrap external_l.l lex.yy.c external_l.c -- flex 
 * ./ylwrap external_y.y y.tab.c external_y.c y.tab.h test1_y.h -- bison 
 * gcc -Ilib -I. -Wall `pkg-config  gobject-2.0 --cflags --libs` external_l.c external_y.c  glib-ext.c -DEXTERNAL_TEST -o exttest
 *
 */
 
#define BUFFERSIZE 1024

int fJacoby = TRUE;

int main()
{
    char buffer[BUFFERSIZE];
    scancontext scanctx;

    memset(&scanctx, 0, sizeof(scanctx));
    g_type_init ();
    ExtInitParse((void **)&scanctx);

    while(fgets(buffer, BUFFERSIZE, stdin) != NULL) {
        ExtStartParse(scanctx.scanner, buffer);
        if(scanctx.ct == COMMAND_EXIT)
            return 0;
        
        if (scanctx.bi.gsName)
            g_string_free(scanctx.bi.gsName, TRUE);
        if (scanctx.bi.gsOpp)
            g_string_free(scanctx.bi.gsOpp, TRUE);

        scanctx.bi.gsName = NULL;
        scanctx.bi.gsOpp = NULL;
    }

    ExtDestroyParse(scanctx.scanner);
    return 0;
}

#endif

