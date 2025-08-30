// equivset.hpp
// Copyright (c) 2005-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_EQUIVSET_HPP
#define LEXERTL_EQUIVSET_HPP

#include "../enums.hpp"
#include "../parser/tree/node.hpp"
#include "../observer_ptr.hpp"

#include <algorithm>
#include <iterator>
#include <set>
#include <vector>

namespace lexertl
{
    namespace detail
    {
        template<typename id_type>
        struct basic_equivset
        {
            using index_set = std::set<id_type>;
            using index_vector = std::vector<id_type>;
            using node = basic_node<id_type>;
            using node_vector = std::vector<observer_ptr<node>>;

            index_vector _index_vector;
            id_type _id = 0;
            greedy_repeat _greedy = greedy_repeat::yes;
            node_vector _followpos;

            basic_equivset() = default;

            basic_equivset(const index_set& index_set_, const id_type id_,
                const greedy_repeat greedy_, const node_vector& followpos_) :
                _index_vector(index_set_.begin(), index_set_.end()),
                _id(id_),
                _greedy(greedy_),
                _followpos(followpos_)
            {
            }

            bool empty() const
            {
                return _index_vector.empty() && _followpos.empty();
            }

            void intersect(basic_equivset& rhs_, basic_equivset& overlap_)
            {
                intersect_indexes(rhs_._index_vector, overlap_._index_vector);

                if (!overlap_._index_vector.empty())
                {
                    // Note that the LHS takes priority in order to
                    // respect rule ordering priority in the lex spec.
                    overlap_._id = _id;
                    process_greedy(rhs_, overlap_);
                    overlap_._followpos = _followpos;

                    auto overlap_begin_ = overlap_._followpos.cbegin();
                    auto overlap_end_ = overlap_._followpos.cend();

                    for (observer_ptr<node> node_ : rhs_._followpos)
                    {
                        if (std::find(overlap_begin_, overlap_end_, node_) ==
                            overlap_end_)
                        {
                            overlap_._followpos.push_back(node_);
                            overlap_begin_ = overlap_._followpos.begin();
                            overlap_end_ = overlap_._followpos.end();
                        }
                    }

                    if (_index_vector.empty())
                    {
                        _followpos.clear();
                    }

                    if (rhs_._index_vector.empty())
                    {
                        rhs_._followpos.clear();
                    }
                }
            }

        private:
            void process_greedy(basic_equivset& rhs_,
                basic_equivset& overlap_) const
            {
                if (_greedy == greedy_repeat::no)
                {
                    if (rhs_._greedy == greedy_repeat::hard)
                        overlap_._greedy = rhs_._greedy;
                    else
                        overlap_._greedy = _greedy;
                }
                else
                    overlap_._greedy = _greedy;
            }

            void intersect_indexes(index_vector& rhs_, index_vector& overlap_)
            {
                std::set_intersection(_index_vector.begin(),
                    _index_vector.end(), rhs_.begin(), rhs_.end(),
                    std::back_inserter(overlap_));

                if (!overlap_.empty())
                {
                    remove(overlap_, _index_vector);
                    remove(overlap_, rhs_);
                }
            }

            void remove(const index_vector& source_, index_vector& dest_) const
            {
                auto inter_ = source_.cbegin();
                auto inter_end_ = source_.cend();
                auto reader_ = std::find(dest_.begin(), dest_.end(), *inter_);
                auto writer_ = reader_;
                auto dest_end_ = dest_.end();

                while (writer_ != dest_end_ && inter_ != inter_end_)
                {
                    if (*reader_ == *inter_)
                    {
                        ++inter_;
                        ++reader_;
                    }
                    else
                    {
                        *writer_++ = *reader_++;
                    }
                }

                while (reader_ != dest_end_)
                {
                    *writer_++ = *reader_++;
                }

                dest_.resize(dest_.size() - source_.size());
            }
        };
    }
}

#endif
