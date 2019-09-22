/*
 * sgf.y
 *
 * Generic SGF file parser.
 *
 * by Gary Wong <gtw@gnu.org>, 2000.
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
 * $Id: sgf_y.y,v 1.10 2015/01/11 15:50:26 plm Exp $
 */

%{
#include "config.h"
#include "list.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>

#include "sgf.h"

static listOLD *plCollection;    
    
extern int sgflex( void );

#define YYERROR_VERBOSE

void ( *SGFErrorHandler )( const char *, int ) = NULL;
 
static int sgferror( const char *s ) {

    if( SGFErrorHandler )
	SGFErrorHandler( s, 1 );
    else
	fprintf( stderr, "%s\n", s );
    
    return 0;
}

static listOLD *NewList( void ) {
    listOLD *pl = calloc(1,  sizeof( *pl ) );
    ListCreate( pl );
    return pl;
}

static char *Concatenate( listOLD *pl ) {

    size_t cch = 0;
    char *sz, *pchDest, *pchSrc;
    
    for( pl = pl->plNext; pl->p; pl = pl->plNext )
	cch += strlen( pl->p );

    pchDest = sz = calloc(1, cch + 1 );
    
    while( pl->plNext != pl ) {
	for( pchSrc = pl->plNext->p; ( *pchDest++ = *pchSrc++ ); )
	    ;

	pchDest--;
	
	free( pl->plNext->p );
	ListDelete( pl->plNext );
    }

    free( pl );
    
    return sz;
}

%}

%name-prefix "sgf"

/* There are 2 shift/reduce conflicts caused by ambiguities at which level
   error handling should be performed (GameTreeSeq, Sequence, and
   PropertySeq); the default shifting produces correct behaviour. */
%expect 2

%union {
    char ach[ 2 ]; /* property identifier */
    char *pch; /* property value */
    property *pp; /* complete property */
    listOLD *pl; /* nodes, sequences, gametrees */
}

%token <ach> PROPERTY <pch> VALUETEXT

%type <pp> Property
%type <pch> Value
%type <pl> Collection GameTreeSeq GameTree Sequence Node PropertySeq ValueSeq
%type <pl> ValueCharSeq

%%
		/* The specification says empty collections are illegal, but
		   we'll try to be accomodating. */
Collection:	GameTreeSeq
		{ $$ = plCollection = $1; }
	;

GameTreeSeq:	/* empty */
		{ $$ = NewList(); }
	|	GameTreeSeq GameTree
		{ ListInsert( $1, $2 ); $$ = $1; }
	|	GameTreeSeq error
	;

GameTree:	'(' Sequence GameTreeSeq ')'
		{ ListInsert( $3->plNext, $2 ); $$ = $3; }
	;

Sequence:	Node
		{ $$ = NewList(); ListInsert( $$, $1 ); }
	|	Sequence Node
		{ ListInsert( $1, $2 ); $$ = $1; }
	|	Sequence error
	;

Node:		';' PropertySeq
		{ $$ = $2; }
	;

PropertySeq:	/* empty */
		{ $$ = NewList(); }
	|	PropertySeq Property
		{ ListInsert( $1, $2 ); $$ = $1; }
		/* FIXME if property already exists in node, augment existing
		   list */
	|	PropertySeq error
	;

Property:	PROPERTY ValueSeq Value
		{ 
		    ListInsert( $2, $3 );
		    $$ = malloc( sizeof(property) ); $$->pl = $2;
		    $$->ach[ 0 ] = $1[ 0 ]; $$->ach[ 1 ] = $1[ 1 ];
		}
	;

ValueSeq:	/* empty */
		{ $$ = NewList(); }
	|	ValueSeq Value
		{ ListInsert( $1, $2 ); $$ = $1; }
	;

Value:		'[' ValueCharSeq ']'
		{ $$ = Concatenate( $2 ); }
	;

ValueCharSeq:	/* empty */
		{ $$ = NewList(); }
	|	ValueCharSeq VALUETEXT
		{ ListInsert( $1, $2 ); $$ = $1; }
	;

%%

extern FILE *sgfin;
extern listOLD *SGFParse( FILE *pf ) {

    
    sgfin = pf;
    plCollection = NULL;

    sgfparse();

    return plCollection;
}
	
#ifdef SGFTEST

#ifdef HAVE_MCHECK_H
#include <mcheck.h>
#endif

static void Indent( int n ) {
    while( n-- )
	putchar( ' ' );
}

static void PrintProperty( property *pp, int n ) {

    listOLD *pl;
    
    Indent( n );
    putchar( pp->ach[ 0 ] );
    if ( pp->ach[ 1 ] ) putchar( pp->ach[ 1 ] );

    for( pl = pp->pl->plNext; pl->p; pl = pl->plNext )
	printf( "[%s]", (char *) pl->p );
    
    putchar( '\n' );
}

static void PrintNode( listOLD *pl, int n ) {

    for( pl = pl->plNext; pl->p; pl = pl->plNext )
	PrintProperty( pl->p, n );
    
    Indent( n ); puts( "-" );
}

static void PrintSequence( listOLD *pl, int n ) {

    for( pl = pl->plNext; pl->p; pl = pl->plNext )
	PrintNode( pl->p, n );
}

static void PrintGameTreeSeq( listOLD *pl, int n );

static void PrintGameTree( listOLD *pl, int n ) {

    pl = pl->plNext;

    Indent( n ); puts( "<<<<<" );
    PrintSequence( pl->p, n );
    PrintGameTreeSeq( pl, n + 1 );
    Indent( n ); puts( ">>>>>" );
}

static void PrintGameTreeSeq( listOLD *pl, int n ) {

    for( pl = pl->plNext; pl->p; pl = pl->plNext )
	PrintGameTree( pl->p, n );
}

static void Error( char *s, int f ) {

    fprintf( stderr, _("sgf error: %s\n"), s );
}

int main( int argc, char *argv[] ) {

    FILE *pf = NULL;
    listOLD *pl;

#ifdef HAVE_MTRACE
    mtrace();
#endif

    SGFErrorHandler = Error;

    if( argc > 1 )
	if( !( pf = fopen( argv[ 1 ], "r" ) ) ) {
	    perror( argv[ 1 ] );
	    return 1;
	}
    
    if( ( pl = SGFParse( pf ? pf : stdin ) ) )
	PrintGameTreeSeq( pl, 0 );
    else {
	puts( _("Fatal error; can't print collection.") );
	fclose( pf );
	return 2;
    }

    fclose( pf );
    return 0;
}
#endif
