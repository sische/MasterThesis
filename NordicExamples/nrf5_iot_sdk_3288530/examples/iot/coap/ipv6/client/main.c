/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup ble_sdk_app_template_main main.c
 * @{
 * @ingroup ble_sdk_app_template
 * @brief Template project main file.
 *
 * This file contains a template for creating a new application. It has the code necessary to wakeup
 * from button, advertise, get a connection restart advertising on disconnect and if no new
 * connection created go back to system-off mode.
 * It can easily be used as a starting point for creating a new application, the comments identified
 * with 'YOUR_JOB' indicates where and how you can customize.
 */
	
#include <stdbool.h>
#include <stdint.h>
#include "boards.h"
#include "nordic_common.h"
#include "nrf_delay.h"
#include "softdevice_handler.h"
#include "mem_manager.h"
#include "app_trace.h"
#include "app_timer_appsh.h"
#include "app_button.h"
#include "ble_advdata.h"
#include "ble_srv_common.h"
#include "ble_ipsp.h"
#include "iot_common.h"
#include "ipv6_api.h"
#include "icmp6_api.h"
#include "udp_api.h"
#include "iot_timer.h"
#include "coap_api.h"
#include "ipv6_medium.h"

// Added by Sindre
//#include <EEPROM.h>
#include <stdio.h>
#include <ctype.h>
#include "config.h"
#include "eeprom_simulator.h"
#include "app_uart.h"
#include "nrf_drv_twi.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include "nrf.h"
#include "bsp.h"
#include "app_util_platform.h"
#include "nrf_drv_config.h"

//#include <Wire.h> 	Ardino specific, replacement in TWI example
//#include <Adafruit_Sensor.h>
//#include <Adafruit_ADXL345_U.h>

/** Modify SERVER_IPV6_ADDRESS according to your setup.
 *  The address provided below is a place holder.  */
//#define SERVER_IPV6_ADDRESS             0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00, \
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01        /**< IPv6 address of the server node. */
																				
// Added by Sindre, server ipv6 address:		******************************************************************************************************************************************* 
// SERVER_IPV6_ADDRESS to NRF52 CoAP server
#define SERVER_IPV6_ADDRESS							0x20, 0x01, 0x0d, 0xB8, 0x00, 0x00, 0x00, 0x00, \
																				0x02, 0xAF, 0xB7, 0xFF, 0xFE, 0xB6, 0x14, 0x94


// The CoAP default port number 5683 MUST be supported by a server.
#define LOCAL_PORT_NUM                  5683                                                  /**< CoAP default port number. */
#define REMOTE_PORT_NUM                 5683                                                  /**< Remote port number. */

#define COAP_MESSAGE_TYPE               COAP_TYPE_CON                                         /**< Message type for all outgoing CoAP requests. */

#define LED_ONE                         BSP_LED_0_MASK
#define LED_TWO                         BSP_LED_1_MASK
#define LED_THREE                       BSP_LED_2_MASK
#define LED_FOUR                        BSP_LED_3_MASK

#define BUTTON_ONE                      BSP_BUTTON_0
#define BUTTON_TWO                      BSP_BUTTON_1
// Added by Sindre
#define BUTTON_THREE										BSP_BUTTON_2

#ifdef COMMISSIONING_ENABLED
#define ERASE_BUTTON_PIN_NO             BSP_BUTTON_3                                          /**< Button used to erase commissioning settings. */
																				// Why isn't this button 4...??
#endif // COMMISSIONING_ENABLED

#define COMMAND_TOGGLE                  0x32

#define APP_TIMER_OP_QUEUE_SIZE         8                                                     /**< Size of timer operation queues. */

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)

#define LED_BLINK_INTERVAL_MS           300                                                   /**< LED blinking interval. */
#define COAP_TICK_INTERVAL_MS           1000                                                  /**< Interval between periodic callbacks to CoAP module. */

#define APP_RTR_SOLICITATION_DELAY      500                                                   /**< Time before host sends an initial solicitation in ms. */

#define DEAD_BEEF                       0xDEADBEEF                                            /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
#define MAX_LENGTH_FILENAME             128                                                   /**< Max length of filename to copy for the debug error handler. */

#define APP_ENABLE_LOGS                 1                                                     /**< Disable debug trace in the application. */

#if (APP_ENABLE_LOGS == 1)

#define APPL_LOG  app_trace_log
#define APPL_DUMP app_trace_dump

#else // APP_ENABLE_LOGS

#define APPL_LOG(...)
#define APPL_DUMP(...)

#endif // APP_ENABLE_LOGS

#define APPL_ADDR(addr) APPL_LOG("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n", \
                                (addr).u8[0],(addr).u8[1],(addr).u8[2],(addr).u8[3],                            \
                                (addr).u8[4],(addr).u8[5],(addr).u8[6],(addr).u8[7],                            \
                                (addr).u8[8],(addr).u8[9],(addr).u8[10],(addr).u8[11],                          \
                                (addr).u8[12],(addr).u8[13],(addr).u8[14],(addr).u8[15])

// Added by Sindre

#define TWI0_INSTANCE_INDEX 0




// Added by Sindre

static const char m_cmd_help_str[] =
        "   p - Print the EEPROM contents in a form: address, 8 bytes of code, ASCII form.\n"
        "   w - Write string starting from address 0.\n"
        "   c - Clear the memory (write 0xff)\n"
        "   x - Get transmission error byte.";


typedef enum
{
    LEDS_INACTIVE = 0,
    LEDS_CONNECTABLE_MODE,
    LEDS_IPV6_IF_DOWN,
    LEDS_IPV6_IF_UP, 
} display_connection_state_t;

static ipv6_medium_instance_t      m_ipv6_medium;
eui64_t                            eui64_local_iid;                                            /**< Local EUI64 value that is used as the IID for*/
static iot_interface_t           * mp_interface;
static const ipv6_addr_t           local_routers_multicast_addr = {{0xFF, 0x02, 0x00, 0x00, \
                                                                    0x00, 0x00, 0x00, 0x00, \
                                                                    0x00, 0x00, 0x00, 0x00, \
                                                                    0x00, 0x00, 0x00, 0x02}};  /**< Multicast address of all routers on the local network segment. */

APP_TIMER_DEF(m_iot_timer_tick_src_id);                                                        /**< App timer instance used to update the IoT timer wall clock. */
static uint8_t                     m_well_known_core[100];
static display_connection_state_t  m_disp_state      = LEDS_INACTIVE;                          /**< Board LED display state. */
static bool                        m_blink_led_three = false;
static bool                        m_blink_led_four  = false;
                                                                    
static const char                  m_uri_part_lights[]  = "lights";
static const char                  m_uri_part_led3[]    = "led3";
static const char                  m_uri_part_led4[]    = "led4";
static int                         m_temperature        = 21;
static uint16_t                    m_global_token_count = 0x0102;

																																		
/**	Added by Sindre
 * @brief TWI master instance
 *
 * Instance of TWI master driver that would be used for communication with simulated
 * eeprom memory.
 */

//const nrf_drv_twi_t =; // NRF_DRV_TWI_INSTANCE(*void); 
 
//volatile unsigned ret_code_t nrf_drv_twi_rx;
//extern ret_code_t nrf_drv_twi_rx;

static const nrf_drv_twi_t m_twi_master = NRF_DRV_TWI_INSTANCE(MASTER_TWI_INST);									
										
//static const nrf_drv_twi_t m_twi_master = NRF_DRV_TWI_INSTANCE(MASTER_TWI_INST);
//Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
																																		
#ifdef COMMISSIONING_ENABLED
static bool                    m_power_off_on_failure = false;
static bool                    m_identity_mode_active;
#endif // COMMISSIONING_ENABLED

/**@brief Function for error handling, which is called when an error has occurred.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name.
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    LEDS_ON((LED_ONE | LED_TWO | LED_THREE | LED_FOUR));
    APPL_LOG("[** ASSERT **]: Error 0x%08lX, Line %ld, File %s\r\n",error_code, line_num, p_file_name);

    // Variables used to retain function parameter values when debugging.
    static volatile uint8_t  s_file_name[MAX_LENGTH_FILENAME];
    static volatile uint16_t s_line_num;
    static volatile uint32_t s_error_code;

    strncpy((char *)s_file_name, (const char *)p_file_name, MAX_LENGTH_FILENAME - 1);
    s_file_name[MAX_LENGTH_FILENAME - 1] = '\0';
    s_line_num                           = line_num;
    s_error_code                         = error_code;
    UNUSED_VARIABLE(s_file_name);
    UNUSED_VARIABLE(s_line_num);
    UNUSED_VARIABLE(s_error_code);

#ifdef COMMISSIONING_ENAB
	
	
    if (m_power_off_on_failure == true)
    {
        LEDS_OFF((LED_ONE | LED_TWO | LED_THREE | LED_FOUR));
        UNUSED_VARIABLE(sd_power_system_off());
    }
    else
    {
        NVIC_SystemReset();
    }
#else // COMMISSIONING_ENABLED
    //NVIC_SystemReset();

    for (;;)
    {
    }
#endif // COMMISSIONING_ENABLED
}


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for the LEDs initialization.
 *
 * @details Initializes all LEDs used by this application.
 */
static void leds_init(void)
{
    // Configure leds.
    LEDS_CONFIGURE((LED_ONE | LED_TWO | LED_THREE | LED_FOUR));

    // Turn leds off.
    LEDS_OFF((LED_ONE | LED_TWO | LED_THREE | LED_FOUR));
}


/**@brief Timer callback used for controlling board LEDs to represent application state.
 *
 */
static void blink_timeout_handler(iot_timer_time_in_ms_t wall_clock_value)
{
    UNUSED_PARAMETER(wall_clock_value);

#ifdef COMMISSIONING_ENABLED
    static bool id_mode_previously_enabled;

    if (m_identity_mode_active == false)
    {
#endif // COMMISSIONING_ENABLED
    switch (m_disp_state)
    {
        case LEDS_INACTIVE:
        {
            LEDS_OFF((LED_ONE | LED_TWO));
            LEDS_OFF((LED_THREE | LED_FOUR));
            break;
        }
        case LEDS_CONNECTABLE_MODE:
        {
            LEDS_INVERT(LED_ONE);
            LEDS_OFF(LED_TWO);
            LEDS_OFF((LED_THREE | LED_FOUR));
            break;
        }
        case LEDS_IPV6_IF_DOWN:
        {
            LEDS_ON(LED_ONE);
            LEDS_INVERT(LED_TWO);
            LEDS_OFF((LED_THREE | LED_FOUR));
            break;
        }
        case LEDS_IPV6_IF_UP:
        {
            LEDS_OFF(LED_ONE);
            LEDS_ON(LED_TWO);

            // If m_blink_led_three is set, keep LED_THREE on for 1 period.
            if LED_IS_ON(LED_THREE)
            {
                LEDS_OFF(LED_THREE);
            }
            else if (m_blink_led_three == true)
            {
                LEDS_ON(LED_THREE);
                m_blink_led_three = false;
            }

            // If m_blink_led_four is set, keep LED_FOUR on for 1 period.
            if LED_IS_ON(LED_FOUR)
            {
                LEDS_OFF(LED_FOUR);
            }
            else if (m_blink_led_four == true)
            {
                LEDS_ON(LED_FOUR);
                m_blink_led_four = false;
            }
            break;
        }
        default:
        {
            break;
        }
    }
#ifdef COMMISSIONING_ENABLED
    }
#endif // COMMISSIONING_ENABLED

#ifdef COMMISSIONING_ENABLED
    if ((id_mode_previously_enabled == false) && (m_identity_mode_active == true))
    {
        LEDS_OFF(LED_THREE | LED_FOUR);
    }

    if ((id_mode_previously_enabled == true) && (m_identity_mode_active == true))
    {
        LEDS_INVERT(LED_THREE | LED_FOUR);
    }

    if ((id_mode_previously_enabled == true) && (m_identity_mode_active == false))
    {
        LEDS_OFF(LED_THREE | LED_FOUR);
    }

    id_mode_previously_enabled = m_identity_mode_active;
#endif // COMMISSIONING_ENABLED
}


/**@brief Function for catering CoAP module with periodic time ticks.
*/
static void app_coap_time_tick(iot_timer_time_in_ms_t wall_clock_value)
{
    (void)coap_time_tick();
}


/**@brief Function for updating the wall clock of the IoT Timer module.
 */
static void iot_timer_tick_callback(void * p_context)
{
    UNUSED_VARIABLE(p_context);
    uint32_t err_code = iot_timer_update();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
static void timers_init(void)
{
    uint32_t err_code;

    // Initialize timer module, making it use the scheduler
    APP_TIMER_APPSH_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    // Create a sys timer.
    err_code = app_timer_create(&m_iot_timer_tick_src_id,
                                APP_TIMER_MODE_REPEATED,
                                iot_timer_tick_callback);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the IoT Timer. */
static void iot_timer_init(void)
{
    uint32_t err_code;

    static const iot_timer_client_t list_of_clients[] =
    {
        {blink_timeout_handler,   LED_BLINK_INTERVAL_MS},
        {app_coap_time_tick,      COAP_TICK_INTERVAL_MS},
#ifdef COMMISSIONING_ENABLED
        {commissioning_time_tick, SEC_TO_MILLISEC(COMMISSIONING_TICK_INTERVAL_SEC)}
#endif // COMMISSIONING_ENABLED
    };

    // The list of IoT Timer clients is declared as a constant.
    static const iot_timer_clients_list_t iot_timer_clients =
    {
        (sizeof(list_of_clients) / sizeof(iot_timer_client_t)),
        &(list_of_clients[0]),
    };

    // Passing the list of clients to the IoT Timer module.
    err_code = iot_timer_client_list_set(&iot_timer_clients);
    APP_ERROR_CHECK(err_code);

    // Starting the app timer instance that is the tick source for the IoT Timer.
    err_code = app_timer_start(m_iot_timer_tick_src_id, \
                               APP_TIMER_TICKS(IOT_TIMER_RESOLUTION_IN_MS, APP_TIMER_PRESCALER), \
                               NULL);
    APP_ERROR_CHECK(err_code);
}


static void coap_response_handler(uint32_t status, void * arg, coap_message_t * p_response)
{
    if (status == NRF_SUCCESS)
    {
        APPL_LOG("[APPL]: Response Code : %d\r\n", p_response->header.code);
        if (p_response->header.code == COAP_CODE_204_CHANGED)
        {
            m_blink_led_three = true;
        }
        else
        {
            m_blink_led_four = true;
        }
    }
		
		// Added by Sindre 
		else
		{
				APPL_LOG("[APPL]: Response Code : %d\r\n", p_response->header.code);
				m_blink_led_four = true;
		}
			
}

/**		Added by Sindre
 * @brief Safely get string from stdin
 *
 * Function reads character by character into given buffer.
 * Maximum @em nmax number of characters are read.
 *
 * Function ignores all nonprintable characters.
 * String may be finished by CR or NL.
 *
 * @attention
 * Remember that after characters read zero would be added to mark the string end.
 * Given buffer should be no smaller than @em nmax + 1
 *
 * @param[out] str  Buffer for the string
 * @param      nmax Maximum number of characters to be readed
 */
static void safe_gets(char * str, size_t nmax)
{
    int c;
    while(1)
    {
        c = getchar();
        if(isprint(c))
        {
            *str++ = (char)c;
            UNUSED_VARIABLE(putc(c, stdout));
            UNUSED_VARIABLE(fflush(stdout));
            if(0 == --nmax)
                break;
        }
        else if('\n' == c || '\r' == c)
        {
            break;
        }
    }
    *str = '\0';
    UNUSED_VARIABLE(putc('\n', stdout));
    UNUSED_VARIABLE(fflush(stdout));
}

/**	Added by Sindre
 * @brief Print address
 *
 * Function used to print address of a row in EEPROM content pretty-printing.
 * Function finishes printed value with a colon and a space.
 *
 * @sa do_print_data
 *
 * @param addr Address to print
 */
static void print_addr(size_t addr)
{
    printf("%.2x: ", (unsigned int)addr);
}

static void print_hex(uint8_t data)
{
    printf("%.2x ", (unsigned int)data);
}

/**		Added by Sindre
 * @brief Pretty-print EEPROM content
 *
 * Respond on memory print command.
 */

/**
 * @brief Put printable character to stdout
 *
 * Function puts given character to stdout.
 * If the character is not printable it is changed to dot before processing.
 *
 * @param c Character to print
 */
static void safe_putc(char c)
{
    if(!isprint((int)c))
    {
        c = '.';
    }
    UNUSED_VARIABLE(putc(c, stdout));
}

/** Added by Sindre
 * @brief Read data from simulated EEPROM
 *
 * Function uses TWI interface to read data from EEPROM memory.
 *
 * @param     addr  Start address to read
 * @param[in] pdata Pointer to the buffer to fill with data
 * @param     size  Byte count of data to read
 *
 * @return NRF_SUCCESS or reason of error.
 */
static ret_code_t eeprom_read(size_t addr, uint8_t * pdata, size_t size)
{
    ret_code_t ret;
    if(size > EEPROM_SIM_SIZE)
        return NRF_ERROR_INVALID_LENGTH;
    do
    {
       uint8_t addr8 = (uint8_t)addr;
       //ret = nrf_drv_twi_tx(&m_twi_master, EEPROM_SIM_ADDR, &addr8, 1, true);
       if(NRF_SUCCESS != ret)
       {
           break;
       }
       //ret = nrf_drv_twi_rx(&m_twi_master, EEPROM_SIM_ADDR, pdata, size, false);
    }while(0);
    return ret;
}


// Alternative to eeprom_read? Added by Sindre

/*static ret_code_t read_reg( uint8_t slave_address, uint8_t register_address) {

    ret_code_t ret;
    uint8_t buff[IN_LINE_PRINT_CNT];
    nrf_drv_twi_tx(&m_twi_master, slave_address, &register_address, 1, true);
    nrf_drv_twi_rx(&m_twi_master, slave_address, buff, IN_LINE_PRINT_CNT, false);
    return ret;
}*/


/** Added by Sindre
 * @brief Function that performs simulated EEPROM clearing
 *
 * Function fills the EEPROM with 0xFF value.
 * It is accessing the EEPROM writing only one byte at once.
 */
static void do_clear_eeprom(void)
{
    //uint8_t clear_val = 0xff;
    size_t addr;
    for(addr=0; addr<EEPROM_SIM_SIZE; ++addr)
    {
        ret_code_t err_code;
        err_code = eeprom_write(addr, &clear_val, 1);
        APP_ERROR_CHECK(err_code);
    }
    UNUSED_VARIABLE(puts("Memory erased"));
    UNUSED_VARIABLE(fflush(stdout));
}

static void do_print_data(void)
{
    size_t addr;
    uint8_t buff[IN_LINE_PRINT_CNT];
    for(addr=0; addr<EEPROM_SIM_SIZE; addr+=IN_LINE_PRINT_CNT)
    {
        unsigned int n;
        ret_code_t err_code;
        err_code = eeprom_read(addr, buff, IN_LINE_PRINT_CNT);
        APP_ERROR_CHECK(err_code);

        print_addr(addr);
        for(n=0; n<IN_LINE_PRINT_CNT; ++n)
        {
            print_hex(buff[n]);
        }

        safe_putc(' '); safe_putc(' ');

        for(n=0; n<IN_LINE_PRINT_CNT; ++n)
        {
            safe_putc((char)buff[n]);
        }
        UNUSED_VARIABLE(putc('\n', stdout));
    }
    UNUSED_VARIABLE(fflush(stdout));
}

/**		Added by Sindre
 * @brief Function that performs the command of writing string to EEPROM
 *
 * Function gets user input and writes given string to EEPROM starting from address 0.
 * It is accessing EEPROM using maximum allowed number of bytes in sequence.
 */
static void do_string_write(void)
{
    char str[EEPROM_SIM_SIZE+1];
    //size_t addr = 0;

    UNUSED_VARIABLE(puts("Waiting for string to write:"));
    safe_gets(str, sizeof(str)-1);
    while(1)
    {
        ret_code_t err_code;
        //size_t to_write = safe_strlen(str+addr, EEPROM_SIM_SEQ_WRITE_MAX);
        //if(0 == to_write)
        //    break;
        //err_code = eeprom_write(addr, (uint8_t const *)str+addr, to_write);
        APP_ERROR_CHECK(err_code);
        //addr += to_write;
    }
}

// Alternative to do_string_write? Added by Sindre

/*static ret_code_t write_reg(uint8_t slave_address, uint8_t target_register, uint8_t data_to_write) {
    ret_code_t ret;
    nrf_drv_twi_tx(&m_twi_master, slave_address, &target_register, 1, true);
    size_t size = sizeof(data_to_write);
    nrf_drv_twi_tx(&m_twi_master, slave_address, &data_to_write, size, false);
    return ret;
}*/




size_t safe_strlen(char const * str, size_t nmax)
{
    size_t n=0;
    while('\0' != *str++)
    {
        ++n;
        if(0 == --nmax)
            break;
    }
    return n;
}


/**		Added by Sindre
 * @brief Write data to simulated EEPROM
 *
 * Function uses TWI interface to write data into EEPROM memory.
 *
 * @param     addr  Start address to write
 * @param[in] pdata Pointer to data to send
 * @param     size  Byte count of data to send
 * @attention       Maximum number of bytes that may be written is @ref EEPROM_SIM_SEQ_WRITE_MAX.
 *                  In sequential write all data have to be in the same page
 *                  (higher address bits do not change).
 *
 * @return NRF_SUCCESS or reason of error.
 *
 * @attention If you wish to communicate with real EEPROM memory chip check its readiness
 * after writing data.
 */

ret_code_t eeprom_write(size_t addr, uint8_t * pdata, size_t size)
{
    ret_code_t ret;
    /* Memory device supports only limited number of bytes written in sequence */
    if((size > EEPROM_SIM_SEQ_WRITE_MAX) && (size > 0))
        return NRF_ERROR_INVALID_LENGTH;
    /* All written data has to be in the same page */
    if((addr/EEPROM_SIM_SEQ_WRITE_MAX) != ((addr+size-1)/EEPROM_SIM_SEQ_WRITE_MAX))
        return NRF_ERROR_INVALID_ADDR;
    do
    {
        //uint8_t addr8 = (uint8_t)addr;
        //ret = nrf_drv_twi_tx(&m_twi_master, EEPROM_SIM_ADDR, &addr8, 1, true);
        if(NRF_SUCCESS != ret)
        {
            break;
        }
        //ret = nrf_drv_twi_tx(&m_twi_master, EEPROM_SIM_ADDR, pdata, size, false);
    }while(0);
    return ret;
} 

/**		Added by Sindre
 * @brief Initialize the master TWI
 *
 * Function used to initialize master TWI interface that would communicate with simulated EEPROM.
 *
 * @return NRF_SUCCESS or the reason of failure
 */
static ret_code_t twi_master_init(void)
{
    ret_code_t ret; // scl and sda changed according to devzone.nordicsemi.com/question/57120/connecting-altimeter-to-nrf52-dk-using-i2c
    const nrf_drv_twi_config_t config =
    {
       .scl                = 27,		
       .sda                = 26,
       .frequency          = NRF_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH
    };

    do
    {
//        ret = nrf_drv_twi_init(&m_twi_master, &config, NULL);				// &m_twi_master will not work!
       if(NRF_SUCCESS != ret)
        {
            break;
        }
//        nrf_drv_twi_enable(&m_twi_master);
    }while(0);
    return ret;
}


/**@brief Function for handling button events.
 *
 * @param[in]   pin_no         The pin number of the button pressed.
 * @param[in]   button_action  The action performed on button.
 */

// BUTTON PUSH !!!! :-D *************************************************************************************************************************************************************************

static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
#ifdef COMMISSIONING_ENABLED
    if ((button_action == APP_BUTTON_PUSH) && (pin_no == ERASE_BUTTON_PIN_NO))
    {
        APPL_LOG("[APPL]: Erasing all commissioning settings from persistent storage... \r\n");
        commissioning_settings_clear();
        return;
    }
#endif // COMMISSIONING_ENABLED

    if(mp_interface == NULL)
    {
        return;
    }

    uint32_t err_code;
    if (button_action == APP_BUTTON_PUSH)
    {
        coap_message_t      * p_request;
        coap_message_conf_t   message_conf;
        memset(&message_conf, 0x00, sizeof(message_conf));
        coap_remote_t         remote_device;
        
        message_conf.type             = COAP_MESSAGE_TYPE;
        message_conf.code             = COAP_CODE_PUT;
        message_conf.port.port_number = LOCAL_PORT_NUM;
        message_conf.id               = 0; // Auto-generate message ID.

        (void)uint16_encode(HTONS(m_global_token_count), message_conf.token);
        m_global_token_count++;

        message_conf.token_len = 2;
        message_conf.response_callback = coap_response_handler;

        err_code = coap_message_new(&p_request, &message_conf);

        APP_ERROR_CHECK(err_code);
        memcpy(&remote_device.addr[0], (uint8_t []){SERVER_IPV6_ADDRESS}, IPV6_ADDR_SIZE);
        remote_device.port_number = REMOTE_PORT_NUM;
        err_code = coap_message_remote_addr_set(p_request, &remote_device);
        APP_ERROR_CHECK(err_code);
        switch (pin_no)
        {
            case BUTTON_ONE:
            {
                err_code = coap_message_opt_str_add(p_request, COAP_OPT_URI_PATH, (uint8_t *)m_uri_part_lights, strlen(m_uri_part_lights));
                APP_ERROR_CHECK(err_code);

                err_code = coap_message_opt_str_add(p_request, COAP_OPT_URI_PATH, (uint8_t *)m_uri_part_led3, strlen(m_uri_part_led3));
                APP_ERROR_CHECK(err_code);

                uint8_t payload[] = {COMMAND_TOGGLE};
                err_code = coap_message_payload_set(p_request, payload, sizeof(payload));
                APP_ERROR_CHECK(err_code);

                uint32_t handle;
                err_code = coap_message_send(&handle, p_request);
                if (err_code != NRF_SUCCESS)
                {
                    APPL_LOG("[APPL]: CoAP Message skipped, error code = 0x%08lX.\r\n", err_code);
                }
                err_code = coap_message_delete(p_request);
                APP_ERROR_CHECK(err_code);

                break;
            }
            case BUTTON_TWO:
            {
                err_code = coap_message_opt_str_add(p_request, COAP_OPT_URI_PATH, (uint8_t *)"lights", 6);
                APP_ERROR_CHECK(err_code);

                err_code = coap_message_opt_str_add(p_request, COAP_OPT_URI_PATH, (uint8_t *)m_uri_part_led4, strlen(m_uri_part_led4));
                APP_ERROR_CHECK(err_code);

                uint8_t payload[] = {COMMAND_TOGGLE};
                err_code = coap_message_payload_set(p_request, payload, sizeof(payload));
                APP_ERROR_CHECK(err_code);

                uint32_t handle;
                err_code = coap_message_send(&handle, p_request);												// &handle -> get the ponter to the handle object, which is needed as input to the function
                if (err_code != NRF_SUCCESS)
                {
                    APPL_LOG("[APPL]: CoAP Message skipped, error code = 0x%08lX.\r\n", err_code);
                }
                err_code = coap_message_delete(p_request);
                APP_ERROR_CHECK(err_code);

                break;
            }
						case BUTTON_THREE:
						{
								/* Add what will happen when button three is pushed!
										
							Thoughts: Start sampling acceleration at a given rate, and send as a coAP message as done with btton ONE and TWO. 
										
								*/
							
							//readTWIUntilStopped();	
							
							
								break;
						}
				
            default:
                break;
        }
    }
}




/*void readTWIUntilStopped()	// Will read forever if not stopped by RESET or BUTTON4
{
	
	
	
}*/



/**@brief Function for the Button initialization.
 *
 * @details Initializes all Buttons used by this application.
 */
static void buttons_init(void)
{
    uint32_t err_code;

    static app_button_cfg_t buttons[] =
    {
        {BUTTON_ONE,          false, BUTTON_PULL, button_event_handler},
        {BUTTON_TWO,          false, BUTTON_PULL, button_event_handler},
				// Added by Sindre
				{BUTTON_THREE, 				false, BUTTON_PULL, button_event_handler},
#ifdef COMMISSIONING_ENABLED
        {ERASE_BUTTON_PIN_NO, false, BUTTON_PULL, button_event_handler}
#endif // COMMISSIONING_ENABLED
    };

    err_code = app_button_init(buttons, sizeof(buttons) / sizeof(buttons[0]), BUTTON_DETECTION_DELAY);
    APP_ERROR_CHECK(err_code);

    err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);
}


static void ip_app_handler(iot_interface_t * p_interface, ipv6_event_t * p_event)
{
    uint32_t    err_code;
    ipv6_addr_t src_addr;

    APPL_LOG("[APPL]: Got IP Application Handler Event on interface 0x%p\r\n", p_interface);

    switch(p_event->event_id)
    {
        case IPV6_EVT_INTERFACE_ADD:
#ifdef COMMISSIONING_ENABLED
            commissioning_joining_mode_timer_ctrl(JOINING_MODE_TIMER_STOP_RESET);
#endif // COMMISSIONING_ENABLED
            APPL_LOG("[APPL]: New interface added!\r\n");
            mp_interface = p_interface;
            m_disp_state = LEDS_IPV6_IF_UP;

            APPL_LOG("[APPL]: Sending Router Solicitation to all routers!\r\n");

            // Create Link Local addresses
            IPV6_CREATE_LINK_LOCAL_FROM_EUI64(&src_addr, p_interface->local_addr.identifier);

            // Delay first solicitation due to possible restriction on other side.
            nrf_delay_ms(APP_RTR_SOLICITATION_DELAY);

            // Send Router Solicitation to all routers.
            err_code = icmp6_rs_send(p_interface,
                                     &src_addr,
                                     &local_routers_multicast_addr);
            APP_ERROR_CHECK(err_code);
            break;

        case IPV6_EVT_INTERFACE_DELETE:
#ifdef COMMISSIONING_ENABLED
            commissioning_joining_mode_timer_ctrl(JOINING_MODE_TIMER_START);
#endif // COMMISSIONING_ENABLED
            APPL_LOG("[APPL]: Interface removed!\r\n");
            mp_interface = NULL;
            m_disp_state = LEDS_IPV6_IF_DOWN;
            break;

        case IPV6_EVT_INTERFACE_RX_DATA:
            APPL_LOG("[APPL]: Got unsupported protocol data!\r\n");
            break;

        default:
            //Unknown event. Should not happen.
            break;
    }
}


void well_known_core_callback(coap_resource_t * p_resource, coap_message_t * p_request)
{
    coap_message_conf_t response_config;
    memset (&response_config, 0, sizeof(coap_message_conf_t));

    if (p_request->header.type == COAP_TYPE_NON)
    {
        response_config.type = COAP_TYPE_NON;
    }
    else if (p_request->header.type == COAP_TYPE_CON)
    {
        response_config.type = COAP_TYPE_ACK;
    }

    // PIGGY BACKED RESPONSE
    response_config.code = COAP_CODE_205_CONTENT;
    // Copy message ID.
    response_config.id = p_request->header.id;
    // Set local port number to use.
    response_config.port.port_number = LOCAL_PORT_NUM;
    // Copy token.
    memcpy(&response_config.token[0], &p_request->token[0], p_request->header.token_len);
    // Copy token length.
    response_config.token_len = p_request->header.token_len;

    coap_message_t * p_response;
    uint32_t err_code = coap_message_new(&p_response, &response_config);
    APP_ERROR_CHECK(err_code);

    err_code = coap_message_remote_addr_set(p_response, &p_request->remote);
    APP_ERROR_CHECK(err_code);

    err_code = coap_message_opt_uint_add(p_response, COAP_OPT_CONTENT_FORMAT, \
                                         COAP_CT_APP_LINK_FORMAT);
    APP_ERROR_CHECK(err_code);

    err_code = coap_message_payload_set(p_response, m_well_known_core, \
                                        strlen((char *)m_well_known_core));
    APP_ERROR_CHECK(err_code);

    uint32_t handle;
    err_code = coap_message_send(&handle, p_response);
    APP_ERROR_CHECK(err_code);

    err_code = coap_message_delete(p_response);
    APP_ERROR_CHECK(err_code);
}


static void sensors_callback(coap_resource_t * p_resource, coap_message_t * p_request)
{
    coap_message_conf_t response_config;
    memset (&response_config, 0, sizeof(coap_message_conf_t));

    if (p_request->header.type == COAP_TYPE_NON)
    {
        response_config.type = COAP_TYPE_NON;
    }
    else if (p_request->header.type == COAP_TYPE_CON)
    {
        response_config.type = COAP_TYPE_ACK;
    }

    // PIGGY BACKED RESPONSE
    response_config.code = COAP_CODE_405_METHOD_NOT_ALLOWED;
    // Copy message ID.
    response_config.id = p_request->header.id;
    // Set local port number to use.
    response_config.port.port_number = LOCAL_PORT_NUM;
    // Copy token.
    memcpy(&response_config.token[0], &p_request->token[0], p_request->header.token_len);
    // Copy token length.
    response_config.token_len = p_request->header.token_len;

    coap_message_t * p_response;
    uint32_t err_code = coap_message_new(&p_response, &response_config);
    APP_ERROR_CHECK(err_code);

    err_code = coap_message_remote_addr_set(p_response, &p_request->remote);
    APP_ERROR_CHECK(err_code);

    uint32_t handle;
    err_code = coap_message_send(&handle, p_response);
    APP_ERROR_CHECK(err_code);

    err_code = coap_message_delete(p_response);
    APP_ERROR_CHECK(err_code);
}


static void thermometer_callback(coap_resource_t * p_resource, coap_message_t * p_request)
{
    coap_message_conf_t response_config;
    memset (&response_config, 0, sizeof(coap_message_conf_t));

    if (p_request->header.type == COAP_TYPE_NON)
    {
        response_config.type = COAP_TYPE_NON;
    }
    else if (p_request->header.type == COAP_TYPE_CON)
    {
        response_config.type = COAP_TYPE_ACK;
    }

    // PIGGY BACKED RESPONSE
    response_config.code = COAP_CODE_405_METHOD_NOT_ALLOWED;
    // Copy message ID.
    response_config.id = p_request->header.id;
    // Set local port number to use.
    response_config.port.port_number = LOCAL_PORT_NUM;
    // Copy token.
    memcpy(&response_config.token[0], &p_request->token[0], p_request->header.token_len);
    // Copy token length.
    response_config.token_len = p_request->header.token_len;

    coap_message_t * p_response;
    uint32_t err_code = coap_message_new(&p_response, &response_config);
    APP_ERROR_CHECK(err_code);

    err_code = coap_message_remote_addr_set(p_response, &p_request->remote);
    APP_ERROR_CHECK(err_code);

    switch (p_request->header.code)
    {
        case COAP_CODE_GET:
        {
            p_response->header.code = COAP_CODE_205_CONTENT;

            // Set response payload to actual thermometer value.
            char response_str[5];
            memset(response_str, '\0', sizeof(response_str));
            sprintf(response_str, "%d", m_temperature);
            err_code = coap_message_payload_set(p_response, response_str, strlen(response_str));
            APP_ERROR_CHECK(err_code);

            break;
        }
        case COAP_CODE_PUT:
        {
            if ((p_request->payload_len == 0) || (p_request->payload_len > 4))  // Input value cannot be more than 4 characters (1 sign + 3 digits).
            {
                p_response->header.code = COAP_CODE_400_BAD_REQUEST;
                break;
            }

            uint32_t i;
            for (i = 0; i < p_request->payload_len; ++i)
            {
                if (i == 0)
                {
                    // The first digit of the input value must be the ASCII code of a decimal number or a minus sign or a plus sign.
                    if ((((p_request->p_payload[i] < 0x30) && (p_request->p_payload[i] != 0x2B)) && \
                        ((p_request->p_payload[i] < 0x30) && (p_request->p_payload[i] != 0x2D))) || \
                        (p_request->p_payload[i] > 0x39))
                    {
                        p_response->header.code = COAP_CODE_400_BAD_REQUEST;
                        break;
                    }
                }
                else
                {
                    // The remaining digits of the input value must be ASCII codes of decimal numbers.
                    if ((p_request->p_payload[i] < 0x30) || (p_request->p_payload[i] > 0x39))
                    {
                        p_response->header.code = COAP_CODE_400_BAD_REQUEST;
                        break;
                    }
                }
            }

            char input_str[5];
            memset(input_str, '\0', sizeof(input_str));
            memcpy(input_str, p_request->p_payload, p_request->payload_len);

            if ((atoi(input_str) < -100) || (atoi(input_str) > 100))    // Input value must be in valid range.
            {
                p_response->header.code = COAP_CODE_400_BAD_REQUEST;
                break;
            }

            m_temperature = atoi(input_str);

            p_response->header.code = COAP_CODE_204_CHANGED;
            break;
        }
        default:
        {
            p_response->header.code = COAP_CODE_400_BAD_REQUEST;
            break;
        }
    }

    uint32_t handle;
    err_code = coap_message_send(&handle, p_response);
    APP_ERROR_CHECK(err_code);

    err_code = coap_message_delete(p_response);
    APP_ERROR_CHECK(err_code);
}


static void coap_endpoints_init(void)
{
    uint32_t err_code;

    static coap_resource_t root;
    err_code = coap_resource_create(&root, "/");
    APP_ERROR_CHECK(err_code);

    static coap_resource_t well_known;
    err_code = coap_resource_create(&well_known, ".well-known");
    APP_ERROR_CHECK(err_code);
    err_code = coap_resource_child_add(&root, &well_known);
    APP_ERROR_CHECK(err_code);

    static coap_resource_t core;
    err_code = coap_resource_create(&core, "core");
    APP_ERROR_CHECK(err_code);

    core.permission = COAP_PERM_GET;
    core.callback   = well_known_core_callback;
    err_code = coap_resource_child_add(&well_known, &core);
    APP_ERROR_CHECK(err_code);

    static coap_resource_t sensors;
    err_code = coap_resource_create(&sensors, "sensors");
    APP_ERROR_CHECK(err_code);

    sensors.callback = sensors_callback;
    err_code = coap_resource_child_add(&root, &sensors);
    APP_ERROR_CHECK(err_code);

    static coap_resource_t thermometer;
    err_code = coap_resource_create(&thermometer, "thermometer");
    APP_ERROR_CHECK(err_code);

    thermometer.permission = (COAP_PERM_GET | COAP_PERM_PUT);
    thermometer.callback = thermometer_callback;
    err_code = coap_resource_child_add(&sensors, &thermometer);
    APP_ERROR_CHECK(err_code);

    uint16_t size = sizeof(m_well_known_core);
    err_code = coap_resource_well_known_generate(m_well_known_core, &size);
    APP_ERROR_CHECK(err_code);
}


/**@brief ICMP6 module event handler.
 *
 * @details Callback registered with the ICMP6 module to receive asynchronous events from
 *          the module, if the ICMP6_ENABLE_ALL_MESSAGES_TO_APPLICATION constant is not zero
 *          or the ICMP6_ENABLE_ND6_MESSAGES_TO_APPLICATION constant is not zero.
 */
uint32_t icmp6_handler(iot_interface_t  * p_interface,
                       ipv6_header_t    * p_ip_header,
                       icmp6_header_t   * p_icmp_header,
                       uint32_t           process_result,
                       iot_pbuffer_t    * p_rx_packet)
{
    APPL_LOG("[APPL]: Got ICMP6 Application Handler Event on interface 0x%p\r\n", p_interface);

    APPL_LOG("[APPL]: Source IPv6 Address: ");
    APPL_ADDR(p_ip_header->srcaddr);
    APPL_LOG("[APPL]: Destination IPv6 Address: ");
    APPL_ADDR(p_ip_header->destaddr);
    APPL_LOG("[APPL]: Process result = 0x%08lx\r\n", process_result);

    switch(p_icmp_header->type)
    {
        case ICMP6_TYPE_DESTINATION_UNREACHABLE:
            APPL_LOG("[APPL]: ICMP6 Message Type = Destination Unreachable Error\r\n");
            break;
        case ICMP6_TYPE_PACKET_TOO_LONG:
            APPL_LOG("[APPL]: ICMP6 Message Type = Packet Too Long Error\r\n");
            break;
        case ICMP6_TYPE_TIME_EXCEED:
            APPL_LOG("[APPL]: ICMP6 Message Type = Time Exceed Error\r\n");
            break;
        case ICMP6_TYPE_PARAMETER_PROBLEM:
            APPL_LOG("[APPL]: ICMP6 Message Type = Parameter Problem Error\r\n");
            break;
        case ICMP6_TYPE_ECHO_REQUEST:
            APPL_LOG("[APPL]: ICMP6 Message Type = Echo Request\r\n");
            break;
        case ICMP6_TYPE_ECHO_REPLY:
            APPL_LOG("[APPL]: ICMP6 Message Type = Echo Reply\r\n");
            break;
        case ICMP6_TYPE_ROUTER_SOLICITATION:
            APPL_LOG("[APPL]: ICMP6 Message Type = Router Solicitation\r\n");
            break;
        case ICMP6_TYPE_ROUTER_ADVERTISEMENT:
            APPL_LOG("[APPL]: ICMP6 Message Type = Router Advertisement\r\n");
            break;
        case ICMP6_TYPE_NEIGHBOR_SOLICITATION:
            APPL_LOG("[APPL]: ICMP6 Message Type = Neighbor Solicitation\r\n");
            break;
        case ICMP6_TYPE_NEIGHBOR_ADVERTISEMENT:
            APPL_LOG("[APPL]: ICMP6 Message Type = Neighbor Advertisement\r\n");
            break;
        default:
            break;
    }

    return NRF_SUCCESS;
}


/**@brief Function for initializing IP stack.
 *
 * @details Initialize the IP Stack.
 */
static void ip_stack_init(void)
{
    uint32_t    err_code;
    ipv6_init_t init_param;

    err_code = ipv6_medium_eui64_get(m_ipv6_medium.ipv6_medium_instance_id, \
                                     &eui64_local_iid);
    APP_ERROR_CHECK(err_code);

    init_param.p_eui64       = &eui64_local_iid;
    init_param.event_handler = ip_app_handler;

    err_code = ipv6_init(&init_param);
    APP_ERROR_CHECK(err_code);

    err_code = icmp6_receive_register(icmp6_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


static void coap_error_handler(uint32_t error_code, coap_message_t * p_message)
{
    // If any response fill the p_response with a appropiate response message.
}


/**@brief Function for starting connectable mode.
 */
static void connectable_mode_enter(void)
{
    uint32_t err_code = ipv6_medium_connectable_mode_enter(m_ipv6_medium.ipv6_medium_instance_id);
    APP_ERROR_CHECK(err_code);

    APPL_LOG("[APPL]: Physical layer in connectable mode.\r\n");
    m_disp_state = LEDS_CONNECTABLE_MODE;
}


static void on_ipv6_medium_evt(ipv6_medium_evt_t * p_ipv6_medium_evt)
{
    switch (p_ipv6_medium_evt->ipv6_medium_evt_id)
    {
        case IPV6_MEDIUM_EVT_CONN_UP:
        {
            APPL_LOG("[APPL]: Physical layer: connected.\r\n");
            m_disp_state = LEDS_IPV6_IF_DOWN;
            break;
        }
        case IPV6_MEDIUM_EVT_CONN_DOWN:
        {
            APPL_LOG("[APPL]: Physical layer: disconnected.\r\n");
            connectable_mode_enter();
            break;
        }
        default:
        {
            break;
        }
    }
}


static void on_ipv6_medium_error(ipv6_medium_error_t * p_ipv6_medium_error)
{
    // Do something.
}


#ifdef COMMISSIONING_ENABLED
void commissioning_id_mode_cb(mode_control_cmd_t control_command)
{
    switch (control_command)
    {
        case CMD_IDENTITY_MODE_ENTER:
        {
            LEDS_OFF(LED_THREE | LED_FOUR);
            m_identity_mode_active = true;

            break;
        }
        case CMD_IDENTITY_MODE_EXIT:
        {
            m_identity_mode_active = false;
            LEDS_OFF((LED_THREE | LED_FOUR));

            break;
        }
        default:
        {
            
            break;
        }
    }
}


void commissioning_power_off_cb(bool power_off_on_failure)
{
    m_power_off_on_failure = power_off_on_failure;

    APPL_LOG("[APPL]: Commissioning: do power_off on failure: %s.\r\n", \
             m_power_off_on_failure ? "true" : "false");
}
#endif // COMMISSIONING_ENABLED


/**@brief Function for application main entry.
 */
int main(void)
{
    uint32_t err_code;
    
		// Added by Sindre
	
//		Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
		
	
    // Initialize
    app_trace_init();
    leds_init();

#ifdef COMMISSIONING_ENABLED
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);
#endif // COMMISSIONING_ENABLED

    timers_init();
    buttons_init();

    static ipv6_medium_init_params_t ipv6_medium_init_params;
    memset(&ipv6_medium_init_params, 0x00, sizeof(ipv6_medium_init_params));
    ipv6_medium_init_params.ipv6_medium_evt_handler    = on_ipv6_medium_evt;
    ipv6_medium_init_params.ipv6_medium_error_handler  = on_ipv6_medium_error;
    ipv6_medium_init_params.use_scheduler              = false;
#ifdef COMMISSIONING_ENABLED
    ipv6_medium_init_params.commissioning_id_mode_cb   = commissioning_id_mode_cb;
    ipv6_medium_init_params.commissioning_power_off_cb = commissioning_power_off_cb;
#endif // COMMISSIONING_ENABLED

    err_code = ipv6_medium_init(&ipv6_medium_init_params, \
                                IPV6_MEDIUM_ID_BLE,       \
                                &m_ipv6_medium);
    APP_ERROR_CHECK(err_code);

    eui48_t ipv6_medium_eui48;
    err_code = ipv6_medium_eui48_get(m_ipv6_medium.ipv6_medium_instance_id, \
                                     &ipv6_medium_eui48);

    ipv6_medium_eui48.identifier[EUI_48_SIZE - 1] = 0x00;

    err_code = ipv6_medium_eui48_set(m_ipv6_medium.ipv6_medium_instance_id, \
                                     &ipv6_medium_eui48);
    APP_ERROR_CHECK(err_code);

    ip_stack_init();

    coap_port_t local_port_list[COAP_PORT_COUNT] = 
    {
        {.port_number = LOCAL_PORT_NUM}
    };
    
    coap_transport_init_t port_list;
    port_list.p_port_table = &local_port_list[0];

    err_code = coap_init(71, &port_list);
    APP_ERROR_CHECK(err_code);

    err_code = coap_error_handler_register(coap_error_handler);
    APP_ERROR_CHECK(err_code);

    coap_endpoints_init();

    iot_timer_init();

    APPL_LOG("\r\n");
    APPL_LOG("[APPL]: Init complete.\r\n");

		
		// Added by Sindre, added main from TWI example file, removed duplicates
		
		//ret_code_t err_code; 	Removed because of redefinition of err_code
		
    /* Initialization of UART */
    LEDS_CONFIGURE(LEDS_MASK);
    LEDS_OFF(LEDS_MASK);

    const app_uart_comm_params_t comm_params =
    {
        RX_PIN_NUMBER,
        TX_PIN_NUMBER,
        RTS_PIN_NUMBER,
        CTS_PIN_NUMBER,
        APP_UART_FLOW_CONTROL_ENABLED,
        false,
        UART_BAUDRATE_BAUDRATE_Baud38400
    };

    //APP_UART_FIFO_INIT(&comm_params,
    //                   UART_RX_BUF_SIZE,
    //                   UART_TX_BUF_SIZE,
    //                   uart_error_handle,
    //                   APP_IRQ_PRIORITY_LOW,
    //                   err_code);

    APP_ERROR_CHECK(err_code);

    /* Initializing simulated EEPROM */
    //err_code = eeprom_simulator_init();
    //APP_ERROR_CHECK(err_code);

    /* Initializing TWI master interface for EEPROM */
    err_code = twi_master_init();
    APP_ERROR_CHECK(err_code);


    /* Welcome message */
    UNUSED_VARIABLE(puts(
            "This is TWIS and TWI usage example\n"
            "You can access simulated EEPROM memory using following commands:"
    ));
    UNUSED_VARIABLE(puts(m_cmd_help_str));
    
    UNUSED_VARIABLE(fflush(stdout));
		
		
		
		
		
		
		
		
    // Start execution
    connectable_mode_enter();

    // Enter main loop
		/* Commented out original code by Sindre, putting in main loop from TWI example
    for (;;)
    {
        power_manage();
    }*/
		
		/* Main loop */
    while(true)
    {
				power_manage();		// From CoAP client example file
        
				uint8_t c;
        while(NRF_SUCCESS != app_uart_get(&c))
        {
            // Just waiting
        }
        switch((char)c)
        {
        case '\n':
        case '\r':
            break;
        case 'p':
            do_print_data();
            break;
        case 'w':
						// Send to coAP 
            do_string_write();
            break;
        case 'c':
            do_clear_eeprom();
            break;
        case 'x':
            {
                //uint32_t error = eeprom_simulator_error_get_and_clear();
                //printf("Error word: %x\n", (unsigned int)error);
            }
            break;
        default:
            printf("You selected %c\n", (char)c);
            UNUSED_VARIABLE(puts("Unknown command, try one of the following:"));
            //UNUSED_VARIABLE(puts(m_cmd_help_str));
            break;
        }
     //   if(eeprom_simulator_error_check())
     //				{
     //       UNUSED_VARIABLE(puts(
     //               "WARNING: EEPROM transmission error detected.\n"
     //               "Use 'x' command to read error word."
     //       ));
     //       UNUSED_VARIABLE(fflush(stdout));
     //   }
    }
		
		
}

/**
 * @}
 */
