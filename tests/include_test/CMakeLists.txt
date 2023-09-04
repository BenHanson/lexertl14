#
# Part of CMake configuration for lexertl library
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
include_directories(.)

add_executable(lexertl_include_tests
	# bitvector.cpp
	blocks.cpp
	charset.cpp
	char_traits.cpp
	debug.cpp
	dot.cpp
	end_node.cpp
	enum_operator.cpp
	enums.cpp
	equivset.cpp
	generate_cpp.cpp
	generator.cpp
	internals.cpp
	iteration_node.cpp
	iterator.cpp
	leaf_node.cpp
	lookup.cpp
	match_results.cpp
	memory_file.cpp
	narrow.cpp
	node.cpp
	observer_ptr.cpp
	parser.cpp
	# ptr_list.cpp
	# ptr_map.cpp
	# ptr_stack.cpp
	# ptr_vector.cpp
	replace.cpp
	re_token.cpp
	re_tokeniser.cpp
	re_tokeniser_helper.cpp
	re_tokeniser_state.cpp
	rules.cpp
	runtime_error.cpp
	scripts.cpp
	selection_node.cpp
	sequence_node.cpp
	serialise.cpp
	sm_to_csm.cpp
	sm_traits.cpp
	state_machine.cpp
	stream_num.cpp
	stream_shared_iterator.cpp
	string_token.cpp
	unicode.cpp
	utf_iterators.cpp
	main.cpp)
target_link_libraries(lexertl_include_tests PRIVATE lexertl)
