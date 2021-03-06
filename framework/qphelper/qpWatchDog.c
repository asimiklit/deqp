/*-------------------------------------------------------------------------
 * drawElements Quality Program Helper Library
 * -------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Watch dog for detecting timeouts
 *//*--------------------------------------------------------------------*/

#include "qpWatchDog.h"

#include "deThread.h"
#include "deClock.h"
#include "deMemory.h"

#include <stdio.h>

#if 0
#	define DBGPRINT(X) qpPrintf X
#else
#	define DBGPRINT(X)
#endif

typedef enum Status_e
{
	STATUS_THREAD_RUNNING = 0,
	STATUS_STOP_THREAD,

	STATUS_LAST
} Status;

struct qpWatchDog_s
{
	qpWatchDogFunc		timeOutFunc;
	void*				timeOutUserPtr;
	int					totalTimeLimit;			/* Total test case time limit in seconds 	*/
	int					intervalTimeLimit;		/* Iteration length limit in seconds 		*/

	volatile deUint64	resetTime;
	volatile deUint64	lastTouchTime;

	deThread			watchDogThread;
	volatile Status		status;
};

static void watchDogThreadFunc (void* arg)
{
	qpWatchDog* dog = (qpWatchDog*)arg;
	DE_ASSERT(dog);

	DBGPRINT(("watchDogThreadFunc(): start\n"));

	while (dog->status == STATUS_THREAD_RUNNING)
	{
		deUint64	curTime					= deGetMicroseconds();
		int			totalSecondsPassed		= (int)((curTime - dog->resetTime) / 1000000ull);
		int			secondsSinceLastTouch	= (int)((curTime - dog->lastTouchTime) / 1000000ull);

		if ((secondsSinceLastTouch > dog->intervalTimeLimit) || (totalSecondsPassed > dog->totalTimeLimit))
		{
			DBGPRINT(("watchDogThreadFunc(): call timeout func\n"));
			dog->timeOutFunc(dog, dog->timeOutUserPtr);
			break;
		}

		deSleep(100);
	}

	DBGPRINT(("watchDogThreadFunc(): stop\n"));
}

qpWatchDog* qpWatchDog_create (qpWatchDogFunc timeOutFunc, void* userPtr, int totalTimeLimitSecs, int intervalTimeLimitSecs)
{
	/* Allocate & initialize. */
	qpWatchDog* dog = (qpWatchDog*)deCalloc(sizeof(qpWatchDog));
	if (!dog)
		return dog;

	DE_ASSERT(timeOutFunc);
	DE_ASSERT((totalTimeLimitSecs > 0) && (intervalTimeLimitSecs > 0));

	DBGPRINT(("qpWatchDog::create(%ds, %ds)\n", totalTimeLimitSecs, intervalTimeLimitSecs));

	dog->timeOutFunc		= timeOutFunc;
	dog->timeOutUserPtr		= userPtr;
	dog->totalTimeLimit		= totalTimeLimitSecs;
	dog->intervalTimeLimit	= intervalTimeLimitSecs;

	/* Reset (sets time values). */
	qpWatchDog_reset(dog);

	/* Initialize watchdog thread. */
	dog->status			= STATUS_THREAD_RUNNING;
	dog->watchDogThread = deThread_create(watchDogThreadFunc, dog, DE_NULL);
	if (!dog->watchDogThread)
	{
		deFree(dog);
		return DE_NULL;
	}

	return dog;
}

void qpWatchDog_reset (qpWatchDog* dog)
{
	deUint64 curTime = deGetMicroseconds();

	DE_ASSERT(dog);
	DBGPRINT(("qpWatchDog::reset()\n"));

	dog->resetTime			= curTime;
	dog->lastTouchTime		= curTime;
}

void qpWatchDog_destroy (qpWatchDog* dog)
{
	DE_ASSERT(dog);
	DBGPRINT(("qpWatchDog::destroy()\n"));

	/* Finish the watchdog thread. */
	dog->status = STATUS_STOP_THREAD;
	deThread_join(dog->watchDogThread);
	deThread_destroy(dog->watchDogThread);

	DBGPRINT(("qpWatchDog::destroy() finished\n"));
	deFree(dog);
}

void qpWatchDog_touch (qpWatchDog* dog)
{
	DE_ASSERT(dog);
	DBGPRINT(("qpWatchDog::touch()\n"));
	dog->lastTouchTime = deGetMicroseconds();
}
