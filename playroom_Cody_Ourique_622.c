/*
 ============================================================================
 Name        : Cody Ourique
 Author      : 
 Version     : 1.0
 Copyright   : Not Yet, Maybe in the future?
 Description : This program demonstrates the utilization of semaphores and mutex locks by
 excluding boys and girls from being in the same play room.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/* the maximum time (in seconds) to eat and play */
#define MAX_EAT_TIME 3
#define MAX_PLAY_TIME 2
/* number of boys, girls */
#define NUM_OF_BOYS 3
#define NUM_OF_GIRLS 4
/* # of games each boy or girl must play before terminate */
#define NUM_OF_GAMES 2
/* playroom cap */
#define MAX_ROOM_CAP 2
/* semaphores and mutex lock */
pthread_mutex_t boys_mutex, girls_mutex;
/* the number of waiting boys, girls */
int boys, girls;
/* counting semaphore - boys, girls waiting to enter playroom */
sem_t boys_q, girls_q;
/* binary semaphore - playroom */
sem_t room_mutex;

void* boysPlayRoom(void* args);
void* girlsPlayRoom(void* args);

int main() {

	printf("%s","CS149 PlayRoom from Cody Ourique.\n");

	//initialize semaphores and mutex then check for errors
	int rv = sem_init(&room_mutex, 0, 1);
	if (rv != 0)
		printf("Error initializing semaphore, error number %d", rv);

	rv = sem_init(&boys_q, 0, MAX_ROOM_CAP);
	if (rv != 0)
		printf("Error initializing semaphore, error number %d", rv);

	rv = sem_init(&girls_q, 0, MAX_ROOM_CAP);
	if (rv != 0)
		printf("Error initializing semaphore, error number %d", rv);

	rv = pthread_mutex_init(&girls_mutex, NULL);
	if (rv != 0)
		printf("Error initializing semaphore, error number %d", rv);

	rv = pthread_mutex_init(&boys_mutex, NULL);
	if (rv != 0)
		printf("Error initializing semaphore, error number %d", rv);


	//Create 2 array of thread id for boys and girls
	pthread_t bTid[NUM_OF_BOYS];
	pthread_t gTid[NUM_OF_GIRLS];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	int bCount[NUM_OF_BOYS];
	int gCount[NUM_OF_GIRLS];
	int errnum = errno;

	//Create all boy threads
	for (int i = 0; i < NUM_OF_BOYS; i++)
	{

		if(pthread_create(&bTid[i], &attr, boysPlayRoom,  (void *)(intptr_t)i) < 0)
			fprintf(stderr, "pthread_create(boysPlayRoom): %s\n", strerror(errnum));
	}

	//Create all girl threads
	for (int k = 0; k < NUM_OF_GIRLS; k++)
	{

			if(pthread_create(&gTid[k], &attr, girlsPlayRoom, (void *)(intptr_t)k) < 0)
				fprintf(stderr, "pthread_create(girlsPlayRoom): %s\n", strerror(errnum));
	}


	//Join all boy threads
	for (int i = 0; i < NUM_OF_BOYS; i++)
	{
			 rv = pthread_join(bTid[i], NULL);

			 if (rv != 0)
				 printf("Join Error: thread number %d, error number %d \n", i, rv);

	}

	//Join all girl threads
	for (int j = 0; j < NUM_OF_GIRLS; j++)
	{
		    rv = pthread_join(gTid[j],NULL);

		    if (rv != 0)
		    	printf("Join Error: thread number %d, error number %d \n", j, rv);
	}


	//Destroy all semaphores, mutex and thread attribute then check for errors
	rv = sem_destroy(&girls_q);
	if (rv != 0)
		printf("Error destroying semaphore girls_q, error number %d", rv);


	rv = sem_destroy(&boys_q);
	if (rv != 0)
		printf("Error destroying semaphore boys_q, error number %d", rv);

	rv = sem_destroy(&room_mutex);
	if (rv != 0)
			printf("Error destroying semaphore room_mutex, error number %d", rv);

	rv = pthread_attr_destroy(&attr);
	if (rv != 0)
		printf("Error destroying attribute attr, error number %d", rv);

	rv = pthread_mutex_destroy(&girls_mutex);
	if (rv != 0)
		printf("Error destroying girls_mutex, error number %d", rv);

	rv = pthread_mutex_destroy(&boys_mutex);
	if (rv != 0)
		printf("Error destroying boys_mutex, error number %d", rv);


	return 0;
}

void* boysPlayRoom(void* args)
{
	int count = (intptr_t)args;
	int numOfGames = 0;
	int seed = count;
	int eat;
	int game;
	int rv;

	while(numOfGames != NUM_OF_GAMES)
	{
		//generate a random eat time that is <= 3
		eat = (rand_r(&seed)% MAX_EAT_TIME)+1;
		printf("boy[%d,%d]: eat for %d seconds \n", count, numOfGames, eat);

		fflush(NULL);

		//Boy thread sleeps for random time simulating eating
		sleep(eat);

		//Boy thread then acquires boys_mutex to increase the boys variable and demonstrate
		//the number of boys waiting at the door of the play room
		rv = pthread_mutex_lock(&boys_mutex);
		if(rv != 0)
			printf("Error locking unsuccessful, error number %d", rv);

		boys = boys +1;
		printf("boy[%d, %d]: arrive, boys (including me) = %d \n", count, numOfGames, boys);

		fflush(NULL);


		//If there is one boy waiting to get inside the play room, he waits until the
		//room is no longer occupied by girls
		if (boys == 1)
			sem_wait(&room_mutex);



		//Unlock the boys mutex because there is no more read/write to boys variable
		rv = pthread_mutex_unlock(&boys_mutex);
		if(rv != 0)
			printf("Error unlocking unsuccessful, error number %d", rv);


		//An arbitrary boy waits for the semaphore to gain access to the room
		sem_wait(&boys_q);

		numOfGames = numOfGames + 1;

		//random play time  <= 2
		game = (rand_r(&seed)% MAX_PLAY_TIME)+1;
		printf("boy[%d,%d]: play for %d second \n", count, numOfGames, game);

		fflush(NULL);

		sleep(game);

		//Signal that a boy has left the room
		sem_post(&boys_q);

		//Lock the boys mutex to protect and decrement the boy variable
		rv = pthread_mutex_lock(&boys_mutex);
		if(rv != 0)
			printf("Error locking unsuccessful, error number %d", rv);

		boys = boys -1;
		printf("boy[%d, %d]: depart, boys (excluding me) = %d \n", count, numOfGames, boys);
		fflush(NULL);

		//If the last boy has left the room we signal that the room is empty and can be acquired by
		//either girls or boys now.
		if(boys == 0)
			sem_post(&room_mutex);

		//Unlock the boys mutex because there is no more read/write done to boys variable
		rv = pthread_mutex_unlock(&boys_mutex);
		if(rv != 0)
			printf("Error unlocking unsuccessful, error number %d", rv);


	}

	pthread_exit(0);
}

void* girlsPlayRoom(void* args)
{
	int count = (intptr_t)args;
	int numOfGames = 0;
    int seed = count;
	int eat;
	int game;
	int rv;

	while(numOfGames != NUM_OF_GAMES)
	{
		eat = (rand_r(&seed)% MAX_EAT_TIME)+1;
		printf("girl[%d,%d]: eat for %d seconds \n", count, numOfGames, eat);

		fflush(NULL);

		sleep(eat);

		//Girl thread then acquires girls_mutex to increase the girls variable and demonstrate
		//the number of girls waiting at the door of the play room
		rv = pthread_mutex_lock(&girls_mutex);
		if(rv != 0)
			printf("Error locking unsuccessful, error number %d", rv);


		girls = girls +1;
		printf("girl[%d, %d]: arrive, girls (including me) = %d \n", count, numOfGames, girls);

		fflush(NULL);

		//If there is one girl waiting to get inside the play room, she waits until the
		//room is no longer occupied by boys
		if (girls == 1)
			sem_wait(&room_mutex);

		//Unlock the girls mutex because there is no more read/write to girls variable
		rv = pthread_mutex_unlock(&girls_mutex);
			if(rv != 0)
				printf("Error unlocking unsuccessful, error number %d", rv);

		//An arbitrary girl waits for the semaphore to gain access to the room
		sem_wait(&girls_q);

		numOfGames = numOfGames + 1;

		//random number of games <= 2
		game = (rand_r(&seed)% MAX_PLAY_TIME)+1;
		printf("girl[%d,%d]: play for %d seconds \n", count, numOfGames, game);

		fflush(NULL);

		sleep(game);

		//Signal that a girl has left the room
		sem_post(&girls_q);

		//Lock the girls mutex to protect and decrement the girl variable
		rv = pthread_mutex_lock(&girls_mutex);
		if(rv != 0)
			printf("Error locking unsuccessful, error number %d", rv);


		girls = girls - 1;
		printf("girl[%d, %d]: depart, girls (excluding me) = %d \n", count, numOfGames, girls);
		fflush(NULL);

		//If the last girl has left the room we signal that the room is empty and can be acquired by
		//either girls or boys now.
		if(girls == 0)
			sem_post(&room_mutex);

		//Unlock the girls mutex because there is no more read/write done to girls variable
		rv = pthread_mutex_unlock(&girls_mutex);
		if(rv != 0)
			printf("Error unlocking unsuccessful, error number %d", rv);


	}

	pthread_exit(0);

}

