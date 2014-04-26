#include "cuckoo.h"
#include "assert.h"
#include "stdio.h"
#include "stdlib.h"

/* modified cuckoo hash, based off of John Tromp's original code */

#define STRSIZE 20
char *str_to_hash;

typedef struct edge_chain edge_chain;

struct edge_chain {
	int        u_disconnected; /* which node we need to look at for finding the next node (1 to look for u, 0 for v */
	node_t     u_cur_node;  /* current node value */
	node_t     v_cur_node;  /* current node value */
	u64        nonce;  /* nonce that was used to create edge */
	u64        j;  /* the product of all previous nonces */
	int        count;  /* the # of nonces in chain */
	edge_chain *prev_edge; /* previous edge in chain */
	edge_chain *next_edge_chain;/* doubly linked list of edge chains */
	edge_chain *free_edge; /*fast way to quickly free every edge*/
};

typedef struct {
	edge_chain *edge;
} storage_array;

u64 chain_count = 0;
u64 edge_count = 0;

/* only keep the active edges in here */
storage_array *store_u;
storage_array *store_v;

/* only store single values on the first pass */
uint first_pass = 0;

/* proof size to seek out first */
uint current_seeking_proofsize = PROOFSIZE;

/*
 * Used for getting amount of memory usage
 * via http://stackoverflow.com/questions/1558402/memory-usage-of-current-process-in-c
 */
typedef struct {
    unsigned long size,resident,share,text,lib,data,dt;
} statm_t;

void read_off_memory_status(statm_t *result)
{
  const char* statm_path = "/proc/self/statm";

  FILE *f = fopen(statm_path,"r");
  if(!f){
    perror(statm_path);
    abort();
  }
  if(7 != fscanf(f,"%ld %ld %ld %ld %ld %ld %ld",
    &result->size,&result->resident,&result->share,
	&result->text,&result->lib,&result->data,&result->dt))
  {
    perror(statm_path);
    abort();
  }
  fclose(f);
}

/* search through existing chains for a match
 * returns: chain to add to, or NULL if one is not found
 */
edge_chain *lookup_chains(node_t edge_u, node_t edge_v) {
	/* first check u, then v edge, should return NULL if V edge isn't found as array is calloc'd */
	if (store_u[edge_u].edge != NULL) {
		edge_chain *current_chain = store_u[edge_u].edge;
		if (current_chain != NULL) {
			edge_chain *next_store = store_u[edge_u].edge->next_edge_chain;
			store_u[edge_u].edge = next_store;
		}
		/* for right now, just return the first one here and pop from list */
		return current_chain;
	} else {
		edge_chain *current_chain = store_v[edge_v].edge;
		if (current_chain != NULL) {
			edge_chain *next_store = store_v[edge_v].edge->next_edge_chain;
			store_v[edge_v].edge = next_store;
		}
		/* for right now, just return the first one here and pop from list */
		return current_chain;
	}
}

/* add to our array of chains */
void add_to_chain(edge_chain *cur,edge_chain *ec) {
	if (cur == NULL) {
		// new edge chain 
		chain_count++;
	}
	edge_count++;
	
	if (ec->u_disconnected) {
		if (store_u[ec->u_cur_node].edge != NULL) {
			ec->next_edge_chain = store_u[ec->u_cur_node].edge;
		}
		store_u[ec->u_cur_node].edge = ec;
		
		//we also have to remove the current edge from the store arrays
		/*if (cur!= NULL) {
			edge_chain *comp = store_v[cur->v_cur_node].edge;
			if (cur == comp) {
				//first in list
				store_v[cur->v_cur_node].edge = store_v[cur->v_cur_node].edge->next_edge_chain;
			} else {
				//somewhere else in list
				edge_chain *prev = NULL;
				while (1) {
					if (cur == comp) {
						prev->next_edge_chain = comp->next_edge_chain;
						break;
					}
					prev = comp;
					comp = comp->next_edge_chain;
				}
			}
		} shouldn't need this as we pop off the list above */		
	} else {
		if (store_v[ec->v_cur_node].edge != NULL) {
			ec->next_edge_chain = store_v[ec->v_cur_node].edge;
		}
		store_v[ec->v_cur_node].edge = ec;
		
		//we also have to remove the current edge from the store arrays
		/*
		if (cur!= NULL) {
			edge_chain *comp = store_u[cur->u_cur_node].edge;
			if (cur == comp) {
				//first in list
				store_u[cur->u_cur_node].edge = store_u[cur->u_cur_node].edge->next_edge_chain;
			} else {
				//somewhere else in list
				edge_chain *prev = NULL;
				while (1) {
					if (cur == comp) {
						prev->next_edge_chain = comp->next_edge_chain;
						break;
					}
					prev = comp;
					comp = comp->next_edge_chain;
				}
			}
		} shouldn't need this as we pop off the list above*/	
	}
}

/* sha256 hash help - http://stackoverflow.com/questions/2262386/generate-sha256-with-openssl-and-c */
unsigned char sha_hash[SHA256_DIGEST_LENGTH];
int best_z_count = 0;

void dump_solution(siphash_ctx *ctx,edge_chain *cur) {
	char outputBuffer[65];
	
	
    SHA256_CTX sha256;
	char *tohash = calloc(100,sizeof(char));
    SHA256_Init(&sha256);
	/* add the string to hash first */
	SHA256_Update(&sha256, str_to_hash, strlen(str_to_hash));
	
	edge_chain *save_cur = cur;
	while (1) {
		if (cur == NULL) { break; }
		snprintf(tohash,100,"%d: u:%llu v:%llu nonce:%llu j:%llu\n", 
			cur->count, (long long unsigned int)cur->u_cur_node, 
			(long long unsigned int)cur->v_cur_node, (long long unsigned int)cur->nonce,
			(long long unsigned int)cur->j);
		SHA256_Update(&sha256, tohash, strlen(tohash));
		cur = cur->prev_edge;
	}
	cur = save_cur;
	
    SHA256_Final(sha_hash, &sha256);
    int i = 0;
	int zero_count = 0;
	char z[] = "0";
	/* form our sha256 into a string */
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02X", sha_hash[i]);
    }
	/* count the number of 0's */
	for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
		if (zero_count == i && strncmp(z,outputBuffer + i,1) == 0 ) {
			zero_count++;
		}
	}
    outputBuffer[64] = 0;
	if (zero_count > best_z_count) {
		best_z_count = zero_count;
	} else {
#ifndef DEBUGPRINT
		return;//return without printing in the normal case
#endif
	}
	
	printf("\nstring: %s",str_to_hash);
	printf("hash: %s (%d zeros)\n",outputBuffer, zero_count);
	
	printf("Sol:\n");
	statm_t  *result = calloc(1,sizeof(statm_t));
	read_off_memory_status(result);
	printf("Size:%u Memory:%lu Chains:%llu Edges:%llu\n",
		   current_seeking_proofsize,result->size,
		   (long long unsigned int)chain_count,(long long unsigned int)edge_count);
	free(result);
	
	while (1) { 
		if (cur == NULL) { break; }
		
		if (cur->u_disconnected) {
			printf("%d: u:%llu(recomputed) v:%llu nonce:%llu j:%llu\n", 
			cur->count, (long long unsigned int)cur->u_cur_node, 
			(long long unsigned int)cur->v_cur_node, (long long unsigned int)cur->nonce,
			(long long unsigned int)cur->j);
		} else {
			printf("%d: u:%llu v:%llu(recomputed) nonce:%llu j:%llu\n", 
			cur->count, (long long unsigned int)cur->u_cur_node, 
			(long long unsigned int)cur->v_cur_node, (long long unsigned int)cur->nonce,
			(long long unsigned int)cur->j);
		}
		
		/* show j value product worked */
		/*printf("what u would be:%llu, what v would be:%llu\n",
			(long long unsigned int)sipnode(ctx, cur->nonce,0),
			(long long unsigned int)sipnode(ctx, 2*cur->nonce,1));*/

		cur = cur->prev_edge;
	}

}

int find_solution(siphash_ctx *ctx,edge_chain *cur) {
	node_t final_val;
	edge_chain *temp = cur;
	
	/* first get final value */
	if (cur->u_disconnected) {
		final_val = cur->u_cur_node;
	} else {
		printf("we can only look for solutions with an even number of nodes!\n");
		assert(0); /* we can only look for solutions with an even number of nodes! */
	}
	
	while (1) { 
		if (cur == NULL) { break; }
		
		if (cur->prev_edge == NULL) { /*first value */
			if (cur->u_cur_node == final_val) {
				/* we have a solution */
					dump_solution(ctx, temp);
					//current_seeking_proofsize = current_seeking_proofsize + 2;
					return 1;
			}
		}
		cur = cur->prev_edge;
	}
	
	return 0;
}

edge_chain *last_alloc = NULL;

/* a function to search existing edges and find a match */
int find_edge(siphash_ctx *ctx, nonce_t nonce,node_t edge_u, node_t edge_v) {
	edge_chain *current_chain = NULL;

	while (1) {
		edge_chain *ec = NULL;
		/* find next edge to add to, will continuously lookup possible edges until no more */
		current_chain = lookup_chains(edge_u, edge_v);
		
		/* add to existing chain, or form new one */
		if (current_chain == NULL) {
			if (first_pass != 0) {
				/* stop forming new chains after a first pass through all possible nonce values */
				break;
			}
			/* no edge found, or 1st lookup */
			ec = calloc(1,sizeof(edge_chain));
			ec->free_edge = last_alloc;
			last_alloc = ec;
			if (!ec) {return 0;}
			ec->u_cur_node = edge_u;
			ec->v_cur_node = edge_v;
			ec->j = 1;
			ec->count = 1;	
		} else {
			if (current_chain->count == current_seeking_proofsize) {
				/* stop forming chains longer than the one we are looking for */
				break;
			}
			ec = calloc(1,sizeof(edge_chain));
			ec->free_edge = last_alloc;
			last_alloc = ec;
			if (!ec) {return 0;}
			ec->u_cur_node = edge_u;
			ec->v_cur_node = edge_v;
			ec->j = (current_chain->j * nonce) % JMOD;
			ec->nonce = nonce;
			ec->u_disconnected = 1 - current_chain->u_disconnected;
			//TODO - recompute u/v value
			if (current_chain->u_disconnected) {
				/* recompute v edge with j value */
				ec->v_cur_node = sipnode(ctx, 2*ec->j*nonce,1);
			} else {
				/* recompute u edge with j value */
				ec->u_cur_node = sipnode(ctx, ec->j*nonce,1);
			}
			ec->count = current_chain->count + 1;
			
			ec->prev_edge = current_chain; //add to linked list of edges
			if (ec->count >= current_seeking_proofsize && ec->count % 2 == 0) {
				//printf("total partial solutions(%d): %llu\n", ec->count, (long long unsigned int)chain_count);
				if (find_solution(ctx,ec) == 1) {
					return 1;
				}
			}
		}
		
		add_to_chain(current_chain,ec);
		
		if (current_chain == NULL) {
			/* we are forming a new edge at this location */
			break;
		}
	}
	
	return 0;
}


int main(int argc, char *argv[]) {

	str_to_hash = calloc(STRSIZE,sizeof(char));
	
	printf("Running Cuckoo cycle finder with parameters\n");
		printf("SIZESHIFT: %llu, SIZE: %llu possible values, STORAGE SIZE: %llu bytes Possible nonce values: %llu\n",
				(long long unsigned int)SIZESHIFT, (long long unsigned int)SIZE, 
				(long long unsigned int)HALFSIZE * 2 * sizeof(storage_array),
				(long long unsigned int)NONCEMAX);

	/* print parameters */
	int mining_nonce = 1;
	while (1) {
	
		store_u = calloc(HALFSIZE,sizeof(storage_array));
		store_v = calloc(HALFSIZE,sizeof(storage_array));
		if (store_u == NULL || store_v == NULL) {
			printf("Failure to allocate main memory arrays\n");
			return 0;
		}
	
		snprintf(str_to_hash,STRSIZE,"%d EXAMPLE BLOCK\n",mining_nonce);
		mining_nonce++;
		
		
		
		/* create the siphash parameters */
		siphash_ctx ctx;
		setheader(&ctx, str_to_hash);
		
		/* find the cycle */
		unsigned int nonce;
		while (1) {
		  for (nonce = 0; nonce < NONCEMAX; nonce++) {
			node_t edge_u =  sipnode(&ctx, nonce,0);
			node_t edge_v =  sipnode(&ctx, 2*nonce,1);
			if (find_edge(&ctx, nonce,edge_u,edge_v) == 1) {
				goto sol_found;
			}
			//printf("%d: h %llu %llu\n",n, (long long unsigned int)bottom, (long long unsigned int) top);
		  }
		  first_pass++;
		}

/* goto tag sol_found */
sol_found: ;

		/* This is a silly way to free memory, but at this point in time the code below actually walking 
			the data structure doesn't fully work? */
		int i = 0;
		while (last_alloc != NULL) {
			edge_chain *temp = last_alloc->free_edge;
			free(last_alloc);
			i++;
			last_alloc = temp;
		}
		/* free all of the memory we just used */
		/*for (nonce = 0; nonce < HALFSIZE; nonce++) {
			while (store_u[nonce].edge != NULL) {
				edge_chain *next = store_u[nonce].edge->next_edge_chain;
				edge_chain *cur = store_u[nonce].edge;
				while (cur != NULL) {
					edge_chain * temp = cur->prev_edge;
					free(cur);
					cur = temp;
				}
				store_u[nonce].edge = next;
			}
			 while (store_v[nonce].edge != NULL) {
				edge_chain *next = store_v[nonce].edge->next_edge_chain;
				edge_chain *cur = store_v[nonce].edge;
				while (cur != NULL) {
					edge_chain *temp = cur->prev_edge;
					free(cur);
					cur = temp;
				}
				store_v[nonce].edge = next;
			}
		}*/
		
		free(store_u);
		free(store_v);
		
		edge_count = 0;
		chain_count = 0;
		
	}
	
	
	return 0;
}
	
	
	
