/* phenotype.h
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
#ifndef _EA_PHENOTYPE_H_
#define _EA_PHENOTYPE_H_

namespace ealib {
    
    /* The idea here is that some kinds of EAs require that an individual's genotype
     be converted to another form prior to fitness evaluation.  This is usually
     referred to as the "encoding type."
     
     Here we define two different encoding types: direct and indirect.
     */
    
    //! Indicates that the individual's genotype directly encodes the phenotype.
    struct directS { };
    
    /*! Indicates that the individaul's genotype indirectly encodes the phenotype
     (i.e., must be translated or generated prior to fitness evaluation).
     */
    struct indirectS { };
    
    /* Generative encodings are not yet defined, though they will likely be added
     later.
     
     These different encoding types are used when make_phenotype() is called
     on an individual.  Note that the default is directS.
     
     To be clear about definitions:
     
     Genotype: That which makes up the genetic component of an individual; inherited.
     In EALib, genotypes == representation.
     
     Phenotype: That component of an individual that is evaluated by the fitness
     function; can be the genotype (direct), an object that was translated
     from the genotype (indirect), or even an object that was generated
     (generative).
     */
    
    namespace traits {
        namespace detail {
            /* The below provide for two different pointer types, and correctly
             initialize them. */
            template <typename T, typename Encoding>
            struct phenotype_ptr {
            };
            
            template <typename T>
            struct phenotype_ptr<T, directS> {
                typedef typename T::phenotype_type* ptr_type;
            };
            
            template <typename T>
            struct phenotype_ptr<T, indirectS> {
                typedef boost::shared_ptr<typename T::phenotype_type> ptr_type;
            };
            
            template <typename T>
            void reset(T& t, directS) {
                t = 0;
            }
            
            template <typename T>
            void reset(T& t, indirectS) {
            }
        }
        
        
        /*! Phenotype trait for an individual.

         In the case of a direct encoding, phenotype_ptr is a plain-old pointer.
         Otherwise, it is a boost::shared_ptr< >.
         
         \note Phenotypes *ARE NOT* serializable - They are generated from their
         respective representation.
         */
        template <typename T>
        struct phenotype_trait {
            //! Phenotype pointer type.
            typedef typename detail::phenotype_ptr<T, typename T::encoding_type>::ptr_type phenotype_ptr_type;
            
            //! Constructor.
            phenotype_trait() {
                detail::reset(_p, typename T::encoding_type());
            }
            
            //! Returns true if a phenotype is present.
            bool has_phenotype() {
                return _p != 0;
            }
            
            //! Resets the phenotype pointer.
            void reset() {
                detail::reset(_p, typename T::encoding_type());
            }

            phenotype_ptr_type _p; //<! Pointer to this individual's phenotype.
        };
        
    } // traits
    
    namespace detail {
        
        //! Direct encoding; returns the individual's representation (its genotype).
        template <typename EA>
        typename EA::phenotype_type& phenotype(typename EA::individual_type& ind, directS, EA& ea) {
            return ind.repr();
        }
        
        //! Indirect encoding; returns a pointer to a phenotype that has been translated from the genotype.
        template <typename EA>
        typename EA::phenotype_type& phenotype(typename EA::individual_type& ind, indirectS, EA& ea) {
            if(!ind.traits().has_phenotype()) {
                typename EA::phenotype_ptr_type p = ind.traits().make_phenotype(ind,ea);
                ind.traits()._p = p;
            }
            return *ind.traits()._p;
        }
        
    } // detail
    
    //! Phenotype trait accessor; lazily decodes a genotype into a phenotype.
    template <typename EA>
    typename EA::phenotype_type& phenotype(typename EA::individual_type& ind, EA& ea) {
        return detail::phenotype(ind, typename EA::encoding_type(), ea);
    }
    
} // ealib

#endif
