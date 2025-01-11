#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cassert>
#include <cstdint>

void generate_test_files();

class Emulator
{
public:
    Emulator() : memory(65536, 0), video_memory(8192, 0), interrupt_flag(false), timer(0), disk_command(0), sector(0), program_counter(0) {}

    void load_memory(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (file.is_open())
        {
            file.read(reinterpret_cast<char *>(memory.data()), memory.size() * sizeof(uint16_t));
            std::cout << "Loaded memory from file: " << filename << std::endl;

            for (int i = 0; i < 10; ++i)
            {
                std::cout << std::hex << memory[i] << " ";
            }
            std::cout << std::endl;
        }
        else
        {
            std::cerr << "Failed to load memory from file: " << filename << std::endl;
        }
    }

    void load_disk(const std::string &filename)
    {
        disk_file = filename;
        std::ifstream file(filename, std::ios::binary);
        if (file.is_open())
        {
            file.read(reinterpret_cast<char *>(disk.data()), disk.size() * sizeof(uint16_t));
        }
    }

    void execute()
    {
        initialize_visualization();

        while (true)
        {
            handle_keyboard_input();

            if (interrupt_flag)
            {
                handle_interrupt();
                interrupt_flag = false;
            }

            uint16_t instruction = fetch_instruction();
            if (!execute_instruction(instruction))
            {
                break;
            }

            timer++;
            if (timer >= 8000)
            {
                interrupt_flag = true;
                timer = 0;
            }

            draw_screen();
            SDL_Delay(16);
        }

        cleanup_visualization();
    }

private:
    std::vector<uint16_t> memory;
    std::vector<uint16_t> video_memory;
    std::vector<uint16_t> disk = std::vector<uint16_t>(1024 * 10, 0);
    std::vector<uint16_t> registers = std::vector<uint16_t>(16, 0);
    std::string disk_file;
    bool interrupt_flag;
    uint16_t timer;
    uint16_t program_counter;

    uint16_t disk_command;
    uint16_t sector;

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Event event;

    void handle_keyboard_input()
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                exit(0);
            }
            else if (event.type == SDL_KEYDOWN)
            {
                std::cout << "Key pressed: " << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
            }
        }
    }

    void handle_interrupt()
    {
        std::cout << "Interrupt handled!" << std::endl;
    }

    void draw_keyboard()
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        for (int i = 0; i < 16; ++i)
        {
            SDL_Rect key_rect = {50 + i * 40, 500, 30, 30};
            SDL_RenderFillRect(renderer, &key_rect);
        }
    }

    void initialize_visualization()
    {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("Emulator Screen", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }

    void cleanup_visualization()
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void draw_screen()
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int row = 0; row < 25; ++row)
        {
            for (int col = 0; col < 80; ++col)
            {
                uint16_t word = video_memory[row * 80 + col];

                for (int segment = 0; segment < 16; ++segment)
                {
                    SDL_Rect rect = {col * 10 + (segment % 4) * 2, row * 20 + (segment / 4) * 4, 2, 4};
                    if (word & (1 << segment))
                    {
                        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    }
                    else
                    {
                        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                    }
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }

        draw_keyboard();

        SDL_RenderPresent(renderer);
    }

    uint16_t fetch_instruction()
    {
        uint16_t instruction = memory[program_counter++];
        std::cout << "PC: " << std::hex << program_counter - 1 << " | Instruction: " << instruction << std::endl;
        return instruction;
    }

    bool execute_instruction(uint16_t instruction)
    {
        switch (instruction & 0xF000)
        {
        case 0x1000:
            return execute_lod(instruction);
        case 0x2000:
            return execute_add(instruction);
        case 0x4000:
            return execute_jmp(instruction);
        case 0xF000:
            return false;
        default:
            std::cerr << "Unknown instruction: " << std::hex << instruction
                      << " | PC: " << std::hex << program_counter - 1 << std::endl;
            return false;
        }
    }

    bool execute_lod(uint16_t instruction)
    {
        uint16_t reg = (instruction >> 8) & 0x0F;
        uint16_t addr = instruction & 0x00FF;

        std::cout << "LOD R" << reg << ", " << addr << std::endl;

        registers[reg] = memory[addr];
        return true;
    }

    bool execute_add(uint16_t instruction)
    {
        uint16_t reg1 = (instruction >> 8) & 0x0F;
        uint16_t reg2 = (instruction >> 4) & 0x0F;

        if (reg1 >= 16 || reg2 >= 16)
        {
            std::cerr << "Invalid register in ADD instruction: R" << reg1 << ", R" << reg2 << std::endl;
            return false;
        }

        registers[reg1] += registers[reg2];
        std::cout << "ADD R" << reg1 << ", R" << reg2 << std::endl;
        return true;
    }

    bool execute_jmp(uint16_t instruction)
    {
        uint16_t addr = instruction & 0x0FFF;
        std::cout << "JMP to " << std::hex << addr << std::endl;
        program_counter = addr;
        return true;
    }
};

void generate_test_files()
{
    uint16_t program[] = {0x100A, 0x2001, 0xF000};
    std::ofstream file("test_program.bin", std::ios::binary);
    file.write(reinterpret_cast<const char *>(program), sizeof(program));
    file.close();

    std::vector<uint16_t> disk_sector(1024, 0xABCD);
    std::ofstream disk_file("test_disk.bin", std::ios::binary);
    disk_file.write(reinterpret_cast<const char *>(disk_sector.data()), disk_sector.size() * sizeof(uint16_t));
    disk_file.close();

    std::cout << "Test files generated: test_program.bin and test_disk.bin" << std::endl;
}

int main(int argc, char *argv[])
{
    // Ispis argumenata komandne linije
    std::cout << "Argumenti: ";
    for (int i = 0; i < argc; ++i)
    {
        std::cout << argv[i] << " ";
    }
    std::cout << std::endl;

    // Kreiramo instancu emulatora
    Emulator emulator;

    if (argc > 1 && std::strcmp(argv[1], "generate") == 0)
    {
        // Ako je prvi argument "generate", generišemo test fajlove
        generate_test_files();
        return 0; // Završavamo program nakon generisanja fajlova
    }
    else if (argc > 1)
    {
        // Ako je prosleđeno ime fajla, učitavamo memoriju iz tog fajla
        std::string program_filename = argv[1];
        emulator.load_memory(program_filename);

        if (argc > 2)
        {
            // Ako je prosleđen i fajl za disk, učitavamo ga
            std::string disk_filename = argv[2];
            emulator.load_disk(disk_filename);
        }
    }
    else
    {

        emulator.load_memory("program.bin");
    }

    // Pokrećemo izvršavanje emulatora
    emulator.execute();

    return 0;
}
