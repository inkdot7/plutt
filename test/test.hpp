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

#ifndef TEST_HPP
#define TEST_HPP

#define TEST_FAIL_CERR_ ++g_test_fails_; \
    std::cerr << __FILE__ << ':' << __LINE__ << ": "

#define TEST_BOOL(cond) do { \
  auto c_ = cond; \
  if (!c_) { \
    TEST_FAIL_CERR_ << "Condition (" << #cond << ") failed.\n"; \
  } \
} while (0)

#define TEST_CMP(l, op, r) do { \
  auto l_ = l; \
  auto r_ = r; \
  if (!(l_ op r_)) { \
    TEST_FAIL_CERR_ << "Comparison (" \
    << #l "=" << l_ << ' ' << #op << ' ' \
    << #r "=" << r_ << ") failed.\n"; \
  } \
} while (0)

#define TEST_TRY \
    try { (void)0
#define TEST_CATCH \
      TEST_FAIL_CERR_ << "Missing exception.\n"; \
    } catch (...) { } (void)0

extern int g_test_fails_;

class Test {
  public:
    Test();
    virtual ~Test();
    void virtual Run() = 0;
    Test *m_next;

  private:
    Test(Test const &);
    Test &operator=(Test const &);
};

#endif
