/* layout.h
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
#ifndef _ANN_LAYOUT_H
#define _ANN_LAYOUT_H

#include <algorithm>
#include <vector>
#include <ea/algorithm.h>

namespace ann {
    namespace layout {

        /*! Generates a multi-layer perceptron style graph.
         
         *f == number of neurons in this layer.
         
         */
        template <typename Graph, typename ForwardIterator>
        void mlp(Graph& g, ForwardIterator f, ForwardIterator l) {
            typedef typename Graph::vertex_descriptor vertex_descriptor;
            typedef std::vector<vertex_descriptor> vec_type;
            typedef std::vector<vec_type> layer_type;
            layer_type layers;
            
            // get all the vertex descriptors we'll need:
            for(; f!=l; ++f) {
                vec_type v;
                for(std::size_t i=0; i<(*f); ++i) {
                    v.push_back(boost::add_vertex(g));
                }
                layers.push_back(v);
            }
            
            // and connect adjoining layers:
            for(std::size_t i=0; i<(layers.size()-1); ++i) {
                for(std::size_t j=0; j<layers[i].size(); ++j) {
                    for(std::size_t k=0; k<layers[i+1].size(); ++k) {
                        boost::add_edge(layers[i][j], layers[i+1][k], g);
                    }
                }
            }
        }
        
        /*! Generates a completely-connected graph.
         
         This is typically used with a Concurrent Time Recurrent Neural Network (CTRNN), which has
         been shown to be a universal smooth approximator.
         */
        template <typename Graph>
        void K(Graph& g) {
            for(std::size_t i=0; i<boost::num_vertices(g); ++i) {
                for(std::size_t j=0; j<boost::num_vertices(g); ++j) {
                    if(i!=j) {
                        boost::add_edge(boost::vertex(i,g), boost::vertex(j,g), g);
                    }
                }
            }
        }
        
        /*! Generates a completely-connected graph with random ordering of connections.
         
         This is typically used with a Concurrent Time Recurrent Neural Network (CTRNN), which has
         been shown to be a universal smooth approximator.
         */
        template <typename Graph, typename RNG>
        void K(Graph& g, RNG& rng) {
            for(std::size_t i=0; i<boost::num_vertices(g); ++i) {
                std::vector<std::size_t> v(boost::num_vertices(g));
                ea::algorithm::iota(v.begin(), v.end());
                std::random_shuffle(v.begin(), v.end(), rng);
                
                for(std::size_t j=0; j<v.size(); ++j) {
                    if(i!=j) {
                        boost::add_edge(boost::vertex(i,g), boost::vertex(v[j],g), g);
                    }
                }
            }
        }        
        
	} // layout
} //nn

#endif
