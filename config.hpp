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

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <list>
#include <map>
#include <string>
#include <cut.hpp>
#include <input.hpp>
#include <node_mexpr.hpp>
#include <trig_map.hpp>
#include <value.hpp>

struct BitfieldArg;
struct FilterRangeCond;
struct FilterRangeArg;
class Node;
class NodeAlias;
class NodeCut;
class NodeCuttable;
class NodeSignal;
class NodeValue;

/*
 * Config, ie node graph builder.
 */
class Config {
  public:
    Config(char const *);
    ~Config();

    NodeValue *AddAlias(char const *, NodeValue *, uint32_t);
    NodeValue *AddBitfield(BitfieldArg *);
    NodeValue *AddCluster(NodeValue *);
    NodeValue *AddCoarseFine(NodeValue *, NodeValue *, double);
    NodeValue *AddCut(CutPolygon *);
    NodeValue *AddFilterRange(std::vector<FilterRangeCond> const &,
        std::vector<NodeValue *> const &);
    void AddFit(char const *, double, double);
    void AddHist1(char const *, NodeValue *, uint32_t, char const *, char
        const *, bool, double);
    void AddHist2(char const *, NodeValue *, NodeValue *, uint32_t, uint32_t,
        char const *, char const *, char const *, bool, double);
    NodeValue *AddLength(NodeValue *);
    NodeValue *AddMatchIndex(NodeValue *, NodeValue *);
    NodeValue *AddMatchValue(NodeValue *, NodeValue *, double);
    NodeValue *AddMax(NodeValue *);
    NodeValue *AddMeanArith(NodeValue *, NodeValue *);
    NodeValue *AddMeanGeom(NodeValue *, NodeValue *);
    NodeValue *AddMember(NodeValue *, char const *);
    NodeValue *AddMExpr(NodeValue *, double, bool, NodeMExpr::Operation);
    void AddPage(char const *);
    NodeValue *AddPedestal(NodeValue *, double, NodeValue *);
    NodeValue *AddSelectIndex(NodeValue *, uint32_t, uint32_t);
    NodeValue *AddSubMod(NodeValue *, NodeValue *, double);
    NodeValue *AddTot(NodeValue *, NodeValue *, double);
    NodeValue *AddTpat(NodeValue *, uint32_t);
    NodeValue *AddTrigMap(char const *, char const *, NodeValue *, NodeValue
        *, double);
    NodeValue *AddZeroSuppress(NodeValue *, double);

    void AppearanceSet(char const *);
    void ClockMatch(NodeValue *, double);
    void ColormapSet(char const *);
    void HistCutAdd(CutPolygon *);
    unsigned UIRateGet() const;
    void UIRateSet(unsigned);

    std::string GetLocStr() const;
    void SetLoc(int, int);

    void BindSignal(std::string const &, char const *, size_t, Input::Type);
    void DoEvent(Input *);
    Input const *GetInput() const;
    Input *GetInput();
    std::list<std::string> GetSignalList() const;

  private:
    Config(Config const &);
    Config &operator=(Config const &);
    void CutListBind(std::string const &);
    void NodeCutAdd(NodeCut *);
    void NodeCuttableAdd(NodeCuttable *);

    struct FitEntry {
      double k;
      double m;
    };
    std::string m_path;
    int m_line, m_col;
    TrigMap m_trig_map;
    std::map<std::string, NodeAlias *> m_alias_map;
    std::map<std::string, NodeSignal *> m_signal_map;
    std::list<NodeCut *> m_cut_node_list;
    std::map<std::string, NodeCuttable *> m_cuttable_map;
    std::map<std::string, CutPolygon *> m_cut_poly_map;
    // Temps to keep cut-links to be resolved when all histograms exist.
    typedef std::list<CutPolygon *> CutPolyList;
    CutPolyList m_cut_poly_list;
    std::map<std::string, CutPolyList> m_cut_ref_map;
    std::map<std::string, FitEntry> m_fit_map;
    struct {
      NodeValue *node;
      double s_from_ts;
      Input::Scalar ts_prev;
      Input::Scalar ts0;
      double t0;
    } m_clock_match;
    size_t m_colormap;
    unsigned m_ui_rate;
    uint64_t m_evid;
    Input *m_input;
};

#endif
