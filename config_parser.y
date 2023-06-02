/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023  Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

%{
#include <config_parser.hpp>
#include <node_bitfield.hpp>
#include <util.hpp>

#define LOC_SAVE(arg) g_config->SetLoc(arg.first_line, arg.first_column)

char const *yycppath;
Config *g_config;
void
yycperror(char const *s)
{
	fprintf(stderr, "%s:%d:%d: %s\n",
	    yycppath, yycplloc.first_line, yycplloc.first_column, s);
	abort();
}
int yylex(void);

char *g_cut_str;
std::vector<CutPolygon::Point> g_cut_vec;

struct FitEntry {
	FitEntry(double a_x, double a_y): x(a_x), y(a_y) {}
	double x;
	double y;
};
std::vector<FitEntry> g_fit_vec;

static NodeValue *g_pedestal_tpat;
static uint32_t g_binsx;
static uint32_t g_binsy;
static char *g_transformx;
static char *g_transformy;
static char *g_fit;
static int g_logy, g_logz;
static double g_drop_old = -1.0;

#define CTDC_BITS 12
#define TAMEX3_BITS 11
#define VFTX2_BITS 11

#define CTDC_FREQ 150e6
#define TAMEX3_FREQ 200e6
#define VFTX2_FREQ  200e6
%}

%locations
%define api.prefix {yycp}

%union {
	double dbl;
	int32_t i32;
	char *str;
	struct {
		uint32_t first;
		uint32_t last;
	} range;
	Node *node;
	NodeValue *value;
	struct BitfieldArg *bitfield;
}

%token <dbl> TK_DOUBLE
%token <i32> TK_INTEGER
%token <str> TK_IDENT
%token <str> TK_STRING

%token TK_APPEARANCE
%token TK_BINSX
%token TK_BINSY
%token TK_BITFIELD
%token TK_CLOCK_MATCH
%token TK_CLUSTER
%token TK_COARSE_FINE
%token TK_COLORMAP
%token TK_CTDC
%token TK_CUT
%token TK_DOUBLE_DASH
%token TK_DROP_OLD
%token TK_FIT
%token TK_HIST
%token TK_HIST2D
%token TK_LOGY
%token TK_LOGZ
%token TK_MATCH_INDEX
%token TK_MATCH_VALUE
%token TK_MAX
%token TK_MEAN_ARITH
%token TK_MEAN_GEOM
%token TK_MIN
%token TK_PAGE
%token TK_PEDESTAL
%token TK_S
%token TK_SELECT_INDEX
%token TK_SUB_MOD
%token TK_TAMEX3
%token TK_TOT
%token TK_TPAT
%token TK_TRANSFORMX
%token TK_TRANSFORMY
%token TK_TRIG_MAP
%token TK_VFTX2
%token TK_ZERO_SUPPRESS

%type <value> alias
%type <value> bitfield
%type <bitfield> bitfield_arg
%type <bitfield> bitfield_args
%type <i32> bitmask
%type <i32> bitmask_item
%type <value> coarse_fine
%type <dbl> clock_period
%type <dbl> cut_inline_value
%type <dbl> dbl_only
%type <dbl> double
%type <i32> integer
%type <range> integer_range
%type <value> max
%type <value> mean_arith
%type <value> mean_geom
%type <value> member
%type <dbl> clock_range
%type <value> select_index
%type <value> sub_mod
%type <value> tot
%type <value> tpat
%type <value> trig_map
%type <dbl> unit_time
%type <value> value
%type <value> zero_suppress

%left '-' '+'
%left '*' '/'

%%

stmt_list
	: stmt
	| stmt_list stmt

stmt
	: assign
	| appearance
	| clock_match
	| cluster
	| colormap
	| cut
	| fit
	| hist
	| match_index
	| match_value
	| page
	| pedestal

appearance
	: TK_APPEARANCE '(' TK_STRING ')' {
		LOC_SAVE(@1);
		g_config->AppearanceSet($3);
		free($3);
	}

clock_match
	: TK_CLOCK_MATCH '(' value ',' double ')' {
		g_config->ClockMatch($3, $5);
	}

colormap
	: TK_COLORMAP '(' ')' {
		g_config->ColormapSet(nullptr);
	}
	| TK_COLORMAP '(' TK_STRING ')' {
		LOC_SAVE(@1);
		g_config->ColormapSet($3);
		free($3);
	}

cut_inline_args
	:
	| cut_inline_args ',' cut_inline_arg
cut_inline_arg
	: '(' cut_inline_value ',' cut_inline_value ')' {
		LOC_SAVE(@1);
		g_cut_vec.push_back(CutPolygon::Point($2, $4));
	}
cut_inline_value
	: TK_DOUBLE { $$ = $1; }
	| TK_INTEGER { $$ = $1; }
cut_invoc
	: TK_CUT '(' TK_STRING cut_inline_args ')' {
		LOC_SAVE(@1);
		g_cut_str = $3;
	}
cut
	: TK_IDENT '=' cut_invoc {
		LOC_SAVE(@3);
		auto node = g_config->AddCut(g_cut_str, g_cut_vec);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		free($1);
		free(g_cut_str);
		g_cut_vec.clear();
	}
	| TK_IDENT ',' TK_IDENT '=' cut_invoc {
		LOC_SAVE(@5);
		auto node = g_config->AddCut(g_cut_str, g_cut_vec);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		LOC_SAVE(@3);
		g_config->AddAlias($3, node, 1);
		free($1);
		free($3);
		free(g_cut_str);
		g_cut_vec.clear();
	}

page
	: TK_PAGE '(' TK_STRING ')' {
		g_config->AddPage($3);
		free($3);
	}

double
	: dbl_only  { $$ = $1; }
	| integer { $$ = $1; }
dbl_only
	: TK_DOUBLE { $$ = $1; }
	| dbl_only '+' dbl_only { $$ = $1 + $3; }
	| dbl_only '-' dbl_only { $$ = $1 - $3; }
	| dbl_only '*' dbl_only { $$ = $1 * $3; }
	| dbl_only '/' dbl_only { $$ = $1 / $3; }
	| '-' dbl_only { $$ = -$2; }
	| '(' dbl_only ')' { $$ = $2; }

integer
	: TK_INTEGER { $$ = $1; }
	| integer '+' integer { $$ = $1 + $3; }
	| integer '-' integer { $$ = $1 - $3; }
	| integer '*' integer { $$ = $1 * $3; }
	| integer '/' integer { $$ = $1 / $3; }
	| '-' integer { $$ = -$2; }
	| '(' integer ')' { $$ = $2; }

integer_range
	: integer TK_DOUBLE_DASH integer {
		LOC_SAVE(@1);
		if ($1 > $3) {
			std::cerr << g_config->GetLocStr() <<
			    ": Integer range inverted!\n";
			throw std::runtime_error(__func__);
		}
		$$.first = $1;
		$$.last = $3;
	}

bitmask
	: bitmask_item             { $$ = $1; }
	| bitmask ',' bitmask_item { $$ = $1 | $3; }
bitmask_item
	: TK_INTEGER { $$ = 1 << $1; }
	| TK_INTEGER TK_DOUBLE_DASH TK_INTEGER {
		$$ = (~(uint32_t)0 << $1) & (~(uint32_t)0 >> (31 - $3));
	}

tpat
	: TK_TPAT '(' value ',' bitmask ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddTpat($3, $5);
	}

clock_period
	: double    { $$ = $1; }
	| TK_CTDC   { $$ = 1e9 / CTDC_FREQ; }
	| TK_TAMEX3 { $$ = 1e9 / TAMEX3_FREQ; }
	| TK_VFTX2  { $$ = 1e9 / VFTX2_FREQ; }
clock_range
	: double    { $$ = $1; }
	| TK_CTDC   { $$ = (1 << CTDC_BITS) * 1e9 / CTDC_FREQ; }
	| TK_TAMEX3 { $$ = (1 << TAMEX3_BITS) * 1e9 / TAMEX3_FREQ; }
	| TK_VFTX2  { $$ = (1 << VFTX2_BITS) * 1e9 / VFTX2_FREQ; }

unit_time
	: TK_S   { $$ = 1.0; }
	| TK_MIN { $$ = 60.0; }

alias
	: TK_IDENT {
		LOC_SAVE(@1);
		$$ = g_config->AddAlias($1, NULL, 0);
		free($1);
	}

fit_args
	: fit_arg
	| fit_args ',' fit_arg
fit_arg
	: '(' double ',' double ')' {
		LOC_SAVE(@1);
		g_fit_vec.push_back(FitEntry($2, $4));
	}
fit
	: TK_IDENT '=' TK_FIT '(' fit_args ')' {
		double k, m;
		FitLinear(
		    &g_fit_vec.at(0).x, sizeof g_fit_vec.at(0),
		    &g_fit_vec.at(0).y, sizeof g_fit_vec.at(0),
		    g_fit_vec.size(), &k, &m);
		g_config->AddFit($1, k, m);
		g_fit_vec.clear();
		free($1);
	}

value
	: alias         { $$ = $1; }
	| bitfield      { $$ = $1; }
	| coarse_fine   { $$ = $1; }
	| max           { $$ = $1; }
	| mean_arith    { $$ = $1; }
	| mean_geom     { $$ = $1; }
	| member        { $$ = $1; }
	| select_index  { $$ = $1; }
	| sub_mod       { $$ = $1; }
	| tot           { $$ = $1; }
	| tpat          { $$ = $1; }
	| trig_map      { $$ = $1; }
	| zero_suppress { $$ = $1; }

member
	: value '.' TK_IDENT {
		LOC_SAVE(@1);
		$$ = g_config->AddMember($1, $3);
		free($3);
	}

assign
	: TK_IDENT '=' value {
		LOC_SAVE(@1);
		g_config->AddAlias($1, $3, 0);
		free($1);
	}

bitfield_args
	: bitfield_arg                   { $$ = $1; }
	| bitfield_args ',' bitfield_arg {
		/* Beware, builds the list in reverse order! */
		$3->next = $1;
		$$ = $3;
	}
bitfield_arg
	: alias ',' integer {
		LOC_SAVE(@1);
		$$ = new BitfieldArg(g_config->GetLocStr(), $1, $3);
	}
bitfield
	: TK_BITFIELD '(' bitfield_args ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddBitfield($3);
	}
cluster
	: TK_IDENT ',' TK_IDENT '=' TK_CLUSTER '(' value ')' {
		LOC_SAVE(@5);
		auto node = g_config->AddCluster($7);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		LOC_SAVE(@3);
		g_config->AddAlias($3, node, 1);
		free($1);
		free($3);
	}
	| TK_IDENT ',' TK_IDENT ',' TK_IDENT '=' TK_CLUSTER '(' value ')' {
		LOC_SAVE(@7);
		auto node = g_config->AddCluster($9);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		LOC_SAVE(@3);
		g_config->AddAlias($3, node, 1);
		LOC_SAVE(@5);
		g_config->AddAlias($5, node, 2);
		free($1);
		free($3);
		free($5);
	}

hist_cut
	: cut_invoc {
		LOC_SAVE(@1);
		g_config->HistCutAdd(g_cut_str, g_cut_vec);
		free(g_cut_str);
		g_cut_vec.clear();
	}
hist_drop_old
	: TK_DROP_OLD '=' double unit_time {
		g_drop_old = $3 * $4;
	}

hist_opts
	:
	| hist_opt_list
hist_opt_list
	: hist_opt
	| hist_opt_list hist_opt
hist_opt
	: ',' hist_arg
hist_arg
	: TK_BINSX '=' integer { g_binsx = $3; }
	| TK_FIT '=' TK_STRING { g_fit = $3; }
	| TK_LOGY { g_logy = 1; }
	| TK_TRANSFORMX '=' TK_IDENT { g_transformx = $3; }
	| hist_cut
	| hist_drop_old
hist2d_opts
	:
	| hist2d_opt_list
hist2d_opt_list
	: hist2d_opt
	| hist2d_opt_list hist2d_opt
hist2d_opt
	: ',' hist2d_arg
hist2d_arg
	: TK_BINSX '=' integer { g_binsx = $3; }
	| TK_BINSY '=' integer { g_binsy = $3; }
	| TK_LOGZ { g_logz = 1; }
	| TK_TRANSFORMX '=' TK_IDENT { g_transformx = $3; }
	| TK_TRANSFORMY '=' TK_IDENT { g_transformy = $3; }
	| hist_cut
	| hist_drop_old

coarse_fine
	: TK_COARSE_FINE '(' value ',' value ',' clock_period ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddCoarseFine($3, $5, $7);
	}
hist
	: TK_HIST '(' TK_STRING ',' value hist_opts ')' {
		LOC_SAVE(@1);
		g_config->AddHist1($3, $5, g_binsx, g_transformx, g_fit,
		    g_logy, g_drop_old);
		g_binsx = 0;
		free(g_transformx); g_transformx = nullptr;
		free(g_fit); g_fit = nullptr;
		g_logy = 0;
		g_drop_old = -1.0;
		free($3);
	}
	| TK_HIST2D '(' TK_STRING ',' value hist2d_opts ')' {
		LOC_SAVE(@1);
		g_config->AddHist2($3, $5, nullptr, g_binsy, g_binsx,
		    g_transformy, g_transformx, g_fit, g_logz, g_drop_old);
		g_binsx = 0;
		g_binsy = 0;
		free(g_transformx); g_transformx = nullptr;
		free(g_transformy); g_transformy = nullptr;
		free(g_fit); g_fit = nullptr;
		g_logz = 0;
		g_drop_old = -1.0;
		free($3);
	}
	| TK_HIST2D '(' TK_STRING ',' value ',' value hist2d_opts ')' {
		LOC_SAVE(@1);
		g_config->AddHist2($3, $5, $7, g_binsy, g_binsx,
		    g_transformy, g_transformx, g_fit, g_logz, g_drop_old);
		g_binsx = 0;
		g_binsy = 0;
		free(g_transformx); g_transformx = nullptr;
		free(g_transformy); g_transformy = nullptr;
		free(g_fit); g_fit = nullptr;
		g_logz = 0;
		g_drop_old = -1.0;
		free($3);
	}
match_index
	: TK_IDENT ',' TK_IDENT '=' TK_MATCH_INDEX '(' value ',' value ')' {
		LOC_SAVE(@5);
		auto node = g_config->AddMatchIndex($7, $9);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		LOC_SAVE(@3);
		g_config->AddAlias($3, node, 1);
		free($1);
		free($3);
	}
match_value
	: TK_IDENT ',' TK_IDENT '=' TK_MATCH_VALUE '(' value ',' value ','
	    double ')' {
		LOC_SAVE(@5);
		auto node = g_config->AddMatchValue($7, $9, $11);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		LOC_SAVE(@3);
		g_config->AddAlias($3, node, 1);
		free($1);
		free($3);
	}
max
	: TK_MAX '(' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddMax($3);
	}
mean_arith
	: TK_MEAN_ARITH '(' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddMeanArith($3, NULL);
	}
	| TK_MEAN_ARITH '(' value ',' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddMeanArith($3, $5);
	}
mean_geom
	: TK_MEAN_GEOM '(' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddMeanGeom($3, NULL);
	}
	| TK_MEAN_GEOM '(' value ',' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddMeanGeom($3, $5);
	}
pedestal_opts
	:
	| pedestal_opt_list
pedestal_opt_list
	: pedestal_opt
	| pedestal_opt_list pedestal_opt
pedestal_opt: ',' pedestal_arg
pedestal_arg
	: TK_TPAT '=' value {
		g_pedestal_tpat = $3;
	}
pedestal
	: TK_IDENT ',' TK_IDENT '=' TK_PEDESTAL
	    '(' value ',' double pedestal_opts ')' {
		LOC_SAVE(@5);
		auto node = g_config->AddPedestal($7, $9, g_pedestal_tpat);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		LOC_SAVE(@3);
		g_config->AddAlias($3, node, 1);
		g_pedestal_tpat = nullptr;
		free($1);
		free($3);
	}
select_index
	: TK_SELECT_INDEX '(' value ',' integer ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddSelectIndex($3, $5, $5);
	}
	| TK_SELECT_INDEX '(' value ',' integer_range ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddSelectIndex($3, $5.first, $5.last);
	}
sub_mod
	: TK_SUB_MOD '(' value ',' value ',' clock_range ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddSubMod($3, $5, $7);
	}
tot
	: TK_TOT '(' value ',' value ',' clock_range ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddTot($3, $5, $7);
	}
trig_map
	: TK_TRIG_MAP '(' TK_STRING ',' TK_STRING ','
	                  value ',' value ',' clock_range ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddTrigMap($3, $5, $7, $9, $11);
		free($3);
		free($5);
	}
zero_suppress
	: TK_ZERO_SUPPRESS '(' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddZeroSuppress($3, 1e-15);
	}
	| TK_ZERO_SUPPRESS '(' value ',' double ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddZeroSuppress($3, $5);
	}

%%
