# MAMBO-V: Dynamic Side-Channel Leakage Analysis on RISC-V

Source code for our RISC-V dynamic binary instrumentation tool based on the ARM [MAMBO](https://github.com/beehive-lab/mambo) framework.

MAMBO-V is a lightweight dynamic binary instrumentation tool for 64-bit RISC-V systems. The microwalk-tracer plugin creates application traces that can be used for side-channel leakage analysis with Microwalk.

Make sure to also checkout the [Microwalk main repository](https://github.com/microwalk-project/Microwalk)!

## Publications
* [Jan Wichelmann, Christopher Peredy, Florian Sieck, Anna Pätschke, Thomas Eisenbarth: MAMBO-V: Dynamic Side-Channel Leakage Analysis on RISC-V (April 2023)](https://arxiv.org/abs/2305.00584). If you use MAMBO-V for your research, please cite this paper.

## Build
__RISC-V (cross compilation):__
Prerequisites :

- [GNU toolchain for RISC-V](https://github.com/riscv-collab/riscv-gnu-toolchain)
- ruby (>=1.9.1)
- Clone repo with `--recurse-submodules` flag
```
    cd mambo
    CROSS_PREFIX=riscv64-unknown-linux-gnu- make
```

## Usage
To launch an application under MAMBO-V, run:

    ./dbm <path_to_executable> [application's command line arguments]

For example to run `ls -a` under MAMBO-V, execute:

    ./dbm /bin/ls -a

Tip: When an application running under MAMBO exits, the string `We're done; exiting with status: <APPLICATION'S EXIT CODE>` will be printed to stderr.


## Plugin API
The plugin API is event-driven. Plugins should use a init function with `__attribute__((constructor))` to register themselves using `mambo_register_plugin()`. Once a plugin is registered, it can install callbacks for various events using the `mambo_register_*_cb()` functions. Callback-related functions are listed in `api/plugin_support.h`. Code generation functions are listed in `api/emit_<INST SET>.h` and code generation helpers are listed in `api/helpers.h`. You can also inspect the sample plugin in the `plugins/` directory.

To build MAMBO/MAMBO-V with plugin support, uncomment the `-DPLUGINS_NEW` CFLAG in the `makefile`. Then, the source code or object file(s) of the plugin you're trying to build must be added to the `PLUGINS=` line in the `makefile`. Note that multiple plugins can be enabled at the same time (and will work correctly if properly designed). For performance reasons, it is recommended to remove unused plugins from the `PLUGINS=` list.
