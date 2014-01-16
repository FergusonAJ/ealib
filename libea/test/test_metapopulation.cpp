/* test_metapopulation.cpp
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
#include "test.h"
#include <ea/metapopulation.h>

BOOST_AUTO_TEST_CASE(test_meta_population) {
    typedef metapopulation<subpopulation<all_ones_ea,constant> > ea_type;
    ea_type M;
    add_std_meta_data(M);
    put<META_POPULATION_SIZE>(5,M);
    
    M.initialize();
    generate_initial_population(M);
    lifecycle::advance_epoch(10,M);
}
