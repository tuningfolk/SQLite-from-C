// C program to read nth byte of a file and
// copy it to another file using lseek
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

void func(char arr[])
{
    int skip = 2;
	// Open the file for READ only.
	int f_write = open("start.txt", O_RDONLY);

	// Open the file for WRITE and READ only.
	int f_read = open("end.txt", O_WRONLY);

	while (read(f_write, arr, 1))
	{
        // SEEK_CUR specifies that
        // the offset provided is relative to the
        // current file position
        lseek (f_write, skip, SEEK_CUR);
        write (f_read, arr, skip);
	}
	close(f_write);
	close(f_read);
}

// Driver code
int main()
{
	char arr[100];
	func(arr);
	return 0;
}
