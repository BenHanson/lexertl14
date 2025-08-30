// iterator.hpp
// Copyright (c) 2015-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_ITERATOR_HPP
#define LEXERTL_ITERATOR_HPP

#include "lookup.hpp"
#include "match_results.hpp"
#include "state_machine.hpp"

#include <iterator>
#include <string>

namespace lexertl
{
    template<typename iter, typename sm_type, typename results>
    class iterator
    {
    public:
        using data_iterator = iter;
        using id_type = typename results::id_type;
        using value_type = results;
        using difference_type = ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::forward_iterator_tag;

        iterator() = default;

        iterator(const iter& start_, const iter& end_, const sm_type& sm_,
            const bool bol_ = true, const id_type state_ = 0) :
            _results(start_, end_, bol_, state_),
            _sm(&sm_)
        {
            lookup();
        }

        iterator& operator ++()
        {
            lookup();
            return *this;
        }

        iterator operator ++(int)
        {
            iterator iter_ = *this;

            lookup();
            return iter_;
        }

        const value_type& operator *() const
        {
            return _results;
        }

        const value_type* operator ->() const
        {
            return &_results;
        }

        friend bool operator ==(const iterator& lhs_, const iterator& rhs_)
        {
            return lhs_._sm == rhs_._sm && (lhs_._sm == nullptr ?
                true :
                lhs_._results == rhs_._results);
        }

        friend bool operator !=(const iterator& lhs_, const iterator& rhs_)
        {
            return !(lhs_ == rhs_);
        }

        const sm_type& sm() const
        {
            return *_sm;
        }

    private:
        value_type _results;
        const sm_type* _sm = nullptr;

        void lookup()
        {
            lexertl::lookup(*_sm, _results);

            if (_results.first == _results.eoi)
            {
                _sm = nullptr;
            }
        }
    };

    using siterator =
        iterator<std::string::const_iterator, state_machine, smatch>;
    using citerator = iterator<const char*, state_machine, cmatch>;
    using wsiterator =
        iterator<std::wstring::const_iterator, wstate_machine, wsmatch>;
    using wciterator = iterator<const wchar_t*, wstate_machine, wcmatch>;
    using u32siterator = iterator<std::u32string::const_iterator,
        u32state_machine, u32smatch>;
    using u32citerator = iterator<const char32_t*, u32state_machine, u32cmatch>;

    using sriterator =
        iterator<std::string::const_iterator, state_machine, srmatch>;
    using criterator = iterator<const char*, state_machine, crmatch>;
    using wsriterator =
        iterator<std::wstring::const_iterator, wstate_machine, wsrmatch>;
    using wcriterator =
        iterator<const wchar_t*, wstate_machine, wcrmatch>;
    using u32sriterator = iterator<std::u32string::const_iterator,
        u32state_machine, u32srmatch>;
    using u32criterator = iterator<const char32_t*, u32state_machine,
        u32crmatch>;
}

#endif
