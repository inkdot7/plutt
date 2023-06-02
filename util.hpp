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

#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <string>
#include <vector>

#define LENGTH(x) (sizeof x / sizeof *x)

class LinearTransform {
  public:
    LinearTransform(double, double);
    double ApplyAbs(double) const;
    double ApplyRel(double) const;

  private:
    double m_k;
    double m_m;
};

void FitLinear(double const *, size_t, double const *, size_t, size_t, double
    *, double *);

// Clips line against rectangle.
// A coordinate 'x' is inside when 'r.x1 <= x < r.x2'.
// Returns false if line is clipped totally.
bool LineClip(
    int, int, int, int,
    int *, int *, int *, int *);

// Caps log input: (a,b) -> log10(max(a, 10^b)).
double Log10Soft(double, double);

// Histogram rebinning.
std::vector<uint32_t> Rebin1(std::vector<uint32_t> const &,
    size_t, double, double,
    size_t, double, double);
std::vector<uint32_t> Rebin2(std::vector<uint32_t> const &,
    size_t, double, double, size_t, double, double,
    size_t, double, double, size_t, double, double);

// SNIP.
std::vector<float> Snip(std::vector<uint32_t> const &, uint32_t);
std::vector<float> Snip2(std::vector<uint32_t> const &, size_t, size_t,
    uint32_t);

// Cyclic subtraction around 0: (a,b,c) -> (a-b+(n+1/2)*c)%c-c/2
double SubMod(double, double, double);

// Tab-completion of filenames.
std::string TabComplete(std::string const &);

// Beautifully global time.
uint64_t Time_get_ms();
void Time_set_ms(uint64_t);
void Time_wait_ms(uint32_t);

// Subtract a double from a uint64_t:
//  -) The u64 is "small" -> treat it like a double.
//  -) The u64 is "huge" -> treat the limit like a u64.
double U64SubDouble(uint64_t, double);

// UTF-8.
// Goes to the character left of str[ofs].
ssize_t Utf8Left(std::string const &, size_t);
// Goes to the character right of str[ofs].
ssize_t Utf8Right(std::string const &, size_t);

#endif
