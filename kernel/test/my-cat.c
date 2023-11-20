/* Very simple cat */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <sys/stat.h>

void check_imode (char *filename)
{
    struct stat st;
    stat (filename, &st);
    printf ("imode: %o\n", st.st_mode & S_IFMT);
}

int main (int argc, char *argv[])
{
    // For landmark
    getpid ();

    int mode = 1;
    while (1) {
        printf ("Please input mode: ");
        scanf ("%d", &mode);
        getpid ();
        switch (mode) {
            case 0:
                printf ("Program exit\n");
                return 0;
            case 1:
                // DIR: Try to execute (cd prodir)
                mkdir ("prodir", 0000);
                if (chdir ("prodir") == -1) {
                    perror ("IFDIR");
                }
                break;
            case 2:
                // IFO: Try to execute
                mknod ("profifo", S_IFIFO | 0000, 0);
                /* check_imode ("profifo"); */

                // open to access
                if (open ("profifo", O_WRONLY) == -1) {
                    perror ("IFIFO");
                }
                break;
            default:
                printf ("Invalid mode\n");
                break;
        }
    }


    switch (mode) {
        case 0:
            // DIR: Try to execute (cd prodir)
            mkdir ("prodir", 0000);
            if (chdir ("prodir") == -1) {
                perror ("chdir");
                exit (1);
            }
            break;
        case 1:
            // IFO: Try to execute
            break;
    }

/*     /1* chmod (filename, 0000); *1/ */
/*     fchmod (fd, 0000); */

/*     // read for test */
/*     char buf[10]; */
/*     int n = read (fd, buf, 10); */
/*     if (n == -1) { */
/*         perror ("read"); */
/*         exit (1); */
/*     } */
/*     printf ("%s\n", buf); */

/*     /1* write (fd, "hello", 5); *1/ */

/*     // close */
/*     close (fd); */

/*     // Re-open */
/*     fd = open (filename, O_RDWR); */
/*     if (fd == -1) { */
/*         perror ("open"); */
/*         exit (1); */
/*     } */

    return 0;
}
