/* test_analysis.cpp
 *
 * This file is part of EALib.
 *
 * Copyright 2014 David B. Knoester.
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
#include <ea/analysis/fitness.h>
#include <ea/analysis/archive.h>

//! Population analysis tests.
BOOST_AUTO_TEST_CASE(test_population_analysis) {
    all_ones_ea ea;
    analysis::unary_population_fitness<all_ones_ea> upf;
    upf(ea);
}

//! Archive tests.
BOOST_AUTO_TEST_CASE(test_archive_analysis) {
    all_ones_ea ea;
	analysis::archive_dominant<all_ones_ea> ad_tool;
	analysis::trim_archive<all_ones_ea> ta_tool;
}
