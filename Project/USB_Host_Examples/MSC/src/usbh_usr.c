/**
  ******************************************************************************
  * @file    usbh_usr.c
  * @author  MCD Application Team
  * @version V2.1.0
  * @date    19-March-2012
  * @brief   This file includes the usb host library user callbacks
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "usbh_usr.h"
#include "app_trace.h"
#include "ff.h"       /* FATFS */
#include "usbh_msc_core.h"
#include "usbh_msc_scsi.h"
#include "usbh_msc_bot.h"

/** @addtogroup USBH_USER
* @{
*/

/** @addtogroup USBH_MSC_DEMO_USER_CALLBACKS
* @{
*/

/** @defgroup USBH_USR 
* @brief    This file includes the usb host stack user callbacks
* @{
*/ 

/** @defgroup USBH_USR_Private_TypesDefinitions
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Defines
* @{
*/ 
#define IMAGE_BUFFER_SIZE    512
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Macros
* @{
*/ 
extern USB_OTG_CORE_HANDLE          USB_OTG_Core;
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Variables
* @{
*/ 
uint8_t USBH_USR_ApplicationState = USH_USR_FS_INIT;
uint8_t filenameString[15]  = {0};

FATFS fatfs;
FIL file;
uint8_t Image_Buf[IMAGE_BUFFER_SIZE];
uint8_t line_idx = 0;   

/*  Points to the DEVICE_PROP structure of current device */
/*  The purpose of this register is to speed up the execution */

USBH_Usr_cb_TypeDef USR_cb =
{
  USBH_USR_Init,
  USBH_USR_DeInit,
  USBH_USR_DeviceAttached,
  USBH_USR_ResetDevice,
  USBH_USR_DeviceDisconnected,
  USBH_USR_OverCurrentDetected,
  USBH_USR_DeviceSpeedDetected,
  USBH_USR_Device_DescAvailable,
  USBH_USR_DeviceAddressAssigned,
  USBH_USR_Configuration_DescAvailable,
  USBH_USR_Manufacturer_String,
  USBH_USR_Product_String,
  USBH_USR_SerialNum_String,
  USBH_USR_EnumerationDone,
  USBH_USR_UserInput,
  USBH_USR_MSC_Application,
  USBH_USR_DeviceNotSupported,
  USBH_USR_UnrecoveredError
    
};

/**
* @}
*/

/** @defgroup USBH_USR_Private_Constants
* @{
*/ 
/*--------------- LCD Messages ---------------*/
const uint8_t MSG_HOST_INIT[]        = "> Host Library Initialized";
const uint8_t MSG_DEV_ATTACHED[]     = "> Device Attached";
const uint8_t MSG_DEV_DISCONNECTED[] = "> Device Disconnected";
const uint8_t MSG_DEV_ENUMERATED[]   = "> Enumeration completed";
const uint8_t MSG_DEV_HIGHSPEED[]    = "> High speed device detected";
const uint8_t MSG_DEV_FULLSPEED[]    = "> Full speed device detected";
const uint8_t MSG_DEV_LOWSPEED[]     = "> Low speed device detected";
const uint8_t MSG_DEV_ERROR[]        = "> Device fault";

const uint8_t MSG_MSC_CLASS[]        = "> Mass storage device connected";
const uint8_t MSG_HID_CLASS[]        = "> HID device connected";
const uint8_t MSG_DISK_SIZE[]        = "> Size of the disk in MBytes: ";
const uint8_t MSG_LUN[]              = "> LUN Available in the device:";
const uint8_t MSG_ROOT_CONT[]        = "> Exploring disk flash ...";
const uint8_t MSG_WR_PROTECT[]       = "> The disk is write protected";
const uint8_t MSG_UNREC_ERROR[]      = "> UNRECOVERED ERROR STATE";

/**
* @}
*/


/** @defgroup USBH_USR_Private_FunctionPrototypes
* @{
*/
static uint8_t Explore_Disk (char* path , uint8_t recu_level);
static uint8_t Image_Browser (char* path);
static void     Show_Image(void);
static void     Toggle_Leds(void);
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Functions
* @{
*/ 


/**
* @brief  USBH_USR_Init 
*         Displays the message on LCD for host lib initialization
* @param  None
* @retval None
*/
void USBH_USR_Init(void)
{
    static uint8_t startup = 0;  
    
    APP_LOG("[APP]: > USBH_USR_Init.\r\n"); 
    if(startup == 0 )
    {
        startup = 1;
        /* Configure the LEDs */
        STM_EVAL_LEDInit(LED1);
        STM_EVAL_LEDInit(LED2);
        STM_EVAL_LEDInit(LED3); 
        STM_EVAL_LEDInit(LED4); 

        STM_EVAL_PBInit(BUTTON_KEY, BUTTON_MODE_GPIO);

        APP_LOG("[APP]: > USB Host library started.\r\n"); 
    }
}

/**
* @brief  USBH_USR_DeviceAttached 
*         Displays the message on LCD on device attached
* @param  None
* @retval None
*/
void USBH_USR_DeviceAttached(void)
{
    APP_LOG("[APP]: %s.\r\n",(unsigned char *)MSG_DEV_ATTACHED);
}


/**
* @brief  USBH_USR_UnrecoveredError
* @param  None
* @retval None
*/
void USBH_USR_UnrecoveredError (void)
{
    APP_LOG("[APP]: %s.\r\n",(unsigned char *)MSG_UNREC_ERROR);
}


/**
* @brief  USBH_DisconnectEvent
*         Device disconnect event
* @param  None
* @retval Staus
*/
void USBH_USR_DeviceDisconnected (void)
{
    APP_LOG("[APP]: %s.\r\n",(uint8_t *)MSG_DEV_DISCONNECTED);
}
/**
* @brief  USBH_USR_ResetUSBDevice 
* @param  None
* @retval None
*/
void USBH_USR_ResetDevice(void)
{
  /* callback for USB-Reset */
    APP_LOG("[APP]: > USR_ResetDevice ...\r\n");
}


/**
* @brief  USBH_USR_DeviceSpeedDetected 
*         Displays the message on LCD for device speed
* @param  Device speed
* @retval None
*/
void USBH_USR_DeviceSpeedDetected(uint8_t DeviceSpeed)
{
    if(DeviceSpeed == HPRT0_PRTSPD_HIGH_SPEED)
    {
        APP_LOG("[APP]: %s.\r\n",(uint8_t *)MSG_DEV_HIGHSPEED);
    }  
    else if(DeviceSpeed == HPRT0_PRTSPD_FULL_SPEED)
    {
        APP_LOG("[APP]: %s.\r\n",(uint8_t *)MSG_DEV_FULLSPEED);
    }
    else if(DeviceSpeed == HPRT0_PRTSPD_LOW_SPEED)
    {
        APP_LOG("[APP]: %s.\r\n",(uint8_t *)MSG_DEV_LOWSPEED);
    }
    else
    {
        APP_LOG("[APP]: %s.\r\n",(uint8_t *)MSG_DEV_ERROR);
    }
}

/**
* @brief  USBH_USR_Device_DescAvailable 
*         Displays the message on LCD for device descriptor
* @param  device descriptor
* @retval None
*/
void USBH_USR_Device_DescAvailable(void *DeviceDesc)
{ 
    USBH_DevDesc_TypeDef *hs = DeviceDesc;  

    APP_LOG("[APP]: VID : %04Xh\r\n",(uint32_t)(*hs).idVendor); 
    APP_LOG("[APP]: PID : %04Xh\r\n",(uint32_t)(*hs).idProduct);
}

/**
* @brief  USBH_USR_DeviceAddressAssigned 
*         USB device is successfully assigned the Address 
* @param  None
* @retval None
*/
void USBH_USR_DeviceAddressAssigned(void)
{
  
}


/**
* @brief  USBH_USR_Conf_Desc 
*         Displays the message on LCD for configuration descriptor
* @param  Configuration descriptor
* @retval None
*/
void USBH_USR_Configuration_DescAvailable(USBH_CfgDesc_TypeDef * cfgDesc,
                                          USBH_InterfaceDesc_TypeDef *itfDesc,
                                          USBH_EpDesc_TypeDef *epDesc)
{
    USBH_InterfaceDesc_TypeDef *id;
    
    /* Check parameters */
    usr_assert(cfgDesc != NULL);
    usr_assert(itfDesc != NULL);
    usr_assert(epDesc != NULL);
    
    id = itfDesc;  
    if((*id).bInterfaceClass  == USH_USR_MSC_CLASS)
    {
        APP_LOG("[APP]: %s\r\n",(uint8_t *)MSG_MSC_CLASS);
    }
    else if((*id).bInterfaceClass  == USH_USR_HID_CLASS)
    {
        APP_LOG("[APP]: %s\r\n",(uint8_t *)MSG_HID_CLASS);
    }
    else if((*id).bInterfaceClass  == USH_USR_VENDOR_DEFINE_CLASS)
    {
        APP_LOG("[APP]: > Vendor defined class...\r\n");
    }
    else
    {
        APP_LOG("[APP]: > Other class : %d...\r\n",(*id).bInterfaceClass);
    }
    APP_LOG("[APP]: > Number of Endpoints used : %d...\r\n",(*id).bNumEndpoints);
}

/**
* @brief  USBH_USR_Manufacturer_String 
*         Displays the message on LCD for Manufacturer String 
* @param  Manufacturer String 
* @retval None
*/
void USBH_USR_Manufacturer_String(void *ManufacturerString)
{
    /* Check parameters */
    usr_assert(ManufacturerString != NULL);
    
    if(ManufacturerString != NULL)
    {
        APP_LOG("[APP]: Manufacturer : %s\r\n", (char *)ManufacturerString);
    }
    else
    {
        APP_LOG("[APP]: No manufacturer string.\r\n");
    }
}

/**
* @brief  USBH_USR_Product_String 
*         Displays the message on LCD for Product String
* @param  Product String
* @retval None
*/
void USBH_USR_Product_String(void *ProductString)
{ 
    /* Check parameters */
    usr_assert(ProductString != NULL);
    
    if(ProductString != NULL)
    {
        APP_LOG("[APP]: Product : %s\r\n", (char *)ProductString);
    }
    else
    {
        APP_LOG("[APP]: No product string.\r\n");
    }
}

/**
* @brief  USBH_USR_SerialNum_String 
*         Displays the message on LCD for SerialNum_String 
* @param  SerialNum_String 
* @retval None
*/
void USBH_USR_SerialNum_String(void *SerialNumString)
{
    if(SerialNumString == NULL)
    {
        APP_LOG("[APP]: Serial Number : %s\r\n", (char *)SerialNumString);
    }
    else
    {
        APP_LOG("[APP]: No serial number string.\r\n");
    }    
} 



/**
* @brief  EnumerationDone 
*         User response request is displayed to ask application jump to class
* @param  None
* @retval None
*/
void USBH_USR_EnumerationDone(void)
{
    /* Enumeration complete */
    APP_LOG("[APP]: %s\r\n", (char *)MSG_DEV_ENUMERATED);  
    APP_LOG("[APP]: To see the root content of the disk : \r\n"); 
    APP_LOG("[APP]: Press Key...\r\n"); 
} 


/**
* @brief  USBH_USR_DeviceNotSupported
*         Device is not supported
* @param  None
* @retval None
*/
void USBH_USR_DeviceNotSupported(void)
{
    APP_LOG("[APP]: > Device not supported.\r\n"); 
}  


/**
* @brief  USBH_USR_UserInput
*         User Action for application state entry
* @param  None
* @retval USBH_USR_Status : User response for key button
*/
USBH_USR_Status USBH_USR_UserInput(void)
{
    USBH_USR_Status usbh_usr_status;

    usbh_usr_status = USBH_USR_NO_RESP;  

    /*Key B3 is in polling mode to detect user action */
    if(STM_EVAL_PBGetState(Button_KEY) == RESET) 
    {
        APP_LOG("[APP]: > UserInput ...\r\n");
        usbh_usr_status = USBH_USR_RESP_OK;
    } 
    return usbh_usr_status;
}  

/**
* @brief  USBH_USR_OverCurrentDetected
*         Over Current Detected on VBUS
* @param  None
* @retval Staus
*/
void USBH_USR_OverCurrentDetected (void)
{
    APP_LOG("[APP]: > Overcurrent detected ...\r\n"); 
}


/**
* @brief  USBH_USR_MSC_Application 
*         Demo application for mass storage
* @param  None
* @retval Staus
*/
int USBH_USR_MSC_Application(void)
{
  FRESULT res;
  uint8_t writeTextBuff[] = "STM32 Connectivity line Host Demo application using FAT_FS   ";
  uint16_t bytesWritten, bytesToWrite;
  
  switch(USBH_USR_ApplicationState)
  {
  case USH_USR_FS_INIT: 
    
    /* Initialises the File System*/
    if ( f_mount( 0, &fatfs ) != FR_OK ) 
    {
      /* efs initialisation fails*/
        APP_LOG("[APP]: > Cannot initialize File System.\r\n"); 
      return(-1);
    }

    APP_LOG("[APP]: > File System initialized.\r\n"); 
    APP_LOG("[APP]: > Disk capacity : %d Bytes\r\n",USBH_MSC_Param.MSCapacity * USBH_MSC_Param.MSPageLength); 
    
    if(USBH_MSC_Param.MSWriteProtect == DISK_WRITE_PROTECTED)
    {
        APP_LOG("[APP]: %s.\r\n",(uint8_t *)MSG_WR_PROTECT); 
    }
    
    USBH_USR_ApplicationState = USH_USR_FS_READLIST;
    break;
    
  case USH_USR_FS_READLIST:
    APP_LOG("[APP]: %s.\r\n",(uint8_t *)MSG_WR_PROTECT); 
  
    Explore_Disk("0:/", 1);
    line_idx = 0;   
    USBH_USR_ApplicationState = USH_USR_FS_WRITEFILE;
    break;
    
  case USH_USR_FS_WRITEFILE:
    APP_LOG("[APP]: Press Key to write file\r\n");
    USB_OTG_BSP_mDelay(100);
    
    /*Key B3 in polling*/
    while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
      (STM_EVAL_PBGetState (BUTTON_KEY) == SET))          
    {
      Toggle_Leds();
    }
    /* Writes a text file, STM32.TXT in the disk*/
    APP_LOG("[APP]: > Writing File to disk flash ...\r\n");
    if(USBH_MSC_Param.MSWriteProtect == DISK_WRITE_PROTECTED)
    {
        APP_LOG("[APP]: > Disk flash is write protected\r\n");
      USBH_USR_ApplicationState = USH_USR_FS_DRAW;
      break;
    }
    
    /* Register work area for logical drives */
    f_mount(0, &fatfs);
    
    if(f_open(&file, "0:STM32.TXT",FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
    { 
      /* Write buffer to file */
      bytesToWrite = sizeof(writeTextBuff); 
      res= f_write (&file, writeTextBuff, bytesToWrite, (void *)&bytesWritten);   
      
      if((bytesWritten == 0) || (res != FR_OK)) /*EOF or Error*/
      {
          APP_LOG("[APP]: > STM32.TXT CANNOT be writen\r\n");
      }
      else
      {
          APP_LOG("[APP]: > 'STM32.TXT' file created\r\n");
      }
      
      /*close file and filesystem*/
      f_close(&file);
      f_mount(0, NULL); 
    }
    else
    {
        APP_LOG("[APP]: > STM32.TXT created in the disk\r\n");
    }

    USBH_USR_ApplicationState = USH_USR_FS_DRAW; 
    APP_LOG("[APP]: To start Image slide show Press Key.\r\n");
    break;
    
  case USH_USR_FS_DRAW:
    
    /*Key B3 in polling*/
    while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
      (STM_EVAL_PBGetState (BUTTON_KEY) == SET))
    {
      Toggle_Leds();
    }
  
    while(HCD_IsDeviceConnected(&USB_OTG_Core))
    {
      if ( f_mount( 0, &fatfs ) != FR_OK ) 
      {
        /* fat_fs initialisation fails*/
        return(-1);
      }
      return Image_Browser("0:/");
    }
    break;
  default: break;
  }
  return(0);
}

/**
* @brief  Explore_Disk 
*         Displays disk content
* @param  path: pointer to root path
* @retval None
*/
static uint8_t Explore_Disk (char* path , uint8_t recu_level)
{

  FRESULT res;
  FILINFO fno;
  DIR dir;
  char *fn;
  char tmp[14];
  
  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    while(HCD_IsDeviceConnected(&USB_OTG_Core)) 
    {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) 
      {
        break;
      }
      if (fno.fname[0] == '.')
      {
        continue;
      }

      fn = fno.fname;
      strcpy(tmp, fn); 

      line_idx++;
      if(line_idx > 9)
      {
        line_idx = 0;
          APP_LOG("[APP]: Press Key to continue...\r\n");

        /*Key B3 in polling*/
        while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
          (STM_EVAL_PBGetState (BUTTON_KEY) == SET))
        {
          Toggle_Leds();
          
        }
      } 
      
      if(recu_level == 1)
      {
          APP_LOG("[APP]:    |__\r\n");
      }
      else if(recu_level == 2)
      {
          APP_LOG("[APP]:   |   |__\r\n");
      }
      if((fno.fattrib & AM_MASK) == AM_DIR)
      {
        strcat(tmp, "\n"); 
          APP_LOG("[APP]: %s\r\n",tmp);
      }
      else
      {
        strcat(tmp, "\n"); 
          APP_LOG("[APP]: %s\r\n",tmp);
      }

      if(((fno.fattrib & AM_MASK) == AM_DIR)&&(recu_level == 1))
      {
        Explore_Disk(fn, 2);
      }
    }
  }
  return res;
}

static uint8_t Image_Browser (char* path)
{
  FRESULT res;
  uint8_t ret = 1;
  FILINFO fno;
  DIR dir;
  char *fn;
  
  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    
    for (;;) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) break;
      if (fno.fname[0] == '.') continue;

      fn = fno.fname;
 
      if (fno.fattrib & AM_DIR) 
      {
        continue;
      } 
      else 
      {
        if((strstr(fn, "bmp")) || (strstr(fn, "BMP")))
        {
          res = f_open(&file, fn, FA_OPEN_EXISTING | FA_READ);
          Show_Image();
          USB_OTG_BSP_mDelay(100);
          ret = 0;
          while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
            (STM_EVAL_PBGetState (BUTTON_KEY) == SET))
          {
            Toggle_Leds();
          }
          f_close(&file);
          
        }
      }
    }  
  }
  
  APP_LOG("[APP]: USB OTG FS MSC Host\r\n");
  APP_LOG("[APP]: USB Host Library v2.1.0\r\n");
  APP_LOG("[APP]: > Disk capacity : %d Bytes\r\n",USBH_MSC_Param.MSCapacity * USBH_MSC_Param.MSPageLength);
  USBH_USR_ApplicationState = USH_USR_FS_READLIST;
  return ret;
}

/**
* @brief  Show_Image 
*         Displays BMP image
* @param  None
* @retval None
*/
static void Show_Image(void)
{
  
//  uint16_t i = 0;
//  uint16_t numOfReadBytes = 0;
//  FRESULT res; 
//  
//  LCD_SetDisplayWindow(239, 319, 240, 320);
//  LCD_WriteReg(R3, 0x1008);
//  LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
//  
//  /* Bypass Bitmap header */ 
//  f_lseek (&file, 54);
//  
//  while (HCD_IsDeviceConnected(&USB_OTG_Core))
//  {
//    res = f_read(&file, Image_Buf, IMAGE_BUFFER_SIZE, (void *)&numOfReadBytes);
//    if((numOfReadBytes == 0) || (res != FR_OK)) /*EOF or Error*/
//    {
//      break; 
//    }
//    for(i = 0 ; i < IMAGE_BUFFER_SIZE; i+= 2)
//    {
//      LCD_WriteRAM(Image_Buf[i+1] << 8 | Image_Buf[i]); 
//    } 
//  }
  
}

/**
* @brief  Toggle_Leds
*         Toggle leds to shows user input state
* @param  None
* @retval None
*/
static void Toggle_Leds(void)
{
  static uint32_t i;
  if (i++ == 0x10000)
  {
    STM_EVAL_LEDToggle(LED1);
    STM_EVAL_LEDToggle(LED2);
    STM_EVAL_LEDToggle(LED3);
    STM_EVAL_LEDToggle(LED4);
    i = 0;
  }  
}
/**
* @brief  USBH_USR_DeInit
*         Deint User state and associated variables
* @param  None
* @retval None
*/
void USBH_USR_DeInit(void)
{
  USBH_USR_ApplicationState = USH_USR_FS_INIT;
}


/**
* @}
*/ 
static USBH_Status USBH_GETATStatus(USB_OTG_CORE_HANDLE *pdev , USBH_HOST *phost)
{
    const uint8_t buffer[] = "AT";
    
    phost->Control.setup.b.bmRequestType = USB_D2H | USB_REQ_TYPE_CLASS | \
                                            USB_REQ_RECIPIENT_INTERFACE;
  
    phost->Control.setup.b.bRequest = USB_REQ_GET_MAX_LUN;
    phost->Control.setup.b.wValue.w = 0;
    phost->Control.setup.b.wIndex.w = 0;
    phost->Control.setup.b.wLength.w = strlen(buffer);  
    
    USBH_CtlSendData (pdev,
                      0,
                      0,
                      phost->Control.hc_num_out);
    return USBH_CtlReq(pdev, phost, MSC_Machine.buff , 1 ); 
}
/**
* @}
*/ 

/**
* @}
*/

/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

