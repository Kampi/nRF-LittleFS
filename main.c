#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include <stdio.h>

#include "filesystem.h"

static nrf_drv_wdt_channel_id WDT_Channel_ID;

/** @brief Watchdog timer event handler.
 */
static void Watchdog_On_Event(void)
{
    NRF_LOG_INFO("Watchdog reset!");
    NRF_LOG_FINAL_FLUSH();

    #ifdef SOFTDEVICE_PRESENT
	sd_nvic_SystemReset();
    #else
	NVIC_SystemReset();
    #endif
}

/** @brief Initialize the watchdog timer.
 */
static void Init_Watchdog(void)
{
    NRF_LOG_DEBUG("Initialize Watchdog Timer...");
    nrf_drv_wdt_config_t WDT_Config = NRF_DRV_WDT_DEAFULT_CONFIG;
    APP_ERROR_CHECK(nrf_drv_wdt_init(&WDT_Config, Watchdog_On_Event));
    APP_ERROR_CHECK(nrf_drv_wdt_channel_alloc(&WDT_Channel_ID));
    nrf_drv_wdt_enable();
}

/** @brief Initialize the logging module.
 */
static void Init_Log(void)
{
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

int main(void)
{
    Init_Log();
    Init_Watchdog();

    NRF_LOG_INFO("--- LittleFS example ---");
    if(FileSystem_Init(false, WDT_Channel_ID))
    {
        NRF_LOG_ERROR("Can not initialize file system!");
    }
    else
    {
        NRF_LOG_INFO("Running memory test...");
        if(FileSystem_MemTest())
        {
            NRF_LOG_ERROR("Memory test failed!");
        }
        else
        {
            NRF_LOG_INFO("Memory test successful!");
            FileSystem_WriteTestFile();
        }
    }

    while(1)
    {
        if(!NRF_LOG_PROCESS())
        {
            NRF_LOG_FLUSH();
        }
    }
}