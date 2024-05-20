#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

#define NSEMLOOPS     63
#define NLOCKLOOPS    120
#define NCVLOOPS      5
#define NTHREADS      32

// 차량 접근 위치
#define NORTH 0
#define SOUTH 1
#define WEST 2
#define EAST 3

// 차량 진행 방향
#define STRAIGHT 0
#define TURNLEFT 1
#define TURNRIGHT 2

static volatile unsigned long testval1;
static volatile unsigned long testval2;
static volatile unsigned long testval3;
static struct semaphore *testsem;
static struct lock *testlock;
static struct cv *testcv;
static struct semaphore *donesem;

// semaphore for assignment
static struct semaphore* NW;
static struct semaphore* NE;
static struct semaphore* SW;
static struct semaphore* SE;
static struct semaphore* printsem;

static
void
inititems(void)
{
	if (printsem == NULL){
		printsem = sem_create("printsem", 1);
		if(printsem == NULL){
			kprintf("Zeo Error");
		}
	}
	if (NW == NULL){
		NW = sem_create("NW", 1);
		if(NW == NULL){
			panic("Zeo Error");
		}
	}
	if (NE == NULL){
		NE = sem_create("NE", 1);
		if(NE == NULL){
			panic("Zeo Error");
		}
	}
	if (SW == NULL){
		SW = sem_create("SW", 1);
		if(SW == NULL){
			panic("Zeo Error");
		}
	}
	if (SE == NULL){
		SE = sem_create("SE", 1);
		if(SE == NULL){
			panic("Zeo Error");
		}
	}
	if(testsem==NULL){
		testem = sem_create("testsem", 2);
		if(testsem == NULL){
			panic("synchtest: sem_create failed\n");
		}
	}
	if(donesem==NULL){
		testem = sem_create("donesem", 2);
		if(donesem == NULL){
			panic("synchtest: sem_create failed\n");
		}
	}	
	if(testlock==NULL){
		testlock = lock_create("testlock");
		if(testlock == NULL){
			panic("synchtest: lock_create failed\n");
		}
	}
	if(testcv==NULL){
		testcv = cv_create("testlock");
		if(testcv == NULL){
			panic("synchtest: sem_create failed");
		}
	}
}

static
void 
message_function(const char *from, const char *to, int car){
	kprintf("    Car %d goes from %s to %s\n", car, from, to);	
}

static
void
gostraight(int first_status, int car_num){

}

static
void turnleft(int first_status, int car_num){
	
}



static
void turnright(int first_status, int car_num){
}

static
void
semtestthread(void *junk, unsigned long num)
{
	(void)junk;
	(void) num;

}        

static
void
makethreads(){
	int i, result;
	
	// 자동차 나타날 위치
	int first_status = random() % 4; // 0: north, 1: south, 2: east, 3: west
					 
	// 자동차가 이동할 방향
	//int direction = random() % 3; // 0: straight, 1: turn left, 2: turn right
	int direction = STRAIGHT;

	kprintf("Car #: %ld, ", num);
	for(i = 0 ; i < NTHREADS; i++){
		result = thread_fork("semtest", NULL, semtestthread, NULL, i);
		if(result){
			panic("i am Kim zeo");
		}
	}
}

int
semtest(int nargs, char **args)
{
	(void)nargs; (void)args;

	inititems();
	makethreads();	

	return 0;
}

static
void
fail(unsigned long num, const char *msg)
{
	kprintf("thread %lu: Mismatch on %s\n", num, msg);
	kprintf("Test failed\n");

	lock_release(testlock);

	V(donesem);
	thread_exit();
}

static
void
locktestthread(void *junk, unsigned long num)
{
	int i;
	(void)junk;

	for (i=0; i<NLOCKLOOPS; i++) {
		lock_acquire(testlock);
		testval1 = num;
		testval2 = num*num;
		testval3 = num%3;

		if (testval2 != testval1*testval1) {
			fail(num, "testval2/testval1");
		}

		if (testval2%3 != (testval3*testval3)%3) {
			fail(num, "testval2/testval3");
		}

		if (testval3 != testval1%3) {
			fail(num, "testval3/testval1");
		}

		if (testval1 != num) {
			fail(num, "testval1/num");
		}

		if (testval2 != num*num) {
			fail(num, "testval2/num");
		}

		if (testval3 != num%3) {
			fail(num, "testval3/num");
		}

		lock_release(testlock);
	}
	V(donesem);
}


int
locktest(int nargs, char **args)
{
	int i, result;

	(void)nargs;
	(void)args;

	inititems();
	kprintf("Starting lock test...\n");

	for (i=0; i<NTHREADS; i++) {
		result = thread_fork("synchtest", NULL, locktestthread,
				     NULL, i);
		if (result) {
			panic("locktest: thread_fork failed: %s\n",
			      strerror(result));
		}
	}
	for (i=0; i<NTHREADS; i++) {
		P(donesem);
	}

	kprintf("Lock test done.\n");

	return 0;
}

static
void
cvtestthread(void *junk, unsigned long num)
{
	int i;
	volatile int j;
	struct timespec ts1, ts2;

	(void)junk;

	for (i=0; i<NCVLOOPS; i++) {
		lock_acquire(testlock);
		while (testval1 != num) {
			gettime(&ts1);
			cv_wait(testcv, testlock);
			gettime(&ts2);

			/* ts2 -= ts1 */
			timespec_sub(&ts2, &ts1, &ts2);

			/* Require at least 2000 cpu cycles (we're 25mhz) */
			if (ts2.tv_sec == 0 && ts2.tv_nsec < 40*2000) {
				kprintf("cv_wait took only %u ns\n",
					ts2.tv_nsec);
				kprintf("That's too fast... you must be "
					"busy-looping\n");
				V(donesem);
				thread_exit();
			}

		}
		kprintf("Thread %lu\n", num);
		testval1 = (testval1 + NTHREADS - 1)%NTHREADS;

		/*
		 * loop a little while to make sure we can measure the
		 * time waiting on the cv.
		 */
		for (j=0; j<3000; j++);

		cv_broadcast(testcv, testlock);
		lock_release(testlock);
	}
	V(donesem);
}

int
cvtest(int nargs, char **args)
{

	int i, result;

	(void)nargs;
	(void)args;

	inititems();
	kprintf("Starting CV test...\n");
	kprintf("Threads should print out in reverse order.\n");

	testval1 = NTHREADS-1;

	for (i=0; i<NTHREADS; i++) {
		result = thread_fork("synchtest", NULL, cvtestthread, NULL, i);
		if (result) {
			panic("cvtest: thread_fork failed: %s\n",
			      strerror(result));
		}
	}
	for (i=0; i<NTHREADS; i++) {
		P(donesem);
	}

	kprintf("CV test done\n");

	return 0;
}

////////////////////////////////////////////////////////////

/*
 * Try to find out if going to sleep is really atomic.
 *
 * What we'll do is rotate through NCVS lock/CV pairs, with one thread
 * sleeping and the other waking it up. If we miss a wakeup, the sleep
 * thread won't go around enough times.
 */

#define NCVS 250
#define NLOOPS 40
static struct cv *testcvs[NCVS];
static struct lock *testlocks[NCVS];
static struct semaphore *gatesem;
static struct semaphore *exitsem;

static
void
sleepthread(void *junk1, unsigned long junk2)
{
	unsigned i, j;

	(void)junk1;
	(void)junk2;

	for (j=0; j<NLOOPS; j++) {
		for (i=0; i<NCVS; i++) {
			lock_acquire(testlocks[i]);
			V(gatesem);
			cv_wait(testcvs[i], testlocks[i]);
			lock_release(testlocks[i]);
		}
		kprintf("sleepthread: %u\n", j);
	}
	V(exitsem);
}

static
void
wakethread(void *junk1, unsigned long junk2)
{
	unsigned i, j;

	(void)junk1;
	(void)junk2;

	for (j=0; j<NLOOPS; j++) {
		for (i=0; i<NCVS; i++) {
			P(gatesem);
			lock_acquire(testlocks[i]);
			cv_signal(testcvs[i], testlocks[i]);
			lock_release(testlocks[i]);
		}
		kprintf("wakethread: %u\n", j);
	}
	V(exitsem);
}

int
cvtest2(int nargs, char **args)
{
	unsigned i;
	int result;

	(void)nargs;
	(void)args;

	for (i=0; i<NCVS; i++) {
		testlocks[i] = lock_create("cvtest2 lock");
		testcvs[i] = cv_create("cvtest2 cv");
	}
	gatesem = sem_create("gatesem", 0);
	exitsem = sem_create("exitsem", 0);

	kprintf("cvtest2...\n");

	result = thread_fork("cvtest2", NULL, sleepthread, NULL, 0);
	if (result) {
		panic("cvtest2: thread_fork failed\n");
	}
	result = thread_fork("cvtest2", NULL, wakethread, NULL, 0);
	if (result) {
		panic("cvtest2: thread_fork failed\n");
	}

	P(exitsem);
	P(exitsem);

	sem_destroy(exitsem);
	sem_destroy(gatesem);
	exitsem = gatesem = NULL;
	for (i=0; i<NCVS; i++) {
		lock_destroy(testlocks[i]);
		cv_destroy(testcvs[i]);
		testlocks[i] = NULL;
		testcvs[i] = NULL;
	}

	kprintf("cvtest2 done\n");
	return 0;
}
