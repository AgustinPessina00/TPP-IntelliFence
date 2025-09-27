
#include <iostream> // Example C++ standard library inclusion
#include "myMain.h"
#include "cmsis_os.h"
#include "messages.h"

#include "cow.h"
#include "fence.h"

#include "sam_m10q.h"
#include "lsm6dso.h"
#include "ina226.h"
#include "threads/dispatcherTask.h"
//#include "threads/distanceTask.h"
//#include "threads/fenceUpdateTask.h"
#include "threads/fsmTask.h"
//#include "threads/gpsTask.h"
//#include "threads/loraTxTask.h"
//#include "threads/loraRxTask.h"
#include "threads/sensorAcqTask.h"
//#include "threads/stimulusTask.h"

extern I2C_HandleTypeDef hi2c2;

/* Definitions for fsm_Task */
osThreadId_t fsm_TaskHandle;
const osThreadAttr_t fsm_Task_attributes = {
  .name = "fsm_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for dispatcher_Task */
osThreadId_t dispatcher_TaskHandle;
const osThreadAttr_t dispatcher_Task_attributes = {
  .name = "dispatcher_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for sensorAcq_Task */
osThreadId_t sensorAcq_TaskHandle;
const osThreadAttr_t sensorAcq_Task_attributes = {
  .name = "sensorAcq_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for dispatcherQueue */
osMessageQueueId_t dispatcherQueueHandle;
const osMessageQueueAttr_t dispatcherQueue_attributes = {
  .name = "dispatcherQueue"
};
/* Definitions for sensorAcqQueue */
osMessageQueueId_t sensorAcqQueueHandle;
const osMessageQueueAttr_t sensorAcqQueue_attributes = {
  .name = "sensorAcqQueue"
};
/* Definitions for stimulusQueue */
osMessageQueueId_t stimulusQueueHandle;
const osMessageQueueAttr_t stimulusQueue_attributes = {
  .name = "stimulusQueue"
};
/* Definitions for gpsQueue */
osMessageQueueId_t gpsQueueHandle;
const osMessageQueueAttr_t gpsQueue_attributes = {
  .name = "gpsQueue"
};
/* Definitions for loraTxQueue */
osMessageQueueId_t loraTxQueueHandle;
const osMessageQueueAttr_t loraTxQueue_attributes = {
  .name = "loraTxQueue"
};
/* Definitions for loraRxQueue */
osMessageQueueId_t loraRxQueueHandle;
const osMessageQueueAttr_t loraRxQueue_attributes = {
  .name = "loraRxQueue"
};
/* Definitions for fsmQueue */
osMessageQueueId_t fsmQueueHandle;
const osMessageQueueAttr_t fsmQueue_attributes = {
  .name = "fsmQueue"
};
/* Definitions for distanceToLimitQueue */
osMessageQueueId_t distanceToLimitQueueHandle;
const osMessageQueueAttr_t distanceToLimitQueue_attributes = {
  .name = "distanceToLimitQueue"
};
/* Definitions for fenceUpdateQueue */
osMessageQueueId_t fenceUpdateQueueHandle;
const osMessageQueueAttr_t fenceUpdateQueue_attributes = {
  .name = "fenceUpdateQueue"
};

extern "C" { // Use extern "C" to ensure C linkage for functions called from C
    void RunCppApplication() {

    	SamM10q gps(&hi2c2, GPS_ADDRESS);
		// TODO: Chequear Params de la imu y de los INA.
		Lsm6dso imu(&hi2c2, IMU_ADDRESS, Lsm6dsoI3C::DISABLED, Lsm6dsoOdrAcc::ODR_52, Lsm6dsoFsAcc::FS_4G, Lsm6dsoOdrGyr::POWER_DOWN, Lsm6dsoFsGyr::FS_250DPS, Lsm6dsoWakeThs::THS_1, Lsm6dsoWakeDur::ODR_1, Lsm6dsoWakeWeight::FS_XL_64, Lsm6dsoSleepDur::DUR_1_512);
		Ina226 inaMcu(&hi2c2, INA_MCU_ADDRESS, 0.1f, 0.1f, Ina226Averaging::AVG_1, Ina226ConvTime::CT_140US, Ina226ConvTime::CT_140US, Ina226Mode::SHUNT_CONTINUOUS);
		Ina226 inaGps(&hi2c2, INA_GPS_ADDRESS, 0.1f, 0.1f, Ina226Averaging::AVG_1, Ina226ConvTime::CT_140US, Ina226ConvTime::CT_140US, Ina226Mode::SHUNT_CONTINUOUS);
		Ina226 inaImu(&hi2c2, INA_IMU_ADDRESS, 0.1f, 0.1f, Ina226Averaging::AVG_1, Ina226ConvTime::CT_140US, Ina226ConvTime::CT_140US, Ina226Mode::SHUNT_CONTINUOUS);

		// Obtaining the STM32WL55JC UID
		DeviceUID cowId = { HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2() };
		Cow cow(cowId);
		Fence fence;

        /* Init scheduler */
    	osKernelInitialize();

    	/* Create the queue(s) */
    	/* creation of dispatcherQueue */
    	dispatcherQueueHandle = osMessageQueueNew (256, sizeof(Message*), &dispatcherQueue_attributes);

    	/* creation of sensorAcqQueue */
    	sensorAcqQueueHandle = osMessageQueueNew (16, sizeof(Message*), &sensorAcqQueue_attributes);

    	/* creation of fsmQueue */
    	fsmQueueHandle = osMessageQueueNew (128, sizeof(Message*), &fsmQueue_attributes);

    	/* Create the thread(s) */
    	/* creation of fsm_Task */
		fsmTaskParams fsmParams = {
		  .cow = &cow,
		  .fence = &fence
		};
    	fsm_TaskHandle = osThreadNew(fsmTask, &fsmParams, &fsm_Task_attributes);

    	/* creation of dispatcher_Task */
    	dispatcher_TaskHandle = osThreadNew(dispatcherTask, NULL, &dispatcher_Task_attributes);

    	/* creation of sensorAcq_Task */
    	sensorAcqTaskParams sensorParams = {
			.gps = &gps,
			.imu = &imu,
			.inaMcu = &inaMcu,
			.inaGps = &inaGps,
			.inaImu = &inaImu
    	};
    	sensorAcq_TaskHandle = osThreadNew(sensorAcqTask, &sensorParams, &sensorAcq_Task_attributes);

    	/* Start scheduler */
		osKernelStart();
    }
}




