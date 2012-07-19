/* artificial_life/artificial_life.h 
 * 
 * This file is part of EALib.
 * 
 * Copyright 2012 David B. Knoester, Heather J. Goldsby.
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

#ifndef _EA_REPLICATION_H_
#define _EA_REPLICATION_H_

#include <boost/serialization/nvp.hpp>
#include <boost/tuple/tuple.hpp>
#include <ea/events.h>
#include <ea/interface.h>

namespace ea {
    
    /*! Selects the location of the first neighbor to the parent as the location
     for an offspring.
     
     This works well when combined with the well_mixed topology.  In this case, 
     the net effect is ~mass action.
     */
    struct first_neighbor {
        template <typename EA>
        std::pair<typename EA::topology_type::iterator, bool> operator()(typename EA::individual_ptr_type& parent, EA& ea) {
            return std::make_pair(ea.topo().neighborhood(parent,ea).first, true);
        }
    };
    
    
    /*! (Re-)Place an offspring in the population, if possible.
     */
    template <typename EA>
    void replace(typename EA::individual_ptr_type parent, typename EA::individual_ptr_type offspring, EA& ea) {
        // is the topology at capacity (meaning that we have to replace someone),
        // or are we growing the population?
        if(ea.topo().size() >= get<POPULATION_SIZE>(ea)) {
            // replace
            typename EA::replacement_type r;
            std::pair<typename EA::topology_type::iterator, bool> l=r(parent, ea);
            
            if(l.second) {
                ea.topo().replace(l.first, offspring, ea);
                offspring->priority() = parent->priority();
                ea.population().append(offspring);
            }
        } else {
            // grow
            ea.topo().place(offspring);
            offspring->priority() = parent->priority();
            ea.population().append(offspring);
        }
    }
    
    
    /*! Replicates a parent p to produce an offspring with representation r.
     */
    template <typename EA>
    void replicate(typename EA::individual_ptr_type p, typename EA::representation_type& r, EA& ea) {
        typename EA::population_type parents, offspring;
        parents.append(p);
        offspring.append(make_population_entry(typename EA::individual_type(r),ea));
        
        mutate(offspring.begin(), offspring.end(), ea);
        inherits(parents, offspring, ea);
        
        // parent is always reprioritized...
        ea.tasklib().prioritize(*p,ea);
        
        replace(*parents.begin(), *offspring.begin(), ea);
    }
    
} // ea

#endif
