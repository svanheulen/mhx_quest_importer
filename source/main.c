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
#include <unistd.h>


#define SYSTEM_SIZE 3709739

const int challenge_quest_ids[] = {
    1020001, 1020002, 1020003, 1020004, 1020005, 1020006, 1020007, 1020008, 1020009, 1020010,
    1020011, 1020012, 1020013, 1020014, 1120001, 1120002, 1120003, 1120004, 1120005, 1120006
};

const int event_quest_ids[] = {
    1010001, 1010002, 1010003, 1010004, 1010101, 1010102, 1010103, 1010104, 1010105, 1010106,
    1010107, 1010108, 1010109, 1010110, 1010111, 1010112, 1010113, 1010114, 1010115, 1010116,
    1010117, 1010118, 1010119, 1010120, 1010121, 1010122, 1010123, 1010124, 1010125, 1010126,
    1010127, 1010128, 1010129, 1010130, 1010131, 1010132, 1010133, 1010134, 1010135, 1010136,
    1010137, 1010138, 1010139, 1010140, 1010141, 1010142, 1010143, 1010144, 1010145, 1010146,
    1010147, 1010148, 1010149, 1010150, 1010151, 1010152, 1010153, 1010154, 1010155, 1010156,
    1010157, 1010158, 1010159, 1010160, 1010161, 1010162, 1010163, 1010164, 1010165, 1010166,
    1010167, 1110001, 1110002, 1110003, 1110004, 1110005, 1110006, 1110101, 1110102, 1110103,
    1110104, 1110105, 1110106, 1110107, 1110108, 1110109, 1110110, 1110111, 1010005, 1010006,
    1010007, 1010008, 1010168, 1010169, 1010170, 1010171, 1010172, 1010173, 1010174, 1010175,
    1010176, 1010177, 1010178, 1010179, 1010180, 1010181, 1010182, 1010183, 1010184, 1010185,
    1010186, 1010187, 1110007, 1110008, 1110009, 1110112, 1110113, 1110114, 1110115, 1110116
};


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

void find_file_ids(int * ids) {
    char file_name[50];
    for (int i = 0; i < 20; i++) {
        sprintf(file_name, "import/q%07d.arc", challenge_quest_ids[i]);
        if (access(file_name, R_OK) == -1)
            ids[i] = 0;
        else
            ids[i] = challenge_quest_ids[i];
    }
    for (int i = 0; i < 120; i++) {
        sprintf(file_name, "import/q%07d.arc", event_quest_ids[i]);
        if (access(file_name, R_OK) == -1)
            ids[20 + i] = 0;
        else
            ids[20 + i] = event_quest_ids[i];
    }
}

void get_ids(int * bits, int * ids) {
    for (int i = 0; i < 140; i++) {
        if (((bits[0] >> i) & 1) == 0)
            ids[i] = 0;
        else
            ids[i] = challenge_quest_ids[i];
    }
    for (int i = 0; i < 120; i++) {
        if (((bits[(i/32)+1] >> (i%32)) & 1) == 0)
            ids[20 + i] = 0;
        else
            ids[20 + i] = event_quest_ids[i];
    }
}

void get_bits(int * bits, int * ids) {
    for (int i = 0; i < 5; i++)
        bits[i] = 0;
    for (int i = 0; i < 20; i++) {
        if (ids[i] != 0)
            bits[0] |= 1 << i;
    }
    for (int i = 0; i < 120; i++) {
        if (ids[20 + i] != 0)
            bits[(i/32)+1] |= 1 << (i%32);
    }
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
    int section_offset;
    FSFILE_Read(fh, NULL, 0xc, &section_offset, 4);
    int quest_offset = section_offset + 0x45d4;
    int quest_id;
    int quest_size;
    char quest_data[0x1400];
    char file_name[50];
    FILE * export;
    for (int i = 0; i < 140; i++) {
        FSFILE_Read(fh, NULL, quest_offset, &quest_id, 4);
        FSFILE_Read(fh, NULL, quest_offset + 4, &quest_size, 4);
        if (quest_id != 0) {
            sprintf(file_name, "export/q%07d.arc", quest_id);
            FSFILE_Read(fh, NULL, quest_offset + 8, quest_data, quest_size);
            printf("    ");
            printf(file_name);
            printf(" ... ");
            refresh();
            export = fopen(file_name, "wb");
            if (export != NULL) {
                fwrite(quest_data, 1, quest_size, export);
                fclose(export);
                printf("\x1b[32;1msuccess.\x1b[0m\n");
                refresh();
            } else {
                printf("\x1b[31;1mfailure.\x1b[0m\n");
                refresh();
            }
        }
        quest_offset += 0x1400;
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
    int section_offset;
    FSFILE_Read(fh, NULL, 0xc, &section_offset, 4);
    int installed_bits[5];
    FSFILE_Read(fh, NULL, section_offset + 0x34, installed_bits, 20);
    int installed_ids[140];
    get_ids(installed_bits, installed_ids);
    int file_ids[140];
    find_file_ids(file_ids);
    int quest_offset = section_offset + 0x45d4;
    int quest_id;
    int quest_size;
    char quest_data[0x1400];
    char file_name[50];
    FILE * import;
    for (int i = 0; i < 140; i++) {
        FSFILE_Read(fh, NULL, quest_offset, &quest_id, 4);
        for (int j = 0; j < 140; j++) {
            int check = quest_id != 0 && file_ids[j] == quest_id;
            check = check || (quest_id == 0 && file_ids[j] != 0 && i < 120 && ((file_ids[j] > 1010000 && file_ids[j] < 1020000) || (file_ids[j] > 1110000 && file_ids[j] < 1120000)));
            check = check || (quest_id == 0 && file_ids[j] != 0 && i >= 120 && ((file_ids[j] > 1020000 && file_ids[j] < 1110000) || file_ids[j] > 1120000));
            if (check) {
                sprintf(file_name, "import/q%07d.arc", file_ids[j]);
                printf("    ");
                printf(file_name);
                printf(" ... ");
                refresh();
                import = fopen(file_name, "rb");
                if (import != NULL) {
                    quest_size = fread(quest_data, 1, 0x1400, import);
                    fclose(import);
                    FSFILE_Write(fh, NULL, quest_offset, &file_ids[j], 4, FS_WRITE_FLUSH);
                    FSFILE_Write(fh, NULL, quest_offset + 4, &quest_size, 4, FS_WRITE_FLUSH);
                    FSFILE_Write(fh, NULL, quest_offset + 8, quest_data, quest_size, FS_WRITE_FLUSH);
                    installed_ids[j] = file_ids[j];
                    printf("\x1b[32;1msuccess.\x1b[0m\n");
                    refresh();
                } else {
                    printf("\x1b[31;1mfailure.\x1b[0m\n");
                    refresh();
                }
                file_ids[j] = 0;
                break;
            }
        }
        quest_offset += 0x1400;
    }
    get_bits(installed_bits, installed_ids);
    FSFILE_Write(fh, NULL, section_offset + 0x34, installed_bits, 20, FS_WRITE_FLUSH);
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

