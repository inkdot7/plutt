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

#if PLUTT_ROOT

#include <root.hpp>
#include <sys/stat.h>
#include <mutex>
#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>
#include <config.hpp>
#include <util.hpp>

class RootImpl {
  public:
    RootImpl(Config &, int, char **);
    ~RootImpl();
    void Buffer();
    bool Fetch();
    std::pair<void const *, size_t> GetData(size_t);

  private:
    void BindBranch(Config &, std::string const &, char const *, char const *,
        bool);

    TChain m_chain;
    TTreeReader m_reader;
    struct Entry {
      Entry(std::string a_name, Input::Type a_type, size_t a_arr_ref_i):
        name(a_name),
        type(a_type),
        arr_ref_i(a_arr_ref_i),
        val_u32(),
        arr_u32(),
        buf()
      {
      }
      Entry(Entry const &a_e):
        name(),
        type(),
        arr_ref_i(),
        val_u32(),
        arr_u32(),
        buf()
      {
        Copy(a_e);
      }
      Entry &operator=(Entry const &a_e)
      {
        Copy(a_e);
        return *this;
      }
      std::string name;
      Input::Type type;
      // 0 = scalar, 1 = m_branch_vec.at(0) etc.
      size_t arr_ref_i;
      TTreeReaderValue<uint32_t> *val_u32;
      TTreeReaderArray<uint32_t> *arr_u32;
      Vector<uint32_t> buf;
      private:
      void Copy(Entry const &a_e)
      {
        name = a_e.name;
        type = a_e.type;
        arr_ref_i = a_e.arr_ref_i;
        // With great power comes great guns to shoot your foot with.
        // We can cheat a bit here:
        // If we never copy m_branch_vec, copying is only done on resizing so
        // there's really only one place for these.
        // The ownership is really with RootImpl since it allocates, so it
        // must also delete.
        val_u32 = a_e.val_u32;
        arr_u32 = a_e.arr_u32;
      }
    };
    std::mutex m_mutex;
    std::vector<Entry> m_branch_vec;
    Long64_t m_ev_n;
    Long64_t m_ev_i;
    Long64_t m_ev_i_latch;
    uint64_t m_progress_t_last;
};

RootImpl::RootImpl(Config &a_config, int a_argc, char **a_argv):
  m_chain(a_argv[0]),
  m_reader(&m_chain),
  m_mutex(),
  m_branch_vec(),
  m_ev_n(),
  m_ev_i(),
  m_ev_i_latch(),
  m_progress_t_last()
{
  auto signal_list = a_config.GetSignalList();

  // Setup chain.
  for (int i = 1; i < a_argc; ++i) {
    struct stat st;
    if (0 != stat(a_argv[i], &st)) {
      std::cerr << a_argv[i] << ": Could not stat.\n";
      throw std::runtime_error(__func__);
    }
    m_chain.Add(a_argv[i]);
  }
  m_ev_n = m_chain.GetEntries();

  // Look for branches for each signal.
  for (auto it = signal_list.begin(); signal_list.end() != it; ++it) {
    BindBranch(a_config, *it,   "",   "", false);
    BindBranch(a_config, *it,  "M",  "M", true);
    BindBranch(a_config, *it,  "I",  "I", true);
    BindBranch(a_config, *it, "MI", "MI", true);
    BindBranch(a_config, *it, "ME", "ME", true);
    BindBranch(a_config, *it,  "v",  "v", true);
    BindBranch(a_config, *it,  "E",  "v", true);
  }
}

RootImpl::~RootImpl()
{
  for (auto it = m_branch_vec.begin(); m_branch_vec.end() != it; ++it) {
    delete it->val_u32;
    delete it->arr_u32;
  }
}

void RootImpl::BindBranch(Config &a_config, std::string const &a_name,
    char const *a_suffix, char const *a_config_suffix, bool a_optional)
{
  auto full_name = a_name + (a_suffix ? a_suffix : "");
  auto branch = m_chain.GetBranch(full_name.c_str());
  if (!branch) {
    if (a_optional) {
      return;
    }
    std::cerr << full_name << ": Could not find branch for signal.\n";
    throw std::runtime_error(__func__);
  }

  // The title looks like "full_name/type" or "full_name[arr_ref]/type".
  auto title = branch->GetTitle();
  if (0 != strncmp(full_name.c_str(), title, full_name.length())) {
    std::cerr << full_name << ": Title (" << title << ") mismatch.\n";
    throw std::runtime_error(__func__);
  }
  auto rover = &title[full_name.length()];
  size_t arr_ref_i = 0;
  if ('[' == *rover) {
    auto bracket = rover + 1;
    auto bracket_end = strchr(bracket, ']');
    if (!bracket_end) {
      std::cerr << title << ": Broken title bracket mismatch.\n";
      throw std::runtime_error(__func__);
    }
    std::string arr_ref(bracket, static_cast<size_t>(bracket_end - bracket));
    // arr_ref must have been defined recently, search backwards.
    for (size_t j = 0;; ++j) {
      if (m_branch_vec.size() == j) {
        std::cerr << title << ": Array size reference undefined.\n";
        throw std::runtime_error(__func__);
      }
      auto &entry = m_branch_vec.at(j);
      if (0 == arr_ref.compare(entry.name)) {
        arr_ref_i = 1 + j;
        break;
      }
    }
    rover = bracket_end + 1;
  }
  if ('/' != *rover) {
    std::cerr << title << ": Branch title missing '/type'.\n";
    throw std::runtime_error(__func__);
  }
  Input::Type type;
  auto type_str = rover + 1;
  if (0 == strcmp(type_str, "i")) {
    type = Input::Type::kUint32;
  } else {
    std::cerr << title << ": Unsupported type.\n";
    throw std::runtime_error(__func__);
  }

  auto id = m_branch_vec.size();
  m_branch_vec.push_back(RootImpl::Entry(full_name, type, arr_ref_i));
  auto &entry = m_branch_vec.back();

  // Reader instantiation ladder.
  switch (type) {
    case Input::Type::kUint32:
      if (arr_ref_i) {
        entry.arr_u32 = new
            TTreeReaderArray<uint32_t>(m_reader, full_name.c_str());
      } else {
        entry.val_u32 = new
            TTreeReaderValue<uint32_t>(m_reader, full_name.c_str());
      }
      break;
    default:
      throw std::runtime_error(__func__);
  }

  a_config.BindSignal(a_name, a_config_suffix, id, type);
}

void RootImpl::Buffer()
{
  // Copy from readers to vectors.
  const std::lock_guard<std::mutex> lock(m_mutex);
  for (auto it = m_branch_vec.begin(); m_branch_vec.end() != it; ++it) {
    // TODO: Error-checking!
    switch (it->type) {
      case Input::Type::kUint32:
        if (it->arr_ref_i) {
          auto const size = it->arr_u32->GetSize();
          it->buf.resize(size);
          if (size) {
            memcpy(&it->buf[0], &it->arr_u32->At(0),
                sizeof(uint32_t) * size);
          }
        } else {
          it->buf.resize(1);
          it->buf[0] = **it->val_u32;
        }
        break;
      default:
        std::cerr << it->name << ": Only uint32 input supported.\n";
        throw std::runtime_error(__func__);
    }
  }
}

bool RootImpl::Fetch()
{
  if (!m_reader.Next()) {
    return false;
  }

  // Progress meter.
  if (Time_get_ms() > m_progress_t_last + 1000 ||
      m_ev_i + 1 == m_ev_n) {
    auto rate = m_ev_i - m_ev_i_latch;
    std::string prefix = "";
    if (rate > 1000) {
      rate /= 1000;
      prefix = "k";
    }
    std::cout << "Event: " <<
        m_ev_i << "/" << m_ev_n <<
        " (" << rate << prefix << "/s)" <<
        "\033[0K\r" << std::flush;
    m_progress_t_last = Time_get_ms();
    m_ev_i_latch = m_ev_i;
  }
  ++m_ev_i;

  return true;
}

std::pair<void const *, size_t> RootImpl::GetData(size_t a_id)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  auto const &entry = m_branch_vec.at(a_id);
  switch (entry.type) {
    case Input::Type::kUint32:
      if (entry.arr_ref_i) {
        if (entry.buf.empty()) {
          return std::make_pair(nullptr, 0);
        }
        return std::make_pair(&entry.buf.at(0), entry.buf.size());
      } else {
        return std::make_pair(&entry.buf.at(0), 1);
      }
    default:
      break;
  }
  std::cerr << entry.name << ": Only uint32 input supported.\n";
  throw std::runtime_error(__func__);
}

Root::Root(Config &a_config, int a_argc, char **a_argv):
  m_impl(new RootImpl(a_config, a_argc, a_argv))
{
}

Root::~Root()
{
  delete m_impl;
}

void Root::Buffer()
{
  m_impl->Buffer();
}

bool Root::Fetch()
{
  return m_impl->Fetch();
}

std::pair<void const *, size_t> Root::GetData(size_t a_id)
{
  return m_impl->GetData(a_id);
}

#endif
