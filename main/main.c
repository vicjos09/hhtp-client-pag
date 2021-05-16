#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_client.h" 
#include "esp_tls.h" 
#include "cJSON.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

//. $HOME/esp/esp-idf/export.sh
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"


//Seguro
#define LOCK_OUTPUT		19
//Buzzer
#define BUZZER_OUTPUT	18
//Filas del teclado
#define GPIO_OUTPUT_IO_0    13 //FILA 4
#define GPIO_OUTPUT_IO_1    12 //FILA 3
#define GPIO_OUTPUT_IO_2    14 //FILA 2
#define GPIO_OUTPUT_IO_3    27 //FILA 1
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1) | (1ULL<<GPIO_OUTPUT_IO_2) | (1ULL<<GPIO_OUTPUT_IO_3) | (1ULL<<BUZZER_OUTPUT) | (1ULL<<LOCK_OUTPUT))

//Sensor de golpes
#define GOLPE_INPUT     4
//Columnas del teclado
#define GPIO_INPUT_IO_0     33 //COLUMNA 3
#define GPIO_INPUT_IO_1     32 //COLUMNA 4
#define GPIO_INPUT_IO_2     25 //COLUMNA 2
#define GPIO_INPUT_IO_3     26 //COLUMNA 1
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IO_2) | (1ULL<<GPIO_INPUT_IO_3) | (1ULL<<GOLPE_INPUT))

//Flag
#define ESP_INTR_FLAG_DEFAULT 0

//****DEBOUNCE KEYBOARD*****
#define DEBOUNCETIME 20
volatile uint32_t anterior = 100;
volatile uint32_t debounceTimeout = 0;
//**************

//****PASSWORD******
volatile int PASSWORD[5] = {1,1,1,1,1};
volatile int counter = -1;
volatile int try[5] = {100,100,100,100,100};
volatile int idx = 0;
//**************


static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void keyboard_task(void* arg)
{
    uint32_t io_num;
    //int* fila = (int*) arg;
    int digito = 100;
    for(;;) {

        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) 
        {
          if(((anterior == io_num) && ((xTaskGetTickCount()-debounceTimeout)>DEBOUNCETIME))||(anterior != io_num))
          {
            if (gpio_get_level(io_num)==1)
            {
            	if (io_num==4)
            	{
            		printf("GOLPE");
            	}
            	else
            	{
            	gpio_set_level(BUZZER_OUTPUT,1);
            	vTaskDelay(250/ portTICK_RATE_MS);
            	gpio_set_level(BUZZER_OUTPUT,0);
            	vTaskDelay(250/ portTICK_RATE_MS);

            	counter++;
            	if (idx == 0)
            	{
            		if (io_num == 26)
            		{
            			digito=100;
            		}
            		if (io_num == 25)
            		{
            			digito=100;
            		}
            		if (io_num == 33)
            		{
            			digito=0;
            		}      
             		if (io_num == 32)
            		{
            			digito=100;
            		}           		      		
            	}
            	if (idx == 1)
            	{
            		if (io_num == 26)
            		{
            			digito=100;
            		}
            		if (io_num == 25)
            		{
            			digito=9;
            		}
            		if (io_num == 33)
            		{
            			digito=8;
            		}      
             		if (io_num == 32)
            		{
            			digito=7;
            		}           		      		
            	}
            	if (idx == 2)
            	{
            		if (io_num == 26)
            		{
            			digito=100;
            		}
            		if (io_num == 25)
            		{
            			digito=6;
            		}
            		if (io_num == 33)
            		{
            			digito=5;
            		}      
             		if (io_num == 32)
            		{
            			digito=4;
            		}           		      		
            	}
            	if (idx == 3)
            	{
            		if (io_num == 26)
            		{
            			digito=100;
            		}
            		if (io_num == 25)
            		{
            			digito=3;
            		}
            		if (io_num == 33)
            		{
            			digito=2;
            		}      
             		if (io_num == 32)
            		{
            			digito=1;
            		}           		      		
            	} 

  	        	if (counter >= 5)
        		{
        			counter = 0;
        		}

        		try[counter] = digito;
        		printf("%d %d %d %d %d\n", try[0],try[1],try[2],try[3],try[4]);

        		if (counter == 4)
        		{
        			if ((try[0]==PASSWORD[0])&&(try[1]==PASSWORD[1])&&(try[2]==PASSWORD[2])&&(try[3]==PASSWORD[3])&&(try[4]==PASSWORD[4]))
        			{
        				printf("CORRECTO");
        				gpio_set_level(LOCK_OUTPUT,1);
        				gpio_set_level(BUZZER_OUTPUT,1);
            			vTaskDelay(3000/ portTICK_RATE_MS);
            			gpio_set_level(BUZZER_OUTPUT,0);
            			vTaskDelay(3000/ portTICK_RATE_MS);
            			gpio_set_level(LOCK_OUTPUT,0);
        			}
        			else
        			{
        				printf("INCORRECTO");
        				for (int i = 0; i < 5 ; ++i)
        				{
        					gpio_set_level(BUZZER_OUTPUT,1);
            				vTaskDelay(100/ portTICK_RATE_MS);
            				gpio_set_level(BUZZER_OUTPUT,0);
            				vTaskDelay(100/ portTICK_RATE_MS);        					
        				}	
        			}
        		}               	
            	}
                anterior = io_num;
                debounceTimeout = xTaskGetTickCount();
            }
          }     
        }
    }
}



static void rows_task(void* arg)
{
    int filas[4] = {13,12,14,27};
    for(;;) 
    {
        printf("\n");
       	gpio_set_level(filas[idx],1);        
        vTaskDelay(20/ portTICK_RATE_MS);
        gpio_set_level(filas[idx],0);
        vTaskDelay(20/ portTICK_RATE_MS);
        idx++;
        if (idx == 4)
        {
        	idx = 0;
        }
        
    }
}

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID		"INFINITUMEB9D"
#define EXAMPLE_ESP_WIFI_PASS		"8670096448"
#define EXAMPLE_ESP_MAXIMUM_RETRY	100
#define EXAMPLE_ESP_REST_URL		"https://my-json-server.typicode.com/vicjos09/dbmicros/micros/19563"

TaskHandle_t httpTaskHandle=NULL;
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about o
ne event
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "JSON";

static int s_retry_num = 0;

//int *passLanus;
RingbufHandle_t xRingbuffer;
int passLanus;
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			if (!esp_http_client_is_chunked_response(evt->client)) {
				//char buffer[512];
				char *buffer = malloc(evt->data_len + 1);
				esp_http_client_read(evt->client, buffer, evt->data_len);
				buffer[evt->data_len] = 0;
				//ESP_LOGI(TAG, "buffer=%s", buffer);
				UBaseType_t res = xRingbufferSend(xRingbuffer, buffer, evt->data_len, pdMS_TO_TICKS(1000));
				if (res != pdTRUE) {
					ESP_LOGW(TAG, "Failed to xRingbufferSend");
				}
				free(buffer);
			}
			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
			break;
	}
	return ESP_OK;
}

static void event_handler(void* arg, esp_event_base_t event_base,
								int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		}
		ESP_LOGE(TAG,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		//ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

void wifi_init_sta()
{
	s_wifi_event_group = xEventGroupCreate();

	tcpip_adapter_init();

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = EXAMPLE_ESP_WIFI_SSID,
			.password = EXAMPLE_ESP_WIFI_PASS
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );

	xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	ESP_LOGI(TAG, "wifi_init_sta finished.");
	ESP_LOGI(TAG, "connect to ap SSID:%s", EXAMPLE_ESP_WIFI_SSID);
}


char *JSON_Types(int type) {
	if (type == cJSON_Invalid) return ("cJSON_Invalid");
	if (type == cJSON_False) return ("cJSON_False");
	if (type == cJSON_True) return ("cJSON_True");
	if (type == cJSON_Raw) return ("cJSON_Raw");
	return NULL;
}

void JSON_Parse(const cJSON * const root,int *valorContra) {
	
    //int indicador=100;
	cJSON *current_element = NULL;

  
    int indicador=20;
	cJSON_ArrayForEach(current_element, root) {
	

		if (current_element->string) {
			const char* string = current_element->string;
			ESP_LOGI(TAG, "[%s]", string);
			if(strcmp(string,"Paswd")){
				indicador=1;}
			else { indicador=0;
			}
			ESP_LOGI(TAG, "[%i]", indicador);
		}
		if (cJSON_IsInvalid(current_element)) {
			ESP_LOGI(TAG, "Invalid");
		} 
		else if (cJSON_IsFalse(current_element)) {
			ESP_LOGI(TAG, "False");
		} else if (cJSON_IsTrue(current_element)) {
			ESP_LOGI(TAG, "True");
		} else if (cJSON_IsNull(current_element)) {
			ESP_LOGI(TAG, "Null");
		} else if (cJSON_IsNumber(current_element)) {
			int valueint = current_element->valueint;

			if(indicador==0&&valueint!=*valorContra&&valueint!=0) {
				*valorContra=valueint;
				printf("Cambio  de contraseña");
				printf("Cambio  de contraseña");
				printf("Cambio  de contraseña");
				gpio_set_level(BUZZER_OUTPUT,1);
            	vTaskDelay(100/ portTICK_RATE_MS);
				gpio_set_level(BUZZER_OUTPUT,1);
            	vTaskDelay(100/ portTICK_RATE_MS);
            	gpio_set_level(BUZZER_OUTPUT,0);
            	vTaskDelay(100/ portTICK_RATE_MS);
            	gpio_set_level(BUZZER_OUTPUT,1);
            	vTaskDelay(200/ portTICK_RATE_MS);
            	gpio_set_level(BUZZER_OUTPUT,0);
            	vTaskDelay(100/ portTICK_RATE_MS);
            	gpio_set_level(BUZZER_OUTPUT,0);
            	vTaskDelay(200/ portTICK_RATE_MS);
            	gpio_set_level(BUZZER_OUTPUT,0);
            	vTaskDelay(100/ portTICK_RATE_MS);
			}

			double valuedouble = current_element->valuedouble;
			ESP_LOGI(TAG, "int=%d double=%f valor_contra=%i", valueint,valuedouble,*valorContra);
		} else if (cJSON_IsString(current_element)) {
			const char* valuestring = current_element->valuestring;
			ESP_LOGI(TAG, "%s", valuestring);
		} else if (cJSON_IsArray(current_element)) {
			//ESP_LOGI(TAG, "Array");
			JSON_Parse(current_element,&passLanus);
		} else if (cJSON_IsObject(current_element)) {
			//ESP_LOGI(TAG, "Object");
			JSON_Parse(current_element,&passLanus);
		} else if (cJSON_IsRaw(current_element)) {
			ESP_LOGI(TAG, "Raw(Not support)");
		}
	}


}

void http_client(char * url)
{
	//configuracion del cliente 
	esp_http_client_config_t config = {
		//.url = "direccion para llamar",
		.url = url,
		.event_handler = _http_event_handler,
	};

    //toma la configuracion y con esta configuracion vamos a generar 
    //cliente
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
	esp_err_t err = esp_http_client_perform(client);
    // si la conexion esta Ok 
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		//Receive an item from no-split ring buffer
		int bufferSize = esp_http_client_get_content_length(client);
		char *buffer = malloc(bufferSize + 1); 
		size_t item_size;
		int	index = 0;
		while (1) {
			char *item = (char *)xRingbufferReceive(xRingbuffer, &item_size, pdMS_TO_TICKS(1000));
			if (item != NULL) {
				for (int i = 0; i < item_size; i++) {
					//printf("%c", item[i]);
					buffer[index] = item[i];
					index++;
					buffer[index] = 0;
				}
				//printf("\n");
				//Return Item
				vRingbufferReturnItem(xRingbuffer, (void *)item);
			} else {
				//Failed to receive item
				ESP_LOGI(TAG, "End of receive item");
				break;
			}
		}
		ESP_LOGI(TAG, "buffer=\n%s", buffer);

		ESP_LOGI(TAG, "Deserialize.....");
		cJSON *root = cJSON_Parse(buffer);
		JSON_Parse(root,&passLanus);
		cJSON_Delete(root);
		free(buffer);

	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
}
void httpTask(void *p){
char url[90];

while(1){
   
	sprintf(url, "%s", EXAMPLE_ESP_REST_URL);
	ESP_LOGI(TAG, "url=%s",url);
	
	http_client(url); 

	//Object
	sprintf(url, "%s/2", EXAMPLE_ESP_REST_URL);
	ESP_LOGI(TAG, "url=%s",url);
	http_client(url); 
	int a=passLanus%10;
    int b=passLanus%100;
    b=(b-a)/10;
    int c=passLanus%1000;
    c=((c-a)-b*10)/100;
    int d=passLanus%10000;
    d=(((d-a)-b*10)-c*100)/1000;
    int e=((((passLanus-a)-b*10)-c*100)-d*1000)/10000;
	printf("El password actual es %i\n",passLanus);				

    PASSWORD[0] = e;
    PASSWORD[1] = d;
    PASSWORD[2] = c;
    PASSWORD[3] = b;
    PASSWORD[4] = a;
 	printf("A = %i\n",PASSWORD[0]);	
	printf("B = %i\n",PASSWORD[1]);	
	printf("C = %i\n",PASSWORD[2]);	
	printf("D = %i\n",PASSWORD[3]);	
	printf("E = %i\n",PASSWORD[4]);	   
    
}

}

void app_main()
{

    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //disable pull-up mode  
    io_conf.pull_up_en = 0;
    //enable pull-down mode
    io_conf.pull_down_en = 1; 
    //configure GPIO with the given settings      
    gpio_config(&io_conf);

    
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(keyboard_task, "Keyboard", 2048, NULL, 10, NULL);
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);
    gpio_isr_handler_add(GPIO_INPUT_IO_2, gpio_isr_handler, (void*) GPIO_INPUT_IO_2);
    gpio_isr_handler_add(GPIO_INPUT_IO_3, gpio_isr_handler, (void*) GPIO_INPUT_IO_3);
    gpio_isr_handler_add(GOLPE_INPUT, gpio_isr_handler, (void*) GOLPE_INPUT);  

    gpio_set_level(LOCK_OUTPUT,0);
    gpio_set_level(BUZZER_OUTPUT,0);


    xTaskCreate(rows_task, "Rows", 2048, NULL, 10, NULL);




	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	  ESP_ERROR_CHECK(nvs_flash_erase());
	  ret = nvs_flash_init();

	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
	wifi_init_sta();

	//Create Ring Buffer 
	//No Split
	xRingbuffer = xRingbufferCreate(1024, RINGBUF_TYPE_NOSPLIT);
	//Allow_Split
	xTaskCreate(httpTask,"httpTask",(1024 * 32),NULL,5,NULL);
   
	//Check everything was created
	configASSERT( xRingbuffer );
    
	//Array
    //sprintf(url, "%s", "https://my-json-server.typicode.com/vicjos09/dbmicros/micros/1");
	//ESP_LOGI(TAG, "url=%s",url);
	//http_client("url"); 
	//http_client(url); 

	//Object
	//sprintf(url, "%s/2", "https://my-json-server.typicode.com/vicjos09/dbmicros/micros/1");
	//ESP_LOGI(TAG, "url=%s",url);
	//http_client(url); 
	
}