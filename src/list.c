/* list.c
 * A simple double-linked-list data-structure and manipulation functions
 *
 * Copyright (C) 2012, Giuseppe Leone <joebew42@gmail.com>,
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

#include <stdlib.h>

#include "list.h"

LIST *list_init()
{
    LIST *list = malloc(sizeof(LIST));
    if (list == NULL)
        return NULL;
    
    list->first = list->last = NULL;

    return list;
}

LIST_NODE *list_push(LIST *list, void *data)
{
    /* Allocate memory for the new node list */
    LIST_NODE *node = malloc(sizeof(LIST_NODE));
    if (node == NULL)
        return;

    node->data = data;
    node->next = node->prev = NULL;

    /*
     * If the first element of list is NULL
     * then put it as the first
     */
    if (list->first == NULL) {
        list->first = list->last = node;
    } else {
        /* Put it as last node */
        node->prev = list->last;
        list->last->next = node;
        list->last = node;
    }

    return node;
}

void *list_pop(LIST *list)
{
    /* Empty list */
    if (list->first == NULL)
        return NULL;

    /* Return the data pointed to first node */
    void *data = list->first->data;

    /* Retrieve th first node of the list */
    LIST_NODE *node = list->first;
    
    if (list->first == list->last) {
        list->first = list->last = NULL;
    } else {
        /* Update the reference to first node */
        list->first = node->next;
        list->first->prev = NULL;
    }
    
    free(node);

    return data;
}

void list_remove(LIST* list, LIST_NODE *node)
{   
    if (list->first == NULL)
        return;
    
    if (node == list->first) {
        list->first = list->first->next;
        if (list->first != NULL)
            list->first->prev = NULL;
    } else if (node == list->last) {
        list->last = list->last->prev;
        list->last->next = NULL;
    } else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    free(node);
}

void list_free(LIST *list)
{
    if (list == NULL)
        return;

    LIST_NODE *node = list->first;
    
    while(node) {
        LIST_NODE *next = node->next;
        free(node);
        node = next;
    }
    
    free(list);
}
