/*
*	Copyright 2012, Andrea Mazzotti <amm263@gmail.com> | Matteo Corfiati | Federico Fucci
*
*	Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted,
*	provided that the above copyright notice and this permission notice appear in all copies.
*
*	THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
*	INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR 
*	ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS 
* 	OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
*	OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define DEBUG 1 // Do you want a debug output and a realistic simulation (by sleep)? (1 = Yes| 0 = No)
//Note: sleep(rand()%2) means having a very poor probability distribution, there may be instances much worse than others.
#define N 141 //Number of parachutists
#define M 10  //Number of seats on the shuttle
#define P 20  //Number of seats on Fandango

//Enumerators
#define ATTESA 0
#define SALITI 1
#define PASSEGGERO 0
#define MEZZO 1
#define PARACADUTE 2
#define SCARICATO 2
#define INSERVIENTE 3

struct Monitor
{
	pthread_cond_t *condition; //Pointer for the creation of an array of conditions.
	int *contatori; //Pointer for the creation of an array of integers (counters).
	pthread_mutex_t mutex; //Mutex for the entry procedures.
};

struct Monitor Gate, Pista; //Global Monitors.
int gente_lanciata = 0; //People launched from Fandango.

void* saliNavetta(void *myId)
{
	pthread_mutex_lock(&(Gate.mutex));  //Enter in the Gate monitor.
	while (Gate.contatori[SALITI]==M) //For all the time in which the shuttle is full.
	{
		Gate.contatori[ATTESA]++; //Put on hold.
		pthread_cond_wait(&(Gate.condition[PASSEGGERO]),&(Gate.mutex)); //Wait.
		Gate.contatori[ATTESA]--; //Take off from waiting and see if it can take a seat.
	}
	Gate.contatori[SALITI]++; //Take the shuttle.
	pthread_cond_signal(&(Gate.condition[MEZZO])); //Tell the driver to leave.
	pthread_cond_wait(&(Gate.condition[SCARICATO]), &(Gate.mutex)); //Wait to actually be dropped on the track.
	pthread_mutex_unlock(&(Gate.mutex)); //Exit from Gate monitor.
}

void* partiNavetta(void *arg)
{
	int gente_trasportata = 0;
	int i;
	pthread_mutex_lock(&(Gate.mutex)); //Enter in the Gate monitor.
	while (gente_trasportata < N)
	{
		if (Gate.contatori[SALITI]==0) //If no one mounted on board...
            pthread_cond_wait(&(Gate.condition[MEZZO]),&(Gate.mutex)); //...put on hold.
		gente_trasportata = gente_trasportata + Gate.contatori[SALITI];
		if (DEBUG)
            {
                printf("Shuttle has dropped %3d passengers | Left %3d\n", Gate.contatori[SALITI], N - gente_trasportata);
                sleep(rand()%2); //Return time.
            }
		for (i=0; i<Gate.contatori[SALITI]; i++) //Confirm the transport of persons occurred.
		{
		    pthread_cond_signal(&(Gate.condition[SCARICATO]));
		}
		Gate.contatori[SALITI] = 0; //No one on board.
		for (i = 0; i < M; i++) //Free M seats.
		{
			pthread_cond_signal(&(Gate.condition[PASSEGGERO]));
		}
	}
	pthread_mutex_unlock(&(Gate.mutex)); //Exit from Gate monitor.
	pthread_exit(NULL);
}

void* prendiParacadute(void *myId)
{
	pthread_mutex_lock(&(Pista.mutex)); //Enter in Track monitor.
	if (Pista.contatori[PARACADUTE] == 0) //If there are no parachutes, get in queue.
	{
		pthread_cond_wait(&(Pista.condition[PARACADUTE]),&(Pista.mutex));
	}
	Pista.contatori[PARACADUTE]--; //Take a parachute.
	pthread_mutex_unlock(&(Pista.mutex)); //Exit from Track monitor.
}

void* saliFandango(void *myId)
{
	pthread_mutex_lock(&(Pista.mutex)); //Enter in Track monitor.
	while (Pista.contatori[SALITI] == P) //For all the time Fandango is full.
	{
		Pista.contatori[ATTESA]++; //Put on hold.
		pthread_cond_wait(&(Pista.condition[PASSEGGERO]),&(Pista.mutex)); //Wait.
		Pista.contatori[ATTESA]--; //Exit from queue and take a seat (I have a parachute and then a secure seat on the next fly).
	}
	Pista.contatori[SALITI]++; //Board on Fandango
	if (Pista.contatori[ATTESA] == 0 || Pista.contatori[SALITI] == P) //If i'm the last on queue or if Fandango is full...
	{
		pthread_cond_signal(&(Pista.condition[MEZZO])); //...tell the pilot to leave.
	}
	pthread_mutex_unlock(&(Pista.mutex)); //Exit from Track monitor.
}

void* raccogliParacadute(void *arg)
{
	int i,da_raccogliere;
	pthread_mutex_lock(&(Pista.mutex)); //Enter in Track monitor.
	while (gente_lanciata < N)
	{
		da_raccogliere=P-Pista.contatori[PARACADUTE]; //I have to collect these parachutes.
		Pista.contatori[PARACADUTE]+=da_raccogliere; //Collected.
		for (i = 0; i < da_raccogliere; i++) //Distribute them to those who wait.
		{
			pthread_cond_signal(&(Pista.condition[PARACADUTE]));
		}
		pthread_cond_wait(&(Pista.condition[INSERVIENTE]), &(Pista.mutex)); //I start waiting for another job.
	}
	pthread_mutex_unlock(&(Pista.mutex)); //Exit from Track monitor.
	pthread_exit(NULL);
}
void* volaFandango(void *arg)
{
	int i;
    pthread_mutex_lock(&(Pista.mutex)); //Enter in Track monitor.
	while (gente_lanciata < N)
	{
		if (Pista.contatori[SALITI]==0) //If no one is on board...
            pthread_cond_wait(&(Pista.condition[MEZZO]),&(Pista.mutex)); //...put on hold.
		gente_lanciata = gente_lanciata + Pista.contatori[SALITI]; //Drop people.
		if (DEBUG)
			{
                printf("Fandango has dropped %3d parachutists | Left: %d \n",Pista.contatori[SALITI],N - gente_lanciata);
                sleep(rand()%2); //Fly time.
			}
		Pista.contatori[SALITI] = 0; //Free seats.
		for (i = 0; i < P; i++)
		{
			pthread_cond_signal(&(Pista.condition[PASSEGGERO])); //Free P people from queue.
		}
		pthread_cond_signal(&(Pista.condition[INSERVIENTE])); //Say to the janitor to collect the parachutes on ground.
	}
	pthread_mutex_unlock(&(Pista.mutex)); //Exit from Track monitor.
	pthread_exit(NULL);
}
void* passeggero(void *myId)
{
	saliNavetta(myId); //Board the shuttle.
	prendiParacadute(myId); //Take the parachute..
	saliFandango(myId); //Board Fandango.
	pthread_exit(NULL); //The parachutes are all defective and the thread dies.
}
int main()
{
	int i = 0, rc = 0;
	void *status;
	// Threads declaration
	pthread_t paracadutisti[N];
	pthread_t navetta;
	pthread_t inserviente;
	pthread_t fandango;
	pthread_attr_t attr;
	// Initialization
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	// Initialization of Gate monitor
	Gate.condition = (pthread_cond_t*)malloc(3*sizeof(pthread_cond_t));
	for(i = 0; i < 3; i++)
	{
		pthread_cond_init(&(Gate.condition[i]), NULL);
	}
	Gate.contatori = (int*)malloc(2*sizeof(int));
	for(i = 0; i < 2; i++)
	{
		Gate.contatori[i] = 0;
	}
	pthread_mutex_init(&(Gate.mutex),NULL);
	// Initialization of Track monitor
	Pista.condition = (pthread_cond_t*)malloc(4*sizeof(pthread_cond_t));
	for(i = 0; i < 4; i++)
	{
		pthread_cond_init(&(Pista.condition[i]), NULL);
	}
	Pista.contatori = (int*)malloc(3*sizeof(int));
	for(i = 0; i < 3; i++)
	{
		Pista.contatori[i] = 0;
	}
	Pista.contatori[PARACADUTE]=P; //Initially, the parachutes are P.
	pthread_mutex_init(&(Pista.mutex),NULL);
	//Create janitor.
	rc = pthread_create(&inserviente, &attr, raccogliParacadute, NULL);
	if(rc)
        printf("Janitor ERROR: code error %d\n", rc);
    //Create shuttle.
	rc=0;
	rc = pthread_create(&navetta, &attr, partiNavetta, NULL);
	if(rc)
        printf("Shuttle ERROR: code error %d\n", rc);
    //Create Fandango.
	rc = pthread_create(&fandango, &attr, volaFandango, NULL);
	if(rc)
        printf("Fandango ERROR: code error %d\n", rc);
	//Create N parachutists threads
	for(i=0;i<N;i++)
	{
		rc = pthread_create(&paracadutisti[i], &attr, passeggero, (void*) i);
		if(rc)
			printf("Parachutist %d ERROR: code error %d\n", i, rc);
	}
	//Wait the join with ended threads.
	for (i = 0; i < N; i++)
	{
		pthread_join(paracadutisti[i], &status);
	}
	pthread_join(navetta, &status);
	pthread_join(inserviente, &status);
	pthread_join(fandango, &status);
	return 0;
}