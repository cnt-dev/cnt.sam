// Copyright (c) 2018, Hunt Zhan
#ifndef CNT_SAM_CPP11_SAM_H_
#define CNT_SAM_CPP11_SAM_H_

#include <unordered_map>
#include <vector>

namespace sam {

// Helper to create accessor/mutator for private member name_.
#define DEFINE_ACCESSOR_AND_MUTATOR(name, type) \
  const type& name() const {                    \
    return name ## _;                           \
  }                                             \
  void set_ ## name(const type &val) {          \
    name ## _ = val;                            \
  }                                             \

// Type aliases.
class SamState;
using CharType = int;
using SamStatePtr = SamState *;
using TransType = std::unordered_map<CharType, SamStatePtr>;

// Classes.
class SamState {
 public:
  DEFINE_ACCESSOR_AND_MUTATOR(link, SamStatePtr)
  DEFINE_ACCESSOR_AND_MUTATOR(maxlen, int)
  DEFINE_ACCESSOR_AND_MUTATOR(touch, int)

  void inc_touch(int inc = 1) {
    touch_ += inc;
  }

  int minlen() const {
    return link_ == nullptr ? 0 : link_->maxlen() + 1;
  }

  bool has_trans(const CharType c) const {
    return trans_.count(c) > 0;
  }

  SamStatePtr trans(const CharType c) const {
    return trans_.at(c);
  }

  const TransType &trans() const {
    return trans_;
  }

  void set_trans(const CharType c, SamStatePtr v) {
    trans_[c] = v;
  }

  void copy_trans(const SamStatePtr other) {
    trans_ = other->trans_;
  }

  SamStatePtr Walk(const std::vector<CharType> &factor);

 private:
  SamStatePtr link_ = nullptr;
  TransType trans_ = {};
  // Empty if maxlen_ & touch_ are zero.
  int maxlen_ = 0;
  int touch_ = 0;
};

class SamStateOpt {
 public:
  SamStateOpt();
  ~SamStateOpt();

  void OnlineConstructSymbol(CharType symbol, int maxlen_limit);
  void OnlineConstructFactor(
      const std::vector<CharType> &factor, int maxlen_limit);
  void Finalize();

  // Inference.
  SamStatePtr Walk(const std::vector<CharType> &factor);

  int OccurCount(const std::vector<CharType> &factor);
  double OccurDegree(const std::vector<CharType> &factor, double cap);

  int OutCount(const std::vector<CharType> &factor);
  double OutDegree(const std::vector<CharType> &factor, const CharType symbol);

 private:
  SamStatePtr root_ = nullptr;
  SamStatePtr last_ = nullptr;
  bool fianlized_ = false;

  std::unordered_map<CharType, int> symbol_cnt_;
  int symbol_total_cnt_ = 0;
};

}  // namespace sam

#endif  // CNT_SAM_CPP11_SAM_H_
