/* nk.h
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
#ifndef _EA_FITNESS_FUNCTIONS_NK_MODEL_H_
#define _EA_FITNESS_FUNCTIONS_NK_MODEL_H_

#include <ea/fitness_function.h>
#include <limits>
#include <cmath>

namespace ealib {
    
    LIBEA_MD_DECL(NK_MODEL_N, "ea.fitness_function.nk_model.n", unsigned int);
    LIBEA_MD_DECL(NK_MODEL_K, "ea.fitness_function.nk_model.k", unsigned int);
	
    //! Selector for arithmetic NK landscape.
    struct arithmeticS { };

    //! Selector for geometric NK landscape.
    struct geometricS { };
    
    /*! Fitness function corresponding to the NK Model~\cite{Kauffmann?}.
     
     The NK model defines a genome S of length N, with each loci s_i having a value 
     drawn from alphabet A (usually binary) and interacting with K other loci.
     
     The total fitness F(S) is defined as:
         F(S) = mean f(s_i)
     (either arithmetic or geometric mean, which is selected via a tag>
     
     f(s_i) is defined as the fitness contribution of a single loci, but each loci
     interacts with K other loci:
         f(s_i) = f(s_i, s^i_1, s^i_2,... s^i_K)
     So: K=0 means that each loci can be independently optimized, which in this case
     degenerates into a search for a specific genome.  K=1 means that each loci 
     interacts with a single other loci, K=N-1 is fully interactive, and so on.  
     The pattern of interactions is usually circular (e.g., {s_1,s_2...}, {s_2,s_3...}).
     
     The value of f(s_i) is typically defined randomly for each s_i.  Specifically,
     each loci s in N has a table of 2^K uniformly distributed random numbers between
     [0,1] (representing all possible states of {s^i_1, s^i_2,... s^i_K}).
     */
    template <typename MeanTag=geometricS>
    struct nk_model : public fitness_function<unary_fitness<double>, constantS, deterministicS> {
        typedef std::vector<double> k_table;
        typedef std::vector<k_table> nk_table;
        nk_table nkt;

        /*! Build the fitness table.
         
         Care must be taken when building the fitness table to allow for repeatability
         both among runs, and in the case where a landscape search is performed (and
         the k table changes size).
         
         In this version of the NK model, we use a new rng for each of the N loci,
         and we seed the rng based on N, so we have repeatability.
         */
        template <typename EA>
        void initialize(EA& ea) {
            int K = get<NK_MODEL_K>(ea);
            int ktsize=1<<(K+1);
            int N = get<NK_MODEL_N>(ea);
            nkt.resize(N);
            int seed = get<FF_RNG_SEED>(ea,0);
            // is this a random sample?  if so, get a random seed and save it for later
            // checkpointing.  we do it this way (instead of a 0 seed) because it's
            // quite likely that subsequent k tables could be generated at the same 
            // wall-clock time (which is what a seed of 0 means).
            if(seed == 0) {
                seed = ea.rng().seed();
                put<FF_RNG_SEED>(seed,ea);
            }
            
            for(int i=0; i<N; ++i) {
                k_table& kt=nkt[i];
                kt.resize(ktsize);
                typename EA::rng_type rng(seed+i);
                for(int j=0; j<ktsize; ++j) {
                    kt[j] = rng.uniform_real_nz(0.0,1.0);
                }
            }
        }
        
        //! Calculate the fitness of the given representation.
        template <typename Individual, typename EA>
        double operator()(Individual& ind, EA& ea) {
            typename EA::genome_type& repr=ind.genome();

            double S=0.0;
            for(std::size_t i=0; i<nkt.size(); ++i) {
                k_table& kt=nkt[i];
                std::size_t entry=0;
                for(std::size_t j=0; j<(get<NK_MODEL_K>(ea)+1); ++j) {
                    entry |= repr[(i+j)%repr.size()] << j;
                }
                
                assert(entry < kt.size());
                S = accumulate(S, kt[entry]);
            }
            S = mean(S, static_cast<double>(nkt.size()));
            return S;
        }
        
        double accumulate(double s, double v) {
            return accumulate(s, v, MeanTag());
        }

        double accumulate(double s, double v, arithmeticS) {
            return s + v;
        }
        
        double accumulate(double s, double v, geometricS) {
            if(v != 0.0) {
                return s + log(v);
            } else {
                return 0.0;
            }
        }
        
        double mean(double s, double n) {
            return mean(s, n, MeanTag());
        }
        
        double mean(double s, double n, arithmeticS) {
            return s/n;
        }
        
        double mean(double s, double n, geometricS) {
            if(s != 0.0) {
                s /= n; 
                return exp(s);
            } else {
                return 0.0;
            }
        }
    };	
}

#endif
