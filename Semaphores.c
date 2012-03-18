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
#include <semaphore.h>

#define DEBUG 1 // Do you want a debug output and a realistic simulation (by sleep)? (1 = Yes| 0 = No)
//Note: sleep(rand()%2) means having a very poor probability distribution, there may be instances much worse than others.
#define N 141 //Number of parachutists
#define M 10  //Number of seats on the shuttle
#define P 20  //Number of seats on Fandango

//Semaphores
pthread_mutex_t mutex_navetta;
pthread_mutex_t mutex_fandango;
sem_t posti_fandango;
sem_t paracadute;
sem_t paracadute_da_raccogliere;
//Global variables
int posti_navetta = M;
int attesa_fandango= 0; //People waiting in front of Fandango with a parachute in hand.

void* arrivaPasseggero(void *myId)
{
	int salito = 0;
	while(salito != 1) //Until not boarded, try to get into the shuttle.
	{
		pthread_mutex_lock(&mutex_navetta);
		if(posti_navetta > 0) //If the shuttle has free seats.
		{
			posti_navetta--; //Fill one seat.
			salito = 1; //On board.
			pthread_mutex_unlock(&mutex_navetta);
            return (void*)NULL;
		}
		pthread_mutex_unlock(&mutex_navetta);
	}
}

void* saliFandango(void *myId)
{
	//Book seat on Fandango
	sem_wait(&posti_fandango);
	//Grabs a parachute
	sem_wait(&paracadute);
	pthread_mutex_lock(&mutex_fandango);
	attesa_fandango++; //Since parachutes are P waiting_fandango will never be > p
	pthread_mutex_unlock(&mutex_fandango);

}

void* raccogliParacadute(void* arg)
{
	int paracadute_raccolti=0;
	int i;
	while(paracadute_raccolti<N)
	{
	    sem_wait(&paracadute_da_raccogliere); //wait for Fandango to gives me the P() X times to collect X parachutes.
	    paracadute_raccolti++;
        sem_post(&paracadute); // Free parachute just collected.
	}
	pthread_exit((void*)NULL);
}

void* volaFandango(void* arg)
{
	int i=0;
	int occupanti=0;
	int lanciati=0;
	//The technicians control that Fandango is operational for this day.
	if (DEBUG)
        sleep(1);
	while(lanciati<N)
	{
        pthread_mutex_lock(&mutex_fandango);
		occupanti=attesa_fandango;
		attesa_fandango=0;
		if (occupanti!=0)
		{
			lanciati=lanciati+occupanti;
			if (DEBUG)
			{
                printf("Fandango has dropped %3d parachutists | Left: %d\n",occupanti,N-lanciati);
                sleep(rand()%2); //Fly time.
			}
			//After the flight launch "occupiers" times P() on seats to free and parachutes to collect.
			for (i=0;i<occupanti;i++)
			{
				sem_post(&posti_fandango);
				sem_post(&paracadute_da_raccogliere);
			}
		}
		pthread_mutex_unlock(&mutex_fandango);
	}
	pthread_exit((void*)NULL);
}


void* passeggero(void *myId)
{
	arrivaPasseggero(myId); //Get to the gate and step onto the shuttle.
	saliFandango(myId); //Wait a parachute, takes it and board on Fandango.
	pthread_exit(NULL); //The parachutes are all defective and the thread dies.
}


void* navetta(void *arg)
{
    int scaricati=0; // Parachutists transported by the shuttle.
	while(scaricati<N)
	{
        pthread_mutex_lock(&mutex_navetta);
        if(posti_navetta < M)
        {
            scaricati+=M-posti_navetta;
            if (DEBUG)
            {
                printf("Navetta has dropped %3d passengers | Left: %3d\n", M - posti_navetta , N-scaricati);
                sleep(rand()%2); //Return time.
            }
            posti_navetta = M;
        }
        pthread_mutex_unlock(&mutex_navetta);
	}
	pthread_exit((void*) 0);
}

int main(void)
{
	int i = 0, rc = 0;
	void *status;
	//Threads
    pthread_t paracadutisti[N];
    pthread_t nave;
    pthread_t fandango;
    pthread_t inserviente;
	if (DEBUG)
        srand ( time(NULL) ); // Initializing random seed.
	pthread_attr_t attr;
	//Init mutex.
	pthread_mutex_init(&mutex_navetta, NULL);
	pthread_mutex_init(&mutex_fandango, NULL);
	//Init semaphores.
	sem_init(&posti_fandango,0,P);
	sem_init(&paracadute,0,P);
	sem_init(&paracadute_da_raccogliere,0,0);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    //Create Janitor.
	rc = pthread_create(&inserviente, &attr, raccogliParacadute, NULL);
	if(rc)
        printf("Janitor ERROR: code error %d\n", rc);
    //Create Shuttle.
	rc=0;
	rc = pthread_create(&nave, &attr, navetta, NULL);
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
			printf("Paracadutista %d ERRORE: code error %d\n", i, rc);
	}
	//Wait the join with ended threads.
	for(i=0;i<N;i++)
	{
		pthread_join(paracadutisti[i], &status);
	}
	pthread_join(fandango, &status);
	pthread_join(nave, &status);
	pthread_join(inserviente, &status);
	//Destroy semaphores and mutex.
	pthread_mutex_destroy(&mutex_navetta);
	pthread_mutex_destroy(&mutex_fandango);
	sem_destroy(&posti_fandango);
	sem_destroy(&paracadute);
    //Destroy the attribute.
	pthread_attr_destroy(&attr);
	return 0;
}