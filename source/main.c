/*
Copyright 2016 Seth VanHeulen

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <3ds.h>

#include <stdlib.h>
#include <stdio.h>

#include <dirent.h>

#define SYSTEM_SIZE 3709739

void refresh() {
    gspWaitForVBlank();
    gfxFlushBuffers();
    gfxSwapBuffers();
}

void display_menu() {
    consoleClear();
    printf("Monster Hunter X Quest Importer\n\n");
    printf("Press (A) to extract all quests\n");
    printf("Press (X) to import all quests\n");
    printf("Press (Y) to backup save file\n");
    printf("Press (B) to return to Homebrew Launcher\n");
    refresh();
}

void extract_quests() {
    consoleClear();
    printf("Extracting quests ...\n");
    refresh();
    int archive_lowpath_data[3] = {MEDIATYPE_SD, 0x1554, 0};
    FS_Archive archive = {ARCHIVE_EXTDATA, {PATH_BINARY, 0xC, (char *) archive_lowpath_data}};
    if (FSUSER_OpenArchive(&archive) != 0) {
        printf("\x1b[31;1mFailed to open ExtData archive.\x1b[0m");
        refresh();
        return;
    }
    Handle fh;
    if (FSUSER_OpenFile(&fh, archive, fsMakePath(PATH_ASCII, "/system"), FS_OPEN_READ, 0) != 0) {
        FSUSER_CloseArchive(&archive);
        printf("\x1b[31;1mFailed to open system file.\x1b[0m");
        refresh();
        return;
    }
    int offset;
    int quest_info[2];
    char quest_data[0x1400];
    char file_name[50];
    FSFILE_Read(fh, NULL, 0xc, &offset, 4);
    offset += 0x45d4;
    for (int i = 0; i < 140; i++) {
        FSFILE_Read(fh, NULL, offset, quest_info, 8);
        if (quest_info[0] != 0) {
            sprintf(file_name, "export/q%07d.arc", quest_info[0]);
            FSFILE_Read(fh, NULL, offset + 8, quest_data, quest_info[1]);
            printf("    ");
            printf(file_name);
            printf(" ... ");
            refresh();
            FILE * export = fopen(file_name, "wb");
            if (export != NULL) {
                fwrite(quest_data, 1, quest_info[1], export);
                fclose(export);
                printf("\x1b[32;1msuccess.\x1b[0m\n");
                refresh();
            } else {
                printf("\x1b[31;1mfailure.\x1b[0m\n");
                refresh();
            }
        }
        offset += 0x1400;
    }
    FSFILE_Close(fh);
    FSUSER_CloseArchive(&archive);
    printf("Complete.\n");
    refresh();
}

void import_quests() {
    consoleClear();
    printf("Importing quests ...\n");
    refresh();
    int file_ids[140];
    DIR * import_dir = opendir("import/");
    if (import_dir == NULL) {
        printf("\x1b[31;1mFailed to open import directory.\x1b[0m");
        refresh();
        return;
    }
    for (int i = 0; i < 140; i++) {
        struct dirent* ent = readdir(import_dir);
        if (ent == NULL)
            file_ids[i] = 0;
        else
            file_ids[i] = atoi(ent->d_name + 1);
    }
    closedir(import_dir);
    int archive_lowpath_data[3] = {MEDIATYPE_SD, 0x1554, 0};
    FS_Archive archive = {ARCHIVE_EXTDATA, {PATH_BINARY, 0xC, (char *) archive_lowpath_data}};
    if (FSUSER_OpenArchive(&archive) != 0) {
        printf("\x1b[31;1mFailed to open ExtData archive.\x1b[0m");
        refresh();
        return;
    }
    Handle fh;
    if (FSUSER_OpenFile(&fh, archive, fsMakePath(PATH_ASCII, "/system"), FS_OPEN_WRITE, 0) != 0) {
        FSUSER_CloseArchive(&archive);
        printf("\x1b[31;1mFailed to open system file.\x1b[0m");
        refresh();
        return;
    }
    int offset;
    int quest_id;
    char quest_data[0x1400];
    char file_name[50];
    FSFILE_Read(fh, NULL, 0xc, &offset, 4);
    offset += 0x45d4;
    for (int i = 0; i < 140; i++) {
        FSFILE_Read(fh, NULL, offset, &quest_id, 4);
        for (int j = 0; j < 140; j++) {
            int check = quest_id != 0 && file_ids[j] == quest_id;
            check = check || (quest_id == 0 && file_ids[j] != 0 && i < 120 && ((quest_id > 1010000 && quest_id < 1020000) || (quest_id > 1110000 && quest_id < 1120000)));
            check = check || (quest_id == 0 && file_ids[j] != 0 && i >= 120 && ((quest_id > 1020000 && quest_id < 1110000) || quest_id > 1120000));
            if (check) {
                sprintf(file_name, "import/q%07d.arc", file_ids[j]);
                printf("    ");
                printf(file_name);
                printf(" ... ");
                refresh();
                FILE * import = fopen(file_name, "rb");
                if (import != NULL) {
                    int quest_size = fread(quest_data, 1, 0x1400, import);
                    fclose(import);
                    FSFILE_Write(fh, NULL, offset, &file_ids[j], 4, FS_WRITE_FLUSH);
                    FSFILE_Write(fh, NULL, offset + 4, &quest_size, 4, FS_WRITE_FLUSH);
                    FSFILE_Write(fh, NULL, offset + 8, quest_data, quest_size, FS_WRITE_FLUSH);
                    printf("\x1b[32;1msuccess.\x1b[0m\n");
                    refresh();
                } else {
                    printf("\x1b[31;1mfailure.\x1b[0m\n");
                    refresh();
                }
            }
            file_ids[j] = 0;
        }
        offset += 0x1400;
    }
    FSFILE_Close(fh);
    FSUSER_CloseArchive(&archive);
    printf("Complete.\n");
    refresh();
}

void backup_save() {
    consoleClear();
    printf("Backing up save file ...\n");
    refresh();
    char * buffer = malloc(SYSTEM_SIZE);
    if (buffer == NULL) {
        printf("\x1b[31;1mFailed to allocate memory.\x1b[0m");
        refresh();
        return;
    }
    int archive_lowpath_data[3] = {MEDIATYPE_SD, 0x1554, 0};
    FS_Archive archive = {ARCHIVE_EXTDATA, {PATH_BINARY, 0xC, (char *) archive_lowpath_data}};
    if (FSUSER_OpenArchive(&archive) != 0) {
        free(buffer);
        printf("\x1b[31;1mFailed to open ExtData archive.\x1b[0m");
        refresh();
        return;
    }
    Handle fh;
    if (FSUSER_OpenFile(&fh, archive, fsMakePath(PATH_ASCII, "/system"), FS_OPEN_READ, 0) != 0) {
        free(buffer);
        FSUSER_CloseArchive(&archive);
        printf("\x1b[31;1mFailed to open system file.\x1b[0m");
        refresh();
        return;
    }
    printf("    backup/system ... ");
    refresh();
    FSFILE_Read(fh, NULL, 0, buffer, SYSTEM_SIZE);
    FILE * output = fopen("backup/system", "wb");
    if (output != NULL) {
        fwrite(buffer, 1, SYSTEM_SIZE, output);
        fclose(output);
        printf("\x1b[32;1msuccess.\x1b[0m\n");
        refresh();
    } else {
        printf("\x1b[31;1mfailure.\x1b[0m\n");
        refresh();
    }
    free(buffer);
    FSFILE_Close(fh);
    FSUSER_CloseArchive(&archive);
    printf("Complete.\n");
    refresh();
}

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    display_menu();
    while (aptMainLoop()) {
        hidScanInput();
        unsigned int kDown = hidKeysDown();
        if (kDown & KEY_A) {
            extract_quests();
            svcSleepThread(5000000000);
            display_menu();
        } else if (kDown & KEY_X) {
            import_quests();
            svcSleepThread(5000000000);
            display_menu();
        } else if (kDown & KEY_Y) {
            backup_save();
            svcSleepThread(5000000000);
            display_menu();
        } else if (kDown & KEY_B) {
            break;
        }
    }
    gfxExit();
    return 0;
}

