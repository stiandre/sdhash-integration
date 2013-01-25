/**
 * sdhash: Command-line interface for file hashing
 * authors: Vassil Roussev, Candice Quates
 */

#include "../sdbf/sdbf_class.h"
#include "../sdbf/sdbf_defines.h"
#include "../sdbf/sdbf_set.h"
#include "sdhash_threads.h"
#include "sdhash.h"
#include "version.h"

#include "boost/filesystem.hpp"
#include "boost/program_options.hpp"
#include "boost/lexical_cast.hpp"

#include <fstream>

namespace fs = boost::filesystem;
namespace po = boost::program_options;

// Global parameter configuration defaults
sdbf_parameters_t sdbf_sys = {
    1,               // threads
    64,              // entr_win_size
    256,             // BF size
    4*KB,            // block_size
    64,              // pop_win_size
    16,              // threshold
    _MAX_ELEM_COUNT, // max_elem
    1,               // output_threshold
    FLAG_OFF,        // warnings
    -1,              // dd block size
    0,              // sample size off
    0,              // verbose mode off
    128*MB,         // segment size
    NULL             // optional filename
};


/** sdhash program main
*/
int main( int argc, char **argv) {
    uint32_t  i, j, k, file_cnt;
    int rcf;

    string config_file;
    string listingfile;
    string input_name;
    string output_name;
    string segment_size;
    vector<string> inputlist;
    po::variables_map vm;
    po::options_description config("Configuration");
    try {
        // Declare a group of options that will be 
        // allowed both on command line and in
        // config file
    //    po::options_description config("Configuration");
        config.add_options()
                ("config-file,C", po::value<string>(&config_file)->default_value("sdhash.cfg"), "name of config file")
                ("hash-list,f",po::value<std::string>(&listingfile),"generate SDBFs from list of filenames")
                ("deep,r", "generate SDBFs from directories and files")
                ("gen-compare,g", "generate SDBFs and compare all pairs")
                ("compare,c","compare all pairs in SDBF file, or compare two SDBF files to each other")
                ("validate","parse SDBF file to check if it is valid")
                ("threshold,t",po::value<int32_t>(&sdbf_sys.output_threshold)->default_value(1),"only show results >=threshold")
                ("block-size,b",po::value<int32_t>(&sdbf_sys.dd_block_size),"hashes input files in nKB blocks")
                ("threads,p",po::value<uint32_t>(&sdbf_sys.thread_cnt)->default_value(1),"compute threads to use")
                ("sample-size,s",po::value<uint32_t>(&sdbf_sys.sample_size)->default_value(0),"sample N filters for comparisons")
                ("segment-size,z",po::value<std::string>(&segment_size),"break files into segments before hashing")
                ("name,n",po::value<std::string>(&input_name),"set SDBF name for stdin mode")
                ("output,o",po::value<std::string>(&output_name),"set output filename")
                ("heat-map,m", "show a heat map of BF matches")
                ("warnings,w","turn on warnings")
                ("verbose","debugging and progress output")
                ("version","show version info")
                ("help,h","produce help message")
            ;

        // options that do not need to be in help message
        po::options_description hidden("Hidden");
        hidden.add_options()
            ("input-files", po::value<vector<std::string> >(&inputlist), "input file") 
            ;

        po::options_description cmdline_options;
        cmdline_options.add(config).add(hidden);

        po::options_description config_file_options;
        config_file_options.add(config);

		// setup for list of files on command line
        po::positional_options_description p;
        p.add("input-files", -1);
        
        store(po::command_line_parser(argc, argv).
              options(cmdline_options).positional(p).run(), vm);
        notify(vm);
        
        ifstream ifs(config_file.c_str());
        if (ifs)
        {
            store(parse_config_file(ifs, config_file_options), vm);
            notify(vm);
        }
    
        if (vm.count("help")) {
            cout << VERSION_INFO << ", rev " << REVISION << endl;
            cout << "Usage: sdhash <options> <source files>|<hash files>"<< endl;
            cout << config << endl;
            return 0;
        }

        if (vm.count("version")) {
            cout << VERSION_INFO << ", rev " << REVISION << endl;
            cout << "       http://sdhash.org, license Apache v2.0" << endl;
            return 0;
        }
        if (vm.count("warnings")) {
            sdbf_sys.warnings = 1;
        }
        if (vm.count("verbose")) {
            sdbf_sys.warnings = 1;
            sdbf_sys.verbose = 1;
        }
        if (vm.count("segment-size")) {
			sdbf_sys.segment_size = (boost::lexical_cast<uint64_t>(segment_size)) * MB;
        }
        if (vm.count("name")) {	
			sdbf_sys.filename=(char*)input_name.c_str();
        }
    }
    catch(exception& e)
    {
        cout << e.what() << "\n";
        return 0;
    }    
    // Initialization
    // set up sdbf object with current options
    sdbf::config = new sdbf_conf(sdbf_sys.thread_cnt, sdbf_sys.warnings, _MAX_ELEM_COUNT, _MAX_ELEM_COUNT_DD);
    // possible two sets to load
    sdbf_set *set1 = new sdbf_set();
    sdbf_set *set2 = new sdbf_set();
    // Perform all-pairs comparison
    if (vm.count("compare")) {
        if (inputlist.size()==1) {
            std::string resultlist;
            // load first set
            try {
                set1=new sdbf_set(inputlist[0].c_str());
            } catch (int e) {
                cerr << "sdhash: ERROR: Could not load SDBF file "<< inputlist[0] << ". Exiting"<< endl;
                return -1;
            }
            resultlist=set1->compare_all(sdbf_sys.output_threshold);
            cout << resultlist;
        } else if (inputlist.size()==2) {
            try {
                set1=new sdbf_set(inputlist[0].c_str());
            } catch (int e) {
                cerr << "sdhash: ERROR: Could not load SDBF file "<< inputlist[0] << ". Exiting"<< endl;
                return -1;
            }
            // load second set for comparison
            try {
                set2=new sdbf_set(inputlist[1].c_str());
            } catch (int e) {
                cerr << "sdhash: ERROR: Could not load SDBF file "<< inputlist[1] << ". Exiting"<< endl;
                return -1;
            }
            std::string resultlist;
            resultlist=set1->compare_to(set2,sdbf_sys.output_threshold, sdbf_sys.sample_size);
            cout << resultlist;
        } else  {
            cerr << "sdhash: ERROR: Comparison requires 1 or 2 arguments." << endl;
			delete set1;
			delete set2;
            return -1;
        }
        int n;
        if (set1!=NULL) {
            for (n=0;n< set1->size(); n++) 
                delete set1->at(n);
			delete set1;
		}
        if (set2!=NULL) {
            for (n=0;n< set2->size(); n++) 
                delete set2->at(n);
			delete set2;
		}
        return 0;
    }
    // validate hashes 
    if (vm.count("validate")) {
		for (i=0; i< inputlist.size(); i++) { 
            // load each set and throw it away
            if (!fs::is_regular_file(inputlist[i]))  {
				cout << "sdhash: ERROR file " << inputlist[i] << " not readable or not found." << endl;
				continue;
			}
            try {
                set1=new sdbf_set(inputlist[i].c_str());
				cout << "sdhash: file " << inputlist[i];
				cout << " SDBFs valid, contains " << set1->size() << " hashes." << endl;
            } catch (int e) {
                cerr << "sdhash: ERROR: Could not load file of SDBFs, "<< inputlist[i] << " is empty or invalid."<< endl;
				continue;
            }
			if (set1!=NULL) {
				for (int n=0;n< set1->size(); n++) 
					delete set1->at(n);
				delete set1;
			}
		}
		return 0;
	}
    std::vector<string> small;
    std::vector<string> large;
    // Otherwise we are hashing. Make sure we have files.
    if (vm.count("input-files")) {
        // process stdin -- look for - arg
        if (inputlist.size()==1 && !inputlist[0].compare("-")) {
            if (sdbf_sys.segment_size == 0) {
                sdbf_sys.segment_size = 128*MB; // not currently allowing no segments
            }
            if (sdbf_sys.dd_block_size >  0) {
                set1=sdbf_hash_stdin();
            } 
            else { 
                // block size is always going to be defaulted in this case
                sdbf_sys.dd_block_size = 16;
                set1=sdbf_hash_stdin();
            }
        } else {
            // input list iterator
			if (sdbf_sys.verbose) 
				cerr << "sdhash: Building list of files to be hashed" << endl;
            vector<string>::iterator inp;
            for (inp=inputlist.begin(); inp < inputlist.end(); inp++) {
                // if recursive mode, then check directories as well
                if ( vm.count("deep") )  {
					try {
						if (fs::is_directory(*inp)) 
							for ( fs::recursive_directory_iterator end, it(*inp); it!= end; ++it ) 
								if (boost::filesystem::is_regular_file(*it)) {
									if (sdbf_sys.verbose) 
										cerr << "sdhash: adding file to hashlist "<< it->path().string() << endl;
									if (fs::file_size(*it) < 16*MB)
										small.push_back(it->path().string());
									else
										large.push_back(it->path().string());
									if ((fs::file_size(it->path()) >= sdbf_sys.segment_size) && sdbf_sys.warnings )  {
										cerr << "Warning: file " << it->path().string() << " will be segmented in ";
										cerr << sdbf_sys.segment_size/MB << "MB chunks prior to hashing."<< endl; 
									}
								}
					} catch (fs::filesystem_error err) {
						cerr << "sdhash: ERROR: Filesystem problem in recursive searching " ;
						cerr << err.what() << endl;
						continue;
					}
                } // always check if regular file. 
                if (fs::is_regular_file(*inp)) {    
					if (sdbf_sys.verbose) 
						cerr << "sdhash: adding file to hashlist "<< *inp << endl;
                    if (fs::file_size(*inp) < 16*MB) 
                        small.push_back(*inp);
                    else 
                        large.push_back(*inp);
                    if ((fs::file_size(*inp) >= sdbf_sys.segment_size) && sdbf_sys.warnings )  {
                        cerr << "sdhash: Warning: file " << *inp << " will be segmented in ";
                        cerr << sdbf_sys.segment_size/MB << "MB chunks prior to hashing."<< endl; 
                    }
                }
            }
        }
    } else if (vm.count("hash-list")) {
        // hash from a list in a file
        struct stat stat_res;
        if( stat( listingfile.c_str(), &stat_res) != 0) {
            cerr << "sdhash: ERROR: Could not access input file "<< listingfile<< ". Exiting." << endl;
            return -1;
        }
        processed_file_t *mlist=process_file(listingfile.c_str(), 1, sdbf_sys.warnings);
        if (!mlist) {
            cerr << "sdhash: ERROR: Could not access input file "<< listingfile<< ". Exiting." << endl;
            return -1;
        }
        i=0;
		std::istringstream fromfile((char*)mlist->buffer);
		std::string fname;
		while (std::getline(fromfile,fname)) {
            if (fs::is_regular_file(fname)) {
                if (fs::file_size(fname) < 16*MB) {
                    small.push_back(fname);
                } else {
                    large.push_back(fname);
                }    
                if ((fs::file_size(fname) >= sdbf_sys.segment_size) && sdbf_sys.warnings )  {
                    cerr << "sdhash: Warning: file " << fname << " will be segmented in ";
                    cerr << sdbf_sys.segment_size/MB << "MB chunks prior to hashing."<< endl; 
                }
            }
        }
    } else {
         cout << VERSION_INFO << ", rev " << REVISION << endl;
         cout << "       http://sdhash.org, license Apache v2.0" << endl;
         cout << "Usage: sdhash <options> <source files>|<hash files>"<< endl;
         cout << config << endl;
    }
    // Having built our lists of small/large files, hash them.
    int smallct=small.size();
    int largect=large.size();
    if (smallct > 0) {
		if (sdbf_sys.verbose)
			cerr << "sdhash: hashing small files"<< endl;
        char **smalllist=(char **)alloc_check(ALLOC_ONLY,smallct*sizeof(char*),"main", "filename list", ERROR_EXIT);
        for (i=0; i < smallct ; i++) {
            smalllist[i]=(char*)alloc_check(ALLOC_ONLY,small[i].length()+1, "main", "filename", ERROR_EXIT);
            strncpy(smalllist[i],small[i].c_str(),small[i].length()+1);
        }
        if (sdbf_sys.dd_block_size < 1 )  {
			if (vm.count("gen-compare") || vm.count("output")) // if we need to save this set for comparison
				sdbf_hash_files( smalllist, smallct, sdbf_sys.thread_cnt,set1);
			else 
				sdbf_hash_files( smalllist, smallct, sdbf_sys.thread_cnt,NULL);
        } else {
            set1->add_and_hash( smalllist, smallct, sdbf_sys.dd_block_size*KB,sdbf_sys.segment_size);
		}
    }
    if (largect > 0) {
		if (sdbf_sys.verbose)
			cerr << "sdhash: hashing large files"<< endl;
        char **largelist=(char **)alloc_check(ALLOC_ONLY,largect*sizeof(char*),"main", "filename list", ERROR_EXIT);
        for (i=0; i < largect ; i++) {
            largelist[i]=(char*)alloc_check(ALLOC_ONLY,large[i].length()+1, "main", "filename", ERROR_EXIT);
            strncpy(largelist[i],large[i].c_str(),large[i].length()+1);
        }
        if (sdbf_sys.dd_block_size == 0 ) {
			if (vm.count("gen-compare")) // if we need to save this set for comparison
				sdbf_hash_files( largelist, largect, sdbf_sys.thread_cnt,set1);
			else
				sdbf_hash_files( largelist, largect, sdbf_sys.thread_cnt,NULL);
        } else {
            if (sdbf_sys.dd_block_size == -1) { 
                if (sdbf_sys.warnings || sdbf_sys.verbose) 
                   cerr << "sdhash: Warning: files over 16MB are being hashed in block mode. Use -b 0 to disable." << endl;
				if (vm.count("gen-compare")|| vm.count("output")) // if we need to save this set for comparison
					set1->add_and_hash( largelist, largect, 16*KB,sdbf_sys.segment_size);
				else
					sdbf_hash_files_dd( largelist, largect, 16*KB,sdbf_sys.segment_size, NULL);
            } else {
				if (vm.count("gen-compare")||vm.count("output")) // if we need to save this set for comparison
					set1->add_and_hash( largelist, largect,sdbf_sys.dd_block_size*KB,sdbf_sys.segment_size);
				else 
					sdbf_hash_files_dd( largelist, largect, sdbf_sys.dd_block_size*KB,sdbf_sys.segment_size, NULL);
            }
        }
    }
    // print it out if we've been asked to
    if (vm.count("gen-compare")) {
        string resultlist;
        resultlist=set1->compare_all(sdbf_sys.output_threshold);
        cout << resultlist;
    } else {
		if (vm.count("output")) {
			std::filebuf fb;
			fb.open (output_name.c_str(),ios::out|ios::binary);
			if (fb.is_open()) {
				std::ostream os(&fb);
				os << set1;
				fb.close();
			} else {
				cerr << "sdhash: ERROR cannot write to file " << output_name<< endl;
				return -1;
			}
		} else {
			cout << set1;
		}
    }
	if (set1!=NULL) {
		for (int n=0;n< set1->size(); n++) 
			delete set1->at(n);
		delete set1;
	}
	if (set2!=NULL) {
		for (int n=0;n< set2->size(); n++) 
			delete set2->at(n);
		delete set2;
	}
	return 0;
}
