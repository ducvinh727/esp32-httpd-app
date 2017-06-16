#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/portmacro.h"
#include "tcpip_adapter.h"
#include "lwip/err.h"
#include "string.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"

// set AP CONFIG values
#ifdef CONFIG_AP_HIDE_SSID
	#define CONFIG_AP_SSID_HIDDEN 1
#else
	#define CONFIG_AP_SSID_HIDDEN 0
#endif	
#ifdef CONFIG_WIFI_AUTH_OPEN
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_OPEN
#endif
#ifdef CONFIG_WIFI_AUTH_WEP
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WEP
#endif
#ifdef CONFIG_WIFI_AUTH_WPA_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA2_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA2_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA_WPA2_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA_WPA2_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA2_ENTERPRISE
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA2_ENTERPRISE
#endif


// Event group
static EventGroupHandle_t event_group;
const int STA_CONNECTED_BIT = BIT0;
const int STA_DISCONNECTED_BIT = BIT1;


// AP event handler
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
		
    case SYSTEM_EVENT_AP_START:
		printf("Access point started\n");
		break;
		
	case SYSTEM_EVENT_AP_STACONNECTED:
		xEventGroupSetBits(event_group, STA_CONNECTED_BIT);
		break;

	case SYSTEM_EVENT_AP_STADISCONNECTED:
		xEventGroupSetBits(event_group, STA_DISCONNECTED_BIT);
		break;		
    
	default:
        break;
    }
   
	return ESP_OK;
}

const static char http_html_hdr[]  =
      "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

const static char http_index_hml[] = "<!DOCTYPE html>"
      "<html>\n"
      "<head>\n"
      "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
      "  <style type=\"text/css\">\n"
      "    html, body, iframe { margin: 0; padding: 0; height: 100%; }\n"
      "    iframe { display: block; width: 100%; border: none; }\n"
      "  </style>\n"
      "<title>Hello world!</title>\n"
      "</head>\n"
      "<body>\n"
      "<h1>Hello world!!!</h1>\n"
      "</body>\n"
      "</html>\n";

static void http_server_netconn_serve(struct netconn *conn) {
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;
  /* Read the data from the port, blocking if nothing yet there.
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf);
  if (err == ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);
    /* Is this an HTTP GET command?
    there are other formats for GET, and we're keeping it very simple )*/
    printf("%s \n", buf);
     /* Send the HTML header
             * subtract 1 from the size, since we dont send the \0 in the string
             * NETCONN_NOCOPY: our data is const static, so no need to copy it
     */
     netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
     /* Send our HTML page */
     netconn_write(conn, http_index_hml, sizeof(http_index_hml)-1, NETCONN_NOCOPY);
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);
  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}

static void http_server(void *pvParameters){
  struct netconn *conn, *newconn;
  err_t err;
  conn = netconn_new(NETCONN_TCP);
  netconn_bind(conn, NULL, 80);
  netconn_listen(conn);
  do {
     err = netconn_accept(conn, &newconn);
     if (err == ERR_OK) {
       http_server_netconn_serve(newconn);
       netconn_delete(newconn);
     }
   } while(err == ERR_OK);
   netconn_close(conn);
   netconn_delete(conn);
}

// print the list of connected stations
void printStationList() {
	printf("Connected stations:\n");
	printf("--------------------------------------------------\n");
	
	wifi_sta_list_t wifi_sta_list;
	tcpip_adapter_sta_list_t adapter_sta_list;
   
	memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
	memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
   
	ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&wifi_sta_list));	
	ESP_ERROR_CHECK(tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list));
	
	for(int i = 0; i < adapter_sta_list.num; i++) {
		
		tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
         printf("%d - mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x - IP: %s\n", i + 1,
				station.mac[0], station.mac[1], station.mac[2],
				station.mac[3], station.mac[4], station.mac[5],
				ip4addr_ntoa(&(station.ip)));
	}
	printf("\n");
}

// Monitor task, receive Wifi AP events
void monitor_task(void *pvParameter){
	while(1) {
		
		EventBits_t staBits = xEventGroupWaitBits(event_group, STA_CONNECTED_BIT | STA_DISCONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		if((staBits & STA_CONNECTED_BIT) != 0) {
			 printf("New station connected\n\n");
		}
		else {
			 printf("A station disconnected\n\n");
		}
	}
}

// Station list task, print station list every 10 seconds
void station_list_task(void *pvParameter){
	while(1) {
		
		printStationList();
		vTaskDelay(10000 / portTICK_RATE_MS);
	}
}

void app_main() {	
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);
	
	// create the event group to handle wifi events
	event_group = xEventGroupCreate();
		
	// initialize the tcp stack
	tcpip_adapter_init();

	// stop DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	
	// start the DHCP server   
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	
	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	
	// initialize the wifi stack in AccessPoint mode with config in RAM
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

	// configure the wifi connection and start the interface
	wifi_config_t ap_config = {
        .ap = {
            .ssid = CONFIG_AP_SSID,
            .password = CONFIG_AP_PASSWORD,
			.ssid_len = 0,
			.channel = CONFIG_AP_CHANNEL,
			.authmode = CONFIG_AP_AUTHMODE,
			.ssid_hidden = CONFIG_AP_SSID_HIDDEN,
			.max_connection = CONFIG_AP_MAX_CONNECTIONS,
			.beacon_interval = CONFIG_AP_BEACON_INTERVAL,			
        },
    };
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    
	// start the wifi interface
	ESP_ERROR_CHECK(esp_wifi_start());
	printf("Starting access point, SSID=%s\n", CONFIG_AP_SSID);
	
	// start the main task
        xTaskCreate(&monitor_task, "monitor_task", 2048, NULL, 5, NULL);
	xTaskCreate(&station_list_task, "station_list_task", 2048, NULL, 5, NULL);
	xTaskCreate(&http_server, "http_server", 2048, NULL, 5, NULL);
}
