#include <receipt/receipt.hpp>

#include <podofo/main/PdfCatalog.h>
#include <podofo/main/PdfDeclarations.h>
#include <podofo/main/PdfDocument.h>
#include <podofo/main/PdfFont.h>
#include <podofo/main/PdfMemDocument.h>
#include <podofo/main/PdfObject.h>
#include <podofo/main/PdfPage.h>
#include <podofo/podofo.h>
#include <spdlog/spdlog.h>

#include <optional>
#include <span>

namespace {
    void setPdfProperties(PoDoFo::PdfDocument& document) {
        PoDoFo::PdfCatalog& catalog = document.GetCatalog();
        catalog.GetDictionary().AddKey(PoDoFo::PdfName("PageMode"), PoDoFo::PdfName("UseNone"));
        catalog.GetDictionary().AddKey(PoDoFo::PdfName("PageLayout"), PoDoFo::PdfName("OneColumn"));
    }

    std::optional<PoDoFo::PdfFont*>
        getAvailableFont(PoDoFo::PdfDocument& document, std::span<std::string const> fonts) {
        for (auto const& font_name : fonts) {
            PoDoFo::PdfFont* currentFont = document.GetFonts().SearchFont(font_name);
            if (currentFont == nullptr) {
                spdlog::warn("could not find font {}", font_name);
                continue;
            }

            return std::make_optional(currentFont);
        }

        return std::nullopt;
    }
} // namespace

namespace fachory::receipt {

    Receipt::Receipt(PdfConfig config)
        : _pdf_config{std::move(config)} {}

    void Receipt::add_item(std::string const& name, std::string const& description, std::uint64_t quantity) {
        auto found = _items.find(name);

        if (found != end(_items)) {
            found->second.first++;
        } else {
            _items[name] = {1, description};
        }
    }

    bool Receipt::to_pdf(std::string const& filename) {
        // // static constexpr std::string_view FONT_ARRAY[] = {"Roboto", "Arial"};
        // static constexpr std::string_view FONT_ARRAY[] = {"Arial", "Arial"};
        // static constexpr std::span<std::string_view const> FONTS{FONT_ARRAY};

        PoDoFo::PdfMemDocument document;
        PoDoFo::PdfPainter painter;

        try {
            setPdfProperties(document);

            PoDoFo::PdfPage& page = document.GetPages().CreatePage(PoDoFo::PdfPageSize::A4);

            std::span<std::string const> fonts{begin(_pdf_config.preferred_fonts), end(_pdf_config.preferred_fonts)};

            auto const maybe_font = getAvailableFont(document, _pdf_config.preferred_fonts);
            if (!maybe_font) {
                spdlog::error("coult not find suitable fonts");
                return false;
            }

            // Drawing Stuff
            painter.SetCanvas(page);
            painter.TextState.SetFont(**maybe_font, 10);
            painter.DrawText("Hello World", 50, page.GetRect().Height - 50);
            painter.FinishDrawing();

            // TODO : How do we draw an image?
            // painter.DrawImage(const PdfImage &obj, double x, double y);

            // File Metadata
            document.GetMetadata().SetCreator(PoDoFo::PdfString("Fachory Receipts"));
            document.GetMetadata().SetAuthor(PoDoFo::PdfString("Matheus Gomes"));
            document.GetMetadata().SetTitle(PoDoFo::PdfString("Your Receipt"));
            document.GetMetadata().SetSubject(PoDoFo::PdfString("Receipt"));

            document.Save(filename);
        } catch (PoDoFo::PdfError& error) {
            spdlog::error("could not create pdf receipt: {}", error.what());
            return false;
        }

        return true;
    }
} // namespace fachory::receipt
