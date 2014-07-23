/* mkv/markov_evolution_algorithm.h
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
#ifndef _MKV_EA_MARKOV_NETWORK_EVOLUTION_H_
#define _MKV_EA_MARKOV_NETWORK_EVOLUTION_H_

#include <boost/algorithm/string/predicate.hpp>
#include <ea/evolutionary_algorithm.h>

#include <ea/lifecycle.h>
#include <ea/cmdline_interface.h>
#include <ea/functional.h>
#include <ea/genome_types/circular_genome.h>
#include <ea/metadata.h>
#include <ea/representation.h>
#include <ea/mkv/analysis.h>
#include <mkv/markov_network.h>

namespace ealib {

	LIBEA_MD_DECL(MKV_UPDATE_N, "markov_network.update.n", std::size_t);
    LIBEA_MD_DECL(MKV_INPUT_N, "markov_network.input.n", std::size_t);
    LIBEA_MD_DECL(MKV_OUTPUT_N, "markov_network.output.n", std::size_t);
    LIBEA_MD_DECL(MKV_HIDDEN_N, "markov_network.hidden.n", std::size_t);
    LIBEA_MD_DECL(MKV_INITIAL_GATES, "markov_network.initial_gates", std::size_t);
    LIBEA_MD_DECL(MKV_GATE_TYPES, "markov_network.gate_types", std::string);

    namespace translators {
        using namespace mkv;
        enum gate_type { LOGIC=42, PROBABILISTIC=43, ADAPTIVE=44 };
        
        /*! Translator to build a Markov network from a circular genome.
         */
        class markov_network_translator {
        public:
            typedef std::set<gate_type> enabled_gate_set;
            
            //! Constructor.
            markov_network_translator(int in_lb=4, int in_ub=4, int out_lb=4, int out_ub=4,
                                      int h_lb=4, int h_ub=4, int wv_steps=1024)
            : _in_lb(in_lb), _in_ub(in_ub), _out_lb(out_lb), _out_ub(out_ub), _h_lb(h_lb), _h_ub(h_ub), _wv_steps(wv_steps) {
                _enabled.insert(LOGIC);
                _enabled.insert(PROBABILISTIC);
                _enabled.insert(ADAPTIVE);
            }
            
            template <typename MarkovNetwork, typename Genome>
            void translate_genome(MarkovNetwork& M, const Genome& g) {
                for(typename Genome::const_iterator f=g.begin(); f!=g.end(); ++f) {
                    if((*f + *(f+1)) == 255) {
                        translate_gene(f,M);
                    }
                }
            }
            
            //! Add the gene starting at f to Markov network N.
            template <typename ForwardIterator, typename MarkovNetwork>
            void translate_gene(ForwardIterator f, MarkovNetwork& N) const {
                if(!_enabled.count(static_cast<gate_type>(*f))) {
                    return;
                }
                switch(*f) {
                    case LOGIC: { // build a logic gate
                        parse_logic_gate(f+2, N);
                        break;
                    }
                    case PROBABILISTIC: { // build a markov gate
                        parse_probabilistic_gate(f+2, N);
                        break;
                    }
                    case ADAPTIVE: { // build an adaptive gate
                        parse_adaptive_gate(f+2, N);
                        break;
                    }
                    default: {
                        // do nothing; bogus start codon.
                    }
                }
            }
            
            //! Retrieves the set of enabled gate types.
            enabled_gate_set& enabled() { return _enabled; }
            
            //! Disable a gate type.
            void disable(gate_type g) {
                _enabled.erase(g);
            }
            
        protected:
            //! Parse the number and indices for a gate's IO vector.
            template <typename ForwardIterator, typename MarkovNetwork>
            ForwardIterator parse_io(ForwardIterator f, index_vector_type& inputs, index_vector_type& outputs, MarkovNetwork& N) const {
                using namespace ealib::algorithm;
                int nin=modnorm(*f++, _in_lb, _in_ub);
                int nout=modnorm(*f++, _out_lb, _out_ub);
                inputs.resize(nin);
                outputs.resize(nout);
                for(int i=0; i<nin; ++i,++f) {
                    inputs[i] = modnorm(static_cast<std::size_t>(*f), static_cast<std::size_t>(0), static_cast<std::size_t>(N.nstates()));
                }
                for(int i=0; i<nout; ++i,++f) {
                    outputs[i] = modnorm(static_cast<std::size_t>(*f), static_cast<std::size_t>(0), static_cast<std::size_t>(N.nstates()));
                }
                return f;
            }
            
            //! Parse a logic gate from f and add it to Markov network N.
            template <typename ForwardIterator, typename MarkovNetwork>
            void parse_logic_gate(ForwardIterator f, MarkovNetwork& N) const {
                typedef logic_gate<typename MarkovNetwork::rng_type> gate_type;
                boost::shared_ptr<gate_type> p(new gate_type());
                gate_type& g=*p;
                f = parse_io(f, g.inputs, g.outputs, N);
                g.M.resize(1<<g.inputs.size());
                for(std::size_t i=0; i<g.M.size(); ++i, ++f) {
                    g.M[i] = *f;
                }
                
                N.gates().push_back(p);
            }
            
            //! Parse a probabilistic gate from f and add it to Markov network N.
            template <typename ForwardIterator, typename MarkovNetwork>
            void parse_probabilistic_gate(ForwardIterator f, MarkovNetwork& N) const {
                typedef probabilistic_gate<typename MarkovNetwork::rng_type> gate_type;
                boost::shared_ptr<gate_type> p(new gate_type());
                gate_type& g=*p;
                f = parse_io(f, g.inputs, g.outputs, N);
                g.M.resize(1<<g.inputs.size(), 1<<g.outputs.size());
                for(std::size_t i=0; i<g.M.size1(); ++i) {
                    row_type row(g.M, i);
                    f = ealib::algorithm::normalize(f, f+g.M.size2(), row.begin(), 1.0);
                }
                
                N.gates().push_back(p);
            }
            
            //! Parse an adaptive gate from f and add it to Markov network N.
            template <typename ForwardIterator, typename MarkovNetwork>
            void parse_adaptive_gate(ForwardIterator f, MarkovNetwork& N) const {
                using namespace ealib::algorithm;
                
                typedef adaptive_gate<typename MarkovNetwork::rng_type> gate_type;
                boost::shared_ptr<gate_type> p(new gate_type());
                gate_type& g=*p;
                
                int nin=2+modnorm(*f++, _in_lb, _in_ub); // +2 for positive and negative feedback
                int nout=modnorm(*f++, _out_lb, _out_ub);
                g.inputs.resize(nin);
                g.outputs.resize(nout);
                for(int i=0; i<nin; ++i,++f) {
                    g.inputs[i] = modnorm(static_cast<std::size_t>(*f), static_cast<std::size_t>(0), static_cast<std::size_t>(N.nstates()));
                }
                for(int i=0; i<nout; ++i,++f) {
                    g.outputs[i] = modnorm(static_cast<std::size_t>(*f), static_cast<std::size_t>(0), static_cast<std::size_t>(N.nstates()));
                }
                
                g.h = modnorm(*f++, _h_lb, _h_ub);
                g.P.resize(g.h);
                for(std::size_t i=0; i<g.h; ++i,++f) {
                    g.P[i] = (*f % _wv_steps) * (1.0 / static_cast<double>(_wv_steps));
                }
                g.N.resize(g.h);
                for(std::size_t i=0; i<g.h; ++i,++f) {
                    g.N[i] = (*f % _wv_steps) * (1.0 / static_cast<double>(_wv_steps));
                }
                g.M.resize(1<<nin, 1<<nout);
                for(std::size_t i=0; i<g.M.size1(); ++i) {
                    row_type row(g.M, i);
                    f = ealib::algorithm::normalize(f, f+g.M.size2(), row.begin(), 1.0);
                }
                g.Q = g.M;
                N.gates().push_back(p);
            }
            
            int _in_lb, _in_ub, _out_lb, _out_ub; //!< Fan-in and fan-out lower and upper bounds.
            int _h_lb, _h_ub, _wv_steps; //!< History vector lower and upper bounds.
            enabled_gate_set _enabled; //!< Enabled gates.
        };
        
        //! Call the Markov network translator.
        struct call_markov_network_translator {
            template <typename EA>
            call_markov_network_translator(EA& ea) {
            }
            
            //! Translate the given genome into an L-System.
            template <typename Genome, typename Phenotype, typename EA>
            void operator()(Genome& G, Phenotype& P, EA& ea) {
                P.resize(get<MKV_INPUT_N>(ea),
                         get<MKV_OUTPUT_N>(ea),
                         get<MKV_HIDDEN_N>(ea));
                ea.lifecycle().translator.translate_genome(P,G);
            }
        };
        
    } // translators
    
    namespace ancestors {
        
        //! Generates random Markov network-based individuals.
        struct markov_network_ancestor {
            template <typename EA>
            typename EA::genome_type operator()(EA& ea) {
                typename EA::genome_type repr;
                repr.resize(get<REPRESENTATION_INITIAL_SIZE>(ea), 127);
                
                for(std::size_t i=0; i<get<MKV_INITIAL_GATES>(ea); ++i) {
                    std::size_t csize=ea.rng()(get<MUTATION_INDEL_MIN_SIZE>(ea),
                                               get<MUTATION_INDEL_MAX_SIZE>(ea));
                    int j=ea.rng()(repr.size()-csize);
                    translators::markov_network_translator& t = ea.lifecycle().translator;
                    int gate=*ea.rng().choice(t.enabled().begin(), t.enabled().end());
                    repr[j] = gate;
                    repr[j+1] = 255-gate;
                    for(std::size_t k=2; k<csize; ++k) {
                        repr[j+k]=ea.rng()(get<MUTATION_UNIFORM_INT_MIN>(ea), get<MUTATION_UNIFORM_INT_MAX>(ea));
                    }
                }
                return repr;
            }
        };
        
    } // ancestors
    
    namespace lifecycles {
        
        /*! Default lifecycle object for EAs that use Markov Networks.
         */
        struct markov_network_lifecycle : ealib::default_lifecycle {
            //! Called after EA initialization.
            template <typename EA>
            void initialize(EA& ea) {
                const std::string& gates = get<MKV_GATE_TYPES>(ea);
                if(!boost::algorithm::icontains(gates, "logic")) {
                    translator.disable(translators::LOGIC);
                }
                if(!boost::algorithm::icontains(gates, "probabilistic")) {
                    translator.disable(translators::PROBABILISTIC);
                }
                if(!boost::algorithm::icontains(gates, "adaptive")) {
                    translator.disable(translators::ADAPTIVE);
                }
            }
            
            ealib::translators::markov_network_translator translator; //!< Genome translator.
        };
        
    } // lifecycles
    
    //! Add the common Markov Network configuration options to the command line interface.
    template <typename EA>
    void add_mkv_options(cmdline_interface<EA>* ci) {
        using namespace ealib;
        // markov network options
        add_option<MKV_UPDATE_N>(ci);
        add_option<MKV_INPUT_N>(ci);
        add_option<MKV_OUTPUT_N>(ci);
        add_option<MKV_HIDDEN_N>(ci);
        add_option<MKV_INITIAL_GATES>(ci);
        add_option<MKV_GATE_TYPES>(ci);
        
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
    
	/*! Markov network evolutionary algorithm.
     
     This class specializes evolutionary_algorithm to provide an algorithm specific
     to evolving Markov networks.  If more advanced control over the features of
     the GA are needed, the reader is referred to evolutionary_algorithm.h.
	 */
	template
    < typename FitnessFunction
	, typename RecombinationOperator
	, typename GenerationalModel
    , typename StopCondition=dont_stop
    , typename PopulationGenerator=fill_population
    , typename Lifecycle=lifecycles::markov_network_lifecycle
    , template <typename> class Traits=fitness_trait
    > class markov_network_evolution
    : public evolutionary_algorithm
    < indirect<circular_genome<int>, mkv::markov_network< >, translators::call_markov_network_translator>
    , FitnessFunction
    , mutation::operators::indel<mutation::operators::per_site<mutation::site::uniform_integer> >
    , RecombinationOperator
    , GenerationalModel
    , ancestors::markov_network_ancestor
    , StopCondition
    , PopulationGenerator
    , Lifecycle
    , Traits
    > {
    };
    
} // ealib

#endif
