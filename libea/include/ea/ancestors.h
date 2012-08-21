/* initialization.h
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
#ifndef _EA_INITIALIZATION_H_
#define _EA_INITIALIZATION_H_

#include <ea/interface.h>
#include <ea/meta_data.h>
#include <ea/generators.h>


namespace ea {
    
    /*! Generates an ancestral population of size n into the given EA.
     
     Ancestral populations are a litle strange, in that even the ancestors must
     themselves have ancestors in order to provide for line-of-descent tracking.
     Individuals in the ancestral population also need the "inherits_from" method
     called on them (as well as the inheritance signal), each of which require
     a parent population.  We handle all of this here, and at the very end, add
     the ancestors to the EA.
     
     We aren't concerned terribly with efficiency here, as it is expected that this
     method is called relatively infrequently.
     */
	template <typename RepresentationGenerator, typename EA>
	void generate_ancestors(RepresentationGenerator g, std::size_t n, EA& ea) {
        BOOST_CONCEPT_ASSERT((EvolutionaryAlgorithmConcept<EA>));

        // build the placeholder ancestor:
        typename EA::individual_ptr_type ap = ea.make_individual(typename EA::representation_type());
        ap->name() = next<INDIVIDUAL_COUNT>(ea);
        ap->generation() = -1.0;
        ap->update() = ea.current_update();
     
        // wrap it in a population:
        typename EA::population_type parents;
        parents.push_back(ap);
        
        // now, build the real ancestral population:
        typename EA::population_type ancestral;
        for( ; n>0; --n) {
            ancestral.push_back(ea.make_individual(g(ea)));
        }

        // trigger inheritance:
        inherits(parents, ancestral, ea);
        
        // and add all the ancestors to the EA:
        ea.append(ancestral.begin(), ancestral.end());
    }


    /*! Fill the EA with individuals constructed from the given representation.
     
     As opposed to the above method, where we generate ancestors, the individuals
     generated here are simply copied from the given representation -- there is no
     attempt made to configure them into an lineage.
     */
    template <typename EA>
    void fill_population(const typename EA::representation_type& r, std::size_t n, EA& ea) {
        BOOST_CONCEPT_ASSERT((EvolutionaryAlgorithmConcept<EA>));

        // build the population:
        typename EA::population_type population;
        for( ; n>0; --n) {
            typename EA::individual_ptr_type p = ea.make_individual(r);
            p->name() = next<INDIVIDUAL_COUNT>(ea);
            p->generation() = -1.0;
            p->update() = ea.current_update();
            population.push_back(p);
        }

        // and add them all to the EA:
        ea.append(population.begin(), population.end());
    }
    
    
    namespace ancestors {
        
        /*! Generates an individual from random bits.
         */
        struct random_bitstring {            
            //! Generate an individual.
            template <typename EA>
            typename EA::representation_type operator()(EA& ea) {
                typename EA::representation_type r;
                r.resize(get<REPRESENTATION_SIZE>(ea));
                
                for(typename EA::representation_type::iterator i=r.begin(); i!=r.end(); ++i) {
                    *i = ea.rng().bit();
                }
                return r;
            }
        };
        
    } // ancestors
    
    namespace initialization {
        /*! Generates an individual from a uniform distribution of integers.
         */
        struct uniform_integer {            
            //! Generate an individual.
            template <typename EA>
            typename EA::population_entry_type operator()(EA& ea) {
                typedef typename EA::representation_type representation_type;
                typename EA::individual_type ind;
                ind.name() = next<INDIVIDUAL_COUNT>(ea);
                ind.repr().resize(get<REPRESENTATION_SIZE>(ea));
                representation_type& repr=ind.repr();
                
                for(typename representation_type::iterator i=repr.begin(); i!=repr.end(); ++i) {
                    *i = ea.rng().uniform_integer(get<INITIALIZATION_UNIFORM_INT_MIN>(ea), get<INITIALIZATION_UNIFORM_INT_MAX>(ea));
                }
                return make_population_entry(ind,ea);
            }
        };
        
        
        /*! Generates an individual from a uniform distribution of reals.
         */
        struct uniform_real {
            template <typename EA>
            typename EA::population_entry_type operator()(EA& ea) {
                typedef typename EA::representation_type representation_type;
                typename EA::individual_type ind;
                ind.name() = next<INDIVIDUAL_COUNT>(ea);
                ind.repr().resize(get<REPRESENTATION_SIZE>(ea));
                representation_type& repr=ind.repr();
                
                for(typename representation_type::iterator i=repr.begin(); i!=repr.end(); ++i) {
                    *i = ea.rng().uniform_real(get<INITIALIZATION_UNIFORM_REAL_MIN>(ea), get<INITIALIZATION_UNIFORM_REAL_MAX>(ea));
                }
                return make_population_entry(ind,ea);
            }
        };
        
        
        /*! Generates a random individual.
         */
        struct random_individual {
            template <typename EA>
            typename EA::population_entry_type operator()(EA& ea) {
                typedef typename EA::representation_type representation_type;
                typename EA::individual_type ind;
                ind.name() = next<INDIVIDUAL_COUNT>(ea);
                ind.repr().resize(get<REPRESENTATION_SIZE>(ea));
                representation_type& repr=ind.repr();
                
                typename EA::mutation_operator_type::mutation_type mt;
                for(typename representation_type::iterator j=repr.begin(); j!=repr.end(); ++j) {
                    mt(repr, j, ea);
                }
                return make_population_entry(ind, ea);
            }
        };
        
        
        /*! Generates a random individual of low fitness.
         */
        struct random_low_fitness {
            template <typename EA>
            typename EA::population_entry_type operator()(EA& ea) {
                // generate a population of random individuals:
                typename EA::population_type population;
                random_individual ig;
                fitness_type(population, ig, get<POPULATION_SIZE>(ea), ea);
                
                // and find the one with the worst fitness:
                typename EA::individual_type mini=ind(population.begin(),ea);
                for(typename EA::population_type::iterator i=population.begin(); i!=population.end(); ++i) {
                    if(ind(i,ea).fitness() < mini.fitness()) {
                        mini = ind(i,ea);
                    }
                }
                return make_population_entry(mini,ea);               
            }
        };
        
        
        /*! Generates a replicate of a given individual, with mutation.
         
         This works by creating the next individual, then replacing its representation
         with the one to be replicated, and then mutating that representation.
         */
        template <typename IndividualType>
        struct replicate_with_mutation {
            replicate_with_mutation(IndividualType i) : _i(i) {
            }
            
            template <typename EA>
            typename EA::population_entry_type operator()(EA& ea) {
                typedef typename EA::representation_type representation_type;
                typename EA::individual_type ind;
                ind.name() = next<INDIVIDUAL_COUNT>(ea);
                ind.repr() = _i.repr();                
                mutate(ind,ea);
                return make_population_entry(ind, ea);
            }
            
            IndividualType _i;
        };
        
        
        /*! Initialization method that generates a complete population.
         */
        template <typename IndividualGenerator>
        struct complete_population {
            template <typename EA>
            void operator()(EA& ea) {
                typename EA::population_type ancestral;
                typename EA::individual_type a = typename EA::individual_type();
                a.name() = next<INDIVIDUAL_COUNT>(ea);
                a.generation() = -1.0;
                a.update() = ea.current_update();
                ancestral.append(make_population_entry(a,ea));
                
                IndividualGenerator ig;
                ea.population().clear();
                generate_individuals_n(ea.population(), ig, get<POPULATION_SIZE>(ea), ea);
                
                for(typename EA::population_type::iterator i=ea.population().begin(); i!=ea.population().end(); ++i) {
                    ea.events().inheritance(ancestral,ind(i,ea),ea);
                }
            }
        };
        
        
        /*! Initialization method whereby the population is grown from a single individual (with mutation).
         */
        template <typename IndividualGenerator>
        struct grown_population {
            template <typename EA>
            void operator()(EA& ea) {
                typedef typename EA::individual_type individual_type;
                // generate the ancestral population:
                typename EA::population_type ancestral;
                IndividualGenerator ig;
                fitness_type(ancestral, ig, 1, ea);
                individual_type& a = ind(ancestral.begin(),ea);
                
                // replicate this ancestor to fill up our population:
                ea.population().clear();
                replicate_with_mutation<individual_type> rg(a);
                fitness_type(ea.population(), rg, get<POPULATION_SIZE>(ea), ea);
                
                for(typename EA::population_type::iterator i=ea.population().begin(); i!=ea.population().end(); ++i) {
                    ea.events().inheritance(ancestral,ind(i,ea),ea);
                }
            }
        };
        
        /*! Initializes all subpopulations that are part of a meta-population EA.
         */
        struct all_subpopulations {
            template <typename EA>
            void operator()(EA& ea) {
                for(typename EA::iterator i=ea.begin(); i!=ea.end(); ++i) {
                    generate_initial_population(*i);
                }
            }
        };
        
    } // initialization
} // ea

#endif
