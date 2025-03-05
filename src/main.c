#include <stdio.h>
#include <stdbool.h>
#include "SDL2/SDL.h"
#include <unistd.h>
#include "chip8.h"
#include "chip8keyboard.h"

const char keyboard_map[CHIP8_TOTAL_KEYS] = {
    SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5,
    SDLK_6, SDLK_7, SDLK_8, SDLK_9, SDLK_a, SDLK_b,
    SDLK_c, SDLK_d, SDLK_e, SDLK_f};

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("You must provide a file to load\n");
        return -1;
    }

    const char *filename = argv[1];
    printf("The filename to load is : %s\n", filename);

    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        printf("Failed to open the file\n");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char buf[size];
    if (fread(buf, size, 1, f) != 1)
    {
        printf("Failed to read from file\n");
        return -1;
    }

    struct chip8 chip8;
    chip8_init(&chip8);
    chip8_load(&chip8, buf, size);
    chip8_keyboard_set_map(&chip8.keyboard, keyboard_map);

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow(
        EMULATOR_WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        REAL_WIDTH,
        REAL_HEIGHT,
        SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_TEXTUREACCESS_TARGET);

    const int FPS = 60;
    const int FRAME_DELAY = 1000 / FPS;
    Uint32 frame_start;
    int frame_time;

    while (1)
    {
        frame_start = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                goto out;

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
            {
                char key = event.key.keysym.sym;
                int vkey = chip8_keyboard_map(&chip8.keyboard, key);
                if (vkey != -1)
                {
                    if (event.type == SDL_KEYDOWN)
                        chip8_keyboard_down(&chip8.keyboard, vkey);
                    else
                        chip8_keyboard_up(&chip8.keyboard, vkey);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);

        for (int x = 0; x < CHIP8_WIDTH; x++)
        {
            for (int y = 0; y < CHIP8_HEIGHT; y++)
            {
                if (chip8_screen_is_set(&chip8.screen, x, y))
                {
                    SDL_Rect r = {x * CHIP8_WINDOW_MULTIPLIER, y * CHIP8_WINDOW_MULTIPLIER,
                                  CHIP8_WINDOW_MULTIPLIER, CHIP8_WINDOW_MULTIPLIER};
                    SDL_RenderFillRect(renderer, &r);
                }
            }
        }
        SDL_RenderPresent(renderer);

        if (chip8.registers.delay_timer > 0)
            chip8.registers.delay_timer--;

        if (chip8.registers.sound_timer > 0)
        {
            printf("\aBeep!\n");
            chip8.registers.sound_timer--;
        }

        for (int i = 0; i < 10; i++)
        {
            unsigned short opcode = chip8_memory_get_short(&chip8.memory, chip8.registers.PC);
            chip8.registers.PC += 2;
            chip8_exec(&chip8, opcode);
        }

        frame_time = SDL_GetTicks() - frame_start;
        if (FRAME_DELAY > frame_time)
        {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }
out:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
