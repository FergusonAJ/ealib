/* markov_network.h
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
#ifndef _EA_MKV_MARKOV_NETWORK_H_
#define _EA_MKV_MARKOV_NETWORK_H_

#include <boost/tuple/tuple.hpp>

#include <ea/configuration.h>
#include <ea/cmdline_interface.h>
#include <ea/representations/circular_genome.h>
#include <ea/mutation.h>
#include <ea/translation.h>
#include <ea/rng.h>
#include <ea/mkv/gates.h>
#include <ea/mkv/translation.h>

namespace ealib {
    namespace mkv {
        enum { IN, OUT, HID }; //!< Indices into the desc_type for number of inputs, outputs, and hidden states.
        
        //! Descriptor (ninput, noutput, nhidden) for a Markov network.
        typedef boost::tuple<std::size_t, std::size_t, std::size_t> desc_type;
    } // mkv
    
    
    /*! Markov Network.
     */
    template <typename RandomNumberGenerator=default_rng_type>
    class markov_network {
    public:
        typedef mkv::state_vector_type state_vector_type; //!< Convenience typedef for the state vector.
        typedef int* iterator; //!< Type for iterators over state variables.
        typedef mkv::abstract_gate<RandomNumberGenerator> abstract_gate_type; //!< Type of abstract gate held by this network.
        typedef boost::shared_ptr<abstract_gate_type> abstract_gate_ptr; //!< Abstract gate pointer type.
        typedef std::vector<abstract_gate_ptr> gate_vector_type; //!< Type for a vector of abstract gates.
        typedef RandomNumberGenerator rng_type; //!< Random number generator type.
        
        //! Constructor.
        markov_network(std::size_t nin, std::size_t nout, std::size_t nhid, unsigned int seed=0) : _rng(seed) {
            resize(nin, nout, nhid);
        }
        
        //! Constructor.
        markov_network(mkv::desc_type desc, unsigned int seed=0) : _rng(seed) {
            resize(desc.get<mkv::IN>(), desc.get<mkv::OUT>(), desc.get<mkv::HID>());
        }
        
        //! Constructor.
        markov_network(std::size_t nin, std::size_t nout, std::size_t nhid, const rng_type& rng) : _rng(rng) {
            resize(nin, nout, nhid);
        }
        
        //! Constructor.
        markov_network(mkv::desc_type desc, const rng_type& rng) : _rng(rng) {
            resize(desc.get<mkv::IN>(), desc.get<mkv::OUT>(), desc.get<mkv::HID>());
        }
        
        //! Resize this network.
        void resize(std::size_t nin, std::size_t nout, std::size_t nhid) {
            _nin = nin;
            _nout = nout;
            _nhid = nhid;
            _T.resize(_nin+_nout+_nhid);
            _Tplus1.resize(_nin+_nout+_nhid);
            clear();
        }
        
        //! Clears this network (resets all state variables).
        void clear() {
            std::fill(_T.begin(), _T.end(), 0);
            std::fill(_Tplus1.begin(), _Tplus1.end(), 0);
        }
        
        //! Reset this network's rng.
        void reset(unsigned int seed) {
            _rng.reset(seed);
        }
        
        //! Retrieve the size of this network, in number of gates.
        std::size_t ngates() const { return _gates.size(); }
        
        //! Retrieve a vector of this network's gates.
        gate_vector_type& gates() { return _gates; }
        
        //! Retrieve gate i.
        abstract_gate_type& operator[](std::size_t i) { return *_gates[i]; }
        
        //! Retrieve gate i (const-qualified).
        const abstract_gate_type& operator[](std::size_t i) const { return *_gates[i]; }
        
        //! Retrieve the number of state variables in this network.
        std::size_t nstates() const { return _T.size(); }
        
        //! Retrieve the number of inputs to this network.
        std::size_t ninputs() const { return _nin; }
        
        //! Retrieve the number of outputs from this network.
        std::size_t noutputs() const { return _nout; }
        
        //! Retrieve the number of hiddenn state variables in this network.
        std::size_t nhidden() const { return _nhid; }
        
        //! Retrieve state variable i.
        int& operator()(std::size_t i) { return _T(i); }
        
        //! Retrieve state variable i (const-qualified).
        const int& operator()(std::size_t i) const { return _T(i); }
        
        //! Retrieve an iterator to the beginning of the outputs.
        iterator begin_output() { return &_T[0] + _nin; }
        
        //! Retrieve an iterator to the end of the outputs.
        iterator end_output() { return &_T[0] + _nin + _nout; }
        
        /*! Zero-copy update.
         
         \param f points to the inputs to this network.
         */
        template <typename RandomAccessIterator>
        void update(RandomAccessIterator f, std::size_t n=1) {
            for( ; n>0; --n) {
                for(typename gate_vector_type::iterator i=_gates.begin(); i!=_gates.end(); ++i) {
                    // get the input to this gate:
                    mkv::index_vector_type& inputs=(*i)->inputs;
                    int x=0;
                    for(std::size_t j=0; j<inputs.size(); ++j) {
                        std::size_t k=inputs[j];
                        if(k<_nin) {
                            x |= (f[k] & 0x01) << j;
                        } else {
                            x |= (_T(k) & 0x01) << j;
                        }
                    }
                    
                    // calculate the output:
                    int y = (**i)(x,_rng);
                    
                    // set the output from this gate:
                    mkv::index_vector_type& outputs=(*i)->outputs;
                    for(std::size_t j=0; j<outputs.size(); ++j) {
                        _Tplus1(outputs[j]) |= ((y>>j) & 0x01);
                    }
                }
            }
            std::swap(_T,_Tplus1);
            std::fill(_Tplus1.begin()+_nin, _Tplus1.end(), 0); // don't reset internal inputs
        }
        
        //! Update this Markov network n times, assuming all inputs have been set.
        void update(std::size_t n=1) {
            update(_T.begin(), n);
        }

    protected:
        rng_type _rng; //<! Random number generator.
        std::size_t _nin, _nout, _nhid; //!< Number of inputs, outputs, and hidden state variables.
        gate_vector_type _gates; //!< Vector of gates.
        state_vector_type _T; //!< State vector for time t.
        state_vector_type _Tplus1; //!< State vector for time t+1.
        
    private:
        //! Copy constructor.
        markov_network(const markov_network& that);
        
        //! Assignment operator.
        markov_network& operator=(const markov_network& that);
    };
    
    
    namespace mkv {
        
        /*! Configuration object for EAs that use Markov Networks.
         */
        template <typename EA>
        struct configuration : public abstract_configuration<EA> {
            typedef indirectS encoding_type;
            typedef markov_network< > phenotype;
            typedef boost::shared_ptr<phenotype> phenotype_ptr;
            
            //! Translate an individual's representation into a Markov Network.
            virtual phenotype_ptr make_phenotype(typename EA::individual_type& ind,
                                                 typename EA::rng_type& rng, EA& ea) {
                phenotype_ptr p(new phenotype(desc, rng));
                translate_genome(ind.repr(), start, translator, *p);
                return p;
            }
            
            //! Called as the first step of an EA's lifecycle.
            virtual void configure(EA& ea) {
            }
            
            //! Called to generate the initial EA population.
            virtual void initial_population(EA& ea) {
                generate_ancestors(mkv::ancestor(), get<POPULATION_SIZE>(ea), ea);
            }
            
            //! Called as the final step of EA initialization.
            virtual void initialize(EA& ea) {
                desc = desc_type(get<MKV_INPUT_N>(ea), get<MKV_OUTPUT_N>(ea), get<MKV_HIDDEN_N>(ea));
            }
            
            desc_type desc; //!< Description for Markov network (# in, out, & hidden).
            start_codon start; //!< Start codon detector.
            genome_translator translator; //!< Genome translator.
        };
        
        typedef circular_genome<int> representation_type;
        typedef mutation::operators::indel<mutation::operators::per_site<mutation::site::uniform_integer> > mutation_type;
        
        /*! Add the common Markov Network configuration options to the command line interface.
         */
        template <typename EA>
        void add_options(cmdline_interface<EA>* ci) {
            using namespace ealib;
            // markov network options
            add_option<MKV_LAYERS_N>(ci);
            add_option<MKV_INPUT_N>(ci);
            add_option<MKV_OUTPUT_N>(ci);
            add_option<MKV_HIDDEN_N>(ci);
            add_option<MKV_INITIAL_GATES>(ci);
            
            // ea options
            add_option<REPRESENTATION_INITIAL_SIZE>(ci);
            add_option<REPRESENTATION_MIN_SIZE>(ci);
            add_option<REPRESENTATION_MAX_SIZE>(ci);
            add_option<MUTATION_PER_SITE_P>(ci);
            add_option<MUTATION_UNIFORM_INT_MIN>(ci);
            add_option<MUTATION_UNIFORM_INT_MAX>(ci);
            add_option<MUTATION_DELETION_P>(ci);
            add_option<MUTATION_INSERTION_P>(ci);
            add_option<MUTATION_INDEL_MIN_SIZE>(ci);
            add_option<MUTATION_INDEL_MAX_SIZE>(ci);
        }
        
    } // mkv
} // ealib

#endif
