#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    const char prog_name[] = "curve";
    Nob_Cmd compile_cmd = {0};
    #define sources "main.c"
    nob_cmd_append(&compile_cmd, "gcc", "-Wall", "-Wextra", "-o", prog_name, sources, "-lraylib", "-lgdi32", "-lwinmm");
    Nob_Cmd run_prog_cmd = {0};
    nob_cmd_append(&run_prog_cmd, prog_name);
    if (!nob_cmd_run_sync(compile_cmd)) return 1;
    if (!nob_cmd_run_sync(run_prog_cmd)) return 1;
    return 0;
}

gcc -o shapes_easings_box_anim shapes_easings_box_anim.c -lraylib -lgdi32 -lwinmm
