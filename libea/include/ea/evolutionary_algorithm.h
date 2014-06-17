/* evolutionary_algorithm.h
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
#ifndef _EA_EVOLUTIONARY_ALGORITHM_H_
#define _EA_EVOLUTIONARY_ALGORITHM_H_

#include <boost/iterator/indirect_iterator.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/shared_ptr.hpp>

#include <ea/ancestors.h>
#include <ea/concepts.h>
#include <ea/data_structures/shared_ptr_vector.h>
#include <ea/events.h>
#include <ea/individual.h>
#include <ea/lifecycle.h>
#include <ea/metadata.h>
#include <ea/mutation.h>
#include <ea/population_structure.h>
#include <ea/selection.h>
#include <ea/recombination.h>
#include <ea/representation.h>
#include <ea/rng.h>
#include <ea/stopping.h>
#include <ea/traits.h>

namespace ealib {
    
    /*! Generic evolutionary algorithm class.
	 
	 This class is designed to be generic, such that all (most) main features of
     evolutionary algorithms can be incorporated.  The focus of this class is on 
     the common features of most EAs, while leaving the problem-specific 
     components easily customizable.

     \warning See note below regarding copy construction.
     */
    template
    < typename Representation
    , typename FitnessFunction
	, typename MutationOperator
	, typename RecombinationOperator
	, typename GenerationalModel
    , typename AncestorGenerator
    , typename StopCondition=dont_stop
    , typename PopulationGenerator=fill_population
    , typename Lifecycle=default_lifecycle
    , template <typename> class IndividualTraits=fitness_trait
    > class evolutionary_algorithm {
    public:
        typedef singlePopulationS population_structure_tag;
        typedef Representation representation_type;
        typedef typename representation_type::genome_type genome_type;
        typedef typename representation_type::phenotype_type phenotype_type;
        typedef typename representation_type::encoding_type encoding_type;
        typedef FitnessFunction fitness_function_type;
        typedef typename fitness_function_type::fitness_type fitness_type;
        typedef MutationOperator mutation_operator_type;
        typedef RecombinationOperator recombination_operator_type;
        typedef GenerationalModel generational_model_type;
        typedef AncestorGenerator ancestor_generator_type;
        typedef StopCondition stop_condition_type;
        typedef PopulationGenerator population_generator_type;
        typedef Lifecycle lifecycle_type;
        typedef IndividualTraits<evolutionary_algorithm> individual_traits_type;
        typedef individual<representation_type, individual_traits_type> individual_type;
        typedef boost::shared_ptr<individual_type> individual_ptr_type;
        typedef metadata md_type;
        typedef default_rng_type rng_type;
        typedef event_handler<evolutionary_algorithm> event_handler_type;
        typedef shared_ptr_vector<individual_ptr_type> population_type;
        typedef boost::indirect_iterator<typename population_type::iterator> iterator;
        typedef boost::indirect_iterator<typename population_type::const_iterator> const_iterator;
        typedef boost::indirect_iterator<typename population_type::reverse_iterator> reverse_iterator;
        typedef boost::indirect_iterator<typename population_type::const_reverse_iterator> const_reverse_iterator;
        
        //! Default constructor.
        evolutionary_algorithm() : _update(0) {
            BOOST_CONCEPT_ASSERT((EvolutionaryAlgorithmConcept<evolutionary_algorithm>));
            BOOST_CONCEPT_ASSERT((IndividualConcept<individual_type>));
            _lifecycle.after_construction(*this);
        }

        /*! Copy constructor.
         
         Regarding copy construction and assignment: There are a number of
         features of EAs that do not lend themselves to copy construction and assignment,
         particularly those related to events (the situation is a bit worse in digital
         evolution, where we also need to configure tasks and resources).  For this
         reason, a copy (or assignment) of an EA, while allowed, does not copy the
         entire EA.  A quick rule of thumb is that if it's configured via the lifecycle
         object, then it's likely not copied (or assigned).
         */
        evolutionary_algorithm(const evolutionary_algorithm& that) {
            _update = that._update;
            _rng = that._rng;
            _md = that._md;
            _fitness_function = that._fitness_function;
            _generational_model = that._generational_model;
            _stop = _stop;
            // _events are not copied; they must be configured via the lifecycle
            _lifecycle = that._lifecycle;

            // call the lifecycle object to take care of non-copyables:
            _lifecycle.after_construction(*this);
            initialize(); // and initialize the EA
            
            // and finally copy the individuals:
            for(const_iterator i=that.begin(); i!=that.end(); ++i) {
                individual_ptr_type q = copy_individual(*i);
                insert(end(),q);
            }
        }

        //! Assignment operator (see note above re: copy construction).
        evolutionary_algorithm& operator=(const evolutionary_algorithm& that) {
            if(this != &that) {
                _update = that._update;
                _rng = that._rng;
                _md = that._md;
                _fitness_function = that._fitness_function;
                _generational_model = that._generational_model;
                _stop = _stop;
                // _events are not copied; they must be configured via the lifecycle
                _lifecycle = that._lifecycle;
                
                // call the lifecycle object to take care of non-copyables:
                _lifecycle.after_construction(*this);
                initialize(); // and initialize the EA
                
                // and finally copy the individuals:
                for(const_iterator i=that.begin(); i!=that.end(); ++i) {
                    individual_ptr_type q = copy_individual(*i);
                    insert(end(),q);
                }
            }
            return *this;
        }

        //! Initializes this EA.
        void initialize() {
            initialize_fitness_function(_fitness_function, *this);
            _lifecycle.initialize(*this);
        }

        //! Marks the beginning of a new epoch.
        void begin_epoch() {
            _events.record_statistics(*this);
        }

        //! Advances this EA by one update.
        void update() {
            if(!_population.empty()) {
                _generational_model(_population, *this);
            }
            _events.end_of_update(*this);
            ++_update;
            _events.record_statistics(*this);
        }

        //! Marks the end of an epoch.
        void end_epoch() {
            _events.end_of_epoch(*this);
        }

        //! Resets the population.
        void reset() {
            nullify_fitness(begin(), end(), *this);
        }

        //! Resets this EA's RNG seed.
        void reset_rng(unsigned int s) {
            put<RNG_SEED>(s,*this); // save the seed!
            _rng.reset(s);
        }

        //! Returns true if this EA should stop.
        bool stop() {
            return _stop(*this);
        }
        
        //! Returns a new individual built from the given genome.
        individual_ptr_type make_individual(const genome_type& g=genome_type()) {
            individual_ptr_type p(new individual_type(g));
            return p;
        }
        
        //! Returns a copy of an individual.
        individual_ptr_type copy_individual(const individual_type& ind) {
            individual_ptr_type p(new individual_type(ind));
            return p;
        }
        
        //! Returns the current update of this EA.
        unsigned long current_update() { return _update; }

        //! Returns the random number generator.
        rng_type& rng() { return _rng; }

        //! Returns this EA's meta-data.
        md_type& md() { return _md; }

        //! Returns this EA's meta-data (const-qualified).
        const md_type& md() const { return _md; }

        //! Returns the fitness function object.
        fitness_function_type& fitness_function() { return _fitness_function; }

        //! Returns the event handler.
        event_handler_type& events() { return _events; }

        //! Returns the lifecycle object.
        lifecycle_type& lifecycle() { return _lifecycle; }

        //! Returns this EA's population.
        population_type& population() { return _population; }

        //! Returns the size of this EA's population.
        std::size_t size() const { return _population.size(); }

        //! Returns the n'th individual in the population.
        individual_type& operator[](std::size_t n) { return *_population[n]; }

        //! Returns a begin iterator to the population.
        iterator begin() { return iterator(_population.begin()); }

        //! Returns an end iterator to the population.
        iterator end() { return iterator(_population.end()); }

        //! Returns a begin iterator to the population (const-qualified).
        const_iterator begin() const { return const_iterator(_population.begin()); }

        //! Returns an end iterator to the population (const-qualified).
        const_iterator end() const { return const_iterator(_population.end()); }

        //! Returns a reverse begin iterator to the population.
        reverse_iterator rbegin() { return reverse_iterator(_population.rbegin()); }

        //! Returns a reverse end iterator to the population.
        reverse_iterator rend() { return reverse_iterator(_population.rend()); }

        //! Returns a reverse begin iterator to the population (const-qualified).
        const_reverse_iterator rbegin() const { return const_reverse_iterator(_population.rbegin()); }
        
        //! Returns a reverse end iterator to the population (const-qualified).
        const_reverse_iterator rend() const { return const_reverse_iterator(_population.rend()); }
        
        //! Inserts individual x into the population before pos.
        void insert(iterator pos, individual_ptr_type x) { _population.insert(pos.base(), x); }
        
        //! Inserts individuals [f,l) into the population before pos.
        template <typename InputIterator>
        void insert(iterator pos, InputIterator f, InputIterator l) { _population.insert(pos.base(), f, l); }
        
        //! Erases the given individual from the population.
        void erase(iterator i) { _population.erase(i.base()); }
        
        //! Erases the given range from the population.
        void erase(iterator f, iterator l) { _population.erase(f.base(), l.base()); }
        
        //! Erases all individuals in this EA.
        void clear() { _population.clear(); }
        
    protected:
        unsigned long _update; //!< Update number for this EA.
        rng_type _rng; //!< Random number generator.
        md_type _md; //!< Meta-data for this evolutionary algorithm instance.
        population_type _population; //!< Population instance.
        fitness_function_type _fitness_function; //!< Fitness function object.
        generational_model_type _generational_model; //!< Generational model instance.
        stop_condition_type _stop; //!< Stop condition.
        event_handler_type _events; //!< Event handler.
        lifecycle_type _lifecycle; //!< Lifecycle methods.

    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & boost::serialization::make_nvp("update", _update);
            ar & boost::serialization::make_nvp("rng", _rng);
            ar & boost::serialization::make_nvp("metadata", _md);
            ar & boost::serialization::make_nvp("population", _population);
        }
    };
    
} // ealib

#endif
