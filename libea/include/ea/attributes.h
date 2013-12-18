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
#include <ea/fitness_function.h>
#include <ea/phenotype.h>
#include <ea/line_of_descent.h>

namespace ealib {
    namespace attr {

        /*! Default attributes for individuals in an evolutionary algorithm.
         
         The default attributes include a bit more than what might be strictly necessary,
         just to limit the number of hoops that need to be jumped through when doing
         fairly common things.  I.e., they include fitness, phenotype, and line of
         descent attributes.  Of these, only fitness is currently serialized.  Note
         that this has the ramification that we do NOT save LoD information across
         checkpoints.
         */
        template <typename EA>
        struct default_attributes : attr::fitness_attribute<EA>, attr::phenotype_attribute<EA>, attr::lod_attributes<EA> {
            template <class Archive>
            void serialize(Archive& ar, const unsigned int version) {
                ar & boost::serialization::make_nvp("fitness_attr", boost::serialization::base_object<attr::fitness_attribute<EA> >(*this));
            }
        };
        
        template <typename EA>
        struct no_attributes {
            template <class Archive>
            void serialize(Archive& ar, const unsigned int version) {
            }
        };
        
    } // attr
    
    namespace access {
        
        //! Accessor for all of an individual's attributes.
        struct attributes {
            template <typename EA>
            typename EA::individual_attr_type operator()(typename EA::individual_type& ind, EA& ea) {
                return ind.attr();
            }
        };

        //! Fitness accessor functor.
        struct fitness {
            template <typename EA>
            typename EA::individual_type::attr_type::fitness_type& operator()(typename EA::individual_type& ind, EA& ea) {
                return ealib::fitness(ind,ea);
            }
        };
        
        //! Accessor for an individual's meta-data.
        template <typename MDType>
        struct meta_data {
            template <typename EA>
            typename MDType::value_type operator()(typename EA::individual_type& ind, EA& ea) {
                return ealib::get<MDType>(ind);
            }
        };
        
    } // access
} // ea

#endif
