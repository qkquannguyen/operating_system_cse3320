// ------------------------------------------- IMPORTS -------------------------------------------
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>


// ----------------------------------- DEFINE GLOBAL VARIABLES -----------------------------------
#define MAX_COMMAND_SIZE 255


// ---------------------------------------- MAIN FUNCTION ----------------------------------------
int main (int argc, char* argv[]) {

    /* ------------- Define Variables ------------- */
    struct timeval begin;                   // The time the program begins
    struct timeval end;                     // The time the program ends
    char s[128];                            // Array for the S-Value    
    int time_duration;                      // Total Time the program took
    int i, j;                               // I-Loop and J-Loop Iterators
    int numProcess;                         // Number of Processes the user needs
    float sVal = 2;                         // Value of Scale
    float sFactor = (sVal - 0.000015) / 49; // The S-Value we decrement by
    

    // Convert to a ASCII Integers
    numProcess = atoi(argv[1]);
    printf("%d\n", numProcess);

    // Begin a time counter for this program
    gettimeofday(&begin, NULL);
    
    /* ------------ Begin Forking ------------ */
    // I-Loop will iterate at least 50 times
    // J-Loop will start forking 50 times 
    for (i = 0 ; i < 50 ; i += numProcess)
    {
        for (j = 0 ; j < numProcess ; j++)
        {
            // Initialize our Mandel file name array
            // Allocate memory to it and incremenet the file by 1 after each iterations
            char mandelFile[128];
            memset(mandelFile, 0, 128);
            sprintf(mandelFile, "mandel%d.bmp", i+j);

            // This will decrement and display our current S-Value after each iteration
            sprintf(s, "%f", sVal);

            // Forking Begings here. We will add the desired amount of processes entered by user.
            // Call execl() and we will create as many mandel images as the number of processes
            pid_t child_pid = fork();

            // Update the S-Value after every iteration
            sVal = sVal - sFactor;

            if (child_pid == 0)
            {
                // Execute the Mandel command and pass in the desired dimensions of the BMP file
                // the S-Value will be decremented by some factor calculated in this program to
                // achieve the desired end S-Value. 
                // 50 BMP Files should be generated.
                execl("./mandel", "mandel", "-x", "0.286932", "-y", "0.014287", "-s", s,
                      "-W", "1800", "-H", "1800", "-m", "500", "-o", mandelFile, NULL);
                
                // Exit
                return 0;
            }
        }

        // Check to see if the number of times the fork has happened.
        // Any more than the number of process, we should end forking
        int count = 0;
        while (count != numProcess)
        {
            int status;
            wait(&status);
            count++;                        
        }
    }
    
    // End the time counter
    gettimeofday(&end, NULL);

    // Calculate the time the program took in total
    // The time displayed will be displayed in microseconds
    time_duration = ((end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_sec - begin.tv_sec));

    // Display to the user how long the program took
    printf("Duration: %d\n", time_duration);

    // Finish
    return 0;
}
