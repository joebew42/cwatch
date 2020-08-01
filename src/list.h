/* list.h
 * A simple double-linked-list data-structure and manipulation functions
 *
 * Copyright (C) 2012, Joe Bew <joebew42@gmail.com>,
 *                     Vincenzo Di Cicco <enzodicicco@gmail.com>
 *
 * This file is part of cwatch
 *
 * cwatch is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * cwatch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __QUEUE_H
#define __QUEUE_H

/*
  TODO: struct and typedef naming conventions
  http://www.montefiore.ulg.ac.be/~piater/Cours/Coding-Style/index.html#id2465262
*/

typedef struct queue_node_t
{
  void *data;
  struct queue_node_t *prev;
  struct queue_node_t *next;
} QueueElement;

typedef struct queue_t
{
  QueueElement *first;
  QueueElement *last;
} Queue;

/* initialize a queue
 *
 * @return Queue * : a pointer to the new queue
 */
Queue *queue_init();

/* add an element at the end of the queue
 *
 * @param  Queue *        : a Queue pointer
 * @param  void *         : a void pointer
 * @return QueueElement * : return the pointer of the element added
 */
QueueElement *queue_enqueue(Queue *, void *);

/* removes and returns the element at the beginning of the queue
 *
 * @param  Queue *  : a Queue pointer
 * @return void *   : a VOID pointer to destination data pointer
 */
void *queue_dequeue(Queue *);

/* removes an element from the queue
 *
 * @param Queue *         : a pointer to the queue
 * @param QueueElement *  : a pointer to the element to remove
 */
void queue_remove(Queue *, QueueElement *);

/* returns the size of a queue
 *
 * @param  Queue * : queue
 * @return int     : number of elements
 */
int queue_size(Queue *);

/* deallocates queue */
void queue_free(Queue *);

#endif /* !__QUEUE_H */
