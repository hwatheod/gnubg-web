%{
/*
 * external_l.l -- lexer for external interface
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
 * $Id: external_l.l,v 1.26 2015/01/17 21:52:23 mdpetch Exp $
 */

#include "config.h"
#include "external.h"
#include <stdio.h>

#define EXTERNAL_Y_H
#include "external_y.h"

extern int ext_get_column (yyscan_t yyscanner );
extern void ext_set_column (int column_no, yyscan_t yyscanner );
extern int ext_parse(yyscan_t scanner);

void escapes(const char *cp, char *tp);

#define PROCESS_YYTEXT_STRING \
        if (yytext[0] == '\'' || yytext[0] == '\"') { \
            yytext[strlen(yytext) - 1] = '\0'; \
            escapes(yytext+1, yytext); \
        } \
        yylval->str = g_string_new(yytext);

#define BUFFERSIZE 2048

%}

%option prefix="ext_"
%option outfile="lex.yy.c"
%option reentrant
%option bison-bridge
%option noyywrap
%option noinput
%option nounput
%option 8bit
%option case-insensitive

%x SBOARDP1 
%x SBOARDP2
%x OPTIONS
%x VALLIST

QSTRING                 (\"[^\"\n]*\")|(\'[^\'\n]*\')
EOT                     [\t\r\n ]+
PLAYERSTR               [[:alnum:]_]+
FLOATSTR                [-+]?([0-9]*\.[0-9]+([eE][-+]?[0-9]+)?)
SIGNEDINTSTR            [-+]?[0-9]+
UNSIGNEDINTSTR          [0-9]+
%%

board:                  {   BEGIN(SBOARDP1);
                            return FIBSBOARD;
                        }

prompt{EOT}             {   return PROMPT; }
new{EOT}                {   return NEW; }
old{EOT}                {   return OLD; }
interface{EOT}          {   return E_INTERFACE; }
help{EOT}               {   return HELP; }
set{EOT}                {   return SET; }
debug{EOT}              {   return DEBUG; }
version{EOT}            {   return INTERFACEVERSION; }
(quit|exit){EOT}        {   return EXIT; }
evaluation{EOT}         {   return EVALUATION; }
fibsboard{EOT}          {   return FIBSBOARD; }

<*>(yes|on|true){EOT}   {   yylval->bool = 1; 
                            return (E_BOOLEAN);
                        }
<*>(no|off|false){EOT}  {   yylval->bool = 0; 
                            return (E_BOOLEAN);
                        }


{QSTRING}               {
                            PROCESS_YYTEXT_STRING;
                            return E_STRING;
                        }
		
<*>{FLOATSTR}           {
                            yylval->floatnum = atof(yytext);
                            return E_FLOAT;
                        }
		
<*>{SIGNEDINTSTR}       {   yylval->intnum = atoi(yytext); 
                            return E_INTEGER; 
                        }
                        
[\[\],\(\)\{\}]         return (yytext[0]);

<OPTIONS>{
resignation{EOT}        {   return RESIGNATION; }
beavers{EOT}            {   return BEAVERS; }
crawfordrule{EOT}       {   return CRAWFORDRULE; }
cube{EOT}               {   return CUBE; }
cubeful{EOT}            {   return CUBEFUL; }
cubeless{EOT}           {   return CUBELESS; }
deterministic{EOT}      {   return DETERMINISTIC; }
jacobyrule{EOT}         {   return JACOBYRULE; }
noise{EOT}              {   return NOISE; }
plies{EOT}              {   return PLIES; }
prune{EOT}              {   return PRUNE; }
}
                        
<VALLIST,SBOARDP1,SBOARDP2>: {
                            return (yytext[0]);
                        }
                        
<VALLIST,SBOARDP1,SBOARDP2>{EOT} {
                            BEGIN(OPTIONS);
                            return FIBSBOARDEND;
                        }

<SBOARDP1>{PLAYERSTR}|{QSTRING}   {
                            PROCESS_YYTEXT_STRING;
                            BEGIN(SBOARDP2);
                            return E_STRING;
                        }
<SBOARDP2>{PLAYERSTR}|{QSTRING}   {
                            PROCESS_YYTEXT_STRING;
                            BEGIN(VALLIST);
                            return E_STRING;
                        }

<*>[\t\r\n ]+             ; /* ignore whitespace */

<<EOF>>                 {   BEGIN(INITIAL); 
                            return (EOL); 
                        }

<*>.                    {   BEGIN(INITIAL);
                            yylval->character = yytext[0]; 
                            return (E_CHARACTER);
                        }

%%

void escapes(const char *cp, char *tp)
{
    while (*cp)
    {
	int	cval = 0;

	if (*cp == '\\' && strchr("0123456789xX", cp[1]))
	{
	    char *dp;
	    const char *hex = "00112233445566778899aAbBcCdDeEfF";
	    int dcount = 0;

	    if (*++cp == 'x' || *cp == 'X')
		for (++cp; (dp = strchr(hex, *cp)) && (dcount++ < 2); cp++)
		    cval = (cval * 16) + (dp - hex) / 2;
	    else if (*cp == '0')
		while (strchr("01234567",*cp) != (char*)NULL && (dcount++ < 3))
		    cval = (cval * 8) + (*cp++ - '0');
	    else
		while ((strchr("0123456789",*cp)!=(char*)NULL)&&(dcount++ < 3))
		    cval = (cval * 10) + (*cp++ - '0');
	}
	else if (*cp == '\\')		/* C-style character escapes */
	{
	    switch (*++cp)
	    {
	    case '\\': cval = '\\'; break;
	    case 'n': cval = '\n'; break;
	    case 't': cval = '\t'; break;
	    case 'b': cval = '\b'; break;
	    case 'r': cval = '\r'; break;
	    default: cval = *cp;
	    }
	    cp++;
	}
	else
	    cval = *cp++;
	*tp++ = cval;
    }
    *tp = '\0';
}

void ExtStartParse(yyscan_t scanner, const char* szCommand)
{
    struct yyguts_t * yyg = (struct yyguts_t *) scanner;
    BEGIN(INITIAL);    
	YY_BUFFER_STATE buf_state = NULL;
	buf_state = yy_scan_string(szCommand, scanner);
	ext_parse(scanner);
    yy_flush_buffer(buf_state, scanner);
	yy_delete_buffer(buf_state, scanner);
}

int ExtInitParse(yyscan_t *scancontext)
{
    return ext_lex_init_extra(scancontext, scancontext);
}

void ExtDestroyParse(yyscan_t scancontext)
{
    yylex_destroy(scancontext);
}
