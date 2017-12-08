#include <types.h>
#include <lib.h>
#include <wchan.h>
#include <thread.h>
#include <synch.h>
#include <clock.h>
#include <philo.h>

#define NPHIL 5

static const char *phil[NPHIL] = { "Aristotle", "Kant", "Spinoza", "Marx", "Russell" };
static const char *stick[NPHIL] = { "stick #1", "stick #2", "stick #3", "stick #4", "stick #5" };
static struct lock *chopstick[NPHIL];
static struct lock *foodex;
static struct semaphore *daend;
static volatile int food;

static
void
philsay(int i, const char *s)
{
	kprintf("%s %s.\n", phil[i],s);
}

static
void
eat(int i)
{
	philsay(i, "eats");
	lock_acquire(foodex);
	food--;
	lock_release(foodex);
}

static
void
think(int i)
{
	philsay(i, "thinks");
	clocksleep(1+random()%3);
}


static
void
pick(int i, int j, const char *name)
{
	char buf[128];

	snprintf(buf, 128, "picks his %s chopstick (#%d)", name, j);
	philsay(i, buf);
	lock_acquire(chopstick[j]);
	thread_yield(); // add more fun
}

static
void
unpick(int i, int j, const char *name)
{

	char buf[128];

	snprintf(buf, 128, "releases his %s chopstick (#%d)", name, j);
	philsay(i, buf);
	lock_release(chopstick[j]);
	thread_yield(); // add more fun
}


static
void
philothread(void *junk, unsigned long num)
{
        int i, burp, left, right;
        (void)junk;
	char buf[128];
	const char *sname[]={ "left", "right" };

	i=(int)num;
	burp=0;
	left=i;
	right=(i+1)%NPHIL;
	if (right<left) {
		left=right;
		right=i;
		sname[0]="right";
		sname[1]="left";
	}
	philsay(i,"enters the room");
	while(food>0) {
		pick(i, left, sname[0]);
		pick(i, right, sname[1]);
		eat(i); burp++;
		unpick(i, left, sname[0]);
		unpick(i, right, sname[1]);
		think(i);
	}
	snprintf(buf,128,"leaves the room (ate %d times)", burp);
	philsay(i,buf);
	V(daend);
}

int
philotest(int nargs, char **args)
{
	int i, result;

	kprintf("Starting Dining Philosophers test...\n");
	daend=sem_create("daend",0);
	if (daend==NULL) panic("philotest: sem_create failed\n");
	foodex=lock_create("foodex");
	if (foodex==NULL) panic("philotest: lock_create failed\n");
	if (nargs==1)
		food=NPHIL*5;
	else
		food=atoi(args[1]);

	for(i=0;i<NPHIL;i++) {
		chopstick[i]=lock_create(stick[i]);
		if (chopstick[i]==NULL) panic("philotest: lock_create failed\n");
	}
	for(i=0;i<NPHIL;i++)  {
		result = thread_fork(phil[i], NULL, philothread, NULL, i);
		if (result)
			panic("philotest: thread_for failed: %s\n",
				strerror(result));
	}
	for(i=0;i<NPHIL;i++) P(daend);
	for(i=0;i<NPHIL;i++)
		lock_destroy(chopstick[i]);
	lock_destroy(foodex);
	kprintf("Dining Philosophers test done.\n");
	return 0;
}
