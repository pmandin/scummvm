/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

%require "3.0"
%defines "engines/hypno/tokens_arc.h"
%output "engines/hypno/grammar_arc.cpp"
%define api.prefix {HYPNO_ARC_}

%{

#include "common/array.h"
#include "hypno/hypno.h"

#undef yyerror
#define yyerror	 HYPNO_ARC_xerror

Hypno::Shoot *shoot;

extern int HYPNO_ARC_lex();
extern int HYPNO_ARC_parse();
extern int HYPNO_ARC_lineno;

void HYPNO_ARC_xerror(const char *str) {
	error("%s at line %d", str, HYPNO_ARC_lineno);
}

int HYPNO_ARC_wrap() {
	return 1;
}

using namespace Hypno;

%}

%union {
	char *s; /* string value */
	int i;	 /* integer value */
}

%token<s> NAME FILENAME BNTOK SNTOK KNTOK YXTOK
%token<i> NUM
// header
%token COMMENT CTOK DTOK HTOK HETOK RETTOK QTOK ENCTOK
%token PTOK FTOK TTOK TPTOK ATOK VTOK OTOK O1TOK NTOK RTOK ITOK JTOK ZTOK

// body
%token FNTOK NONETOK A0TOK P0TOK WTOK

// end
%token XTOK

// bytes??
%token CB3TOK C02TOK

%%

start: YXTOK { g_parsedArc->mode = $1; } header ZTOK RETTOK body XTOK
	| RETTOK start
	;

header: hline header
	| RETTOK header
	| /* nothing */
	; 

hline: 	CTOK NUM {
		g_parsedArc->id = $2; 
		debugC(1, kHypnoDebugParser, "C %d", $2); }
	| FTOK NUM { debugC(1, kHypnoDebugParser, "F %d", $2); }
	| DTOK NUM  { debugC(1, kHypnoDebugParser, "D %d", $2); }
	| PTOK NUM NUM { debugC(1, kHypnoDebugParser, "P %d %d", $2, $3); }
	| ATOK NUM NUM { debugC(1, kHypnoDebugParser, "A %d %d", $2, $3); }
	| VTOK NUM NUM { debugC(1, kHypnoDebugParser, "V %d %d", $2, $3); }
	| OTOK NUM NUM { debugC(1, kHypnoDebugParser, "O %d %d", $2, $3); }
	| O1TOK NUM NUM { debugC(1, kHypnoDebugParser, "O1 %d %d", $2, $3); }
	| TPTOK FILENAME NUM FILENAME {
		g_parsedArc->transitionVideo = $2;
		g_parsedArc->transitionTime = $3;
		debugC(1, kHypnoDebugParser, "Tp %s %d %s", $2, $3, $4); 
	}
	| TTOK FILENAME NUM { 
		g_parsedArc->transitionVideo = $2;
		g_parsedArc->transitionTime = $3;
		debugC(1, kHypnoDebugParser, "T %s %d", $2, $3); 
	}
	| TTOK NONETOK NUM { debugC(1, kHypnoDebugParser, "T NONE %d", $3); }
	| NTOK FILENAME  { 
		g_parsedArc->background = $2; 
		debugC(1, kHypnoDebugParser, "N %s", $2); 
	}
	| RTOK FILENAME  { debugC(1, kHypnoDebugParser, "R %s", $2); }
	| ITOK FILENAME { 
		g_parsedArc->player = $2; 
		debugC(1, kHypnoDebugParser, "I %s", $2); 
		}
	| QTOK NUM NUM { debugC(1, kHypnoDebugParser, "Q %d %d", $2, $3); }
	| BNTOK FILENAME {
		if (Common::String("B0") == $1)
			g_parsedArc->intros.push_back($2);
		else if(Common::String("B1") == $1 || Common::String("B2") == $1)
			g_parsedArc->winVideos.push_back($2);
		else if(Common::String("B3") == $1 || Common::String("B4") == $1)
			g_parsedArc->defeatVideos.push_back($2);

		debugC(1, kHypnoDebugParser, "BN %s", $2); 
	}
	| SNTOK FILENAME enc {
		if (Common::String("S0") == $1)
			g_parsedArc->music = $2;
		else if (Common::String("S1") == $1)
			g_parsedArc->shootSound = $2;
		else if (Common::String("S4") == $1)
			g_parsedArc->enemySound = $2; 

		debugC(1, kHypnoDebugParser, "SN %s", $2); 
	}
	| HETOK C02TOK NUM NUM { debugC(1, kHypnoDebugParser, "HE %d %d", $3, $4); }
	| HETOK CB3TOK NUM NUM { debugC(1, kHypnoDebugParser, "HE %d %d", $3, $4); }
	| HTOK CB3TOK NUM NUM { 
		g_parsedArc->health = $3;
		debugC(1, kHypnoDebugParser, "H %d %d", $3, $4); 
	}
	;

enc: ENCTOK
	| /* nothing */
	;

body: bline body
	| RETTOK body
	| /* nothing */
	;

bline: FNTOK FILENAME { 
		shoot = new Shoot();
		shoot->animation = $2;
		debugC(1, kHypnoDebugParser, "FN %s", $2); 
	}
	| FNTOK NONETOK { 
		shoot = new Shoot();
		shoot->animation = "NONE";
		debugC(1, kHypnoDebugParser, "FN NONE"); 
	}
	| FTOK FILENAME { 
		shoot = new Shoot();
		shoot->animation = $2;
		debugC(1, kHypnoDebugParser, "FN %s", $2); 
	}
	| ITOK NAME  { 
		shoot->name = $2;
		debugC(1, kHypnoDebugParser, "I %s", $2); 
	}
	| ITOK BNTOK  {  // Workaround for NAME == B1
		shoot->name = $2;
		debugC(1, kHypnoDebugParser, "I %s", $2); 
	}
	| ITOK ATOK  { // Workaround for NAME == A
		shoot->name = "A";
		debugC(1, kHypnoDebugParser, "I A"); 
	}
	| ITOK CTOK  { // Workaround for NAME == C
		shoot->name = "C";
		debugC(1, kHypnoDebugParser, "I C"); 
	}
	| ITOK DTOK  { // Workaround for NAME == D
		shoot->name = "D";
		debugC(1, kHypnoDebugParser, "I D"); 
	}
	| ITOK FTOK  { // Workaround for NAME == F
		shoot->name = "F";
		debugC(1, kHypnoDebugParser, "I F"); 
	}
	| ITOK HTOK  { // Workaround for NAME == H
		shoot->name = "H";
		debugC(1, kHypnoDebugParser, "I H"); 
	}
	| ITOK ITOK  { // Workaround for NAME == I
		shoot->name = "I";
		debugC(1, kHypnoDebugParser, "I I"); 
	}
	| ITOK JTOK  { // Workaround for NAME == I
		shoot->name = "J";
		debugC(1, kHypnoDebugParser, "I J"); 
	}
	| ITOK NTOK  { // Workaround for NAME == N
		shoot->name = "N";
		debugC(1, kHypnoDebugParser, "I N"); 
	}
	| ITOK OTOK  { // Workaround for NAME == O
		shoot->name = "O";
		debugC(1, kHypnoDebugParser, "I O"); 
	}
	| ITOK PTOK  { // Workaround for NAME == P
		shoot->name = "P";
		debugC(1, kHypnoDebugParser, "I P"); 
	}
	| ITOK QTOK  { // Workaround for NAME == Q
		shoot->name = "Q";
		debugC(1, kHypnoDebugParser, "I Q"); 
	}
	| ITOK RTOK  { // Workaround for NAME == R
		shoot->name = "R";
		debugC(1, kHypnoDebugParser, "I R"); 
	}
	| ITOK SNTOK  {  // Workaround for NAME == S1
		shoot->name = $2;
		debugC(1, kHypnoDebugParser, "I %s", $2); 
	}
	| ITOK TTOK  { // Workaround for NAME == T
		shoot->name = "T";
		debugC(1, kHypnoDebugParser, "I T"); 
	}
	| JTOK NUM  {
		debugC(1, kHypnoDebugParser, "J %d", $2); 
	}
	| A0TOK NUM NUM { 
		shoot->position = Common::Point($2, $3);
		debugC(1, kHypnoDebugParser, "A0 %d %d", $2, $3); 
	}
	| RTOK NUM NUM  { debugC(1, kHypnoDebugParser, "R %d %d", $2, $3); }
	| BNTOK NUM NUM { debugC(1, kHypnoDebugParser, "BN %d %d", $2, $3); }
	| KNTOK NUM NUM { 
		//if (Common::String("K0") == $1)
		shoot->explosionFrame = $3;
		debugC(1, kHypnoDebugParser, "KN %d %d", $2, $3);
	}
	| P0TOK NUM NUM { debugC(1, kHypnoDebugParser, "P0 %d %d", $2, $3); }
	| OTOK NUM NUM { 
		debugC(1, kHypnoDebugParser, "O %d %d", $2, $3); 
	}
	| CTOK NUM  { debugC(1, kHypnoDebugParser, "C %d", $2); } 
	| HTOK NUM  { debugC(1, kHypnoDebugParser, "H %d", $2); }
	| WTOK NUM  { debugC(1, kHypnoDebugParser, "W %d", $2); }
	| DTOK NUM  { 
		shoot->damage = $2;
		debugC(1, kHypnoDebugParser, "D %d", $2); 
	}
	| SNTOK FILENAME enc { 
		if (Common::String("S1") == $1)
			shoot->endSound = $2;
		//else if (Common::String("S2") == $1)
		//	shoot->startSound = $2;
		 
		debugC(1, kHypnoDebugParser, "SN %s", $2); }
	| NTOK { debugC(1, kHypnoDebugParser, "N"); }
	| ZTOK {
		g_parsedArc->shoots.push_back(*shoot); 
		//delete shoot; 
		//shoot = nullptr;
		debugC(1, kHypnoDebugParser, "Z"); 
	}
	;

