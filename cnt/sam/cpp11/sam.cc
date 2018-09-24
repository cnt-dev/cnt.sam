// Copyright (c) 2018, Hunt Zhan
#include "cpp11/sam.h"
#include <stdexcept>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <cmath>

namespace sam {

SamStateOpt::SamStateOpt() {
  root_ = CreateState();
  last_ = root_;
}

void SamStateOpt::OnlineConstructSymbol(SymbolType symbol, int maxlen_limit) {
  symbol_cnt_[symbol] += 1;
  symbol_total_cnt_ += 1;

  SamStateIdType cur = GetEmptyId();
  if (maxlen_limit < 0) {
    // Default SAM behaviour: creating a new state.
    cur = CreateState();

  } else if (maxlen_limit >= 2) {
    // `maxlen_limit` has been set.
    // 1. Find a state with minlen under the limit.
    while (MinLen(last_) + 1 > maxlen_limit) {
      last_ = Ref(last_).link();
    }
    // 2. Now we have last.minlen + 1 <= maxlen_limit.
    // Check if we need to create a new state. If not, return directly.
    if (Ref(last_).has_trans(symbol)) {
      last_ = Ref(last_).trans(symbol);
      Ref(last_).inc_touch();
      return;
    }
    cur = CreateState();

  } else {
    throw std::runtime_error("maxlen_limit is 0 or 1.");
  }
  // Init touch & maxlen.
  Ref(cur).set_touch(1);
  Ref(cur).set_maxlen(Ref(last_).maxlen() + 1);

  auto p = last_;
  last_ = cur;
  while (!IsEmpty(p) && !Ref(p).has_trans(symbol)) {
    Ref(p).set_trans(symbol, cur);
    p = Ref(p).link();
  }
  // Not exists: trans(p, c) = q
  if (IsEmpty(p)) {
    Ref(cur).set_link(root_);
    return;
  }

  auto q = Ref(p).trans(symbol);
  if (Ref(p).maxlen() + 1 == Ref(q).maxlen()) {
    // No need to split q.
    Ref(cur).set_link(q);

  } else {
    // Split sq from q and link(q) = sq.
    auto sq = CreateState();
    // Since we will accumulate touch, set touch to 0 here.
    Ref(sq).set_touch(0);
    // Set maxlen & copy trans.
    Ref(sq).set_maxlen(Ref(p).maxlen() + 1);
    Ref(sq).copy_trans(Ref(q).trans());
    // Redirect trans.
    while (!IsEmpty(p)
           && Ref(p).has_trans(symbol) && Ref(p).trans(symbol) == q) {
      Ref(p).set_trans(symbol, sq);
      p = Ref(p).link();
    }
    // Redirect links.
    Ref(sq).set_link(Ref(q).link());
    Ref(q).set_link(sq);
    // Finally, link(cur) to sq.
    Ref(cur).set_link(sq);
  }
}

void SamStateOpt::OnlineConstructFactor(
    const std::vector<SymbolType> &factor, int maxlen_limit) {
  for (auto symbol : factor) {
    OnlineConstructSymbol(symbol, maxlen_limit);
  }
}

std::vector<SamStateIdType> SamStateOpt::TopologySortedStateIds() {
  std::unordered_set<SamStateIdType> searched;
  std::vector<SamStateIdType> sorted;
  std::vector<SamStateIdType> temp;

  for (SamStateIdType state_id = GetLastId(); state_id != 0; --state_id) {
    if (searched.count(state_id) > 0) {
      continue;
    }
    auto cur = state_id;
    while (!IsEmpty(cur) && searched.count(cur) == 0) {
      temp.push_back(cur);
      searched.insert(cur);
      cur = Ref(cur).link();
    }
    sorted.insert(sorted.end(), temp.rbegin(), temp.rend());
    temp.clear();
  }
  return sorted;
}

void SamStateOpt::Finalize() {
  auto sorted = TopologySortedStateIds();
  for (auto iter = sorted.rbegin(); iter != sorted.rend(); ++iter) {
    auto state_id = *iter;
    auto &cur = Ref(state_id);
    if (IsEmpty(cur.link())) {
      continue;
    }
    auto &pre = Ref(cur.link());
    pre.inc_touch(cur.touch());
  }
  fianlized_ = true;
}

SamStateIdType SamStateOpt::Walk(const std::vector<SymbolType> &factor) {
  if (!fianlized_) {
    throw std::runtime_error("not finalized.");
  }

  SamStateIdType state = GetEmptyId();
  for (int idx = 0; idx < factor.size(); ++idx) {
    // Handle the first char. Notice we don't store the id in state.
    if (idx == 0) {
      if (!Ref(root_).has_trans(factor[0])) {
        break;
      }
      state = Ref(root_).trans(factor[0]);
      continue;
    }
    // Handle the rest of chars.
    if (!Ref(state).has_trans(factor[idx])) {
      state = GetEmptyId();
      break;
    }
    state = Ref(state).trans(factor[idx]);
  }
  return state;
}

int SamStateOpt::OccurCount(const std::vector<SymbolType> &factor) {
  // Occurrence of factor.
  auto state = Walk(factor);
  return IsEmpty(state) ? -1 : Ref(state).touch();
}

double SamStateOpt::OccurDegree(
    const std::vector<SymbolType> &factor, double cap) {
  // Measure how "unlikely" the factor is randomly seleted.
  auto occur = OccurCount(factor);
  if (occur < 0) {
    return -1.0;
  }
  // to log space.
  double log_prob = log(occur);
  for (auto c : factor) {
    log_prob -= log(symbol_cnt_.at(c));
  }
  log_prob += (factor.size() - 1) * log(symbol_total_cnt_);
  // check for cap to avoid overflow.
  if (cap > 0 && log_prob > log(cap)) {
    return cap;
  }
  return exp(log_prob);
}

int SamStateOpt::OutCount(const std::vector<SymbolType> &factor) {
  // The number of "options".
  auto state = Walk(factor);
  return IsEmpty(state) ? -1 : Ref(state).trans().size();
}

double SamStateOpt::OutDegree(
    const std::vector<SymbolType> &factor, const SymbolType symbol) {
  // The weight of one option.
  auto state = Walk(factor);
  if (IsEmpty(state) || !Ref(state).has_trans(symbol)) {
    return -1.0;
  }
  auto next_state = Ref(state).trans(symbol);
  // Approximated.
  return exp(log(Ref(next_state).touch()) - log(Ref(state).touch()));
}

}  // namespace sam
