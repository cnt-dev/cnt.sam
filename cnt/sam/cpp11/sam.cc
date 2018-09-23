// Copyright (c) 2018, Hunt Zhan
#include "cpp11/sam.h"
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <cmath>

namespace sam {

SamStateOpt::SamStateOpt() {
  root_ = new SamState;
  last_ = root_;
}

void SamStateOpt::OnlineConstructSymbol(CharType symbol, int maxlen_limit) {
  symbol_cnt_[symbol] += 1;
  symbol_total_cnt_ += 1;

  SamStatePtr cur = nullptr;
  if (maxlen_limit < 0) {
    // Default SAM behaviour: creating a new state.
    cur = new SamState;

  } else if (maxlen_limit >= 2) {
    // `maxlen_limit` has been set.
    // 1. Find a state with minlen under the limit.
    while (last_->minlen() + 1 > maxlen_limit) {
      last_ = last_->link();
    }
    // 2. Now we have last.minlen + 1 <= maxlen_limit.
    // Check if we need to create a new state. If not, return directly.
    if (last_->has_trans(symbol)) {
      last_ = last_->trans(symbol);
      last_->inc_touch();
      return;
    }
    cur = new SamState;

  } else {
    throw std::runtime_error("maxlen_limit is 0 or 1.");
  }
  // Init touch & maxlen.
  cur->set_touch(1);
  cur->set_maxlen(last_->maxlen() + 1);

  auto p = last_;
  last_ = cur;
  while (p && !p->has_trans(symbol)) {
    p->set_trans(symbol, cur);
    p = p->link();
  }
  // Not exists: trans(p, c) = q
  if (p == nullptr) {
    cur->set_link(root_);
    return;
  }

  auto q = p->trans(symbol);
  if (p->maxlen() + 1 == q->maxlen()) {
    // No need to split q.
    cur->set_link(q);

  } else {
    // Split sq from q and link(q) = sq.
    auto sq = new SamState;
    // Since we will accumulate touch, set touch to 0 here.
    sq->set_touch(0);
    // Set maxlen & copy trans.
    sq->set_maxlen(p->maxlen() + 1);
    sq->copy_trans(q);
    // Redirect trans.
    while (p && p->has_trans(symbol) && p->trans(symbol) == q) {
      p->set_trans(symbol, sq);
      p = p->link();
    }
    // Redirect links.
    sq->set_link(q->link());
    q->set_link(sq);
    // Finally, link(cur) to sq.
    cur->set_link(sq);
  }
}

void SamStateOpt::OnlineConstructFactor(
    const std::vector<CharType> &factor, int maxlen_limit) {
  for (auto symbol : factor) {
    OnlineConstructSymbol(symbol, maxlen_limit);
  }
}


struct SamStateHash {
    size_t operator()(const SamStatePtr val) const {
        static const size_t shift =
            static_cast<size_t>(log2(1 + sizeof(SamState)));
        return reinterpret_cast<size_t>(val) >> shift;
    }
};

using ReversedLinkType = std::unordered_map<
    SamStatePtr, std::vector<SamStatePtr>, SamStateHash>;
using SamStatePtrUnorderedSetType = std::unordered_set<
    SamStatePtr, SamStateHash>;

void SearchSam(
    SamStatePtr root,
    ReversedLinkType *reversed_links_ptr,
    SamStatePtrUnorderedSetType *searched_ptr) {

  auto& reversed_links = *reversed_links_ptr;
  auto& searched = *searched_ptr;

  std::queue<SamStatePtr> q;
  q.push(root);

  while (!q.empty()) {
    auto u = q.front();
    q.pop();

    if (u && searched.count(u) > 0) {
      continue;
    }
    searched.insert(u);

    auto v = u->link();
    if (v) {
      reversed_links[v].push_back(u);
    }

    for (auto &c_ptr : u->trans()) {
      q.push(c_ptr.second);
    }
  }
}

ReversedLinkType CollectReversedLink(SamStatePtr root) {
  SamStatePtrUnorderedSetType searched;
  ReversedLinkType reversed_links;

  SearchSam(root, &reversed_links, &searched);

  return reversed_links;
}

SamStatePtrUnorderedSetType CollectAllStates(SamStatePtr root) {
  SamStatePtrUnorderedSetType searched;
  ReversedLinkType reversed_links;

  SearchSam(root, &reversed_links, &searched);

  return searched;
}

void FinalizeTouch(SamStatePtr state, ReversedLinkType *reversed_links_ptr) {
  if (reversed_links_ptr->count(state) > 0) {
    for (auto child : reversed_links_ptr->at(state)) {
      FinalizeTouch(child, reversed_links_ptr);
      state->inc_touch(child->touch());
    }
  }
}

void SamStateOpt::Finalize() {
  auto reversed_links = CollectReversedLink(root_);
  FinalizeTouch(root_, &reversed_links);
  fianlized_ = true;
}

SamStateOpt::~SamStateOpt() {
  for (auto state : CollectAllStates(root_)) {
    state->set_link(nullptr);
    delete state;
  }
  root_ = last_ = nullptr;
}

SamStatePtr SamState::Walk(const std::vector<CharType> &factor) {
  auto state = this;
  for (auto c : factor) {
    if (state->has_trans(c)) {
      state = state->trans(c);
    } else {
      state = nullptr;
      break;
    }
  }
  return state;
}

SamStatePtr SamStateOpt::Walk(const std::vector<CharType> &factor) {
  if (!fianlized_) {
    throw std::runtime_error("not finalized.");
  }
  return root_->Walk(factor);
}

int SamStateOpt::OccurCount(const std::vector<CharType> &factor) {
  auto state = Walk(factor);
  return state == nullptr ? -1 : state->touch();
}

double SamStateOpt::OccurDegree(const std::vector<CharType> &factor) {
  auto occur = OccurCount(factor);
  if (occur < 0) {
    return -1.0;
  }
  double log_prob = log(occur);
  for (auto c : factor) {
    log_prob -= log(symbol_cnt_.at(c));
  }
  log_prob += (factor.size() - 1) * log(symbol_total_cnt_);
  return exp(log_prob);
}

}  // namespace sam
