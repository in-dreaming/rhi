#include <SDL3/SDL.h>
#include <stdio.h>
int main() {
    if (!SDL_Init(0)) return 1;
    printf("BasePath: %s\n", SDL_GetBasePath());
    SDL_Quit();
    return 0;
}
