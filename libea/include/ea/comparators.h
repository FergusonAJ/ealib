/* comparators.h
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
#ifndef _EA_COMPARATORS_H_
#define _EA_COMPARATORS_H_

#include <ea/traits.h>
#include <ea/fitness_function.h>

namespace ealib {
    namespace comparators {
        
        //! Compare (pointers to) individuals based on the natural order of their fitnesses in ascending order.
        template <typename EA>
        struct fitness {
            //! Constructor.
            fitness(EA& ea) : _ea(ea) {
            }
            
            //! Returns true if fitness(x) < fitness(y), false otherwise.
            bool operator()(typename EA::individual_ptr_type x, typename EA::individual_ptr_type y) {
                return ealib::fitness(*x,_ea) < ealib::fitness(*y,_ea);
            }

            //! Returns true if fitness(x) < fitness(y), false otherwise.
            bool operator()(typename EA::individual_type& x, typename EA::individual_type& y) {
                return ealib::fitness(x,_ea) < ealib::fitness(y,_ea);
            }

            EA& _ea; //!< Reference to the EA in which the individuals to be compared reside.
        };
        
        //! Compare (pointers to) individuals based on the natural order of meta-data in ascending order.
        template <typename MDType, typename EA>
        struct metadata {
            //! Constructor.
            metadata() {
            }
            
            //! Returns true if metadata(x) < metadata(y), false otherwise.
            bool operator()(typename EA::individual_ptr_type x, typename EA::individual_ptr_type y) {
                return get<MDType>(*x) < get<MDType>(*y);
            }
        };
        
        //! Compare individuals based on the natural order of their i'th objective.
        template <typename EA>
        struct objective {
            //! Constructor.
            objective(std::size_t i, EA& ea) : _i(i), _ea(ea) {
            }
            
            //! Returns true if objective(i) of x < objective(i) of y, false otherwise.
            bool operator()(typename EA::individual_ptr_type x, typename EA::individual_ptr_type y) {
                return ealib::fitness(*x,_ea)[_i] < ealib::fitness(*y,_ea)[_i];
            }
            
            //! Returns true if objective(i,x) < objective(i,y), false otherwise.
            bool operator()(typename EA::individual_type& x, typename EA::individual_type& y) {
                return ealib::fitness(x,_ea)[_i] < ealib::fitness(y,_ea)[_i];
            }

            std::size_t _i; //!< Index of the objective that individuals will be compared by.
            EA& _ea; //!< Reference to the EA in which the individuals to be compared reside.
        };
        
        //! Compare individuals based on the natural order of their i'th objective.
        template <typename AttributeAccessor, typename EA>
        struct attribute {
            //! Constructor.
            attribute(EA& ea) : _ea(ea) {
            }
            
            //! Returns true if attr(x) < attr(y), false otherwise.
            bool operator()(typename EA::individual_ptr_type x, typename EA::individual_ptr_type y) {
                return _acc(*x,_ea) < _acc(*y,_ea);
            }
            
            EA& _ea; //!< Reference to the EA in which the individuals to be compared reside.
            AttributeAccessor _acc; //!< Accessor for the compared attribute.
        };
    } // comparators
} // ea

#endif
