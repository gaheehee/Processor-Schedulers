/**********************************************************************
 * Copyright (c) 2019-2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

/* THIS FILE IS ALL YOURS; DO WHATEVER YOU WANT TO DO IN THIS FILE */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;


/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;


/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];


/**
 * Monotonically increasing ticks
 */
extern unsigned int ticks;


/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;


/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order 요청 순서에 따라 서비스
 *   without considering the priority. See the comments in sched.h 우선순위는 고려하지 않고
 ***********************************************************************/
bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_WAIT;

	/* And append current to waitqueue */ //리소스의 wait queue에 들어감
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available. false를 반환하여 리소스를 사용할 수 없음을 나타냄
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION 현재 구현은 우선 순위를 고려하지 않고 요청 순서에 따라 리소스를 제공
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter is in the wait status
		 */
		assert(waiter->status == PROCESS_WAIT);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}



#include "sched.h"

/***********************************************************************
 * FIFO scheduler
 ***********************************************************************/
static int fifo_initialize(void)
{
	return 0;
}

static void fifo_finalize(void)
{
}

static struct process *fifo_schedule(void)
{
	struct process *next = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, when the current process is blocked
	 * while acquiring a resource, @current is (supposed to be) attached
	 * to the waitqueue of the corresponding resource. In this case just
	 * pick the next as well.
	 */
	/* 이전 tick에서 실행할 프로세스가 없는 경우(시뮬레이션 시작 부분에서도 실행됨), current프로세스는 없다
	 * 이 경우 current프로세스를 검사하지 않고 next프로세스를 선택한다 
	 * 또한 리소스를 획득하는 동안 current프로세스가 차단(block)되면 @current는 해당 리소스의 대기열(waitqueue)에 연결된다
     * 이 경우 next도 함께 선택한다
	 */

	if (!current || current->status == PROCESS_WAIT) {	
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) {	// age: # of ticks the process was scheduled in
		return current;
	}
// 돌아야 하는 만큼 다 돌았음 
pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) {
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list);
		/**
		 * Detach the process from the ready queue. Note we use list_del_init()
		 * instead of list_del() to maintain the list head tidy. Otherwise,
		 * the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list);
	}

	/* Return the next process to run */
	return next;
}

struct scheduler fifo_scheduler = {
	.name = "FIFO",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.schedule = fifo_schedule,
};


/***********************************************************************
 * SJF scheduler
 ***********************************************************************/
static struct process *sjf_schedule(void)
{
	struct process *next = NULL;
	struct process* temp = NULL;
	int i = 0;
	//dump_status();

	// current가 없는 경우
	if (!current || current->status == PROCESS_WAIT) {	
		goto pick_next;
	}
	// 아직 돌게 남음
	// no preemptition: 다 돌 때까지 계속 돌림
	if (current->age < current->lifespan) {		
		return current;
	}
				
pick_next:		// 돌아야 하는 만큼 다 돌았음  // 새로운 process 뽑기
				// readyqueue 비어있지 않으면, 우선순위 높은거 뽑기
	if (!list_empty(&readyqueue)) {		
														
		list_for_each_entry(temp, &readyqueue, list){	// i = max(lifespan)
			if(i < temp->lifespan){						
				i = temp->lifespan;
			}
		}

		list_for_each_entry(temp, &readyqueue, list){	// 가장 작은 lifespan 찾음
			if(i > temp->lifespan){						
				i = temp->lifespan;
			}
		}
		list_for_each_entry_reverse(temp, &readyqueue, list){	// i와 같은 lifespan 뽑아냄
			if(i == temp->lifespan){							// 꼭 reverse 안 써도 됌 
				next = temp;									// 돌다가 처음 만나면 바로 지우고 리턴해주고 끝내면 됌
			}
		} 
		list_del_init(&next->list);		// ready queue에서 해당 process 삭제
	}
	return next;
}


struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = sjf_schedule,		 /* TODO: Assign sjf_schedule()
								to this function pointer to activate
								SJF in the system */
};


/***********************************************************************
 * SRTF scheduler
 ***********************************************************************/
static struct process *srtf_schedule(void){

	struct process *next = NULL;
	struct process* temp;
	int i = 0;

	//dump_status();

	// current가 없는 경우
	if (!current || current->status == PROCESS_WAIT) {	
		goto pick_next;
	}

	// current가 아직 돌게 남음
	if (current->age < current->lifespan)		
	{	
		current->status = PROCESS_READY;			// current를 ready 상태로 바꾼 후
		list_add(&current->list, &readyqueue);		// readyqueue에 넣어줌	 
		
	}

pick_next:	// readyqueue에서 lifespan 적은 거 고름

	if (!list_empty(&readyqueue)) {

		list_for_each_entry(temp, &readyqueue, list){	// i = max(lifespan)
			if(i < temp->lifespan){						
				i = temp->lifespan;
			}
		}

		list_for_each_entry(temp, &readyqueue, list) {	// i = 가장 작은 remain time(lifespan-age)
			if (i > temp->lifespan-temp->age){	
				i = temp->lifespan-temp->age;
			}
		}

		list_for_each_entry(temp, &readyqueue, list) {	// 가장 작은 remain time을 가진 process 만나면 리턴
			if (i == temp->lifespan-temp->age){
				next = temp;
				list_del_init(&next->list);
				return next;
			}
		}	
	}
return next;

} 


struct scheduler srtf_scheduler = {
	.name = "Shortest Remaining Time First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = srtf_schedule,
	/* You need to check the newly created processes to implement SRTF.
	 * Use @forked() callback to mark newly created processes */
	/* Obviously, you should implement srtf_schedule() and attach it here */
};


/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/
static struct process *rr_schedule(void){
	struct process *next = NULL;
	struct process* temp = NULL;

	// current가 없는 경우
	if (!current || current->status == PROCESS_WAIT) {	
		goto pick_next;
	}
	// current가 아직 돌게 남음		// preemption
	if (current->age < current->lifespan)		
	{	
		current->status = PROCESS_READY;				// current를 ready 상태로 바꾼 후
		list_add_tail(&current->list, &readyqueue);	 	// readyqueue 끝에 넣어줌	
	}

pick_next:

	if (!list_empty(&readyqueue)) {
		
		next = list_first_entry(&readyqueue, struct process, list);
		list_del_init(&next->list);
		return next;
	}
	
return next;

}

struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = rr_schedule,
	/* Obviously, you should implement rr_schedule() and attach it here */
};


/***********************************************************************
 * Priority scheduler
 ***********************************************************************/

void prio_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	struct process *waiter = NULL;
	struct process *temp = NULL;
	int i = 0;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);
	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		
		list_for_each_entry(temp, &r->waitqueue, list){	// 가장 큰 prio 찾음
			if(i < temp->prio){	
				i = temp->prio;
				waiter = temp;
			}
		}
		assert(waiter->status == PROCESS_WAIT);
		list_del_init(&waiter->list);

		waiter->status = PROCESS_READY;
		list_add_tail(&waiter->list, &readyqueue);
	}
}

static struct process *prio_schedule(void){    //priority base: 동일한 우선 순위를 가진 프로세스는 라운드 로빈 방식으로 처리해야 함

	struct process *next = NULL;
	struct process* temp = NULL;
	int i = 0;

	//dump_status();

	// current가 없는 경우
	if (!current || current->status == PROCESS_WAIT) {	
		goto pick_next;
	}
	// 아직 돌게 남음		// preemption
	if (current->age < current->lifespan) {		
		current->status = PROCESS_READY;			// current를 ready 상태로 바꾼 후
		list_add_tail(&current->list, &readyqueue);		// readyqueue 끝에 넣어줌
	}
				
pick_next:		
	// 우선순위 높은거 뽑기
	if (!list_empty(&readyqueue)) {		

		list_for_each_entry(temp, &readyqueue, list){	// 가장 큰 prio 찾음
			if(i < temp->prio){	
				i = temp->prio;
			}
		}

		list_for_each_entry(temp, &readyqueue, list){	// i와 같은 prio 뽑아냄
			if(i == temp->prio){								 
				next = temp;	
				list_del_init(&next->list);	
				return next;									
			}
		} 
	}
	return next;	
}


struct scheduler prio_scheduler = {
	.name = "Priority",
	.acquire = fcfs_acquire,
	.release = prio_release,
	.schedule = prio_schedule,
	/**
	 * Implement your own acqure/release function to make priority
	 * scheduler correct.
	 */
	/* Implement your own prio_schedule() and attach it here */
};


/***********************************************************************
 * Priority scheduler with aging
 ***********************************************************************/
static struct process *pa_schedule(void){	

	struct process *next = NULL;
	struct process* temp = NULL;
	int i = 0;
	//dump_status();

	// current가 없는 경우
	if (!current || current->status == PROCESS_WAIT) {	
		goto pick_next;
	}
	// 아직 돌게 남음		// preemption
	if (current->age < current->lifespan) {		
		current->status = PROCESS_READY;			// current를 ready 상태로 바꾼 후
		list_add_tail(&current->list, &readyqueue);		// readyqueue 끝에 넣어줌
	}
				
pick_next:		
	// 우선순위 높은거 뽑기
	if (!list_empty(&readyqueue)) {		

		list_for_each_entry(temp, &readyqueue, list){	// 가장 큰 prio(i) 찾음
			temp->prio = (temp-> prio + 1);				// prio 모두 ++
			if(temp->prio > MAX_PRIO){					// 아무리 커져도 MAX_PRIO 보다는 못 커짐
				temp->prio = MAX_PRIO;
			}
			if(i < temp->prio){	
				i = temp->prio;
			}
		}

		list_for_each_entry(temp, &readyqueue, list){	// i와 같은 prio 뽑아냄
			if(i == temp->prio){								 
				next = temp;	
				next->prio = next->prio_orig;			// origin prio로 되돌려놓기
				list_del_init(&next->list);	
				return next;									
			}
		} 

	}
	return next;	
}


struct scheduler pa_scheduler = {
	.name = "Priority + aging",
	.acquire = fcfs_acquire,
	.release = prio_release,
	.schedule = pa_schedule,
	/**
	 * Implement your own acqure/release function to make priority
	 * scheduler correct.
	 */
	/* Implement your own prio_schedule() and attach it here */
};


/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/
bool pcp_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;
	struct process *temp =NULL;

	// 쓰는 애 없으니까 가져감
	if (!r->owner) {	
		r->owner = current;		// 리소스 주인을 자기로 함
		current->prio = MAX_PRIO;	// 리소스 쓰는 애의 prio를 MAX로 해줌
		return true;
	}

	// 쓰는 애 있음 
	current->status = PROCESS_WAIT;
	list_add_tail(&current->list, &r->waitqueue);	//리소스의 wait queue에 들어감
	return false;	// 사용 못함을 나타냄
}

void pcp_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	struct process *waiter = NULL;
	struct process *temp = NULL;
	int i = 0, inherit_prio = 0;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);
	current->prio = current->prio_orig;		// 원래 prio로 돌아옴
	r->owner = NULL;	// 리소스 소유 취소

	/* Let's wake up ONE waiter (if exists) that came first */	//먼저 온 웨이터 한 개 깨움
	if (!list_empty(&r->waitqueue)) {

		list_for_each_entry(temp, &r->waitqueue, list){	// 가장 큰 prio 찾음
			if(i < temp->prio){	
				i = temp->prio;
				waiter = temp;
			}
		}
		// Ensure the waiter is in the wait status
		assert(waiter->status == PROCESS_WAIT);

		list_del_init(&waiter->list);
		waiter->prio = MAX_PRIO;		// 다음 리소스 잡을 애의 prio를 MAX_PRIO로 해줌
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}


static struct process *pcp_schedule(void){	

	struct process *next = NULL;
	struct process* temp = NULL;
	int i = 0;

	//dump_status();

	// current가 없는 경우
	if (!current || current->status == PROCESS_WAIT) {	
		goto pick_next;
	}
	// 아직 돌게 남음		// preemption
	if (current->age < current->lifespan) {		
		current->status = PROCESS_READY;			// current를 ready 상태로 바꾼 후
		list_add_tail(&current->list, &readyqueue);		// readyqueue 끝에 넣어줌
	}
				
pick_next:		
	// 우선순위 높은거 뽑기
	if (!list_empty(&readyqueue)) {		

		list_for_each_entry(temp, &readyqueue, list){	// 가장 큰 prio 찾음
			if(i <= temp->prio){	
				i = temp->prio;
			}
		}

		list_for_each_entry(temp, &readyqueue, list){	// i와 같은 prio 뽑아냄
			if(i == temp->prio){								 
				next = temp;	
				list_del_init(&next->list);	
				return next;									
			}
		}
	}
	return next;	
}

struct scheduler pcp_scheduler = {
	.name = "Priority + PCP Protocol",
	.acquire = pcp_acquire,
	.release = pcp_release,
	.schedule = pcp_schedule,
	/**
	 * Implement your own acqure/release function too to make priority
	 * scheduler correct.
	 */
};


/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/

bool pip_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;
	struct process *temp =NULL;
	int inherit_prio = 0;

	//dump_status();
	// 쓰는 애 없으니까 가져감
	if (!r->owner) {	

		r->owner = current;		// 리소스 주인을 자기로 함
		
		list_for_each_entry(temp, &r->waitqueue, list){
			if (inherit_prio < temp->prio_orig){ 
				inherit_prio = temp->prio_orig;
			}
		}

		if(current->prio < inherit_prio){
			current->prio = inherit_prio;
		}

		return true;
	}

	// 쓰는 애 있음 
	// wait 하기 전에 리소스 쥐고 있는 애의 priority가 작으면 내꺼 빌려줌   

	if(r->owner->prio < current->prio_orig){
		r->owner->prio = current->prio_orig;
	}

	current->status = PROCESS_WAIT;
	list_add_tail(&current->list, &r->waitqueue);	//리소스의 wait queue에 들어감
	return false;	// 사용 못함을 나타냄

	
}

void pip_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	struct process *waiter = NULL;
	struct process *temp = NULL;
	int i = 0, inherit_prio = 0;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	//inherit_prio = current->prio;			
	current->prio = current->prio_orig;		// 원래 prio로 돌아옴
	r->owner = NULL;	// 리소스 소유 취소

	/* Let's wake up ONE waiter (if exists) that came first */	//먼저 온 웨이터 한 개 깨움
	if (!list_empty(&r->waitqueue)) {

		list_for_each_entry(temp, &r->waitqueue, list){	// 가장 큰 prio 찾음
			if(i < temp->prio_orig){	
				i = temp->prio_orig;
				waiter = temp;
			}
		}

		// Ensure the waiter is in the wait status
		assert(waiter->status == PROCESS_WAIT);

		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;

		list_add_tail(&waiter->list, &readyqueue);
	}
}

static struct process *pip_schedule(void){	

	struct process *next = NULL;
	struct process* temp = NULL;
	int i = 0;

	dump_status();

	// current가 없는 경우
	if (!current || current->status == PROCESS_WAIT) {	
		goto pick_next;
	}
	// 아직 돌게 남음		// preemption
	if (current->age < current->lifespan) {		
		//current->status = PROCESS_READY;			// current를 ready 상태로 바꾼 후
		list_add_tail(&current->list, &readyqueue);		// readyqueue 끝에 넣어줌
	}
				
pick_next:		
	// 우선순위 높은거 뽑기
	if (!list_empty(&readyqueue)) {		

		list_for_each_entry(temp, &readyqueue, list){	// 가장 큰 prio 찾음
			if(i <= temp->prio){	
				i = temp->prio;
			}
		}

		list_for_each_entry(temp, &readyqueue, list){	// i와 같은 prio 뽑아냄
			if(i == temp->prio){								 
				next = temp;	
				list_del_init(&next->list);	
				return next;									
			}
		}
	}
	return next;	
}

struct scheduler pip_scheduler = {
	.name = "Priority + PIP Protocol",
	.acquire = pip_acquire,
	.release = pip_release,
	.schedule = pip_schedule,
	/**
	 * Ditto
	 */
};
