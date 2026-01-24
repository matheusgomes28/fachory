#ifndef RECEIPT_RECEIPT_H
#define RECEIPT_RECEIPT_H

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace fachory::receipt {

    struct PdfConfig {
        std::vector<std::string> preferred_fonts;
    };


    class Receipt {
    public:
        Receipt(PdfConfig config);
        void add_item(std::string const& name, std::string const& description, std::uint64_t quantity);

        bool to_pdf(std::string const& filename);

    private:
        std::map<std::string, std::pair<std::uint64_t, std::string>> _items;
        PdfConfig _pdf_config;
    };

} // namespace fachory::receipt


#endif //  RECEIPT_RECEIPT_H
