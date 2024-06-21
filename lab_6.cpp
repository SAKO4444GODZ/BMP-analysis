#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm> // for std::remove_if

// Structure to hold RGB color and count
struct ColorEntry {
    int r, g, b;
    std::string name;
    int count;

    ColorEntry(int red, int green, int blue, const std::string &colorName)
        : r(red), g(green), b(blue), name(colorName), count(0) {}
};

// Trim whitespace from the beginning and end of a string
std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return ""; // no content
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

// Function to load colors from Colors64.txt
std::vector<ColorEntry> loadColors(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error opening color file: " << filename << std::endl;
        exit(1);
    }

    std::vector<ColorEntry> colors;
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        int r, g, b;
        ss >> r >> g >> b;

        // Read the rest of the line as the color name
        std::string name;
        std::getline(ss, name);

        name = trim(name);  // Remove leading and trailing spaces

        // Check for valid RGB range
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
            std::cerr << "Invalid color value in: " << name << std::endl;
            continue;
        }

        colors.emplace_back(r, g, b, name);
    }

    return colors;
}

// Function to read and process the bitmap
std::vector<unsigned char> loadBitmap(const std::string &filename, int &width, int &height) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening bitmap file: " << filename << std::endl;
        exit(1);
    }

    // Reading bitmap file header
    unsigned char fileHeader[14];
    file.read(reinterpret_cast<char *>(fileHeader), 14);

    // Reading bitmap info header
    unsigned char infoHeader[40];
    file.read(reinterpret_cast<char *>(infoHeader), 40);

    // Extract width and height
    width = *(int*)&infoHeader[4];
    height = *(int*)&infoHeader[8];

    // Read bitmap data
    std::vector<unsigned char> data(width * height * 3); // 3 bytes per pixel (RGB)
    file.read(reinterpret_cast<char *>(data.data()), data.size());

    return data;
}

// Function to match and count colors
void matchAndCountColors(const std::vector<unsigned char> &bitmap, int width, int height, std::vector<ColorEntry> &colors) {
    int comparisons = 0;
    int unmatched = 0;

    auto start = std::chrono::high_resolution_clock::now();

    // Process each pixel in the bitmap
    for (int i = 0; i < width * height * 3; i += 3) {
        unsigned char b = bitmap[i];
        unsigned char g = bitmap[i + 1];
        unsigned char r = bitmap[i + 2];

        bool matched = false;

        // Check against each color
        for (auto &color : colors) {
            comparisons++;
            // Use inline assembly to compare colors
            bool match = false;
            __asm__ (
                "cmpb %[r], %[color_r];"   // Compare red values (byte)
                "jne no_match;"
                "cmpb %[g], %[color_g];"   // Compare green values (byte)
                "jne no_match;"
                "cmpb %[b], %[color_b];"   // Compare blue values (byte)
                "jne no_match;"
                "movb $1, %[matched];"     // Set match flag if all components match
                "jmp done;"
                "no_match:;"
                "movb $0, %[matched];"     // Set match flag to 0 if no match
                "done:;"
                : [matched] "=r" (match)
                : [r] "r" (r), [g] "r" (g), [b] "r" (b), [color_r] "r" ((unsigned char)color.r), [color_g] "r" ((unsigned char)color.g), [color_b] "r" ((unsigned char)color.b)
                : "cc"
            );

            if (match) {
                color.count++;
                matched = true;
                break;
            }
        }

        if (!matched) {
            unmatched++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    // Print results
    std::cout << "Color Matches:\n";
    for (const auto &color : colors) {
        if (color.count > 0) {
            std::cout << color.count << " pixel(s) of: " << color.name << "\n";
        }
    }
    std::cout << "Unmatched Pixels: " << unmatched << "\n";
    std::cout << "Total Comparisons: " << comparisons << "\n";
    std::cout << "CPU Time: " << elapsed.count() << " seconds\n";
}

int main() {
    // Load color data
    std::vector<ColorEntry> colors = loadColors("/mnt/data/Colors64.txt");

    // Load bitmap data
    int width, height;
    std::vector<unsigned char> bitmap = loadBitmap("/mnt/data/CS230.bmp", width, height);

    // Match and count colors
    matchAndCountColors(bitmap, width, height, colors);

    return 0;
}
