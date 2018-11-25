#include <SDL2/SDL.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>	/* for fprintf */
#include <string.h>	/* for memcpy */

void main(int args,char** argv)
{
    /* setup joystick */
    if (SDL_Init(SDL_INIT_JOYSTICK) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return;
    }
    if(SDL_NumJoysticks() < 1) {
        printf("no joysticks\n");
        return;
    }
    printf("num joysticks: %i\n", SDL_NumJoysticks());
    SDL_Joystick *stick = SDL_JoystickOpen(0);
    printf("%i\n", SDL_JoystickEventState(SDL_IGNORE));
    printf("%s\n",SDL_JoystickName(stick));
    if(SDL_JoystickNumAxes(stick) < 2) {
        printf("no axis\n");
        return;
    }
    printf("num axis: %i\n", SDL_JoystickNumAxes(stick));
    /* Setup socket */
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in myaddr;
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);
    bind(s, (struct sockaddr *)&myaddr, sizeof(myaddr));
    /* setup connection to robot */
    struct hostent *hp;     /* host information */
    struct sockaddr_in servaddr;    /* server address */

    /* fill in the server's address and data */
    memset((char*)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);

    /* look up the address of the server given its name */
    hp = gethostbyname("ESP_185A55");
    if (!hp) {
        fprintf(stderr, "could not obtain address of ESP_185A55\n");
    }

    /* put the host's address into the server address structure */
    memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
    /* main loop */
    usleep(1000000);
    while(1) {
        SDL_JoystickUpdate();
        signed char buf[2];
        buf[0] = -SDL_JoystickGetAxis(stick,1)/256-1;
        buf[1] = -SDL_JoystickGetAxis(stick,4)/256-1;
        printf("buf[0]: %i - buf[1]: %i\n",buf[0],buf[1]);
        printf("axis 4: %i - axis 1: %i\n",SDL_JoystickGetAxis(stick,4),SDL_JoystickGetAxis(stick,1));
        sendto(s, buf, 2, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        usleep(50000);
    }
}
