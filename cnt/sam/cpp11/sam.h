// Copyright (c) 2018, Hunt Zhan
#ifndef CNT_SAM_CPP11_SAM_H_
#define CNT_SAM_CPP11_SAM_H_

#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

namespace sam {

// TODO(huntzhan): provide option to change SamStateIdType.
using SamStateIdType = uint32_t;
const SamStateIdType SAM_STATE_ID_MAX = \
    std::numeric_limits<SamStateIdType>::max();
// TODO(huntzhan): provide option to change SymbolType.
using SymbolType = uint32_t;
using TransType = std::unordered_map<SymbolType, SamStateIdType>;

class SamState {
 public:
  const SamStateIdType link() const {
    return link_;
  }
  void set_link(const SamStateIdType &val) {
    link_ = val;
  }

  const int maxlen() const {
    return maxlen_;
  }
  void set_maxlen(const int &val) {
    maxlen_ = val;
  }

  const int touch() const {
    return touch_;
  }
  void set_touch(const int &val) {
    touch_ = val;
  }
  void inc_touch(int inc = 1) {
    touch_ += inc;
  }

  bool has_trans(const SymbolType c) const {
    return trans_.count(c) > 0;
  }
  SamStateIdType trans(const SymbolType c) const {
    return trans_.at(c);
  }
  const TransType &trans() const {
    return trans_;
  }
  void set_trans(const SymbolType c, SamStateIdType v) {
    trans_[c] = v;
  }
  void copy_trans(const TransType &trans) {
    trans_ = trans;
  }

 private:
  SamStateIdType link_ = 0;
  TransType trans_ = {};
  // Empty if maxlen_ & touch_ are zero.
  int maxlen_ = 0;
  int touch_ = 0;
};

class SamStateOpt {
 public:
  SamStateOpt();

  void OnlineConstructSymbol(SymbolType symbol, int maxlen_limit);
  void OnlineConstructFactor(
      const std::vector<SymbolType> &factor, int maxlen_limit);
  void Finalize();

  // Inference.
  SamStateIdType Walk(const std::vector<SymbolType> &factor);

  int OccurCount(const std::vector<SymbolType> &factor);
  double OccurDegree(const std::vector<SymbolType> &factor, double cap);

  int OutCount(const std::vector<SymbolType> &factor);
  double OutDegree(
      const std::vector<SymbolType> &factor, const SymbolType symbol);

 private:
  SamStateIdType root_ = GetEmptyId();
  SamStateIdType last_ = GetEmptyId();
  bool fianlized_ = false;

  std::unordered_map<SymbolType, int> symbol_cnt_;
  int symbol_total_cnt_ = 0;

  std::vector<SamState> sam_states_ = {};

  // Resource Management.
  // Id Opts.
  SamStateIdType GetLastId() {
    return sam_states_.size() - 1;
  }
  SamStateIdType GetEmptyId() {
    // Use the maximum value to identify empty state.
    return SAM_STATE_ID_MAX;
  }
  bool IsEmpty(SamStateIdType state_id) {
    return state_id == SAM_STATE_ID_MAX;
  }
  // Just return the id.
  SamStateIdType CreateState() {
    if (GetLastId() == SAM_STATE_ID_MAX - 1) {
      throw "All Ids Exhausted.";
    }
    sam_states_.emplace_back();
    Ref(GetLastId()).set_link(GetEmptyId());
    return GetLastId();
  }
  // Get reference to SamState.
  SamState& Ref(SamStateIdType state_id) {
    return sam_states_[state_id];
  }
  // Shortcuts.
  int MinLen(SamStateIdType state_id) {
    auto link = Ref(state_id).link();
    return IsEmpty(link)?
           0 : Ref(link).maxlen() + 1;
  }

  // Used by Finalize.
  std::vector<SamStateIdType> TopologySortedStateIds();
};

}  // namespace sam

#endif  // CNT_SAM_CPP11_SAM_H_
