#include "queue_array.h"

/******************************************************
 *                      Macros
 ******************************************************/


/******************************************************
 *                    Constants
 ******************************************************/


/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/


/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/
queue_t *create_queue(int maxsize)
{
	queue_t *q;

	q = malloc(sizeof(queue_t));
	if(q == NULL) {
		printf( "Out of space!!!" );
		return NULL;
	}
	q->array = malloc(sizeof(q_elem_t) * maxsize);
	if(q->array == NULL) {
		printf( "Out of Space!!!" );
		return NULL;
	}
	q->maxsize = maxsize;
	q->size = 0;
	q->front = 1;
	q->rear = 0;
	return q;
}

void destroy_queue(queue_t *q)
{
	if (q != NULL)
	{
		free(q->array);
		free(q);
	}
}

int queue_is_full(queue_t *q )
{
  return q->size == q->maxsize;
}

int queue_is_empty(queue_t *q)
{
  return q->size == 0;
}

q_elem_t *get_front_elem(queue_t *q)
{		
	if(queue_is_empty(q)) {
		printf( "Empty queue" );
		return NULL;
	}
	return &q->array[q->front];
}

q_elem_t *get_rear_elem(queue_t *q)
{
	if(queue_is_full(q)) {
		printf( "Full queue" );
		return NULL;
	}
	return &q->array[q->rear];
}

bool en_queue(queue_t *q, q_elem_t *elem)  
{  
	if(queue_is_full(q)) {
		return false;
	}
	if(elem != NULL) {
		q->array[q->rear] = *elem;
	}
	q->rear = (q->rear+1)%q->maxsize;
	q->size++;
	return true;
}

bool de_queue(queue_t *q, q_elem_t *elem)
{  
	if(queue_is_empty(q)) {
		return false;
	}
	if(elem != NULL) {
		*elem = q->array[q->front];
	}
	q->front = (q->front+1)%q->maxsize;
	q->size--;
	return true;
}

q_elem_t *get_front_dequeue(queue_t *q)
{
	q_elem_t *temp = 0;
	if(queue_is_empty(q)) {
    	printf("Empty queue");
		return NULL;
	}
	temp = &q->array[q->front];
	q->front = (q->front+1)%q->maxsize;
	q->size--;
	return temp;
}