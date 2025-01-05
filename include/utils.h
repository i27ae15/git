#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

// Define color codes as macros
#define RESET       "\033[0m"
#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"
#define BOLD        "\033[1m"
#define UNDERLINE   "\033[4m"
#define PINK        "\033[95m"
#define ORANGE      "\033[38;5;214m"

#define BIG_SEP "========================================================="
#define SML_SEP "---------------------------------------------------------"


// Utility macros for easy colored output
#define PRINT_BIG_SEPARATION     std::cout << PINK << BIG_SEP << RESET << "\x0A"
#define PRINT_SML_SEPARATION     std::cout << PINK << SML_SEP << RESET << "\x0A"

#define PRINT_SUCCESS(text)      std::cout << GREEN  << text << RESET << "\x0A"
#define PRINT_WARNING(text)      std::cout << YELLOW << text << RESET << "\x0A"
#define PRINT_ERROR(text)        std::cout << RED    << text << RESET << "\x0A"
#define PRINT_HIGHLIGHT(text)    std::cout << PINK   << text << RESET << "\x0A"
#define PRINT_COLOR(color, text) std::cout << color  << text << RESET << "\x0A"

#define PRINT_SUCCESS_NO_SPACE(text)      std::cout << GREEN  << text << RESET
#define PRINT_WARNING_NO_SPACE(text)      std::cout << YELLOW << text << RESET
#define PRINT_ERROR_NO_SPACE(text)        std::cout << RED    << text << RESET
#define PRINT_HIGHLIGHT_NO_SPACE(text)    std::cout << PINK   << text << RESET
#define PRINT_COLOR_NO_SPACE(color, text) std::cout << color  << text << RESET

