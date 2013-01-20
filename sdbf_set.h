
/** \file sdbf_set.h

	Contains definition of sdbf_set object.

*/

#include <ostream>
#include <string>
#include <vector>
#include <boost/thread/thread.hpp>
#include "sdbf_class.h"

#ifndef __SDBF_SET_H
#define __SDBF_SET_H

/** 
	sdbf_set: set for containing sdbf classes
*/
/// sdbf_set class
class sdbf_set {

    friend std::ostream& operator<<(std::ostream& os, const sdbf_set& s); ///< output operator
    friend std::ostream& operator<<(std::ostream& os, const sdbf_set *s); ///< output operator

public:

	/// creates blank sdbf_set
	sdbf_set(); 

	/// loads an sdbf_set from a file
	sdbf_set(const char *fname); 
	
	sdbf_set(FILE *in);

	/// accessor method for individual hashes
	class sdbf* at(uint32_t pos); 

	/// adds a single hash to this set
	void add(class sdbf *hash);

	/// adds the items in another set to this set
	void add(sdbf_set *hashset);

	/// hashes a list of filenames and adds them here.
	void add_and_hash( char **filenames, uint32_t file_count, uint32_t dd_block_size, uint64_t chunk_size) ;

	/// Returns the number of sdbfs in this set
	uint64_t size( ) ; 

	/// Computes the data size of this set
	uint64_t input_size( ) ; 

	/// Compares all objects in a set to each other
	std::string compare_all(int32_t threshold); 

	///queries one set for the contents of another
	std::string compare_to(sdbf_set *other,int32_t threshold, uint32_t sample_size); 

	/// return a string which contains the output-encoded sdbfs in this set
	std::string to_string() const;

	/// is this empty?
	int empty();

	/// retrieve name of set
	std::string name() const;

	/// name this set.
	void set_name(std::string name);

private:
	/// sdbf_set is a set for containing sdbfs
	std::vector<class sdbf*> items;
	std::string setname;
	boost::mutex add_hash_mutex;
};

#endif
