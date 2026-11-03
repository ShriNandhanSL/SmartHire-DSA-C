#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NAME 60
#define MAX_SKILLS 10
#define MAX_SKILL_LEN 30
#define HASH_SIZE 101
#define MAX_CANDIDATES 500

typedef struct {
    int id;
    char name[MAX_NAME];
    int experience;
    int skill_count;
    char skills[MAX_SKILLS][MAX_SKILL_LEN];
    double score;
} Candidate;

typedef struct Job {
    int id;
    char title[MAX_NAME];
    int min_experience;
    int skill_count;
    char skills[MAX_SKILLS][MAX_SKILL_LEN];
    struct Job *next;
} Job;

typedef struct HashNode {
    int candidate_index;
    struct HashNode *next;
} HashNode;

static Candidate candidates[MAX_CANDIDATES];
static int candidate_count = 0;
static Job *jobs = NULL;
static int next_candidate_id = 1, next_job_id = 1;
static HashNode *skill_table[HASH_SIZE] = {0};

static void read_line(const char *prompt, char *out, size_t size) {
    printf("%s", prompt);
    if (fgets(out, (int)size, stdin)) {
        out[strcspn(out, "\n")] = '\0';
    } else out[0] = '\0';
}
static int read_int(const char *prompt, int min, int max) {
    char buf[64], *end;
    long value;
    for (;;) {
        read_line(prompt, buf, sizeof buf);
        value = strtol(buf, &end, 10);
        while (isspace((unsigned char)*end)) end++;
        if (end != buf && *end == '\0' && value >= min && value <= max)
            return (int)value;
        printf("Enter a number from %d to %d.\n", min, max);
    }
}
static void lower_copy(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; src[i] && i + 1 < n; i++)
        dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = '\0';
}
static unsigned hash_skill(const char *skill) {
    char key[MAX_SKILL_LEN];
    unsigned h = 5381;
    lower_copy(key, skill, sizeof key);
    for (int i = 0; key[i]; i++) h = ((h << 5) + h) + (unsigned char)key[i];
    return h % HASH_SIZE;
}
static int has_skill(const Candidate *c, const char *skill) {
    char a[MAX_SKILL_LEN], b[MAX_SKILL_LEN];
    lower_copy(b, skill, sizeof b);
    for (int i = 0; i < c->skill_count; i++) {
        lower_copy(a, c->skills[i], sizeof a);
        if (strcmp(a, b) == 0) return 1;
    }
    return 0;
}
static void free_hash(void) {
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode *n = skill_table[i];
        while (n) { HashNode *next = n->next; free(n); n = next; }
        skill_table[i] = NULL;
    }
}
static void build_hash(void) {
    free_hash();
    for (int i = 0; i < candidate_count; i++) {
        for (int j = 0; j < candidates[i].skill_count; j++) {
            unsigned bucket = hash_skill(candidates[i].skills[j]);
            HashNode *n = malloc(sizeof *n);
            if (!n) { fprintf(stderr, "Memory allocation failed.\n"); exit(EXIT_FAILURE); }
            n->candidate_index = i;
            n->next = skill_table[bucket];
            skill_table[bucket] = n;
        }
    }
}
static void search_skill(void) {
    char skill[MAX_SKILL_LEN];
    read_line("Enter skill to search: ", skill, sizeof skill);
    unsigned bucket = hash_skill(skill);
    int found = 0;
    for (HashNode *n = skill_table[bucket]; n; n = n->next) {
        Candidate *c = &candidates[n->candidate_index];
        if (has_skill(c, skill)) {
            printf("ID: %d | %s | Experience: %d years\n",
                   c->id, c->name, c->experience);
            found = 1;
        }
    }
    if (!found) printf("No candidate found with skill '%s'.\n", skill);
}
static void add_candidate(void) {
    if (candidate_count >= MAX_CANDIDATES) {
        printf("Candidate limit reached.\n"); return;
    }
    Candidate *c = &candidates[candidate_count];
    c->id = next_candidate_id++;
    read_line("Candidate name: ", c->name, sizeof c->name);
    c->experience = read_int("Experience (years): ", 0, 60);
    c->skill_count = read_int("Number of skills (1-10): ", 1, MAX_SKILLS);
    for (int i = 0; i < c->skill_count; i++) {
        char prompt[50];
        snprintf(prompt, sizeof prompt, "Skill %d: ", i + 1);
        read_line(prompt, c->skills[i], sizeof c->skills[i]);
    }
    c->score = 0;
    candidate_count++;
    build_hash();
    printf("Candidate added successfully. ID = %d\n", c->id);
}
static void display_candidates(void) {
    if (!candidate_count) { printf("No candidates available.\n"); return; }
    for (int i = 0; i < candidate_count; i++) {
        Candidate *c = &candidates[i];
        printf("\nID: %d\nName: %s\nExperience: %d years\nSkills: ",
               c->id, c->name, c->experience);
        for (int j = 0; j < c->skill_count; j++)
            printf("%s%s", c->skills[j], j + 1 == c->skill_count ? "" : ", ");
        printf("\n");
    }
}
static void add_job(void) {
    Job *j = calloc(1, sizeof *j);
    if (!j) { printf("Memory allocation failed.\n"); return; }
    j->id = next_job_id++;
    read_line("Job title: ", j->title, sizeof j->title);
    j->min_experience = read_int("Minimum experience (years): ", 0, 60);
    j->skill_count = read_int("Number of required skills (1-10): ", 1, MAX_SKILLS);
    for (int i = 0; i < j->skill_count; i++) {
        char prompt[50];
        snprintf(prompt, sizeof prompt, "Required skill %d: ", i + 1);
        read_line(prompt, j->skills[i], sizeof j->skills[i]);
    }
    j->next = jobs;
    jobs = j;
    printf("Job added successfully. ID = %d\n", j->id);
}
static Job *find_job(int id) {
    for (Job *j = jobs; j; j = j->next) if (j->id == id) return j;
    return NULL;
}
static void display_jobs(void) {
    if (!jobs) { printf("No jobs available.\n"); return; }
    for (Job *j = jobs; j; j = j->next) {
        printf("\nID: %d | Title: %s | Minimum experience: %d years\nRequired skills: ",
               j->id, j->title, j->min_experience);
        for (int i = 0; i < j->skill_count; i++)
            printf("%s%s", j->skills[i], i + 1 == j->skill_count ? "" : ", ");
        printf("\n");
    }
}
static double calculate_score(const Candidate *c, const Job *j) {
    int matched = 0;
    for (int i = 0; i < j->skill_count; i++)
        if (has_skill(c, j->skills[i])) matched++;
    /* Skills contribute 80%; experience contributes up to 20%. */
    double skill_score = 80.0 * matched / j->skill_count;
    double exp_score = j->min_experience == 0 ? 20.0 :
        (c->experience >= j->min_experience ? 20.0 :
         20.0 * c->experience / j->min_experience);
    return skill_score + exp_score;
}
static void merge(Candidate a[], int left, int mid, int right) {
    int n = right - left + 1;
    Candidate *tmp = malloc((size_t)n * sizeof *tmp);
    if (!tmp) { fprintf(stderr, "Memory allocation failed.\n"); exit(EXIT_FAILURE); }
    int i = left, j = mid + 1, k = 0;
    while (i <= mid && j <= right)
        tmp[k++] = (a[i].score >= a[j].score) ? a[i++] : a[j++];
    while (i <= mid) tmp[k++] = a[i++];
    while (j <= right) tmp[k++] = a[j++];
    for (i = 0; i < n; i++) a[left + i] = tmp[i];
    free(tmp);
}
static void merge_sort(Candidate a[], int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    merge_sort(a, left, mid);
    merge_sort(a, mid + 1, right);
    merge(a, left, mid, right);
}
static void match_and_rank(void) {
    if (!candidate_count || !jobs) {
        printf("Add at least one candidate and one job first.\n"); return;
    }
    display_jobs();
    int id = read_int("Enter job ID to match: ", 1, 1000000);
    Job *j = find_job(id);
    if (!j) { printf("Invalid job ID.\n"); return; }
    for (int i = 0; i < candidate_count; i++)
        candidates[i].score = calculate_score(&candidates[i], j);
    merge_sort(candidates, 0, candidate_count - 1);
    printf("\nCandidate ranking for: %s\n", j->title);
    printf("--------------------------------------------------\n");
    for (int i = 0; i < candidate_count; i++) {
        Candidate *c = &candidates[i];
        printf("%d. %-20s Score: %5.1f%% | Experience: %d years\n",
               i + 1, c->name, c->score, c->experience);
    }
}
static void save_candidates(void) {
    FILE *fp = fopen("candidates.csv", "w");
    if (!fp) { perror("candidates.csv"); return; }
    fprintf(fp, "id,name,experience,skills\n");
    for (int i = 0; i < candidate_count; i++) {
        Candidate *c = &candidates[i];
        fprintf(fp, "%d,%s,%d,", c->id, c->name, c->experience);
        for (int j = 0; j < c->skill_count; j++)
            fprintf(fp, "%s%s", c->skills[j], j + 1 == c->skill_count ? "" : "|");
        fputc('\n', fp);
    }
    fclose(fp);
    printf("Candidates saved to candidates.csv\n");
}
static void load_candidates(void) {
    FILE *fp = fopen("candidates.csv", "r");
    if (!fp) { printf("No saved candidate file found yet.\n"); return; }
    char line[1024];
    fgets(line, sizeof line, fp); /* skip header */
    candidate_count = 0;
    while (candidate_count < MAX_CANDIDATES && fgets(line, sizeof line, fp)) {
        Candidate c = {0};
        char *id = strtok(line, ",");
        char *name = strtok(NULL, ",");
        char *exp = strtok(NULL, ",");
        char *skills = strtok(NULL, "\n");
        if (!id || !name || !exp || !skills) continue;
        c.id = atoi(id);
        snprintf(c.name, sizeof c.name, "%s", name);
        c.experience = atoi(exp);
        char *token = strtok(skills, "|");
        while (token && c.skill_count < MAX_SKILLS) {
            snprintf(c.skills[c.skill_count++], MAX_SKILL_LEN, "%s", token);
            token = strtok(NULL, "|");
        }
        if (c.skill_count) {
            candidates[candidate_count++] = c;
            if (c.id >= next_candidate_id) next_candidate_id = c.id + 1;
        }
    }
    fclose(fp);
    build_hash();
    printf("Loaded %d candidate(s).\n", candidate_count);
}
static void free_jobs(void) {
    while (jobs) { Job *next = jobs->next; free(jobs); jobs = next; }
}
int main(void) {
    load_candidates();
    for (;;) {
        printf("\n========== SMART HIRE ==========\n");
        printf("1. Add candidate\n");
        printf("2. Display candidates\n");
        printf("3. Add job\n");
        printf("4. Display jobs\n");
        printf("5. Search candidates by skill (Hash Table)\n");
        printf("6. Match and rank candidates (Merge Sort)\n");
        printf("7. Save candidates\n");
        printf("0. Exit\n");
        int choice = read_int("Enter choice: ", 0, 7);
        switch (choice) {
            case 1: add_candidate(); break;
            case 2: display_candidates(); break;
            case 3: add_job(); break;
            case 4: display_jobs(); break;
            case 5: search_skill(); break;
            case 6: match_and_rank(); break;
            case 7: save_candidates(); break;
            case 0:
                save_candidates();
                free_hash();
                free_jobs();
                printf("Thank you for using SmartHire.\n");
                return 0;
        }
    }
}