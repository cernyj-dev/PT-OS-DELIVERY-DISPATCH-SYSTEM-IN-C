#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char line[256];
    char origin[32];
    char destination[32];
    float weight;

    // Loop continues as long as stdin is open. 
    // When central closes its write-end of the pipe, fgets returns NULL (EOF).
    while (fgets(line, sizeof(line), stdin) != NULL) {
        
        // Parse the input strictly as <origin> <destination> <weight>
        if (sscanf(line, "%31s %31s %f", origin, destination, &weight) == 3) {
            
            // Dummy business logic for testing:
            // Calculate a deterministic distance and price based on the input lengths and weight
            float distance = (strlen(origin) + strlen(destination)) * 1.2f;
            float price = (distance * 0.8f) + (weight * 1.5f);

            // Output must be exactly <distance_km> <value_euros> followed by a newline
            printf("%.1f %.2f\n", distance, price);
            
            // CRITICAL: You must flush stdout. 
            // Because stdout is connected to a pipe, the C library fully buffers it by default.
            // If you don't flush, central will hang forever waiting for the data!
            fflush(stdout); 
        } else {
            // Failsafe for badly formatted input from central
            printf("0.0 0.0\n");
            fflush(stdout);
        }
    }

    // Exits cleanly when central shuts down the pipe
    return 0;
}