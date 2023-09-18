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

#if PLUTT_SDL2

#include <sdl_gui.hpp>
#include <sstream>
#include <implutt.hpp>
#include <util.hpp>

SdlGui::PlotWrap::PlotWrap():
  name(),
  plot(),
  plot_state(0),
  pixels()
{
}

SdlGui::SdlGui(char const *a_title, unsigned a_width, unsigned a_height):
  m_window(new ImPlutt::Window(a_title, (int)a_width, (int)a_height)),
  m_page_vec(),
  m_page_sel()
{
}

SdlGui::~SdlGui()
{
  for (auto it = m_page_vec.begin(); m_page_vec.end() != it; ++it) {
    auto page = *it;
    auto &vec = page->plot_wrap_vec;
    for (auto it2 = vec.begin(); vec.end() != it2; ++it2) {
      auto plot_wrap = *it2;
      delete plot_wrap;
    }
    delete page;
  }
  delete m_window;
}

void SdlGui::AddPage(std::string const &a_name)
{
  m_page_vec.push_back(new Page);
  auto page = m_page_vec.back();
  page->name = a_name;
}

uint32_t SdlGui::AddPlot(std::string const &a_name, Plot *a_plot)
{
  if (m_page_vec.empty()) {
    AddPage("Default");
  }
  auto page = m_page_vec.back();
  auto &vec = page->plot_wrap_vec;
  vec.push_back(new PlotWrap);
  auto plot_wrap = vec.back();
  plot_wrap->name = a_name;
  plot_wrap->plot = a_plot;
  return ((uint32_t)m_page_vec.size() - 1) << 16 | ((uint32_t)vec.size() - 1);
}

bool SdlGui::Draw(double a_event_rate)
{
  if (m_window->DoClose() || ImPlutt::DoQuit()) {
    return false;
  }
  if (m_page_vec.empty()) {
    return true;
  }

  m_window->Begin();

  // Fetch selected page.
  if (m_page_vec.size() > 1) {
    for (auto it = m_page_vec.begin(); m_page_vec.end() != it; ++it) {
      auto page = *it;
      if (m_window->Button(page->name.c_str())) {
        m_page_sel = page;
      }
    }
    m_window->Newline();
    m_window->HorizontalLine();
  }
  if (!m_page_sel) {
    m_page_sel = m_page_vec.front();
  }

  // Measure status bar.
  std::ostringstream oss;
  oss << "Events/s: ";
  if (a_event_rate < 1e3) {
    oss << a_event_rate;
  } else {
    oss << a_event_rate * 1e-3 << "k";
  }
  auto size1 = m_window->TextMeasure(ImPlutt::Window::TEXT_BOLD,
      oss.str().c_str());

  auto status = Status_get();
  auto size2 = m_window->TextMeasure(ImPlutt::Window::TEXT_BOLD,
      status.c_str());

  // Get max and pad a little.
  auto h = std::max(size1.y, size2.y);
  h += h / 5;

  // Draw selected page.
  auto size = m_window->GetSize();
  size.y = std::max(0, size.y - h);
  {
    auto &vec = m_page_sel->plot_wrap_vec;
    int rows = (int)sqrt((double)vec.size());
    int cols = ((int)vec.size() + rows - 1) / rows;
    ImPlutt::Pos elem_size(size.x / cols, size.y / rows);
    ImPlutt::Rect elem_rect;
    elem_rect.x = 0;
    elem_rect.y = 0;
    elem_rect.w = elem_size.x;
    elem_rect.h = elem_size.y;
    int i = 0;
    for (auto it2 = vec.begin(); vec.end() != it2; ++it2) {
      auto plot_wrap = *it2;
      m_window->Push(elem_rect);
      plot_wrap->plot->Draw(this);
      m_window->Pop();
      if (cols == ++i) {
        m_window->Newline();
        i = 0;
      }
    }
  }

  // Draw status line.
  m_window->Newline();
  m_window->HorizontalLine();

  ImPlutt::Rect r;
  r.x = 0;
  r.y = 0;
  r.w = 20 * h;
  r.h = h;
  m_window->Push(r);

  m_window->Advance(ImPlutt::Pos(h, 0));
  m_window->Text(ImPlutt::Window::TEXT_BOLD, oss.str().c_str());

  m_window->Pop();

  m_window->Text(ImPlutt::Window::TEXT_BOLD, status.c_str());

  m_window->End();

  return true;
}

void SdlGui::SetHist1(uint32_t a_id, Axis const &a_axis, bool a_is_log_y,
    std::vector<uint32_t> const &a_v)
{
  auto page = m_page_vec.at(a_id >> 16);
  auto plot_wrap = page->plot_wrap_vec.at(a_id & 0xffff);

#if 0
  // Header.
  m_window->Checkbox("Log-y", &m_is_log_y);
  m_window->Text(ImPlutt::Window::TEXT_NORMAL,
      ", x=%.3g(%.3g)", m_range.GetMean(), m_range.GetSigma());
#endif

  auto dy = m_window->Newline();
  auto size_tot = m_window->GetSize();
  ImPlutt::Pos size(size_tot.x, size_tot.y - dy);

#if 0
  auto minx = m_transform.ApplyAbs(m_axis_copy.min);
  auto maxx = m_transform.ApplyAbs(m_axis_copy.max);
#endif
  auto minx = a_axis.min;
  auto maxx = a_axis.max;

  uint32_t max_y = 1;
  for (auto it = a_v.begin(); a_v.end() != it; ++it) {
    max_y = std::max(max_y, *it);
  }

  ImPlutt::Plot plot(m_window, &plot_wrap->plot_state,
      plot_wrap->name.c_str(), size,
      ImPlutt::Point(minx, 0.0),
      ImPlutt::Point(maxx, max_y * 1.1),
      false);

  m_window->PlotHist1(&plot,
      minx, maxx,
      a_v, (size_t)a_axis.bins);
}

void SdlGui::SetHist2(uint32_t a_id, Axis const &a_axis_x, Axis const
    &a_axis_y, bool a_is_log_z, std::vector<uint32_t> const &a_v)
{
  auto page = m_page_vec.at(a_id >> 16);
  auto plot_wrap = page->plot_wrap_vec.at(a_id & 0xffff);

#if 0
  // Header.
  m_window->Checkbox("Log-z", &m_is_log_z);
  m_window->Text(ImPlutt::Window::TEXT_NORMAL,
      ", x=%.1g(%.1g), y=%.1g(%.1g)",
      m_range_x.GetMean(), m_range_x.GetSigma(),
      m_range_y.GetMean(), m_range_y.GetSigma());
#endif

  auto dy = m_window->Newline();
  auto size_tot = m_window->GetSize();
  ImPlutt::Pos size(size_tot.x, size_tot.y - dy);

#if 0
  auto minx = m_transformx.ApplyAbs(m_axis_x_copy.min);
  auto miny = m_transformy.ApplyAbs(m_axis_y_copy.min);
  auto maxx = m_transformx.ApplyAbs(m_axis_x_copy.max);
  auto maxy = m_transformy.ApplyAbs(m_axis_y_copy.max);
#endif
  auto minx = a_axis_x.min;
  auto miny = a_axis_y.min;
  auto maxx = a_axis_x.max;
  auto maxy = a_axis_y.max;

  ImPlutt::Plot plot(m_window, &plot_wrap->plot_state,
      plot_wrap->name.c_str(), size,
      ImPlutt::Point(minx, miny),
      ImPlutt::Point(maxx, maxy),
      true);

  m_window->PlotHist2(&plot, /*m_colormap*/0,
      ImPlutt::Point(minx, miny),
      ImPlutt::Point(maxx, maxy),
      a_v, a_axis_y.bins, a_axis_x.bins,
      plot_wrap->pixels);
}

#endif
