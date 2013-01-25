/**
 * sdbf_core.c: Similarity digest calculation functions. 
 * authors Vassil Roussev, Candice Quates
 */

#include <boost/thread/thread.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include "sdbf_class.h"
#include "sdbf_defines.h"

/**
 * Generate ranks for a file chunk.
 */
void 
sdbf::gen_chunk_ranks( uint8_t *file_buffer, const uint64_t chunk_size, uint16_t *chunk_ranks, uint16_t carryover) {
    uint64_t offset, entropy=0;
    uint8_t *ascii = (uint8_t *)alloc_check( ALLOC_ZERO, 256, "gen_chunk_ranks", "ascii", ERROR_EXIT);;

    if( carryover > 0) {
        memcpy( chunk_ranks, chunk_ranks+chunk_size-carryover, carryover*sizeof(uint16_t));
    }
    memset( chunk_ranks+carryover,0, (chunk_size-carryover)*sizeof( uint16_t));
    for( offset=0; offset<chunk_size-config->entr_win_size; offset++) {
        // Initial/sync entropy calculation
         if( offset % config->block_size == 0) {
            entropy = config->entr64_init_int( file_buffer+offset, ascii);
        // Incremental entropy update (much faster)
        } else {
            entropy = config->entr64_inc_int( entropy, file_buffer+offset-1, ascii);
        }
        chunk_ranks[offset] = config->ENTR64_RANKS[entropy >> ENTR_POWER];
    }
    free( ascii);
}

/**
 * Generate scores for a ranks chunk.
 */
void 
sdbf::gen_chunk_scores( const uint16_t *chunk_ranks, const uint64_t chunk_size, uint16_t *chunk_scores, int32_t *score_histo) { 
    uint64_t i, j;
    uint32_t pop_win = config->pop_win_size;
    uint64_t min_pos = 0;
    uint16_t min_rank = chunk_ranks[min_pos]; 

    memset( chunk_scores, 0, chunk_size*sizeof( uint16_t));
    for( i=0; i<chunk_size-pop_win; i++) {
        // try sliding on the cheap    
        if( i>0 && min_rank>0) {
            while( chunk_ranks[i+pop_win] >= min_rank && i<min_pos && i<chunk_size-pop_win+1) {
                if( chunk_ranks[i+pop_win] == min_rank)
                    min_pos = i+pop_win;
                chunk_scores[min_pos]++;
                i++;
            }
        }      
        min_pos = i;
        min_rank = chunk_ranks[min_pos];
        for( j=i+1; j<i+pop_win; j++) {
            if( chunk_ranks[j] < min_rank && chunk_ranks[j]) {
                min_rank = chunk_ranks[j];
                min_pos = j;
            } else if( min_pos == j-1 && chunk_ranks[j] == min_rank) {
                min_pos = j;
            }
        }
        if( chunk_ranks[min_pos] > 0) {
            chunk_scores[min_pos]++;
        }
    }
    // Generate score histogram (for b-sdbf signatures)
    if( score_histo) {
        for( i=0; i<chunk_size-pop_win; i++)
            score_histo[chunk_scores[i]]++;
    }
}
/**
 * Generate SHA1 hashes and add them to the SDBF--original stream version.
 */
void 
sdbf::gen_chunk_hash( uint8_t *file_buffer, const uint64_t chunk_pos, const uint16_t *chunk_scores, const uint64_t chunk_size) {
    uint64_t i;
    uint32_t sha1_hash[5];
    uint32_t bf_count = this->bf_count;
    uint32_t last_count = this->last_count;
    uint8_t *curr_bf = this->buffer + (bf_count-1)*(this->bf_size);

    for( i=0; i<chunk_size-config->pop_win_size; i++) {
        if( chunk_scores[i] > config->threshold) {
            SHA1( file_buffer+chunk_pos+i, config->pop_win_size, (uint8_t *)sha1_hash);
            uint32_t bits_set = bf_sha1_insert( curr_bf, 0, (uint32_t *)sha1_hash);
            // Avoid potentially repetitive features
            if( !bits_set)
                continue;
            last_count++;
            // Todo: check curr_bf for size vs. alloc_buffer
            if( last_count == this->max_elem) {
                curr_bf += this->bf_size;
                bf_count++;
                last_count = 0;

            }
        }
    }
    this->bf_count = bf_count;
    this->last_count = last_count;//
}
/**
 * Generate SHA1 hashes and add them to the SDBF--block-aligned version.
 */
void 
sdbf::gen_block_hash( uint8_t *file_buffer, uint64_t file_size, const uint64_t block_num, const uint16_t *chunk_scores, \
             const uint64_t block_size, class sdbf *hashto, uint32_t rem, uint32_t threshold, int32_t allowed) {

    uint8_t  *bf = hashto->buffer + block_num*(hashto->bf_size);  // BF to be filled
    uint8_t  *data = file_buffer + block_num*block_size;  // Start of data
    uint32_t  i, hash_cnt=0, sha1_hash[5];
    uint32_t  max_offset = (rem > 0) ? rem : block_size;

    for( i=0; i<max_offset-config->pop_win_size && hash_cnt< config->max_elem_dd; i++) {
        if(  chunk_scores[i] > threshold || 
            (chunk_scores[i] == threshold && allowed > 0)) {
                SHA1( data+i, config->pop_win_size, (uint8_t *)sha1_hash);
                uint32_t bits_set = bf_sha1_insert( bf, 0, (uint32_t *)sha1_hash);
                if( !bits_set)
                    continue;
                hash_cnt++;
                if( chunk_scores[i] == threshold) 
                    allowed--;
        }
    }
    hashto->elem_counts[block_num] = hash_cnt; 
}

/**
 * Generate SDBF hash for a buffer--stream version.
 */
void
sdbf::gen_chunk_sdbf( uint8_t *file_buffer, uint64_t file_size, uint64_t chunk_size) {
    assert( chunk_size > config->pop_win_size);
    
    uint32_t i, k, sum, allowed;
    int32_t score_histo[66];  // Score histogram 
    uint64_t buff_size = ((file_size >> 11) + 1) << 8; // Estimate sdbf size (reallocate later)
    buff_size = (buff_size < 256) ? 256 : buff_size;                // Ensure min size
    this->buffer = (uint8_t *)alloc_check( ALLOC_ZERO, buff_size, "gen_chunk_sdbf", "sdbf_buffer", ERROR_EXIT);

    // Chunk-based computation
    uint64_t qt = file_size/chunk_size;
    uint64_t rem = file_size % chunk_size;

    uint64_t chunk_pos = 0;
    uint16_t *chunk_ranks = (uint16_t *)alloc_check( ALLOC_ONLY, (chunk_size)*sizeof( uint16_t), "gen_chunk_sdbf", "chunk_ranks", ERROR_EXIT);
    uint16_t *chunk_scores = (uint16_t *)alloc_check( ALLOC_ZERO, (chunk_size)*sizeof( uint16_t), "gen_chunk_sdbf", "chunk_scores", ERROR_EXIT);

    for( i=0; i<qt; i++, chunk_pos+=chunk_size) {
        gen_chunk_ranks( file_buffer+chunk_size*i, chunk_size, chunk_ranks, 0);
        memset( score_histo, 0, sizeof( score_histo));
        gen_chunk_scores( chunk_ranks, chunk_size, chunk_scores, score_histo);

        // Calculate thresholding paremeters
        for( k=65, sum=0; k>=config->threshold; k--) {
            if( (sum <= this->max_elem) && (sum+score_histo[k] > this->max_elem))
                break;
            sum += score_histo[k];
        }
        allowed = this->max_elem-sum;
        gen_chunk_hash( file_buffer, chunk_pos, chunk_scores, chunk_size);
    } 
    if( rem > 0) {
        gen_chunk_ranks( file_buffer+qt*chunk_size, rem, chunk_ranks, 0);
        gen_chunk_scores( chunk_ranks, rem, chunk_scores, 0);
        gen_chunk_hash( file_buffer, chunk_pos, chunk_scores, rem);
    }

    // Chop off last BF if its membership is too low (eliminates some FPs)
    if( this->bf_count > 1 && this->last_count < this->max_elem/8) {
        this->bf_count = this->bf_count-1;
        this->last_count = this->max_elem;
    }
    // Trim BF allocation to size
    if( this->bf_count*this->bf_size < buff_size) {
        this->buffer = (uint8_t*)realloc_check( this->buffer, (this->bf_count*this->bf_size));
    }
    free( chunk_ranks);
    free( chunk_scores);

}

/**
 * Worker thread for multi-threaded block hash generation.  // NOT iN CLASS?
 */
void *
sdbf::thread_gen_block_sdbf( void *task_param) {
    uint32_t i, k, sum, allowed;
    int32_t  score_histo[66];
    blockhash_task_t *hashtask = (blockhash_task_t *)task_param;
    uint64_t block_size = hashtask->block_size;
    uint8_t *buffer = hashtask->buffer;
    uint64_t file_size = hashtask->file_size;
    
    uint64_t qt = file_size/block_size;
    uint64_t rem = file_size % block_size;

    uint64_t chunk_pos = 0;
    uint16_t *chunk_ranks = (uint16_t *)alloc_check( ALLOC_ONLY, (block_size)*sizeof( uint16_t), "gen_block_sdbf", "chunk_ranks", ERROR_EXIT);
    uint16_t *chunk_scores = (uint16_t *)alloc_check( ALLOC_ZERO, (block_size)*sizeof( uint16_t), "gen_block_sdbf", "chunk_scores", ERROR_EXIT);

    for( i=hashtask->tid; i<qt; i+=hashtask->tcount, chunk_pos+=hashtask->tcount*block_size) {
        gen_chunk_ranks( buffer+block_size*i, block_size, chunk_ranks, 0);
        memset( score_histo, 0, sizeof( score_histo));
        gen_chunk_scores( chunk_ranks, block_size, chunk_scores, score_histo);
        // Calculate thresholding paremeters
        for( k=65, sum=0; k>=config->threshold; k--) {
            if( (sum <= config->max_elem_dd) && (sum+score_histo[k] > config->max_elem_dd))
                break;
            sum += score_histo[k];
        }
        allowed = config->max_elem_dd-sum;
        gen_block_hash( buffer, file_size, i, chunk_scores, block_size, hashtask->sdbf, 0, k, allowed);
    } 
    free( chunk_ranks);
    free( chunk_scores);
    return NULL;
}

/** 
    dd-mode hash generation.
*/
void
sdbf::gen_block_sdbf_mt( uint8_t *file_buffer, uint64_t file_size, uint64_t block_size,  uint32_t thread_cnt) {
        
    blockhash_task_t *tasks = (blockhash_task_t *) alloc_check( ALLOC_ONLY, thread_cnt*sizeof( blockhash_task_t), "gen_block_sdbf_mt", "tasks", ERROR_EXIT);
    boost::thread *hash_pool[MAX_THREADS];
    int t;
    for( t=0; t<thread_cnt; t++) {
        tasks[t].tid = t;
        tasks[t].tcount = thread_cnt;
        tasks[t].buffer = file_buffer;
        tasks[t].file_size = file_size;
        tasks[t].block_size = block_size;
        tasks[t].sdbf = this;
        hash_pool[t] = new boost::thread(sdbf::thread_gen_block_sdbf,tasks+t);
    }
    for( t=0; t<thread_cnt; t++) {
        hash_pool[t]->join();
    }
    // Deal with the "tail" if necessary
    uint64_t qt = file_size/block_size;
    uint64_t rem = file_size % block_size;

    if( rem >= MIN_FILE_SIZE) {
        uint16_t *chunk_ranks = (uint16_t *)alloc_check( ALLOC_ONLY, (block_size)*sizeof( uint16_t), "gen_block_sdbf_mt", "chunk_ranks", ERROR_EXIT);
        uint16_t *chunk_scores = (uint16_t *)alloc_check( ALLOC_ZERO, (block_size)*sizeof( uint16_t), "gen_block_sdbf_mt", "chunk_scores", ERROR_EXIT);

        gen_chunk_ranks( file_buffer+block_size*qt, rem, chunk_ranks, 0);
        gen_chunk_scores( chunk_ranks, rem, chunk_scores, NULL);
        gen_block_hash( file_buffer, file_size, qt, chunk_scores, block_size, this, rem, config->threshold, this->max_elem);     

        free( chunk_ranks);
        free( chunk_scores);
    }
    for( t=0; t<thread_cnt; t++) {
         delete hash_pool[t];
    }
    free(tasks);
}

/**
 * Threading envelope for sdbf_max_score
 */ 
void *
sdbf::thread_sdbf_max_score( void *task_param) {
    sdbf_task_t *task = (sdbf_task_t *)task_param;
    uint32_t i;
    while( 1 ) {
        task->sem_start.wait();
        if (task->done)
            break;
        sdbf_max_score( (sdbf_task_t *)task, FLAG_OFF);
        task->sem_end.post();
    }
	return 0;
}

/**
 * Calculates the score between two digests
 */
int 
sdbf::sdbf_score( sdbf *sdbf_1, sdbf *sdbf_2, uint32_t map_on, uint32_t sample) {

    double max_score, score_sum = -1;
    uint32_t i, t, thread_cnt = config->thread_cnt;
    uint32_t bf_count_1, rand_offset;
    boost::thread *thread_pool[MAX_THREADS];
    sdbf_task_t *tasklist = NULL;

    if( !sdbf_1->hamming)
        sdbf_1->compute_hamming();
    if( !sdbf_2->hamming)
        sdbf_2->compute_hamming();
        
    // if sampling, set sample count here. 
    if ((sample > 0 ) && (sdbf_1->bf_count > sample)) {
        bf_count_1 = sample;
    } else {
        bf_count_1 = sdbf_1->bf_count;
    }
    // Make sure |sdbf_1| <<< |sdbf_2| 
    // does this matter anymore?
    if( (bf_count_1 > sdbf_2->bf_count) ||
        (bf_count_1 == sdbf_2->bf_count && 
            ((get_elem_count( sdbf_1, bf_count_1-1) > get_elem_count( sdbf_2, sdbf_2->bf_count-1)) ||
              strcmp( (char*)sdbf_1->hashname, (char*)sdbf_2->hashname) > 0 ))) {
            sdbf *tmp = sdbf_1;
            sdbf_1 = sdbf_2;
            sdbf_2 = tmp;
            bf_count_1 = sdbf_1->bf_count;
    }
    
    tasklist = (sdbf_task_t *) alloc_check( ALLOC_ZERO, thread_cnt*sizeof( sdbf_task_t), "sdbf_score", "tasklist", ERROR_EXIT);
    // Initialize common data for thread task(s)
    for( t=0; t<thread_cnt; t++) {
        tasklist[t].tid = t;
        tasklist[t].tcount = thread_cnt;
        tasklist[t].ref_sdbf = sdbf_1;
        tasklist[t].tgt_sdbf = sdbf_2;
        tasklist[t].done = 0;
    }
    if( thread_cnt > 1) {
        for( t=0; t<thread_cnt; t++) {
                thread_pool[t] = new boost::thread(thread_sdbf_max_score,tasklist+t);
        }
    }
    rand_offset = 1 ;
    srand(time(NULL));
    for( i=0; i< bf_count_1; i++) {
        if (sample > 0 && bf_count_1 > sample)  {
            rand_offset = rand() % (uint32_t)(sdbf_1->bf_count / sample);
         }
        // No threading
        if( thread_cnt < 2) {
            tasklist[0].ref_index=i*rand_offset;
            max_score = sdbf_max_score( tasklist, map_on);
        // === Threading ===
        } else {
            for( t=0; t<thread_cnt; t++) {
                tasklist[t].ref_index=i*rand_offset;
                tasklist[t].sem_start.post();
            }
            for( t=0; t<thread_cnt; t++) {
                tasklist[t].sem_end.wait();
            }
            max_score = tasklist[0].result;
            for( t=1; t<thread_cnt; t++) {
                max_score = (tasklist[t].result > max_score) ? tasklist[t].result : max_score;
            }
        // === Done threading ===
        }
        score_sum = (score_sum < 0) ? max_score : score_sum + max_score;
        if( map_on == FLAG_ON) {
            printf( "  %5.3f\n", max_score);
        }
    }
    uint64_t denom = bf_count_1;
    if (thread_cnt > 1) {
        for (t = 0; t < thread_cnt; t++) {
                tasklist[t].done = 1;
                tasklist[t].sem_start.post();
                thread_pool[t]->join();
        }
        for (t = 0; t < thread_cnt; t++) {
                delete thread_pool[t];
        }
    }
    free(tasklist);
    return (score_sum < 0) ? -1 : boost::math::round( 100.0*score_sum/(denom));
    
}

/**
 * Given a BF and an SDBF, calculates the maximum match (0-100)
 */

double 
sdbf::sdbf_max_score( sdbf_task_t *task, uint32_t map_on) {
    assert( task != NULL);
        
    double score, max_score=-1;
    uint32_t i, s1, s2, min_est, max_est, match, cut_off, slack=48;
    uint32_t bf_size = task->ref_sdbf->bf_size;
    uint16_t *bf_1, *bf_2;

    s1 = get_elem_count( task->ref_sdbf, task->ref_index);
    // Are there enough elements to even consider comparison?
    if( s1 < MIN_ELEM_COUNT)
        return max_score;
    bf_1 = (uint16_t *)(task->ref_sdbf->buffer + task->ref_index*bf_size);
    uint32_t e1_cnt = task->ref_sdbf->hamming[task->ref_index];
    uint32_t comp_cnt = task->tgt_sdbf->bf_count;
    for( i=task->tid; i<comp_cnt; i+=task->tcount) {
        bf_2 = (uint16_t *)(task->tgt_sdbf->buffer + i*bf_size);
        s2 = get_elem_count( task->tgt_sdbf, i);
        if( task->ref_sdbf->bf_count > 1 && s2 < MIN_REF_ELEM_COUNT)
            continue;
        uint32_t e2_cnt = task->tgt_sdbf->hamming[i];

        // Max/min number of matching bits & zero cut off
        max_est = (e1_cnt < e2_cnt) ? e1_cnt : e2_cnt;
        min_est = bf_match_est( 8*bf_size, task->ref_sdbf->hash_count, s1, s2, 0);
        cut_off = boost::math::round( SD_SCORE_SCALE*(double)(max_est-min_est)+(double)min_est);
        // Max/min number of matching bits & zero cut off
        max_est = (e1_cnt < e2_cnt) ? e1_cnt : e2_cnt;
        min_est = bf_match_est( 8*bf_size, task->ref_sdbf->hash_count, s1, s2, 0);
        cut_off = boost::math::round( SD_SCORE_SCALE*(double)(max_est-min_est)+(double)min_est);

        // Find matching bits
        match = bf_bitcount_cut_256( (uint8_t *)bf_1, (uint8_t *)bf_2, cut_off, slack);
        if( match > 0) {
            match = bf_bitcount_cut_256( (uint8_t *)bf_1, (uint8_t *)bf_2, 0, 0);
        }
        score = (match <= cut_off) ? 0 : (double)(match-cut_off)/(max_est-cut_off);
        if( map_on == FLAG_ON && config->thread_cnt == 1) {
            printf( "%s", (score > 0) ? "+" : ".");
        }
        max_score = (score > max_score) ? score : max_score;
    }
    task->result = max_score;

    return max_score;
}
