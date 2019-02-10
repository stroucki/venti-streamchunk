/**
 * @file dedup.h
 * @author Pavan Kumar Alampalli
 * @date 15-mar-2013
 * @brief Interface header for dedup library
 *
 * This header file defines the interface for the dedup library.
 * The interface follows the basic init, call-in-a-loop, free 
 * pattern.  It also declares an opaque structure (rabinpoly_t) 
 * that will be used by all the functions in the library to maintain 
 * the state. 
 *
 * Note that the memory allocated by rabin_init() has to be freed
 * the calling rabin_free() in the end. You can reuse the same 
 * rabinpoly_t structure by calling a rabin_reset().
 */

#ifndef _DEDUP_H_
#define _DEDUP_H_ 

/**
 * Rabin fingerprinting algorithm structure declaration
 */
struct rabinpoly;
typedef struct rabinpoly rabinpoly_t;

/**
 * @brief Initializes the rabin fingerprinting algorithm. 
 *
 * This method has to be called in order to create a handle that 
 * can be passed to all other functions in the library. The handle
 * should later be free'ed by passing it to rabin_free(). 
 * 
 * The window size is the size of the sliding window that the 
 * algorithm uses to compute the rabin fingerprint (~32-128 bytes)
 *
 * @param [in] window_size Rabin fingerprint window size in bytes
 * @param [in] avg_segment_size Average desired segment size in KB
 * @param [in] min_segment_size Minumim size of the produced segment in KB
 * @param [in] max_segment_size Maximum size of the produced segment in KB
 *
 * @retval rp Pointer to a allocated rabin_poly_t structure
 * @retval NULL Incase of errors during initialization
 */
rabinpoly_t *rabin_init(unsigned int window_size,
						unsigned int avg_segment_size, 
						unsigned int min_segment_size,
						unsigned int max_segment_size);

/**
 * @brief Find the next segment boundary.
 *
 * Consumes the characters in the buffer and returns when it
 * finds a segment boundary in the given buffer. The segments 
 * defined by this function will never be longer than max_segment_size
 * and will never be shorter than min_segment_size. 
 *
 * It returns the number of bytes processed by the rabin fingerprinting
 * algorithm. Note that it can be <= the number of bytes in the
 * input buffer depending on where the new segment was found (similar
 * to shortcounts in write() system call). So you may need to call 
 * this function in a loop to consume all the bytes in the buffer.
 *
 * @param [in] rp Pointer to the rabinpoly_t structure returned by rabin_init
 * @param [in] buf Pointer to a characher buffer containing data
 * @param [in] bytes Number of bytes to read from the buf
 * @param [out] is_new_segment Pointer to an integer flag indicating segment
 *                             boundary. 1: new segemnt starts here
 *                                       0: otherwise.
 *
 * @retval int Number of bytes processed by the rabin algorithm.
 *         -1  Error     
 */
int rabin_segment_next(rabinpoly_t *rp, 
						const char *buf, 
						unsigned int bytes,
						int *is_new_segment);

/**
 * @brief Resets the Rabin Fingerprinting algorithm's datastructure
 *
 * Call this function to reuse the rp for a different file or stream.
 * It has the same effect as calling rabin_init(), but does not do
 * any allocation of rabinpoly_t.
 *
 * @param [in] rp Pointer to the rabinpoly_t structure returned by rabin_init
 *
 * @retval void None
 */
void rabin_reset(rabinpoly_t *rp);

/**
 * @brief Frees the Rabin Fingerprinting algorithm's datastructure
 *
 * This function should be called at the end to free all the memory 
 * allocated by rabin_init.
 *
 * @param [in] p_rp Address of the pointer returned by rabin_init()
 *
 * @retval void None
 */
void rabin_free(rabinpoly_t **p_rp);

#endif /* _DEDUP_H_ */
