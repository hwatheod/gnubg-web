%{
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

%}
%pure-parser
%file-prefix "y"
%lex-param   {void *scanner}
%parse-param {scancontext *scanner}
     
%defines
%error-verbose
%token-table

%union {
    gboolean bool;
    gchar character;
    gfloat floatnum;
    gint intnum;
    GString *str;
    GValue *gv;
    GList *list;
    commandinfo *cmd;
}

%{
%}

%token EOL EXIT DISABLED INTERFACEVERSION 
%token DEBUG SET NEW OLD OUTPUT E_INTERFACE HELP PROMPT
%token E_STRING E_CHARACTER E_INTEGER E_FLOAT E_BOOLEAN
%token FIBSBOARD FIBSBOARDEND EVALUATION
%token CRAWFORDRULE JACOBYRULE RESIGNATION BEAVERS
%token CUBE CUBEFUL CUBELESS DETERMINISTIC NOISE PLIES PRUNE

%type <bool>        E_BOOLEAN
%type <str>         E_STRING
%type <character>   E_CHARACTER
%type <intnum>      E_INTEGER
%type <floatnum>    E_FLOAT

%type <gv>          float_type
%type <gv>          integer_type
%type <gv>          string_type
%type <gv>          boolean_type
%type <gv>          basic_types

%type <gv>          list_type
%type <gv>          list_element
%type <gv>          board_element

%type <list>        list
%type <list>        list_elements
%type <list>        board
%type <list>        board_elements

%type <list>        evaloptions
%type <list>        evaloption
%type <list>        sessionoption
%type <list>        sessionoptions
%type <list>        setcommand

%type <gv>          evalcommand
%type <gv>          boardcommand
%type <cmd>         command

%destructor { if ($$) g_string_free($$, TRUE); } <str>
%destructor { if ($$) g_list_free($$); } <list>
%destructor { if ($$) { g_value_unsetfree($$); }} <gv>
%destructor { if ($$) { g_free($$); }} <cmd>
%%

commands:
    EOL
        {
            extcmd->ct = COMMAND_NONE;
            YYACCEPT;
        }
    |
    SET setcommand EOL
        {
            extcmd->pCmdData = $2;
            extcmd->ct = COMMAND_SET;
            YYACCEPT;
        }
    |
    INTERFACEVERSION EOL
        {
            extcmd->ct = COMMAND_VERSION;
            YYACCEPT;
        }
    |
    HELP EOL
        {
            extcmd->ct = COMMAND_HELP;
            YYACCEPT;
        }
    |
    EXIT EOL
        {
            extcmd->ct = COMMAND_EXIT;
            YYACCEPT;
        }
    |
    command EOL
        {
            if ($1->cmdType == COMMAND_LIST) {
                g_value_unsetfree($1->pvData);
                extcmd->ct = $1->cmdType;
                YYACCEPT;
            } else {
                GMap *optionsmap = (GMap *)g_value_get_boxed((GValue *)g_list_nth_data(g_value_get_boxed($1->pvData), 1));
                GList *boarddata = (GList *)g_value_get_boxed((GValue *)g_list_nth_data(g_value_get_boxed($1->pvData), 0));
                extcmd->ct = $1->cmdType;
                extcmd->pCmdData = $1->pvData;

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
                    g_free($1);
                    
                    YYACCEPT;
                } else {
                    yyerror(scanner, "Invalid board. Maximum number of elements is 52");
                    g_value_unsetfree($1->pvData);                
                    g_free($1);
                    YYERROR;
                }
            }
        }
        ;
        
setcommand:
    DEBUG boolean_type
        {
            $$ = create_str2gvalue_tuple (KEY_STR_DEBUG, $2);
        }
    |
    E_INTERFACE NEW
        {
            GVALUE_CREATE(G_TYPE_INT, int, 1, gvint); 
            $$ = create_str2gvalue_tuple (KEY_STR_NEWINTERFACE, gvint);
        }
    |
    E_INTERFACE OLD
        {
            GVALUE_CREATE(G_TYPE_INT, int, 0, gvint); 
            $$ = create_str2gvalue_tuple (KEY_STR_NEWINTERFACE, gvint);
        }
    |
    PROMPT string_type
        {
            $$ = create_str2gvalue_tuple (KEY_STR_PROMPT, $2);
        }
    ;
    
command:
    boardcommand
        {
            commandinfo *cmdInfo = g_malloc0(sizeof(commandinfo));
            cmdInfo->pvData = $1;
            cmdInfo->cmdType = COMMAND_FIBSBOARD;
            $$ = cmdInfo;
        }
    |
    evalcommand
        {
            commandinfo *cmdInfo = g_malloc0(sizeof(commandinfo));
            cmdInfo->pvData = $1;
            cmdInfo->cmdType = COMMAND_EVALUATION;
            $$ = cmdInfo;
        }
    |
    DISABLED list 
        { 
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, $2, gvptr);
            g_list_free($2);
            commandinfo *cmdInfo = g_malloc0(sizeof(commandinfo));
            cmdInfo->pvData = gvptr;
            cmdInfo->cmdType = COMMAND_LIST;
            $$ = cmdInfo;
        }
    ;
    
board_element:
    integer_type
    ;

board_elements: 
    board_element 
        { 
            $$ = g_list_prepend(NULL, $1); 
        }
    |
    board_elements ':' board_element 
        { 
            $$ = g_list_prepend($1, $3); 
        }
    ;
    
endboard:
    FIBSBOARDEND
    ;

sessionoption:
    JACOBYRULE boolean_type 
        { 
            $$ = create_str2gvalue_tuple (KEY_STR_JACOBYRULE, $2); 
        }
    | 
    CRAWFORDRULE boolean_type
        { 
            $$ = create_str2gvalue_tuple (KEY_STR_CRAWFORDRULE, $2);
        }
    | 
    RESIGNATION integer_type
        { 
            $$ = create_str2gvalue_tuple (KEY_STR_RESIGNATION, $2);
        }
    | 
    BEAVERS boolean_type
        { 
            $$ = create_str2gvalue_tuple (KEY_STR_BEAVERS, $2);
        }
    ;
    
evaloption:
    PLIES integer_type
        { 
            $$ = create_str2gvalue_tuple (KEY_STR_PLIES, $2); 
        }
    | 
    NOISE float_type
        {
            $$ = create_str2gvalue_tuple (KEY_STR_NOISE, $2); 
        }
    |
    NOISE integer_type
        {
            float floatval = (float) g_value_get_int($2) / 10000.0f;
            GVALUE_CREATE(G_TYPE_FLOAT, float, floatval, gvfloat); 
            $$ = create_str2gvalue_tuple (KEY_STR_NOISE, gvfloat); 
            g_value_unsetfree($2);
        }
    |
    PRUNE
        { 
            $$ = create_str2int_tuple (KEY_STR_PRUNE, TRUE);
        }
    |
    PRUNE boolean_type 
        { 
            $$ = create_str2gvalue_tuple (KEY_STR_PRUNE, $2);
        }
    |
    DETERMINISTIC
        { 
            $$ = create_str2int_tuple (KEY_STR_DETERMINISTIC, TRUE);
        }
    |
    DETERMINISTIC boolean_type 
        { 
            $$ = create_str2gvalue_tuple (KEY_STR_DETERMINISTIC, $2);
        }
    |
    CUBE boolean_type 
        { 
            $$ = create_str2gvalue_tuple (KEY_STR_CUBEFUL, $2);
        }
    |
    CUBEFUL 
        { 
            $$ = create_str2int_tuple (KEY_STR_CUBEFUL, TRUE); 
        }
    |
    CUBELESS
        { 
            $$ = create_str2int_tuple (KEY_STR_CUBEFUL, FALSE); 
        }
    ;

sessionoptions:
    /* Empty */
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
            $$ = defaults;
        }
    |
    sessionoptions sessionoption
        { 
            STR2GV_MAP_ADD_ENTRY($1, $2, $$); 
        }
    ;

evaloptions:
    /* Empty */
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
            $$ = defaults;
        }
    |
    evaloptions evaloption
        { 
            STR2GV_MAP_ADD_ENTRY($1, $2, $$); 
        }
    |
    evaloptions sessionoption
        { 
            STR2GV_MAP_ADD_ENTRY($1, $2, $$); 
        }
    ;

boardcommand:
    board sessionoptions
        {
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, $1, gvptr1);
            GVALUE_CREATE(G_TYPE_BOXED_MAP_GV, boxed, $2, gvptr2);
            GList *newList = g_list_prepend(g_list_prepend(NULL, gvptr2), gvptr1);  
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, newList, gvnewlist);
            $$ = gvnewlist;
            g_list_free(newList);
            g_list_free($1);
            g_list_free($2);
        }
    ;

evalcommand:
    EVALUATION FIBSBOARD board evaloptions
        {
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, $3, gvptr1);
            GVALUE_CREATE(G_TYPE_BOXED_MAP_GV, boxed, $4, gvptr2);
            
            GList *newList = g_list_prepend(g_list_prepend(NULL, gvptr2), gvptr1);  
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, newList, gvnewlist);
            $$ = gvnewlist;
            g_list_free(newList);
            g_list_free($3);
            g_list_free($4);
        }
    ;
        
board:
    FIBSBOARD E_STRING ':' E_STRING ':' board_elements endboard
        {
            GVALUE_CREATE(G_TYPE_GSTRING, boxed, $4, gvstr1); 
            GVALUE_CREATE(G_TYPE_GSTRING, boxed, $2, gvstr2); 
            $6 = g_list_reverse($6);
            $$ = g_list_prepend(g_list_prepend($6, gvstr1), gvstr2); 
            g_string_free($4, TRUE);
            g_string_free($2, TRUE);
        }
    ;
    
float_type:
    E_FLOAT 
        { 
            GVALUE_CREATE(G_TYPE_FLOAT, float, $1, gvfloat); 
            $$ = gvfloat; 
        }
    ;

string_type:
    E_STRING 
        { 
            GVALUE_CREATE(G_TYPE_GSTRING, boxed, $1, gvstr); 
            g_string_free ($1, TRUE); 
            $$ = gvstr; 
        }
    ;

integer_type:
    E_INTEGER 
        { 
            GVALUE_CREATE(G_TYPE_INT, int, $1, gvint); 
            $$ = gvint; 
        }
    ;

boolean_type:
    E_BOOLEAN 
        { 
            GVALUE_CREATE(G_TYPE_INT, int, $1, gvint); 
            $$ = gvint; 
        }
    ;

list_type:
    list 
        { 
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, $1, gvptr);
            g_list_free($1);
            $$ = gvptr;
        } 
    ;
    
    
basic_types:
    boolean_type | string_type | float_type | integer_type
    ;
    
    
list: 
    '(' list_elements ')' 
        { 
            $$ = g_list_reverse($2);
        }
    ;
    
list_element: 
    basic_types | list_type 
    ;
    
list_elements: 
    /* Empty */ 
        { 
            $$ = NULL; 
        }
    | 
    list_element 
        { 
            $$ = g_list_prepend(NULL, $1);
        }
    |
    list_elements ',' list_element 
        { 
            $$ = g_list_prepend($1, $3); 
        }
    ;
%%

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
