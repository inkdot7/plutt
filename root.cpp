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
      Entry(std::string a_name, EDataType a_in_type, Input::Type a_out_type,
            bool a_is_vector, bool a_is_double):
        name(a_name),
        in_type(a_in_type),
        out_type(a_out_type),
        is_vector(a_is_vector),
        is_double(a_is_double),
        val_u8(),
        arr_u8(),
        val_u16(),
        arr_u16(),
        val_u32(),
        arr_u32(),
        val_u64(),
        arr_u64(),
        val_float(),
        arr_float(),
        val_double(),
        arr_double(),
        buf(),
        bufd()
      {
      }
      Entry(Entry const &a_e):
        name(),
        in_type(),
        out_type(),
        is_vector(),
        is_double(),
        val_u8(),
        arr_u8(),
        val_u16(),
        arr_u16(),
        val_u32(),
        arr_u32(),
        val_u64(),
        arr_u64(),
        val_float(),
        arr_float(),
        val_double(),
        arr_double(),
        buf(),
        bufd()
      {
        Copy(a_e);
      }
      Entry &operator=(Entry const &a_e)
      {
        Copy(a_e);
        return *this;
      }
      std::string name;
      EDataType in_type;
      Input::Type out_type;
      bool is_vector;
      bool is_double;
      TTreeReaderValue<uint8_t> *val_u8;
      TTreeReaderArray<uint8_t> *arr_u8;
      TTreeReaderValue<uint16_t> *val_u16;
      TTreeReaderArray<uint16_t> *arr_u16;
      TTreeReaderValue<uint32_t> *val_u32;
      TTreeReaderArray<uint32_t> *arr_u32;
      TTreeReaderValue<uint64_t> *val_u64;
      TTreeReaderArray<uint64_t> *arr_u64;
      TTreeReaderValue<float> *val_float;
      TTreeReaderArray<float> *arr_float;
      TTreeReaderValue<double> *val_double;
      TTreeReaderArray<double> *arr_double;
      Vector<uint64_t> buf;
      Vector<double> bufd;
      private:
      void Copy(Entry const &a_e)
      {
        name = a_e.name;
        in_type = a_e.in_type;
        out_type = a_e.out_type;
        is_vector = a_e.is_vector;
        is_double = a_e.is_double;
        // With great power comes great guns to shoot your foot with.
        // We can cheat a bit here:
        // If we never copy m_branch_vec, copying is only done on resizing so
        // there's really only one place for these.
        // The ownership is really with RootImpl since it allocates, so it
        // must also delete.
        val_u8 = a_e.val_u8;
        arr_u8 = a_e.arr_u8;
        val_u16 = a_e.val_u16;
        arr_u16 = a_e.arr_u16;
        val_u32 = a_e.val_u32;
        arr_u32 = a_e.arr_u32;
        val_u64 = a_e.val_u64;
        arr_u64 = a_e.arr_u64;
        arr_float = a_e.arr_float;
        arr_double = a_e.arr_double;
      }
    };
    std::vector<Entry> m_branch_vec;
    Long64_t m_ev_n;
    Long64_t m_ev_i;
    Long64_t m_ev_i_latch;
    uint64_t m_progress_t_last;
};

RootImpl::RootImpl(Config &a_config, int a_argc, char **a_argv):
  m_chain(a_argv[0]),
  m_reader(&m_chain),
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
    delete it->val_u8;
    delete it->arr_u8;
    delete it->val_u16;
    delete it->arr_u16;
    delete it->val_u32;
    delete it->arr_u32;
    delete it->val_u64;
    delete it->arr_u64;
    delete it->arr_float;
    delete it->arr_double;
  }
}

void RootImpl::BindBranch(Config &a_config, std::string const &a_name, char
    const *a_suffix, char const *a_config_suffix, bool a_optional)
{
  // full = name + suffix
  // If full = "m*", then GetBranch("m*").GetTitle() = "m*".
  // If full = "b.m*", then GetBranch("b.m*").GetTitle() = "m*".
  auto full_name = a_name + (a_suffix ? a_suffix : "");
  auto dot = full_name.find_first_of('.');
  std::string member =
      full_name.npos == dot ? full_name : full_name.substr(dot + 1);

  auto branch = m_chain.GetBranch(full_name.c_str());
  if (!branch) {
    if (a_optional) {
      return;
    }
    std::cerr << full_name << ": Could not find branch for signal.\n";
    throw std::runtime_error(__func__);
  }

  // The title looks like "full_name*" or "full_name[arr_ref]*".
  auto title = branch->GetTitle();
  if (0 != strncmp(member.c_str(), title, member.length())) {
    std::cerr << full_name << ": member=" << member <<
        " title=" << title << " mismatch.\n";
    throw std::runtime_error(__func__);
  }
  auto bracket = &title[member.length()];
  bool is_vector = '[' == *bracket;
  bool is_double = false;

  TClass *exp_cls;
  EDataType exp_type;
  branch->GetExpectedType(exp_cls, exp_type);
  if (exp_cls) {
    std::cerr << full_name << ": Class members not supported.\n";
    throw std::runtime_error(__func__);
  }
  Input::Type out_type;
  switch (exp_type) {
    case kUChar_t:
    case kUShort_t:
    case kUInt_t:
    case kULong_t:
      out_type = Input::Type::kUint64;
      break;
    case kFloat_t:
    case kDouble_t:
      out_type = Input::Type::kDouble;
      is_double = true;
      break;
    default:
      std::cerr << full_name <<
          ": Unsupported ROOT type " << exp_type << ".\n";
      throw std::runtime_error(__func__);
  }

  auto id = m_branch_vec.size();
  m_branch_vec.push_back(RootImpl::Entry(full_name, exp_type, out_type,
        is_vector, is_double));
  auto &entry = m_branch_vec.back();

  // Reader instantiation ladder.
  switch (exp_type) {
#define READER_MAKE_TYPE(root_type, c_type, arr_member, val_member)     \
    case root_type: \
      if (is_vector) { \
        entry.arr_member = new \
            TTreeReaderArray<c_type>(m_reader, full_name.c_str()); \
      } else { \
        entry.val_member = new                                        \
            TTreeReaderValue<c_type>(m_reader, full_name.c_str()); \
      } \
      break
#define READER_MAKE_UINT(root_type, depth) \
    READER_MAKE_TYPE(root_type, uint##depth##_t, arr_u##depth, val_u##depth)

    READER_MAKE_UINT(kUChar_t, 8);
    READER_MAKE_UINT(kUShort_t, 16);
    READER_MAKE_UINT(kUInt_t, 32);
    READER_MAKE_UINT(kULong_t, 64);
    READER_MAKE_TYPE(kFloat_t, float, arr_float, val_float);
    READER_MAKE_TYPE(kDouble_t, double, arr_double, val_double);
    default:
      std::cerr << full_name <<
          ": Non-implemented input type " << out_type << " (BindBranch).\n";
      throw std::runtime_error(__func__);
  }

  a_config.BindSignal(a_name, a_config_suffix, id, out_type);
}

void RootImpl::Buffer()
{
  // Copy from readers to vectors.
  for (auto it = m_branch_vec.begin(); m_branch_vec.end() != it; ++it) {
    // TODO: Error-checking!
    switch (it->in_type) {
#define BUF_COPY_TYPE(root_type, c_buf, arr_member, val_member) \
      case root_type: \
        if (it->is_vector) { \
          auto const size = it->arr_member->GetSize(); \
          it->c_buf.resize(size); \
          for (size_t i = 0; i < size; ++i) { \
            it->c_buf[i] = it->arr_member->At(i); \
          } \
        } else { \
          it->c_buf.resize(1); \
          it->c_buf[0] = **it->val_member; \
        } \
        break
#define BUF_COPY_UINT(root_type, depth)                         \
      BUF_COPY_TYPE(root_type, buf, arr_u##depth, val_u##depth)

      BUF_COPY_UINT(kUChar_t, 8);
      BUF_COPY_UINT(kUShort_t, 16);
      BUF_COPY_UINT(kUInt_t, 32);
      BUF_COPY_UINT(kULong_t, 64);
      BUF_COPY_TYPE(kFloat_t, bufd, arr_float, val_float);
      BUF_COPY_TYPE(kDouble_t, bufd, arr_double, val_double);
      default:
        std::cerr << it->name << ": Non-implemented input type (Buffer).\n";
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
  auto const &entry = m_branch_vec.at(a_id);
  if (!entry.is_double) {
    if (entry.is_vector) {
      if (entry.buf.empty()) {
        return std::make_pair(nullptr, 0);
      }
      return std::make_pair(&entry.buf.at(0), entry.buf.size());
    }
    return std::make_pair(&entry.buf.at(0), 1);
  } else {
    if (entry.is_vector) {
      if (entry.bufd.empty()) {
        return std::make_pair(nullptr, 0);
      }
      return std::make_pair(&entry.bufd.at(0), entry.bufd.size());
    }
    return std::make_pair(&entry.bufd.at(0), 1);
  }
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
