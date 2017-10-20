/*
 * Copyright 2017 Rafal Zajac <rzajac@gmail.com>.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include <esp_eb.h>
#include <mem.h>
#include <user_interface.h>
#include <esp_tim.h>

// Structure defining event.
typedef struct
{
  char *name;           // Event name.
  esp_eb_cb *cb;        // Event callback function.
  uint32_t ctime_us;    // Last time callback was called.
  uint32_t throttle_us; // Throttle callback calls (0 - no throttle).
                        // The minimum number of microseconds to wait between callback executions.
  void *payload;        // Passed to callback.
} eb_event;

// Linked list of attached events.
typedef struct node
{
  eb_event *event;   // The event.
  struct node *next; // The pointer to the next node on the list.
} eb_node;

// The linked list head node.
static eb_node *head;

/**
 * Duplicate string.
 *
 * User must take care of releasing memory allocated to
 * string duplicate.
 *
 * @param str The string to duplicate.
 */
static char *ICACHE_FLASH_ATTR
esp_eb_strdup(const char *str)
{
  size_t len;
  char *copy;

  len = strlen(str) + 1;
  if (!(copy = os_zalloc(len))) return 0;
  memcpy(copy, str, len);

  return copy;
}

/**
 * Create new event structure.
 *
 * @param event_name  The event name.
 * @param cb          The callback.
 * @param throttle_us Throttle callback calls (0 - no throttle).
 */
static eb_event *ICACHE_FLASH_ATTR
event_new(const char *event_name, esp_eb_cb *cb, uint32_t throttle_us)
{
  eb_event *new = os_zalloc(sizeof(eb_event));
  if (new == NULL) return NULL;

  new->name = esp_eb_strdup(event_name);
  new->cb = cb;
  new->throttle_us = throttle_us;

  return new;
}

/**
 * Release memory allocated for en event structure.
 *
 * @param event The event structure.
 */
static void ICACHE_FLASH_ATTR
free_event(eb_event *event)
{
  os_free(event->name);
  os_free(event);
}

/**
 * Create new event list node.
 *
 * @param event_name  The event name.
 * @param cb          The callback.
 * @param throttle_us Throttle callback calls (0 - no throttle).
 */
static eb_node *ICACHE_FLASH_ATTR
new_node(const char *event_name, esp_eb_cb *cb, uint32_t throttle_us)
{
  eb_event *event = event_new(event_name, cb, throttle_us);
  if (event == NULL) return NULL;

  eb_node *new_node = os_zalloc(sizeof(eb_node));
  if (new_node == NULL) {
    os_free(event);
    return NULL;
  }
  new_node->event = event;

  return new_node;
}

/**
 * Release memory allocated for node list.
 *
 * @param node The node.
 */
static void ICACHE_FLASH_ATTR
free_node(eb_node *node)
{
  free_event(node->event);
  os_free(node);
}

/**
 * Find node on the list.
 *
 * Cases:
 *                ret,  prev
 *  - empty list: NULL, NULL
 *  - head:       head, NULL
 *  - not found:  NULL, tail
 *  - found:      node, prev
 *
 * @param event The event name.
 * @param cb    The event callback.
 * @param prev  The previous node to the found one.
 *
 * @return The found node or NULL
 */
static eb_node *ICACHE_FLASH_ATTR
find_node(const char *event_name, esp_eb_cb *cb, eb_node **prev)
{
  eb_node *curr = head;
  if (prev != NULL && *prev != NULL) *prev = NULL;

  // Empty list.
  if (curr == NULL) return NULL;

  while (curr) {
    if (strcmp(curr->event->name, event_name) == 0 && curr->event->cb == cb) break;
    if (prev != NULL) *prev = curr;
    curr = curr->next;
  }

  return curr;
}

esp_eb_err ICACHE_FLASH_ATTR
esp_eb_attach_throttled(const char *event_name, esp_eb_cb *cb, uint32_t throttle_us)
{
  eb_node *tail = NULL;
  eb_node *node = find_node(event_name, cb, &tail);

  // Empty list.
  if (node == NULL && tail == NULL) {
    head = new_node(event_name, cb, throttle_us);
    if (head == NULL) return ESP_EB_ATTACH_MEM;
    ESP_EB_DEBUG("Added head (%p) %s %d %p\n", head, event_name, throttle_us, cb);

    return ESP_EB_ATTACH_OK;
  }

  // If node already exists we return.
  if (node != NULL) return ESP_EB_ATTACH_EXISTED;

  tail->next = new_node(event_name, cb, throttle_us);
  if (tail->next == NULL) return ESP_EB_ATTACH_MEM;

  ESP_EB_DEBUG("Added %s %d %p\n", event_name, throttle_us, cb);
  return ESP_EB_ATTACH_OK;
}

esp_eb_err ICACHE_FLASH_ATTR
esp_eb_attach(const char *event, esp_eb_cb *cb)
{
  return esp_eb_attach_throttled(event, cb, 0);
}

bool ICACHE_FLASH_ATTR
esp_eb_detach(const char *event_name, esp_eb_cb *cb)
{
  eb_node *prev = NULL;
  eb_node *curr = find_node(event_name, cb, &prev);

  // Not found.
  if (curr == NULL) return true;
  if (prev != NULL) prev->next = curr->next;
  if (curr == head) head = curr->next;
  free_node(curr);

  ESP_EB_DEBUG("Detached node %s %p\n", event_name, cb);
  return true;
}

bool ICACHE_FLASH_ATTR
esp_eb_remove_cb(esp_eb_cb *cb)
{
  eb_node *prev = NULL;
  eb_node *curr = head;

  while (curr) {
    if (curr->event->cb == cb) break;
    prev = curr;
    curr = curr->next;
  }

  // Node not found or empty list.
  if (curr == NULL) return true;

  if (prev) prev->next = curr->next;
  if (curr == head) head = curr->next;
  free_node(curr);

  ESP_EB_DEBUG("Detached node %s %p\n", curr->event->name, curr->event->cb);
  return true;
}

/**
 * Event timer callback.
 *
 * @param arg
 */
static void ICACHE_FLASH_ATTR
timer_cb(void *arg)
{
  esp_tim_timer *timer = arg;
  eb_event *event = timer->payload;

  // Event no longer exists.
  eb_node *node = find_node(event->name, event->cb, NULL);
  if (node == NULL) {
    free_event(event);
    return;
  }

  uint32_t now = system_get_time();
  uint32_t diff = now - node->event->ctime_us;

  if (event->throttle_us == 0) {
    node->event->ctime_us = now;
    event->cb(event->name, event->payload);
  } else if (event->throttle_us > 0 && diff >= event->throttle_us) {
    node->event->ctime_us = now;
    event->cb(event->name, event->payload);
  }

  free_event(event);
  esp_tim_stop(timer);
}

static bool ICACHE_FLASH_ATTR
timer_start(const eb_node *node, void *payload, uint32_t delay)
{
  // We are scheduling callback, by the time it is called
  // it is possible node will no longer exist.
  eb_event *event = event_new(node->event->name, node->event->cb, node->event->throttle_us);
  if (!event) return false;
  event->payload = payload;

  ESP_EB_DEBUG("Scheduling %s in %d ms.\n", node->event->name, delay);
  return esp_tim_start_delay(timer_cb, event, delay);
}

void ICACHE_FLASH_ATTR
esp_eb_trigger_delayed(const char *event_name, uint32_t delay, void *arg)
{
  eb_node *curr = head;

  while (curr) {
    if (strcmp(curr->event->name, event_name) == 0) {
      if (timer_start(curr, arg, delay) == false) {
        ESP_EB_ERROR("Scheduling delayed timer timer for %s failed.\n", event_name);
      }
    }
    curr = curr->next;
  }
}

void ICACHE_FLASH_ATTR
esp_eb_trigger(const char *event, void *arg)
{
  esp_eb_trigger_delayed(event, ESP_EB_TIMER_MS, arg);
}

void ICACHE_FLASH_ATTR
esp_eb_print_list()
{
  os_printf("List state:\n");
  eb_node *node = head;
  while (node != NULL) {
    os_printf("    %s %p\n", node->event->name, node->event->cb);
    node = node->next;
  }
}
