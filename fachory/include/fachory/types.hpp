#ifndef FACHORY_TYPES_H
#define FACHORY_TYPES_H

#include <string>
#include <variant>

namespace fachory::app {

    struct ImagePrint {
        std::string filename;
    };

    struct PdfPrint {
        std::string filename;
    };

    struct TextPrint {
        std::string text;
    };


    using PrintItem = std::variant<std::monostate, ImagePrint, PdfPrint, TextPrint>;
} // namespace fachory::app

#endif // FACHORY_TYPES_H
