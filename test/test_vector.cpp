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
#include <vector.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_vector_;

void MyTest::Run()
{
  {
    // Test empty vector.
    Vector<int> v;

    TEST_TRY;
    v.at(0);
    TEST_CATCH;
    TEST_TRY;
    v[0];
    TEST_CATCH;

    TEST_TRY;
    v.front();
    TEST_CATCH;
    TEST_TRY;
    v.back();
    TEST_CATCH;

    TEST_BOOL(v.empty());
    TEST_CMP(v.size(), ==, 0U);

    TEST_CMP(v.begin(), ==, v.end());
  }

  {
    // Test 1-element vector.
    Vector<int> v;

    v.push_back(2);

    TEST_CMP(v.at(0), ==, 2);
    TEST_TRY;
    v.at(1);
    TEST_CATCH;
    TEST_CMP(v[0], ==, 2);
    TEST_TRY;
    v[1];
    TEST_CATCH;

    TEST_CMP(v.front(), ==, 2);
    TEST_CMP(v.back(), ==, 2);

    TEST_BOOL(!v.empty());
    TEST_CMP(v.size(), ==, 1U);

    TEST_CMP(v.begin(), !=, v.end());
    int sum = 0;
    for (auto it = v.begin(); v.end() != it; ++it) {
      TEST_CMP(*it, ==, 2);
      sum += *it;
    }
    TEST_CMP(sum, ==, 2);

    ++v.at(0);
    TEST_CMP(v.at(0), ==, 3);
    TEST_CMP(v[0], ==, 3);
    TEST_CMP(v.front(), ==, 3);
    TEST_CMP(v.back(), ==, 3);

    ++v[0];
    TEST_CMP(v.at(0), ==, 4);
    TEST_CMP(v[0], ==, 4);
    TEST_CMP(v.front(), ==, 4);
    TEST_CMP(v.back(), ==, 4);

    ++v.front();
    TEST_CMP(v.at(0), ==, 5);
    TEST_CMP(v[0], ==, 5);
    TEST_CMP(v.front(), ==, 5);
    TEST_CMP(v.back(), ==, 5);

    ++v.back();
    TEST_CMP(v.at(0), ==, 6);
    TEST_CMP(v[0], ==, 6);
    TEST_CMP(v.front(), ==, 6);
    TEST_CMP(v.back(), ==, 6);

    v.clear();

    TEST_TRY;
    v.at(0);
    TEST_CATCH;
    TEST_TRY;
    v[0];
    TEST_CATCH;

    TEST_TRY;
    v.front();
    TEST_CATCH;
    TEST_TRY;
    v.back();
    TEST_CATCH;

    TEST_BOOL(v.empty());
    TEST_CMP(v.size(), ==, 0U);

    TEST_CMP(v.begin(), ==, v.end());
  }

  {
    // Test pre-sized vector.
    Vector<int> v(3);

    TEST_BOOL(!v.empty());
    TEST_CMP(v.size(), ==, 3U);

    TEST_CMP(v.begin() + 3, ==, v.end());
  }

  {
    // Test resizing.
    Vector<int> v;

    TEST_BOOL(v.empty());
    TEST_CMP(v.size(), ==, 0U);

    v.resize(3);

    TEST_BOOL(!v.empty());
    TEST_CMP(v.size(), ==, 3U);

    v.resize(1);

    TEST_BOOL(!v.empty());
    TEST_CMP(v.size(), ==, 1U);
  }
}

}
