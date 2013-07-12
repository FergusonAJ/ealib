/* individual.h
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
#ifndef _EA_INDIVIDUAL_H_
#define _EA_INDIVIDUAL_H_

#include <boost/serialization/nvp.hpp>
#include <ea/meta_data.h>

namespace ealib {

	/*! Definition of EALib individuals.
     
     Individuals in EALib are containers for a representation, attributes, and meta-data.
	 */
	template <typename EA>
	class individual {
	public:
		typedef typename EA::representation_type representation_type; //!< Representation type; the "genome."
        typedef typename EA::individual_attr_type attr_type; //!< Attributes type for this individual.
        typedef meta_data md_type; //!< Meta-data type.
		
		//! Constructor.
		individual() : _name(0), _generation(0.0), _update(0) {
		}
        
		//! Constructor that builds an individual from a representation.
		individual(const representation_type& r) : _name(0), _generation(0.0), _update(0), _repr(r) {
		}
        
        //! Copy constructor.
        individual(const individual& that) {
            _name = that._name;
            _generation = that._generation;
            _update = that._update;
            _repr = that._repr;
            _md = that._md;
            _attr = that._attr;
        }
        
        //! Assignment operator.
        individual& operator=(const individual& that) {
            if(this != &that) {
                _name = that._name;
                _generation = that._generation;
                _update = that._update;
                _repr = that._repr;
                _md = that._md;
                _attr = that._attr;
            }
            return *this;
        }
        
        //! Destructor.
        virtual ~individual() {
        }
        
        //! Retrieve this individual's name.
        long& name() { return _name; }
		
        //! Retrieve this individual's name (const-qualified).
        const long& name() const { return _name; }

        //! Retrieve this individual's generation.
        double& generation() { return _generation; }

        //! Retrieve this individual's generation (const-qualified).
        const double& generation() const { return _generation; }
        
        //! Retrieve this individual's birth update.
        long& birth_update() { return _update; }

        //! Retrieve this individual's update (const-qualified).
        const long& update() const { return _update; }

		//! Retrieve this individual's representation.
		representation_type& repr() { return _repr; }
        
		//! Retrieve this individual's representation (const-qualified).
		const representation_type& repr() const { return _repr; }
        
        //! Retrieve this individual's meta data.
        meta_data& md() { return _md; }

        //! Retrieve this individual's meta data (const-qualified).
        const meta_data& md() const { return _md; }
        
        //! Retrieve this individual's attributes.
        attr_type& attr() { return _attr; }
        
        //! Retrieve this individual's attributes (const-qualified).
        const attr_type& attr() const { return _attr; }

	protected:
        long _name; //!< Name (id number) of this individual.
        double _generation; //!< Generation of this individual.
        long _update; //!< Update at which this individual was born.
		representation_type _repr; //!< This individual's representation.
        meta_data _md; //!< This individual's meta data.
        attr_type _attr; //!< This individual's attributes.
        
	private:
		friend class boost::serialization::access;

        //! Serialize this individual.
        template <class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::make_nvp("name", _name);
            ar & boost::serialization::make_nvp("generation", _generation);
			ar & boost::serialization::make_nvp("representation", _repr);
            ar & boost::serialization::make_nvp("update", _update);
            ar & boost::serialization::make_nvp("meta_data", _md);
            ar & boost::serialization::make_nvp("attributes", _attr);
		}
	};

} // ea

#endif
