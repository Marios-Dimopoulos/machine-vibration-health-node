// SPDX-License-Identifier: MIT
// First recipe of meta-vibration-node: proves our layer can build and install a program.
#include <stdio.h>
#include <sys/utsname.h>

int main(void)
{
    struct utsname u;

    // uname() asks the kernel which machine we run on: "aarch64" on the board.
    if (uname(&u) != 0) {
        perror("uname");
        return 1;
    }
    printf("vibration-hello: running on %s (%s), kernel %s\n", u.nodename, u.machine, u.release);
    return 0;
}
