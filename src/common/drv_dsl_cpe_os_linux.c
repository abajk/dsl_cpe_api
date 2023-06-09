/******************************************************************************

                          Copyright (c) 2007-2015
                     Lantiq Beteiligungs-GmbH & Co. KG

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifdef __LINUX__

#define DSL_INTERN

#include "drv_dsl_cpe_api.h"
#include "drv_dsl_cpe_api_ioctl.h"

#include "drv_dsl_cpe_intern.h"
#include "drv_dsl_cpe_intern_mib.h"

#include "drv_dsl_cpe_debug.h"

#include <linux/device.h>

#include "drv_mei_cpe_api_intern.h" /* MEI_InternalDevLayout */

#undef DSL_DBG_BLOCK
#define DSL_DBG_BLOCK DSL_DBG_OS

#ifdef __cplusplus
   extern "C" {
#endif

#define DSL_DRV_STATIC

#ifdef INCLUDE_DSL_CPE_DEBUG_LOGGER_SUPPORT
   #ifndef DSL_DBG_MSG_NETLINK_ID
      #define DSL_DBG_MSG_NETLINK_ID 28
   #endif
   #define NL_DBG_MSG_GROUP 1
extern struct sock *nl_debug_sock;
#endif

static const char* dsl_cpe_api_version = "@(#)DSL CPE API V" DSL_CPE_API_PACKAGE_VERSION;

static DSL_ssize_t DSL_DRV_Write(DSL_DRV_file_t *pFile, const DSL_char_t * pBuf,
                                 DSL_DRV_size_t nSize, DSL_DRV_offset_t * pLoff);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static DSL_int_t DSL_DRV_Ioctls(DSL_DRV_inode_t * pINode, DSL_DRV_file_t * pFile,
                         DSL_uint_t nCommand, unsigned long nArg);
#else
static DSL_long_t DSL_DRV_Ioctls(DSL_DRV_file_t * pFile,
                         DSL_uint_t nCommand, unsigned long nArg);
#endif

static int DSL_DRV_Open(DSL_DRV_inode_t * ino, DSL_DRV_file_t * fil);

static int DSL_DRV_Release(DSL_DRV_inode_t * ino, DSL_DRV_file_t * fil);

static DSL_uint_t DSL_DRV_Poll(DSL_DRV_file_t *pFile, DSL_DRV_Poll_Table_t *wait);

#ifdef INCLUDE_DSL_CPE_DEBUG_LOGGER_SUPPORT
static void DSL_DRV_NlSendMsg(DSL_char_t* pMsg);
#endif

/*DSL_int_t DSL_DRV_Ioctls(DSL_DRV_inode_t * pINode, DSL_DRV_file_t * pFile,
                           unsigned long nCommand, unsigned long nArg);*/

/* global parameter debug_level: LOW (1), NORMAL (2), HIGH (3), OFF (4) */

#ifdef DSL_DBG_MAX_LEVEL
   #if (DSL_DBG_MAX_LEVEL >= DSL_DBGLVL_MSG)
      DSL_DRV_STATIC DSL_uint8_t debug_level = 1;
   #elif (DSL_DBG_MAX_LEVEL >= DSL_DBGLVL_WRN)
      DSL_DRV_STATIC DSL_uint8_t debug_level = 2;
   #elif (DSL_DBG_MAX_LEVEL >= DSL_DBGLVL_ERR)
      DSL_DRV_STATIC DSL_uint8_t debug_level = 3;
   #else
      DSL_DRV_STATIC DSL_uint8_t debug_level = 4;
   #endif
#else
   /* Activate high level by default */
   DSL_DRV_STATIC DSL_uint8_t debug_level = 3;
#endif

DSL_uint8_t g_MaxDeviceNumber = -1;
DSL_uint8_t g_LinesPerDevice = -1;
DSL_uint8_t g_ChannelsPerLine = -1;

DSL_uint8_t g_MaxEntieties = 1;


/* the major number of this driver */
static int nMajorNum = DRV_DSL_CPE_API_DEV_MAJOR;
static struct class *dsl_class;

#ifndef _lint
static struct file_operations dslCpeApiOperations = {
   owner:   THIS_MODULE,
   open:    DSL_DRV_Open,
   release: DSL_DRV_Release,
   write:   DSL_DRV_Write,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
   ioctl   : DSL_DRV_Ioctls,
#else
   unlocked_ioctl   : DSL_DRV_Ioctls,
#endif
   poll:    DSL_DRV_Poll
};

#else
static struct file_operations dslCpeApiOperations = {
   /*open:*/    DSL_DRV_Open,
   /*release:*/ DSL_DRV_Release,
   /*read*/     DSL_NULL,
   /*write:*/   DSL_DRV_Write,
   /*llseek*/   DSL_NULL,
   /*ioctl:*/   DSL_DRV_Ioctls,
   /*poll:*/    DSL_DRV_Poll
};
#endif /* #ifndef _lint*/

static int DSL_DRV_Open(DSL_DRV_inode_t * ino, DSL_DRV_file_t * fil)
{
   int num = MINOR(ino->i_rdev);
   DSL_OpenContext_t *pOpenCtx;

   DSL_DEBUG(DSL_DBG_MSG, (DSL_NULL, SYS_DBG_MSG"Device will be opened..."DSL_DRV_CRLF));

   if ( DSL_DRV_HandleInit(num, &pOpenCtx) != DSL_SUCCESS )
   {
      return -EIO;
   }

   fil->private_data = pOpenCtx;

   DSL_DEBUG(DSL_DBG_MSG, (DSL_NULL, SYS_DBG_MSG"Open successfull..."DSL_DRV_CRLF));

   return 0;
}

static int DSL_DRV_Release(DSL_DRV_inode_t * ino, DSL_DRV_file_t * fil)
{
   int num = MINOR(ino->i_rdev);
   DSL_DEBUG(DSL_DBG_MSG,
      (DSL_NULL, SYS_DBG_MSG"Device will be closed..."DSL_DRV_CRLF));

   if (num >= DSL_DRV_ENTITIES)
   {
      return -EIO;
   }

   DSL_DRV_HandleDelete((DSL_OpenContext_t*)fil->private_data);

   DSL_DEBUG(DSL_DBG_MSG,
      (DSL_NULL, SYS_DBG_MSG"Close successfull..."DSL_DRV_CRLF));

   return 0;
}

static DSL_ssize_t DSL_DRV_Write(DSL_DRV_file_t *pFile, const DSL_char_t * pBuf,
                                 DSL_DRV_size_t nSize, DSL_DRV_offset_t * pLoff)
{
   DSL_Error_t nErrCode = DSL_ERROR;
   DSL_uint32_t nOffset=0;

   if (pFile->private_data == DSL_NULL)
   {
      DSL_DEBUG(DSL_DBG_ERR,
         (DSL_NULL, SYS_DBG_ERR"Private data of DSL_DRV_file_t "
                               "structure is NULL"DSL_DRV_CRLF));
      return -EIO;
   }

   if (((DSL_OpenContext_t*)pFile->private_data)->pDevCtx == DSL_NULL)
   {
      DSL_DEBUG(DSL_DBG_ERR,
         (DSL_NULL, SYS_DBG_ERR"Device context pointer is NULL"DSL_DRV_CRLF));
      return -EIO;
   }

   ((DSL_devCtx_t*)(((DSL_OpenContext_t*)pFile->private_data)
      ->pDevCtx))->bFirmwareReady = DSL_FALSE;

   nErrCode = DSL_DRV_DEV_FwDownload(
      ((DSL_devCtx_t*)(((DSL_OpenContext_t*)pFile->private_data)->pDevCtx))->pContext,
      pBuf, (DSL_uint32_t)nSize, DSL_NULL, 0, (DSL_int32_t *)pLoff,
      (DSL_int32_t*)&nOffset, DSL_TRUE, DSL_TRUE);

   if (nErrCode != DSL_SUCCESS)
      return -EIO;

   ((DSL_devCtx_t*)(((DSL_OpenContext_t*)pFile->private_data)->pDevCtx))
      ->bFirmwareReady = DSL_TRUE;
   DSL_DEBUG(DSL_DBG_MSG, (DSL_NULL, SYS_DBG_MSG"Firmware was downloaded successfully "
                                    "%d bytes has written..."DSL_DRV_CRLF, nOffset));
   return (DSL_ssize_t)nOffset;
}

/*
   IO controls for user space accessing

   \param   ino      Pointer to the stucture of inode.
   \param   fil      Pointer to the stucture of file.
   \param   command     The ioctl command.
   \param   lon      The address of data.
   \return  Success or failure.
   \ingroup Internal
*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static DSL_int_t DSL_DRV_Ioctls(DSL_DRV_inode_t * pINode, DSL_DRV_file_t * pFile,
                         DSL_uint_t nCommand, unsigned long nArg)
#else
static DSL_long_t DSL_DRV_Ioctls(DSL_DRV_file_t * pFile,
                         DSL_uint_t nCommand, unsigned long nArg)
#endif
{
   DSL_int_t nErr=0;
   DSL_boolean_t bIsInKernel;
   DSL_Error_t nRetCode = DSL_SUCCESS;
   DSL_Context_t *pContext;
   DSL_devCtx_t *pDevCtx;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
   DSL_DRV_inode_t * pINode;
#endif

   DSL_OpenContext_t *pOpenCtx;

   DSL_DEBUG(DSL_DBG_MSG,
      (DSL_NULL, SYS_DBG_MSG"DSL: IN - DSL_DRV_Ioctls: The ioctl "
      "command(0x%X) is called" DSL_DRV_CRLF, nCommand));

   if ((pOpenCtx = (DSL_OpenContext_t *)pFile->private_data) == DSL_NULL)
   {
      /* This should never happen */
      DSL_DEBUG(DSL_DBG_ERR,
         (DSL_NULL, SYS_DBG_ERR"DSL: Ioctl call for file which was not opened"
         DSL_DRV_CRLF));

      return -EFAULT;
   }
   else
   {
      if ((pDevCtx = pOpenCtx->pDevCtx) == DSL_NULL)
      {
         /* This should never happen */
         DSL_DEBUG(DSL_DBG_ERR,
            (DSL_NULL, SYS_DBG_ERR"DSL: DSL_DRV_Poll: !!! Ioctl call "
            "for file which was not opened correctly" DSL_DRV_CRLF));

         return -EFAULT;
      }
      else
      {
         if ((pContext = pDevCtx->pContext) == DSL_NULL)
         {
            /* This should never happen */
            DSL_DEBUG(DSL_DBG_ERR, (DSL_NULL, SYS_DBG_ERR"DSL: Ioctl call to device "
               "which was not ready" DSL_DRV_CRLF));

            return -EFAULT;
         }
      }
   }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0))
   pINode = file_inode(pFile);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
   if (pFile->f_dentry != DSL_NULL)
   {
      pINode = pFile->f_dentry->d_inode;
   }
   else
   {
      pINode = DSL_NULL;
   }
#endif

   if (pINode == DSL_NULL)
   {
      bIsInKernel = DSL_TRUE;
   }
   else
   {
      bIsInKernel = DSL_FALSE;
   }

   if ( (_IOC_TYPE(nCommand) == DSL_IOC_MAGIC_CPE_API) ||
        (_IOC_TYPE(nCommand) == DSL_IOC_MAGIC_CPE_API_G997) ||
        (_IOC_TYPE(nCommand) == DSL_IOC_MAGIC_CPE_API_PM) ||
        (_IOC_TYPE(nCommand) == DSL_IOC_MAGIC_CPE_API_BND) ||
        (_IOC_TYPE(nCommand) == DSL_IOC_MAGIC_CPE_API_DEP) ||
        (_IOC_TYPE(nCommand) == DSL_IOC_MAGIC_CPE_API_RTT) )
   {
      nRetCode = DSL_DRV_IoctlHandle(pOpenCtx, pContext, bIsInKernel, nCommand, nArg);

      if (nRetCode < DSL_SUCCESS)
      {
         nErr = DSL_DRV_ErrorToOS(nRetCode);
      }
   }
#if defined(INCLUDE_DSL_ADSL_MIB)
   else if (_IOC_TYPE(nCommand) == DSL_IOC_MAGIC_MIB)
   {
      nRetCode = DSL_DRV_MIB_IoctlHandle(pContext, bIsInKernel, nCommand, nArg);
      nErr = nRetCode;
   }
#endif
   else
   {
      DSL_DEBUG(DSL_DBG_ERR, (DSL_NULL, SYS_DBG_ERR"DSL: The ioctl command(0x%X) is not "
         "supported!" DSL_DRV_CRLF, nCommand));

      nErr = -ENOIOCTLCMD;

      return nErr;
   }

   DSL_DEBUG(DSL_DBG_MSG,
      (DSL_NULL, SYS_DBG_MSG"DSL: OUT - DSL_DRV_Ioctls(), retCode=%d"
      DSL_DRV_CRLF, nErr));

   return nErr;
}

static DSL_uint_t DSL_DRV_Poll(DSL_DRV_file_t *pFile, DSL_DRV_Poll_Table_t *wait)
{
   DSL_int_t nRet = 0;
   DSL_OpenContext_t *pOpenCtx;

   DSL_DEBUG(DSL_DBG_MSG, (DSL_NULL, SYS_DBG_MSG"IN - DSL_DRV_Poll" DSL_DRV_CRLF));

   if ((pOpenCtx = (DSL_OpenContext_t *)pFile->private_data) == DSL_NULL)
   {
      /* This should never happen */
      DSL_DEBUG(DSL_DBG_ERR, (DSL_NULL, SYS_DBG_ERR"!!! Ioctl call for file which "
         "was not opened" DSL_DRV_CRLF));

      return (DSL_uint_t)(-EFAULT);
   }
   poll_wait(pFile, &pOpenCtx->eventWaitQueue, wait);

   if(DSL_DRV_MUTEX_LOCK(pOpenCtx->eventMutex))
   {
      DSL_DEBUG( DSL_DBG_ERR, (DSL_NULL, SYS_DBG_ERR"Couldn't lock event mutex"DSL_DRV_CRLF));
      return (DSL_uint_t)DSL_ERROR;
   }

   if (pOpenCtx->eventFifo == DSL_NULL
      || pOpenCtx->eventFifoBuf == DSL_NULL
      || pOpenCtx->bEventActivation == DSL_FALSE)
   {
      DSL_DEBUG(DSL_DBG_WRN,
         (DSL_NULL, SYS_DBG_WRN"!!! Ioctl call for file which was not configured"
                    " for event handling!!!" DSL_DRV_CRLF));
   }
   else
   {
      if ( DSL_Fifo_isEmpty( pOpenCtx->eventFifo ) == 0 )
      {
         nRet |= POLLIN | POLLRDNORM; /* an event available */
      }
   }

   DSL_DRV_MUTEX_UNLOCK(pOpenCtx->eventMutex);

   DSL_DEBUG(DSL_DBG_MSG, (DSL_NULL, SYS_DBG_MSG"OUT - DSL_DRV_Poll" DSL_DRV_CRLF));

   return (DSL_uint_t)nRet;
}

static int DSL_DRV_DevNodeInit(DSL_void_t)
{
   DSL_int_t i;
   static dev_t dsl_devt;

   if (register_chrdev(nMajorNum, DRV_DSL_CPE_API_DEV_NAME,
                        &dslCpeApiOperations) != 0)
   {
      DSL_DEBUG(DSL_DBG_ERR,
         (DSL_NULL, SYS_DBG_ERR""DSL_DRV_CRLF DSL_DRV_CRLF"unable to register "
         "major for %s!!!", DRV_DSL_CPE_API_DEV_NAME));
      return -ENODEV;
   }

   /* create a device class used for createing /dev/ entries */
   dsl_class = class_create(THIS_MODULE, DRV_DSL_CPE_API_DEV_NAME);
   if (IS_ERR(dsl_class)) {
      DSL_DEBUG(DSL_DBG_ERR,
         (DSL_NULL, SYS_DBG_ERR""DSL_DRV_CRLF DSL_DRV_CRLF
          "can not create class for %s", DRV_DSL_CPE_API_DEV_NAME));
      return PTR_ERR(dsl_class);
   }

   /* create /dev/ entry for each device */
   for (i=0; i < DSL_DRV_ENTITIES; i++)
   {
      dsl_devt = MKDEV(nMajorNum, i);
      device_create(dsl_class, NULL, dsl_devt, NULL, "%s/%i", "dsl_cpe_api", i);
   }

   return 0;
}

/*
   For a detailed description of the function, its arguments and return value
   please refer to the description in the header file 'drv_dsl_cpe_os.h'
*/
int DSL_DRV_debug_printf(DSL_Context_t const *pContext, DSL_char_t const *fmt, ...)
{
   DSL_int_t nRet = 0;
#ifndef _lint
   DSL_int_t nLength = 0;
   DSL_boolean_t bPrint = DSL_FALSE;
#ifdef INCLUDE_DSL_CPE_DEBUG_LOGGER_SUPPORT
#ifndef DSL_DEBUG_DISABLE
   DSL_char_t debugString[DSL_DBG_MAX_DEBUG_PRINT_CHAR + 1] = {0};
   va_list ap;   /* points to each unnamed arg in turn */

   if (bPrint == DSL_FALSE)
   {
      /* add the debug string itself */
      va_start(ap, fmt);   /* set ap pointer to 1st unnamed arg */
      nRet = vsnprintf(&debugString[0], DSL_DBG_MAX_DEBUG_PRINT_CHAR, fmt, ap);
      va_end(ap);

      if (nRet < 0)
      {
         nLength = DSL_DBG_MAX_DEBUG_PRINT_CHAR;
         printk(KERN_ERR "DSL CPE API: WARNING - printout truncated in "
                        "'DSL_DRV_debug_printf'!" DSL_DRV_CRLF );
      }
      else
      {
         nLength = nRet;
      }

      /* send the formed string to the logger */
      if (DSL_g_dbgDestination == DSL_DBG_DST_CONSOLE)
      {
         nRet = printk("%s",debugString);      
      }
      else
      {
         DSL_DRV_NlSendMsg(debugString);
      }
   }
#endif /* DSL_DEBUG_DISABLE */
   return nRet;
#else
   DSL_char_t msg[DSL_DBG_MAX_DEBUG_PRINT_CHAR + 1] = "\0";
   va_list ap;   /* points to each unnamed arg in turn */

   va_start(ap, fmt);   /* set ap pointer to 1st unnamed arg */

   if (bPrint == DSL_FALSE)
   {
      nRet = vsnprintf(msg, DSL_DBG_MAX_DEBUG_PRINT_CHAR, fmt, ap);

      if (nRet < 0)
      {
         nLength = DSL_DBG_MAX_DEBUG_PRINT_CHAR;
         printk(KERN_ERR "DSL CPE API: WARNING - printout truncated in "
                        "'DSL_DRV_debug_printf'!" DSL_DRV_CRLF );
      }
      else
      {
         nLength = nRet;
      }

      nRet = printk("%s",msg);
   }

   va_end(ap);
#endif
#endif /* _lint */

   return nRet;
}

/*
   For a detailed description of the function, its arguments and return value
   please refer to the description in the header file 'drv_dsl_cpe_os.h'
*/
int DSL_DRV_ErrorToOS(DSL_Error_t nError)
{
   switch (nError)
   {
      case DSL_ERR_POINTER:
      case DSL_ERR_INVALID_PARAMETER:
         return -EINVAL;

      case DSL_ERR_NOT_IMPLEMENTED:
      case DSL_ERR_NOT_SUPPORTED:
      case DSL_ERR_NOT_SUPPORTED_BY_DEVICE:
         return -ENOTSUPP;

      case DSL_ERR_NOT_SUPPORTED_BY_FIRMWARE:
         return -ENOIOCTLCMD;

      case DSL_ERR_MEMORY:
         return -ENOMEM;

      case DSL_ERR_FILE_CLOSE:
      case DSL_ERR_FILE_OPEN:
      case DSL_ERR_FILE_READ:
      case DSL_ERR_FILE_WRITE:
         return -EPERM;

      case DSL_WRN_LAST:
      case DSL_SUCCESS:
         return 0;

      case DSL_ERR_INTERNAL:
      case DSL_ERR_TIMEOUT:
      case DSL_ERROR:
      default:
         return -EFAULT;
   }
}

/*
   For a detailed description of the function, its arguments and return value
   please refer to the description in the header file 'drv_dsl_cpe_os.h'
*/
DSL_void_t* DSL_DRV_VMalloc(
   DSL_DRV_size_t    nSize)
{
   /* VRX500-BU: Better to use vmalloc or vzmalloc here?! */
   return __vmalloc((unsigned long)nSize, GFP_KERNEL, PAGE_KERNEL);
   /*   return vmalloc(nSize);*/
}

/*
   For a detailed description of the function, its arguments and return value
   please refer to the description in the header file 'drv_dsl_cpe_os.h'
*/
DSL_void_t DSL_DRV_VFree(
   DSL_void_t*    pPtr)
{
   vfree(pPtr);
}

/*
   For a detailed description of the function, its arguments and return value
   please refer to the description in the header file 'drv_dsl_cpe_os.h'
*/
DSL_int_t DSL_DRV_snprintf(
   DSL_char_t        *pStr,
   DSL_DRV_size_t        nStrSz,
   const DSL_char_t  *pFormat,  ...)
{
   int rv = 0;
#ifndef _lint
   va_list arg;

   va_start(arg, pFormat);
   rv = vsnprintf(pStr, nStrSz, pFormat, arg);
   va_end(arg);
#endif /* #ifndef _lint*/

   return rv;
}

/*
   For a detailed description of the function, its arguments and return value
   please refer to the description in the header file 'drv_dsl_cpe_os.h'
*/
DSL_void_t* DSL_IoctlMemCpyFrom(
   DSL_boolean_t bIsInKernel,
   DSL_void_t    *pDest,
   DSL_void_t    *pSrc,
   DSL_DRV_size_t    nSize)
{
   DSL_void_t *pRet;

   if (bIsInKernel == DSL_TRUE)
   {
      pRet = memcpy(pDest, pSrc, nSize);
   }
   else
   {
      pRet = (DSL_void_t*)copy_from_user(pDest, pSrc, nSize);
   }

   return pRet;
}

/*
   For a detailed description of the function, its arguments and return value
   please refer to the description in the header file 'drv_dsl_cpe_os.h'
*/
DSL_void_t* DSL_IoctlMemCpyTo(
   DSL_boolean_t bIsInKernel,
   DSL_void_t    *pDest,
   DSL_void_t    *pSrc,
   DSL_DRV_size_t    nSize)
{
   DSL_void_t *pRet;

   if (bIsInKernel == DSL_TRUE)
   {
      pRet = memcpy(pDest, pSrc, nSize);
   }
   else
   {
      pRet = (DSL_void_t*)copy_to_user(pDest, pSrc, nSize);
   }

   return pRet;
}

#ifndef INCLUDE_DSL_CPE_API_IFXOS_SUPPORT
/**
   LINUX Kernel - Thread stub function. The stub function will be called
   before calling the user defined thread routine. This gives
   us the possibility to add checks etc.

\par Implementation
   Before the stub function enters the user thread routin the following setup will
   be done:
   - make the kernel thread to a daemon
   - asign the parent to the init process (avoid termination if the parent thread dies).
   - setup thread name, and signal handling (if required).
   After this the user thread routine will be entered.

\param
   pThrCntrl   Thread information data

\return
   - IFX_SUCCESS on success
   - IFX_ERROR on error
*/
DSL_DRV_STATIC DSL_int32_t DSL_DRV_KernelThreadStartup(
                              DSL_DRV_ThreadCtrl_t *pThrCntrl)
{
   DSL_int32_t retVal          = -1;
#ifndef _lint

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
   struct task_struct *kthread = current;
#endif

   if(!pThrCntrl)
   {
      DSL_DEBUG( DSL_DBG_ERR,
         (DSL_NULL, SYS_DBG_ERR"IFXOS ERROR - Kernel ThreadStartup, missing object"
         DSL_DRV_CRLF));

      return retVal;
   }

   /* terminate the name if necessary */
   pThrCntrl->thrParams.pName[16 -1] = 0;

   DSL_DEBUG( DSL_DBG_MSG,
      (DSL_NULL, SYS_DBG_MSG"ENTER - Kernel Thread Startup <%s>"
      DSL_DRV_CRLF, pThrCntrl->thrParams.pName));

   /* do LINUX specific setup */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
   daemonize();
   reparent_to_init();

   /* lock the kernel. A new kernel thread starts without
      the big kernel lock, regardless of the lock state
      of the creator (the lock level is *not* inheritated)
   */
   lock_kernel();

   /* Don't care about any signals. */
   siginitsetinv(&current->blocked, 0);

   /* set name of this process */
   strcpy(kthread->comm, pThrCntrl->thrParams.pName);

   /* let others run */
   unlock_kernel();
#else
   daemonize(pThrCntrl->thrParams.pName);

#endif

   /*DSL_DRV_ThreadPriorityModify(pThrCntrl->nPriority);*/

   pThrCntrl->thrParams.bRunning = 1;
   retVal = pThrCntrl->pThrFct(&pThrCntrl->thrParams);
   pThrCntrl->thrParams.bRunning = 0;

   complete_and_exit(&pThrCntrl->thrCompletion, (long)retVal);

   DSL_DEBUG( DSL_DBG_MSG,
      (DSL_NULL, SYS_DBG_MSG"EXIT - Kernel Thread Startup <%s>" DSL_DRV_CRLF,
        pThrCntrl->thrParams.pName));

#endif /* #ifndef _lint*/

   return retVal;
}

/**
   LINUX Kernel - Creates a new thread / task.

\par Implementation
   - Allocate and setup the internal thread control structure.
   - setup the LINUX specific thread parameter (see "init_completion").
   - start the LINUX Kernel thread with the internal stub function (see "kernel_thread")

\param
   pThrCntrl         Pointer to thread control structure. This structure has to
                     be allocated outside and will be initialized.
\param
   pName             specifies the 8-char thread / task name.
\param
   pThreadFunction   specifies the user entry function of the thread / task.
\param
   nStackSize        specifies the size of the thread stack - not used.
\param
   nPriority         specifies the thread priority, 0 will be ignored
\param
   nArg1             first argument passed to thread / task entry function.
\param
   nArg2             second argument passed to thread / task entry function.

\return
   - IFX_SUCCESS thread was successful started.
   - IFX_ERROR thread was not started
*/
DSL_int32_t DSL_DRV_ThreadInit(
               DSL_DRV_ThreadCtrl_t *pThrCntrl,
               DSL_char_t     *pName,
               DSL_DRV_ThreadFunction_t pThreadFunction,
               DSL_uint32_t   nStackSize,
               DSL_uint32_t   nPriority,
               DSL_uint32_t   nArg1,
               DSL_uint32_t   nArg2)
{
   if(pThrCntrl)
   {
      if (DSL_DRV_THREAD_INIT_VALID(pThrCntrl) == DSL_FALSE)
      {
         /* set thread function arguments */
         strcpy(pThrCntrl->thrParams.pName, pName);
         pThrCntrl->nPriority = nPriority;
         pThrCntrl->thrParams.nArg1 = nArg1;
         pThrCntrl->thrParams.nArg2 = nArg2;

         /* set thread control settings */
         pThrCntrl->pThrFct = pThreadFunction;
         init_completion(&pThrCntrl->thrCompletion);

         /* start kernel thread via the wrapper function */
         pThrCntrl->pid = kernel_thread( (DSL_DRV_KERNEL_THREAD_StartRoutine)DSL_DRV_KernelThreadStartup,
                        (void *)pThrCntrl,
                        DSL_DRV_DRV_THREAD_OPTIONS);

         pThrCntrl->bValid = DSL_TRUE;

         return 0;
      }
      else
      {
         DSL_DEBUG( DSL_DBG_ERR,
            (DSL_NULL, SYS_DBG_ERR"IFXOS ERROR - Kernel ThreadInit, object already valid"
            DSL_DRV_CRLF));
      }
   }
   else
   {
      DSL_DEBUG( DSL_DBG_ERR,
         (DSL_NULL, SYS_DBG_ERR"IFXOS ERROR - Kernel ThreadInit, missing object"
         DSL_DRV_CRLF));
   }

   return -1;
}

#ifndef _lint
/**
   LINUX Kernel - Shutdown and terminate a given thread.
   Therefore the thread delete functions triggers the user thread function
   to shutdown. In case of not responce (timeout) the thread will be canceled.

\par Implementation
   - force a shutdown via the shutdown flag and wait.
   - wait for completion (see "wait_for_completion").
   - free previous allocated internal data.

\param
   pThrCntrl   Thread control struct.
\param
   waitTime_ms - Time [ms] to wait for "self-shutdown" of the user thread.

\return
   - DSL_SUCCESS thread was successful deleted - thread control struct is freed.
   - DSL_ERROR thread was not deleted
*/
DSL_int32_t DSL_DRV_ThreadDelete(
               DSL_DRV_ThreadCtrl_t *pThrCntrl,
               DSL_uint32_t       waitTime_ms)
{
   DSL_uint32_t   waitCnt = 1;

   if(pThrCntrl)
   {
      if (DSL_DRV_THREAD_INIT_VALID(pThrCntrl) == DSL_TRUE)
      {
         if (pThrCntrl->thrParams.bRunning == DSL_TRUE)
         {
            /* trigger user thread routine to shutdown */
            pThrCntrl->thrParams.bShutDown = DSL_TRUE;

            if (waitTime_ms != DSL_DRV_THREAD_DELETE_WAIT_FOREVER)
            {
               waitCnt = waitTime_ms / DSL_DRV_THREAD_DOWN_WAIT_POLL_MS;
            }

            while (waitCnt && (pThrCntrl->thrParams.bRunning == DSL_TRUE) )
            {
               DSL_DRV_MSecSleep(DSL_DRV_THREAD_DOWN_WAIT_POLL_MS);

               if (waitTime_ms != DSL_DRV_THREAD_DELETE_WAIT_FOREVER)
                  waitCnt--;
            }

            /* wait for thread end */
            wait_for_completion (&pThrCntrl->thrCompletion);
         }
         else
         {
            DSL_DEBUG( DSL_DBG_WRN,
               (DSL_NULL, SYS_DBG_WRN"IFXOS WRN - Kernel Thread Delete <%s> - not running"
               DSL_DRV_CRLF, pThrCntrl->thrParams.pName));
         }

         pThrCntrl->bValid = DSL_FALSE;

         if (pThrCntrl->thrParams.bRunning != DSL_FALSE)
         {
            DSL_DEBUG( DSL_DBG_ERR,
               (DSL_NULL, SYS_DBG_ERR"ERROR - Kernel Thread Delete <%s> not stopped"
               DSL_DRV_CRLF, pThrCntrl->thrParams.pName));

            return -1;
         }

         return 0;
      }
      else
      {
         DSL_DEBUG( DSL_DBG_ERR,
            (DSL_NULL, SYS_DBG_ERR"IFXOS ERROR - Kernel ThreadDelete, invalid object"
            DSL_DRV_CRLF));
      }
   }
   else
   {
      DSL_DEBUG( DSL_DBG_ERR,
         (DSL_NULL, SYS_DBG_ERR"IFXOS ERROR - Kernel ThreadDelete, missing object"
         DSL_DRV_CRLF));
   }

   return -1;
}
#endif /* _lint*/

/**
   LINUX Kernel - Shutdown a given thread.
   Therefore the thread delete functions triggers the user thread function
   to shutdown.

\par Implementation
   - force a shutdown via the shutdown flag.
   - wait for completion only if the thread down (see "wait_for_completion").
   - free previous allocated internal data.

\param
   pThrCntrl   Thread control struct.
\param
   waitTime_ms - Time [ms] to wait for "self-shutdown" of the user thread.

\return
   - DSL_SUCCESS thread was successful deleted - thread control struct is freed.
   - DSL_ERROR thread was not deleted
*/
DSL_int32_t DSL_DRV_ThreadShutdown(
               DSL_DRV_ThreadCtrl_t *pThrCntrl,
               DSL_uint32_t       waitTime_ms)
{
   DSL_uint32_t   waitCnt = 1;

   if(pThrCntrl)
   {
      if (DSL_DRV_THREAD_INIT_VALID(pThrCntrl) == DSL_TRUE)
      {
         if (pThrCntrl->thrParams.bRunning == DSL_TRUE)
         {
            /* trigger user thread routine to shutdown */
            pThrCntrl->thrParams.bShutDown = DSL_TRUE;

            if (waitTime_ms != DSL_DRV_THREAD_DELETE_WAIT_FOREVER)
            {
               waitCnt = waitTime_ms / DSL_DRV_THREAD_DOWN_WAIT_POLL_MS;
            }

            while (waitCnt && (pThrCntrl->thrParams.bRunning == DSL_TRUE) )
            {
               DSL_DRV_MSecSleep(DSL_DRV_THREAD_DOWN_WAIT_POLL_MS);

               if (waitTime_ms != DSL_DRV_THREAD_DELETE_WAIT_FOREVER)
                  waitCnt--;
            }

            if (pThrCntrl->thrParams.bRunning == DSL_FALSE)
            {
               wait_for_completion (&pThrCntrl->thrCompletion);
            }
         }
         else
         {
            DSL_DEBUG( DSL_DBG_WRN,
               (DSL_NULL, SYS_DBG_WRN"IFXOS WRN - Kernel Thread Shutdown <%s> - not running"
               DSL_DRV_CRLF, pThrCntrl->thrParams.pName));
         }

         pThrCntrl->bValid = DSL_FALSE;

         if (pThrCntrl->thrParams.bRunning == DSL_TRUE)
         {
            DSL_DEBUG( DSL_DBG_ERR,
               (DSL_NULL, SYS_DBG_ERR"ERROR - Kernel Thread Shutdown <%s> not stopped"
               DSL_DRV_CRLF, pThrCntrl->thrParams.pName));
         }
         return 0;
      }
      else
      {
         DSL_DEBUG( DSL_DBG_ERR,
            (DSL_NULL, SYS_DBG_ERR"IFXOS ERROR - Kernel Thread Shutdown, invalid object"
            DSL_DRV_CRLF));
      }
   }
   else
   {
      DSL_DEBUG( DSL_DBG_ERR,
         (DSL_NULL, SYS_DBG_ERR"IFXOS ERROR - Kernel Thread Shutdown, missing object"
         DSL_DRV_CRLF));
   }

   return -1;
}
#endif /* #ifndef INCLUDE_DSL_CPE_API_IFXOS_SUPPORT*/

DSL_uint32_t DSL_DRV_SysTimeGet(DSL_uint32_t nOffset)
{
   struct timeval tv;
   DSL_uint32_t nTime = 0;

   memset(&tv, 0, sizeof(tv));
   do_gettimeofday(&tv);
   nTime = (DSL_uint32_t)tv.tv_sec;

   if ( (nOffset == 0) || (nOffset > nTime) )
   {
      return nTime;
   }

   return (nTime - nOffset);
}

#ifndef INCLUDE_DSL_CPE_API_IFXOS_SUPPORT
#define DSL_DRV_RESOLUTION_MS    (1000LL / HZ)
#if (DSL_DRV_RESOLUTION_MS > 1)
#define DSL_DRV_JIFFIES_SCALE    (ULONG_MAX >> ((DSL_DRV_RESOLUTION_MS + 1)/2))
#else
#define DSL_DRV_JIFFIES_SCALE    (ULONG_MAX)
#endif

#define DSL_DRV_MSEC_SCALE      (DSL_DRV_JIFFIES_SCALE * DSL_DRV_RESOLUTION_MS)

DSL_uint32_t DSL_DRV_ElapsedTimeMSecGet(
               DSL_uint32_t refTime_ms)
{
   DSL_uint32_t currTime_ms;

   if (refTime_ms > DSL_DRV_JIFFIES_SCALE * DSL_DRV_RESOLUTION_MS)
      return 0;

   currTime_ms = (jiffies % (DSL_DRV_JIFFIES_SCALE + 1)) * DSL_DRV_RESOLUTION_MS;

   return (currTime_ms >= refTime_ms) ? (currTime_ms - refTime_ms) :
       (DSL_DRV_MSEC_SCALE - refTime_ms + currTime_ms + DSL_DRV_RESOLUTION_MS);
}

#if defined(INCLUDE_DSL_CPE_API_VRX)
/**
   LINUX Kernel - Map the physical address to a virtual memory space.
   For virtual memory management this is required.

\par Implementation
   - check if the given physical memory region is free (see "check_mem_region")
   - reserve the given physical memory region (see "request_mem_region")
   - map the given physical memory region - no cache (see "ioremap_nocache")

\attention
   This sequence will reserve the requested memory region, so no following user
   can remap the same area after this.
\attention
   Other users (driver) which have map the area before (without reservation)
   will still have access to the area.

\param
   physicalAddr         The physical address for mapping [I]
\param
   addrRangeSize_byte   Range of the address space to map [I]
\param
   pName                The name of the address space, for administration [I]
\param
   ppVirtAddr           Returns the pointer to the virtual mapped address [O]

\return
   0 if the mapping was successful and the ppVirtAddr is set, else
   -1   if something was wrong.

*/
DSL_int32_t DSL_DRV_Phy2VirtMap(
               DSL_uint32_t    physicalAddr,
               DSL_uint32_t    addrRangeSize_byte,
               DSL_char_t     *pName,
               DSL_uint8_t    **ppVirtAddr)
{
   DSL_uint8_t *pVirtAddr = IFX_NULL;

   if (ppVirtAddr == DSL_NULL)
      return -1;

   if (*ppVirtAddr != DSL_NULL)
      return -1;

   if (addrRangeSize_byte == 0)
      return -1;

   if ( check_mem_region(physicalAddr, addrRangeSize_byte) )
   {
      DSL_DEBUG(DSL_DBG_ERR,
         (DSL_NULL, SYS_DBG_ERR"ERROR Phy2Virt map, region check - addr 0x%08lX (size 0x%lX) not free"
         DSL_DRV_CRLF, physicalAddr, addrRangeSize_byte));

      return -1;
   }

   /* can't fail */
   request_mem_region(physicalAddr, addrRangeSize_byte, pName);

   /* remap memory (not cache able): physical --> virtual */
   pVirtAddr = (DSL_uint8_t *)ioremap_nocache( physicalAddr,
                                               addrRangeSize_byte );
   if (pVirtAddr == DSL_NULL)
   {
      DSL_DEBUG(DSL_DBG_ERR,
         (DSL_NULL, SYS_DBG_ERR"ERROR Phy2Virt map failed - addr 0x%08lX (size 0x%lX)"
         DSL_DRV_CRLF, physicalAddr, addrRangeSize_byte));

      release_mem_region(physicalAddr, addrRangeSize_byte);

      return -1;
   }

   *ppVirtAddr = pVirtAddr;

   return 0;
}

/**
   LINUX Kernel - Release the virtual memory range of a mapped physical address.
   For virtual memory management this is required.

\par Implementation
   - unmap the given physical memory region (see "iounmap")
   - release the given physical memory region (see "release_mem_region")

\param
   pPhysicalAddr        Points to the physical address for release mapping [IO]
                        (Cleared if success)
\param
   addrRangeSize_byte   Range of the address space to map [I]
\param
   ppVirtAddr           Provides the pointer to the virtual mapped address [IO]
                        (Cleared if success)

\return
   0 if the release was successful. The physicalAddr and the ppVirtAddr
               pointer is cleared, else
   -1   if something was wrong.
*/
DSL_int32_t DSL_DRV_Phy2VirtUnmap(
               DSL_uint32_t    *pPhysicalAddr,
               DSL_uint32_t    addrRangeSize_byte,
               DSL_uint8_t    **ppVirtAddr)
{
   /* unmap the virtual address */
   if ( (ppVirtAddr != DSL_NULL) && (*ppVirtAddr != DSL_NULL) )
   {
      iounmap((void *)(*ppVirtAddr));
      *ppVirtAddr = IFX_NULL;
   }

   /* release the memory region */
   if ( (pPhysicalAddr != DSL_NULL)  && (*pPhysicalAddr != 0) )
   {

      release_mem_region( (unsigned long)(*pPhysicalAddr), addrRangeSize_byte );
      *pPhysicalAddr = 0;
   }

   return 0;
}
#endif /* #if defined(INCLUDE_DSL_CPE_API_VRX) */
#endif /* #ifndef INCLUDE_DSL_CPE_API_IFXOS_SUPPORT*/

static void DSL_DRV_DebugInit(void)
{
   printk(DSL_DRV_CRLF "Predefined debug level: %d" DSL_DRV_CRLF,
      (DSL_int_t)debug_level);

#ifndef DSL_DEBUG_DISABLE
   switch (debug_level)
   {
      case 1:
         DSL_g_dbgLvl[DSL_DBG_CPE_API].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_G997].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_PM].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_MIB].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_CEOC].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_LED].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_SAR].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_DEVICE].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_AUTOBOOT_THREAD].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_OS].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_CALLBACK].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_MESSAGE_DUMP].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_LOW_LEVEL_DRIVER].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_MULTIMODE].nDbgLvl = DSL_DBG_MSG;
         DSL_g_dbgLvl[DSL_DBG_NOTIFICATIONS].nDbgLvl = DSL_DBG_MSG;
         break;
      case 2:
         DSL_g_dbgLvl[DSL_DBG_CPE_API].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_G997].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_PM].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_MIB].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_CEOC].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_LED].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_SAR].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_DEVICE].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_AUTOBOOT_THREAD].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_OS].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_CALLBACK].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_MESSAGE_DUMP].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_LOW_LEVEL_DRIVER].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_MULTIMODE].nDbgLvl = DSL_DBG_WRN;
         DSL_g_dbgLvl[DSL_DBG_NOTIFICATIONS].nDbgLvl = DSL_DBG_WRN;
         break;
      case 3:
         DSL_g_dbgLvl[DSL_DBG_CPE_API].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_G997].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_PM].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_MIB].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_CEOC].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_LED].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_SAR].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_DEVICE].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_AUTOBOOT_THREAD].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_OS].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_CALLBACK].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_MESSAGE_DUMP].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_LOW_LEVEL_DRIVER].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_MULTIMODE].nDbgLvl = DSL_DBG_ERR;
         DSL_g_dbgLvl[DSL_DBG_NOTIFICATIONS].nDbgLvl = DSL_DBG_ERR;
         break;
      case 4:
         DSL_g_dbgLvl[DSL_DBG_CPE_API].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_G997].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_PM].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_MIB].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_CEOC].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_LED].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_SAR].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_DEVICE].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_AUTOBOOT_THREAD].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_OS].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_CALLBACK].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_MESSAGE_DUMP].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_LOW_LEVEL_DRIVER].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_MULTIMODE].nDbgLvl = DSL_DBG_NONE;
         DSL_g_dbgLvl[DSL_DBG_NOTIFICATIONS].nDbgLvl = DSL_DBG_NONE;
         break;
      default:
         /* Nothing to do */
         break;
   }
#endif /* #ifndef DSL_DEBUG_DISABLE*/

   return;
}

#ifdef INCLUDE_DSL_CPE_DEBUG_LOGGER_SUPPORT
static void DSL_DRV_NlSendMsg(DSL_char_t* pMsg)
{
   struct nlmsghdr *pNlMsgHdr;
   struct sk_buff *pSkbOut;
   DSL_int_t nMsgSize = 0;

   if(pMsg == IFX_NULL)
   {
      printk("ERROR: Debug string is empty!\n");
      return;
   }

   nMsgSize = strlen(pMsg);

   pSkbOut = nlmsg_new(nMsgSize, GFP_KERNEL);

   if (!pSkbOut)
   {
      printk("Failed to allocate new skb\n");
      return;
   }
   pNlMsgHdr = nlmsg_put(pSkbOut, 0, 0, NLMSG_DONE, nMsgSize, 0);
   NETLINK_CB(pSkbOut).dst_group = NL_DBG_MSG_GROUP;
   strncpy(nlmsg_data(pNlMsgHdr), pMsg, nMsgSize);

	if (!nl_debug_sock) {
		kfree_skb(pSkbOut);
      printk("Failed to free skb\n");
      return;
	}
   
   nlmsg_multicast(nl_debug_sock, pSkbOut, 0, NL_DBG_MSG_GROUP, GFP_KERNEL);
}
#endif

/* Entry point of driver */
int __init DSL_ModuleInit(void)
{
   DSL_int_t i;

   printk(DSL_DRV_CRLF DSL_DRV_CRLF "Lantiq CPE API Driver version: %s" DSL_DRV_CRLF,
      &(dsl_cpe_api_version[4]));

   /** When autoload active */
   if (g_MaxDeviceNumber == (IFX_uint8_t)(-1)
      && g_LinesPerDevice == (IFX_uint8_t)(-1)
      && g_ChannelsPerLine == (IFX_uint8_t)(-1))
   {
      DSL_uint32_t mei_MaxDeviceNumber = -1;
      DSL_uint32_t mei_LinesPerDevice = -1;
      DSL_uint32_t mei_ChannelsPerLine = -1;

      printk(DSL_DRV_CRLF DSL_DRV_CRLF "Lantiq CPE API Driver - autoloading layout..." DSL_DRV_CRLF);

      MEI_InternalGetDevLayout(
                     &mei_MaxDeviceNumber,
                     &mei_LinesPerDevice,
                     &mei_ChannelsPerLine);

      g_MaxDeviceNumber = (DSL_uint8_t)mei_MaxDeviceNumber;
      g_LinesPerDevice = (DSL_uint8_t)mei_LinesPerDevice;
      g_ChannelsPerLine = (DSL_uint8_t)mei_ChannelsPerLine;

      debug_level = 3;
   }

   printk(DSL_DRV_CRLF DSL_DRV_CRLF "Lantiq CPE API Device layout: %d devices"
          ", %d lines, %d channels" DSL_DRV_CRLF,g_MaxDeviceNumber, g_LinesPerDevice, g_ChannelsPerLine);
   if (
      (g_MaxDeviceNumber <= 0)    || (g_MaxDeviceNumber > 2) ||
      (g_LinesPerDevice <= 0)     || (g_LinesPerDevice > 1) ||
      (g_ChannelsPerLine <= 0)    || (g_ChannelsPerLine > 1)
      )
   {
        printk(DSL_DRV_CRLF DSL_DRV_CRLF "Invalid VRX configuration:MaxDeviceNumber:%d, LinesPerDevice:%d ChannelsPerLine:%d" DSL_DRV_CRLF,
               g_MaxDeviceNumber, g_LinesPerDevice, g_ChannelsPerLine);
        return -1;
   }
#ifndef INCLUDE_FW_REQUEST_SUPPORT
   else if (g_MaxDeviceNumber * g_LinesPerDevice > 1)
   {
        printk(DSL_DRV_CRLF DSL_DRV_CRLF "FW Request support missing, needs to be enabled on bonding enabled boards" DSL_DRV_CRLF);
        return -1;
   }
#endif

   g_MaxEntieties = g_MaxDeviceNumber * g_LinesPerDevice;
   DSL_DRV_MemSet( ifxDevices, 0, sizeof(DSL_devCtx_t) * g_MaxEntieties );

   /* Apply initial debug levels. The lines below should be updated in case of
      new modules insert */
   DSL_DRV_DebugInit();

   /* Get handles for lower level driver */
   for (i = 0; i < g_MaxEntieties; i++)
   {
      ifxDevices[i].lowHandle = DSL_DRV_DEV_DriverHandleGet(0,i);
      if (ifxDevices[i].lowHandle == DSL_NULL)
      {
         printk("Get BSP Driver Handle Fail!"DSL_DRV_CRLF);
      }
#ifdef INCLUDE_DSL_NFC_HANDLE
      ifxDevices[i].nfc_lowHandle = DSL_DRV_DEV_DriverHandleGet(0,i);
      if (ifxDevices[i].lowHandle == DSL_NULL)
      {
         printk("Get BSP Driver NFC Handle Fail!"DSL_DRV_CRLF);
      }
#endif /* INCLUDE_DSL_NFC_HANDLE*/

      ifxDevices[i].nUsageCount = 0;
      ifxDevices[i].bFirstPowerOn = DSL_TRUE;
      DSL_DEBUG(DSL_DBG_MSG, (DSL_NULL, SYS_DBG_MSG"ifxDevices[%d].lowHandle=%p"
         DSL_DRV_CRLF, i, ifxDevices[i].lowHandle));
   }

   DSL_DRV_DevNodeInit();

   return 0;
}

void __exit DSL_ModuleCleanup(void)
{
   DSL_int_t i;
   static dev_t dsl_devt;

   printk("Module will be unloaded"DSL_DRV_CRLF);

   for (i=0; i < g_MaxEntieties; i++)
   {
      dsl_devt = MKDEV(nMajorNum, i);
      device_destroy(dsl_class, dsl_devt);
   }
   class_destroy(dsl_class);
   dsl_class = NULL;

   unregister_chrdev(nMajorNum, DRV_DSL_CPE_API_DEV_NAME);
   
   DSL_DRV_Cleanup();

   return;
}

#ifndef _lint
MODULE_LICENSE("Dual BSD/GPL");

/* install parameter debug_level: LOW (1), NORMAL (2), HIGH (3), OFF (4) */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
MODULE_PARM(debug_level, "b");
MODULE_PARM(g_MaxDeviceNumber, "b");
MODULE_PARM(g_LinesPerDevice, "b");
MODULE_PARM(g_ChannelsPerLine, "b");
#else
module_param(debug_level, byte, 0);
module_param(g_MaxDeviceNumber, byte, 0);
module_param(g_LinesPerDevice, byte, 0);
module_param(g_ChannelsPerLine, byte, 0);

#endif
MODULE_PARM_DESC(debug_level, "set to get more (1) or fewer (4) debug outputs");

module_init(DSL_ModuleInit);
module_exit(DSL_ModuleCleanup);
#endif /* #ifndef _lint*/

//EXPORT_SYMBOL(DSL_ModuleInit);
#ifdef __cplusplus
}
#endif

#endif /* __LINUX__*/
