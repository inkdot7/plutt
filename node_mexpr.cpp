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

#include <node_mexpr.hpp>
#include <cassert>
#include <cmath>
#include <util.hpp>

NodeMExpr::NodeMExpr(std::string const &a_loc, NodeValue *a_node, double a_d,
    bool a_node_is_left, Operation a_op):
  NodeValue(a_loc),
  m_node(a_node),
  m_d(a_d),
  m_node_is_left(a_node_is_left),
  m_op(a_op),
  m_value()
{
}

Value const &NodeMExpr::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeMExpr::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_node, a_evid);

  m_value.Clear();

  auto const &val = m_node->GetValue();
  if (Input::kNone == val.GetType()) {
    return;
  }
  m_value.SetType(Input::kDouble);

  auto const &miv = val.GetMI();
  auto const &mev = val.GetME();
  uint32_t vi = 0;
  for (uint32_t i = 0; i < miv.size(); ++i) {
    uint32_t mi = miv[i];
    uint32_t me = mev[i];
    for (; vi < me; ++vi) {
      auto v = val.GetV(vi, true);
      switch (m_op) {
        case ADD:
          v += m_d;
          break;
        case SUB:
          v = m_node_is_left ? v - m_d : m_d - v;
          break;
        case MUL:
          v *= m_d;
          break;
        case DIV:
          v = m_node_is_left ? v / m_d : m_d / v;
          break;
        case COS:
          v = cos(v);
          break;
        case SIN:
          v = cos(v);
          break;
        case TAN:
          v = tan(v);
          break;
        case ACOS:
          v = acos(v);
          break;
        case ASIN:
          v = acos(v);
          break;
        case ATAN:
          v = atan(v);
          break;
        case SQRT:
          v = sqrt(v);
          break;
        case EXP:
          v = exp(v);
          break;
        case LOG:
          v = log(v);
          break;
        case ABS:
          v = std::abs(v);
          break;
        case POW:
          v = m_node_is_left ? pow(v, m_d) : pow(m_d, v);
          break;
      }
      if (!std::isnan(v) && !std::isinf(v)) {
        Input::Scalar s;
        s.dbl = v;
        m_value.Push(mi, s);
      }
    }
  }
}
