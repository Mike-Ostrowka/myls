#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

const char OPTION_ERROR []= "Error: Unsupported Option\n";
const char DIR_ERROR [] = "Error : Nonexistent files or directories";

char **paths; //array of path parameters
bool inode = false;
bool longList = false;
bool recursive = false;
bool cwdMode = true; //true when no path provided

bool setOptions = false; //no cmdline options set
bool setPath = false; //no path set
int pathCount = 0; //amount of paths set
int optionCount = 0; //amount of option parms

//process the path and options provided by user
void processArgs(int argc, char*argv[]) {
    bool setPathCount = false; //true once number of paths has been counted
    for(int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if(arg[0] == '-') { //option
            setOptions = true;
            int j = 0;
            char opt = arg[j];
            while(opt != '\0' || setPathCount) {
                j++;
                if(opt == 'i') {
                    inode = true;
                } else if(opt == 'l') {
                    longList = true;
                } else if(opt == 'R') {
                    recursive = true;
                } else if(opt == ' ' || opt == '\0') {
                    break;
                } else if(opt== '-') {
                    opt = arg[j];
                    continue;
                } else {
                    printf(OPTION_ERROR);
                    exit(EXIT_FAILURE);
                }
                opt = arg[j];
            }
        } else { //is path
            if(!setPathCount) { 
                setPath = true;
                pathCount = argc - i + 1; //number of paths
                cwdMode = false;
                paths = (char **)malloc(pathCount * sizeof(char *));
                setPathCount = true;
                pathCount = 0;
            }
            paths[pathCount] = arg;
            pathCount++;             
        }
    }            
}

//format and print the time
void printTime(struct tm time) {
    switch(time.tm_mon) {
        case 0:
            printf("Jan ");
            break;
        case 1:
            printf("Feb ");
            break;
        case 2:
            printf("Mar ");
            break;
        case 3:
            printf("Apr ");
            break;
        case 4:
            printf("May ");
            break;
        case 5:
            printf("Jun ");
            break;
        case 6:
            printf("Jul ");
            break;
        case 7:
            printf("Aug ");
            break;
        case 8:
            printf("Sep ");
            break;
        case 9:
            printf("Oct ");
            break;
        case 10:
            printf("Nov ");
            break;
        case 11:
            printf("Dec ");
            break;  
        default:
            printf("invalid time");
            exit(EXIT_FAILURE);              
    }
    if(time.tm_mday < 10) {
        printf(" ");
    } 
    printf("%d ", time.tm_mday);
    int year = time.tm_year + 1900;
    printf("%d ", year);
    if(time.tm_hour < 10) {
        printf("0");
    }
    printf("%d:", time.tm_hour);
    if(time.tm_min < 10) {
        printf("0");
    }
    printf("%d ", time.tm_min);
}

//print the info about a given file
void printFile(char *name, struct stat fileStat, bool lstat, char *fullName) {
    if(inode) {
        printf("%lu ", fileStat.st_ino);
    }
    if(longList) {
        struct group *group = getgrgid(fileStat.st_gid);
        struct passwd *user = getpwuid(fileStat.st_uid);

        //ADAPTED CODE
        /* adapted from : 
        * https://stackoverflow.com/questions/10323060/printing-file-permissions-like-ls-l-using-stat2-in-c
        */
        printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
        printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
        printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
        printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
        //END OF ADAPTED CODE
        printf(" %lu ", fileStat.st_nlink);
        printf("%s ", user->pw_name);
        printf("%s ", group->gr_name);
        printf("%ld ", fileStat.st_size); 
        struct tm time;
        tzset();
        localtime_r(&(fileStat.st_mtim.tv_sec), &time);
        printTime(time);

        char linkName[512];
        int readSize = readlink(fullName, linkName, 512);
        if(readSize < 0) {
            printf("%s\n", name);    
        } else { //is link
            printf("%s -> ", name);
            linkName[readSize] = '\0';
            printf("%s\n", linkName);
        }
    } else {
        printf("%s\n", name);
    }    
}

//get the info on the file/directory, can be called recursively
void getFileInfo(char *name) { 
    DIR *dir;
    struct dirent **files;
    int fileIndex = 0;
    if(!(dir = opendir(name))) {
        struct stat fileStat;
        if(stat(name, &fileStat) < 0) {
            printf(DIR_ERROR);
            exit(EXIT_FAILURE);
        } else {
            printFile(name, fileStat, false, name); 
            return;   
        }
    }
    int scanReturn = scandir(name, &files, NULL, alphasort);
    if(scanReturn < 0) {
        printf(DIR_ERROR);
        exit(EXIT_FAILURE);
    }
 
    while(fileIndex < scanReturn) {
        struct stat fileStat;
        if(files[fileIndex]->d_name[0] == '.') {
            fileIndex++;
            continue;
        }
        if(files[fileIndex]->d_type == DT_REG) {
            char fullName[512];
            sprintf(fullName, "%s/%s", name, files[fileIndex]->d_name);
            stat(fullName, &fileStat);
            printFile(files[fileIndex]->d_name, fileStat, false, fullName);
        }
        if(files[fileIndex]->d_type == DT_DIR) {
            if(!strcmp(files[fileIndex]->d_name, ".") || !strcmp(files[fileIndex]->d_name, "..")) {
                fileIndex++;
                continue;
            }
            char fullName[512];
            sprintf(fullName, "%s/%s", name, files[fileIndex]->d_name);
            stat(fullName, &fileStat);
            printFile(files[fileIndex]->d_name, fileStat, false, fullName);
            if(recursive) {
                printf("\n%s:\n", fullName);
                getFileInfo(fullName);    
            } 
        }
        if(files[fileIndex]->d_type == DT_LNK) {
            char fullName[512];
            sprintf(fullName, "%s/%s", name, files[fileIndex]->d_name);
            lstat(fullName, &fileStat);
            printFile(files[fileIndex]->d_name, fileStat, true, fullName);    
        }
        fileIndex++;
    }
    closedir(dir);
    for(int i = 0; i < scanReturn; i++) {
        free(files[i]);
    }
    free(files);
}

int main(int argc, char *argv[]) {
    processArgs(argc, argv);
    if(cwdMode) {
        char *path = (char *)malloc(2 * sizeof(char));
        paths = (char **)malloc(1 * sizeof(char *));
        strcpy(path, ".");
        path[1] = '\0';
        paths[0] = path;
        pathCount = 1;
    }

    for(int i = 0; i < pathCount; i++) {
        DIR *dir;
        if(pathCount > 1) {
            if((dir = opendir(paths[i]))) {
                printf("\n%s:\n", paths[i]);
                closedir(dir);
            }
        }   

        getFileInfo(paths[i]);
        
        if((pathCount - i) != 1) {
            if((dir = opendir(paths[i]))) {
                printf("\n");
                closedir(dir);
            }
        }
    }

    if(cwdMode) {
        free(paths[0]);
    } 
    free(paths);
    return 0;
}