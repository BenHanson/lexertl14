// debug.hpp
// Copyright (c) 2005-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_DEBUG_HPP
#define LEXERTL_DEBUG_HPP

#include <algorithm>
#include <map>
#include <ostream>
#include "rules.hpp"
#include "sm_to_csm.hpp"
#include "state_machine.hpp"
#include "stream_num.hpp"
#include "string_token.hpp"
#include <vector>

namespace lexertl
{
    template<typename sm, typename char_type, typename id_type = uint16_t,
        bool is_dfa = true>
    class basic_debug
    {
    public:
        using char_state_machine =
            basic_char_state_machine<char_type, id_type, is_dfa>;
        using ostream = std::basic_ostream<char_type>;
        using rules = basic_rules<char_type, char_type, id_type>;
        using string = std::basic_string<char_type>;

        static void dump(rules& rules_, ostream& stream_)
        {
            // Take by value: we will modify the macros
            macro_map macros_ = rules_.macros();
            macro_size_map macro_sizes_;
            // Take by value: we will modify the regexes
            token_vector_vector_vector all_regexes_ = rules_.regexes();
            const id_vector_vector& all_ids_ = rules_.ids();
            const id_vector_vector& next_dfas_ = rules_.next_dfas();
            const id_vector_vector& pushes_ = rules_.pushes();
            const bool_vector_vector& pops_ = rules_.pops();

            output_states(rules_, stream_);
            preprocess_macros(macros_, macro_sizes_);
            insert_macros(all_regexes_, macro_sizes_, macros_);
            macro_sizes_.clear();
            postprocess_macros(macros_);

            perc_perc(stream_);
            stream_ << std::endl;

            for (const auto& pair_ : macros_)
            {
                stream_ << pair_.first << static_cast<char_type>(' ');

                for (const auto& token_ : pair_.second)
                {
                    dump_token(token_, stream_);
                }

                stream_ << std::endl;
            }

            perc_perc(stream_);
            stream_ << std::endl;

            for (std::size_t state_ = 0, states_ = all_ids_.size();
                state_ < states_; ++state_)
            {
                auto dfa_iter_ = next_dfas_[state_].cbegin();
                auto push_iter_ = pushes_[state_].cbegin();
                auto pop_iter_ = pops_[state_].cbegin();
                auto ids_iter_ = all_ids_[state_].cbegin();

                for (const auto& regex_ : all_regexes_[state_])
                {
                    const id_type dfa_ = *dfa_iter_++;
                    const id_type push_state_ = *push_iter_++;
                    const bool pop_ = *pop_iter_++;
                    auto regexes_iter_ = regex_.cbegin();
                    auto regexes_end_ = regex_.cend();
                    const bool initial_ = state_ == dfa_ && state_ == 0;

                    if (push_state_ != rules_.npos() || pop_ ||
                        !initial_)
                    {
                        stream_ << static_cast<char_type>('<') <<
                            rules_.state(static_cast<id_type>(state_)) <<
                            static_cast<char_type>('>');
                    }

                    for (; regexes_iter_ != regexes_end_; ++regexes_iter_)
                    {
                        dump_token(*regexes_iter_, stream_);
                    }

                    stream_ << static_cast<char_type>(' ') <<
                        static_cast<char_type>('{');

                    if (push_state_ != rules_.npos())
                    {
                        if (push_state_ && push_state_ != dfa_)
                        {
                            Begin(stream_);
                            stream_ << rules_.state(push_state_);
                            stream_ << static_cast<char_type>(')') <<
                                static_cast<char_type>(';');
                        }

                        yy_push_state(stream_);
                        stream_ << rules_.state(dfa_);
                        stream_ << static_cast<char_type>(')') <<
                            static_cast<char_type>(';');
                    }
                    else if (pop_)
                    {
                        yy_pop_state(stream_);
                    }
                    else if (state_ != dfa_)
                    {
                        Begin(stream_);
                        stream_ << rules_.state(dfa_);
                        stream_ << static_cast<char_type>(')') <<
                            static_cast<char_type>(';');
                    }

                    if (*ids_iter_)
                    {
                        ret(stream_);
                        stream_num<std::size_t>(*ids_iter_, stream_);
                        stream_ << static_cast<char_type>(';');
                    }

                    stream_ << static_cast<char_type>(' ') <<
                        static_cast<char_type>('}');
                    stream_ << std::endl;
                    ++ids_iter_;
                }
            }

            perc_perc(stream_);
            stream_ << std::endl;
        }

        static void dump(const sm& sm_, rules& rules_, ostream& stream_)
        {
            char_state_machine csm_;

            sm_to_csm(sm_, csm_);
            dump(csm_, rules_, stream_);
        }

        static void dump(const sm& sm_, ostream& stream_)
        {
            char_state_machine csm_;

            sm_to_csm(sm_, csm_);
            dump(csm_, stream_);
        }

        static void dump(const char_state_machine& csm_, rules& rules_,
            ostream& stream_)
        {
            for (std::size_t dfa_ = 0, dfas_ = csm_.size();
                dfa_ < dfas_; ++dfa_)
            {
                lexer_state(stream_);
                stream_ << rules_.state(dfa_) << std::endl << std::endl;

                dump_ex(csm_._sm_vector[dfa_], stream_);
            }
        }

        static void dump(const char_state_machine& csm_, ostream& stream_)
        {
            for (std::size_t dfa_ = 0, dfas_ = csm_.size();
                dfa_ < dfas_; ++dfa_)
            {
                lexer_state(stream_);
                 stream_num<std::size_t>(dfa_, stream_);
                stream_ << std::endl << std::endl;

                dump_ex(csm_._sm_vector[dfa_], stream_);
            }
        }

    protected:
        using bool_vector_vector = typename rules::bool_vector_vector;
        using dfa_state = typename char_state_machine::state;
        using id_type_string_token_pair =
            typename dfa_state::id_type_string_token_pair;
        using id_vector_vector = typename rules::id_vector_vector;
        using macro_map = typename rules::macro_map;
        using macro_size_map =
            std::multimap<std::size_t, string, std::greater<>>;
        using string_token = typename dfa_state::string_token;
        using stringstream = std::basic_stringstream<char_type>;
        using token = detail::basic_re_token<char_type, char_type>;
        using token_vector_vector_vector =
            typename rules::token_vector_vector_vector;

        static void output_states(const rules& rules_, ostream& stream_)
        {
            token_vector_vector_vector all_regexes_ = rules_.regexes();

            if (all_regexes_.size() > 1)
            {
                stream_ << static_cast<char_type>('%') <<
                    static_cast<char_type>('x');

                for (std::size_t i_ = 1, size_ = all_regexes_.size();
                    i_ < size_; ++i_)
                {
                    stream_ << static_cast<char_type>(' ') <<
                        rules_.state(static_cast<id_type>(i_));
                }

                stream_ << std::endl;
            }
        }

        static void preprocess_macros(macro_map& macros_,
            macro_size_map& macro_sizes_)
        {
            for (auto& pair_ : macros_)
            {
                if (pair_.second.size() > 3)
                {
                    pair_.second.front()._type = detail::token_type::OPENPAREN;
                    pair_.second.front()._str.
                        insert(string_token(static_cast<char_type>('('),
                            static_cast <char_type>('(')));
                    pair_.second.back()._type = detail::token_type::CLOSEPAREN;
                    pair_.second.back()._str.
                        insert(string_token(static_cast<char_type>(')'),
                            static_cast <char_type>(')')));
                }
                else
                {
                    pair_.second.erase(pair_.second.begin());
                    pair_.second.pop_back();
                }

                macro_sizes_.emplace(pair_.second.size(), pair_.first);
            }
        }

        static void insert_macros(token_vector_vector_vector& all_regexes_,
            const macro_size_map& macro_sizes_, const macro_map& macros_)
        {
            for (auto& regexes_ : all_regexes_)
            {
                for (auto& regex_ : regexes_)
                {
                    for (const auto& macro_ : macro_sizes_)
                    {
                        auto macro_iter_ = macros_.find(macro_.second);

                        if (macro_iter_ != macros_.cend())
                        {
                            auto iter_ = std::search(regex_.begin(),
                                regex_.end(), macro_iter_->second.begin(),
                                macro_iter_->second.end());

                            while (iter_ != regex_.end())
                            {
                                token token_(detail::token_type::MACRO);

                                token_._extra = static_cast<char_type>('{') +
                                    macro_.second +
                                    static_cast<char_type>('}');
                                *iter_ = token_;

                                if (macro_.first > 1)
                                {
                                    ++iter_;
                                    regex_.erase(iter_,
                                        iter_ + (macro_.first - 1));
                                }

                                iter_ = std::search(regex_.begin(),
                                    regex_.end(), macro_iter_->second.begin(),
                                    macro_iter_->second.end());
                            }
                        }
                    }
                }
            }
        }

        static void postprocess_macros(macro_map& macros_)
        {
            for (auto& pair_ : macros_)
            {
                if (pair_.second.size() > 1)
                {
                    pair_.second.erase(pair_.second.begin());
                    pair_.second.pop_back();
                }
            }
        }

        static void dump_ex(const typename char_state_machine::dfa& dfa_,
            ostream& stream_)
        {
            const std::size_t states_ = dfa_._states.size();
            const id_type bol_index_ = dfa_._bol_index;

            for (std::size_t i_ = 0; i_ < states_; ++i_)
            {
                const dfa_state& state_ = dfa_._states[i_];

                state(stream_);
                stream_num(i_, stream_);
                stream_ << std::endl;

                if (state_._end_state)
                {
                    end_state(stream_);

                    if (state_._push_pop_dfa ==
                        dfa_state::push_pop_dfa::push_dfa)
                    {
                        push(stream_);
                        stream_num(state_._push_dfa, stream_);
                    }
                    else if (state_._push_pop_dfa ==
                        dfa_state::push_pop_dfa::pop_dfa)
                    {
                        pop(stream_);
                    }

                    id(stream_);
                    stream_num(static_cast<std::size_t>(state_._id), stream_);
                    user_id(stream_);
                    stream_num(static_cast<std::size_t>(state_._user_id),
                        stream_);
                    dfa(stream_);
                    stream_num(static_cast<std::size_t>(state_._next_dfa),
                        stream_);
                    stream_ << std::endl;
                }

                if (i_ == 0 && bol_index_ != char_state_machine::npos())
                {
                    bol(stream_);
                    stream_num(static_cast<std::size_t>(bol_index_), stream_);
                    stream_ << std::endl;
                }

                if (state_._eol_index != char_state_machine::npos())
                {
                    eol(stream_);
                    stream_num(static_cast<std::size_t>(state_._eol_index),
                        stream_);
                    stream_ << std::endl;
                }

                for (const auto& tran_ : state_._transitions)
                    dump_transition(tran_, stream_);

                stream_ << std::endl;
            }
        }

        static void dump_transition(const id_type_string_token_pair& tran_,
            ostream& stream_)
        {
            indent(stream_);
            dump_charset(tran_.second, stream_);
            goes_to(stream_);
            stream_num(static_cast<std::size_t>(tran_.first), stream_);
            stream_ << std::endl;
        }

        static void dump_token(const token& token_, ostream& stream_)
        {
            switch (token_._type)
            {
            case lexertl::detail::token_type::OR:
                stream_ << static_cast<char_type>('|');
                break;
            case lexertl::detail::token_type::CHARSET:
                dump_charset(token_._str, stream_);
                break;
            case lexertl::detail::token_type::BOL:
                stream_ << static_cast<char_type>('^');
                break;
            case lexertl::detail::token_type::EOL:
                stream_ << static_cast<char_type>('$');
                break;
            case lexertl::detail::token_type::MACRO:
                stream_ << token_._extra;
                break;
            case lexertl::detail::token_type::OPENPAREN:
                stream_ << static_cast<char_type>('(');
                break;
            case lexertl::detail::token_type::CLOSEPAREN:
                stream_ << static_cast<char_type>(')');
                break;
            case lexertl::detail::token_type::OPT:
                stream_ << static_cast<char_type>('?');
                break;
            case lexertl::detail::token_type::AOPT:
                stream_ << static_cast<char_type>('?') <<
                    static_cast<char_type>('?');
                break;
            case lexertl::detail::token_type::ZEROORMORE:
                stream_ << static_cast<char_type>('*');
                break;
            case lexertl::detail::token_type::AZEROORMORE:
                stream_ << static_cast<char_type>('*') <<
                    static_cast<char_type>('?');
                break;
            case lexertl::detail::token_type::ONEORMORE:
                stream_ << static_cast<char_type>('+');
                break;
            case lexertl::detail::token_type::AONEORMORE:
                stream_ << static_cast<char_type>('+') <<
                    static_cast<char_type>('?');
                break;
            case lexertl::detail::token_type::REPEATN:
                stream_ << static_cast<char_type>('{') <<
                    token_._extra <<
                    static_cast<char_type>('}');
                break;
            case lexertl::detail::token_type::AREPEATN:
                stream_ << static_cast<char_type>('{') <<
                    token_._extra <<
                    static_cast<char_type>('}') <<
                    static_cast<char_type>('?');
                break;
            default:
                break;
            }
        }

        static void dump_charset(const string_token& in_token_,
            ostream& stream_)
        {
            string_token token_ = in_token_;
            bool negated_ = false;

            if (token_._ranges.size() > 1 ||
                token_._ranges.front().first != token_._ranges.front().second)
            {
                open_bracket(stream_);
            }
            else
            {
                const char_type c_ = token_._ranges.front().first;

                switch (c_)
                {
                case static_cast<char_type>('|'):
                case static_cast<char_type>('('):
                case static_cast<char_type>(')'):
                case static_cast<char_type>('?'):
                case static_cast<char_type>('*'):
                case static_cast<char_type>('+'):
                case static_cast<char_type>('{'):
                case static_cast<char_type>('}'):
                case static_cast<char_type>('['):
                case static_cast<char_type>(']'):
                case static_cast<char_type>('.'):
                case static_cast<char_type>('/'):
                case static_cast<char_type>('\\'):
                case static_cast<char_type>('"'):
                    stream_ << static_cast<char_type>('\\');
                    break;
                default:
                    break;
                }
            }

            if (!token_.any() && token_.negatable())
            {
                token_.negate();
                negated(stream_);
                negated_ = true;
            }

            string chars_;

            for (const auto& range_ : token_._ranges)
            {
                if (range_.first == static_cast<char_type>('-') ||
                    range_.first == static_cast<char_type>('^') ||
                    (range_.first == static_cast<char_type>(']') &&
                    range_.first != range_.second))
                {
                    stream_ << static_cast<char_type>('\\');
                }

                chars_ = string_token::escape_char
                (range_.first);

                if (range_.first != range_.second)
                {
                    if (range_.first + 1 < range_.second)
                    {
                        chars_ += static_cast<char_type>('-');
                    }

                    if (range_.second == static_cast<char_type>('-') ||
                        range_.second == static_cast<char_type>('^') ||
                        range_.second == static_cast<char_type>(']'))
                    {
                        stream_ << static_cast<char_type>('\\');
                    }

                    chars_ += string_token::escape_char(range_.second);
                }

                stream_ << chars_;
            }

            if (token_._ranges.size() > 1 || negated_ ||
                token_._ranges.front().first != token_._ranges.front().second)
            {
                close_bracket(stream_);
            }
        }

        static void perc_perc(std::ostream& stream_)
        {
            stream_ << "%%";
        }

        static void perc_perc(std::wostream& stream_)
        {
            stream_ << L"%%";
        }

        static void perc_perc(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"%%";
        }

        static void indent(std::ostream& stream_)
        {
            stream_ << "  ";
        }

        static void indent(std::wostream& stream_)
        {
            stream_ << L"  ";
        }

        static void indent(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"  ";
        }

        static void yy_push_state(std::ostream& stream_)
        {
            stream_ << " yy_push_state(";
        }

        static void yy_push_state(std::wostream& stream_)
        {
            stream_ << L" yy_push_state(";
        }

        static void yy_push_state(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U" yy_push_state(";
        }

        static void yy_pop_state(std::ostream& stream_)
        {
            stream_ << " yy_pop_state(); ";
        }

        static void yy_pop_state(std::wostream& stream_)
        {
            stream_ << L" yy_pop_state(); ";
        }

        static void yy_pop_state(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U" yy_pop_state(); ";
        }

        static void Begin(std::ostream& stream_)
        {
            stream_ << " BEGIN(";
        }

        static void Begin(std::wostream& stream_)
        {
            stream_ << L" BEGIN(";
        }

        static void Begin(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U" BEGIN(";
        }

        static void ret(std::ostream& stream_)
        {
            stream_ << " return ";
        }

        static void ret(std::wostream& stream_)
        {
            stream_ << L" return ";
        }

        static void ret(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U" return ";
        }

        static void goes_to(std::ostream& stream_)
        {
            stream_ << " -> ";
        }

        static void goes_to(std::wostream& stream_)
        {
            stream_ << L" -> ";
        }

        static void goes_to(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U" -> ";
        }

        static void lexer_state(std::ostream& stream_)
        {
            stream_ << "Lexer state: ";
        }

        static void lexer_state(std::wostream& stream_)
        {
            stream_ << L"Lexer state: ";
        }

        static void lexer_state(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"Lexer state: ";
        }

        static void state(std::ostream& stream_)
        {
            stream_ << "State: ";
        }

        static void state(std::wostream& stream_)
        {
            stream_ << L"State: ";
        }

        static void state(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"State: ";
        }

        static void bol(std::ostream& stream_)
        {
            stream_ << "  BOL -> ";
        }

        static void bol(std::wostream& stream_)
        {
            stream_ << L"  BOL -> ";
        }

        static void bol(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"  BOL -> ";
        }

        static void eol(std::ostream& stream_)
        {
            stream_ << "  EOL -> ";
        }

        static void eol(std::wostream& stream_)
        {
            stream_ << L"  EOL -> ";
        }

        static void eol(std::basic_ostream<char32_t> & stream_)
        {
            stream_ << U"  EOL -> ";
        }

        static void end_state(std::ostream& stream_)
        {
            stream_ << "  END STATE";
        }

        static void end_state(std::wostream& stream_)
        {
            stream_ << L"  END STATE";
        }

        static void end_state(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"  END STATE";
        }

        static void id(std::ostream& stream_)
        {
            stream_ << ", Id = ";
        }

        static void id(std::wostream& stream_)
        {
            stream_ << L", Id = ";
        }

        static void id(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U", Id = ";
        }

        static void push(std::ostream& stream_)
        {
            stream_ << ", PUSH ";
        }

        static void push(std::wostream& stream_)
        {
            stream_ << L", PUSH ";
        }

        static void push(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U", PUSH ";
        }

        static void pop(std::ostream& stream_)
        {
            stream_ << ", POP";
        }

        static void pop(std::wostream& stream_)
        {
            stream_ << L", POP";
        }

        static void pop(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U", POP";
        }

        static void user_id(std::ostream& stream_)
        {
            stream_ << ", User Id = ";
        }

        static void user_id(std::wostream& stream_)
        {
            stream_ << L", User Id = ";
        }

        static void user_id(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U", User Id = ";
        }

        static void open_bracket(std::ostream& stream_)
        {
            stream_ << "[";
        }

        static void open_bracket(std::wostream& stream_)
        {
            stream_ << L"[";
        }

        static void open_bracket(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"[";
        }

        static void negated(std::ostream& stream_)
        {
            stream_ << "^";
        }

        static void negated(std::wostream& stream_)
        {
            stream_ << L"^";
        }

        static void negated(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"^";
        }

        static void close_bracket(std::ostream& stream_)
        {
            stream_ << "]";
        }

        static void close_bracket(std::wostream& stream_)
        {
            stream_ << L"]";
        }

        static void close_bracket(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"]";
        }

        static void dfa(std::ostream& stream_)
        {
            stream_ << ", dfa = ";
        }

        static void dfa(std::wostream& stream_)
        {
            stream_ << L", dfa = ";
        }

        static void dfa(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U", dfa = ";
        }
    };

    using debug = basic_debug<state_machine, char>;
    using wdebug = basic_debug<wstate_machine, wchar_t>;
    using u32debug = basic_debug<u32state_machine, char32_t>;
}

#endif
