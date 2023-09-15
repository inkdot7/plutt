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

void Page::Draw(ImPlutt::Window *a_window, ImPlutt::Pos const &a_size)
{
  if (m_plot_list.empty()) {
    return;
  }
  int rows = (int)sqrt((double)m_plot_list.size());
  int cols = ((int)m_plot_list.size() + rows - 1) / rows;
  ImPlutt::Pos elem_size(a_size.x / cols, a_size.y / rows);
  ImPlutt::Rect elem_rect;
  elem_rect.x = 0;
  elem_rect.y = 0;
  elem_rect.w = elem_size.x;
  elem_rect.h = elem_size.y;
  int i = 0;
  for (auto it2 = m_plot_list.begin(); m_plot_list.end() != it2; ++it2) {
    a_window->Push(elem_rect);
    (*it2)->Draw(a_window, elem_size);
    a_window->Pop();
    if (cols == ++i) {
      a_window->Newline();
      i = 0;
    }
  }
}

SdlGui::SdlGui(uint16_t a_port):
  m_server(),
  m_page_vec()
{
  std::ostringstream oss;
  oss << "http:" << a_port;
  m_server = new THttpServer(oss.str().c_str());
}

SdlGui::~SdlGui()
{
  for (auto it = m_page_vec.begin(); m_page_vec.end() != it; ++it) {
    auto page = *it;
    auto &vec = page->plot_vec;
    for (auto it2 = vec.begin(); vec.end() != it2; ++it2) {
      auto plot = *it2;
      if (plot->h1) {
        m_server->Unregister(plot->h1);
        delete plot->h1;
      }
      if (plot->h2) {
        m_server->Unregister(plot->h2);
        delete plot->h2;
      }
      delete plot;
    }
    delete page;
  }
  delete m_server;
}

void SdlGui::AddPage(char const *a_name)
{
  m_page_vec.push_back(new Page);
  auto page = m_page_vec.back();
  page->name = a_name;
}

uint32_t SdlGui::AddPlot(char const *a_name)
{
  auto page = m_page_vec.back();
  auto &vec = page->plot_vec;
  vec.push_back(new Plot);
  auto plot = vec.back();
  plot->name = a_name;
  return ((uint32_t)m_page_vec.size() - 1) << 16 | ((uint32_t)vec.size() - 1);
}

void SdlGui::SetHist1(uint32_t a_id, Axis const &a_axis,
    std::vector<uint32_t> const &a_v)
{
  auto page = m_page_vec.at(a_id >> 16);
  auto plot = page->plot_vec.at(a_id & 0xffff);
  if (plot->h2) {
    throw std::runtime_error(__func__);
  }
  if (plot->h1) {
    auto axis = plot->h1->GetXaxis();
    if (axis->GetNbins() != a_axis.bins ||
        axis->GetXmin()  != a_axis.min ||
        axis->GetXmax()  != a_axis.max) {
      m_server->Unregister(plot->h1);
      delete plot->h1;
      plot->h1 = nullptr;
    }
  }
  if (!plot->h1) {
    plot->h1 = new TH1I(plot->name.c_str(), plot->name.c_str(),
        (int)a_axis.bins, a_axis.min, a_axis.max);
    m_server->Register((page->name + '/' + plot->name).c_str(), plot->h1);
  }
  for (size_t i = 0; i < a_axis.bins; ++i) {
    plot->h1->SetBinContent(1 + (int)i, a_v.at(i));
  }
}

void SdlGui::SetHist2(uint32_t a_id, Axis const &a_axis_x, Axis const
    &a_axis_y, std::vector<uint32_t> const &a_v)
{
  auto page = m_page_vec.at(a_id >> 16);
  auto plot = page->plot_vec.at(a_id & 0xffff);
  if (plot->h1) {
    throw std::runtime_error(__func__);
  }
  if (plot->h2) {
    auto axis_x = plot->h2->GetXaxis();
    auto axis_y = plot->h2->GetYaxis();
    if (axis_x->GetNbins() != a_axis_x.bins ||
        axis_x->GetXmin()  != a_axis_x.min ||
        axis_x->GetXmax()  != a_axis_x.max ||
        axis_y->GetNbins() != a_axis_y.bins ||
        axis_y->GetXmin()  != a_axis_y.min ||
        axis_y->GetXmax()  != a_axis_y.max) {
      m_server->Unregister(plot->h2);
      delete plot->h2;
      plot->h2 = nullptr;
    }
  }
  if (!plot->h2) {
    plot->h2 = new TH2I(plot->name.c_str(), plot->name.c_str(),
        (int)a_axis_x.bins, a_axis_x.min, a_axis_x.max,
        (int)a_axis_y.bins, a_axis_y.min, a_axis_y.max);
    m_server->Register((page->name + '/' + plot->name).c_str(), plot->h2);
  }
  size_t k = 0;
  for (size_t i = 0; i < a_axis_y.bins; ++i) {
    for (size_t j = 0; j < a_axis_x.bins; ++j) {
      plot->h2->SetBinContent(1 + (int)j, 1 + (int)i, a_v.at(k++));
    }
  }
}

bool SdlGui::Begin()
{
  return gSystem->ProcessEvents();
}

void SdlGui::End()
{
}

#endif
