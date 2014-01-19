/* digital_evolution/digital_evolution.h 
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

#ifndef _EA_ORGANISM_H_
#define _EA_ORGANISM_H_

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <fstream>
#include <deque>
#include <vector>
#include <map>

#include <ea/digital_evolution/position.h>
#include <ea/digital_evolution/hardware.h>
#include <ea/digital_evolution/schedulers.h>
#include <ea/meta_data.h>

namespace ealib {
	    
	/*! Definition of an organism.
	 
	 Organisms within ealib are four things:
	 1) A container for a representation
     2) A container for virtual hardware
	 3) A container for a priority
	 4) A container for meta data
	 */
    class organism {
	public:
        //! Individual pointer type.
        typedef boost::shared_ptr<organism> individual_ptr_type;
        //! Type of hardware operating this organism.
        typedef hardware hardware_type;
        //! Type of representation used by the hardware.
        typedef hardware_type::representation_type representation_type;
        //! Type of mutation needed by the representation.
        typedef hardware_type::mutation_operator_type mutation_operator_type;

        typedef int io_type; //!< Type for input and output values.
        typedef std::deque<io_type> iobuffer_type; //!< Type for buffering inputs and outputs.
		typedef std::map<std::string, double> phenotype_map_type; //!< Type for storing phenotype information.

		//! Constructor.
		organism() : _name(0), _generation(0.0), _update(0), _alive(true), _priority(1.0), _tracecb(0) {
		}
        
		//! Constructor that builds an organism from a representation.
		organism(const representation_type& r) 
        : _name(0), _generation(0.0), _update(0), _alive(true), _priority(1.0), _hw(r), _tracecb(0) {
		}
        
        //! Copy constructor.
        organism(const organism& that) {
            _name = that._name;
            _generation = that._generation;
            _update = that._update;
            _alive = that._alive;
            _priority = that._priority;
            _hw = that._hw;
            _md = that._md;
            _inputs = that._inputs;
            _outputs = that._outputs;
            _phenotype = that._phenotype;
            _position = that._position;
            _tracecb = 0;
        }
        
        //! Assignment operator.
        organism& operator=(const organism& that) {
            if(this != &that) {
                _name = that._name;
                _generation = that._generation;
                _update = that._update;
                _alive = that._alive;
                _priority = that._priority;
                _hw = that._hw;
                _md = that._md;
                _inputs = that._inputs;
                _outputs = that._outputs;
                _phenotype = that._phenotype;
                _position = that._position;
                _tracecb = 0;
            }
            return *this;
        }

        //! Returns true if hardware(s) are equivalent.
        bool operator==(const organism& that) const {
            return (_name == that._name)
            && (_generation == that._generation)
            && (_update == that._update)
            && (_alive == that._alive)
            && (_priority == that._priority)
            && (_hw == that._hw)
            && (_md == that._md)
            && (_inputs == that._inputs)
            && (_outputs == that._outputs)
            && (_phenotype == that._phenotype)
            && (_position == that._position);
        }

        //! Retrieve this organism's name.
        long& name() { return _name; }
		
        //! Retrieve this organism's id (const-qualified).
        const long& name() const { return _name; }
        
        //! Retrieve this organism's generation.
        double& generation() { return _generation; }
        
        //! Retrieve this organism's generation (const-qualified).
        const double& generation() const { return _generation; }
        
        //! Retrieve this organism's update.
        unsigned long& birth_update() { return _update; }
        
        //! Retrieve this organism's update (const-qualified).
        const unsigned long& birth_update() const { return _update; }
        
		//! Retrieve this organism's priority.
		priority_type& priority() { return _priority; }
		
		//! Retrieve this organism's priority (const-qualified).
		const priority_type& priority() const { return _priority; }
		
		//! Retrieve this organism's representation.
		representation_type& repr() { return _hw.repr(); }
        
		//! Retrieve this organism's representation (const-qualified).
		const representation_type& repr() const { return _hw.repr(); }
        
        //! Retreive this organism's hardware.
        hardware_type& hw() { return _hw; }

        //! Retreive this organism's hardware (const-qualified).
        const hardware_type& hw() const { return _hw; }
        
        //! Retrieve this organism's meta data.
        meta_data& md() { return _md; }
        
        //! Retrieve this organism's meta data (const-qualified).
        const meta_data& md() const { return _md; }
        
        //! Retrieve whether this organism is alive.
        bool& alive() { return _alive; }
        
        //! Retrieve this organism's inputs.
        iobuffer_type& inputs() { return _inputs; }

        //! Retrieve this organism's outputs.
        iobuffer_type& outputs() { return _outputs; }

        //! Retrieve this organism's phenotype.
        phenotype_map_type& phenotype() { return _phenotype; }
        
        //! Return's this organism's position.
        position_type& position() { return _position; }
        
        //! Turn on hardware tracing for this organism.
        void trace(typename hardware_type::abstract_hardware_trace* cb) {
            _tracecb = cb;
        }
        
        //! Turn off hardware tracing for this organism.
        void clear_trace() {
            _tracecb = 0;
        }
        
	protected:
        long _name; //!< Name (id number) of this organism.
        double _generation; //!< Generation of this organism.
        unsigned long _update; //!< Update at which this organism was born.
        bool _alive; //!< Is this organism currently alive?
		priority_type _priority; //!< This organism's priority.
        hardware_type _hw; //!< This organism's virtual hardware.
        meta_data _md; //!< This organism's meta data.
        iobuffer_type _inputs; //!< This organism's inputs.
        iobuffer_type _outputs; //!< This organism's outputs.
        phenotype_map_type _phenotype; //!< This organism's phenotype.
        position_type _position; //!< This organism's position.
        typename hardware_type::abstract_hardware_trace* _tracecb; //!< Trace handler, if so configured.

	private:
		/* These enable serialization and de-serialization of organisms.
		 This would be easy, except for the fact that we can't round-trip NaNs.  So,
		 we store a flag instead of priority in that case.
         */ 
		friend class boost::serialization::access;
		template<class Archive>
		void save(Archive & ar, const unsigned int version) const {
            ar & boost::serialization::make_nvp("name", _name);
            ar & boost::serialization::make_nvp("generation", _generation);
            ar & boost::serialization::make_nvp("update", _update);
            ar & boost::serialization::make_nvp("alive", _alive);
			bool null_priority=_priority.is_null();
			ar & BOOST_SERIALIZATION_NVP(null_priority);
			if(!null_priority) {
				ar & boost::serialization::make_nvp("priority", _priority);
			}
            ar & boost::serialization::make_nvp("hardware", _hw);
            ar & boost::serialization::make_nvp("meta_data", _md);
            ar & boost::serialization::make_nvp("inputs", _inputs);
            ar & boost::serialization::make_nvp("outputs", _outputs);
            ar & boost::serialization::make_nvp("phenotype", _phenotype);
            ar & boost::serialization::make_nvp("position", _position);
		}
		
		template<class Archive>
		void load(Archive & ar, const unsigned int version) {
            ar & boost::serialization::make_nvp("name", _name);
            ar & boost::serialization::make_nvp("generation", _generation);
            ar & boost::serialization::make_nvp("update", _update);
            ar & boost::serialization::make_nvp("alive", _alive);
			bool null_priority=true;
			ar & BOOST_SERIALIZATION_NVP(null_priority);
			if(null_priority) {
                _priority.nullify();
			} else {
				ar & boost::serialization::make_nvp("priority", _priority);
			}
            ar & boost::serialization::make_nvp("hardware", _hw);
            ar & boost::serialization::make_nvp("meta_data", _md);
            ar & boost::serialization::make_nvp("inputs", _inputs);
            ar & boost::serialization::make_nvp("outputs", _outputs);
            ar & boost::serialization::make_nvp("phenotype", _phenotype);
            ar & boost::serialization::make_nvp("position", _position);
        }
		BOOST_SERIALIZATION_SPLIT_MEMBER();
	};	
    
} // ea

#endif
