/* test_neuroevolution.cpp
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
#include <boost/test/unit_test.hpp>
#include <ea/ann/graph/neurodevelopment.h>
#include <ea/ann/graph/feed_forward.h>
#include <ea/graph.h>
#include "test.h"

using namespace ann;

struct graph_fitness : public fitness_function<unary_fitness<double> > {
    template <typename Individual, typename EA>
    double operator()(Individual& ind, EA& ea) {
//        neural_network<neuroevolution<feed_forward_neuron< > > > G;
//
//        delta_growth(G, get<DEV_VERTICES_N>(ea), ind.repr(), ea.rng());
        return 1.0;
    }
};

template <typename EA>
struct graph_configuration : public abstract_configuration<EA> {
    typedef random_ann representation_generator_type;

    void initial_population(EA& ea) {
//        generate_ancestors(random_ann(), get<POPULATION_SIZE>(ea), ea);
    }
};

typedef evolutionary_algorithm<
developmental_network,
mutation::graph_mutator,
graph_fitness,
graph_configuration,
recombination::asexual
> graph_ea;


BOOST_AUTO_TEST_CASE(test_neurodevelopment) {
	using namespace ealib;
    graph_ea E;
}
