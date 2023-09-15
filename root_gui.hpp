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

#ifndef ROOT_GUI_HPP
#define ROOT_GUI_HPP

#if PLUTT_ROOT

#include <gui.hpp>
#include <string>

class TCanvas;
class TH1I;
class TH2I;
class THttpServer;

class RootGui: public Gui {
  public:
    RootGui(uint16_t);
    ~RootGui();

    void AddPage(std::string const &);
    uint32_t AddPlot(std::string const &, Plot *);

    bool Draw();

    void SetHist1(uint32_t, Axis const &,
        std::vector<uint32_t> const &);
    void SetHist2(uint32_t, Axis const &, Axis const &,
        std::vector<uint32_t> const &);

  private:
    struct PlotWrap {
      std::string name;
      Plot *plot;
      TH1I *h1;
      TH2I *h2;
    };
    struct Page {
      std::string name;
      TCanvas *canvas;
      std::vector<PlotWrap *> plot_wrap_vec;
    };
    THttpServer *m_server;
    std::vector<Page *> m_page_vec;
};

#endif

#endif
