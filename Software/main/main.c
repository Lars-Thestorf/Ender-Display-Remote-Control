#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32/rom/gpio.h"
#include "driver/gpio.h"
#include <esp_log.h>
#include <esp_http_server.h>
#include "esp_netif.h"
#include "esp_event.h"

#include "spi_sniffer.h"
#include "wifi_sta.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "website.h"

#define INV_IN 32
#define INV_OUT 27

#define EN_A 4
#define EN_B 13
#define EN_BTN 23

#define TLEN 2048
WORD_ALIGNED_ATTR uint8_t spi_buffer[TLEN];

uint8_t displaybuffer[4][256];
bool displayupdated = false;

/* A simple example that demonstrates using websocket echo server
 */
static const char *TAG = "PrinterDisplayReader";

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

/*
 * async send function, which we put into the httpd work queue
 */
static void ws_async_send(void *arg)
{
    static const uint8_t * data = displaybuffer[0];
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = 1024;
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
{
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    return httpd_queue_work(handle, ws_async_send, resp_arg);
}

void actionLeft() {
	gpio_set_level(EN_A, 1);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_B, 1);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_A, 0);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_B, 0);
}
void actionRight() {
	gpio_set_level(EN_B, 1);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_A, 1);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_B, 0);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_A, 0);
}
void actionClick() {
	gpio_set_level(EN_BTN, 1);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(EN_BTN, 0);
}

/*
 * This handler echos back the received ws data
 * and triggers an async send if certain message received
 */
static esp_err_t echo_handler(httpd_req_t *req)
{
    uint8_t buf[128] = { 0 };
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = buf;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 128);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);
	if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
		if (strcmp((char*)ws_pkt.payload,"Trigger async") == 0) {
			return trigger_async_send(req->handle, req);
		}
		if (strcmp((char*)ws_pkt.payload,"action left") == 0) {
			actionLeft();
			ESP_LOGI(TAG, "left command");
		}
		if (strcmp((char*)ws_pkt.payload,"action right") == 0) {
			actionRight();
			ESP_LOGI(TAG, "right command");
		}
		if (strcmp((char*)ws_pkt.payload,"action click") == 0) {
			actionClick();
			ESP_LOGI(TAG, "click command");
		}
	}

    ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    }
    return ret;
}

static esp_err_t html_handler(httpd_req_t *req){
    httpd_resp_send(req, ressource_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
static esp_err_t js_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_send(req, ressource_js, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t ws = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = echo_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};
static const httpd_uri_t html = {
        .uri        = "/",
        .method     = HTTP_GET,
        .handler    = html_handler,
        .user_ctx   = NULL,
        .is_websocket = false
};
static const httpd_uri_t js = {
        .uri        = "/script.js",
        .method     = HTTP_GET,
        .handler    = js_handler,
        .user_ctx   = NULL,
        .is_websocket = false
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Registering the ws handler
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &html);
        httpd_register_uri_handler(server, &js);
        httpd_register_uri_handler(server, &ws);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

static void broadcast_frame(httpd_handle_t server){
	size_t amount = 8;
	int sockets[amount];
	esp_err_t ret = httpd_get_client_list(server, &amount, sockets);
	if (ret != ESP_OK)
		return;
	ESP_LOGI(TAG,"Currently %d clients", amount);
	for(int i = 0; i < amount; i++) {
		if(sockets[i] != -1){
			struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
			resp_arg->hd = server;
			resp_arg->fd = sockets[i];
			httpd_queue_work(server, ws_async_send, resp_arg);
		}
	}
}

void app_main(void)
{
    spi_sniffer_init(spi_buffer, TLEN);
    gpio_set_direction(INV_IN, GPIO_MODE_INPUT);
    gpio_matrix_in(INV_IN, SIG_IN_FUNC224_IDX, false);
    gpio_set_direction(INV_OUT, GPIO_MODE_OUTPUT);
    gpio_matrix_out(INV_OUT, SIG_IN_FUNC224_IDX, true, false);
	
	gpio_reset_pin(EN_A);
	gpio_set_direction(EN_A, GPIO_MODE_OUTPUT);
	gpio_set_level(EN_A, 0);
	gpio_pullup_dis(EN_A);
	gpio_reset_pin(EN_B);
	gpio_set_direction(EN_B, GPIO_MODE_OUTPUT);
	gpio_set_level(EN_B, 0);
	gpio_pullup_dis(EN_B);
	gpio_reset_pin(EN_BTN);
	gpio_set_direction(EN_BTN, GPIO_MODE_OUTPUT);
	gpio_set_level(EN_BTN, 0);
	gpio_pullup_dis(EN_BTN);

    static httpd_handle_t server = NULL;

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    //ESP_ERROR_CHECK(example_connect());
    wifi_init_sta();

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));

    /* Start the server for the first time */
    server = start_webserver();


    while (1) {
        uint16_t length = spi_sniffer_sniff();
		printf("len: %d\n", length);
        if (length == 4864 || length == 4865) { //Mainboard 4.2.7 somehow transmits one bit more
			if((spi_buffer[0] & 0xF8) == 0xF8) { //transmission is perfectly in sync
				if((spi_buffer[1] == 0x80 || spi_buffer[1] == 0x90) && (spi_buffer[4] == 0x00 || spi_buffer[4] == 0x80)){
					int section = (spi_buffer[1] == 0x90) | (spi_buffer[4] == 0x80) << 1;
					printf("%d\n", section);
					int rowdataoffset = 6;
					for(int row=0; row < 16; row++) {
						for(int rowbytes = 0; rowbytes < 16; rowbytes++) {
							displaybuffer[section][row*16 + rowbytes] = 
								((spi_buffer[rowdataoffset+rowbytes*2] & 0xF0) | 
								(spi_buffer[(rowdataoffset+rowbytes*2)+1] & 0xF0) >> 4);
						}
						rowdataoffset += 38;
					}
					if (section == 3){
						displayupdated = true;
						broadcast_frame(server);
					}
				} else {
					printf("unknown section: %x %x %x %x %x\n", spi_buffer[0], spi_buffer[1], spi_buffer[2], spi_buffer[3], spi_buffer[4]);
				}
			} else if((spi_buffer[0] & 0xF8>>1) == 0xF8>>1) { //transmission is one bit off, can be caused by slow reaction to CS signal, but is luckily not a huge problem at all
				if((spi_buffer[1] == 0x80>>1 || spi_buffer[1] == 0x90>>1) && (spi_buffer[4] == 0x00>>1 || spi_buffer[4] == 0x80>>1)){
					int section = (spi_buffer[1] == 0x90>>1) | (spi_buffer[4] == 0x80>>1) << 1;
					printf("%d\n", section);
					int rowdataoffset = 6;
					for(int row=0; row < 16; row++) {
						for(int rowbytes = 0; rowbytes < 16; rowbytes++) {
							displaybuffer[section][row*16 + rowbytes] = 
								(
									((spi_buffer[rowdataoffset+rowbytes*2] & 0xF0>>1)<<1) | 
									(spi_buffer[(rowdataoffset+rowbytes*2)+1] & 0xF0>>1) >> 3
								);
						}
						rowdataoffset += 38;
					}
					if (section == 3){
						displayupdated = true;
						broadcast_frame(server);
					}
				} else {
					printf("unknown section: %x %x %x %x %x\n", spi_buffer[0], spi_buffer[1], spi_buffer[2], spi_buffer[3], spi_buffer[4]);
				}
			} else {
				printf("corruptded data: %x %x %x %x %x\n", spi_buffer[0], spi_buffer[1], spi_buffer[2], spi_buffer[3], spi_buffer[4]);
			}
        }
    }
}
