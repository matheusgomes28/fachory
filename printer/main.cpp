#include "printer_manager.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <array>
#include <iostream>

int main(int arc, char** arv) {
    PrinterManager manager{};

    for (auto const printer : manager.printers()) {
        std::cout << fmt::format("{}\n", printer);
    }
    std::cout << std::endl;

    // Print one item
    if (!manager.print_pdf("terow", "./memes/cat.pdf")) {
        spdlog::error("could not print pdf");
    }

    // clang-format off
    std::array<std::string, 6> to_print{
      "[ ] Going to the Gym",
      "[ ] Helping BB",
      "[ ] Eat fazenda",
      "[ ] Do chore",
      "[ ] Do Work",
      "[ ] Do Food",
    };
    // clang-format on

    std::string whole_print = "";
    for (auto const text : to_print) {
        whole_print = fmt::format("{}\n{}", whole_print, text);
    }

    if (!manager.print_text("terow", whole_print)) {
        spdlog::error("could not print");
        return -1;
    }

    return -1;
}
