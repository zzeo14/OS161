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
#define TURNRIGHT 1
#define TURNLEFT 2

// 차량 상태
#define START 0
#define GOING 1
#define OUT 2
#define DIRECTION 3

static volatile unsigned long testval1;
static volatile unsigned long testval2;
static volatile unsigned long testval3;
static struct semaphore *testsem;
static struct lock *testlock;
static struct cv *testcv;
static struct semaphore *donesem;

// semaphore for assignment
static struct semaphore* semNW;
static struct semaphore* semNE;
static struct semaphore* semSW;
static struct semaphore* semSE;
static struct semaphore* semprint;
static struct semaphore* semthreads;

static
void
inititems(void)
{
	if (semthreads == NULL){
		semthreads = sem_create("semthreads", 0);
		if(semthreads == NULL){
			kprintf("semthreads error");
		}
	}
	if (semprint == NULL){
		semprint = sem_create("printsem", 1);
		if(semprint == NULL){
			kprintf("Zeo Error");
		}
	}
	if (semNW == NULL){
		semNW = sem_create("NW", 1);
		if(semNW == NULL){
			panic("Zeo Error");
		}
	}
	if (semNE == NULL){
		semNE = sem_create("NE", 1);
		if(semNE == NULL){
			panic("Zeo Error");
		}
	}
	if (semSW == NULL){
		semSW = sem_create("SW", 1);
		if(semSW == NULL){
			panic("Zeo Error");
		}
	}
	if (semSE == NULL){
		semSE = sem_create("SE", 1);
		if(semSE == NULL){
			panic("Zeo Error");
		}
	}
	if(testsem==NULL){
		testsem = sem_create("testsem", 2);
		if(testsem == NULL){
			panic("synchtest: sem_create failed\n");
		}
	}
	if(donesem==NULL){
		donesem = sem_create("donesem", 2);
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
message_function(const char *from, const char *to, int car_num, int status){
	P(semprint);
	if(status == DIRECTION){
		kprintf("Car %d starts for %s\n", car_num, from);
	}
	else if(status == START){
		kprintf("Car %d starts from %s\n", car_num, from);
	}
	else if(status == GOING) {
		kprintf("Car %d goes from %s to %s\n", car_num, from, to);	
	}
	else if(status == OUT){
		kprintf("Car %d goes out to %s\n", car_num, to);
	}
	V(semprint);
}

static
void
gostraight(int first_status, int car_num){
	if(first_status == NORTH){  // north는 SW 먹고 NW 먹기
		P(semSW); 	
			P(semNW);
			message_function("North", "", car_num, START);
			message_function("North", "NW", car_num, GOING);
			V(semNW);
		message_function("NW", "SW", car_num, GOING);
		message_function("SW", "South", car_num, GOING);
		message_function("", "South", car_num, OUT);
		V(semSW);

	}
	else if(first_status == SOUTH){ // south는 NE 먹고 SE 먹기
		P(semNE);
			P(semSE);
			message_function("South", "", car_num, START);
			message_function("South", "SE", car_num, GOING);
			V(semSE);
		message_function("SE", "NE", car_num, GOING);
		message_function("NE", "North", car_num, GOING);
		message_function("", "North", car_num, OUT);
		V(semNE);
	}
	else if(first_status == EAST){ // east는 NE 먹고 NW 먹기
		P(semNE);
		message_function("East", "", car_num, START);
		message_function("East", "NE", car_num, GOING);
		V(semNE);

		P(semNW);
		message_function("NE", "NW", car_num, GOING);
		message_function("NW", "West", car_num, GOING);
		message_function("", "West", car_num, OUT);
		V(semNW);
	}
	else if(first_status == WEST){ // west는 SW먹고 SE 먹기
		P(semSW);
		message_function("West", "", car_num, START);
		message_function("West", "SW", car_num, GOING);
		V(semSW);

		P(semSE);
		message_function("SW", "SE", car_num, GOING);
		message_function("SE", "East", car_num, GOING);
		message_function("", "East", car_num, OUT);
		V(semSE);
	}
}

static
void turnleft(int first_status, int car_num){
	if(first_status == NORTH){ // SW -> NW -> SE 순으로 먹기  
		P(semSW);
			P(semNW);
			message_function("North", "", car_num, START);
			message_function("North", "NW", car_num, GOING);
			V(semNW);
		message_function("NW", "SW", car_num, GOING);
		V(semSW);

		P(semSE);
		message_function("SW", "SE", car_num, START);
		message_function("SE", "East", car_num, GOING);
		message_function("", "East", car_num, OUT);
		V(semSE);		
	}
	else if(first_status == SOUTH){ // NE -> NW -> SE 순으로 먹기
		P(semNE);
			P(semNW);
				P(semSE);
				message_function("South", "", car_num, START);
				message_function("South", "SE", car_num, GOING);
				V(semSE);
		message_function("SE", "NE", car_num, GOING);
		V(semNE);
			message_function("NE", "NW", car_num, START);
			message_function("NW", "West", car_num, GOING);
			message_function("", "West", car_num, OUT);
			V(semNW);		
	}
	else if(first_status == EAST){ // SW -> NE -> NW 순으로 먹기
		P(semSW);
			P(semNE);
			message_function("East", "", car_num, START);
			message_function("East", "NE", car_num, GOING);
			V(semNE);

			P(semNW);
			message_function("NE", "NW", car_num, GOING);		
			V(semNW);
		message_function("NW", "SW", car_num, GOING);		
		message_function("SW", "South", car_num, GOING);		
		message_function("","South", car_num, OUT);
		V(semSW);
	}
	else if(first_status == WEST){ // SW -> NE -> SE 순으로 먹기
		P(semSW);
		message_function("West", "", car_num, START);
		message_function("West", "SW", car_num, GOING);
		V(semSW);
		P(semNE);
			P(semSE);
			message_function("SW", "SE", car_num, GOING);		
			V(semSE);
		message_function("SE", "NE", car_num, GOING);		
		message_function("NE", "North", car_num, GOING);		
		message_function("","North", car_num, OUT);
		V(semNE);
	}
}



static
void turnright(int first_status, int car_num){
	if(first_status == NORTH){
		P(semNW);
		message_function("North", "", car_num, START);
		message_function("North", "NW", car_num, GOING);
		message_function("NW", "WEST", car_num, GOING);
		message_function("", "West", car_num, OUT);
		V(semNW);
	}
	else if(first_status == SOUTH){
		P(semSE);
		message_function("South", "", car_num, START);
		message_function("South", "SE", car_num, GOING);
		message_function("SE", "EAST", car_num, GOING);
		message_function("", "East", car_num, OUT);
		V(semSE);
	}
	else if(first_status == EAST){
		P(semNE);
		message_function("East", "", car_num, START);
		message_function("EAST", "NE", car_num, GOING);
		message_function("NE", "North", car_num, GOING);
		message_function("", "North", car_num, OUT);
		V(semNE);
	}
	else if(first_status == WEST){
		P(semSW);
		message_function("West", "", car_num, START);
		message_function("WEST", "SW", car_num, GOING);
		message_function("SW", "South", car_num, GOING);
		message_function("", "South", car_num, OUT);	
		V(semSW);
	}
}	

static
void
semtestthread(void *junk, unsigned long num)
{
	(void)junk;

	// 자동차 나타날 위치
	int first_status = random() % 4; // 0: north, 1: south, 2: east, 3: west
					 
	// 자동차가 이동할 방향
	int direction = random() % 3; // 0: straight, 1: turn left, 2: turn right

	if(direction == STRAIGHT){
		message_function("go straight", "", num, DIRECTION);	
		gostraight(first_status, num);
	}
	else if(direction == TURNLEFT){
		message_function("turn left", "", num, DIRECTION);
		turnleft(first_status, num);	
	}
	else if(direction == TURNRIGHT){
		message_function("turn right", "", num, DIRECTION);
		turnright(first_status, num);
	}

	V(semthreads);
}        

static
void
makethreads(){
	int i, result;
	
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

	for(int i = 0 ; i < NTHREADS ; i++){
		P(semthreads);
	}
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
