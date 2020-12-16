#include "../include/lexertl/generator.hpp"
#include <iomanip>
#include <fstream>
#include "../include/lexertl/iterator.hpp"
#include "../include/lexertl/lookup.hpp"
#include "../include/lexertl/memory_file.hpp"

void lex_unicode_data(lexertl::memory_file& mf_, std::ostream& os_,
	std::ostream& ucs_)
{
	lexertl::rules rules_;
	lexertl::state_machine state_machine_;
	const char* start_ = mf_.data();
	const char* end_ = start_ + mf_.size();
	lexertl::cmatch results_(start_, end_);
	enum { eNumber = 1, eName };

	std::size_t num_ = 0;
	using string_token = lexertl::basic_string_token<std::size_t>;
	using map = std::map<std::string, string_token>;
	map map_;

	rules_.push_state("LONG_NAME");
	rules_.push_state("SHORT_NAME");
	rules_.push_state("FINISH");

	rules_.push("INITIAL", "^[A-F0-9]+", eNumber, "LONG_NAME");
	rules_.push("LONG_NAME", ";[^;]+;", lexertl::rules::skip(), "SHORT_NAME");
	rules_.push("SHORT_NAME", "[A-Z][a-z]?", eName, "FINISH");
	rules_.push("FINISH", ".*\n", lexertl::rules::skip(), "INITIAL");
	lexertl::generator::build(rules_, state_machine_);

	do
	{
		lexertl::lookup(state_machine_, results_);

		if (results_.id == eNumber)
		{
			std::stringstream ss_;

			ss_ << results_.str();
			ss_ >> std::hex >> num_;
		}
		else if (results_.id == eName)
		{
			const std::string name_ = results_.str();

			map_[name_].insert(lexertl::basic_string_token<std::size_t>::range
			(num_, num_));
		}
	} while (results_.id != 0);

	using string_map = std::map<std::string, std::string>;
	map::const_iterator i_ = map_.begin();
	map::const_iterator e_ = map_.end();
	std::string line_;
	char c_ = 0;

	for (; i_ != e_; ++i_)
	{
		if (c_ != i_->first[0])
		{
			if (c_ != 0)
			{
				os_ << "]\";\n    }\n";
			}

			c_ = i_->first[0];
			os_ << "\n    static const char* " << c_ << "()\n" <<
				"    {\n        return \"[";
			ucs_ << "    { \"" << c_ << "\", " << c_ << " },\n";
		}

		os_ << "\\\\p{" << i_->first << '}';
		ucs_ << "    { \"" << i_->first << "\", " << i_->first << " },\n";
	}

	os_ << "]\";\n    }\n";
	i_ = map_.begin();
	e_ = map_.end();

	for (; i_ != e_; ++i_)
	{
		os_ << "\n    static const char *" << i_->first <<
			"()\n" << "    {\n";
		line_ = "        return \"[";

		auto ri = i_->second._ranges.cbegin();
		auto re = i_->second._ranges.cend();

		for (; ri != re; ++ri)
		{
			std::size_t max_chars = 79;
			std::ostringstream ss_;
			std::string range_;

			ss_ << "\\\\x" << std::hex << ri->first;

			if (ri->first != ri->second)
			{
				if (ri->second - ri->first > 1)
				{
					--max_chars;
					ss_ << '-';
				}

				ss_ << "\\\\x" << ri->second;
			}

			range_ = ss_.str();

			if (ri + 1 == re)
			{
				max_chars -= 3;
			}

			if (line_.size() + range_.size() > max_chars)
			{
				line_ += "\"\n";
				os_ << line_;
				line_ = "            \"";
			}

			line_ += range_;
		}

		line_ += "]\";\n    }\n";
		os_ << line_;
	}
}

void case_mapping(lexertl::memory_file& mf_, std::ostream& os2_,
	std::ostream& os4_)
{
	lexertl::rules rules_;
	lexertl::state_machine sm_;
	lexertl::cmatch results_(mf_.data(), mf_.data() + mf_.size());
	enum e_Token { eEOF, eCodeValue, eName, eLl, eLu, eNeither, eMapping, eEmpty };
	e_Token eToken = eEOF;
	std::string code_;
	std::string mapping_;
	std::map<std::size_t, std::size_t> map;

	rules_.push_state("NAME");
	rules_.push_state("TYPE");
	rules_.push_state("Ll");
	rules_.push_state("Lu");
	rules_.push_state("MAPPING");
	rules_.push_state("END");

	rules_.push("INITIAL", "^[0-9A-F]{4,6};", eCodeValue, "NAME");
	rules_.push("NAME", "[^;]*;", lexertl::rules::skip(), "TYPE");
	rules_.push("TYPE", "Ll;", eLl, "Ll");
	rules_.push("Ll", "([^;]*;){9}", lexertl::rules::skip(), "MAPPING");
	rules_.push("TYPE", "Lu;", eLu, "Lu");
	rules_.push("Lu", "([^;]*;){10}", lexertl::rules::skip(), "MAPPING");
	rules_.push("TYPE", "[^;]*;", eNeither, "END");
	rules_.push("MAPPING", ";", eEmpty, "END");
	rules_.push("MAPPING", "[0-9A-F]{4,6};", eMapping, "END");
	rules_.push("END", "[^\n]*\n", lexertl::rules::skip(), "INITIAL");
	lexertl::generator::build(rules_, sm_);

	do
	{
		lexertl::lookup(sm_, results_);
		eToken = static_cast<e_Token>(results_.id);

		if (eToken == eEOF)
		{
			break;
		}
		else if (eToken != eCodeValue)
		{
			throw std::runtime_error("Syntax error");
		}

		code_ = results_.str();
		lexertl::lookup(sm_, results_);
		eToken = static_cast<e_Token>(results_.id);

		if (eToken != eLl && eToken != eLu && eToken != eNeither)
		{
			throw std::runtime_error("Syntax error");
		}

		if (eToken != eNeither)
		{
			lexertl::lookup(sm_, results_);
			eToken = static_cast<e_Token>(results_.id);

			if (eToken == eMapping)
			{
				std::stringstream ss_;
				std::size_t code_val_;
				std::size_t mapping_val_;

				mapping_ = results_.str();
				ss_ << code_;
				ss_ >> std::hex >> code_val_;
				ss_.str("");
				ss_ << mapping_;
				ss_ >> std::hex >> mapping_val_;
				map[code_val_] = mapping_val_;
				code_.clear();
				mapping_.clear();
			}
		}
	} while (results_.id != 0);

	std::pair<std::size_t, std::size_t> first_{ 0, 0 };
	std::pair<std::size_t, std::size_t> second_{ 0, 0 };

	for (auto& pair : map)
	{
		if (first_.first == 0)
		{
			first_.first = first_.second = pair.first;
			second_.first = second_.second = pair.second;
		}
		else if (pair.first == first_.second + 1 &&
			(pair.second == second_.second + 1 ||
				(pair.second <= pair.first &&
					pair.second == second_.second - 1)))
		{
			first_.second = pair.first;
			second_.second = pair.second;
		}
		else
		{
			std::ostream& ss_ = first_.first > 0xffff ? os4_ : os2_;

			ss_ << "            {{0x" << std::hex << std::setfill('0') <<
				std::setw(4) << first_.first << ", 0x" << std::setfill('0') <<
				std::setw(4) << first_.second << "}, {0x" <<
				std::setfill('0') << std::setw(4) << second_.first << ", 0x" <<
				std::setfill('0') << std::setw(4) << second_.second << "}},\n";
			first_.first = first_.second = pair.first;
			second_.first = second_.second = pair.second;
		}
	}

	os4_ << "            {{0x" << std::hex << std::setfill('0') <<
		std::setw(4) << first_.first << ", 0x" << std::setfill('0') <<
		std::setw(4) << first_.second << "}, {0x" << std::setfill('0') <<
		std::setw(4) << second_.first << ", 0x" << std::setfill('0') <<
		std::setw(4) << second_.second << "}},\n";
}

void lex_blocks_data(lexertl::memory_file& mf_, std::ostream& dcpps_,
	std::ostream& ucs_)
{
	enum
	{
		eStartRange = 1, eEndRange, eName
	};
	lexertl::rules rules_;
	lexertl::state_machine sm_;

	rules_.push_state("DOT_DOT");
	rules_.push_state("END_RANGE");
	rules_.push_state("SEP");
	rules_.push_state("NAME");
	rules_.push("INITIAL", "[A-F0-9]+", eStartRange, "DOT_DOT");
	rules_.push("DOT_DOT", "[.][.]", rules_.skip(), "END_RANGE");
	rules_.push("END_RANGE", "[A-F0-9]+", eEndRange, "SEP");
	rules_.push("SEP", "; ", rules_.skip(), "NAME");
	rules_.push("NAME", ".+", eName, "INITIAL");
	rules_.push("#.*|\\s+", rules_.skip());
	lexertl::generator::build(rules_, sm_);

	lexertl::citerator iter_(mf_.data(), mf_.data() + mf_.size(), sm_);
	lexertl::citerator end_;
	bool first_ = true;

	for (; iter_ != end_; ++iter_)
	{
		std::string name_;
		std::string fname_;
		std::string range_;

		range_ = "\"[\\\\x" + iter_->str() + "-\\\\x";
		++iter_;
		range_ += iter_->str() + "]\"";
		++iter_;
		name_ = iter_->str();
		std::transform(name_.begin(), name_.end(), name_.begin(),
			[](const char c)
			{
				return c == ' ' ? '_' : c;
			});

		fname_ = name_;
		std::transform(fname_.begin(), fname_.end(), fname_.begin(),
			[](const char c)
			{
				return c == '-' ? '_' : c;
			});
		dcpps_ << "\n    static const char *" << fname_ <<
			"()\n" << "    {\n" << "        return " << range_ <<
			";\n    }\n";

		ucs_ << "    { \"" << "In" << name_ << "\", " << fname_ << " },\n";
		first_ = false;
	}

	ucs_ << "    { 0, 0 }\n";
}

int main()
{
	// http://www.unicode.org/Public/14.0.0/ucd/
	lexertl::memory_file umf_("UnicodeData-14.0.0d3.txt");
	lexertl::memory_file bmf_("Blocks-14.0.0d3.txt");
	std::ofstream us_("../include/lexertl/parser/tokeniser/unicode.hpp",
		std::ofstream::out);
	std::ofstream fs2_("../include/lexertl/parser/tokeniser/fold2.inc",
		std::ofstream::out);
	std::ofstream fs4_("../include/lexertl/parser/tokeniser/fold4.inc",
		std::ofstream::out);
	std::ofstream dcpps_("../include/lexertl/parser/tokeniser/blocks.hpp",
		std::ofstream::out);
	std::ofstream ucs_("../include/lexertl/parser/tokeniser/table.inc",
		std::ofstream::out);

	lex_unicode_data(umf_, us_, ucs_);
	case_mapping(umf_, fs2_, fs4_);
	us_.close();
	fs2_.close();
	fs4_.close();
	lex_blocks_data(bmf_, dcpps_, ucs_);
	dcpps_.close();
	ucs_.close();
	return 0;
}
