/* turtle.h
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
#ifndef _EA_LSYS_TURTLE_H_
#define _EA_LSYS_TURTLE_H_

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <deque>

#include <ea/lsys/lsystem.h>
#include <ea/mutation.h>
#include <ea/representation.h>
namespace bnu = boost::numeric::ublas;

namespace ealib {

    LIBEA_MD_DECL(TURTLE_MAX_DRAWS, "ea.turtle.max_draws", int);

    namespace lsys {
        
        /*! Context for 2D turtles.
         */
        struct turtle_context2 {
            typedef bnu::vector<double> vector_type;
            typedef vector_type point_type;
            typedef bnu::matrix<double> rotation_matrix_type;
            
            //! Default constructor.
            turtle_context2() : _mag(1.0), _scale(1.0) {
                origin(0.0, 0.0);
                heading(1.0, 0.0);
                angle(90.0);
            }
            
            //! Sets the magnitude of the distance traveled during a single step.
            turtle_context2& step_magnitude(double d) {
                _mag = d;
                return *this;
            }
            
            //! Set the depth scaling factor.
            turtle_context2& scaling_factor(double d) {
                _scale = d;
                return *this;
            }
            
            //! Sets the initial origin of this context.
            turtle_context2& origin(double x, double y) {
                _p.resize(2);
                _p(0) = x; _p(1) = y;
                return *this;
            }
            
            //! Sets the initial heading of this context.
            turtle_context2& heading(double x, double y) {
                _h.resize(2);
                _h(0) = x; _h(1) = y;
                _h = _h / bnu::norm_2(_h);
                return *this;
            }
            
            //! Sets the angle for both cw (-d) and ccw (+d) rotations.
            turtle_context2& angle(double d) {
                double theta=d*boost::math::constants::pi<double>() / 180.0;
                rotation_matrix(_Rccw, theta);
                rotation_matrix(_Rcw, -theta);
                return *this;
            }

            //! Sets the given matrix to rotate by theta radians.
            void rotation_matrix(rotation_matrix_type& R, double theta) {
                R.resize(2,2);
                R(0,0) = R(1,1) = cos(theta);
                R(0,1) = -sin(theta);
                R(1,0) = -R(0,1);
            }
            
            //! Rotates this context by R.
            void rotate(const rotation_matrix_type& R) {
                _h = bnu::prod(R, _h);
            }
            
            //! Rotate ccw n times.
            void rotate_ccw(std::size_t n=1) {
                for( ; n>0; --n) {
                    rotate(_Rccw);
                }
            }
            
            //! Rotate cw n times.
            void rotate_cw(std::size_t n=1) {
                for( ; n>0; --n) {
                    rotate(_Rcw);
                }
            }
            
            /*! Move x steps from the current position in the direction of the
             current heading, scaled by the given depth d.
             */
            void step(double x, double d=1.0) {
                _p += x * _mag * pow(_scale,d) * _h;
            }
            
            //! Returns the current position of this context.
            point_type point() {
                return _p;
            }

            double _mag; //!< Step magnitude.
            double _scale; //!< Depth scaling factor.
            vector_type _p; //!< Current position.
            vector_type _h; //!< Current heading.
            rotation_matrix_type _Rccw, _Rcw; //! CCW and CW rotation matrices.
        };
        
        
        //! Tag to select drawing lines.
        struct lineS { };
        
        //! Tag to select drawing points.
        struct pointS { };
        
        //! Default turtle symbols.
        enum turtle2_symbol { SYM_BEGIN=0, SYM_F=0, SYM_G, SYM_PLUS, SYM_MINUS, SYM_LBRACKET, SYM_RBRACKET, SYM_PIPE, SYM_END };
        
        /*! 2D turtle for an L-system.
         */
        template
        < typename CoordinateSystem
        , typename LineSelector=lineS
        , typename LSystem=lsystem<int>
        > class lsystem_turtle2 : public LSystem {
        public:
            typedef LSystem parent;
            typedef CoordinateSystem coor_system_type;
            typedef turtle_context2 context_type;
            typedef turtle_context2::point_type point_type;
            typedef std::deque<context_type> context_stack_type;
            typedef std::deque<int> param_stack_type;
            typedef LineSelector line_selector_tag;
            
            //! Constructor.
            lsystem_turtle2() {
                parent::symbol(SYM_F)
                .symbol(SYM_G)
                .symbol(SYM_PLUS)
                .symbol(SYM_MINUS)
                .symbol(SYM_LBRACKET)
                .symbol(SYM_RBRACKET)
                .symbol(SYM_PIPE);
            }
            
            //! Clears the current drawing context.
            void clear() {
                _cstack.clear();
                _pstack.clear();
            }
            
            //! Returns the initial drawing context.
            context_type& context() { return _initial; }
            
            //! Draw string s into the given coordinate system.
            void draw(coor_system_type& coor, const typename parent::string_type& s, unsigned int max_draws=0) {
                clear();
                _cstack.push_back(_initial);
                unsigned int c=0;
                for(typename parent::string_type::const_iterator i=s.begin(); (i!=s.end()) && ((max_draws == 0) || (c < max_draws)); ++i, ++c) {
                    switch(*i) {
                        case SYM_F: line(coor); break;
                        case SYM_G: fwd(coor); break;
                        case SYM_PLUS: ccw(coor); break;
                        case SYM_MINUS: cw(coor); break;
                        case SYM_LBRACKET: push(); break;
                        case SYM_RBRACKET: pop(); break;
                        case SYM_PIPE: scaled_line(coor); break;
                        default: {
                            // if we recognize the symbol, it's a variable that
                            // should be ignored.  otherwise, it's a parameter:
                            if(parent::_V.find(*i) != parent::_V.end()) {
                                // turning off parameters for now...
                                // _pstack.push_back(*i);
                            }
                            break;
                        }
                    }
                }
            }
            
            //! Execute the L-system to a depth of n, and draw its output into coor.
            void draw(coor_system_type& coor, std::size_t n, unsigned int max_draws=0) {
                draw(coor, parent::exec_n(n), max_draws);
            }
            
        protected:
            //! Returns the top value from the parameter stack, or 1 if none.
            inline int param() {
                int n=1;
                if(!_pstack.empty()) {
                    n = _pstack.back();
                    _pstack.pop_back();
                }
                return n;
            }
            
            //! Returns the current context.
            inline context_type& current_context() {
                assert(!_cstack.empty());
                return _cstack.back();
            }
            
            //! Draw a line.
            void draw(coor_system_type& coor, const context_type::point_type& p1, const context_type::point_type& p2, lineS) {
                coor.line(p1, p2);
            }
            
            //! Draw a point.
            void draw(coor_system_type& coor, const context_type::point_type& p1, const context_type::point_type& p2, pointS) {
                coor.point(p2);
            }
            
            //! Draw a line from the current position to a single step.
            void line(coor_system_type& coor) {
                context_type& c=current_context();
                context_type::point_type p1=c.point();
                c.step(param());
                context_type::point_type p2=c.point();
                draw(coor, p1, p2, line_selector_tag());
            }
            
            //! Draw a line scaled by the current recursion depth.
            void scaled_line(coor_system_type& coor) {
                context_type& c=current_context();
                context_type::point_type p1=c.point();
                c.step(param(), static_cast<double>(_cstack.size()));
                context_type::point_type p2=c.point();
                draw(coor, p1, p2, line_selector_tag());
            }
            
            //! Move forward.
            void fwd(coor_system_type& coor) {
                current_context().step(param());
            }
            
            //! Rotate counter-clockwise.
            void ccw(coor_system_type& coor) {
                current_context().rotate_ccw(param());
            }
            
            //! Rotate clockwise.
            void cw(coor_system_type& coor) {
                current_context().rotate_cw(param());
            }
            
            //! Push the current context.
            void push() {
                _cstack.push_back(_cstack.back());
            }
            
            //! Pop the current context.
            void pop() {
                if(_cstack.size() > 1) {
                    _cstack.pop_back();
                }
            }
            
            context_type _initial; //!< Initial context.
            context_stack_type _cstack; //!< Stack for context.
            param_stack_type _pstack; //!< Stack for parameters.
        };
        
    } // lsys
    
    
	namespace ancestors {

		//! Generates a random 2D LSystem turtle ancestor.
		struct random_turtle2 {
			template <typename EA>
			typename EA::genome_type operator()(EA& ea) {
                using namespace ealib::lsys;
                typename EA::genome_type g;
                g.resize(get<REPRESENTATION_INITIAL_SIZE>(ea), 127);
                
                // add the symbols:
                for(std::size_t i=SYM_BEGIN; i<SYM_END; ++i) {
                    int j=ea.rng()(g.size());
                    g[j] = translators::lsystem::SYMBOL;
                    g[++j] = 255 - translators::lsystem::SYMBOL;
                    g[++j] = i;
                }
                
                // add the axiom (a couple times for redundancy):
                for(std::size_t i=0; i<4; ++i) {
                    int j=ea.rng()(g.size());
                    g[j] = translators::lsystem::AXIOM;
                    g[++j] = 255 - translators::lsystem::AXIOM;
                    g[++j] = SYM_F;
                }
                
                // add a few random rules:
                for(std::size_t i=0; i<get<LSYS_INITIAL_RULES>(ea); ++i) {
                    int j=ea.rng()(g.size());
                    g[j] = translators::lsystem::RULE;
                    g[++j] = 255 - translators::lsystem::RULE;
                    std::size_t rsize=ea.rng()(get<MUTATION_INDEL_MIN_SIZE>(ea),
                                               get<MUTATION_INDEL_MAX_SIZE>(ea));
                    g[++j] = rsize;
                    for(std::size_t k=0; k<rsize; ++k) {
                        g[++j] = ea.rng()(SYM_BEGIN, SYM_END);
                    }
                }
                return g;
			}
		};
		
	} // ancestors
} // ealib

#endif
