#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdint>
#include <bitset>
#include <map>

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

std::map<char, uint16_t> asciiToSegment = {
    {'A', 0b1110111}, {'B', 0b1111100}, {'C', 0b0111001}, {'D', 0b1011110},
    {'E', 0b1111001}, {'F', 0b1110001}, {'0', 0b0111111}, {'1', 0b0000110},
    {'2', 0b1011011}, {'3', 0b1001111}, {'4', 0b1100110}, {'5', 0b1101101},
    {'6', 0b1111101}, {'7', 0b0000111}, {'8', 0b1111111}, {'9', 0b1101111},
};

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
    if (asciiToSegment.find(key) != asciiToSegment.end())
    {
        return asciiToSegment[key];
    }
    else
    {
        return 0;
    }
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

    cpuLoop();

    return 0;
}