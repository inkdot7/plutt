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

NodeMExpr::NodeMExpr(std::string const &a_loc, NodeValue *a_l, NodeValue *a_r,
    double a_d, Operation a_op):
  NodeValue(a_loc),
  m_l(a_l),
  m_r(a_r),
  m_d(a_d),
  m_mix(),
  m_op(a_op),
  m_value()
{
  if (a_l && a_r) {
    m_mix = 0;
  } else if (a_l) {
    m_mix = 1;
  } else {
    m_mix = 2;
  }
}

Value const &NodeMExpr::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeMExpr::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  Value const *val_l;
  Value const *val_r;
  Value const *val;
  if (m_l) {
    NODE_PROCESS(m_l, a_evid);
    val = val_l = &m_l->GetValue();
    if (Input::kNone == val_l->GetType()) {
      return;
    }
  }
  if (m_r) {
    NODE_PROCESS(m_r, a_evid);
    val = val_r = &m_r->GetValue();
    if (Input::kNone == val_r->GetType()) {
      return;
    }
  }

  m_value.Clear();
  m_value.SetType(Input::kDouble);

  auto const &miv = val->GetMI();
  auto const &mev = val->GetME();
  uint32_t vi = 0;
  for (uint32_t i = 0; i < miv.size(); ++i) {
    if (0 == m_mix) {
      if (val_l->GetMI().at(i) != val_r->GetMI().at(i) ||
          val_l->GetME().at(i) != val_r->GetME().at(i)) {
        std::cerr << GetLocStr() <<
            ": Data operands must have identical structure!\n";
        throw std::runtime_error(__func__);
      }
    }
    uint32_t mi = miv[i];
    uint32_t me = mev[i];
    for (; vi < me; ++vi) {
      double v, l, r;
      switch (m_mix) {
        case 0:
          l = val_l->GetV(vi, true);
          r = val_r->GetV(vi, true);
          break;
        case 1:
          l = val_l->GetV(vi, true);
          r = m_d;
          break;
        case 2:
          l = m_d;
          r = val_r->GetV(vi, true);
          break;
      }
      switch (m_op) {
        case ADD:  v = l + r; break;
        case SUB:  v = l - r; break;
        case MUL:  v = l * r; break;
        case DIV:  v = l / r; break;
        case COS:  v = cos(l); break;
        case SIN:  v = sin(l); break;
        case TAN:  v = tan(l); break;
        case ACOS: v = acos(l); break;
        case ASIN: v = asin(l); break;
        case ATAN: v = atan(l); break;
        case SQRT: v = sqrt(l); break;
        case EXP:  v = exp(l); break;
        case LOG:  v = log(l); break;
        case ABS:  v = std::abs(l); break;
        case POW:  v = pow(l, r); break;
      }
      if (!std::isnan(v) && !std::isinf(v)) {
        Input::Scalar s;
        s.dbl = v;
        m_value.Push(mi, s);
      }
    }
  }
}
