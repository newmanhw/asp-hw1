#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define LINE_SIZE 512

struct Agent {
    char id[6];      
    double customer_rating;
    int houses_sold;
};

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

void process_line(char *line, struct Agent agents[], int *num_agents) {
    char *fields[3]; 
    split_line(line, fields);

    char *agentID   = fields[0];
    char *stateID   = fields[1];
    char *custRate  = fields[2];
    
    
    double rating_formatted = strtod(custRate, NULL);

    int itr;
    for (itr = 0; itr < *num_agents; itr++) {
        if (strcmp(agents[itr].id, agentID) == 0) break;
    }

    if (itr < *num_agents) {
        // existing agent
        agents[itr].customer_rating += rating_formatted; 
        agents[itr].houses_sold++;
    } else {
        // new agent
        strcpy(agents[*num_agents].id, agentID);
        agents[*num_agents].customer_rating = rating_formatted;
        agents[*num_agents].houses_sold = 1;
        (*num_agents)++;
    }

    //double average_gain_loss = gain_formatted;
    double average_rating = agents[itr].customer_rating / agents[itr].houses_sold;
    
    // stdout/err handling
    char out[256];
    int len;
    
    // write to stdout 
    len = snprintf(out, sizeof(out),
        "%s, %.1f\n",
        agentID, average_rating
    );
    write(1, out, len);

    // write to stderr
    len = snprintf(out, sizeof(out),
        "%s, %.1f\n",
        stateID, average_rating
    );
    write(2, out, len);
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

    // Split stdin by line
    while ((bytes_read = read(0, inbuf, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (inbuf[i] == '\n') {
                line[line_pos] = '\0';
                process_line(line, agents, &num_agents);
                line_pos = 0;
            } else {
                line[line_pos++] = inbuf[i];
            }
        }
    }
    
    return 0;
}

