#include <project_config.hpp>

#include <cstdint>
#include <iterator>

#include "L2_HAL/displays/led/onboard_led.hpp"
#include "utility/log.hpp"
#include "utility/time.hpp"

/* FreeRTOS+FAT includes. */
#include "third_party/FreeRTOS-Plus-FAT/include/ff_headers.h"
#include "ff_sys.h"
#include "ff_stdio.h"

FF_Disk_t *FF_SDCard_FAT_Init( char *pcName,
                          uint32_t ulSectorCount,
                          size_t xIOManagerCacheSize );

void DIRCommand( const char *pcDirectoryToScan )
{
FF_FindData_t *pxFindStruct;
const char  *pcAttrib,
            *pcWritableFile = "writable file",
            *pcReadOnlyFile = "read only file",
            *pcDirectory = "directory";

    /* FF_FindData_t can be large, so it is best to allocate the structure
    dynamically, rather than declare it as a stack variable. */
    pxFindStruct = ( FF_FindData_t * ) pvPortMalloc( sizeof( FF_FindData_t ) );

    /* FF_FindData_t must be cleared to 0. */
    memset( pxFindStruct, 0x00, sizeof( FF_FindData_t ) );

    /* The first parameter to ff_findfist() is the directory being searched.  Do
    not add wildcards to the end of the directory name. */
    if( ff_findfirst( pcDirectoryToScan, pxFindStruct ) == 0 )
    {
        do
        {
            /* Point pcAttrib to a string that describes the file. */
            if( ( pxFindStruct->ucAttributes & FF_FAT_ATTR_DIR ) != 0 )
            {
                pcAttrib = pcDirectory;
            }
            else if( pxFindStruct->ucAttributes & FF_FAT_ATTR_READONLY )
            {
                pcAttrib = pcReadOnlyFile;
            }
            else
            {
                pcAttrib = pcWritableFile;
            }

            /* Print the files name, size, and attribute string. */
            printf( "%s [%s] [size=%d]", pxFindStruct->pcFileName,
                                                  pcAttrib,
                                                  pxFindStruct->ulFileSize );

        } while( ff_findnext( pxFindStruct ) == 0 );
    }

    /* Free the allocated FF_FindData_t structure. */
    vPortFree( pxFindStruct );
}

void test_task(void * pvParam)
{
  LOG_INFO("Staring Hello World Application");
  LOG_INFO("Initializing LEDs...");
  OnBoardLed leds;
  leds.Initialize();
  LOG_INFO("LEDs Initialized! %f", 1234.123456);

  LOG_INFO("Starting FATFS");
  FF_Disk_t * onbard_disk = FF_SDCard_FAT_Init("phat", 1024, 4096);
  LOG_INFO("FATFS Started!");

  for (int i = 0; i < 3; i++)
  {
    DIRCommand("/");
  }



  while (true)
  {
    for (uint8_t i = 0; i < 15; i++)
    {
      LOG_INFO("Hello World 0x%X", i);
      leds.SetAll(i);
      vTaskDelay(500);
    }
  }
}

int main(void)
{

  xTaskCreate(test_task, "test_task", 4096, NULL, 1, NULL);
  vTaskStartScheduler();

  while(true);

  return 0;
}
