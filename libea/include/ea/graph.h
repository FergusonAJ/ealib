/* graph.h
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
#ifndef _EA_GRAPH_H_
#define _EA_GRAPH_H_

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <vector>
#include <sstream>
#include <map>

#include <ea/algorithm.h>
#include <ea/metadata.h>
#include <ea/mutation.h>
#include <ea/rng.h>

namespace ealib {
    
    /* The following code provides a mechanism to evolve a graph G(E,V), where
     E is set of edges connecting the vertices in V.  Each e_ij \in E connects
     v_i -> v_j, where v_i and v_j \in V.
     
     The mutation operator on a graph is {\em delta growth}, a slight variant of
     Adami-Hintze graph growth~\cite{biology-direct}.
     */
    
    LIBEA_MD_DECL(GRAPH_EVENTS_N, "graph.events.n", int);
    LIBEA_MD_DECL(GRAPH_VERTEX_EVENT_P, "graph.vertex.event.p", double);
    LIBEA_MD_DECL(GRAPH_VERTEX_ADDITION_P, "graph.vertex.addition.p", double);
    LIBEA_MD_DECL(GRAPH_EDGE_EVENT_P, "graph.edge.event.p", double);
    LIBEA_MD_DECL(GRAPH_EDGE_ADDITION_P, "graph.edge.addition.p", double);
    LIBEA_MD_DECL(GRAPH_DUPLICATE_EVENT_P, "graph.duplicate.event.p", double);
    LIBEA_MD_DECL(GRAPH_DUPLICATE_VERTEX_P, "graph.duplicate.vertex.p", double);
    LIBEA_MD_DECL(GRAPH_MUTATION_EVENT_P, "graph.mutation.event.p", double);
    LIBEA_MD_DECL(GRAPH_MUTATION_VERTEX_P, "graph.mutation.vertex.p", double);
    
    namespace graph {
        
        //! Mix-in for mutable vertices.
        struct mutable_vertex {
            typedef int module_type;
            
            mutable_vertex() : module(0) { }
            
            template <typename EA>
            void mutate(EA& ea) {
            }
            
            template <class Archive>
            void serialize(Archive& ar, const unsigned int version) {
            }

            module_type module; //!< Module assignment for this vertex.
        };
        
        //! Mix-in for mutable edges.
        struct mutable_edge {
            template <typename EA>
            void mutate(EA& ea) {
            }

            template <class Archive>
            void serialize(Archive& ar, const unsigned int version) {
            }
        };
        
        //! Copy E_in(u) -> E_in(v).
        template <typename VertexDescriptor, typename Graph>
        void copy_in_edges(VertexDescriptor u, VertexDescriptor v, Graph& G) {
            typedef std::vector<std::pair<typename Graph::vertex_descriptor,
            typename Graph::edge_descriptor> > ve_list;
            ve_list adjacent;
            
            typename Graph::inv_adjacency_iterator iai,iai_end;
            for(boost::tie(iai,iai_end)=boost::inv_adjacent_vertices(u,G); iai!=iai_end; ++iai) {
                adjacent.push_back(std::make_pair(*iai, boost::edge(*iai,u,G).first));
            }
            for(typename ve_list::iterator i=adjacent.begin(); i!=adjacent.end(); ++i) {
                boost::add_edge(i->first, v, G[i->second], G);
            }
        }
        
        //! Copy E_out(u) -> E_out(v)
        template <typename VertexDescriptor, typename Graph>
        void copy_out_edges(VertexDescriptor u, VertexDescriptor v, Graph& G) {
            typedef std::vector<std::pair<typename Graph::vertex_descriptor,
            typename Graph::edge_descriptor> > ve_list;
            ve_list adjacent;
            
            typename Graph::adjacency_iterator ai,ai_end;
            for(boost::tie(ai,ai_end)=boost::adjacent_vertices(u,G); ai!=ai_end; ++ai) {
                adjacent.push_back(std::make_pair(*ai, boost::edge(u,*ai,G).first));
            }
            for(typename ve_list::iterator i=adjacent.begin(); i!=adjacent.end(); ++i) {
                boost::add_edge(v, i->first, G[i->second], G);
            }
        }
        
        /*! Independent probabilities:
         P_V is Node-event probability.
         P_E is Edge-event probability.
         P_D is Duplication-event probability.
         */
        namespace growth {
            enum probability { P_V=0, P_E, P_D };
        }
        
        /*! Conditional probabilities:
         p is Conditional node addition probability.
         q is Conditional edge addition probability.
         r is Conditional node duplication probability.
         */
        namespace conditional {
            enum probability { p=0, q, r };
        }
        
        /*! Holds information related to graph event probabilities and module
         assignments.
         */
        struct growth_descriptor {
            typedef std::vector<double> pr_sequence_type;
            typedef boost::numeric::ublas::matrix<double> assortativity_matrix_type;
            
            //! Default constructor.
            growth_descriptor() : Pe(3,0.0), Pc(3,0.0), Pm(1,1.0), M(1,1) {
                M(0,0) = 1.0;
            }
            
            growth_descriptor(double pv, double pe, double pd, double p, double q, double r)
            : Pe(3,0.0), Pc(3,0.0), Pm(1,1.0), M(1,1) {
                Pe[growth::P_V] = pv;
                Pe[growth::P_E] = pe;
                Pe[growth::P_D] = pd;
                Pc[conditional::p] = p;
                Pc[conditional::q] = q;
                Pc[conditional::r] = r;
                M(0,0) = 1.0;
            }
            
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version) {
                ar & BOOST_SERIALIZATION_NVP(Pe);
                ar & BOOST_SERIALIZATION_NVP(Pc);
                ar & BOOST_SERIALIZATION_NVP(Pm);
                ar & BOOST_SERIALIZATION_NVP(M);
            }
            
            pr_sequence_type Pe; //!< Event probabilities.
            pr_sequence_type Pc; //!< Conditional probabilities.
            pr_sequence_type Pm; //!< Module probabilities.
            assortativity_matrix_type M; //!< Module assortativity matrix.
        };
        
        //! Add a vertex, and select its color from an existing module.
        template <typename Graph, typename RNG>
        typename Graph::vertex_descriptor add_vertex(Graph& G, RNG& rng, const growth_descriptor& D=growth_descriptor()) {
            typename Graph::vertex_descriptor v=boost::add_vertex(G);
            G[v].module = algorithm::roulette_wheel(rng.p(),D.Pm.begin(), D.Pm.end()).first;
            return v;
        }
        
        //! Remove a randomly selected vertex.
        template <typename Graph, typename RNG>
        void remove_vertex(Graph& G, RNG& rng) {
            if(boost::num_vertices(G) == 0) {
                return;
            }
            
            typename Graph::vertex_descriptor u=boost::vertex(rng(boost::num_vertices(G)),G);
            boost::clear_vertex(u,G);
            boost::remove_vertex(u,G);
        }
        
        //! Add an edge between two distinct randomly selected vertices.
        template <typename Graph, typename RNG>
        std::pair<typename Graph::edge_descriptor,bool> add_edge(Graph& G, RNG& rng) {
            if(boost::num_vertices(G) <= 1) {
                return std::make_pair(typename Graph::edge_descriptor(),false);
            }
            
            std::size_t un,vn;
            boost::tie(un,vn) = rng.choose_two_ns(0, static_cast<int>(boost::num_vertices(G)));
            typename Graph::vertex_descriptor u=boost::vertex(un,G);
            typename Graph::vertex_descriptor v=boost::vertex(vn,G);
            return boost::add_edge(u,v,G);
        }
        
        //! Remove a randomly selected edge.
        template <typename Graph, typename RNG>
        void remove_edge(Graph& G, RNG& rng) {
            if(boost::num_edges(G) == 0) {
                return;
            }
            
            typename Graph::edge_iterator ei,ei_end;
            boost::tie(ei,ei_end) = boost::edges(G);
            ei = rng.choice(ei,ei_end);
            boost::remove_edge(*ei,G);
        }
        
        //! Duplicate a randomly selected vertex.
        template <typename Graph, typename RNG>
        void duplicate_vertex(Graph& G, RNG& rng) {
            if(boost::num_vertices(G) == 0) {
                return;
            }
            
            typename Graph::vertex_descriptor u=boost::vertex(rng(boost::num_vertices(G)),G);
            typename Graph::vertex_descriptor v=boost::add_vertex(G);
            G[v].module = G[u].module;
            copy_in_edges(u,v,G);
            copy_out_edges(u,v,G);
        }
        
        //! Merge two randomly selected vertices.
        template <typename Graph, typename RNG>
        void merge_vertices(Graph& G, RNG& rng) {
            if(boost::num_vertices(G) <= 1) {
                return;
            }
            
            std::size_t un,vn;
            boost::tie(un,vn) = rng.choose_two_ns(0, static_cast<int>(boost::num_vertices(G)));
            typename Graph::vertex_descriptor u=boost::vertex(un,G);
            typename Graph::vertex_descriptor v=boost::vertex(vn,G);
            copy_in_edges(v,u,G);
            copy_out_edges(v,u,G);
            boost::clear_vertex(v,G);
            boost::remove_vertex(v,G);
        }
        
        /*! Perform n growth events on graph G via the given growth descriptor.
         
         Vertices in G must have an internal "module" property.  Inheriting from
         mutable_vertex (see above) will suffice.  All vertices initially in G
         are assumed to have a valid module (0 is acceptable).
         */
        template <typename Graph, typename RNG>
        void delta_growth_n(Graph& G, int n, growth_descriptor& D, RNG& rng) {
            // sanity...
            assert(D.Pm.size() == D.M.size1());
            assert(D.Pm.size() == D.M.size2());
            assert(D.Pe.size() == 3);
            assert(D.Pc.size() == 3);
            
            // normalize event and module probabilities to 1.0:
            algorithm::normalize(D.Pe.begin(), D.Pe.end(), 1.0);
            algorithm::normalize(D.Pm.begin(), D.Pm.end(), 1.0);
            
            for( ; n>0; --n) {
                switch(algorithm::roulette_wheel(rng.p(), D.Pe.begin(), D.Pe.end()).first) {
                    case 0: {
                        if(rng.p(D.Pc[conditional::p])) {
                            add_vertex(G,rng,D);
                        } else {
                            remove_vertex(G,rng);
                        }
                        break;
                    }
                    case 1: {
                        if(rng.p(D.Pc[conditional::q])) {
                            add_edge(G,rng);
                        } else {
                            remove_edge(G,rng);
                        }
                        break;
                    }
                    case 2: {
                        if(rng.p(D.Pc[conditional::r])) {
                            duplicate_vertex(G,rng);
                        } else {
                            merge_vertices(G,rng);
                        }
                        break;
                    }
                    default: {
                        assert(false);
                    }
                }
            }
        }
        
        //! Convenience method to convert a Graph to a graphviz-compatible string (boost::write_graphviz doesn't work with EALib).
        template <typename Graph>
        std::string graph2string(Graph& G) {
            std::ostringstream out;
            
            out << "digraph {" << std::endl << "edge [ arrowsize=0.75 ];" << std::endl;
            
            for(std::size_t i=0; i<boost::num_vertices(G); ++i) {
                out << boost::vertex(i,G) << ";" << std::endl;
            }
            
            typename Graph::edge_iterator ei,ei_end;
            for(boost::tie(ei,ei_end)=boost::edges(G); ei!=ei_end; ++ei) {
                out << boost::source(*ei,G) << "->" << boost::target(*ei,G) << ";" << std::endl;
            }
            
            out << "}" << std::endl;
            return out.str();
        }
    } // graph
    
    namespace mutation {
        namespace operators {
            
            /*! Mutate a graph via the delta growth function.
             
             This mutation operator performs one graph growth event, and may then
             attempt to mutate either a vertex or edge.
             
             Vertices and edges must both provide a mutate operator.  See
             graph::mutable_vertex and graph::mutable_edge.
             
             \warning The graph mutation types described here allow self-loops and
             do not explicitly prevent parallel edges (though careful selection of
             the underlying graph type can do so).
             */
            struct delta_growth {
                template <typename EA>
                void operator()(typename EA::genome_type& G, EA& ea) {
                    graph::growth_descriptor D(get<GRAPH_VERTEX_EVENT_P>(ea),
                                               get<GRAPH_EDGE_EVENT_P>(ea),
                                               get<GRAPH_DUPLICATE_EVENT_P>(ea),
                                               get<GRAPH_VERTEX_ADDITION_P>(ea),
                                               get<GRAPH_EDGE_ADDITION_P>(ea),
                                               get<GRAPH_DUPLICATE_VERTEX_P>(ea));
                    
                    graph::delta_growth_n(G, 1, D, ea.rng());
                    
                    if(ea.rng().p(get<GRAPH_MUTATION_EVENT_P>(ea))) {
                        if(ea.rng().p(get<GRAPH_MUTATION_VERTEX_P>(ea))) {
                            mutate_vertex(G,ea);
                        } else {
                            mutate_edge(G,ea);
                        }
                    }
                }
                
                template <typename EA>
                void operator()(typename EA::individual_type& ind, EA& ea) {
                    operator()(ind.genome(), ea);
                }
                
                //! Mutate a randomonly selected vertex.
                template <typename Graph, typename EA>
                void mutate_vertex(Graph& G, EA& ea) {
                    if(boost::num_vertices(G) == 0) {
                        return;
                    }
                    
                    typename Graph::vertex_descriptor u=boost::vertex(ea.rng()(boost::num_vertices(G)),G);
                    G[u].mutate(ea);
                }
                
                //! Mutate a randomonly selected edge.
                template <typename Graph, typename EA>
                void mutate_edge(Graph& G, EA& ea) {
                    if(boost::num_edges(G) == 0) {
                        return;
                    }
                    
                    typename Graph::edge_iterator ei,ei_end;
                    boost::tie(ei,ei_end) = boost::edges(G);
                    ei = ea.rng().choice(ei,ei_end);
                    G[*ei].mutate(ea);
                }
            };
            
        } // operators
    } // mutation
    
    namespace ancestors {
        
        /*! Generates a random graph representation based on calling the graph
         mutation operator GRAPH_EVENTS_N times.
         */
        struct random_delta_graph {
            template <typename EA>
            typename EA::genome_type operator()(EA& ea) {
                typename EA::genome_type G;
                mutation::operators::delta_growth gm;
                for(std::size_t i=0; i<get<GRAPH_EVENTS_N>(ea); ++i) {
                    gm(G,ea);
                }
                return G;
            }
        };
        
    } // ancestors
    
    
    /* The following code defines a "developmental template" for graphs.  Here,
     instead of evolving a graph G(V,E) directly, we evolve a graph D(M,L) and
     use it to construct an instance of G.  Specifically:
     
     G(V,E) = \phi(n, D(M,L)),
     
     where n is the desired number of vertices in G and \phi is a function that
     constructs G(V,E) from D(M,L).
     */
    namespace graph {
        
        /*! Vertex in a developmental network.
         
         Each vertex corresponds to a module m \in M of D(M,L).
         */
        struct developmental_vertex : graph::mutable_vertex {
            developmental_vertex(double w=0.0, double m=0.0, double v=0.0)
            : weight(w), degree_mean(m), degree_var(v) {
            }
            
            template <typename EA>
            void mutate(EA& ea) {
            }
            
            double weight; //!< Relative weight of this vertex.
            double degree_mean; //!< Mean degree of neurons belonging to this vertex.
            double degree_var; //!< Variance of the degree of neurons belonging to this vertex.
            
            // while per-module learning rates are appealing, best leave this out for now:
            //double learning_rate; //!< Learning rate for neurons belonging to this vertex.
            //double decay_rate; //!< Decay rate for neurons belonging to this vertex.
        };
        
        /*! Edge in a developmental network.
         
         Each edge corresponds to a link l \in L of D(M,L).
         */
        struct developmental_edge : graph::mutable_edge {
            developmental_edge(double w=0.0) : weight(w) {
            }
            
            template <typename EA>
            void mutate(EA& ea) {
            }
            
            double weight; //!< Relative weight of this edge.
        };
        
        //! Convenience typedef for a typical Developmental network, D(M,L).
        typedef boost::adjacency_list
        <boost::setS
        , boost::vecS
        , boost::bidirectionalS
        , developmental_vertex
        , developmental_edge
        > developmental_graph;
        
        /*! \phi, which produces a graph from a developmental template.
         
         Specifically, G(V,E) = \phi(n, D(M,L)).
         */
        template <typename Graph, typename DevGraph, typename RNG>
        void phi(Graph& G, std::size_t n, DevGraph& D, RNG& rng) {
            // calculate the normalized module weights in D:
            typedef std::map<typename DevGraph::vertex_descriptor, double> module_weights;
            module_weights W;
            {
                typename DevGraph::vertex_iterator vi, vi_end;
                for(boost::tie(vi,vi_end)=boost::vertices(D); vi!=vi_end; ++vi) {
                    W[*vi] = D[*vi].weight;
                }
            }
            // algorithm::normalize(M.begin(), M.end(), 1.0);
            
            // track existing vertex module assignments in G:
            typedef std::set<typename Graph::vertex_descriptor> vertex_set;
            typedef std::map<typename DevGraph::vertex_descriptor, vertex_set> module_assignment;
            module_assignment M_extant, M_new;
            {
                typename Graph::vertex_iterator vi,vi_end;
                for(boost::tie(vi,vi_end)=boost::vertices(G); vi!=vi_end; ++vi) {
                    M_extant[boost::vertex(G[*vi].module, D)].insert(*vi);
                }
            }
            
            // allocate n new vertices in G, and assign them to modules:
            int module=0;
            for(typename module_weights::iterator i=W.begin(); i!=W.end(); ++i, ++module) {
                // this can be replaced by a roulette wheel...
                for(std::size_t j=0; j<static_cast<std::size_t>(n*i->second); ++j) {
                    typename Graph::vertex_descriptor u=boost::add_vertex(G);
                    G[u].module = module;
                    M_new[i->first].insert(u);
                    M_extant[i->first].insert(u);
                }
            }
            
            // link up the new vertices in G:
            for(typename module_assignment::iterator i=M_new.begin(); i!=M_new.end(); ++i) {
                for(typename vertex_set::iterator j=i->second.begin(); j!=i->second.end(); ++j) {
                    // source vertex:
                    typename Graph::vertex_descriptor u=*j;
                    
                    // possible other modules are based on the links of m in D;
                    typename DevGraph::vertex_descriptor m=i->first;
                    typename DevGraph::out_edge_iterator ei,ei_end;
                    for(boost::tie(ei,ei_end)=boost::out_edges(m,D); ei!=ei_end; ++ei) {
                        if(rng.p(D[*ei].weight)) {
                            // other module:
                            vertex_set& n=M_extant[boost::target(*ei,D)];
                            typename Graph::vertex_descriptor v=*rng.choice(n.begin(), n.end());
                            boost::add_edge(u,v,G);
                        }
                    }
                }
            }
        }

    } // graph

} // ea


// the below chunks of code are for manipulating growth descriptors;
// but there's a bit of work to get this all hooked up.
//
//            /*! Mutate a graph growth descriptor.
//             */
//            struct growth_descriptor_mutator {
//                typedef mutation::operators::per_site<mutation::site::uniform_real> sequence_mutator_type;
//
//                template <typename Representation, typename EA>
//                void operator()(Representation& repr, EA& ea) {
//                    sequence_mutator_type sm;
//
//                    sm(repr.Pe, ea);
//                    sm(repr.Pc, ea);
//                    sm(repr.Pm, ea);
//                    sm(repr.M.data(), ea);
//
//                    // P of changing # of modules is persite / #modules
//                    if(ea.rng().p(get<MUTATION_PER_SITE_P>(ea)/static_cast<double>(repr.Pm.size()))) {
//                        if(ea.rng().bit()) {
//                            // increase
//                            repr.Pm.push_back(ea.rng().p());
//                            repr.M.resize(repr.M.size1()+1, repr.M.size2()+1, true);
//                            for(std::size_t i=0; i<repr.M.size1(); ++i) {
//                                repr.M(i,repr.M.size2()-1) = ea.rng().p();
//                            }
//                            for(std::size_t j=0; j<repr.M.size2(); ++j) {
//                                repr.M(repr.M.size1()-1,j) = ea.rng().p();
//                            }
//                        } else if(repr.Pm.size() > 1) {
//                            // decrease
//                            repr.Pm.pop_back();
//                            repr.M.resize(repr.M.size1()-1, repr.M.size2()-1, true);
//                        }
//                    }
//                }
//            };
//
///*! Generates a random growth descriptor for a single module graph.
// */
//struct random_growth_descriptor {
//    template <typename EA>
//    typename EA::representation_type operator()(EA& ea) {
//        typename EA::representation_type repr;
//
//        std::generate(repr.Pe.begin(), repr.Pe.end(), probability_generator<typename EA::rng_type>(ea.rng()));
//        std::generate(repr.Pc.begin(), repr.Pc.end(), probability_generator<typename EA::rng_type>(ea.rng()));
//
//        // we rely on default construction of the growth descriptor to be sane;
//        // ie, one module, 100% likelihood of edges connecting.
//        return repr;
//    }
//};

#endif
