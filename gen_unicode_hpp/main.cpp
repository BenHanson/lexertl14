#include "../include/lexertl/generator.hpp"
#include <iomanip>
#include <fstream>
#include "../include/lexertl/lookup.hpp"
#include "../include/lexertl/memory_file.hpp"

void lex_unicode_data(lexertl::memory_file& mf_, std::ostream& os_)
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
	string_map code_map;
	map::const_iterator i_ = map_.begin();
	map::const_iterator e_ = map_.end();
	std::string line_;

	code_map["Cc"] = "other_control";
	code_map["Cf"] = "other_format";
	code_map["Co"] = "other_private";
	code_map["Cs"] = "other_surrogate";
	code_map["Ll"] = "letter_lowercase";
	code_map["Lm"] = "letter_modifier";
	code_map["Lo"] = "letter_other";
	code_map["Lt"] = "letter_titlecase";
	code_map["Lu"] = "letter_uppercase";
	code_map["Mc"] = "mark_combining";
	code_map["Me"] = "mark_enclosing";
	code_map["Mn"] = "mark_nonspacing";
	code_map["Nd"] = "number_decimal";
	code_map["Nl"] = "number_letter";
	code_map["No"] = "number_other";
	code_map["Pc"] = "punctuation_connector";
	code_map["Pd"] = "punctuation_dash";
	code_map["Pe"] = "punctuation_close";
	code_map["Pf"] = "punctuation_final";
	code_map["Pi"] = "punctuation_initial";
	code_map["Po"] = "punctuation_other";
	code_map["Ps"] = "punctuation_open";
	code_map["Sc"] = "symbol_currency";
	code_map["Sk"] = "symbol_modifier";
	code_map["Sm"] = "symbol_math";
	code_map["So"] = "symbol_other";
	code_map["Zl"] = "separator_line";
	code_map["Zp"] = "separator_paragraph";
	code_map["Zs"] = "separator_space";

	for (; i_ != e_; ++i_)
	{
		os_ << "\n    static const char *" << code_map[i_->first] <<
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

void case_mapping(lexertl::memory_file& mf_, std::ostream& os2_, std::ostream& os4_)
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

int main()
{
	// http://www.unicode.org/Public/14.0.0/ucd/UnicodeData.txt
	lexertl::memory_file mf_("UnicodeData-14.0.0d3.txt");
	std::ofstream us_("../include/lexertl/parser/tokeniser/unicode.hpp", std::ofstream::out);
	std::ofstream fs2_("../include/lexertl/parser/tokeniser/fold2.inc", std::ofstream::out);
	std::ofstream fs4_("../include/lexertl/parser/tokeniser/fold4.inc", std::ofstream::out);

	lex_unicode_data(mf_, us_);
	case_mapping(mf_, fs2_, fs4_);
	us_.close();
	fs2_.close();
	fs4_.close();
	return 0;
}
