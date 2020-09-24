#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define SEED time(0)
#define D 120 // exam time
#define R 3 // rows
#define C 10 // columns
#define N 30 /* Number of total students */
#define M 5 /* Number of cunning students */
#define T 3 /* Number of total teachers */
#define K 5 // cunning students' random decision time
#define L 10 // cheating time
#define W 5 // teachers' table changing time
#define V 10 // file write interval
using namespace std;
pthread_mutex_t s_locks[R+1][C+1];
pthread_mutex_t t_locks[R+1][C+1];
pthread_mutex_t file_lock;
bool cunning[N+1] = {false};
bool s_running[N+1] = {false};
bool t_running[N+1] = {false};
int cheating_from[N+1] = {0};
int student_table[R+1][C+1] = {0};
int teacher_table[R+1][C+1] = {0};
pair<int,int> student_locations[N+1];
pair<int,int> teacher_locations[T+1];
//left, top, right
int sx_dir[3] = {-1, 0, 1};
int sy_dir[3] = {0, 1, 0};
//left, top-left, top, top-right, right, bottom-right, bottom, bottom-left
int tx_dir[8] = {-1,-1,0,1,1,1,0,-1};
int ty_dir[8] = {0,1,1,1,0,-1,-1,-1};

void init();
void allocate_student(int ID);
void allocate_teacher(int ID);
bool validate(int i, int j);
int apprehend(int sID);
void * Teacher(void * ID);
void * Student(void * ID);

int main() {
    srand(SEED);
    pthread_t students[N], teachers[T];
    int rc;
    int i;
    int start, end;
    double elapsed;
    init();
    for (int i=0,j=0; i < N; i++)
    {
        int ran = rand()%10;
        if (ran>=5 && j!=M)
        {
            cunning[i+1] = true;
            j++;
        }
        else cunning[i+1] = false;
    }
    start = clock();
    //Creating students
    for(int i = 0; i < N; i++ ) {
        allocate_student(i+1);
        s_running[i+1] = true;
        rc = pthread_create(&students[i], NULL,Student, (void *)(i+1));

        if (rc) {
            cout << "Error:unable to create thread," << rc << endl;
            exit(-1);
        }
    }

    //Creating teachers
    for( i = 0; i < T; i++ ) {
        allocate_teacher(i+1);
        t_running[i+1] = true;
        rc = pthread_create(&teachers[i], NULL,Teacher, (void *)(i+1));

        if (rc) {
            cout << "Error:unable to create thread," << rc << endl;
            exit(-1);
        }
    }
    end=clock();
    elapsed = ((double)(end-start))/CLOCKS_PER_SEC;
    while (elapsed<=D)
    {
        end = clock();
        elapsed = ((double)(end-start))/CLOCKS_PER_SEC;
    }
    for (int i=0; i < N; i++)
    {
        s_running[i+1] = false;
    }
    for (int i=0; i < T; i++)
    {
        s_running[i+1] = false;
    }
    return 0;
}

void init()
{
    fill(student_locations, student_locations+100,make_pair(0,0));
    fill(teacher_locations, teacher_locations+100,make_pair(0,0));
}
void allocate_student(int ID)
{
    int r, c;
    do {
        r = rand() % R + 1;
        c = rand() % C + 1;
        if (!student_table[r][c]) {
            student_table[r][c] = ID;
            student_locations[ID].first = r;
            student_locations[ID].second = c;
        }
    }
    while (!student_locations[ID].first);
}

void allocate_teacher(int ID)
{
    int r, c;
    do {
        r = rand() % R + 1;
        c = rand() % C + 1;
        if (!teacher_table[r][c]) {
            teacher_table[r][c] = ID;
            teacher_locations[ID].first = r;
            teacher_locations[ID].second = c;
        }
    }
    while (!teacher_locations[ID].first);
}

bool validate(int i, int j)
{
    if ((i>=1 && i<=R) && (j>=1 && j<=C))
    {
        return true;
    }
    return false;
}

int apprehend(int sID)
{
    int row, col;
    int isExpelled = 0;
    row = student_locations[sID].first;
    col = student_locations[sID].second;
    pthread_mutex_lock(&s_locks[row][col]);
    isExpelled = cheating_from[sID];
    pthread_mutex_unlock(&s_locks[row][col]);
    return isExpelled;
}

void * Teacher(void * ID){
    long tID = long(ID);
    while(t_running[tID])
    {
        int sID=student_table[teacher_locations[tID].first][teacher_locations[tID].second];
        int dirIdx;
        int cheat_source;
        cheat_source = apprehend(sID);
        if (cheat_source)
        {
            pthread_mutex_lock(&file_lock);
            FILE *fp;
            fp = fopen("hall_of_shame.txt", "w+");
            fprintf(fp, "Teacher %d expelled Student %d for copying from Student %d\n",tID,sID,cheat_source);
            //cout << "Teacher " << tID << " expelled Student " << sID <<
            //" for copying from Student"<< cheat_source <<endl;
            fclose(fp);
            pthread_mutex_unlock(&file_lock);
            s_running[sID]=false;
        }
        sleep(W);
        pthread_mutex_lock(&t_locks[teacher_locations[tID].first][teacher_locations[tID].second]);
        teacher_table[teacher_locations[tID].first][teacher_locations[tID].second] = 0;
        pthread_mutex_unlock(&t_locks[teacher_locations[tID].first][teacher_locations[tID].second]);

        dirIdx = rand()%8;
        int newR = teacher_locations[tID].first+ty_dir[dirIdx];
        int newC = teacher_locations[tID].second+tx_dir[dirIdx];
        while(validate(newR,newC))
        {
            //check if index is free and move there
            pthread_mutex_lock(&t_locks[newR][newC]);
            if (teacher_table[newR][newC] == 0){
                teacher_table[newR][newC] = tID;
                cout << "Teacher " <<tID <<" moved from row " <<teacher_locations[tID].first
                     << ", column " << teacher_locations[tID].second << "to row " <<newR
                     << ", column " <<newC<<endl;
                teacher_locations[tID].first = newR;
                teacher_locations[tID].second = newC;
                pthread_mutex_unlock(&t_locks[newR][newC]);
                break;
            }
            else
            {
                pthread_mutex_lock(&t_locks[newR][newC]);
            }
            dirIdx = rand()%8;
            newR = teacher_locations[tID].first+ty_dir[dirIdx];
            newC = teacher_locations[tID].second+tx_dir[dirIdx];
        }
    }
    cout << "Teacher: " <<tID << " exiting hall." <<endl;
    pthread_exit(NULL);
}
void * Student(void * ID)
{
    long sID = long(ID);
    if (cunning[sID])
    {
        cout << "Naughty student, ID: " << sID <<" starting exam"<<endl;
        while (s_running[sID])
        {
            int decision = rand()%2;
            if (decision==0)
            {
                int oldR, oldC;
                int newR, newC;
                int dirIdx = rand()%3;
                oldR = student_locations[sID].first;
                oldC = student_locations[sID].second;
                newR = student_locations[sID].first+sy_dir[dirIdx];
                newC = student_locations[sID].second+sx_dir[dirIdx];
                while(!validate(newR,newC))
                {
                    dirIdx = rand()%3;
                    newR = student_locations[sID].first+sy_dir[dirIdx];
                    newC = student_locations[sID].second+sx_dir[dirIdx];
                }
                pthread_mutex_lock(&s_locks[oldR][oldC]);
                cheating_from[sID] = student_table[newR][newC];
                pthread_mutex_unlock(&s_locks[oldR][oldC]);
                printf("Naughty student, ID: %d (r: %d, c:%d) is "
                       "cheating from student ID: %d (r: %d, c: %d)", sID, oldR,
                       oldC, student_table[newR][newC], newR, newC);
                sleep(L);
                pthread_mutex_lock(&s_locks[oldR][oldC]);
                cheating_from[sID] = 0;
                pthread_mutex_unlock(&s_locks[oldR][oldC]);
                printf("Naughty student, ID: %d (r: %d, c:%d) is"
                       " done cheating", sID, oldR,
                       oldC);
            }
            cout << "Naughty student, ID: " << sID <<" is resting"<<endl;
            sleep(K);
        }
    }
    else {
        while (s_running[sID]) {
            cout << "Good student, ID: " << sID << " starting exam" << endl;
            sleep(K);         }
    }
    cout << "Student: " <<sID << " ending exam." <<endl;
    pthread_exit(NULL);
}