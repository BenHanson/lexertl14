// generator.hpp
// Copyright (c) 2005-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_GENERATOR_HPP
#define LEXERTL_GENERATOR_HPP

#include "char_traits.hpp"
#include "enum_operator.hpp"
#include "enums.hpp"
#include "internals.hpp"
#include "parser/parser.hpp"
#include "partition/charset.hpp"
#include "partition/equivset.hpp"
#include "observer_ptr.hpp"
#include "rules.hpp"
#include "runtime_error.hpp"
#include "state_machine.hpp"

#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace lexertl
{
    template<typename rules, typename sm,
        typename char_traits = basic_char_traits
        <typename sm::traits::input_char_type> >
        class basic_generator
    {
    public:
        using id_type = typename rules::id_type;
        using rules_char_type = typename rules::rules_char_type;
        using sm_traits = typename sm::traits;
        using parser = detail::basic_parser<rules_char_type, sm_traits>;
        using charset_map = typename parser::charset_map;
        using node = typename parser::node;
        using node_ptr_vector = typename parser::node_ptr_vector;

        static void build(const rules& rules_, sm& sm_)
        {
            const auto size_ =
                static_cast<id_type>(rules_.statemap().size());
            // Strong exception guarantee
            // http://www.boost.org/community/exception_safety.html
            internals internals_;
            sm temp_sm_;
            node_ptr_vector node_ptr_vector_;
            std::set<id_type> used_ids_;
            id_type unique_id_ = 0;

            internals_._eoi = rules_.eoi();
            internals_.add_states(size_);

            for (id_type index_ = 0; index_ < size_; ++index_)
            {
                if (rules_.regexes()[index_].empty())
                {
                    std::ostringstream ss_;

                    ss_ << "Lexer states with no rules are not allowed "
                        "(lexer state " << index_ << ".)";
                    throw runtime_error(ss_.str());
                }
                else
                {
                    // Note that the following variables are per DFA.
                    // Map of regex charset tokens (strings) to index
                    charset_map charset_map_;
                    // Used to fix up $ and \n clashes.
                    id_type cr_id_ = sm_traits::npos();
                    id_type nl_id_ = sm_traits::npos();
                    // Regex syntax tree
                    observer_ptr<node> root_ = build_tree(rules_, index_,
                        node_ptr_vector_, charset_map_, cr_id_, nl_id_,
                        unique_id_);

                    check_zero_len(rules_, root_);
                    build_dfa(charset_map_, root_, internals_, temp_sm_, index_,
                        cr_id_, nl_id_, rules_.flags(), used_ids_);

                    if (internals_._dfa[index_].size() /
                        internals_._dfa_alphabet[index_] >= sm_traits::npos())
                    {
                        // Overflow
                        throw runtime_error("The id_type you have chosen "
                            "cannot hold this many DFA rows.");
                    }
                }
            }

            check_suppressed(rules_, unique_id_, used_ids_);
            // If you get a compile error here the id_type from rules and
            // state machine do no match.
            create(internals_, temp_sm_, rules_.features(), lookup());
            sm_.swap(temp_sm_);
        }

        static observer_ptr<node> build_tree(const rules& rules_,
            const std::size_t dfa_, node_ptr_vector& node_ptr_vector_,
            charset_map& charset_map_, id_type& cr_id_, id_type& nl_id_,
            id_type& unique_id_)
        {
            parser parser_(rules_.locale(), node_ptr_vector_, charset_map_,
                rules_.eoi());
            const auto& regexes_ = rules_.regexes();
            auto regex_iter_ = regexes_[dfa_].cbegin();
            auto regex_iter_end_ = regexes_[dfa_].cend();
            const auto& ids_ = rules_.ids();
            const auto& user_ids_ = rules_.user_ids();
            auto id_iter_ = ids_[dfa_].cbegin();
            auto user_id_iter_ = user_ids_[dfa_].cbegin();
            const auto& next_dfas_ = rules_.next_dfas();
            const auto& pushes_ = rules_.pushes();
            const auto& pops_ = rules_.pops();
            auto next_dfa_iter_ = next_dfas_[dfa_].cbegin();
            auto push_dfa_iter_ = pushes_[dfa_].cbegin();
            auto pop_dfa_iter_ = pops_[dfa_].cbegin();
            const bool seen_bol_ =
                (rules_.features()[dfa_] & *feature_bit::bol) != 0;
            observer_ptr<node> root_ = nullptr;

            root_ = parser_.parse(*regex_iter_, *id_iter_, *user_id_iter_,
                ++unique_id_, *next_dfa_iter_, *push_dfa_iter_, *pop_dfa_iter_,
                cr_id_, nl_id_, seen_bol_);
            ++regex_iter_;
            ++id_iter_;
            ++user_id_iter_;
            ++next_dfa_iter_;
            ++push_dfa_iter_;
            ++pop_dfa_iter_;

            // Build syntax trees
            while (regex_iter_ != regex_iter_end_)
            {
                observer_ptr<node> rhs_ = parser_.parse(*regex_iter_, *id_iter_,
                    *user_id_iter_, ++unique_id_, *next_dfa_iter_,
                    *push_dfa_iter_, *pop_dfa_iter_, cr_id_, nl_id_,
                    (rules_.features()[dfa_] & *feature_bit::bol) != 0);

                node_ptr_vector_.push_back(std::make_unique<selection_node>
                    (root_, rhs_));
                root_ = node_ptr_vector_.back().get();

                ++regex_iter_;
                ++id_iter_;
                ++user_id_iter_;
                ++next_dfa_iter_;
                ++push_dfa_iter_;
                ++pop_dfa_iter_;
            }

            return root_;
        }

    protected:
        using compressed = std::integral_constant<bool, sm_traits::compressed>;
        using equivset = detail::basic_equivset<id_type>;
        using equivset_list = std::list<std::unique_ptr<equivset>>;
        using equivset_ptr = std::unique_ptr<equivset>;
        using sm_char_type = typename sm_traits::char_type;
        using charset = detail::basic_charset<sm_char_type, id_type>;
        using charset_ptr = std::unique_ptr<charset>;
        using charset_list = std::list<std::unique_ptr<charset>>;
        using internals = detail::basic_internals<id_type>;
        using id_type_set = typename std::set<id_type>;
        using id_type_vector = typename internals::id_type_vector;
        using id_vector_vector = typename rules::id_vector_vector;
        using index_set = typename charset::index_set;
        using index_set_vector = std::vector<index_set>;
        using is_dfa = std::integral_constant<bool, sm_traits::is_dfa>;
        using lookup = std::integral_constant<bool, sm_traits::lookup>;
        using node_set = std::set<observer_ptr<const node>>;
        using node_set_vector = std::vector<std::unique_ptr<node_set>>;
        using node_vector = typename node::node_vector;
        using node_vector_vector = std::vector<std::unique_ptr<node_vector>>;
        using selection_node = typename parser::selection_node;
        using size_t_vector = typename std::vector<std::size_t>;
        using std_string_vector_vector =
            typename rules::std_string_vector_vector;
        using string_token = typename parser::string_token;

        static void build_dfa(const charset_map& charset_map_,
            const observer_ptr<node> root_, internals& internals_, sm& sm_,
            const id_type dfa_index_, id_type& cr_id_, id_type& nl_id_,
            const std::size_t flags_, std::set<id_type>& used_ids_)
        {
            // partitioned charset list
            charset_list charset_list_;
            // vector mapping token indexes to partitioned token index sets
            index_set_vector set_mapping_;
            auto& dfa_ = internals_._dfa[dfa_index_];
            std::size_t dfa_alphabet_ = 0;
            const node_vector& followpos_ = root_->firstpos();
            node_set_vector seen_sets_;
            node_vector_vector seen_vectors_;
            size_t_vector hash_vector_;
            id_type zero_id_ = sm_traits::npos();
            id_type_set eol_set_;

            set_mapping_.resize(charset_map_.size());
            partition_charsets(charset_map_, charset_list_, is_dfa());
            build_set_mapping(charset_list_, internals_, dfa_index_,
                set_mapping_);

            if (cr_id_ != sm_traits::npos() || nl_id_ != sm_traits::npos())
            {
                if (cr_id_ != sm_traits::npos())
                {
                    cr_id_ = *set_mapping_[cr_id_].begin();
                }

                if (nl_id_ != sm_traits::npos())
                {
                    nl_id_ = *set_mapping_[nl_id_].begin();
                }

                zero_id_ = sm_traits::compressed ?
                    *set_mapping_[charset_map_.find(string_token(0, 0))->
                    second].begin() : sm_traits::npos();
            }

            dfa_alphabet_ = charset_list_.size() + *state_index::transitions +
                (cr_id_ == sm_traits::npos() &&
                    nl_id_ == sm_traits::npos() ? 0 : 1);

            if (dfa_alphabet_ > sm_traits::npos())
            {
                // Overflow
                throw runtime_error("The id_type you have chosen cannot hold "
                    "the dfa alphabet.");
            }

            internals_._dfa_alphabet[dfa_index_] =
                static_cast<id_type>(dfa_alphabet_);
            // 'jam' state
            dfa_.resize(dfa_alphabet_, 0);
            closure(followpos_, seen_sets_, seen_vectors_, hash_vector_,
                static_cast<id_type>(dfa_alphabet_), dfa_, flags_, used_ids_);

            // Loop over states
            for (id_type index_ = 0; index_ < static_cast<id_type>
                (seen_vectors_.size()); ++index_)
            {
                equivset_list equiv_list_;

                // Intersect charsets
                build_equiv_list(*seen_vectors_[index_], set_mapping_,
                    equiv_list_, is_dfa());

                for (auto& equivset_ : equiv_list_)
                {
                    prune_eol_clashes(equivset_->_followpos, cr_id_, nl_id_,
                        set_mapping_);

                    const id_type transition_ =
                        closure(equivset_->_followpos, seen_sets_,
                            seen_vectors_, hash_vector_,
                            static_cast<id_type>(dfa_alphabet_), dfa_, flags_,
                            used_ids_);

                    if (transition_ != sm_traits::npos())
                    {
                        // The end state is set as part of closure(),
                        // so wait until here to check for it.
                        observer_ptr<id_type> ptr_ = &dfa_.front() +
                            ((static_cast<std::size_t>(index_) + 1) *
                                dfa_alphabet_);

                        // Prune abstemious transitions from end states.
                        if (!(*ptr_ && !(*ptr_ & *state_bit::greedy) &&
                            equivset_->_greedy == greedy_repeat::no))
                        {
                            set_transitions(transition_, equivset_.get(), dfa_,
                                ptr_, index_, eol_set_);
                        }
                    }
                }
            }

            fix_clashes(eol_set_, cr_id_, nl_id_, zero_id_, dfa_, dfa_alphabet_,
                compressed());
            append_dfa(charset_list_, internals_, sm_, dfa_index_, lookup());
        }

        // Removing clashes will cause an error about rules that cannot match,
        // unless this has been suppressed (bad idea).
        // Because of this we don't worry about end_states that are part of a
        // followset that have other transitions as we should error out anyway
        // (i.e. we could be suppressing other paths that have nothing to do
        // with the end_states we are interested in).
        static void prune_eol_clashes(node_vector& followpos_,
            const id_type cr_id_, const id_type nl_id_,
            const index_set_vector& set_mapping_)
        {
            auto iter_ = followpos_.begin();
            auto end_ = followpos_.end();

            for (; iter_ != end_; ++iter_)
            {
                auto node_ = *iter_;

                if (!node_->end_state())
                {
                    if (node_->token() == parser::eol_token())
                    {
                        prune_NL(iter_, end_, followpos_, cr_id_, nl_id_,
                            set_mapping_);
                    }
                    else
                    {
                        prune_eol(iter_, end_, followpos_, cr_id_, nl_id_,
                            set_mapping_);
                    }
                }
            }
        }

        static void prune_NL(typename node_vector::iterator& iter_,
            typename node_vector::iterator& end_,
            node_vector& followpos_,
            const id_type cr_id_, const id_type nl_id_,
            const index_set_vector& set_mapping_)
        {
            // Search for NL followed by end state
            auto nl_iter_ = iter_;

            ++nl_iter_;

            while (nl_iter_ != end_)
            {
                node* node_ = *nl_iter_;

                if (node_->end_state())
                {
                    ++nl_iter_;
                    continue;
                }

                const auto& cr_set_ =
                    set_mapping_[node_->token()];

                if (cr_set_.find(cr_id_) != cr_set_.end())
                {
                    const auto& cr_followpos_ =
                        node_->followpos();

                    for (auto cr_node_ : cr_followpos_)
                    {
                        if (cr_node_->end_state())
                            continue;

                        const auto& nl_set_ =
                            set_mapping_[cr_node_->token()];

                        if (nl_set_.find(nl_id_) != nl_set_.end())
                        {
                            const auto& nl_followpos_ =
                                cr_node_->followpos();
                            bool found_ = false;

                            for (auto nl_node_ : nl_followpos_)
                            {
                                found_ = nl_node_->end_state();

                                if (found_)
                                    break;
                            }

                            if (found_)
                            {
                                // This might be suppressing additional paths
                                // but we should be erroring out anyway.
                                nl_iter_ = followpos_.erase(nl_iter_);
                                end_ = followpos_.end();
                                continue;
                            }
                        }
                    }
                }

                if (nl_iter_ == end_)
                    break;

                const auto& nl_set_ =
                    set_mapping_[node_->token()];

                if (nl_set_.find(nl_id_) != nl_set_.end())
                {
                    const auto& nl_followpos_ =
                        node_->followpos();
                    bool found_ = false;

                    for (auto nl_node_ : nl_followpos_)
                    {
                        found_ = nl_node_->end_state();

                        if (found_)
                            break;
                    }

                    if (found_)
                    {
                        // This might be suppressing additional paths
                        // but we should be erroring out anyway.
                        nl_iter_ = followpos_.erase(nl_iter_);
                        end_ = followpos_.end();
                        continue;
                    }
                }

                ++nl_iter_;
            }
        }

        static void prune_eol(typename node_vector::iterator& iter_,
            typename node_vector::iterator& end_,
            node_vector& followpos_,
            const id_type cr_id_, const id_type nl_id_,
            const index_set_vector& set_mapping_)
        {
            auto node_ = *iter_;
            const auto& set_ = set_mapping_[node_->token()];

            if (set_.find(cr_id_) != set_.end())
            {
                const auto& cr_followpos_ = node_->followpos();
                bool found_ = false;

                for (auto cr_node_ : cr_followpos_)
                {
                    if (cr_node_->end_state())
                        continue;

                    const auto& cr_set_ = set_mapping_[cr_node_->token()];

                    if (cr_set_.find(nl_id_) != cr_set_.end())
                    {
                        const auto& nl_followpos_ = cr_node_->followpos();

                        found_ = false;

                        for (auto nl_node_ : nl_followpos_)
                        {
                            found_ = nl_node_->end_state();

                            if (found_)
                                break;
                        }

                        if (found_)
                        {
                            // Search for any EOL tokens
                            auto nl_iter_ = iter_;

                            ++nl_iter_;

                            while (nl_iter_ != end_)
                            {
                                node_ = *nl_iter_;

                                if (!node_->end_state() &&
                                    node_->token() == parser::eol_token())
                                {
                                    nl_iter_ = followpos_.erase(nl_iter_);
                                    end_ = followpos_.end();
                                    continue;
                                }

                                ++nl_iter_;
                            }
                        }
                    }
                }
            }

            if (set_.find(nl_id_) != set_.end())
            {
                const auto& nl_followpos_ = node_->followpos();
                bool found_ = false;

                for (auto nl_node_ : nl_followpos_)
                {
                    found_ = nl_node_->end_state();

                    if (found_)
                        break;
                }

                if (found_)
                {
                    // Search for any EOL tokens
                    auto nl_iter_ = iter_;

                    ++nl_iter_;

                    while (nl_iter_ != end_)
                    {
                        node_ = *nl_iter_;

                        if (!node_->end_state() &&
                            node_->token() == parser::eol_token())
                        {
                            nl_iter_ = followpos_.erase(nl_iter_);
                            end_ = followpos_.end();
                            continue;
                        }

                        ++nl_iter_;
                    }
                }
            }
        }

        static void set_transitions(const id_type transition_,
            equivset* equivset_, typename internals::id_type_vector& dfa_,
            id_type* ptr_, const id_type index_, id_type_set& eol_set_)
        {
            for (id_type i_ : equivset_->_index_vector)
            {
                if (i_ == parser::bol_token())
                {
                    dfa_.front() = transition_;
                }
                else if (i_ == parser::eol_token())
                {
                    ptr_[*state_index::eol] = transition_;
                    eol_set_.insert(index_ + 1);
                }
                else
                {
                    ptr_[i_ + *state_index::transitions] = transition_;
                }
            }
        }

        static void check_zero_len(const rules& rules_,
            const observer_ptr<node> root_)
        {
            if ((rules_.flags() & *regex_flags::match_zero_len) == 0)
            {
                const auto& firstpos_ = root_->firstpos();

                for (observer_ptr<const node> node_ : firstpos_)
                {
                    if (node_->end_state())
                    {
                        std::ostringstream ss_;

                        ss_ << "The following regex can match zero chars: " <<
                            regex_from_idx(node_->unique_id() - 1,
                                rules_.regex_strings()) <<
                            "\n(Use regex_flags::match_zero_len "
                            "to suppress this error.)";
                        throw runtime_error(ss_.str());
                    }
                }
            }
        }

        static void check_suppressed(const rules& rules_,
            const id_type unique_id_, std::set<id_type>& used_ids_)
        {
            if (!(rules_.flags() & *regex_flags::allow_suppressed_rules))
            {
                for (id_type id_ = 0; id_ < unique_id_; ++id_)
                {
                    if (used_ids_.find(id_ + 1) == used_ids_.end())
                    {
                        std::ostringstream ss_;

                        ss_ << "The following regex cannot be matched: " <<
                            regex_from_idx(id_, rules_.regex_strings()) <<
                            "\n(Use regex_flags::allow_suppressed_rules "
                            "to suppress this error.)";
                        throw runtime_error(ss_.str());
                    }
                }
            }
        }

        static std::string regex_from_idx(std::size_t idx_,
            const std_string_vector_vector& regexes)
        {
            for (const auto& regex_vec : regexes)
            {
                if (idx_ >= regex_vec.size())
                    idx_ -= regex_vec.size();
                else
                    return regex_vec[idx_];
            }

            return std::string();
        }

        // Uncompressed
        static void fix_clashes(const id_type_set& eol_set_,
            const id_type cr_id_, const id_type nl_id_,
            const id_type /*zero_id_*/,
            typename internals::id_type_vector& dfa_,
            const std::size_t dfa_alphabet_, const std::false_type&)
        {
            for (const auto& eol_ : eol_set_)
            {
                observer_ptr<id_type> ptr_ = &dfa_.front() +
                    eol_ * dfa_alphabet_;
                const id_type eol_state_ = ptr_[*state_index::eol];
                const id_type cr_state_ =
                    ptr_[cr_id_ + *state_index::transitions];
                const id_type nl_state_ =
                    ptr_[nl_id_ + *state_index::transitions];

                if (cr_state_)
                {
                    ptr_[*state_index::transitions + cr_id_] = 0;
                    ptr_ = &dfa_.front() + eol_state_ * dfa_alphabet_;

                    if (ptr_[*state_index::transitions + cr_id_] == 0)
                    {
                        ptr_[*state_index::transitions + cr_id_] = cr_state_;
                    }
                }

                if (nl_state_)
                {
                    ptr_[*state_index::transitions + nl_id_] = 0;
                    ptr_ = &dfa_.front() + eol_state_ * dfa_alphabet_;

                    if (ptr_[*state_index::transitions + nl_id_] == 0)
                    {
                        ptr_[*state_index::transitions + nl_id_] = nl_state_;
                    }
                }
            }
        }

        // Compressed
        static void fix_clashes(const id_type_set& eol_set_,
            const id_type cr_id_, const id_type nl_id_, const id_type zero_id_,
            typename internals::id_type_vector& dfa_,
            const std::size_t dfa_alphabet_, const std::true_type&)
        {
            std::size_t i_ = 0;

            for (const auto& eol_ : eol_set_)
            {
                observer_ptr<id_type> ptr_ = &dfa_.front() +
                    eol_ * dfa_alphabet_;
                const id_type eol_state_ = ptr_[*state_index::eol];
                id_type cr_state_ = 0;
                id_type nl_state_ = 0;

                for (; i_ < (sm_traits::char_24_bit ? 2 : 1); ++i_)
                {
                    ptr_ = &dfa_.front() +
                        ptr_[*state_index::transitions + zero_id_] *
                        dfa_alphabet_;
                }

                cr_state_ = ptr_[*state_index::transitions + cr_id_];

                if (cr_state_)
                {
                    ptr_ = &dfa_.front() + eol_state_ * dfa_alphabet_;

                    if (ptr_[*state_index::transitions + zero_id_] != 0)
                        continue;

                    ptr_[*state_index::transitions + zero_id_] =
                        static_cast<id_type>(dfa_.size() / dfa_alphabet_);
                    dfa_.resize(dfa_.size() + dfa_alphabet_, 0);

                    for (i_ = 0; i_ < (sm_traits::char_24_bit ? 1 : 0); ++i_)
                    {
                        ptr_ = &dfa_.front() + dfa_.size() - dfa_alphabet_;
                        ptr_[*state_index::transitions + zero_id_] =
                            static_cast<id_type>(dfa_.size() / dfa_alphabet_);
                        dfa_.resize(dfa_.size() + dfa_alphabet_, 0);
                    }

                    ptr_ = &dfa_.front() + dfa_.size() - dfa_alphabet_;
                    ptr_[*state_index::transitions + cr_id_] = cr_state_;
                }

                nl_state_ = ptr_[*state_index::transitions + nl_id_];

                if (nl_state_)
                {
                    ptr_ = &dfa_.front() + eol_state_ * dfa_alphabet_;

                    if (ptr_[*state_index::transitions + zero_id_] != 0)
                        continue;

                    ptr_[*state_index::transitions + zero_id_] =
                        static_cast<id_type>(dfa_.size() / dfa_alphabet_);
                    dfa_.resize(dfa_.size() + dfa_alphabet_, 0);

                    for (i_ = 0; i_ < (sm_traits::char_24_bit ? 1 : 0); ++i_)
                    {
                        ptr_ = &dfa_.front() + dfa_.size() - dfa_alphabet_;
                        ptr_[*state_index::transitions + zero_id_] =
                            static_cast<id_type>(dfa_.size() / dfa_alphabet_);
                        dfa_.resize(dfa_.size() + dfa_alphabet_, 0);
                    }

                    ptr_ = &dfa_.front() + dfa_.size() - dfa_alphabet_;
                    ptr_[*state_index::transitions + nl_id_] = nl_state_;
                }
            }
        }

        // char_state_machine version
        static void append_dfa(const charset_list& charset_list_,
            const internals& internals_, sm& sm_, const id_type dfa_index_,
            const std::false_type&)
        {
            std::size_t size_ = charset_list_.size();
            typename sm::string_token_vector token_vector_;

            token_vector_.reserve(size_);

            for (const auto& charset_ : charset_list_)
            {
                token_vector_.push_back(charset_->_token);
            }

            sm_.append(token_vector_, internals_, dfa_index_);
        }

        // state_machine version
        static void append_dfa(const charset_list&, const internals&, sm&,
            const id_type, const std::true_type&)
        {
            // Nothing to do - will use create() instead
        }

        // char_state_machine version
        static void create(internals&, sm&, const id_type_vector&,
            const std::false_type&)
        {
            // Nothing to do - will use append_dfa() instead
        }

        // state_machine version
        static void create(internals& internals_, sm& sm_,
            const id_type_vector& features_, const std::true_type&)
        {
            for (std::size_t i_ = 0, size_ = internals_._dfa.size();
                i_ < size_; ++i_)
            {
                internals_._features |= features_[i_];
            }

            if (internals_._dfa.size() > 1)
            {
                internals_._features |= *feature_bit::multi_state;
            }

            sm_.data().swap(internals_);
        }

        // NFA version
        static void partition_charsets(const charset_map& map_,
            charset_list& lhs_, const std::false_type&)
        {
            fill_rhs_list(map_, lhs_);
        }

        // DFA version
        static void partition_charsets(const charset_map& map_,
            charset_list& lhs_, const std::true_type&)
        {
            charset_list rhs_;

            fill_rhs_list(map_, rhs_);

            if (!rhs_.empty())
            {
                typename charset_list::iterator iter_;
                typename charset_list::iterator end_;
                charset_ptr overlap_ = std::make_unique<charset>();

                lhs_.push_back(std::move(rhs_.front()));
                rhs_.pop_front();

                while (!rhs_.empty())
                {
                    charset_ptr r_(rhs_.front().release());

                    rhs_.pop_front();
                    iter_ = lhs_.begin();
                    end_ = lhs_.end();

                    while (!r_->empty() && iter_ != end_)
                    {
                        auto l_iter_ = iter_;

                        (*l_iter_)->intersect(*r_.get(), *overlap_.get());

                        if (overlap_->empty())
                        {
                            ++iter_;
                        }
                        else if ((*l_iter_)->empty())
                        {
                            l_iter_->reset(overlap_.release());
                            overlap_ = std::make_unique<charset>();
                            ++iter_;
                        }
                        else if (r_->empty())
                        {
                            r_.reset(overlap_.release());
                            overlap_ = std::make_unique<charset>();
                            break;
                        }
                        else
                        {
                            iter_ = lhs_.insert(++iter_, charset_ptr());
                            iter_->reset(overlap_.release());
                            overlap_ = std::make_unique<charset>();
                            ++iter_;
                            end_ = lhs_.end();
                        }
                    }

                    if (!r_->empty())
                    {
                        lhs_.push_back(std::move(r_));
                    }
                }
            }
        }

        static void fill_rhs_list(const charset_map& map_, charset_list& list_)
        {
            for (const auto& pair_ : map_)
            {
                list_.push_back(std::make_unique<charset>
                    (pair_.first, pair_.second));
            }
        }

        static void build_set_mapping(const charset_list& charset_list_,
            internals& internals_, const id_type dfa_index_,
            index_set_vector& set_mapping_)
        {
            auto iter_ = charset_list_.cbegin();
            auto end_ = charset_list_.cend();

            for (id_type index_ = 0; iter_ != end_; ++iter_, ++index_)
            {
                observer_ptr<const charset> cs_ = iter_->get();

                fill_lookup(cs_->_token, &internals_._lookup[dfa_index_],
                    index_, lookup());

                for (const id_type i_ : cs_->_index_set)
                {
                    set_mapping_[i_].insert(index_);
                }
            }
        }

        // char_state_machine version
        static void fill_lookup(const string_token&,
            observer_ptr<id_type_vector>,
            const id_type, const std::false_type&)
        {
            // Do nothing (lookup not used)
        }

        // state_machine version
        static void fill_lookup(const string_token& charset_,
            observer_ptr<id_type_vector> lookup_, const id_type index_,
            const std::true_type&)
        {
            observer_ptr<id_type> ptr_ = &lookup_->front();

            for (const auto& range_ : charset_._ranges)
            {
                for (typename char_traits::index_type char_ = range_.first;
                    char_ < range_.second; ++char_)
                {
                    // Note char_ must be unsigned
                    ptr_[char_] = index_ +
                        static_cast<id_type>(*state_index::transitions);
                }

                // Note range_.second must be unsigned
                ptr_[range_.second] = index_ +
                    static_cast<id_type>(*state_index::transitions);
            }
        }

        static id_type closure(const node_vector& followpos_,
            node_set_vector& seen_sets_, node_vector_vector& seen_vectors_,
            size_t_vector& hash_vector_, const id_type size_,
            id_type_vector& dfa_, const std::size_t flags_,
            std::set<id_type>& used_ids_)
        {
            bool end_state_ = false;
            id_type id_ = 0;
            id_type user_id_ = sm_traits::npos();
            id_type next_dfa_ = 0;
            id_type push_dfa_ = sm_traits::npos();
            bool pop_dfa_ = false;
            std::size_t hash_ = 0;
            greedy_repeat greedy_ = greedy_repeat::yes;

            if (followpos_.empty()) return sm_traits::npos();

            id_type index_ = 0;
            std::unique_ptr<node_set> set_ptr_ = std::make_unique<node_set>();
            std::unique_ptr<node_vector> vector_ptr_ =
                std::make_unique<node_vector>();

            for (observer_ptr<node> node_ : followpos_)
            {
                closure_ex(node_, end_state_, id_, user_id_, next_dfa_,
                    push_dfa_, pop_dfa_, *set_ptr_, *vector_ptr_, hash_,
                    greedy_, flags_, used_ids_);
            }

            bool found_ = false;
            auto hash_iter_ = hash_vector_.cbegin();
            auto hash_end_ = hash_vector_.cend();
            auto set_iter_ = seen_sets_.cbegin();

            for (; !found_ && hash_iter_ != hash_end_;
                ++hash_iter_, ++set_iter_)
            {
                found_ = *hash_iter_ == hash_ && *(*set_iter_) == *set_ptr_;
                ++index_;
            }

            if (!found_)
            {
                seen_sets_.push_back(std::move(set_ptr_));
                seen_vectors_.push_back(std::move(vector_ptr_));
                hash_vector_.push_back(hash_);
                // State 0 is the jam state...
                index_ = static_cast<id_type>(seen_sets_.size());

                const std::size_t old_size_ = dfa_.size();

                dfa_.resize(old_size_ + size_, 0);

                if (end_state_)
                {
                    dfa_[old_size_] |= *state_bit::end_state;

                    if (greedy_ != greedy_repeat::no)
                        dfa_[old_size_] |= *state_bit::greedy;

                    if (pop_dfa_)
                    {
                        dfa_[old_size_] |= *state_bit::pop_dfa;
                    }

                    dfa_[old_size_ + *state_index::id] = id_;
                    dfa_[old_size_ + *state_index::user_id] = user_id_;
                    dfa_[old_size_ + *state_index::push_dfa] = push_dfa_;
                    dfa_[old_size_ + *state_index::next_dfa] = next_dfa_;
                }
            }

            return index_;
        }

        static void closure_ex(observer_ptr<node> node_, bool& end_state_,
            id_type& id_, id_type& user_id_, id_type& next_dfa_,
            id_type& push_dfa_, bool& pop_dfa_, node_set& set_ptr_,
            node_vector& vector_ptr_, std::size_t& hash_,
            greedy_repeat& greedy_, const std::size_t flags_,
            std::set<id_type>& used_ids_)
        {
            if (node_->end_state())
            {
                if (!end_state_)
                {
                    end_state_ = true;
                    id_ = node_->id();
                    user_id_ = node_->user_id();
                    next_dfa_ = node_->next_dfa();
                    push_dfa_ = node_->push_dfa();
                    pop_dfa_ = node_->pop_dfa();
                    greedy_ = node_->greedy();

                    if (!(flags_ & *regex_flags::allow_suppressed_rules))
                        used_ids_.insert(node_->unique_id());
                }
            }

            if (set_ptr_.insert(node_).second)
            {
                vector_ptr_.push_back(node_);
                hash_ += reinterpret_cast<std::size_t>(node_);
            }
        }

        // NFA version
        static void build_equiv_list(const node_vector& vector_,
            const index_set_vector& set_mapping_, equivset_list& lhs_,
            const std::false_type&)
        {
            fill_rhs_list(vector_, set_mapping_, lhs_);
        }

        // DFA version
        static void build_equiv_list(const node_vector& vector_,
            const index_set_vector& set_mapping_, equivset_list& lhs_,
            const std::true_type&)
        {
            equivset_list rhs_;

            fill_rhs_list(vector_, set_mapping_, rhs_);

            if (!rhs_.empty())
            {
                typename equivset_list::iterator iter_;
                typename equivset_list::iterator end_;
                equivset_ptr overlap_ = std::make_unique<equivset>();

                lhs_.push_back(std::move(rhs_.front()));
                rhs_.pop_front();

                while (!rhs_.empty())
                {
                    equivset_ptr r_(rhs_.front().release());

                    rhs_.pop_front();
                    iter_ = lhs_.begin();
                    end_ = lhs_.end();

                    while (!r_->empty() && iter_ != end_)
                    {
                        auto l_iter_ = iter_;

                        (*l_iter_)->intersect(*r_.get(), *overlap_.get());

                        if (overlap_->empty())
                        {
                            ++iter_;
                        }
                        else if ((*l_iter_)->empty())
                        {
                            l_iter_->reset(overlap_.release());
                            overlap_ = std::make_unique<equivset>();
                            ++iter_;
                        }
                        else if (r_->empty())
                        {
                            r_.reset(overlap_.release());
                            overlap_ = std::make_unique<equivset>();
                            break;
                        }
                        else
                        {
                            iter_ = lhs_.insert(++iter_, equivset_ptr());
                            iter_->reset(overlap_.release());
                            overlap_ = std::make_unique<equivset>();
                            ++iter_;
                            end_ = lhs_.end();
                        }
                    }

                    if (!r_->empty())
                    {
                        lhs_.push_back(std::move(r_));
                    }
                }
            }
        }

        static void fill_rhs_list(const node_vector& vector_,
            const index_set_vector& set_mapping_, equivset_list& list_)
        {
            for (observer_ptr<const node> node_ : vector_)
            {
                if (!node_->end_state())
                {
                    const id_type token_ = node_->token();

                    if (token_ != node::null_token())
                    {
                        if (token_ == parser::bol_token() ||
                            token_ == parser::eol_token())
                        {
                            std::set<id_type> index_set_;

                            index_set_.insert(token_);
                            list_.push_back(std::make_unique<equivset>
                                (index_set_, token_, node_->greedy(),
                                    node_->followpos()));
                        }
                        else
                        {
                            list_.push_back(std::make_unique<equivset>
                                (set_mapping_[token_], token_, node_->greedy(),
                                    node_->followpos()));
                        }
                    }
                }
            }
        }
    };

    using generator = basic_generator<rules, state_machine>;
    using wgenerator = basic_generator<wrules, wstate_machine>;
    using u32generator = basic_generator<u32rules, u32state_machine>;
    using char_generator = basic_generator<rules, char_state_machine>;
    using wchar_generator = basic_generator<wrules, wchar_state_machine>;
    using u32char_generator = basic_generator<u32rules, u32char_state_machine>;
}

#endif
