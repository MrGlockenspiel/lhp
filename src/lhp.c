#include <fcgiapp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

// i hate you alpine linux i hate you
static char *lhp_strdup(const char *s) {
    if (!s) {
        return NULL;
    }
    size_t len = strlen(s);
    char *p = malloc(len + 1);
    if (!p) {
        return NULL;
    }
    memcpy(p, s, len);
    p[len] = '\0';
    return p;
}

// print() replacement
static int lhp_lua_print(lua_State *L) {
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        const char *str = lua_tostring(L, i);
        if (str) {
            FCGX_PutStr(str, strlen(str),
                        (FCGX_Stream *)lua_touserdata(L, lua_upvalueindex(1)));
            FCGX_PutChar('\n',
                         (FCGX_Stream *)lua_touserdata(L, lua_upvalueindex(1)));
        }
    }
    FCGX_PutStr("\n", 1, (FCGX_Stream *)lua_touserdata(L, lua_upvalueindex(1)));
    return 0;
}

static char *lhp_dirname(const char *path) {
    const char *slash = NULL;
    const char *p = path;

    while (*p) {
        if (*p == '/') {
            slash = p;
        }
        p++;
    }

    // no slash
    if (!slash) {
        return lhp_strdup(".");
    }

    // root "/"
    if (slash == path) {
        return lhp_strdup("/");
    }

    size_t len = (size_t)(slash - path);
    char *dir = malloc(len + 1);
    if (!dir) {
        return NULL;
    }

    memcpy(dir, path, len);
    dir[len] = '\0';
    return dir;
}

// set package.path
static void lhp_set_package_path(lua_State *L, const char *script_path) {
    char *path = lhp_strdup(script_path);
    if (!path) {
        return;
    }

    char *dir = lhp_dirname(path);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");

    const char *old_path = lua_tostring(L, -1);
    if (!old_path) {
        old_path = "";
    }

    size_t len =
        strlen(dir) * 2 + strlen(old_path) + strlen("/?.lua;/?/init.lua;;") + 1;

    char *new_path = malloc(len);
    if (!new_path) {
        free(path);
        lua_pop(L, 2);
        return;
    }

    snprintf(new_path, len, "%s/?.lua;%s/?/init.lua;%s", dir, dir, old_path);

    lua_pop(L, 1);
    lua_pushstring(L, new_path);
    lua_setfield(L, -2, "path");

    lua_pop(L, 1);

    free(new_path);
    free(path);
    free(dir);
}

void lhp_process_file(const char *path, FCGX_Stream *out) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        FCGX_FPrintF(out, "Status: 404 Not Found\r\n\r\n");
        return;
    }

    // read entire file
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    if (size <= 0) {
        fclose(fp);
        return;
    }

    char *buf = malloc(size + 1);
    if (!buf) {
        fclose(fp);
        return;
    }

    fread(buf, 1, size, fp);
    buf[size] = 0;
    fclose(fp);

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    // override print()
    lua_pushlightuserdata(L, out);
    lua_pushcclosure(L, lhp_lua_print, 1);
    lua_setglobal(L, "print");

    lua_pushstring(L, __DATE__ " " __TIME__);
    lua_setglobal(L, "_LHP_BUILD_DATE");

#ifdef __VERSION__
    lua_pushstring(L, __VERSION__);
#elif
    lua_pushstring(L, "(null)");
#endif
    lua_setglobal(L, "_LHP_COMPILER_VERSION");

    lhp_set_package_path(L, path);

    const char *p = buf;

    while (1) {
        const char *start = strstr(p, "<?lua");
        if (!start) {
            FCGX_PutStr(p, strlen(p), out);
            break;
        }

        // emit html before Lua block
        FCGX_PutStr(p, start - p, out);

        // skip '<?lua'
        start += 5;

        const char *end = strstr(start, "?>");
        if (!end) {
            FCGX_FPrintF(out, "\n<!-- lhp parse error: missing '?>' -->\n");
            break;
        }

        size_t len = (size_t)(end - start);

        char *code = malloc(len + 1);
        if (!code) {
            FCGX_FPrintF(out, "\n<!-- lhp memory allocation failure -->\n");
            break;
        }

        memcpy(code, start, len);
        code[len] = 0;

        if (luaL_dostring(L, code)) {
            const char *err = lua_tostring(L, -1);
            FCGX_FPrintF(out, "<!-- lua error: %s -->", err);
            lua_pop(L, 1);
        }

        free(code);

        p = end + 2;
    }

    lua_close(L);
    free(buf);
}

int main(void) {
    FCGX_Request req;

    FCGX_Init();
    FCGX_InitRequest(&req, 0, 0);

    while (FCGX_Accept_r(&req) == 0) {
        FCGX_Stream *out = req.out;
        char *script = FCGX_GetParam("SCRIPT_FILENAME", req.envp);

        if (!script) {
            FCGX_FPrintF(out, "Status: 500\r\nContent-Type: "
                              "text/plain\r\n\r\nNo SCRIPT_FILENAME");
            // dont care
            goto end_req;
        }

        FCGX_FPrintF(out, "Content-Type: text/html\r\n\r\n");
        lhp_process_file(script, out);

    end_req:
        FCGX_Finish_r(&req);
    }

    return 0;
}
