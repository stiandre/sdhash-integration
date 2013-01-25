// sdhash_threads.cc
// author: candice quates
// threading code for hashing individual files 
// and for hashing stdin.

#include "../sdbf/sdbf_class.h"
#include "../sdbf/sdbf_defines.h"
#include "sdhash.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <boost/thread/thread.hpp>

using namespace std;

// Global parameters

extern sdbf_parameters_t sdbf_sys;


// NOT in sdbf class
/** 
    the actual processing task for the file-list hashing without block mode
*/
void 
*thread_sdbf_hashfile( void *task_param) {
    filehash_task_t *task = (filehash_task_t *)task_param;
    struct stat file_stat;
    int32_t chunks = 0,csize = 0;
    ifstream *is = new ifstream();

    int i;
    for( i=task->tid; i<task->file_count; i+=task->tcount) {
        if (stat(task->filenames[i],&file_stat))
            continue;
        is->open(task->filenames[i], ios::binary);
        try {
			if (sdbf_sys.verbose) 
				cerr << "sdhash: digesting file " << task->filenames[i] << endl;
            class sdbf *sdbfm = new sdbf(task->filenames[i],is,0,file_stat.st_size);
            if (!task->addset) {
                cout << sdbfm;
                delete sdbfm;            
            } else 
                task->addset->add(sdbfm);
            
        } catch (int e) {
            if (e==-2)
               exit(-2);
            if (e==-3 && sdbf_sys.warnings) {
                    cerr <<"Input file too small for processing: "<< task->filenames[i] << endl;
            }
       }
       is->close();
    }
	delete is;
}

sdbf_set
*sdbf_hash_stdin( ) {
    sdbf_set *myset = new sdbf_set();
    const char *default_name= "stdin";
    char *stream;
    int32_t j = 0;
    if (sdbf_sys.filename == NULL) {
        stream = (char*)default_name;
    } else {
        stream = sdbf_sys.filename;
    }
    
    char *readbuf = (char *)alloc_check(ALLOC_ONLY,sizeof(char)*sdbf_sys.segment_size, "sdbf_hash_stdin","read buffer",ERROR_EXIT);
    if (readbuf!=NULL) {
        int done = 0;
        while (!done) {
            std::stringstream namestr;
            namestr << stream;
            memset(readbuf,0,sdbf_sys.segment_size);
            size_t sz = fread(readbuf,1,sdbf_sys.segment_size,stdin);
            if (sz != sdbf_sys.segment_size) 
                done = 1;
            //if (j >0) { // always start at 0M
                namestr.fill('0');
                namestr << "." << setw(4) << j*sdbf_sys.segment_size/MB << "M" ;
			//}
            string myname = namestr.str();
            char *fname = (char*)alloc_check(ALLOC_ONLY,myname.length()+1, "sdbf_hash_stdin", "generated filename",ERROR_EXIT);
            strncpy(fname,myname.c_str(),myname.length()+1);
            if (sdbf_sys.verbose) 
				cerr << "sdhash: "<< stream << " segment begin "<< j*sdbf_sys.segment_size/MB << "M" << endl;
            try {
                class sdbf *sdbfm = new sdbf(fname,readbuf,sdbf_sys.dd_block_size*KB,sz);
                //myset->insert(sdbfm);
                myset->add(sdbfm);
            } catch (int e) {
                if (e==-2)
                   exit(-2);
            } 
            j++;
        }
    }
    if (sdbf_sys.verbose) 
		cerr << "sdhash: finished hashing stdin." << endl;
    return myset;
}

/**
 * Compute SD for a list of files & add them to a new set.
 * Not block-wise.
 \returns sdbf_set with the hashes in it
 */
void
sdbf_hash_files( char **filenames, uint32_t file_count, int32_t thread_cnt,sdbf_set *addto ) {
    int32_t i, t, j;
    //sdbf_set *myset = new sdbf_set();
    struct stat file_stat;
    int32_t chunks = 0,csize = 0;
    ifstream *is = new ifstream();

    // Sequential implementation
    if( thread_cnt == 1) {
        for( i=0; i<file_count; i++) {
            if (stat(filenames[i],&file_stat))
                continue;
            is->open(filenames[i], ios::binary);
            try {
				if (sdbf_sys.verbose) 
					cerr << "sdhash: digesting file " << filenames[i] << endl;
                class sdbf *sdbfm = new sdbf(filenames[i],is,0,file_stat.st_size);
                if (!addto) {
                            cout << sdbfm ;
                    delete sdbfm;
                } else 
                    addto->add(sdbfm);
            } catch (int e) {
                if (e==-2)
                   exit(-2);
                if (e==-3 && sdbf_sys.warnings) {
					cerr << "Input file too small for processing: "<< filenames[i]<< endl;
                }
            }
            is->close();
        }
    // Threaded implementation
    } else {
		boost::thread *thread_pooll[MAX_THREADS];
        filehash_task_t *tasks = (filehash_task_t *) alloc_check( ALLOC_ZERO, thread_cnt*sizeof( filehash_task_t), "sdbf_hash_files", "tasks", ERROR_EXIT);
        for( t=0; t<thread_cnt; t++) {
            tasks[t].tid = t;
            tasks[t].tcount = thread_cnt;
            tasks[t].filenames = filenames;
            tasks[t].file_count = file_count;
            tasks[t].addset = addto;
			thread_pooll[t] = new boost::thread(thread_sdbf_hashfile,tasks+t);
        }
        for( t=0; t<thread_cnt; t++) {
            thread_pooll[t]->join();
        }
        for( t=0; t<thread_cnt; t++) {
            delete thread_pooll[t];
        }
		free(tasks);
    // End threading
    }
    delete is;
}

void
sdbf_hash_files_dd( char **filenames, uint32_t file_count, uint32_t dd_block_size, uint64_t chunk_size, sdbf_set *addto) {
    int32_t i,j, result = 0;
    struct stat file_stat;
    int32_t chunks = 0,csize = 0;
    ifstream *is = new ifstream();
	int tailflag = 0;

    for( i=0; i<file_count; i++) {
		tailflag=0;
        if (stat(filenames[i],&file_stat)) {
			if (sdbf_sys.warnings) 
				cerr<< "sdhash: cannot access " << filenames[i] << endl;
            continue; // try the next one if cannot access
        }
		if (sdbf_sys.verbose) 
			cerr << "sdhash: digesting file " << filenames[i] << endl;
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
                if (j > 0 || (chunks>0)) { // assuming MB sizing.
                    namestr.fill('0');
                    namestr << "." << setw(4) << j*chunk_size/MB << "M" ;
                }
                string myname = namestr.str();
                char *fname = (char*)alloc_check(ALLOC_ONLY,myname.length()+1,"hash_files_dd","filename",ERROR_EXIT);
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
                    if (addto!=NULL) {
                        addto->add(sdbfm);
                    } else {
                        std::cout <<sdbfm;
						delete sdbfm;
                    }
                } catch (int e) {
                    if (e==-2)
                       exit(-2);
                }
            }
        } else {
            try {
                class sdbf *sdbfm = new sdbf(filenames[i],is,dd_block_size,file_stat.st_size);
                if (addto!=NULL) {
                    addto->add(sdbfm);
                } else {
                    std::cout <<sdbfm;
					delete sdbfm;
                }
            } catch (int e) {
                if (e==-2)
                   exit(-2);
            }
        }
        is->close();
    }
    delete is;
}

