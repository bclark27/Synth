#include "ConfigParser.h"

#define COMMAND_CONNECT "connect"
#define COMMAND_CONTROL "control"
#define SPLIT           "+++"

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

#define LINEBUF_SIZE 512

bool ConfigParser_Parse(FullConfig* cfg, char* fname)
{
    if (!cfg || !fname) return 0;

    FILE* f = fopen(fname, "r");
    if (!f) {
        perror(fname);
        return false;
    }

    memset(cfg, 0, sizeof(*cfg));

    char line[LINEBUF_SIZE];
    int inModuleSection = 1;
    ModuleConfig* current = NULL;

    while (fgets(line, sizeof(line), f))
    {
        // Strip newline characters
        line[strcspn(line, "\r\n")] = 0;

        // Skip blank or comment lines
        if (line[0] == '\0' || line[0] == '#')
            continue;

        // Handle section break
        if (strcmp(line, SPLIT) == 0)
        {
            if (inModuleSection)
                inModuleSection = 0;   // switch from module list to module configs
            else
                current = NULL;         // end of one module block
            continue;
        }

        // Phase 1: module declarations
        if (inModuleSection)
        {
            if (cfg->moduleCount >= MAX_RACK_SIZE) {
                fprintf(stderr, "ConfigParser: too many modules (max %d)\n", MAX_RACK_SIZE);
                break;
            }

            ModuleConfig* m = &cfg->modules[cfg->moduleCount++];
            memset(m, 0, sizeof(ModuleConfig));

            if (sscanf(line, "%31s %31s", m->name, m->type) != 2) {
                fprintf(stderr, "ConfigParser: bad module line: '%s'\n", line);
                continue;
            }

            continue;
        }

        // Phase 2: module definitions
        char word[32];
        if (sscanf(line, "%31s", word) != 1)
            continue;

        // Starting a new module block
        if (!current)
        {
            current = NULL;
            for (int i = 0; i < cfg->moduleCount; i++)
            {
                if (strcmp(cfg->modules[i].name, word) == 0)
                {
                    current = &cfg->modules[i];
                    break;
                }
            }

            if (!current)
            {
                fprintf(stderr, "ConfigParser: unknown module name '%s'\n", word);
            }

            continue;
        }

        // if current module is still null, skip
        if (!current)
            continue;

        if (strcmp(word, COMMAND_CONNECT) == 0)
        {
            ConnectionInfo c;
            if (sscanf(line, "%*s %31s %31s %31s", c.inPort, c.otherModule, c.otherOutPort) == 3)
            {
                if (current->connectionCount < (int)(sizeof(current->connections) / sizeof(current->connections[0])))
                    current->connections[current->connectionCount++] = c;
                else
                    fprintf(stderr, "ConfigParser: too many connections for module %s\n", current->name);
            }
            else {
                fprintf(stderr, "ConfigParser: bad connect line: '%s'\n", line);
            }
        }
        else if (strcmp(word, COMMAND_CONTROL) == 0)
        {
            ControlInfo ctrl;
            memset(&ctrl, 0, sizeof(ctrl)); // recommended safety

            // First: try to match a quoted string.
            // Format:  control <name> "<string>"
            char tmpString[MAX_BYTE_ARR + 1];
            if (sscanf(line, "%*s %31s \"%149[^\"]\"", ctrl.controlName, tmpString) == 2)
            {
                ctrl.type = ModulePortType_ByteArrayControl; 
                memcpy(ctrl.value.bytes, tmpString, MAX_BYTE_ARR);
                ctrl.valueLen = MIN(MAX_BYTE_ARR, strlen(tmpString));
                current->controls[current->controlCount++] = ctrl;
            }
            // Otherwise: try to parse float
            else if (sscanf(line, "%*s %31s %f", ctrl.controlName, &ctrl.value.volt) == 2)
            {
                ctrl.type = ModulePortType_VoltControl;
                current->controls[current->controlCount++] = ctrl;
            }
            // Neither matched → error
            else {
                fprintf(stderr, "ConfigParser: bad control line: '%s'\n", line);
            }
        }
        else {
            fprintf(stderr, "ConfigParser: unknown command in '%s'\n", line);
        }
    }

    fclose(f);

    return true;
}

void ConfigParser_Write(FullConfig* cfg, char* fname)
{
    if (!cfg || !fname) return;

    FILE* f = fopen(fname, "w");
    if (!f) {
        perror("fopen");
        return;
    }

    // --- Write the module name/type section ---
    for (int i = 0; i < cfg->moduleCount; i++) {
        ModuleConfig* mod = &cfg->modules[i];
        fprintf(f, "%s %s\n", mod->name, mod->type);
    }
    fprintf(f, "%s\n", SPLIT); // +++

    // --- Write each module’s details ---
    for (int i = 0; i < cfg->moduleCount; i++) {
        ModuleConfig* mod = &cfg->modules[i];

        fprintf(f, "%s\n", mod->name);

        // Connections
        for (int j = 0; j < mod->connectionCount; j++) {
            ConnectionInfo* c = &mod->connections[j];
            fprintf(f, "%s %s %s %s\n",
                    COMMAND_CONNECT,
                    c->inPort,
                    c->otherModule,
                    c->otherOutPort);
        }

        // Controls
        for (int j = 0; j < mod->controlCount; j++) {
            ControlInfo* ctrl = &mod->controls[j];
            switch (ctrl->type)
            {
                case ModulePortType_VoltControl:
                {
                    fprintf(f, "%s %s %f\n",
                            COMMAND_CONTROL,
                            ctrl->controlName,
                            ctrl->value.volt);
                    break;
                }
                case ModulePortType_ByteArrayControl:
                {
                    char tmp[MAX_BYTE_ARR + 1];
                    memcpy(tmp, ctrl->value.bytes, ctrl->valueLen);
                    tmp[ctrl->valueLen] = 0;
                    fprintf(f, "%s %s \"%s\"\n",
                            COMMAND_CONTROL,
                            ctrl->controlName,
                            tmp);
                    break;
                }
                default:
                {

                }
            }
        }

        fprintf(f, "%s\n", SPLIT); // +++
    }

    fclose(f);
}

void ConfigParser_Print(FullConfig* cfg)
{
    printf("=== CONFIG ===\n");
    printf("Module count: %d\n\n", cfg->moduleCount);

    for (int i = 0; i < cfg->moduleCount; i++)
    {
        ModuleConfig* m = &cfg->modules[i];
        printf("[%d] %s (%s)\n", i, m->name, m->type);

        // Connections
        for (int j = 0; j < m->connectionCount; j++)
        {
            ConnectionInfo* c = &m->connections[j];
            printf("  connect %-8s <- %-8s.%s\n",
                   c->inPort, c->otherModule, c->otherOutPort);
        }

        // Controls
        for (int j = 0; j < m->controlCount; j++)
        {
            ControlInfo* ctrl = &m->controls[j];
            printf("  control %-8s = %.3f\n",
                   ctrl->controlName, ctrl->value.volt);
        }

        printf("\n");
    }
}
