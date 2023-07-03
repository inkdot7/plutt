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

#ifndef PLOT_HPP
#define PLOT_HPP

#include <cstdint>
#include <cstdlib>
#include <list>
#include <mutex>
#include <string>
#include <vector>
#include <implutt.hpp>
#include <util.hpp>
#include <value.hpp>

class Plot;

class Page {
  public:
    Page(std::string const &);
    Page(Page const &);
    void AddPlot(Plot *);
    void Draw(ImPlutt::Window *, ImPlutt::Pos const &);
    std::string const &GetLabel() const;

  private:
    Page &operator=(Page const &);
    std::string m_label;
    std::list<Plot *> m_plot_list;
    ImPlutt::TextInputState *m_textinput_state;
};

struct Axis {
  uint32_t bins;
  double min;
  double max;
};

/*
 * Tries to guess the kind of data and its range, eg:
 *  -) Only integers? Limit # bins to integer multiples.
 *  -) Is there a huge peak? Forget about the tails.
 */
class Range {
  public:
    enum Mode {
      MODE_ALL,
      MODE_STATS
    };
    Range(double);
    void Add(Value::Type, Value::Scalar const &);
    Axis GetExtents(uint32_t) const;
    double GetMax() const;
    double GetMean() const;
    double GetMin() const;
    double GetSigma() const;
    void SetMode(Mode);
  private:
    Mode m_mode;
    Value::Type m_type;
    uint64_t m_drop_old_ms;
    struct {
      double min;
      double max;
      double sum;
      double sum2;
      uint32_t num;
      uint64_t t_oldest;
    } m_stat[10];
    size_t m_stat_i;
};

class Plot {
  public:
    struct Peak {
      Peak(double, double, double, double);
      double peak_x;
      double ofs_y;
      double amp_y;
      double std_x;
    };

    Plot(Page *);
    virtual ~Plot() {}
    virtual void Draw(ImPlutt::Window *, ImPlutt::Pos const &) = 0;
};

class PlotHist: public Plot {
  public:
    enum Fitter {
      FITTER_NONE,
      FITTER_GAUSS
    };

    PlotHist(Page *, std::string const &, uint32_t, LinearTransform const &,
        char const *, bool, double);
    void Draw(ImPlutt::Window *, ImPlutt::Pos const &);
    void Fill(Value::Type, Value::Scalar const &);
    void Fit();
    void Prefill(Value::Type, Value::Scalar const &);

  private:
    void FitGauss(std::vector<uint32_t> const &, Axis const &);

    std::string m_title;
    uint32_t m_xb;
    LinearTransform m_transform;
    Fitter m_fitter;
    Range m_range;
    Axis m_axis;
    std::mutex m_hist_mutex;
    std::vector<uint32_t> m_hist;
    Axis m_axis_copy;
    std::vector<uint32_t> m_hist_copy;
    ImPlutt::CheckboxState m_is_log_y;
    std::vector<Peak> m_peak_vec;
    ImPlutt::PlotState m_plot_state;
};

class PlotHist2: public Plot {
  public:
    PlotHist2(Page *, std::string const &, size_t, uint32_t, uint32_t,
        LinearTransform const &, LinearTransform const &, char const *, bool,
        double);
    void Draw(ImPlutt::Window *, ImPlutt::Pos const &);
    void Fill(
        Value::Type, Value::Scalar const &,
        Value::Type, Value::Scalar const &);
    void Fit();
    void Prefill(
        Value::Type, Value::Scalar const &,
        Value::Type, Value::Scalar const &);

  private:
    std::string m_title;
    size_t m_colormap;
    uint32_t m_xb;
    uint32_t m_yb;
    LinearTransform m_transformx;
    LinearTransform m_transformy;
    Range m_range_x;
    Range m_range_y;
    Axis m_axis_x;
    Axis m_axis_y;
    std::mutex m_hist_mutex;
    std::vector<uint32_t> m_hist;
    Axis m_axis_x_copy;
    Axis m_axis_y_copy;
    std::vector<uint32_t> m_hist_copy;
    ImPlutt::CheckboxState m_is_log_z;
    ImPlutt::PlotState m_plot_state;
    std::vector<uint8_t> m_pixels;
};

// TODO: Should all this be global?
void plot(ImPlutt::Window *, double);
// TODO: Change name...
Page *plot_page_add();
void plot_page_create(char const *);

#endif
