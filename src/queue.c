/* queue.c
 * A queue data-structure with its manipulation functions
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

#include <stdlib.h>

#include "queue.h"

Queue *queue_init()
{
    Queue *queue = malloc(sizeof(Queue));
    if (queue == NULL)
        return NULL;

    queue->first = queue->last = NULL;

    return queue;
}

QueueElement *queue_enqueue(Queue *queue, void *data)
{
    QueueElement *element = (QueueElement *)malloc(sizeof(QueueElement));
    if (element == NULL)
        return NULL;

    element->data = data;
    element->next = element->prev = NULL;

    if (queue->first == NULL)
    {
        queue->first = queue->last = element;
    }
    else
    {
        element->prev = queue->last;
        queue->last->next = element;
        queue->last = element;
    }

    return element;
}

void *queue_dequeue(Queue *queue)
{
    if (queue->first == NULL)
        return NULL;

    void *data = queue->first->data;

    QueueElement *element = queue->first;

    if (queue->first == queue->last)
    {
        queue->first = queue->last = NULL;
    }
    else
    {
        queue->first = element->next;
        queue->first->prev = NULL;
    }

    free(element);

    return data;
}

void queue_remove(Queue *queue, QueueElement *element)
{
    if (queue->first == NULL)
        return;

    if (element->prev == NULL)
        queue->first = element->next;
    else
        element->prev->next = element->next;

    if (element->next == NULL)
        queue->last = element->prev;
    else
        element->next->prev = element->prev;

    free(element);
}

int queue_size(Queue *queue)
{
    if (queue == NULL || queue->first == NULL)
        return 0;

    int size = 0;
    QueueElement *element = queue->first;
    while (element != NULL)
    {
        element = element->next;
        ++size;
    }

    return size;
}

void queue_free(Queue *queue)
{
    if (queue == NULL)
        return;

    QueueElement *element = queue->first;

    while (element)
    {
        QueueElement *next = element->next;
        free(element);
        element = next;
    }

    free(queue);
}
