<div align="center">
<img src="tools/website/assets/logo.png" width="150"/>
<h2>Fennix Operating System</h2>
</div>

<p align="center">
 <p align="center">
        <img alt="GitHub Repo stars" src="https://img.shields.io/github/stars/EnderIce2/Fennix">
    <img alt="GitHub Actions Workflow Status" src="https://img.shields.io/github/actions/workflow/status/EnderIce2/Fennix/makefile.yml">
    <img alt="GitHub commit activity" src="https://img.shields.io/github/commit-activity/m/EnderIce2/Fennix">
    <img alt="GitHub License" src="https://img.shields.io/github/license/EnderIce2/Fennix">
 </p>
</p>

<p align='center'>Fennix is an operating system built from scratch using C and C++.</p>

---

> [!CAUTION]
> The project is still in development and is not yet ready for production use.
>
> Please use a virtual machine to run the OS.

## Features

- 🧩 Flexible and modular
    - You can easily choose the components you want to include in the kernel
- 🎛️ Compatible with multiple platforms
    - Supports i386, AMD64 and AArch64 platforms
- 📦 Support for different operating system binaries
    - You can run Linux and Windows binaries
- 🖨️ Driver support
    - Supports a variety of drivers for different hardware components
- 📖 Detailed documentation
    - The project is well-documented using Doxygen

> [!IMPORTANT]
> Some features are still in development and may not be fully functional.

## Getting Started

### 💾 Download ISO Image

You can download the development ISO image from the [Actions](https://github.com/EnderIce2/Fennix/actions/workflows/makefile.yml) tab on the GitHub repository.

### 🛠️ Compile from Source

Check out the [installation guide](INSTALL.md) to compile the project from source.

## 📚 Documentation

The current documentation is available [here](https://fennix.enderice2.com/docs/index.html).

To generate the documentation, run:

```sh
make docs
```

The documentation will be available in the `doxygen-doc/docs` directory.

## 👨‍💻 Contributing

Contributions are welcome!  
Please read the [CONTRIBUTING.md](CONTRIBUTING.md) file for guidelines on how to contribute to this project.

### Contributors

<a href="https://github.com/EnderIce2/Fennix/graphs/contributors">
    <img src="https://contrib.rocks/image?repo=EnderIce2/Fennix" />
</a>

## 📃 License

Fennix is licensed under the GNU General Public License v3.0.
See the [LICENSE.md](LICENSE.md) file for more details.

Other licenses used in the project can be found in the [ATTRIBUTION.md](ATTRIBUTION.md) file.
