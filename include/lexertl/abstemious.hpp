// abstemious.hpp
// Copyright (c) 2005-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_ABSTEMIOUS_HPP
#define LEXERTL_ABSTEMIOUS_HPP

#include "parser/tokeniser/re_tokeniser.hpp"

namespace lexertl
{
	template<typename rules_char_type, typename char_type>
	class abstemious
	{
	public:
		using token = detail::basic_re_token<rules_char_type, char_type>;
		using token_vector = std::vector<token>;
		using index_vector = std::vector<std::size_t>;

		static void prune(token_vector& tokens_, index_vector& indexes_)
		{
			while (!indexes_.empty())
			{
				auto start_ = indexes_.back();
				const auto idx_ = start_;

				if (is_end(tokens_, idx_))
				{
					auto& op_ = tokens_[idx_];

					switch (op_._type)
					{
					case lexertl::detail::token_type::AOPT:
					case lexertl::detail::token_type::AZEROORMORE:
						remove_sequence(tokens_, start_, idx_);
						break;
					case lexertl::detail::token_type::AONEORMORE:
						tokens_.erase(tokens_.begin() + idx_);
						break;
					case lexertl::detail::token_type::AREPEATN:
						op_._type = lexertl::detail::token_type::REPEATN;
						op_._extra = op_._extra.substr(0, op_._extra.find(','));
						break;
					default:
						break;
					}
				}

				indexes_.pop_back();

				while (!indexes_.empty())
				{
					const auto back_ = indexes_.back();

					if (back_ >= start_ && back_ <= idx_)
						indexes_.pop_back();
					else
						break;
				}
			}
		}

	protected:
		static bool is_end(const token_vector& tokens_, const std::size_t start_)
		{
			bool ret_ = true;

			for (auto idx_ = start_ + 1, size_ = tokens_.size(); idx_ < size_;)
			{
				const auto& token_ = tokens_[idx_];

				switch (token_._type)
				{
				case lexertl::detail::token_type::OR:
					idx_ = end_block(tokens_, idx_ + 1);
					break;
				case lexertl::detail::token_type::CLOSEPAREN:
					++idx_;
					break;
				case lexertl::detail::token_type::END:
					return true;
				default:
					return false;
				}
			}

			return ret_;
		}

		static std::size_t end_block(const token_vector& tokens_,
			const std::size_t start_)
		{
			auto idx_ = start_ + 1;
			std::size_t parens_ = 0;

			for (auto size_ = tokens_.size(); idx_ < size_; ++idx_)
			{
				const auto& token_ = tokens_[idx_];

				switch (token_._type)
				{
				case lexertl::detail::token_type::OR:
					if (parens_ == 0)
						return idx_;

					break;
				case lexertl::detail::token_type::OPENPAREN:
					++parens_;
					break;
				case lexertl::detail::token_type::CLOSEPAREN:
					if (parens_ == 0)
						return idx_;

					--parens_;
					break;
				case lexertl::detail::token_type::END:
					return idx_;
				default:
					break;
				}
			}

			return idx_;
		}

		static void remove_sequence(token_vector& tokens_, std::size_t& start_,
			const std::size_t idx_)
		{
			auto iter_ = tokens_.begin() + idx_ - 1;

			if (iter_->_type ==
				lexertl::detail::token_type::CLOSEPAREN)
			{
				// Find beginning of block
				std::size_t parens_ = 1;

				do
				{
					--iter_;

					switch (iter_->_type)
					{
					case lexertl::detail::token_type::OPENPAREN:
						--parens_;
						break;
					case lexertl::detail::token_type::CLOSEPAREN:
						++parens_;
						break;
					default:
						break;
					}
				} while (parens_);
			}

			start_ = iter_ - tokens_.begin();
			iter_ = tokens_.erase(iter_, tokens_.begin() + idx_ + 1);

			if (iter_->_type == lexertl::detail::token_type::OR)
				tokens_.erase(iter_);
			else if (iter_->_type != lexertl::detail::token_type::BEGIN &&
				(iter_ - 1)->_type == lexertl::detail::token_type::OR)
			{
				tokens_.erase(--iter_);
			}
		}
	};
}

#endif
