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

#ifndef NODE_MATCH_INDEX_HPP
#define NODE_MATCH_INDEX_HPP

#include <node.hpp>
#include <value.hpp>

/*
 * Matches the index of both nodes into two new values.
 */
class NodeMatchIndex: public NodeValue {
  public:
    NodeMatchIndex(std::string const &, NodeValue *, NodeValue *);
    Value const &GetValue(uint32_t);
    void Process(uint64_t);

  private:
    NodeMatchIndex(NodeMatchIndex const &);
    NodeMatchIndex &operator=(NodeMatchIndex const &);

    NodeValue *m_node_l;
    NodeValue *m_node_r;
    Value m_val_l;
    Value m_val_r;
};

#endif
