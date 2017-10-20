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

#include <esp_cmd.h>
#include <mem.h>

// Structure used to track TCP connections.
typedef struct {
  struct espconn *conn; // The tracked connection.
  uint8_t index;        // The index in connection pool.
  uint8 rem_ip[4];      // The remote IP associated with the connection.
  int rem_port;         // The remote port associated with the connection.
} tcp_conn;

// Structure used during scheduled server stop.
typedef struct {
  os_timer_t *timer;
  esp_cmd_stop_cb *stop_cb;
  void *payload;
} disc_timer;

// The command TCP server.
static struct espconn *tcp_server;

// The connection pool.
// We keep track of all connections so we can
// force close them and stop the command server.
static tcp_conn *conn_pool[ESP_CMD_MAX_CONN];

// The command handler callback.
static esp_cmd_cb *cmd_cb;

/**
 * Find connection in connection pool.
 *
 * The connection is searched by IP and port and if found the
 * tcp_conn_t->conn gets updated with the passed one.
 *
 * We update connection because it might be different in
 * each of the espconn callbacks.
 *
 * @param conn      The connection.
 * @param rem_ip    The connection remote IP address.
 * @param rem_port  The connection remote port.
 *
 * @return Returns NULL when connection is not found.
 */
static tcp_conn *ICACHE_FLASH_ATTR
find_conn(struct espconn *conn, uint8 *rem_ip, int rem_port)
{
  uint8_t i;
  for (i = 0; i < ESP_CMD_MAX_CONN; i++) {
    if (conn_pool[i] && conn_pool[i]->rem_port == rem_port && memcmp(conn_pool[i]->rem_ip, rem_ip, 4) == 0) {
      conn_pool[i]->conn = conn;

      return conn_pool[i];
    }
  }

  ESP_CMD_ERROR("Unknown connection: %d.%d.%d.%d:%d\n",
                IP2STR(conn->proto.tcp->remote_ip),
                conn->proto.tcp->remote_port);

  return NULL;
}

static void ICACHE_FLASH_ATTR
disconnect_conn_cb(void *arg)
{
  sint8_t err;
  disc_timer *dt = (disc_timer *) arg;

  err = espconn_disconnect((struct espconn *) dt->payload);
  if (err) ESP_CMD_ERROR("Disconnection failure (%d).\n", err);

  os_timer_disarm(dt->timer);
  os_free(dt->timer);
  os_free(dt);
}

static bool ICACHE_FLASH_ATTR
schedule_disconnect(struct espconn *conn)
{
  disc_timer *dt = os_zalloc(sizeof(disc_timer));
  if (!dt) return false;

  dt->timer = os_zalloc(sizeof(os_timer_t));
  if (dt->timer == NULL) {
    os_free(dt);
    return false;
  }
  dt->payload = (void *) conn;

  os_timer_setfn(dt->timer, disconnect_conn_cb, (void *) dt);
  os_timer_arm(dt->timer, 10, false);

  return true;
}

/**
 * Called when data is received on TCP connection.
 *
 * @param tcp_conn The TCP connection the data arrived at.
 * @param cmd      The received data.
 * @param length   The received data length.
 */
static void ICACHE_FLASH_ATTR
receive_cb(void *tcp_conn, char *cmd, unsigned short length)
{
  struct espconn *conn = tcp_conn;
  sint8 err;
  char *resp_buff = os_zalloc(ESP_CMD_MAX_RESP_LEN);
  uint16 resp_buff_use;

  ESP_CMD_DEBUG("REC: %s (%d)\n", cmd, length);

  // Call user callback.
  if (cmd_cb != NULL) {
    resp_buff_use = cmd_cb((uint8_t *) resp_buff, ESP_CMD_MAX_RESP_LEN, (const uint8_t *) cmd, length);

    // Return response.
    if (resp_buff_use > 0) {
      err = espconn_send(conn, (uint8 *) resp_buff, resp_buff_use);
      if (err) ESP_CMD_ERROR("Sending response failed with error %d.\n", err);
    }
  }

  schedule_disconnect(conn);
  os_free(resp_buff);
}

/**
 * Called when TCP client disconnects.
 *
 * @param tcp_conn The connection that has been disconnected.
 */
static void ICACHE_FLASH_ATTR
disconnect_cb(void *tcp_cn)
{
  uint8_t i;
  struct espconn *conn = tcp_cn;
  ESP_CMD_DEBUG("DISC: %d.%d.%d.%d:%d\n",
                IP2STR(conn->proto.tcp->remote_ip),
                conn->proto.tcp->remote_port);

  tcp_conn *cp = find_conn(conn, conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port);
  if (cp == NULL) {
    // This should not happen.
    return;
  }

  i = cp->index;
  os_free(conn_pool[i]);
  conn_pool[i] = NULL;
}

/**
 * Callback when data is successfully sent via TCP connection.
 *
 * @param tcp_cn The pointer to corresponding connection structure.
 */
static void ICACHE_FLASH_ATTR
sent_cb(void *tcp_cn)
{
  struct espconn *conn = tcp_cn;
  ESP_CMD_DEBUG("SENT: %d.%d.%d.%d:%d\n", IP2STR(conn->proto.tcp->remote_ip), conn->proto.tcp->remote_port);
  // Called just to update the tcp_cn->conn.
  find_conn(conn, conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port);
}

/**
 * Callback when TCP connection broke.
 *
 * @param tcp_cn The pointer to corresponding connection structure.
 * @param err    The error code.
 */
static void ICACHE_FLASH_ATTR
reconnect_cb(void *tcp_cn, sint8 err)
{
  struct espconn *conn = tcp_cn;
  ESP_CMD_DEBUG("RECO: %d.%d.%d.%d:%d (%d)\n", IP2STR(conn->proto.tcp->remote_ip), conn->proto.tcp->remote_port, err);
  // Called just to update the tcp_cn->conn.
  find_conn(conn, conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port);
}

/**
 * Called on successful TCP connection.
 *
 * @param tcp_cn The ne TCP connection.
 */
static void ICACHE_FLASH_ATTR
connect_cb(void *tcp_cn)
{
  uint8_t i;
  struct espconn *conn = tcp_cn;
  ESP_CMD_DEBUG("CONN: %d.%d.%d.%d:%d\n", IP2STR(conn->proto.tcp->remote_ip), conn->proto.tcp->remote_port);

  // Find empty slot in connection pool.
  for (i = 0; i < ESP_CMD_MAX_CONN; i++) if (conn_pool[i] == NULL) break;
  if (i == ESP_CMD_MAX_CONN) {
    // This should never happen because we used
    // espconn_tcp_set_max_con_allow but you never know :)
    ESP_CMD_ERROR("Connection limit reached.\n");
    schedule_disconnect(conn);
    return;
  }

  conn_pool[i] = os_zalloc(sizeof(tcp_conn));
  if (conn_pool[i] == NULL) {
    ESP_CMD_ERROR("Out of memory.\n");
    // We do not even try to disconnect.
    return;
  }
  conn_pool[i]->index = i;
  conn_pool[i]->conn = conn;
  conn_pool[i]->rem_port = conn->proto.tcp->remote_port;
  os_memcpy(conn_pool[i]->rem_ip, conn->proto.tcp->remote_ip, 4);

  // Register data receive callback.
  espconn_regist_recvcb(conn, receive_cb);
  // Register reconnect callback.
  espconn_regist_reconcb(conn, reconnect_cb);
  // Register disconnect callback.
  espconn_regist_disconcb(conn, disconnect_cb);
  // Register successful data send callback
  espconn_regist_sentcb(conn, sent_cb);

  // https://github.com/jeelabs/esp-link/blob/4222cbf12abf4e20ad6ca0a9e552cef050eaa608/httpd/httpd.c#L611
  espconn_set_opt(conn, ESPCONN_REUSEADDR | ESPCONN_NODELAY);
}

sint8 ICACHE_FLASH_ATTR
esp_cmd_start(int port, uint8 max_conn, esp_cmd_cb *cb)
{
  if (tcp_server != NULL) return ESP_CMD_ERR_ALREADY_STARTED;

  tcp_server = os_zalloc(sizeof(struct espconn));
  if (tcp_server == NULL) return ESPCONN_MEM;

  tcp_server->type = ESPCONN_TCP;
  tcp_server->state = ESPCONN_NONE;

  tcp_server->proto.tcp = os_zalloc(sizeof(esp_tcp));
  if (tcp_server->proto.tcp == NULL) return ESPCONN_MEM;
  tcp_server->proto.tcp->local_port = port;

  espconn_regist_connectcb(tcp_server, connect_cb);
  espconn_regist_reconcb(tcp_server, reconnect_cb);

  sint8 err = espconn_accept(tcp_server);
  if (err != ESPCONN_OK) {
    os_free(tcp_server->proto.tcp);
    os_free(tcp_server);
    tcp_server = NULL;
    return err;
  }

  cmd_cb = cb;
  espconn_tcp_set_max_con_allow(tcp_server, max_conn);

  ESP_CMD_DEBUG("Command server *:%d started with max_conn: %d.\n", port, max_conn);

  return ESPCONN_OK;
}

sint8 ICACHE_FLASH_ATTR
esp_cmd_stop()
{
  uint8_t i;
  sint8_t err;
  esp_cmd_cb *cmd_cb_back;
  sint8 max_conn_back;

  if (tcp_server == NULL) return ESP_CMD_ERR_ALREADY_STOPPED;

  // Prepare backups in case we have to backtrack.
  cmd_cb_back = cmd_cb;
  cmd_cb = NULL; // Don't call user callback anymore.
  max_conn_back = espconn_tcp_get_max_con_allow(tcp_server);

  // No more connections allowed.
  espconn_tcp_set_max_con_allow(tcp_server, 0);

  for (i = 0; i < ESP_CMD_MAX_CONN; i++) {
    if (conn_pool[i] == NULL) continue;

    err = espconn_disconnect(conn_pool[i]->conn);
    if (err == ESPCONN_OK) {
      ESP_CMD_DEBUG("Force close %d.%d.%d.%d:%d OK.\n",
                    IP2STR(conn_pool[i]->conn->proto.tcp->remote_ip),
                    conn_pool[i]->conn->proto.tcp->remote_port);
    } else {
      ESP_CMD_ERROR("Closing connection %d.%d.%d.%d:%d (%d) failed.\n",
                    IP2STR(conn_pool[i]->conn->proto.tcp->remote_ip),
                    conn_pool[i]->conn->proto.tcp->remote_port,
                    err);
    }

    os_free(conn_pool[i]);
  }

  //err = espconn_abort(&tcp_server); // OK but port still open
  //err = espconn_delete(&tcp_server); // Does not work (err -5)
  err = espconn_disconnect(tcp_server); // OK but port still open
  if (err == ESPCONN_OK) {
    ESP_CMD_DEBUG("Server stopped.\n");
    os_free(tcp_server->proto.tcp);
    os_free(tcp_server);
    tcp_server = NULL;
  } else {
    ESP_CMD_ERROR("Failed to stop server (%d).\n", err);
    // Restore user callback and max connections values.
    cmd_cb = cmd_cb_back;
    espconn_tcp_set_max_con_allow(tcp_server, (uint8) max_conn_back);
  }

  return err;
}

static void ICACHE_FLASH_ATTR
server_stop_cb(void *arg)
{
  disc_timer *dt = (disc_timer *) arg;
  os_timer_disarm(dt->timer);
  os_free(dt->timer);

  sint8 err = esp_cmd_stop();
  if (dt->stop_cb) dt->stop_cb(err);
  os_free(dt);
}

sint8 ICACHE_FLASH_ATTR
esp_cmd_schedule_stop(esp_cmd_stop_cb *cb)
{
  if (tcp_server == NULL) return ESP_CMD_ERR_ALREADY_STOPPED;

  disc_timer *dt = os_zalloc(sizeof(disc_timer));
  if (!dt) return ESPCONN_MEM;

  dt->timer = os_zalloc(sizeof(os_timer_t));
  if (dt->timer == NULL) {
    os_free(dt);
    return ESPCONN_MEM;
  }
  dt->stop_cb = cb;

  os_timer_setfn(dt->timer, server_stop_cb, (void *) dt);
  os_timer_arm(dt->timer, 10, false);

  return ESPCONN_OK;
}