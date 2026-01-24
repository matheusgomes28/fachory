#ifndef FACHORY_TYPES_H
#define FACHORY_TYPES_H

#include <variant>
#include <string>

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
}

#endif // FACHORY_TYPES_H
