#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdint>
#include <bitset>

constexpr uint16_t VIDEO_MEMORY_START = 8192;
constexpr uint16_t VIDEO_MEMORY_WIDTH = 80;
constexpr uint16_t VIDEO_MEMORY_HEIGHT = 25;
constexpr uint16_t KEYBOARD_PORT = 0xFFFF;
constexpr uint16_t DISK_COMMAND_PORT = 0xFFFE;
constexpr uint16_t DISK_SECTOR_PORT = 0xFFFD;
constexpr uint16_t DISK_DATA_PORT = 0xFFFC;
constexpr uint16_t ROM_SIZE = 1024;
constexpr uint16_t DISK_SECTOR_SIZE = 1024;
constexpr uint16_t CLOCK_FREQUENCY_HZ = 8000000;
constexpr uint16_t INTERRUPT_INTERVAL_MS = 20;

std::vector<uint16_t> memory(ROM_SIZE + VIDEO_MEMORY_WIDTH * VIDEO_MEMORY_HEIGHT, 0);
std::vector<uint16_t> videoMemory(VIDEO_MEMORY_WIDTH *VIDEO_MEMORY_HEIGHT, 0);
uint16_t keyboardInput = 0;

void displayCharacter(uint16_t value)
{
    for (int i = 15; i >= 0; --i)
        std::cout << ((value & (1 << i)) ? "*" : " ");
    std::cout << std::endl;
}

void renderVideoMemory()
{
    for (int y = 0; y < VIDEO_MEMORY_HEIGHT; ++y)
    {
        for (int x = 0; x < VIDEO_MEMORY_WIDTH; ++x)
        {
            uint16_t value = memory[VIDEO_MEMORY_START + y * VIDEO_MEMORY_WIDTH + x];
            displayCharacter(value);
        }
        std::cout << std::endl;
    }
}

uint16_t readKeyboardInput()
{
    char key;
    std::cout << "Unesite znak: ";
    std::cin >> key;
    return static_cast<uint16_t>(key);
}

void handleDiskCommand(uint16_t command, uint16_t sectorNumber, uint16_t *data)
{
    std::fstream diskFile("disk.img", std::ios::binary | std::ios::in | std::ios::out);

    if (!diskFile)
    {
        std::cerr << "Greska: Nije moguce otvoriti datoteku diska!" << std::endl;
        return;
    }

    switch (command)
    {
    case 0:
        std::cout << "Disk resetovan." << std::endl;
        break;
    case 1:
        diskFile.seekg(sectorNumber * DISK_SECTOR_SIZE * sizeof(uint16_t));
        diskFile.read(reinterpret_cast<char *>(data), DISK_SECTOR_SIZE * sizeof(uint16_t));
        std::cout << "Podaci procitani iz sektora." << std::endl;
        break;
    case 2:
        diskFile.seekp(sectorNumber * DISK_SECTOR_SIZE * sizeof(uint16_t));
        diskFile.write(reinterpret_cast<char *>(data), DISK_SECTOR_SIZE * sizeof(uint16_t));
        std::cout << "Podaci upisani u sektor." << std::endl;
        break;
    default:
        std::cerr << "Nepoznata komanda!" << std::endl;
    }

    diskFile.close();
}

/*void cpuLoop()
{
    int i = 0;
    while (true)
    {
        uint16_t instruction = 0; // fetchInstruction();

        // executeInstruction(instruction);

        static auto lastInterrupt = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastInterrupt).count() >= INTERRUPT_INTERVAL_MS)
        {
            std::cout << "Interapt signal generisan!" << ++i << std::endl;
            lastInterrupt = now;
        }

        std::this_thread::sleep_for(std::chrono::nanoseconds(125));
    }
}*/

void cpuLoop()
{
    int i = 0;
    auto startTime = std::chrono::steady_clock::now();

    while (true)
    {

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count() >= 5)
            break;

        static auto lastInterrupt = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastInterrupt).count() >= INTERRUPT_INTERVAL_MS)
        {
            std::cout << "Interapt signal generisan! " << ++i << std::endl;
            lastInterrupt = now;
        }

        std::this_thread::sleep_for(std::chrono::nanoseconds(125));
    }
}

void bootLoader()
{
    std::cout << "Ucitavanje eFORTH OS-a..." << std::endl;
    std::fstream diskFile("disk.img", std::ios::binary | std::ios::in);
    if (diskFile)
    {
        diskFile.read(reinterpret_cast<char *>(memory.data()), ROM_SIZE * sizeof(uint16_t));
        std::cout << "OS uspjesno ucitan." << std::endl;
    }
    else
    {
        std::cerr << "Greska: Nije moguce ucitati OS sa diska!" << std::endl;
    }
}

int main()
{
    std::ofstream diskFile("disk.img", std::ios::binary | std::ios::out);
    std::vector<uint16_t> sector(DISK_SECTOR_SIZE, 0);
    diskFile.write(reinterpret_cast<char *>(sector.data()), sector.size() * sizeof(uint16_t));
    diskFile.close();

    bootLoader();

    while (true)
    {
        std::cout << "1. Prikazi video memoriju\n2. Unesi podatke sa tastature\n3. Upravljaj diskom\nIzbor: ";
        int choice;
        std::cin >> choice;

        switch (choice)
        {
        case 1:
            renderVideoMemory();
            break;
        case 2:
            keyboardInput = readKeyboardInput();
            memory[KEYBOARD_PORT] = keyboardInput;
            std::cout << "Uneseni znak: " << static_cast<char>(keyboardInput) << std::endl;
            break;
        case 3:
        {
            uint16_t command, sectorNumber;
            std::vector<uint16_t> buffer(DISK_SECTOR_SIZE, 0);
            std::cout << "Unesite komandu (0-reset, 1-read, 2-write): ";
            std::cin >> command;
            std::cout << "Unesite sektor: ";
            std::cin >> sectorNumber;
            handleDiskCommand(command, sectorNumber, buffer.data());
            break;
        }
        default:
            std::cout << "Nepoznata opcija." << std::endl;
        }

        if (choice == 2)
        {
            std::thread cpuThread(cpuLoop);
            cpuThread.detach();
            break;
        }
    }

    /* std::thread cpuThread(cpuLoop);

   while (true)
    {
        std::cout << "1. Prikazi video memoriju\n2. Unesi podatke sa tastature\n3. Upravljaj diskom\nIzbor: ";
        int choice;
        std::cin >> choice;

        switch (choice)
        {
        case 1:
            renderVideoMemory();
            break;
        case 2:
            keyboardInput = readKeyboardInput();
            memory[KEYBOARD_PORT] = keyboardInput;
            std::cout << "Uneseni znak: " << static_cast<char>(keyboardInput) << std::endl;
            break;
        case 3:
        {
            uint16_t command, sectorNumber;
            std::vector<uint16_t> buffer(DISK_SECTOR_SIZE, 0);
            std::cout << "Unesite komandu (0-reset, 1-read, 2-write): ";
            std::cin >> command;
            std::cout << "Unesite sektor: ";
            std::cin >> sectorNumber;
            handleDiskCommand(command, sectorNumber, buffer.data());
            break;
        }
        default:
            std::cout << "Nepoznata opcija." << std::endl;
        }
    }

    cpuThread.join();*/
    return 0;
}