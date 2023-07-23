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

#include <node_signal.hpp>
#include <config.hpp>
#include <cassert>
#include <sstream>

NodeSignal::NodeSignal(Config &a_config, char const *a_name):
  NodeValue(a_config.GetLocStr()),
  // This is nasty, but a_config will always outlive all nodes.
  m_config(&a_config),
  m_name(a_name),
  m_value(),
  m_M(),
  m_MI(),
  m_ME(),
  m_(),
  m_v()
{
}

void NodeSignal::BindSignal(std::string const &a_suffix, size_t a_id,
    Input::Type a_type)
{
std::cout << m_name << '.' << a_suffix << " id=" << a_id << " type=" << a_type
    << '\n';
  Member **mem;
  if (0 == a_suffix.compare("")) {
    mem = &m_;
  } else if (0 == a_suffix.compare("M")) {
    mem = &m_M;
  } else if (0 == a_suffix.compare("I") || 0 == a_suffix.compare("MI")) {
    mem = &m_MI;
  } else if (0 == a_suffix.compare("ME")) {
    mem = &m_ME;
  } else if (0 == a_suffix.compare("v")) {
    mem = &m_v;
  } else {
    std::cerr << GetLocStr() << ": Weird signal suffix '" << a_suffix <<
        "'.\n";
    throw std::runtime_error(__func__);
  }
  if (*mem) {
    std::cerr << GetLocStr() << ": Signal member '" << m_name << a_suffix <<
        "' already set.\n";
    throw std::runtime_error(__func__);
  }
  *mem = new Member;
  switch (a_type) {
    case Input::Type::kUint64:
      (*mem)->type = Value::Type::kUint64;
      break;
    default:
    std::cerr << GetLocStr() << ": Non-implemented input type (BindSignal).\n";
      throw std::runtime_error(__func__);
  }
  (*mem)->id = a_id;
}

Value const &NodeSignal::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeSignal::Process(uint64_t a_evid)
{
  m_value.Clear();

#define FETCH_SIGNAL_DATA(SUFF) \
  auto const p_##SUFF = m_config->GetInput()->GetData(m_##SUFF->id); \
  auto const p64_##SUFF = static_cast<uint64_t const *>(p_##SUFF.first); \
  auto const len_##SUFF = p_##SUFF.second
#define SIGNAL_LEN_CHECK(l, op, r) do { \
  auto l_ = l; \
  auto r_ = r; \
  if (!(l_ op r_)) { \
    std::cerr << __FILE__ << ':' << __LINE__ << ':' << m_name << \
        ": Signal check failed: (" << \
        #l "=" << l_ << " " #op " " #r "=" << r_ << ").\n"; \
    return; \
  } \
} while (0)
  if (m_ME) {
    // Multi-hit array.
    FETCH_SIGNAL_DATA(M);
    FETCH_SIGNAL_DATA(MI);
    FETCH_SIGNAL_DATA(ME);
    FETCH_SIGNAL_DATA();
    FETCH_SIGNAL_DATA(v);
    SIGNAL_LEN_CHECK(1U, ==, len_M);
    SIGNAL_LEN_CHECK(len_ME, ==, len_MI);
    SIGNAL_LEN_CHECK(1U, ==, len_);
    SIGNAL_LEN_CHECK(*p64_, <=, len_v);
    m_value.SetType(m_v->type);
    uint32_t v_i = 0;
    for (uint32_t i = 0; i < *p64_M; ++i) {
      auto mi = (uint32_t)p64_MI[i];
      auto me = p64_ME[i];
      for (; v_i < me; ++v_i) {
        Value::Scalar s;
        s.u64 = p64_v[v_i];
        m_value.Push(mi, s);
      }
    }
  } else if (m_MI) {
    // Single-hit array.
    FETCH_SIGNAL_DATA();
    FETCH_SIGNAL_DATA(MI);
    FETCH_SIGNAL_DATA(v);
    SIGNAL_LEN_CHECK(1U, ==, len_);
    SIGNAL_LEN_CHECK(*p64_, <=, len_MI);
    SIGNAL_LEN_CHECK(*p64_, <=, len_v);
    m_value.SetType(m_v->type);
    for (uint32_t i = 0; i < *p64_; ++i) {
      Value::Scalar s;
      auto mi = (uint32_t)p64_MI[i];
      s.u64 = p64_v[i];
      m_value.Push(mi, s);
    }
  } else if (m_v) {
    // Non-indexed array.
    FETCH_SIGNAL_DATA();
    FETCH_SIGNAL_DATA(v);
    SIGNAL_LEN_CHECK(1U, ==, len_);
    SIGNAL_LEN_CHECK(*p64_, <=, len_v);
    m_value.SetType(m_v->type);
    for (uint32_t i = 0; i < *p64_; ++i) {
      Value::Scalar s;
      s.u64 = p64_v[i];
      m_value.Push(0, s);
    }
  } else {
    // Scalar.
    FETCH_SIGNAL_DATA();
    SIGNAL_LEN_CHECK(1U, >=, len_);
    m_value.SetType(m_->type);
    Value::Scalar s;
    if (p64_) {
      s.u64 = *p64_;
      m_value.Push(0, s);
    }
  }
}

void NodeSignal::SetLocStr(std::string const &a_loc)
{
  m_loc = a_loc;
}
