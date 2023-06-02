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

#include <iostream>
#include <test/test.hpp>

namespace {
  Test *g_test_head_ = nullptr;
}

int g_test_fails_;

Test::Test():
  m_next(g_test_head_)
{
  g_test_head_ = this;
}

Test::~Test()
{
}

int main()
{
  std::cout << "Testing...\n";

  for (auto p = g_test_head_; p; p = p->m_next) {
    p->Run();
  }

  if (g_test_fails_) {
    std::cerr << "Failed tests = " << g_test_fails_ << ".\n";
    exit(EXIT_FAILURE);
  }
  std::cout << "All good!\n";
  return 0;
}
