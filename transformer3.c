#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
#include <string.h>
#include <math.h>

#define BUFFER_SIZE 1024
#define LINE_SIZE 512

struct Agent {
    char id[6];      
    double max_customer_rating;
};

struct State {
    char stateID[3];
    double max_customer_rating;
};

// compares via absolute value (fabs)
double sign_adjusted_max(double current, double candidate) {
    if (fabs(candidate) > fabs(current)) {
        return candidate;
    }
    return current;
}

void split_line(char *line, char *fields[]) {
    int cur_field = 0;
    fields[cur_field++] = line;

    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == ' ' && line[i-1] == ',') {
            line[i-1] = '\0'; // replace the comma before whitespace with null terminator
            fields[cur_field++] = &line[i + 1]; // start next line at character after the whitespace
        }
    }
}

// https://www.vervecopilot.com/hot-blogs/strip-string-in-c-interviews#:~:text=Can%20you%20see%20a%20concise%20example%20of,manipulation%2C%20correct%20null%2Dtermination%2C%20and%20efficient%20one%2Dpass%20behavior.
void removeCharacter(char* str, char charToRemove) {
    char *src, *dst;
    for (src = dst = str; *src; src++) {
        if (*src != charToRemove) {
            *dst++ = *src;
        }
    }
    *dst = '\0'; // Null-terminate the new string
}

void process_line(char *line, struct Agent agents[], int *num_agents, struct State states[], int *num_states) {
    char *fields[3]; 
    split_line(line, fields);

    char *agentID   = fields[0];
    char *stateID   = fields[1];
    char *custRate  = fields[2];
    
    
    double rating_formatted = strtod(custRate, NULL);

    // create/modify unique agent
    int itr;
    for (itr = 0; itr < *num_agents; itr++) {
        if (strcmp(agents[itr].id, agentID) == 0) break;
    }

    if (itr < *num_agents) {
        // existing agent
        agents[itr].max_customer_rating = sign_adjusted_max(agents[itr].max_customer_rating, rating_formatted);
    } else { // new agent creation
        strcpy(agents[*num_agents].id, agentID);
        agents[*num_agents].max_customer_rating = rating_formatted;
        (*num_agents)++;
    }

    // create/modify unique state
    int itr2;
    for (itr2 = 0; itr2 < *num_states; itr2++) {
        if (strcmp(states[itr2].stateID, stateID) == 0) break;
    }

    if (itr2 < *num_states) {
        // existing state
        states[itr2].max_customer_rating = sign_adjusted_max(states[itr2].max_customer_rating, rating_formatted);
    } else { // new state creation
        strcpy(states[*num_states].stateID, stateID);
        states[*num_states].max_customer_rating = rating_formatted;
        (*num_states)++;
    }
}

int main() {
    // File descriptor 0: stdin
    // stdin automatically opened when process starts
    char inbuf[BUFFER_SIZE];
    char line[LINE_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;

    // Agent array information
    struct Agent agents[10]; // shouldn't be too many agents
    int num_agents = 0;

    // State array information
    struct State states[10]; // shouldn't be too many agents
    int num_states = 0;

    // Split stdin by line
    while ((bytes_read = read(0, inbuf, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (inbuf[i] == '\n') {
                line[line_pos] = '\0';
                process_line(line, agents, &num_agents, states, &num_states);
                line_pos = 0;
            } else {
                line[line_pos++] = inbuf[i];
            }
        }
    }

    // stdout
    setlocale(LC_NUMERIC, "");

    for (int i = 0; i < num_agents; i++) {
        printf("%s, %.1f\n",
            agents[i].id,
            agents[i].max_customer_rating
            );
    }

    // stderr
    for (int i = 0; i < num_states; i++) {
        fprintf(stderr, "%s, %.1f\n",
                states[i].stateID,
                states[i].max_customer_rating);
    }
    
    return 0;
}

