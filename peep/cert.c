/***** Includes *****/

#include <string.h>
#include "system.h"
#include "cert.h"
#include "nvs.h"
#include "nvs_flash.h"

/***** Local Defines **/

#define CERT_PARTITION_NAME         "nvs_cert"
#define CERT_NAMESPACE_NAME         "peep_cert"

/***** Local Data *****/

typedef struct
{
   SemaphoreHandle_t Mutex;
   nvs_handle        handle;
} CertContext_t;

static CertContext_t CertContext;

/***** Local Functions *****/

static char *CertItemToString(cert_item_t item)
{
   char *Result;

   switch(item)
   {
      default:
      case CERT_ITEM_INVALID:
         Result = NULL;
         break;
      case CERT_ITEM_ROOT_CA:
         Result = "peep_root_ca";
         break;
      case CERT_ITEM_CERTIFICATE:
         Result = "peep_cert";
         break;
      case CERT_ITEM_UUID:
         Result = "peep_uuid";
         break;
      case CERT_ITEM_KEY:
         Result = "peep_key";
         break;
   }

   return(Result);
}

/***** Global Functions *****/

bool cert_init(void)
{
   bool      Result;
   esp_err_t esp_err;

   /* Initialize the flash partition that is used for stored per-peep   */
   /* certificates and UUID.                                            */
   esp_err = nvs_flash_init_partition(CERT_PARTITION_NAME);
   if(esp_err == ESP_OK)
   {
      /* Create recursive mutex to guard globals.                       */
      CertContext.Mutex = xSemaphoreCreateRecursiveMutex();
      if (CertContext.Mutex != NULL)
      {
         /* Open the NVS partition.                                     */
         esp_err = nvs_open_from_partition(CERT_PARTITION_NAME, CERT_NAMESPACE_NAME, NVS_READONLY, &(CertContext.handle));
         if(esp_err == ESP_OK)
         {
            LOGI("%s: Certificate module initialized", __func__);

            Result = true;
         }
         else
         {
            LOGE("%s: Failed to open %s namespace for nvs operations 0x%02X", __func__, CERT_NAMESPACE_NAME, (uint8_t)esp_err);

            Result = false;
         }
      }
      else
      {
         LOGE("%s: failed to create mutex", __func__);

         Result = false;
      }
   }
   else
   {
      LOGE("%s: Failed to initialize partition nvs_cert for nvs operation 0x%02X", __func__, (uint8_t)esp_err);

      Result = false;
   }

   return(Result);
}

void cert_deinit(void)
{
   esp_err_t         esp_err;
   SemaphoreHandle_t temp;

   if (CertContext.Mutex != NULL)
   {
      /* Acquire the context mutex.                                     */
      if(xSemaphoreTake(CertContext.Mutex, portMAX_DELAY))
      {
         /* Close the nvs partion.                                      */
         nvs_close(CertContext.handle);

         CertContext.handle = (nvs_handle)0;

         /* De-init the flash partition.                                */
         esp_err = nvs_flash_deinit_partition(CERT_PARTITION_NAME);
         if(esp_err != ESP_OK)
         {
            LOGE("%s: failed to de-init nvs flash partition %s", __func__, CERT_PARTITION_NAME);
         }

         /* Delete the semaphore.                                       */
         temp              = CertContext.Mutex;
         CertContext.Mutex = NULL;
         vSemaphoreDelete(temp);
      }
   }
}

bool cert_get_item_size(cert_item_t item, size_t *item_length)
{
   bool       Result;
   char      *item_string;
   esp_err_t  esp_err;

   /* Verify the input parameters.                                      */
   if ((CertContext.Mutex != NULL) && (item_length != NULL))
   {
      /* Acquire the context mutex.                                     */
      if(xSemaphoreTake(CertContext.Mutex, portMAX_DELAY))
      {
         /* Convert enum to string we can use to query the NVS API.     */
         item_string = CertItemToString(item);
         if(item_string != NULL)
         {
            /* Call nvs_get_str() with out_val == NULL to get required  */
            /* length.                                                  */
            esp_err = nvs_get_str(CertContext.handle, item_string, NULL, item_length);
            if(esp_err == ESP_OK)
            {
               LOGI("%s: item %s required length %u", __func__, item_string, (unsigned int)*item_length);

               Result = true;
            }
            else
            {
               LOGE("%s: Failed to get NVS Key %s, error 0x%02X", __func__, item_string, (uint8_t)esp_err);

               Result = false;
            }
         }
         else
         {
            LOGE("%s: invalid item %d", __func__, item);

            Result = false;
         }

         /* Release the mutex.                                          */
         xSemaphoreGive(CertContext.Mutex);
      }
      else
      {
         LOGE("%s: failed to acquire mutex", __func__);

         Result = false;
      }
   }
   else
   {
      LOGE("%s: invalid state or parameters", __func__);

      Result = false;
   }

   return(Result);
}

bool cert_get_item(cert_item_t item, size_t *item_buffer_length, char *item_buffer)
{
   bool       Result;
   char      *item_string;
   esp_err_t  esp_err;

   /* Verify the input parameters.                                      */
   if ((CertContext.Mutex != NULL) && (item_buffer_length != NULL) && (*item_buffer_length > 0) && (item_buffer != NULL))
   {
      /* Acquire the context mutex.                                     */
      if(xSemaphoreTake(CertContext.Mutex, portMAX_DELAY))
      {
         /* Convert enum to string we can use to query the NVS API.     */
         item_string = CertItemToString(item);
         if(item_string != NULL)
         {
            /* Call nvs_get_str() with out_val == NULL to get required  */
            /* length.                                                  */
            esp_err = nvs_get_str(CertContext.handle, item_string, item_buffer, item_buffer_length);
            if(esp_err == ESP_OK)
            {
               LOGI("%s: Read Item = %s of length %u", __func__, item_string, (unsigned int)*item_buffer_length);

               LOGD("%s: %s = %s", __func__, item_string, item_buffer);

               Result = true;
            }
            else
            {
               LOGE("%s: Failed to get NVS Key %s, error 0x%02X", __func__, item_string, (uint8_t)esp_err);

               Result = false;
            }
         }
         else
         {
            LOGE("%s: invalid item %d", __func__, item);

            Result = false;
         }

         /* Release the mutex.                                          */
         xSemaphoreGive(CertContext.Mutex);
      }
      else
      {
         LOGE("%s: failed to acquire mutex", __func__);

         Result = false;
      }
   }
   else
   {
      LOGE("%s: invalid state or parameters", __func__);

      Result = false;
   }

   return(Result);
}
