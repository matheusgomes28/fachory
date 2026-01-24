from conan import ConanFile


class WavyTuneConan(ConanFile):
    settings = ("os", "compiler", "build_type", "arch")
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("crowcpp-crow/1.3.0")
        self.requires("cxxopts/3.2.0")
        self.requires("fmt/11.2.0")
        self.requires("gtest/1.17.0")
        self.requires("ms-gsl/4.1.0")
        self.requires("nlohmann_json/3.12.0")
        self.requires("podofo/1.0.3")
        self.requires("qt/6.5.3")
        self.requires("spdlog/1.15.3")
        self.requires("sqlitecpp/3.3.3")
        self.requires("stduuid/1.2.3")

    def configure(self):
        self.options["sqlitecpp/*"].with_sqlcipher = True
        self.options["podofo/*"].with_jpeg = "libjpeg"
