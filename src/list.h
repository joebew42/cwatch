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

#ifndef __LIST_H
#define __LIST_H

/*
  TODO: struct and typedef naming conventions
  http://www.montefiore.ulg.ac.be/~piater/Cours/Coding-Style/index.html#id2465262
*/

typedef struct queue_node_t
{
  void *data;
  struct queue_node_t *prev;
  struct queue_node_t *next;
} QueueNode;

typedef struct queue_t
{
  QueueNode *first;
  QueueNode *last;
} Queue;

/* initialize list data structure
 *
 * @return list * : a pointer to the new allocated list structure
 */
Queue *queue_init();

/* push an element at the end of list
 *
 * @param  Queue *    : a Queue pointer
 * @param  void *    : a void pointer
 * @return QueueNode : return the pointer of the node added
 */
QueueNode *queue_enqueue(Queue *, void *);

/* remove and return the first element of the list
 *
 * @param  Queue *  : a Queue pointer
 * @return void *  : a VOID pointer to destination data pointer
 */
void *queue_dequeue(Queue *);

/* remove a node from list
 *
 * @param Queue *      : a pointer to the list
 * @param QueueNode * : a to the list_node to remove
 */
void queue_remove(Queue *, QueueNode *);

/* returns the size of a list
 *
 * @param  Queue * : list
 * @return int    : number of elements
 */
int queue_size(Queue *);

/* deallocates list data structure */
void queue_free(Queue *);

#endif /* !__LIST_H */
