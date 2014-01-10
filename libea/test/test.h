/* test.h
 * 
 * This file is part of EALib.
 * 
 * Copyright 2012 David B. Knoester.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _TEST_H_
#define _TEST_H_

#include <boost/test/unit_test.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <sstream>

#include <ea/evolutionary_algorithm.h>
#include <ea/representations/bitstring.h>
#include <ea/fitness_functions/all_ones.h>
#include <ea/cmdline_interface.h>
#include <ea/datafiles/fitness.h>

using namespace ealib;

typedef evolutionary_algorithm
< individual<bitstring, all_ones>
, ancestors::random_bitstring
, mutation::operators::per_site<mutation::site::bit>
, recombination::two_point_crossover
, generational_models::steady_state< >
> all_ones_ea;

template <typename EA>
void add_std_meta_data(EA& ea) {
	put<POPULATION_SIZE>(1024,ea);
	put<STEADY_STATE_LAMBDA>(2,ea);
	put<REPRESENTATION_SIZE>(10,ea);
	put<MUTATION_PER_SITE_P>(0.1,ea);
	put<TOURNAMENT_SELECTION_N>(2,ea);
	put<TOURNAMENT_SELECTION_K>(1,ea);	
}


#endif
