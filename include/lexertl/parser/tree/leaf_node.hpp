// leaf_node.hpp
// Copyright (c) 2005-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_LEAF_NODE_HPP
#define LEXERTL_LEAF_NODE_HPP

#include "../../enums.hpp" // null_token
#include "node.hpp"

namespace lexertl
{
    namespace detail
    {
        template<typename id_type>
        class basic_leaf_node : public basic_node<id_type>
        {
        public:
            using node = basic_node<id_type>;
            using bool_stack = typename node::bool_stack;
            using const_node_stack = typename node::const_node_stack;
            using node_ptr_vector = typename node::node_ptr_vector;
            using node_stack = typename node::node_stack;
            using node_type = typename node::node_type;
            using node_vector = typename node::node_vector;

            basic_leaf_node(const id_type token_, const bool greedy_) :
                node(token_ == node::null_token()),
                _token(token_),
                _set_greedy(!greedy_),
                _greedy(greedy_)
            {
                if (!node::nullable())
                {
                    node::firstpos().push_back(this);
                    node::lastpos().push_back(this);
                }
            }

            ~basic_leaf_node() override = default;

            void append_followpos
                (const node_vector& followpos_) override
            {
                _followpos.insert(_followpos.end(),
                    followpos_.begin(), followpos_.end());
            }

            node_type what_type() const override
            {
                return node::node_type::LEAF;
            }

            bool traverse(const_node_stack&/*node_stack_*/,
                bool_stack&/*perform_op_stack_*/) const override
            {
                return false;
            }

            id_type token() const override
            {
                return _token;
            }

            bool set_greedy() const override
            {
                return _set_greedy;
            }

            void greedy(const bool greedy_) override
            {
                if (!_set_greedy)
                {
                    _greedy = greedy_;
                    _set_greedy = true;
                }
            }

            bool greedy() const override
            {
                return _greedy;
            }

            const node_vector& followpos() const override
            {
                return _followpos;
            }

            node_vector& followpos() override
            {
                return _followpos;
            }

        private:
            id_type _token;
            bool _set_greedy;
            bool _greedy;
            node_vector _followpos;

            void copy_node(node_ptr_vector& node_ptr_vector_,
                node_stack& new_node_stack_, bool_stack&/*perform_op_stack_*/,
                bool&/*down_*/) const override
            {
                node_ptr_vector_.push_back(std::make_unique<basic_leaf_node>
                    (_token, _greedy));
                new_node_stack_.push(node_ptr_vector_.back().get());
            }
        };
    }
}

#endif
