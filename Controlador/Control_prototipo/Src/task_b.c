/*
 * Copyright (c) 2023 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @file   : task_b.c
 * @date   : Set 26, 2023
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 * @version	v1.0.0
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"

/********************** macros and definitions *******************************/
#define G_TASK_B_CNT_INI	0ul

#define TASK_B_CNT_INI		0ul
#define TASK_B_CNT_MAX		50ul

#define TASK_B_DEL_INI		0ul
#define TASK_B_DEL_MAX		500ul

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_b 		= "Task B - Non-Blocking Code";

/********************** external data declaration ****************************/
uint32_t g_task_b_cnt;

/********************** external functions definition ************************/
void task_b_init(void *parameters)
{
	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - %s", GET_NAME(task_b_init), p_task_b);

	/* Init & Print out: Task execution counter */
	g_task_b_cnt = G_TASK_B_CNT_INI;
	LOGGER_INFO("   %s = %lu", GET_NAME(g_task_b_cnt), g_task_b_cnt);
}

void task_b_update(void *parameters)
{
	#if (TEST_X == TEST_0)

	static uint32_t task_b_cnt = TASK_B_CNT_INI;

	/* Update Task Counter */
	g_task_b_cnt++;

	if (task_b_cnt < TASK_B_CNT_MAX)
		task_b_cnt++;
	else
		task_b_cnt = TASK_B_CNT_INI;

	#endif

	#if (TEST_X == TEST_1)

	static uint32_t then = TASK_B_DEL_INI;
	static uint32_t now = TASK_B_DEL_INI;

	/* Check the current tick */
	now = HAL_GetTick();
	if ((now - then) >= TASK_B_DEL_MAX)
	{
		/* Only if the current tick is TASK_B_DEL_MAX mS after the last */
		/* Reset then = now */
		then = now;
	}

	#endif

	#if (TEST_X == TEST_2)

	/* Here Chatbot Artificial Intelligence generated code */

	#endif
}

/********************** end of file ******************************************/
