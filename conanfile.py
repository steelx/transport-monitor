from conan import ConanFile

class ConanPackage(ConanFile):
    name = 'network-monitor'
    version = "0.1.0"

    # Define the settings your project uses
    settings = "os", "compiler", "build_type", "arch"

    # Use the new Conan 2.x generators
    generators = "CMakeDeps", "CMakeToolchain"

    requires = [
        "boost/1.89.0"
    ]

    default_options = {
        "boost/*:shared": False
    }
