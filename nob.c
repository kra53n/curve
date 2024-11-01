#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    #define prog_name "curve"
    #define sources "main.c"

    nob_shift_args(&argc, &argv);

    Nob_Cmd compile_cmd = {0};
    if (*argv && (strcmp(*argv, "build") == 0 || strcmp(*argv, "b") == 0)) {
        nob_mkdir_if_not_exists("build");
        nob_mkdir_if_not_exists("build/assets");
        nob_cmd_append(&compile_cmd, "gcc", "-Wall", "-Wextra", "-mwindows", "-static", "-o", "build/"prog_name, sources, "-lraylib", "-lgdi32", "-lwinmm");
        nob_copy_directory_recursively("assets", "build/assets");
    } else {
        nob_cmd_append(&compile_cmd, "gcc", "-Wall", "-Wextra", "-o", prog_name, sources, "-lraylib", "-lgdi32", "-lwinmm");
    }

    Nob_Cmd run_prog_cmd = {0};
    nob_cmd_append(&run_prog_cmd, prog_name);

    if (!nob_cmd_run_sync(compile_cmd)) {
        return 1;
    }

    if (*argv && (strcmp(*argv, "run") == 0 || strcmp(*argv, "r") == 0)) {
        if (!nob_cmd_run_sync(run_prog_cmd)) {
            return 1;
        }
    }
    return 0;
}
