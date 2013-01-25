// sdbf_set.cc
// author: candice quates
// set function implementation 

#include "../sdbf/sdbf_class.h"
#include "../sdbf/sdbf_defines.h"

#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <fstream>
using namespace std;

namespace fs = boost::filesystem;


/** Creates empty sdbf_set
*/
sdbf_set::sdbf_set() {
    setname="default";    
}

/** 
    Loads all sdbfs from a file into a new set
    \param fname name of sdbf file
*/
sdbf_set::sdbf_set(const char *fname) {
    if (fs::is_regular_file(fname)) {
        FILE *in = fopen( fname, "r");
        if (in!=NULL) {  // if fail to open leave set empty
            setname=(string)fname;
            int bar=getc( in);
            if (!feof(in)) {
                ungetc(bar,in);
            }
            while( !feof( in)) {
                class sdbf *sdbfm = new sdbf( in);
                items.push_back( sdbfm);
                getc( in);
                int bar=getc( in);
                if (!feof(in)) {
                    ungetc(bar,in);
                }
            }
        }
        fclose(in);
    }
}

/** 
	Loads all exiting sdbfs from a file into a new set
	\param in open input file
 */
sdbf_set::sdbf_set(FILE *in) {
	if (in!=NULL) {  // if fail to open leave set empty		
		int bar=getc( in);
        if (!feof(in)) {
			ungetc(bar,in);
		}
		while( !feof( in)) {
			class sdbf *sdbfm = new sdbf( in);
            items.push_back( sdbfm);
            getc( in);
            int bar=getc( in);
            if (!feof(in)) {
				ungetc(bar,in);
			}
		}
	}
}

/**
    Accessor method for a single sdbf* in this set
    \param pos position 0 to size()
    \returns sdbf* or NULL if position not valid
*/
class sdbf*
sdbf_set::at(uint32_t pos) {
    if (pos < items.size()) 
        return items.at(pos);    
    else 
        return NULL;
}

/** 
    Adds a single hash to this set
    \param hash an existing sdbf hash
*/
void 
sdbf_set::add(sdbf *hash) {
    this->add_hash_mutex.lock();
    items.push_back(hash);
    this->add_hash_mutex.unlock();
}

/**
    Adds all items in another set to this set
    \param hashset sdbf_set* to be added
*/
void 
sdbf_set::add(sdbf_set *hashset) {
    // for all in hashset->items, add to this->items
    for (std::vector<sdbf*>::const_iterator it = hashset->items.begin(); it!=hashset->items.end() ; ++it)  {
        items.push_back(*it);
    }    
}

/**
    Computes the data size of this set, from the
    input_size() values of its' content sdbf hashes.
    \returns uint64_t total of input sizes
*/
uint64_t 
sdbf_set::input_size()  {
    uint64_t size=0;
    for (std::vector<sdbf*>::const_iterator it = items.begin(); it!=items.end() ; ++it)  {
        size+=(*it)->input_size();
    }
    return size;
}

/** 
    Number of items in this set    
    \returns uint64_t number of items in this set
*/
uint64_t
sdbf_set::size() {
    return items.size();
}

/** 
    Checks empty status of container
    \returns int 1 if empty, 0 if non-empty
*/
int
sdbf_set::empty() {
    if (items.size() > 0) 
        return 0;
    else 
        return 1;
}

/**
    Generates a string which contains the output-encoded sdbfs in this set
    \returns std::string containing sdbfs.
*/
std::string 
sdbf_set::to_string() const {
    std::stringstream builder;
    for (std::vector<sdbf*>::const_iterator it = items.begin(); it!=items.end() ; ++it)  {
        builder << *it ;    
    }
    return builder.str();
}

/** 
    Write this sdbf_set to stream 
    \param os target output stream
    \param *s set to write out
    \returns ostream& output stream
*/
ostream& operator<<(ostream& os, const sdbf_set& s) {
    os << s.to_string();
    return os;
}

/**
    Write this sdbf_set to stream 
    \param os target output stream
    \param *s set to write out
    \returns ostream& output stream
*/
ostream& operator<<(ostream& os, const sdbf_set *s) {
    os << s->to_string();
    return os;
}

/**
    Retrieve name of this set
    \returns string name
*/
std::string
sdbf_set::name() const {
    return setname;
}

/**
    Change name of this set
    \param string name 
*/
void
sdbf_set::set_name(std::string name) {
    setname=name;
}

/**
    Compares each sdbf object in target to every other sdbf object in target
    and returns the results as a list stored in a string 

    \param threshold output threshold, defaults to 1
    \returns std::string result listing
*/
std::string 
sdbf_set::compare_all(int32_t threshold) { 
    int32_t score = 0;
    uint32_t map_on = 0;
    std::stringstream out;
    out.fill('0');
    int end = this->items.size();
    for (int i = 0; i < end ; i++) {
        for (int j = i; j < end ; j++) {
            if (i == j) continue;
            score = this->items.at(i)->compare(this->items.at(j),map_on,0);
            if (score >= threshold)  {
                out << this->items.at(i)->name() << "|" << this->items.at(j)->name() ;
                out << "|" << setw (3) << score << std::endl;
            }
        }            
    }
    return out.str();
}

/**
    Compares each sdbf object in other to each object in this set, and returns
    the results as a list stored in a string.

    \param other set
    \param threshold output threshold, defaults to 1
    \param sample_size size of bloom filter sample. send 0 for no sampling
    \returns std::string result listing
*/
std::string
sdbf_set::compare_to(sdbf_set *other,int32_t threshold,uint32_t sample_size) {
    int32_t score = 0;
    uint32_t map_on = 0;
    std::stringstream out;
    out.fill('0');
    int tend = other->size();
    int qend = this->size();
    for (int i = 0; i < qend ; i++) {
        for (int j = 0; j < tend ; j++) {
            score = this->items.at(i)->compare(other->items.at(j),map_on,sample_size);
            if (score >= threshold) {
                out << this->items.at(i)->name() << "|" << other->items.at(j)->name() ;
                out << "|" << setw (3) << score << std::endl;
            }
        }
    }
    return out.str();
}


/**
 * Compute block-based SD for a list of files & add them to the set.
    \param filenames list of filenames to hash
    \param file_count amount of files in the list
    \param dd_block_size size of block to use when hashing
    \param chunk_size of segment to read before hashing it
 */
void
sdbf_set::add_and_hash( char **filenames, uint32_t file_count, uint32_t dd_block_size, uint64_t chunk_size) {
    int32_t i,j, result = 0;
    struct stat file_stat;
    int32_t chunks = 0,csize = 0;
    ifstream *is = new ifstream();
	int tailflag = 0;

    for( i=0; i<file_count; i++) {
		tailflag=0;
        if (stat(filenames[i],&file_stat)) {
            continue; // try the next one if cannot access
        } 
        is->open(filenames[i], ios::binary);
        if (file_stat.st_size > chunk_size && chunk_size > 0) {
            chunks = file_stat.st_size / (chunk_size) ;
			// adjusting for too small of a last fragment
			// on a single segment piece
			if (file_stat.st_size - chunk_size <= 512) {
				chunks=0;
			} 
			// last segment adjustment
			if (file_stat.st_size - chunks*chunk_size <= 512) {
				chunks--;
				tailflag=1;
			} 
            for (j=0;j<=chunks;j++) {
                std::stringstream namestr;
                namestr << filenames[i];
                if (j > 0 || chunks > 0) { // assuming MB sizing.
                    namestr.fill('0');
                    namestr << "." << setw(4) << j*chunk_size/MB << "M" ;
                }
                string myname = namestr.str();
                char *fname = (char*)alloc_check(ALLOC_ONLY,myname.length()+1,"add_and_hash","filename",ERROR_EXIT);
                strncpy(fname,myname.c_str(),myname.length()+1);
                if (j==chunks) {
                    csize=file_stat.st_size - (chunks*chunk_size) ;
					if (tailflag) 
						csize=csize+(file_stat.st_size - (chunks+1*chunk_size)) ;
                } else {
                    csize=chunk_size;
				}
                try {
                    class sdbf *sdbfm = new sdbf(fname,is,dd_block_size,csize);
                    //if (addto!=NULL) {
                        items.push_back(sdbfm);
                    //} else {
                        //std::cout <<sdbfm;
                    //}
                } catch (int e) {
                    if (e==-2)
                       exit(-2);
                }
            }
        } else {
            try {
                class sdbf *sdbfm = new sdbf(filenames[i],is,dd_block_size,file_stat.st_size);
                //if (addto!=NULL) {
                    items.push_back(sdbfm);
                //} else {
                    //std::cout <<sdbfm;
                //}
            } catch (int e) {
                if (e==-2)
                   exit(-2);
            }
        }
        is->close();
    }
    delete is;
}
