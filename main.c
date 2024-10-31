#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>

#include "raylib.h"
#include "raymath.h"

#include "utils.h"

// TODO(kra53n): make glow affect for circles

// TODO(kra53n): space should start and then if it was pressed again should stop cirve computing

#define nil 0
#define pi (atan2f(1, 1) * 4)
#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

int screen_width = 720;
int screen_height = 460;

Color point_col;
Color point_col1;
Color point_col2;
const float point_radius = 10.0f;
const float point_radius_big = 20.0f;

float overshoot_animation(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return 1 - cos(4*pi * t) * (1-t) * 0.3;
}

typedef enum {
    APP_INTERACTIVE = 0,
    APP_EDITOR,
} APP_STATE;

typedef struct {
    int n;
    int edges;
    Vector2 *points;
    Vector2 *inner_points;
    Vector2 curr_point;
    int selected_point;
} BezierCurve;

// b - B(t) from https://en.wikipedia.org/wiki/B%C3%A9zier_curve
void compute_curve(BezierCurve *bc, float b, Vector2 *points, int n, int edges) {
    if (n == 1) {
        bc->inner_points[edges] = points[0];
        bc->curr_point = points[0];
        return;
    }
    Vector2 *inner_points = (Vector2*)malloc(sizeof(Vector2) * n-1);
    Vector2 p;
    for (int i = 0; i < n-1; i++) {
        Vector2 p1 = points[i];
        Vector2 p2 = points[i+1];
        p.x = p1.x + (p2.x - p1.x) * b;
        p.y = p1.y + (p2.y - p1.y) * b;
        inner_points[i] = p;
        bc->inner_points[edges++] = p1;
        bc->inner_points[edges] = p2;
    }
    edges++;
    compute_curve(bc, b, inner_points, n-1, edges);
    free(inner_points);
}

void touch_curve_point(BezierCurve *bc) {
    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < bc->n; i++) {
        Vector2 p = {
            .x = bc->points[i].x * screen_width,
            .y = bc->points[i].y * screen_height
        };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointCircle(mouse, p, point_radius_big)) {
            bc->selected_point = i;
            break;
        }
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        bc->selected_point = -1;
    }
    if (bc->selected_point >= 0) {
        bc->points[bc->selected_point] = mouse;
        bc->points[bc->selected_point].x /= screen_width;
        bc->points[bc->selected_point].y /= screen_height;
    }
}

void draw_curve_connection_lines(const BezierCurve *bc) {
    for (int i = bc->n, off = 0; i >= 0; i--) {
        for (int j = 0; j < i-1; j++) {
            Vector2 p1 = bc->inner_points[off];
            Vector2 p2 = bc->inner_points[off+1];
            p1.x *= screen_width; p1.y *= screen_height;
            p2.x *= screen_width; p2.y *= screen_height;
            DrawLineEx(p1, p2, 3.f, WHITE);
            off++;
        }
        off++;
    }
}

void draw_curve_points(const BezierCurve *bc) {
    for (int i = 0; i < bc->edges; i++) {
        Vector2 p = bc->inner_points[i];
        Color col = point_col;
        if (i == bc->edges-1) {
            col = point_col2;
        } else if (i >= bc->n) {
            col = point_col1;
        }
        DrawCircle(p.x * screen_width, p.y * screen_height, point_radius, col);
        drawc(p.x * screen_width, p.y * screen_height, point_radius_big, col);
    }
}

int count_curve_edges(int n) {
    if (n == 1) return 1;
    return n + count_curve_edges(n-1);
}

void init_quad_curve(BezierCurve *bc) {
    bc->n = 3;
    bc->points = (Vector2*)malloc(sizeof(Vector2) * bc->n);
    bc->points[0] = (Vector2) { 0.2f, 0.8f, };
    bc->points[1] = (Vector2) { 0.5f, 0.2f, };
    bc->points[2] = (Vector2) { 0.8f, 0.8f, };
    bc->edges = count_curve_edges(bc->n);
    bc->inner_points = (Vector2*)malloc(sizeof(Vector2) * bc->edges);
    bc->selected_point = -1;
}

void init_cubic_curve(BezierCurve *bc) {
    bc->n = 4;
    bc->points = (Vector2*)malloc(sizeof(Vector2) * 20);
    bc->points[0] = (Vector2) { 0.2f, 0.2f, };
    bc->points[1] = (Vector2) { 0.4f, 0.8f, };
    bc->points[2] = (Vector2) { 0.6f, 0.8f, };
    bc->points[3] = (Vector2) { 0.8f, 0.2f, };
    bc->edges = count_curve_edges(bc->n);
    bc->inner_points = (Vector2*)malloc(sizeof(Vector2) * count_curve_edges(20));
    bc->selected_point = -1;
}

typedef enum {
    CONTEXT_MENU_INTERACTIVE = 0,
    CONTEXT_MENU_EDITOR,
} CONTEXT_MENU_TYPE;

typedef struct {
    CONTEXT_MENU_TYPE type;

    Font *font;
    Color col;

    int padding;
    int sz;

    Vector2 pos;
    Rectangle bound;

    char n;
    const char **options;
    float *option_widths;
    int max_option_width;

    bool active;
    int choosed;

    float last_pressed;

    float *animations; /* -1..0 - sleep
                           0..1 - play
                           1..2 - play when mouse hovers option
                       */
} ContextMenu;

ContextMenu _new_context_menu(const ContextMenu *old, CONTEXT_MENU_TYPE type, const char *options[], int len) {
    ContextMenu menu;
    menu.type = type;
    menu.font = old->font;
    menu.col = old->col;
    menu.padding = old->padding;
    menu.sz = old->sz;
    menu.active = false;
    menu.n = len;
    menu.options= options;
    menu.option_widths = (float*)malloc(sizeof(float) * len);
    menu.max_option_width = 0.0f;
    menu.animations = (float*)malloc(sizeof(float) * len);

    for (int i = 0; i < len; i++) {
        Vector2 bound = MeasureTextEx(*menu.font, options[i], menu.sz, nil);
        menu.max_option_width = MAX(menu.max_option_width, bound.x);
        menu.option_widths[i] = bound.x;
        menu.animations[i] = -1.0f;
    }

    menu.bound = (Rectangle) {
        .x = 0,
        .y = 0,
        .width = menu.max_option_width + 4 * menu.padding,
        .height = menu.padding * 3 + (menu.sz + menu.padding) * len,
    };

    return menu;
}

#define new_context_menu(old_context_menu, type, ...)                    \
    _new_context_menu(old_context_menu, type, (const char*[]){__VA_ARGS__}, sizeof((const char*[]){__VA_ARGS__}) / sizeof(const char*))

void invoke_context_menu(ContextMenu *cm) {
    Vector2 mouse = GetMousePosition();
    cm->active = true;
    cm->pos = (Vector2) {
        .x = mouse.x + cm->padding,
        .y = mouse.y + cm->padding,
    };

    cm->bound.x = mouse.x - cm->padding;
    cm->bound.y = mouse.y - cm->padding;

    float beyound;
    if ((beyound = cm->bound.x + cm->bound.width - screen_width) > 0) {
        cm->pos.x -= beyound;
        cm->bound.x -= beyound - cm->padding;
    }
    if ((beyound = cm->bound.y + cm->bound.height - screen_height) > 0) {
        cm->pos.y = mouse.y - cm->bound.height + cm->padding;
        cm->bound.y = mouse.y - cm->bound.height + cm->padding;
    }

    for (int i = 0; i < cm->n; i++) {
        cm->animations[i] = -1.0f;
    }

    cm->choosed = -1;
}

void update_context_menu(ContextMenu *cm) {
    Vector2 mouse = GetMousePosition();
    Vector2 pos = cm->pos;
    pos.y += cm->padding;
    float dt = GetFrameTime();
    for (int i = 0; i < cm->n; i++) {
        Rectangle r = {
            .x = pos.x,
            .y = pos.y,
            .width = cm->max_option_width,
            .height = cm->sz,
        };
        if (cm->animations[i] >= 0.0f) {
            cm->animations[i] += dt;
        }
        if (CheckCollisionPointRec(mouse, r)) {
            if (cm->animations[i] < 1.0f) {
                cm->animations[i] = 1.0f;
            } else if (cm->animations[i] >= 2.0f) {
                cm->animations[i] = 2.0f;
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
                cm->choosed = i;
                cm->active = false;
            }
        } else if (cm->animations[i] > 1.0f) {
            cm->animations[i] -= 1.0f;
        } else if (0.98 <= cm->animations[i] && cm->animations[i] <= 1.0f) {
            cm->animations[i] = -1.0f;
        }

        pos.y += cm->sz + cm->padding;
    }
}

void keep_context_menu(ContextMenu *cm) {
    cm->active = true;
    cm->choosed = -1;
}

void draw_context_menu(ContextMenu *context_menu) {
    Vector2 pos = context_menu->pos;
    pos.y += context_menu->padding;
    for (int i = 0; i < context_menu->n; i++) {
        float x = pos.x + (context_menu->max_option_width/2 - context_menu->option_widths[i]/2);
        float y = pos.y;
        float sz = context_menu->sz;
        if (0 <= context_menu->animations[i] && context_menu->animations[i] <= 2.0f) {
            float val = context_menu->animations[i];
            if (context_menu->animations[i] >= 1.0f) {
                val -= 1.0f;
            }
            float scale = overshoot_animation(val);
            sz *= scale;
            x += context_menu->option_widths[i]/2 * (1-scale);
            y += context_menu->sz/2 * (1-scale);
        }
        DrawTextEx(*context_menu->font, context_menu->options[i], (Vector2) {x, y}, sz, nil, context_menu->col);
        pos.y += context_menu->sz + context_menu->padding;
    }
}

typedef struct {
    Vector2 curve[100];
    int last_curve_point_index;
    int curr_curve_point_index;
    float b;
    BezierCurve bc;
    bool animate;
} Interactive;

void update_interactive(Interactive *i) {
    if (i->animate) {
        float dt = GetFrameTime();
        i->b += dt / 3;
        if (i->b >= 1.0f) {
            i->b = 1.0f;
        }
    }

    if (IsKeyPressed(KEY_SPACE)) {
        if (i->b >= 1.0f || (i->animate == false && i->b <= 0.01f)) {
            i->b = 0.0f;
            i->last_curve_point_index = -1;
            i->curr_curve_point_index = -1;
        }
        i->animate = !i->animate;
    }

    touch_curve_point(&i->bc);
    if (i->bc.selected_point >= 0) {
        i->b = 0.0f;
        i->last_curve_point_index = -1;
        i->curr_curve_point_index = -1;
    }

    int new_curve_point_index = (int)(i->b * 100);
    if (new_curve_point_index - i->last_curve_point_index > 0) {
        i->curr_curve_point_index++;
        i->last_curve_point_index = new_curve_point_index;
        if (i->curr_curve_point_index >= 100) {
            i->curr_curve_point_index = 100-1;
        }
        i->curve[i->curr_curve_point_index] = i->bc.curr_point;
    }
}

void nil_b_in_interactive(Interactive *i) {
    i->b = 0.0f;
    i->last_curve_point_index = -1;
    i->curr_curve_point_index = -1;
}


int get_max_distance_idx_beetween_vecs(Vector2 *vs, int n) {
    float max_dist = 0.0f;
    int idx = 0;
    for (int i = n-2; i >= 0; i--) {
        float dist = Vector2Distance(vs[i], vs[i+1]);
        if (max_dist <= dist) {
            max_dist = dist;
            idx = i;
        }
    }
    return idx;
}

int get_min_distance_idx_beetween_vecs(Vector2 *vs, int n) {
    float min_dist = 33333.0f;
    int idx = 0;
    for (int i = n-2; i >= 0; i--) {
        float dist = Vector2Distance(vs[i], vs[i+1]);
        if (min_dist > dist) {
            min_dist = dist;
            idx = i;
        }
    }
    return idx;
}

void draw_interactive(Interactive *i) {
    if (i->curr_curve_point_index >= 2){
        for (int j = 0; j < i->curr_curve_point_index; j++) {
            Vector2 p1 = i->curve[j];
            Vector2 p2 = i->curve[j+1];
            p1.x *= screen_width; p1.y *= screen_height;
            p2.x *= screen_width; p2.y *= screen_height;
            DrawLineEx(p1, p2, 3.f, point_col);
        }
    }
    compute_curve(&i->bc, i->b, i->bc.points, i->bc.n, 0);
    draw_curve_connection_lines(&i->bc);
    draw_curve_points(&i->bc);
}

typedef struct {
    Rectangle r;
    float max_sz;
    float thick;
    Color background_col;
    Color point_col;
    Color line_col;
    Color line_col1;
    Vector2 points[4];
    int point_drag;

    float b;
} Editor;

void update_editor(Editor *e) {
    Vector2 mouse = GetMousePosition();
    float sz = MIN(screen_width, screen_height);

    e->r.width = MIN(sz * 0.7, 800);
    e->r.height = e->r.width;
    e->r.x = screen_width/2 - e->r.width/2;
    e->r.y = screen_height/2 - e->r.height/2;

    Vector2 points[4];
    for (int i = 0; i < 4; i++) {
        points[i] = e->points[i];
        Vector2 *p = &points[i];
        p->x = p->x * e->r.width + e->r.x;
        p->y = (1 - p->y) * e->r.height + e->r.y;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointCircle(mouse, (Vector2){p->x, p->y}, point_radius_big)) {
                e->point_drag = i;
            }
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            e->point_drag = -1;
        }
    }

    if (e->point_drag >= 0) {
        Vector2 p = mouse;
        e->points[e->point_drag].x = (p.x - e->r.x) / e->r.width;
        e->points[e->point_drag].y = 1 - ((p.y - e->r.y) / e->r.height);
    }

    float dt = GetFrameTime();
    e->b += dt;
    if (e->b >= 1.0f) {
        e->b = 1.0f;
    }

    if (IsKeyPressed(KEY_SPACE)) {
        e->b = 0.0f;
    }
}

void draw_animated_square_in_editor(const Editor *e) {
    float off = 50.0f;
    float sz = 25.0f;
    float x = e->r.x + e->r.width + off;
    float start_y = e->r.y + e->r.height - sz;
    float end_y = e->r.y;
    DrawRectangle(x, Lerp(start_y, end_y, e->b), sz, sz, YELLOW);
}

void draw_editor(const Editor *e) {
    DrawRectangleLinesEx(e->r, e->thick, e->background_col);
    for (int i = 1; i < 5; i++) {
        float x = e->r.x + e->r.width * ((float)i/5);
        float y = e->r.y + e->r.height * ((float)i/5);
        DrawLineEx((Vector2){x, e->r.y}, (Vector2){x, e->r.y + e->r.height}, e->thick, e->background_col);
        DrawLineEx((Vector2){e->r.x, y}, (Vector2){e->r.x + e->r.width, y}, e->thick, e->background_col);
    }

    for (int i = 0; i < 4; i += 2) {
        Vector2 p1 = e->points[i];
        Vector2 p2 = e->points[i+1];
        p1.x = p1.x * e->r.width + e->r.x;
        p1.y = (1 - p1.y) * e->r.height + e->r.y;
        p2.x = p2.x * e->r.width + e->r.x;
        p2.y = (1 - p2.y) * e->r.height + e->r.y;
        DrawLineEx(p1, p2, e->thick, e->line_col1);
    }

    Vector2 points[4];
    for (int i = 0; i < 4; i++) {
        points[i] = e->points[i];
        Vector2 *p = &points[i];
        p->x = p->x * e->r.width + e->r.x;
        p->y = (1 - p->y) * e->r.height + e->r.y;
        DrawCircleV(*p, point_radius, e->point_col);
    }
    DrawSplineBezierCubic(points, 4, e->thick, e->line_col);
    draw_animated_square_in_editor(e);
}

int main(void) {
    ConfigFlags window_flags = FLAG_WINDOW_RESIZABLE;

    InitWindow(screen_width, screen_height, "bezier curve animation");
    SetWindowState(window_flags);
    SetTargetFPS(60);

    point_col = ColorFromHSV(57, .78f, .87f);
    point_col1 = ColorFromHSV(254, .78f, .87f);
    point_col2 = ColorFromHSV(0, 1.f, 1.f);
    Color background_col = (Color){0x18, 0x18, 0x18, 0xff};

    int font_sz = 32;

    Font font = LoadFontEx("assets/Alegreya-Regular.ttf", font_sz, nil, nil);
    ContextMenu context_menu = {
        .font = &font,
        .sz = font.baseSize,
        .col = WHITE,
        .padding = 10,
    };

    ContextMenu interactive_menu = new_context_menu(&context_menu, CONTEXT_MENU_INTERACTIVE, "increase", "decrease", "reset", "editor");
    ContextMenu editor_menu = new_context_menu(&context_menu, CONTEXT_MENU_EDITOR, "interactive");
    ContextMenu *curr_menu = &interactive_menu;

    APP_STATE app_state = APP_EDITOR;
    
    Interactive interactive = {
    	.last_curve_point_index = -1,
    	.curr_curve_point_index = -1,
    	.b = 0.0f,
        .animate = false,
    };
    init_cubic_curve(&interactive.bc);
    
    Editor editor = {
        .max_sz = 700,
        .thick = 2,
        .background_col = (Color){0x2d, 0x2d, 0x2d, 0xff},
        .point_col = YELLOW,
        .line_col = YELLOW,
        .line_col1 = (Color){0x80, 0x80, 0x80, 0xff},
        .point_drag = -1,
        .points = {
            (Vector2){ 0.0f, 0.0f },
            (Vector2){ 0.2f, 0.2f },
            (Vector2){ 0.8f, 0.8f },
            (Vector2){ 1.0f, 1.0f },
        },
    };

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        if (IsWindowResized()) {
            screen_width = GetScreenWidth();
            screen_height = GetScreenHeight();
        }
        if (IsKeyPressed(KEY_F11)) {
            window_flags ^= FLAG_BORDERLESS_WINDOWED_MODE;
            SetWindowState(window_flags);
            screen_width = GetScreenWidth();
            screen_height = GetScreenHeight();
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !curr_menu->active) {
            invoke_context_menu(curr_menu);
        } else {
            if (!CheckCollisionPointRec(mouse, curr_menu->bound)) {
                curr_menu->active = false;
            }
        }

        switch (app_state) {
        case APP_INTERACTIVE: {
            if (IsWindowResized() || IsKeyPressed(KEY_F11)) {
                nil_b_in_interactive(&interactive);
            }

            if (curr_menu->active) {
                update_context_menu(curr_menu);

                // TODO(kra53n): show points num in context menu
                if (curr_menu->type == CONTEXT_MENU_INTERACTIVE && curr_menu->choosed >= 0) {
                    if (strcmp(curr_menu->options[curr_menu->choosed], "editor") == 0) {
                        app_state = APP_EDITOR;
                        curr_menu = &editor_menu;
                    } else if (strcmp(curr_menu->options[curr_menu->choosed], "reset") == 0) {
                        for (int i = 0; i < interactive.bc.n; i++) {
                            interactive.bc.points[i].x = (i / (float)(interactive.bc.n-1)) * 0.7 + 0.15;
                            interactive.bc.points[i].y = 0.5f;
                        }
                        nil_b_in_interactive(&interactive);
                    } else if (strcmp(curr_menu->options[curr_menu->choosed], "increase") == 0) {
                        if (interactive.bc.n+1 != 20) {
                            int idx = get_max_distance_idx_beetween_vecs(interactive.bc.points, interactive.bc.n) + 1;
                            memmove(interactive.bc.points+(idx+1), interactive.bc.points+(idx), sizeof(Vector2)*(interactive.bc.n-idx+1));
                            interactive.bc.points[idx].x = (interactive.bc.points[idx-1].x + interactive.bc.points[idx+1].x) / 2;
                            interactive.bc.points[idx].y =  (interactive.bc.points[idx-1].y + interactive.bc.points[idx+1].y) / 2;
                            interactive.bc.n++;
                            interactive.bc.edges = count_curve_edges(interactive.bc.n);
                            nil_b_in_interactive(&interactive);
                        }
                        keep_context_menu(curr_menu);
                    } else if (strcmp(curr_menu->options[curr_menu->choosed], "decrease") == 0) {
                        if (interactive.bc.n >= 3) {
                            int idx = get_min_distance_idx_beetween_vecs(interactive.bc.points, interactive.bc.n);
                            if (idx == 0) {
                                idx = 1;
                            }
                            memmove(interactive.bc.points+(idx), interactive.bc.points+(idx+1), sizeof(Vector2)*(interactive.bc.n-idx));
                            interactive.bc.n--;
                            interactive.bc.edges = count_curve_edges(interactive.bc.n);
                            nil_b_in_interactive(&interactive);
                        }
                        keep_context_menu(curr_menu);
                    }
                }
            }

            update_interactive(&interactive);

            BeginDrawing();
            {
                ClearBackground(background_col);

                draw_interactive(&interactive);
                if (curr_menu->active) {
                    // TODO(kra53): blur and shadow background
                    draw_context_menu(curr_menu);
                }
            }
            EndDrawing();
        } break;

        case APP_EDITOR: {
            if (curr_menu->active) {
                update_context_menu(curr_menu);

                if (curr_menu->type == CONTEXT_MENU_EDITOR && curr_menu->choosed >= 0) {
                    if (strcmp(curr_menu->options[curr_menu->choosed], "interactive") == 0) {
                        app_state = APP_INTERACTIVE;
                        curr_menu = &interactive_menu;
                    }
                }
            }

            update_editor(&editor);

            BeginDrawing();
            {
                ClearBackground(background_col);
                draw_editor(&editor);
                if (curr_menu->active) {
                    draw_context_menu(curr_menu);
                }
            }
            EndDrawing();
        } break;

        }
    }
    CloseWindow();

    return 0;
}
