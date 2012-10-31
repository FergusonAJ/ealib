/* attributes.h
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
#ifndef _EA_ATTRIBUTES_H_
#define _EA_ATTRIBUTES_H_

#include <boost/serialization/nvp.hpp>
#include <ea/meta_data.h>

namespace ea {
    
    //! Fitness attribute.
    template <typename EA>
    struct fitness_attribute {
        typedef typename EA::fitness_type fitness_type;
        
        fitness_type& fitness() { return _v; }
        
        template <class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::make_nvp("fitness_attr", _v);
        }
        
        fitness_type _v;
    };
    
    //! Fitness accessor method.
    template <typename T>
    typename T::attr_type::fitness_type& fitness(T& t) {
        return t.attr().fitness();
    }
    
    namespace access {
        
        //! Fitness accessor functor.
        struct fitness {
            template <typename T>
            typename T::attr_type::fitness_type& operator()(T& t) {
                return t.attr().fitness();
            }
        };
        
        //! Accessor for an individual's meta-data.
        template <typename MDType>
        struct meta_data {
            template <typename Individual>
            typename MDType::value_type operator()(Individual& x) {
                return ea::get<MDType>(x);
            }
        };
        
    } // access
    
} // ea

#endif
