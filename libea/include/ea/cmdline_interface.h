/* cmdline_interface.h
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
#ifndef _EA_CMDLINE_INTERFACE_H_
#define _EA_CMDLINE_INTERFACE_H_

#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/serialization/version.hpp>
#include <string>
#include <map>
#include <iostream>

#include <ea/algorithm.h>
#include <ea/concepts.h>
#include <ea/events.h>
#include <ea/meta_data.h>
#include <ea/structure.h>
#include <ea/analysis/tool.h>
#include <ea/lifecycle.h>
#include <ea/datafiles/runtime.h>

namespace ealib {

    //! Abstract base class allowing for a limited set of interactions with an EA.
    class ea_interface {
    public:
        //! Execute an EA based on the given command-line parameters.
        virtual void exec(int argc, char* argv[]) = 0;
	};
    

    /*! This class is used to interface the runtime environment with an EA interface.
     */
    class registrar : ea_interface {
    public:
        //! Retrieve the registrar.
		static registrar* instance() {
			if(!_instance) {
				_instance = new registrar();
			}
			return _instance;
		}
        
        //! Execute an EA based on the given command-line parameters.
        virtual void exec(int argc, char* argv[]) {
            _ea->exec(argc, argv);
        }

        //! Register an interface to an EA.
        void register_ea(ea_interface* ea) {
            _ea = ea;
        }
        
    protected:
        //! Constructor.
        registrar() : _ea(0) {
        }
        
		static registrar* _instance; //!< Singleton.
        ea_interface* _ea; //!< Interface to the EA.
    };
    

    template <typename T>
    struct pointer_map {
        typedef T value_type;
        typedef T* ptr_type;
        typedef std::map<std::string, boost::shared_ptr<value_type> > map_type;
        map_type _m;
        
        pointer_map() { }
        virtual ~pointer_map() { }
        
        template <typename U>
        void put() {
            boost::shared_ptr<value_type> p(new U());
            _m[U::name()] = p;
        }
        
        template <typename U, typename EA>
        void put(EA& ea) {
            boost::shared_ptr<value_type> p(new U(ea));
            _m[U::name()] = p;
        }
        
        ptr_type get(const std::string& k) {
            return _m[k].get();
        }
    };
    
    // pre-dec
    template <typename EA> class cmdline_interface;
    
    
    /* The below are unbound template functions that are to be called from the 
     appropriate "gather" method in subclasses of the cmdline_interface.  It's done
     this way to avoid additional template / macro craziness.
     */
    
    //! Add a command line option to the given EA interface.
    template <typename MDType, typename EA>
    void add_option(cmdline_interface<EA>* ci) {
        ci->_od->add_options()(MDType::key(), boost::program_options::value<std::string>());
    }
    
    //! Add an analysis tool to the tools that are registered for an EA.
    template <template <typename> class Tool, typename EA>
    void add_tool(cmdline_interface<EA>* ci) {
        typedef Tool<EA> tool_type;
        ci->_tools.template put<tool_type>();
    }
    
    //! Add an event to the list of events that are registered for an EA.
    template <template <typename> class Event, typename EA>
    void add_event(cmdline_interface<EA>* ci, EA& ea) {
        typedef Event<EA> event_type;
        boost::shared_ptr<event_type> p(new event_type(ea));
        ci->_events.push_back(p);
    }
    
    /*! Command-line interface to an EA.
     */
    template <typename EA>
    class cmdline_interface : public ea_interface {
    public:
        typedef EA ea_type; //!< Type of the EA being used.
        typedef ealib::analysis::unary_function<ea_type> tool_type; //!< Type for analysis tools.
        typedef pointer_map<tool_type> tool_registry; //!< Storage for analysis tools.
        typedef std::vector<boost::shared_ptr<ealib::event> > event_list; //!< Storage for events.
        
        //! Constructor.
        cmdline_interface() {
            registrar::instance()->register_ea(this);
        }

        //! Parse command-line (and potentially config file) options.
        boost::program_options::variables_map parse_command_line(int argc, char* argv[]) {
            namespace po = boost::program_options;
            using namespace std;
            
            // these options are only available on the command line.  when adding options,
            // if they must be available to the EA, don't add them here -- use meta_data
            // instead.
            po::options_description cmdline_only_options("Command-line only options");
            cmdline_only_options.add_options()
            ("help,h", "produce this help message")
            ("config,c", po::value<string>()->default_value("ealib.cfg"), "ealib configuration file")
            ("checkpoint,l", po::value<string>(), "load a checkpoint file")
            ("override", "override checkpoint options")
            ("reset", "reset all fitness values prior to continuing a checkpoint")
            ("analyze", po::value<string>(), "analyze the results of this EA")
            ("with-time", "output the instantaneous and mean wall-clock time per update");
            
            po::options_description ea_options("Configuration file and command-line options");
            gather_options(ea_options);
            
            po::options_description all_options;
            all_options.add(cmdline_only_options).add(ea_options);
            
            po::variables_map vm;
            po::store(po::parse_command_line(argc, argv, all_options), vm);
            po::notify(vm);
            
            string cfgfile(vm["config"].as<string>());
            ifstream ifs(cfgfile.c_str());
            if(ifs.good()) {
                po::parsed_options opt=po::parse_config_file(ifs, ea_options, true);
                vector<string> unrec = po::collect_unrecognized(opt.options, po::exclude_positional);
                if(!unrec.empty()) {
                    ostringstream msg;
                    msg << "Unrecognized options were found in " << cfgfile << ":" << endl;
                    for(std::size_t i=0; i<unrec.size(); ++i) {
                        msg << "\t" << unrec[i] << endl;
                    }
                    msg << "Exiting..." << endl;
                    throw ealib::ealib_exception(msg.str());
                }
                po::store(opt, vm);
                ifs.close();
            }
            
            if(vm.count("help")) {
                ostringstream msg;
                msg << "Usage: " << argv[0] << " [-c config_file] [-l checkpoint] [--override] [--analyze] [--option_name value...]" << endl;
                msg << all_options << endl;
                throw ealib::ealib_exception(msg.str());
            }
            
            return vm;
        }

        //! Execute an EA based on the given command-line parameters.
        virtual void exec(int argc, char* argv[]) {
            boost::program_options::variables_map vm = parse_command_line(argc, argv);
            
            if(vm.count("analyze")) {
                analyze(vm);
            } else if(vm.count("checkpoint")) {
                continue_checkpoint(vm);
            } else {
                run(vm);
            }
        }
        
        //! Gather all the registered options into an options description.
        void gather_options(boost::program_options::options_description& od) {
            _od = &od;
            gather_options();
        }
        
        //! Gather the options supported by this EA.
        virtual void gather_options() { }
        
        //! Gather the analysis tools supported by this EA.
        virtual void gather_tools() { }
        
        //! Gather the events that occur during a trial of this EA.
        virtual void gather_events(EA& ea) { }

        //! Analyze an EA instance.
		virtual void analyze(boost::program_options::variables_map& vm) {
			ea_type ea;
            ea.configure();
            load_if(vm, ea);
            apply(vm, ea);
            ea.initialize();
            gather_tools();
            
            typename tool_registry::ptr_type p =_tools.get(vm["analyze"].as<std::string>());
            p->initialize(ea);
            (*p)(ea);
        }
        
        //! Continue a previously-checkpointed EA.
		virtual void continue_checkpoint(boost::program_options::variables_map& vm) {
            ea_type ea;
            ea.configure();
            load(vm, ea);
            
            // conditionally apply command-line and/or file parameters:
            if(vm.count("override")) {
                apply(vm, ea);
            }
            
            // conditionally reset all fitnesses
            if(vm.count("reset")) {
                ea.reset();
            }
            
            ea.initialize();
            gather_events(ea);
            if(vm.count("with-time")) {
                add_event<datafiles::runtime>(this,ea);
            }
            execute(ea);
        }        
        
		//! Run the EA.
		virtual void run(boost::program_options::variables_map& vm) {
			ea_type ea;
            ea.configure();
            apply(vm, ea);
            
            if(exists<RNG_SEED>(ea)) {
                ea.rng().reset(get<RNG_SEED>(ea));
            }
            
            ea.initialize();
            gather_events(ea);
            if(vm.count("with-time")) {
                add_event<datafiles::runtime>(this,ea);
            }
            ea.initial_population();
            execute(ea);
        }
        
    protected:

        //! Apply meta-data to a single population.
        void apply(const std::string& k, const std::string& v, EA& ea, singlePopulationS) {
            std::cerr << "\t" << k << "=" << v << std::endl;
            put(k, v, ea.md());
        }
        
        //! Apply meta-data to multiple populations.
        void apply(const std::string& k, const std::string& v, EA& ea, multiPopulationS) {
            std::cerr << "\t" << k << "=" << v << " (+subpopulations)" << std::endl;
            put(k, v, ea.md());
            for(typename EA::iterator i=ea.begin(); i!=ea.end(); ++i) {
                put(k,v,i->md());
            }
        }

        //! Apply any command line options to the EA.
        void apply(boost::program_options::variables_map& vm, EA& ea) {
            namespace po = boost::program_options;
            po::notify(vm);
            std::cerr << std::endl << "Active configuration options:" << std::endl;
            for(po::variables_map::iterator i=vm.begin(); i!=vm.end(); ++i) {
                const std::string& k=i->first;
                const std::string& v=i->second.as<std::string>();
                apply(k, v, ea, typename EA::population_structure_tag());
            }
            std::cerr << std::endl;
        }

        //! Returns true if a checkpoint file should be loaded.
        bool has_checkpoint(boost::program_options::variables_map& vm) {
            return (vm.count("checkpoint") > 0);
        }

        //! Load the EA from the checkpoint file, if present.
        void load_if(boost::program_options::variables_map& vm, ea_type& ea) {
            if(has_checkpoint(vm)) {
                load(vm, ea);
            }
        }
        
        //! Load the EA from the checkpoint file.
        void load(boost::program_options::variables_map& vm, ea_type& ea) {
            if(!has_checkpoint(vm)) {
                throw fatal_error_exception("required checkpoint file not found.");
            }
            std::string cpfile(vm["checkpoint"].as<std::string>());
            lifecycle::load_checkpoint(cpfile, ea);
        }
        
        //! Execute the EA for the configured number of epochs.
        void execute(ea_type& ea) {
			// and run it!
			for(int i=0; i<get<RUN_EPOCHS>(ea); ++i) {
                lifecycle::advance_epoch(get<RUN_UPDATES>(ea), ea);
                if(!get<CHECKPOINT_OFF>(ea,0)) {
                    std::ostringstream filename;
                    filename << get<CHECKPOINT_PREFIX>(ea) << "-" << ea.current_update() << ".xml";
                    lifecycle::save_checkpoint(filename.str(), ea);
                }
			}
		}
        
        // This grants the following templated functions access to the internals
        // of the cmdline_interface... a little hackish, but it avoids macro/template
        // craziness.
        template <typename T, typename U> friend void add_option(cmdline_interface<U>*);
        template <template <typename> class T, typename U> friend void add_tool(cmdline_interface<U>* ci);
        template <template <typename> class T, typename U> friend void add_event(cmdline_interface<U>* ci, U& u);

        boost::program_options::options_description* _od; //!< Options that are configured for this EA.
        tool_registry _tools; //!< Registry for EA analysis tools.
        event_list _events; //!< List of all the events attached to an EA.
    };

} // ea


/*! Declare an instance of an evolutionary algorithm, and connect it to the registrar
 for command-line access.
 */
#define LIBEA_CMDLINE_INSTANCE( ea_type, cmdline_type ) \
cmdline_type<ea_type> cmdline_type##_instance;
//BOOST_CLASS_VERSION(ea_type::individual_type, 2) 
//BOOST_CLASS_VERSION(ea_type::generational_model_type, 1)

#endif
