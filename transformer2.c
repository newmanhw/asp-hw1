#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#define BUFFER_SIZE 1024
#define LINE_SIZE 512

struct Agent {
    char name[11];
    char id[6];  
    double max_adjusted_gain;
    int last_seen;
};

struct State {
    char stateID[3];
    double max_adjusted_gain;
    int last_seen;
};

// compares via absolute value (fabs)
double sign_adjusted_max(double current, double candidate) {
    if (fabs(candidate) > fabs(current)) {
        return candidate;
    }
    return current;
}

/* Helper that puts an integer into a growing buffer */
static void append_int(char **buf, size_t *len, int value, const char *format)
{
    /* Estimate the max chars needed for this chunk */
    size_t needed = strlen(format) + 20;          /* plenty of room */
    *buf = realloc(*buf, *len + needed);
    int written = snprintf(*buf + *len, needed, format, value);
    *len += written;
}

/* Return a new string containing n formatted with commas.  Caller must free() */
char *printfcomma_str(int n)
{
    char *out = NULL;
    size_t outlen = 0;
    int n2   = 0;
    int scale = 1;

    if (n < 0) {
        append_int(&out, &outlen, '-', "%c");
        n = -n;
    }

    /* Separate the number into 3‑digit groups */
    while (n >= 1000) {
        n2   += scale * (n % 1000);
        n /= 1000;
        scale *= 1000;
    }

    append_int(&out, &outlen, n, "%d");         

    while (scale != 1) {
        scale /= 1000;
        int group = n2 / scale;
        n2       %= scale;
        append_int(&out, &outlen, group, ",%03d");
    }

    return out; /* remember to free() the caller */
}

char *format_gain(double value)
{
    int is_negative = value < 0;
    int is_zero = (value == 0.0);

    value = fabs(value);

    long long int_part = (long long)value;
    double frac = value - int_part;

    char *int_str = printfcomma_str((int)int_part);

    char frac_str[8];
    snprintf(frac_str, sizeof(frac_str), "%.2f", frac);
    // e.g. "0.25" → ".25"

    size_t len = strlen(int_str) + strlen(frac_str) + 3;
    char *out = malloc(len);

    if (is_zero) {
        snprintf(out, len, "%s%s",
                 int_str,
                 frac_str + 1);
    } else {
        snprintf(out, len, "%s%s%s",
                 is_negative ? "-" : "+",
                 int_str,
                 frac_str + 1);
    }

    free(int_str);
    return out;
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

void process_line(char *line, struct Agent agents[], int *num_agents, struct State states[], int *num_states, int trans_counter) {
    char *fields[6]; 
    split_line(line, fields);

    char *agentName = fields[0];
    char *agentID   = fields[1];
    char *transID   = fields[2];
    char *stateID   = fields[3];
    char *salePrice = fields[4];
    char *gain      = fields[5];
    
    
    removeCharacter(gain, ',');
    double gain_formatted = strtod(gain, NULL);

    // create/modify unique agent
    int itr;
    for (itr = 0; itr < *num_agents; itr++) {
        if (strcmp(agents[itr].id, agentID) == 0) break;
    }

    if (itr < *num_agents) {
        // existing agent
        agents[itr].max_adjusted_gain = sign_adjusted_max(agents[itr].max_adjusted_gain, gain_formatted);
        agents[itr].last_seen = trans_counter;
    } else { // new agent creation
        strcpy(agents[*num_agents].name, agentName);
        strcpy(agents[*num_agents].id, agentID);
        agents[*num_agents].max_adjusted_gain = gain_formatted;
        agents[itr].last_seen = trans_counter;
        (*num_agents)++;
    }

    // create/modify unique state
    int itr2;
    for (itr2 = 0; itr2 < *num_states; itr2++) {
        if (strcmp(states[itr2].stateID, stateID) == 0) break;
    }

    if (itr2 < *num_states) {
        // existing state
        states[itr2].max_adjusted_gain = sign_adjusted_max(states[itr2].max_adjusted_gain, gain_formatted);
        states[itr2].last_seen = trans_counter;
    } else { // new state creation
        strcpy(states[*num_states].stateID, stateID);
        states[*num_states].max_adjusted_gain = gain_formatted;
        (*num_states)++;
        states[itr2].last_seen = trans_counter;
    }
}

int main() {
    // File descriptor 0: stdin
    // stdin automatically opened when process starts
    char inbuf[BUFFER_SIZE];
    char line[LINE_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;

    // keep track of transactions
    int trans_counter = 0;

    // Agent array information
    struct Agent agents[10]; // shouldn't be too many agents
    int num_agents = 0;

    struct State states[10];
    int num_states = 0;

    // Split stdin by line
    while ((bytes_read = read(0, inbuf, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (inbuf[i] == '\n') {
                line[line_pos] = '\0';
                process_line(line, agents, &num_agents, states, &num_states, trans_counter++);
                line_pos = 0;
            } else {
                line[line_pos++] = inbuf[i];
            }
        }
    }

    // qsort(agents, num_agents, sizeof(struct Agent), compare_agents_last_seen);
    //qsort(states, num_states, sizeof(struct State), compare_agents_last_seen);

    // stdout
    // setlocale(LC_NUMERIC, ""); -- can't use because it's non-standard in C17

    for (int i = 0; i < num_agents; i++) {
        char *gain_str = format_gain(agents[i].max_adjusted_gain);

        printf("%s, %s, %s\n",
            agents[i].name,
            agents[i].id,
            gain_str);
        
        free(gain_str);
    }

    // stderr
    for (int i = 0; i < num_states; i++) {
        char *gain_str = format_gain(states[i].max_adjusted_gain);

        fprintf(stderr, "%s, %s\n",states[i].stateID,gain_str);

        free(gain_str);
    }

    return 0;
}