/* Standard includes. */
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "portmacro.h"

/* FreeRTOS+FAT includes. */
#include "ff_headers.h"
#include "ff_sys.h"

/* SD Card API includes*/
#include "L2_HAL/memory/sddriver.hpp"

extern "C" int32_t prvWriteSD (uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk)
{
	onboard_card.WriteBlock(ulSectorNumber * 512, pucBuffer, ulSectorCount);
	return FF_ERR_NONE;
}

extern "C" int32_t prvReadSD (uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk)
{
	onboard_card.ReadBlock(ulSectorNumber * 512, pucBuffer, ulSectorCount);
	return FF_ERR_NONE;
}


FF_Disk_t * FF_SDCard_FAT_Init( char *pcName, uint32_t ulSectorCount, size_t xIOManagerCacheSize )
{
FF_Error_t xError;
FF_Disk_t *pxDisk = NULL;
FF_CreationParameters_t xParameters;

    /* Check the validity of the xIOManagerCacheSize parameter. */
    configASSERT( ( xIOManagerCacheSize % ramSECTOR_SIZE ) == 0 );
    configASSERT( ( xIOManagerCacheSize >= ( 2 * ramSECTOR_SIZE ) ) );

    /* Attempt to allocated the FF_Disk_t structure. */
    pxDisk = ( FF_Disk_t * ) pvPortMalloc( sizeof( FF_Disk_t ) );

    if( pxDisk != NULL )
    {
        /* It is advisable to clear the entire structure to zero after it has been
        allocated - that way the media driver will be compatible with future
        FreeRTOS+FAT versions, in which the FF_Disk_t structure may include
        additional members. */
        memset( pxDisk, '\0', sizeof( FF_Disk_t ) );

        /* The pvTag member of the FF_Disk_t structure allows the structure to be
        extended to also include media specific parameters.  The only media
        specific data that needs to be stored in the FF_Disk_t structure for a
        RAM disk is the location of the RAM buffer itself - so this is stored
        directly in the FF_Disk_t's pvTag member. */
        pxDisk->pvTag = NULL;

        /* The signature is used by the disk read and disk write functions to
        ensure the disk being accessed is a RAM disk. */
        pxDisk->ulSignature = 0;

        /* The number of sectors is recorded for bounds checking in the read and
        write functions. */
        pxDisk->ulNumberOfSectors = ulSectorCount;

        /* Create the IO manager that will be used to control the RAM disk -
        the FF_CreationParameters_t structure completed with the required
        parameters, then passed into the FF_CreateIOManager() function. */
        memset (&xParameters, '\0', sizeof xParameters);
        xParameters.pucCacheMemory = NULL;
        xParameters.ulMemorySize = xIOManagerCacheSize;
        xParameters.ulSectorSize = 512;
        xParameters.fnWriteBlocks = prvWriteSD;
        xParameters.fnReadBlocks = prvReadSD;
        xParameters.pxDisk = pxDisk;

        /* The driver is re-entrant as it just accesses RAM using memcpy(), so
        xBlockDeviceIsReentrant can be set to pdTRUE.  In this case the
        semaphore is only used to protect FAT data structures, and not the read
        and write function. */
        xParameters.pvSemaphore = ( void * ) xSemaphoreCreateRecursiveMutex();
        xParameters.xBlockDeviceIsReentrant = pdFALSE;


        pxDisk->pxIOManager = FF_CreateIOManger( &xParameters, &xError );

        if( ( pxDisk->pxIOManager != NULL ) && ( FF_isERR( xError ) == pdFALSE ) )
        {
            /* Record that the RAM disk has been initialised. */
            pxDisk->xStatus.bIsInitialised = pdTRUE;

            /* Initialize SD Card and Mount it. sd_card_info might be the key
            to determining the size of the SD card in the future. */
            Sd::CardInfo_t sd_card_info;
            onboard_card.Initialize();
            xError = (0 == onboard_card.Mount(&sd_card_info));

            if( FF_isERR( xError ) == pdFALSE )
            {
                /* Record the partition number the FF_Disk_t structure is, then
                mount the partition. */
                pxDisk->xStatus.bPartitionNumber = 0;

                /* Mount the partition. */
                xError = FF_Mount( pxDisk, 0);
            }

            if( FF_isERR( xError ) == pdFALSE )
            {
                /* The partition mounted successfully, add it to the virtual
                file system - where it will appear as a directory off the file
                system's root directory. */
                FF_FS_Add( pcName, pxDisk );
            }
        }
        else
        {
            /* The disk structure was allocated, but the disk's IO manager could
            not be allocated, so free the disk again. */
            pxDisk = NULL;
        }
    }

    return pxDisk;
}